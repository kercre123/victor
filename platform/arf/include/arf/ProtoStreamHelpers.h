#pragma once

#include <cstdint>
#include <fstream>
#include <iostream>


namespace ARF {

template <typename T> int64_t write_proto(const T &proto, std::ostream &os) {
  int64_t byte_size = static_cast<int64_t>(proto.ByteSizeLong());
  os.write(reinterpret_cast<char *>(&byte_size), sizeof(byte_size));
  proto.SerializeToOstream(&os);
  return byte_size;
}

template <typename T> bool read_proto(std::istream &is, T *proto) {
  if (!is.good())
    return false;
  std::streampos before_next_size = is.tellg();
  int64_t next_size;
  is.read(reinterpret_cast<char *>(&next_size), sizeof(next_size));
  if (!is.good()) {
    return false;
  }
  std::streampos after_next_size = is.tellg();
  if (after_next_size - before_next_size != sizeof(next_size)) {
    printf("Bad size bytes: %ld\n", after_next_size - before_next_size);
    return false;
  }
  char* dst = new char[static_cast<unsigned int>(next_size)];
  is.read(dst, static_cast<std::streamsize>(next_size));
  std::streampos after_message_size = is.tellg();
  if (after_message_size - after_next_size != next_size) {
    printf("Bad message bytes: %ld instead of %lld\n",
           after_message_size - after_next_size, next_size);
    return false;
  }
  proto->Clear();
  bool parse_ok = proto->ParseFromArray(dst, next_size);
  delete[] dst;
  return parse_ok;
}

template <class ProtoType> class ProtoWriter {
public:
  ProtoWriter() = default;
  ~ProtoWriter() { CloseFile(); }

  bool OpenFile(const std::string &path) {
    out_file_.open(path, std::ios::binary | std::ofstream::out |
                             std::ofstream::trunc);
    return out_file_.is_open();
  }

  bool Write(const ProtoType &proto) {
    if (!IsFileOpen())
      return false;
    write_proto<ProtoType>(proto, out_file_);
    return true;
  }

  bool IsFileOpen() const { return out_file_.is_open(); }

  void CloseFile() {
    if (out_file_.is_open()) {
      out_file_.close();
    }
  }

private:
  std::ofstream out_file_;
};

template <class ProtoType> class ProtoReader {
public:
  ProtoReader() = default;

  // Opens the file in binary mode, returns false if that fails.
  bool OpenFile(const std::string &path) {
    in_file_.open(path, std::ios::binary | std::ofstream::in);
    return in_file_.is_open();
  }

  bool Read(ProtoType *message) {
    if (!in_file_.is_open()) {
      return false;
    }
    return read_proto<ProtoType>(in_file_, message);
  }

  bool IsFileOpen() const { return in_file_.is_open(); }

private:
  std::ifstream in_file_;
};
} // namespace ARF