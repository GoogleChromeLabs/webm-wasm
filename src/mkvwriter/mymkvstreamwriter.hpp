#ifndef MYMKVSTREAMWRITER_HPP
#define MYMKVSTREAMWRITER_HPP

#include <cstdlib>
#include <cstring>
#include <inttypes.h>

#include "emscripten/bind.h"

#include "mkvmuxer.hpp"

using namespace emscripten;

class MyMkvStreamWriter : public mkvmuxer::IMkvWriter {
 public:
  explicit MyMkvStreamWriter(val cb);
  virtual ~MyMkvStreamWriter();

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

#endif  // MYMKVSTREAMWRITER_HPP
