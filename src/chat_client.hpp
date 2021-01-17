#pragma once

#include <functional>
#include <mesh_message.hpp>
#include <memory>
#undef F
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <deque>
#include <boost/thread.hpp>
namespace ssl = boost::asio::ssl;

class chat_client {
  typedef ssl::stream<tcp::socket> ssl_socket;
  typedef std::deque<mesh_message> chat_message_queue;
  typedef std::function<void(uint32_t from, TSTRING& msg)> receivedCallback_t;
  typedef std::function<void(const mesh_message& msg)> fmm;

 public:
  chat_client(boost::asio::io_service &io_service, std::string url, std::string port);

  void setHandleMessageReceiveHandler(
      std::function<void(const mesh_message& msg)>
          handle_message_receive_handler);

  void write(const mesh_message& msg);

  void close();

 private:
  void handle_connect(const boost::system::error_code& error);
  void initialize(bool first);
  void handle_header_message(const boost::system::error_code& error);

  void handle_body_message(const boost::system::error_code& error);
  void write_body(mesh_message msg, const boost::system::error_code& error);
  void do_write(mesh_message msg);
  void handle_write(const boost::system::error_code& error);

  void do_close();

  ssl::context _ctx;
  boost::asio::ip::tcp::resolver::query _query;
  boost::asio::io_service &_io_service;
  boost::asio::ip::tcp::resolver _resolver;
  boost::asio::ip::tcp::resolver::iterator _iterator;
  std::unique_ptr<boost::thread> _service_thread;
  std::unique_ptr<ssl_socket> _socket;
  mesh_message _read_msg;
  chat_message_queue _write_msgs;
  std::function<void(const mesh_message& msg)> _handle_message_receive_handler;
};
