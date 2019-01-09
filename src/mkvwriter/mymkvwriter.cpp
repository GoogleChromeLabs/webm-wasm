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

#include "mymkvwriter.hpp"

MyMkvWriter::MyMkvWriter(val cb_) : pos(0), len(0), cb(cb_), cap(1024) {
  buf = (uint8_t*) malloc(cap);
}

MyMkvWriter::~MyMkvWriter() {
}

int32_t MyMkvWriter::Write(const void* buffer, uint32_t length) {
  while(pos + length >= cap) {
    cap *= 2;
    buf = (uint8_t*) realloc((void*)buf, cap);
  }
  memcpy(buf + pos, buffer, length);
  len += length;
  pos += length;
  return 0;
}

int64_t MyMkvWriter::Position() const {
  return pos;
}

int32_t MyMkvWriter::Position(int64_t position) {
  pos = position;
  return 0;
}

bool MyMkvWriter::Seekable() const {
  return true;
}

void MyMkvWriter::ElementStartNotify(uint64_t, int64_t) {
}

void MyMkvWriter::Notify() {
  cb(val(typed_memory_view(len, buf)));
}
