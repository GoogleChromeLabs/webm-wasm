#include "mymkvstreamwriter.hpp"

MyMkvStreamWriter::MyMkvStreamWriter(val cb_) : cb(cb_) {
}

MyMkvStreamWriter::~MyMkvStreamWriter() {
}

int32_t MyMkvStreamWriter::Write(const void* buffer, uint32_t length) {
  buf = (uint8_t*) realloc((void*)buf, len + length);
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
  free(buf);
  buf = NULL;
}
