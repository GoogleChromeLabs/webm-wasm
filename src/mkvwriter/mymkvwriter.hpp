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

#ifndef MYMKVWRITER_HPP
#define MYMKVWRITER_HPP

#include <cstdlib>
#include <cstring>
#include <inttypes.h>

#include "emscripten/bind.h"

#include "mkvmuxer.hpp"

using namespace emscripten;

class MyMkvWriter : public mkvmuxer::IMkvWriter {
 public:
  explicit MyMkvWriter(val cb);
  virtual ~MyMkvWriter();

  virtual int64_t Position() const;
  virtual int32_t Position(int64_t position);
  virtual bool Seekable() const;
  virtual int32_t Write(const void* buffer, uint32_t length);
  virtual void ElementStartNotify(uint64_t element_id, int64_t position);

  void Notify();

 private:
  uint8_t* buf;
  uint64_t pos;
  uint64_t len;
  uint64_t cap;
  val cb;
};

#endif  // MYMKVWRITER_HPP
