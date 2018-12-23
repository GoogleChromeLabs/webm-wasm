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

using namespace emscripten;
using namespace mkvmuxer;

// Globals
vpx_codec_err_t err;
uint8_t *buffer = NULL;

int version() {
  return VPX_CODEC_ABI_VERSION;
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

void generate_frame(vpx_image_t *img, int frame) {
  auto *row = img->planes[VPX_PLANE_Y];

  clear_image(img);
  row += img->stride[VPX_PLANE_Y] * frame;

  for(int y = 0; y < 10; y++) {
    memset(row + frame, 127, 10);
    row += img->stride[VPX_PLANE_Y];
  }
}

bool encode_frame(vpx_codec_ctx_t *ctx, vpx_image_t *img, Segment *segment, int frame_cnt) {
  vpx_codec_iter_t iter = NULL;
  const vpx_codec_cx_pkt_t *pkt;
  err = vpx_codec_encode(
    ctx,
    img,
    frame_cnt, /* time of frame */
    1, /* length of frame */
    0, /* flags. Use VPX_EFLAG_FORCE_KF to force a keyframe. */
    0 /* deadline. 0 = best quality, 1 = realtime */
  );
  if(err != VPX_CODEC_OK) {
    return false;
  }
  while((pkt = vpx_codec_get_cx_data(ctx, &iter)) != NULL) {
    if(pkt->kind != VPX_CODEC_CX_FRAME_PKT) {
      continue;
    }
    auto frame_size = pkt->data.frame.sz;
    auto is_keyframe = (pkt->data.frame.flags & VPX_FRAME_IS_KEY) != 0;
    segment->AddFrame(
      (uint8_t*) pkt->data.frame.buf,
      pkt->data.frame.sz,
      1, /* track id */
      pkt->data.frame.pts * 1000000000ULL/30,
      is_keyframe
    );
  }
  return true;
}

#define BUFFER_SIZE 8 * 1024 * 1024

val encode() {
  vpx_codec_ctx_t ctx;
  auto iface = vpx_codec_vp8_cx();
  vpx_codec_enc_cfg_t cfg;

  err = vpx_codec_enc_config_default(iface, &cfg, 0);
  if(err != VPX_CODEC_OK) {
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
    return val::undefined();
  }

  buffer = (uint8_t*) malloc(BUFFER_SIZE);
  if(buffer == NULL) {
    return val::undefined();
  }
  memset(buffer, 0, BUFFER_SIZE);
  auto f = fmemopen(buffer, BUFFER_SIZE, "wb");
  auto mkv_writer = new MkvWriter(f);
  auto main_segment = new Segment();
  if(!main_segment->Init(mkv_writer)) {
    return val::undefined();
  }
  if(main_segment->AddVideoTrack(cfg.g_w, cfg.g_h, 1 /* track id */) == 0) {
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
    return val::undefined();
  }

  for(int i = 0; i < 100; i++) {
    generate_frame(img, i);
    if(!encode_frame(&ctx, img, main_segment, i)) {
      return val::undefined();
    }
  }
  vpx_img_free(img);

  // Flush
  if(!encode_frame(&ctx, NULL, main_segment, -1)) {
    return val::undefined();
  }

  if(!main_segment->Finalize()) {
    return val::undefined();
  }
  fflush(f);
  // delete main_segment;
  // delete mkv_writer;
  return val(typed_memory_view(mkv_writer->Position(), buffer));
}

val last_error() {
  return val(std::string(vpx_codec_err_to_string(err)));
}

void free_buffer() {
  free(buffer);
  // buffer = NULL;
  // buffer_size = 0;
}

EMSCRIPTEN_BINDINGS(my_module) {
  function("version", &version);
  function("encode", &encode);
  function("free_buffer", &free_buffer);
  function("last_error", &last_error);
}
