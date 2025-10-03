#pragma once

#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <string>
#include <memory>
#include "db.hpp"

namespace http = boost::beast::http;

namespace routes {

// 路由处理器
class RouteHandler {
private:
    std::shared_ptr<db::DatabaseManager> db_manager;
    std::string uploads_dir;
    
public:
    RouteHandler(std::shared_ptr<db::DatabaseManager> db, const std::string& uploads_dir);
    
    // 处理所有HTTP请求的入口
    template<class Body, class Allocator>
    http::message_generator handle_request(
        http::request<Body, http::basic_fields<Allocator>>&& req,
        const std::string& doc_root);
    
private:
    // API路由处理
    http::response<http::string_body> handle_api_submit(const http::request<http::string_body>& req);
    http::response<http::string_body> handle_api_view(const std::string& id);
    http::response<http::string_body> handle_api_like(const std::string& id);
    
    // 静态文件服务
    template<class Body, class Allocator>
    http::message_generator serve_file(
        const http::request<Body, http::basic_fields<Allocator>>& req,
        const std::string& path,
        const std::string& doc_root);
    
    // 辅助函数
    std::string extract_post_id_from_path(const std::string& path);
    bool parse_multipart_form(const std::string& body, const std::string& boundary,
                             std::string& content, std::vector<std::string>& image_files);
    std::string create_json_response(const std::string& status, const std::string& message, 
                                   const std::string& data = "");
    
    // HTTP响应创建
    http::response<http::string_body> bad_request(const std::string& why);
    http::response<http::string_body> not_found(const std::string& target);
    http::response<http::string_body> server_error(const std::string& what);
    http::response<http::string_body> ok_response(const std::string& content, 
                                                 const std::string& content_type = "application/json");
    
    // CORS处理
    template<class Body>
    void add_cors_headers(http::response<Body>& res);
};

} // namespace routes