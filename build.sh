#!/bin/bash

set -e

export OPTIMIZE="-Os"
export LDFLAGS="${OPTIMIZE}"
export CFLAGS="${OPTIMIZE}"
export CPPFLAGS="${OPTIMIZE}"

echo "============================================="
echo "Compiling libyuv"
echo "============================================="
test -n "$SKIP_LIBYUV" || (
  rm -rf build-yuv || true
  mkdir build-yuv; cd build-yuv
  # emcmake cmake -DCMAKE_BUILD_TYPE="Release" ../node_modules/libyuv
  # emcmake cmake --build . --config Release
  emcmake cmake -DCMAKE_BUILD_TYPE=Release ../node_modules/libyuv
  emmake make
)
echo "============================================="
echo "Compiling libyuv done"
echo "============================================="

echo "============================================="
echo "Compiling libvpx"
echo "============================================="
test -n "$SKIP_LIBVPX" || (
  rm -rf build-vpx || true
  mkdir build-vpx; cd build-vpx
  emconfigure ../node_modules/libvpx/configure \
    --disable-vp9-decoder \
    --disable-vp9-encoder \
    --disable-vp8-decoder \
    --target=generic-gnu
  emmake make
)
echo "============================================="
echo "Compiling libvpx done"
echo "============================================="

echo "============================================="
echo "Compiling libwebm"
echo "============================================="
test -n "$SKIP_LIBWEBM" ||(
  rm -rf build-webm || true
  mkdir build-webm; cd build-webm
  emcmake cmake ../node_modules/libwebm
  emmake make
)
echo "============================================="
echo "Compiling libwebm done"
echo "============================================="

echo "============================================="
echo "Compiling wasm bindings"
echo "============================================="
(
  emcc \
    ${OPTIMIZE} \
    --bind \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s ASSERTIONS=0 \
    -s MODULARIZE=1 \
    -s 'EXPORT_NAME="webmWasm"' \
    --std=c++11 \
    -I node_modules/libyuv/include \
    -I node_modules/libvpx \
    -I node_modules/libwebm \
    -I build-vpx \
    -I build-webm \
    -o ./webm-wasm.js \
    -x c++ \
    src/webm-wasm.cpp \
    src/mkvwriter/mymkvwriter.cpp \
    src/mkvwriter/mymkvstreamwriter.cpp \
    build-yuv/libyuv.a \
    build-vpx/libvpx.a \
    build-webm/libwebm.a
  mkdir dist || true
  mv webm-wasm.{js,wasm} dist
)
echo "============================================="
echo "Compiling wasm bindings done"
echo "============================================="

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "Did you update your docker image?"
echo "Run docker pull trzeci/emscripten"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
