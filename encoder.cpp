#include <cstdlib>
// emscripten
#include <emscripten/bind.h>
#include <emscripten/val.h>
// libvpx
#include "vpxenc.h"
#include "vpx/vp8cx.h"
// libwebm
#include "mkvmuxer.hpp"
#include "mkvwriter.hpp"
// libyuv
#include "libyuv.h"

using namespace emscripten;
using namespace mkvmuxer;

// Globals
std::string last_error;
uint8_t *buffer = NULL;

int version() {
  return VPX_CODEC_ABI_VERSION;
}

void generate_frame(uint8_t *img, int frame) {
  int stride = 300 * 4;
  auto *row = img;

  row += stride * frame;

  for(int y = 0; y < 10; y++) {
    for(int x = 0; x < 10; x++) {
      row[(frame + x) * 4 + 1 + (frame % 3)] = 255;
    }
    row += stride;
  }
}

bool encode_frame(vpx_codec_ctx_t *ctx, vpx_image_t *img, Segment *segment, int frame_cnt) {
  vpx_codec_iter_t iter = NULL;
  const vpx_codec_cx_pkt_t *pkt;
  vpx_codec_err_t err;

  err = vpx_codec_encode(
    ctx,
    img,
    frame_cnt, /* time of frame */
    1, /* length of frame */
    0, /* flags. Use VPX_EFLAG_FORCE_KF to force a keyframe. */
    0 /* deadline. 0 = best quality, 1 = realtime */
  );
  if(err != VPX_CODEC_OK) {
    last_error = std::string(vpx_codec_err_to_string(err));
    return false;
  }
  while((pkt = vpx_codec_get_cx_data(ctx, &iter)) != NULL) {
    if(pkt->kind != VPX_CODEC_CX_FRAME_PKT) {
      continue;
    }
    auto frame_size = pkt->data.frame.sz;
    auto is_keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;
    if(!segment->AddFrame(
      (uint8_t*) pkt->data.frame.buf,
      pkt->data.frame.sz,
      1, /* track id */
      pkt->data.frame.pts * 1000000000ULL/30,
      is_keyframe
    )) {
      last_error = "Could not add frame";
      return false;
    }
  }
  return true;
}

int vpx_img_plane_width(const vpx_image_t *img, int plane) {
  if ((plane == 1 || plane == 2) && img->x_chroma_shift > 0 )
    return (img->d_w + 1) >> img->x_chroma_shift;
  else
    return img->d_w;
}

int vpx_img_plane_height(const vpx_image_t *img, int plane) {
  if ((plane == 1 || plane == 2) && img->y_chroma_shift > 0)
    return (img->d_h + 1) >> img->y_chroma_shift;
  else
    return img->d_h;
}

void clear_image(vpx_image_t *img) {
  for(int plane = 0; plane < 4; plane++) {
    auto *row = img->planes[plane];
    if(!row) {
      continue;
    }
    auto plane_width = vpx_img_plane_width(img, plane);
    auto plane_height = vpx_img_plane_height(img, plane);
    uint8_t value = plane == 3 ? 1 : 0;
    for(int y = 0; y < plane_height; y++) {
      memset(row, value, plane_width);
      row += img->stride[plane];
    }
  }
}

#define BUFFER_SIZE 8 * 1024 * 1024

val encode() {
  vpx_codec_ctx_t ctx;
  auto iface = vpx_codec_vp8_cx();
  vpx_codec_enc_cfg_t cfg;
  vpx_codec_err_t err;

  err = vpx_codec_enc_config_default(iface, &cfg, 0);
  if(err != VPX_CODEC_OK) {
    last_error = std::string(vpx_codec_err_to_string(err));
    return val::undefined();
  }

  cfg.g_timebase.num = 1;
  cfg.g_timebase.den = 30; // = fps
  cfg.g_w = 300;
  cfg.g_h = 300;
  cfg.rc_target_bitrate = 200; // FIXME

  err = vpx_codec_enc_init(
    &ctx,
    iface,
    &cfg,
    0 /* flags */
  );
  if(err != VPX_CODEC_OK) {
    last_error = std::string(vpx_codec_err_to_string(err));
    return val::undefined();
  }

  buffer = (uint8_t*) malloc(BUFFER_SIZE);
  if(buffer == NULL) {
    last_error = "Could not allocate buffer";
    return val::undefined();
  }
  memset(buffer, 0, BUFFER_SIZE);
  auto f = fmemopen(buffer, BUFFER_SIZE, "wb");
  auto mkv_writer = new MkvWriter(f);
  auto main_segment = new Segment();
  if(!main_segment->Init(mkv_writer)) {
    last_error = "Could not initialize main segment";
    return val::undefined();
  }
  if(main_segment->AddVideoTrack(cfg.g_w, cfg.g_h, 1 /* track id */) == 0) {
    last_error = "Could not add video track";
    return val::undefined();
  }
  main_segment->set_mode(Segment::Mode::kFile);
  auto info = main_segment->GetSegmentInfo();
  // BRANDING yo
  auto muxing_app = std::string(info->muxing_app()) + " but in wasm";
  auto writing_app = std::string(info->writing_app()) + " but in wasm";
  info->set_writing_app(writing_app.c_str());
  info->set_muxing_app(muxing_app.c_str());

  vpx_image_t *img = vpx_img_alloc(
    NULL, /* allocate buffer on the heap */
    VPX_IMG_FMT_I420,
    300,
    300,
    0 /* align. simple_encoder says 1? */
  );
  if(img == NULL) {
    last_error = "Could not allocate vpx image buffer";
    return val::undefined();
  }

  uint8_t img_buffer[300 * 300 * 4];
  for(int i = 0; i < 100; i++) {
    clear_image(img);
    memset(img_buffer, 0, 300 * 300 * 4);
    generate_frame(img_buffer, i);
    // litten endian BGRA means it is ARGB in memory
    if(
      libyuv::BGRAToI420(
        img_buffer,
        300*4,
        img->planes[VPX_PLANE_Y],
        img->stride[VPX_PLANE_Y],
        img->planes[VPX_PLANE_U],
        img->stride[VPX_PLANE_U],
        img->planes[VPX_PLANE_V],
        img->stride[VPX_PLANE_V],
        300,
        300
      ) != 0) {
        last_error = "Could not convert to I420";
        return val::undefined();
      }
    if(!encode_frame(&ctx, img, main_segment, i)) {
      last_error = "Could not encode frame";
      return val::undefined();
    }
  }
  vpx_img_free(img);

  // Flush
  if(!encode_frame(&ctx, NULL, main_segment, -1)) {
    last_error = "Could not encode flush frame";
    return val::undefined();
  }

  if(!main_segment->Finalize()) {
    last_error = "Could not finalize mkv";
    return val::undefined();
  }
  fflush(f);
  auto len = mkv_writer->Position();
  delete main_segment;
  delete mkv_writer;
  return val(typed_memory_view(len, buffer));
}

val last_error_desc() {
  return val(last_error);
}

void free_buffer() {
  free(buffer);
}

EMSCRIPTEN_BINDINGS(my_module) {
  function("version", &version);
  function("encode", &encode);
  function("free_buffer", &free_buffer);
  function("last_error_desc", &last_error_desc);
}
