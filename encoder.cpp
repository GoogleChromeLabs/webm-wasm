#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <webmenc.h>
#include "vpxenc.h"
#include "vpx/vp8cx.h"

#include <cstdlib>

using namespace emscripten;

// Globals
vpx_codec_err_t err;
unsigned char *buffer = NULL;
int bufferSize = 0;

int version() {
  return VPX_CODEC_ABI_VERSION;
}

void putNoiseInYPlane(vpx_image_t *img) {
  unsigned char *row = img->planes[0];
  for(int y = 0; y < img->h; y++) {
    unsigned char *p = row;
    for(int x = 0; x < img->w; x++) {
      *p = rand();
      p++;
    }
    row += img->stride[0];
  }
}

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
    buffer = (unsigned char*) realloc((void*)buffer, bufferSize + pkt->data.frame.sz);
    memcpy(buffer + bufferSize, pkt->data.frame.buf, pkt->data.frame.sz);
    bufferSize += pkt->data.frame.sz;
  }
  return true;
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

  for(int i = 0; i < 30; i++) {
    // putNoiseInYPlane(img);
    if(!encode_frame(&ctx, img, i)) {
      return val::undefined();
    }
  }
  vpx_img_free(img);

  // Flush
  if(!encode_frame(&ctx, NULL, -1)) {
    return val::undefined();
  }
  return val(typed_memory_view(bufferSize, buffer));
}

val last_error() {
  return val(std::string(vpx_codec_err_to_string(err)));
}

void free_buffer() {
  free(buffer);
  buffer = NULL;
  bufferSize = 0;
}

EMSCRIPTEN_BINDINGS(my_module) {
  function("version", &version);
  function("encode", &encode);
  function("free_buffer", &free_buffer);
  function("last_error", &last_error);
}
