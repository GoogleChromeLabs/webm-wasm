#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <webmenc.h>
#include "vpxenc.h"
#include "vpx/vp8cx.h"

#include <cstdlib>

using namespace emscripten;

// Globals
vpx_codec_err_t err;
uint8_t *buffer = NULL;
int buffer_size = 0;

void grow_buffer(int delta) {
  buffer = (uint8_t*) realloc((void *)buffer, buffer_size + delta);
  buffer_size += delta;
}

int version() {
  return VPX_CODEC_ABI_VERSION;
}

void add_noise_in_y_plane(vpx_image_t *img) {
  uint8_t *row = img->planes[0];
  for(int y = 0; y < img->h; y++) {
    uint8_t *p = row;
    for(int x = 0; x < img->w; x++) {
      *p = rand();
      p++;
    }
    row += img->stride[0];
  }
}

#pragma pack(1)
struct ivf_frame_header {
  uint32_t length; // length of frame without this header
  uint64_t timestamp; //
};

bool encode_frame(vpx_codec_ctx_t *ctx, vpx_image_t *img, int frame_cnt) {
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
    grow_buffer(frame_size + 12);
    auto header = (struct ivf_frame_header *) &buffer[buffer_size - (frame_size + 12)];
    header->length = frame_size;
    header->timestamp = pkt->data.frame.pts;
    memcpy(&buffer[buffer_size - frame_size], pkt->data.frame.buf, frame_size);
  }
  return true;
}

#pragma pack(1)
struct ivf_header {
  uint8_t signature[4]; // = "DKIF"
  uint16_t version; // = 0
  uint16_t length; // = 32
  uint8_t fourcc[4]; // = "VP80"
  uint16_t width;
  uint16_t height;
  uint32_t framerate; // = timebase.den
  uint32_t timescale; // = timebase.num
  uint32_t frames;
  uint32_t unused; // = 0
};

void prepend_ivf_header(vpx_codec_enc_cfg_t *cfg, int frames) {
  grow_buffer(32);
  memmove(buffer+32, buffer, buffer_size-32);
  auto header = (struct ivf_header *) &buffer[0];
  memcpy(&header->signature, "DKIF", 4);
  header->version = 0;
  header->length = 32;
  memcpy(&header->fourcc, "VP80", 4);
  header->width = (uint16_t) cfg->g_w;
  header->height = (uint16_t) cfg->g_h;
  header->framerate = (uint32_t) cfg->g_timebase.den;
  header->timescale = (uint32_t) cfg->g_timebase.num;
  header->frames = (uint32_t) frames;
  header->unused = 0;
}

val encode() {
  vpx_codec_ctx_t ctx;
  auto iface = vpx_codec_vp8_cx();
  vpx_codec_enc_cfg_t cfg;
  vpx_codec_err_t err;

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

  for(int i = 0; i < 300; i++) {
    // add_noise_in_y_plane(img);
    if(!encode_frame(&ctx, img, i)) {
      return val::undefined();
    }
  }
  vpx_img_free(img);

  // Flush
  if(!encode_frame(&ctx, NULL, -1)) {
    return val::undefined();
  }

  prepend_ivf_header(&cfg, 30);

  return val(typed_memory_view(buffer_size, buffer));
}

val last_error() {
  return val(std::string(vpx_codec_err_to_string(err)));
}

void free_buffer() {
  free(buffer);
  buffer = NULL;
  buffer_size = 0;
}

EMSCRIPTEN_BINDINGS(my_module) {
  function("version", &version);
  function("encode", &encode);
  function("free_buffer", &free_buffer);
  function("last_error", &last_error);
}
