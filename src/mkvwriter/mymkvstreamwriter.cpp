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
