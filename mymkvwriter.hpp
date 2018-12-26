#ifndef MYMKVWRITER_HPP
#define MYMKVWRITER_HPP

#include <cstdlib>
#include <cstring>
#include <inttypes.h>

#include "mkvmuxer.hpp"

class MyMkvWriter : public mkvmuxer::IMkvWriter {
 public:
  explicit MyMkvWriter();
  virtual ~MyMkvWriter();

  virtual int64_t Position() const;
  virtual int32_t Position(int64_t position);
  virtual bool Seekable() const;
  virtual int32_t Write(const void* buffer, uint32_t length);
  virtual void ElementStartNotify(uint64_t element_id, int64_t position);

  uint8_t* buf;
  uint64_t pos;
  uint64_t len;
};

#endif  // MYMKVWRITER_HPP
