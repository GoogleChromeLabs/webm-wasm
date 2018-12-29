#include "mymkvwriter.hpp"

MyMkvWriter::MyMkvWriter(val cb_) : pos(0), len(0), cb(cb_), cap(1024) {
  buf = (uint8_t*) malloc(cap);
}

MyMkvWriter::~MyMkvWriter() {
}

int32_t MyMkvWriter::Write(const void* buffer, uint32_t length) {
  while(len + length >= cap) {
    cap *= 2;
    buf = (uint8_t*) realloc((void*)buf, cap);
  }
  memcpy(buf + len, buffer, length);
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
