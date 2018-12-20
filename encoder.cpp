#include <emscripten/bind.h>
#include <webmenc.h>
#include "vpxenc.h"
#include "vpx/vp8cx.h"

#include <stdio.h>

using namespace emscripten;

int version() {
  return VPX_CODEC_ABI_VERSION;
}

void printError(vpx_codec_err_t err) {
  switch(err) {
   case VPX_CODEC_OK:
    printf("VPX_CODEC_OK\n");
   break;
   case VPX_CODEC_ERROR:
    printf("VPX_CODEC_ERROR\n");
   break;
   case VPX_CODEC_MEM_ERROR:
    printf("VPX_CODEC_MEM_ERROR\n");
   break;
   case VPX_CODEC_ABI_MISMATCH:
    printf("VPX_CODEC_ABI_MISMATCH\n");
   break;
   case VPX_CODEC_INCAPABLE:
    printf("VPX_CODEC_INCAPABLE\n");
   break;
   case VPX_CODEC_UNSUP_BITSTREAM:
    printf("VPX_CODEC_UNSUP_BITSTREAM\n");
   break;
   case VPX_CODEC_UNSUP_FEATURE:
    printf("VPX_CODEC_UNSUP_FEATURE\n");
   break;
   case VPX_CODEC_CORRUPT_FRAME:
    printf("VPX_CODEC_CORRUPT_FRAME\n");
   break;
   case VPX_CODEC_INVALID_PARAM:
    printf("VPX_CODEC_INVALID_PARAM\n");
   break;
   case VPX_CODEC_LIST_END:
    printf("VPX_CODEC_LIST_END\n");
   break;
  }
}

int encode() {
  vpx_codec_ctx_t ctx;
  auto iface = vpx_codec_vp8_cx();
  vpx_codec_enc_cfg_t cfg;
  vpx_codec_err_t err;

  err = vpx_codec_enc_config_default(iface, &cfg, 0);
  if(err != VPX_CODEC_OK) {
    printError(err);
    return 20 + err;
  }

  err = vpx_codec_enc_init(&ctx, iface, &cfg, 0 /* flags */);
  if(err != VPX_CODEC_OK) {
    printError(err);
    return 10 + err;
  }

  cfg.g_timebase = vpx_rational_t {1, 30};
  cfg.g_w = 300;
  cfg.g_h = 300;

  // err = vpx_codec_enc_config_set(&ctx, &cfg);
  // if(err != VPX_CODEC_OK) {
  //   printError(err);
  //   return 30;
  // }

  vpx_image_t *img = vpx_img_alloc(
    NULL, /* allocate buffer on the heap */
    VPX_IMG_FMT_RGB24,
    300,
    300,
    0
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
    err = vpx_codec_encode(
      &ctx,
      img,
      i, /* time of frame */
      10, /* length of frame */
      0, /* flags */
      0  /* deadline */
    );
    if(err != VPX_CODEC_OK) {
      printError(err);
      return 1000 + i;
    }
  }

  #if 0
  vpx_img_free(img);
  // Flush
  if(vpx_codec_encode(
    &ctx,
    img,
    30, /* time of frame */
    1, /* length of frame */
    0, /* flags */
    0  /* deadline */
  ) != VPX_CODEC_OK) {
    return 50;
  }
  #endif
  return 0;
}

EMSCRIPTEN_BINDINGS(my_module) {
  function("version", &version);
  function("encode", &encode);
}
