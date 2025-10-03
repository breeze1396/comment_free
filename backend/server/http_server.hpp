#pragma once

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/config.hpp>
#include <memory>
#include <string>

namespace http = boost::beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace server {

// HTTP请求处理器
class HttpServer {
private:
    net::io_context ioc;
    tcp::acceptor acceptor;
    std::string doc_root;
    unsigned short port;
    
public:
    HttpServer(const std::string& address, unsigned short port, const std::string& doc_root);
    
    // 启动服务器
    void run();
    
    // 停止服务器
    void stop();
    
private:
    // 接受连接
    void do_accept();
    
    // 处理HTTP会话
    void on_accept(boost::beast::error_code ec, tcp::socket socket);
};

// HTTP会话处理
class HttpSession : public std::enable_shared_from_this<HttpSession> {
private:
    tcp::socket socket_;
    boost::beast::flat_buffer buffer_;
    http::request<http::string_body> req_;
    std::string doc_root_;
    
public:
    HttpSession(tcp::socket&& socket, const std::string& doc_root);
    
    // 开始会话
    void run();
    
private:
    void do_read();
    void on_read(boost::beast::error_code ec, std::size_t bytes_transferred);
    void on_write(bool close, boost::beast::error_code ec, std::size_t bytes_transferred);
    void do_close();
    
    // 处理请求
    template<class Body, class Allocator>
    http::message_generator handle_request(
        http::request<Body, http::basic_fields<Allocator>>&& req);
};

// MIME类型辅助函数
std::string mime_type(const std::string& path);

// 路径相关函数
std::string path_cat(const std::string& base, const std::string& path);

} // namespace server