/**
 * Copyright 2018 Google Inc. All Rights Reserved.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *     http://www.apache.org/licenses/LICENSE-2.0
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "mymkvstreamwriter.hpp"

MyMkvStreamWriter::MyMkvStreamWriter(val cb_) : cb(cb_), pos(0), len(0), cap(1024) {
  buf = (uint8_t*) malloc(cap);
}

MyMkvStreamWriter::~MyMkvStreamWriter() {
}

int32_t MyMkvStreamWriter::Write(const void* buffer, uint32_t length) {
  while(len + length >= cap) {
    cap *= 2;
    buf = (uint8_t*) realloc((void*)buf, cap);
  }
  memcpy(buf + len, buffer, length);
  len += length;
  pos += length;
  return 0;
}

int64_t MyMkvStreamWriter::Position() const {
  return pos;
}

int32_t MyMkvStreamWriter::Position(int64_t position) {
  return -1;
}

bool MyMkvStreamWriter::Seekable() const {
  return false;
}

void MyMkvStreamWriter::ElementStartNotify(uint64_t, int64_t) {
}

void MyMkvStreamWriter::Notify() {
  cb(val(typed_memory_view(len, buf)));
  len = 0;
}
