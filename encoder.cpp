#include <emscripten/bind.h>
#include <webmenc.h>

using namespace emscripten;

int version() {
  return VPX_CODEC_ABI_VERSION;
}

EMSCRIPTEN_BINDINGS(my_module) {
  function("version", &version);
}
