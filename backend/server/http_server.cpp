#include "http_server.hpp"
#include "routes.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <iostream>
#include <memory>
#include <thread>

namespace beast = boost::beast;
namespace http = beast::http;
namespace net = boost::asio;
using tcp = net::ip::tcp;

namespace server {

// MIME类型映射
std::string mime_type(const std::string& path) {
    using beast::iequals;
    auto const ext = [&path] {
        auto const pos = path.rfind(".");
        if (pos == std::string::npos)
            return std::string{};
        return path.substr(pos);
    }();
    
    if (iequals(ext, ".htm"))  return "text/html";
    if (iequals(ext, ".html")) return "text/html";
    if (iequals(ext, ".php"))  return "text/html";
    if (iequals(ext, ".css"))  return "text/css";
    if (iequals(ext, ".txt"))  return "text/plain";
    if (iequals(ext, ".js"))   return "application/javascript";
    if (iequals(ext, ".json")) return "application/json";
    if (iequals(ext, ".xml"))  return "application/xml";
    if (iequals(ext, ".swf"))  return "application/x-shockwave-flash";
    if (iequals(ext, ".flv"))  return "video/x-flv";
    if (iequals(ext, ".png"))  return "image/png";
    if (iequals(ext, ".jpe"))  return "image/jpeg";
    if (iequals(ext, ".jpeg")) return "image/jpeg";
    if (iequals(ext, ".jpg"))  return "image/jpeg";
    if (iequals(ext, ".gif"))  return "image/gif";
    if (iequals(ext, ".bmp"))  return "image/bmp";
    if (iequals(ext, ".ico"))  return "image/vnd.microsoft.icon";
    if (iequals(ext, ".tiff")) return "image/tiff";
    if (iequals(ext, ".tif"))  return "image/tiff";
    if (iequals(ext, ".svg"))  return "image/svg+xml";
    if (iequals(ext, ".svgz")) return "image/svg+xml";
    return "application/text";
}

// 路径连接
std::string path_cat(const std::string& base, const std::string& path) {
    if (base.empty())
        return path;
    std::string result = base;
    if (result.back() != '/' && !path.empty() && path.front() != '/')
        result.append("/");
    result.append(path);
    return result;
}

HttpServer::HttpServer(const std::string& address, unsigned short port, const std::string& doc_root)
    : ioc{1}, acceptor{ioc}, doc_root(doc_root), port(port) {
    
    beast::error_code ec;
    
    // 解析地址
    auto const addr = net::ip::make_address(address);
    tcp::endpoint endpoint{addr, port};
    
    // 打开acceptor
    acceptor.open(endpoint.protocol(), ec);
    if (ec) {
        std::cerr << "打开acceptor失败: " << ec.message() << std::endl;
        return;
    }
    
    // 允许地址重用
    acceptor.set_option(net::socket_base::reuse_address(true), ec);
    if (ec) {
        std::cerr << "设置socket选项失败: " << ec.message() << std::endl;
        return;
    }
    
    // 绑定到服务器地址
    acceptor.bind(endpoint, ec);
    if (ec) {
        std::cerr << "绑定地址失败: " << ec.message() << std::endl;
        return;
    }
    
    // 开始监听连接
    acceptor.listen(net::socket_base::max_listen_connections, ec);
    if (ec) {
        std::cerr << "监听失败: " << ec.message() << std::endl;
        return;
    }
    
    std::cout << "HTTP服务器启动在 " << address << ":" << port << std::endl;
    std::cout << "文档根目录: " << doc_root << std::endl;
}

void HttpServer::run() {
    do_accept();
    ioc.run();
}

void HttpServer::stop() {
    ioc.stop();
}

void HttpServer::do_accept() {
    acceptor.async_accept(
        [this](beast::error_code ec, tcp::socket socket) {
            on_accept(ec, std::move(socket));
        });
}

void HttpServer::on_accept(beast::error_code ec, tcp::socket socket) {
    if (ec) {
        std::cerr << "接受连接失败: " << ec.message() << std::endl;
    } else {
        // 创建新的会话并运行
        std::make_shared<HttpSession>(std::move(socket), doc_root)->run();
    }
    
    // 继续接受连接
    do_accept();
}

// HttpSession实现
HttpSession::HttpSession(tcp::socket&& socket, const std::string& doc_root)
    : socket_(std::move(socket)), doc_root_(doc_root) {
}

void HttpSession::run() {
    do_read();
}

void HttpSession::do_read() {
    req_ = {};
    
    http::async_read(socket_, buffer_, req_,
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            self->on_read(ec, bytes_transferred);
        });
}

void HttpSession::on_read(beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    
    if (ec == http::error::end_of_stream) {
        return do_close();
    }
    
    if (ec) {
        std::cerr << "读取请求失败: " << ec.message() << std::endl;
        return;
    }
    
    // 处理请求
    auto response = handle_request(std::move(req_));
    
    // 发送响应
    beast::async_write(socket_, std::move(response),
        [self = shared_from_this()](beast::error_code ec, std::size_t bytes_transferred) {
            self->on_write(false, ec, bytes_transferred);
        });
}

void HttpSession::on_write(bool close, beast::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);
    
    if (ec) {
        std::cerr << "写入响应失败: " << ec.message() << std::endl;
        return;
    }
    
    if (close) {
        return do_close();
    }
    
    // 读取下一个请求
    do_read();
}

void HttpSession::do_close() {
    beast::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_send, ec);
}

template<class Body, class Allocator>
http::message_generator HttpSession::handle_request(
    http::request<Body, http::basic_fields<Allocator>>&& req) {
    
    // 创建路由处理器实例（这里需要传入数据库管理器实例）
    // 注意：在实际应用中，应该在服务器启动时创建数据库连接
    extern std::shared_ptr<db::DatabaseManager> g_db_manager;
    routes::RouteHandler handler(g_db_manager, "uploads");
    
    return handler.handle_request(std::move(req), doc_root_);
}

} // namespace server