#!/bin/bash
# Copyright 2018 Google Inc. All Rights Reserved.
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#     http://www.apache.org/licenses/LICENSE-2.0
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
    -s STRICT=1 \
    -s ALLOW_MEMORY_GROWTH=1 \
    -s ASSERTIONS=0 \
    -s MODULARIZE=1 \
    -s FILESYSTEM=0 \
    -s EXPORT_ES6=1 \
    -s MALLOC=emmalloc \
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
