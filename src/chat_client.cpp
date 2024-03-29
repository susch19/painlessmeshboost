#include <algorithm>
#include <functional>
#include <iostream>
//#include <iterator>
#include "Arduino.h"

#include <chat_client.hpp>
#undef F
#include <boost/algorithm/hex.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/ssl/stream.hpp>
#include <boost/bind.hpp>
#include <fstream>
#include <cstdlib>


chat_client::chat_client(boost::asio::io_service &io_service, std::string url, std::string port)
    : _ctx(ssl::context::tlsv12_client),
      _io_service(io_service),
      _query(url, port),
      _resolver(_io_service),
      _iterator(_resolver.resolve(_query))
{
    _ctx.set_default_verify_paths();
    _ctx.set_verify_mode(ssl::verify_peer);
    {
        std::cout << "loading client key.pem" << std::endl;
        std::ifstream stream("/etc/ssl/private/appbroker_client_key.pem");
        std::string str((std::istreambuf_iterator<char>(stream)),
                        std::istreambuf_iterator<char>());

        std::cout << "loaded client key.pem" << std::endl;

        auto temp = new char[str.size()+1];
        strcpy(temp, str.c_str());
        const boost::asio::const_buffer c(str.c_str(), str.size());
                std::cout << "Buf:"<<(void *)c.data() << " cstr:"  << (void *)str.c_str() << std::endl;
        _ctx.use_private_key(c, ssl::context::file_format::pem);
        std::cout << "used client key.pem" << std::endl;
    }
    {
        std::cout << "loading client cert.pem" << std::endl;
        std::ifstream stream("/etc/ssl/certs/appbroker_client_cert.pem");
        std::string str((std::istreambuf_iterator<char>(stream)),
                        std::istreambuf_iterator<char>());

        std::cout << "loaded client cert.pem" << std::endl;

        auto temp = new char[str.size()+1];
        strcpy(temp, str.c_str());
        const boost::asio::const_buffer c(temp, str.size());
        _ctx.use_certificate(c, ssl::context::file_format::pem);
        std::cout << "used client cert.pem" << std::endl;
    }
    initialize(true);
}

void chat_client::initialize(bool first){
    using boost::asio::ip::tcp;

    _socket = std::make_unique<ssl_socket>(_io_service, _ctx);
    boost::asio::async_connect(_socket->lowest_layer(), _iterator,
                               boost::bind(&chat_client::handle_connect, this,
                                           boost::asio::placeholders::error));
}

void chat_client::setHandleMessageReceiveHandler(
        std::function<void(const mesh_message& msg)>
        handle_message_receive_handler) {
    _handle_message_receive_handler = handle_message_receive_handler;
}

void chat_client::write(const mesh_message& msg) {
    _io_service.post(boost::bind(&chat_client::do_write, this, msg));
}

void chat_client::close() {
    _io_service.post(boost::bind(&chat_client::do_close, this));
}


void chat_client::handle_connect(const boost::system::error_code& error) {
    if (!error) {
        _socket->set_verify_mode(ssl::verify_peer);

        auto verifyCallback = [](bool preverified,
                boost::asio::ssl::verify_context& ctx) {

                std::cout << "Verifying certificate, pre-verified: " << std::string(preverified ? "true" : "false");
                char subject_name[256];
                X509* cert = X509_STORE_CTX_get_current_cert(ctx.native_handle());
                X509_NAME_oneline(X509_get_subject_name(cert), subject_name, 256);

                unsigned char thumbprint[256];
                auto digest = EVP_get_digestbyname("SHA1");
                unsigned int len;
                X509_digest(cert, digest, thumbprint, &len);
                std::string tempthumb((char*)thumbprint, len);

                std::cout << "Verifying, subject: " << subject_name << " thumb:" ;

                boost::algorithm::hex(tempthumb.begin(), tempthumb.end(), std::ostream_iterator<char> {std::cout, ""});
                std::cout << "\n";

                char issuer_name[256];
                X509_NAME_oneline(X509_get_issuer_name(cert), issuer_name, 256);
                std::cout << "Verifying, issuer: " << issuer_name << "\n";

                return true;
              };

        _socket->set_verify_callback(verifyCallback);
        //???? _socket->set_verify_callback(ssl::rfc2818_verification("smarthome.susch.eu"));
        _socket->handshake(ssl_socket::client);
        boost::asio::async_read(
                    *_socket,
                    boost::asio::buffer(_read_msg.header(), mesh_message::header_length),
                    boost::bind(&chat_client::handle_header_message, this,
                                boost::asio::placeholders::error));
        std::cout << "Handle connect finished" << std::endl;
        if(!_write_msgs.empty())
            boost::asio::async_write(
                        *_socket,
                        boost::asio::buffer(_write_msgs.front().header(),
                                            _write_msgs.front().header_length),
                        boost::bind(&chat_client::write_body, this, _write_msgs.front(),
                                    boost::asio::placeholders::error));
    }
    else if(error.value() == 111){
        usleep(1000000); //tweak for cpu usage
        std::cout << error.message() << std::endl;
        initialize(false);
    }
}

void chat_client::handle_header_message(
        const boost::system::error_code& error) {
    if (!error && _read_msg.body_length() >= 0) {
        _read_msg.alloc_body();
        auto buffer =
                boost::asio::buffer(_read_msg.body(), _read_msg.body_length());
        boost::asio::async_read(*_socket, buffer,
                                boost::bind(&chat_client::handle_body_message, this,
                                            boost::asio::placeholders::error));
    } else {
        do_close();
    }
}

void chat_client::handle_body_message(const boost::system::error_code& error) {
    if (!error) {
        std::cout << _read_msg.body();
        std::cout << "\n";
        _handle_message_receive_handler(_read_msg);
        boost::asio::async_read(
                    *_socket,
                    boost::asio::buffer(_read_msg.header(), mesh_message::header_length),
                    boost::bind(&chat_client::handle_header_message, this,
                                boost::asio::placeholders::error));
    } else {
        do_close();
    }
}

void chat_client::write_body(mesh_message msg,
                             const boost::system::error_code& error) {
    if (!error) {
        boost::asio::async_write(
                    *_socket,
                    boost::asio::buffer(msg.body(),
                                        msg.body_length()),
                    boost::bind(&chat_client::handle_write, this,
                                boost::asio::placeholders::error));
    } else {
        do_close();
    }
}

void chat_client::do_write(mesh_message msg) {
    bool write_in_progress = !_write_msgs.empty();
    _write_msgs.push_back(msg);
    if (!write_in_progress) {
        boost::asio::async_write(
                    *_socket,
                    boost::asio::buffer(_write_msgs.front().header(),
                                        _write_msgs.front().header_length),
                    boost::bind(&chat_client::write_body, this, _write_msgs.front(),
                                boost::asio::placeholders::error));
    }
}

void chat_client::handle_write(const boost::system::error_code& error) {
    if (!error) {
        _write_msgs.pop_front();
        if (!_write_msgs.empty()) {
            boost::asio::async_write(
                        *_socket,
                        boost::asio::buffer(_write_msgs.front().body(),
                                            _write_msgs.front().body_length()),
                        boost::bind(&chat_client::handle_write, this,
                                    boost::asio::placeholders::error));
        }
    } else {
        do_close();
    }
}

void chat_client::do_close() {
    // _socket.close();
    try {
        if(_socket)
            _socket->shutdown();

    }  catch (const boost::system::system_error &e) {
        std::cout << e.what() << std::endl;
    }
    initialize(false);
}
