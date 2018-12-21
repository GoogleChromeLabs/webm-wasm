#include <emscripten/bind.h>
#include <webmenc.h>
#include "vpxenc.h"
#include "vpx/vp8cx.h"

#include <stdio.h>

using namespace emscripten;

int version() {
  return VPX_CODEC_ABI_VERSION;
}

// TODO: Remove from final build
void printError(vpx_codec_err_t err, vpx_codec_ctx_t *ctx) {
  printf("%s\n", vpx_codec_err_to_string(err));
  printf("%s\n", ctx->err_detail);
}

void putNoiseInY(vpx_image_t *img) {
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

int encode() {
  vpx_codec_ctx_t ctx;
  auto iface = vpx_codec_vp8_cx();
  vpx_codec_enc_cfg_t cfg;
  vpx_codec_err_t err;

  memset(&ctx, 0, sizeof(ctx));
  memset(&cfg, 0, sizeof(cfg));

  err = vpx_codec_enc_config_default(iface, &cfg, 0);
  if(err != VPX_CODEC_OK) {
    printError(err, &ctx);
    return 20 + err;
  }

  cfg.g_timebase.num = 1;
  cfg.g_timebase.den = 30; // = fps
  cfg.g_w = 300;
  cfg.g_h = 300;
  cfg.rc_target_bitrate = 200; // FIXME

  err = vpx_codec_enc_init(&ctx, iface, &cfg, 0 /* flags */);
  if(err != VPX_CODEC_OK) {
    printError(err, &ctx);
    return 10 + err;
  }

  // err = vpx_codec_enc_config_set(&ctx, &cfg);
  // if(err != VPX_CODEC_OK) {
  //   printError(err, &ctx);
  //   return 30;
  // }

  vpx_image_t *img = vpx_img_alloc(
    NULL, /* allocate buffer on the heap */
    VPX_IMG_FMT_I420,
    // VPX_IMG_FMT_RGB24,
    300,
    300,
    0 /* align. simple_encoder says 1? */
  );
  if(img == NULL) {
    return 40;
  }
  // img->cs = VPX_CS_SRGB;
  // img->range = VPX_CR_FULL_RANGE;
  // img->bit_depth = 8;
  // img->d_w = 300;
  // img->d_h = 300;
  // img->r_w = 300;
  // img->r_h = 300;

  for(int i = 0; i < 30; i++) {
    putNoiseInY(img);
    err = vpx_codec_encode(
      &ctx,
      img,
      i, /* time of frame */
      1, /* length of frame */
      0, /* flags */
      0 /* deadline */
    );
    if(err != VPX_CODEC_OK) {
      printError(err, &ctx);
      return 1000 + i;
    }
  }

  vpx_img_free(img);
  // Flush
  if(vpx_codec_encode(
    &ctx,
    NULL,
    -1, /* time of frame */
    0, /* length of frame */
    0, /* flags */
    0 /* deadline */
  ) != VPX_CODEC_OK) {
    return 50;
  }
  return 0;
}

EMSCRIPTEN_BINDINGS(my_module) {
  function("version", &version);
  function("encode", &encode);
}
