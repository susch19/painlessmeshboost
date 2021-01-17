#pragma once

#include <boost/endian/arithmetic.hpp>
#include <string>
#include <memory>
using namespace boost::endian;

class mesh_message {
 public:
  enum { header_length = sizeof(little_int32_t) };

  mesh_message() : body_length_(0) {}

  mesh_message(const std::string& body)
      : body_length_(body.size()),
        body_str(std::make_unique<std::string>(body)) {}

  mesh_message(const mesh_message& msg)
      : body_length_(msg.body_length()),
        body_str(std::make_unique<std::string>(msg.body())) {}

  const char* header() const {
    return reinterpret_cast<const char*>(&body_length_);
  }

  char* header() { return reinterpret_cast<char*>(&body_length_); }

  size_t length() const { return header_length + body_length_; }

  std::string& body() const { return *body_str; }

  char* body() { return const_cast<char*>(body_str->data()); }

  size_t body_length() const { return body_length_; }

  void alloc_body() {
    body_str = std::make_unique<std::string>(body_length(), '\0');
  }

 private:
  little_int32_t body_length_;
  std::unique_ptr<std::string> body_str;
};
