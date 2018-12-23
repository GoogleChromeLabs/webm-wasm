#!/bin/bash

set -e

export OPTIMIZE="-Os"
export LDFLAGS="${OPTIMIZE}"
export CFLAGS="${OPTIMIZE}"
export CPPFLAGS="${OPTIMIZE}"

echo "============================================="
echo "Installing doxygen"
echo "============================================="
test -n "$SKIP_DOCS" || (
  apt-get update
  apt-get install -qqy doxygen
)
echo "============================================="
echo "Installing doxygen done"
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
    -s ASSERTIONS=2 \
    --std=c++11 \
    -I node_modules/libvpx \
    -I node_modules/libwebm \
    -I build-vpx \
    -I build-webm \
    -o ./encoder.js \
    -x c++ \
    encoder.cpp \
    build-vpx/libvpx.a \
    build-webm/libwebm.a
)
echo "============================================="
echo "Compiling wasm bindings done"
echo "============================================="

echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
echo "Did you update your docker image?"
echo "Run docker pull trzeci/emscripten"
echo "!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!"
