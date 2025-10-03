#include "routes.hpp"
#include "utils.hpp"
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/json.hpp>
#include <fstream>
#include <iostream>
#include <sstream>

namespace beast = boost::beast;
namespace http = beast::http;
namespace json = boost::json;

namespace routes {

RouteHandler::RouteHandler(std::shared_ptr<db::DatabaseManager> db, const std::string& uploads_dir)
    : db_manager(db), uploads_dir(uploads_dir) {
}

template<class Body, class Allocator>
http::message_generator RouteHandler::handle_request(
    http::request<Body, http::basic_fields<Allocator>>&& req,
    const std::string& doc_root) {
    
    std::string target = std::string(req.target());
    auto method = req.method();
    
    std::cout << "收到请求: " << req.method_string() << " " << target << std::endl;
    
    // API路由处理
    if (target.starts_with("/api/")) {
        if (target == "/api/submit" && method == http::verb::post) {
            return handle_api_submit(req);
        } else if (target.starts_with("/api/view/") && method == http::verb::get) {
            std::string id = extract_post_id_from_path(target);
            return handle_api_view(id);
        } else if (target.starts_with("/api/like/") && method == http::verb::post) {
            std::string id = extract_post_id_from_path(target);
            return handle_api_like(id);
        } else {
            return not_found(target);
        }
    }
    
    // 静态文件服务
    return serve_file(req, target);
}

http::response<http::string_body> RouteHandler::handle_api_submit(const http::request<http::string_body>& req) {
    try {
        // 解析Content-Type获取boundary
        std::string content_type = std::string(req[http::field::content_type]);
        std::string boundary;
        
        size_t boundary_pos = content_type.find("boundary=");
        if (boundary_pos != std::string::npos) {
            boundary = content_type.substr(boundary_pos + 9);
            // 移除可能的引号
            if (boundary.front() == '"') boundary.erase(0, 1);
            if (boundary.back() == '"') boundary.pop_back();
        }
        
        if (boundary.empty()) {
            return bad_request("缺少multipart boundary");
        }
        
        // 解析multipart/form-data
        std::string content;
        std::vector<std::string> image_files;
        
        if (!parse_multipart_form(req.body(), boundary, content, image_files)) {
            return bad_request("解析表单数据失败");
        }
        
        // 验证内容长度
        if (!utils::StringUtils::validate_content_length(content, 50)) {
            return bad_request("评论内容不能少于50字");
        }
        
        // 验证图片数量
        if (image_files.size() > 9) {
            return bad_request("最多只能上传9张图片");
        }
        
        // 生成ID
        utils::IdGenerator id_gen;
        std::string post_id = id_gen.generate();
        
        // 创建评论对象
        db::Post post;
        post.id = post_id;
        post.content = content;
        post.image_paths = image_files;
        
        // 保存到数据库
        if (!db_manager->save_post(post)) {
            return server_error("保存评论失败");
        }
        
        // 返回成功响应
        std::string response_data = "{\"id\":\"" + post_id + "\"}";
        return ok_response(utils::JsonUtils::create_success_response(response_data));
        
    } catch (const std::exception& e) {
        std::cerr << "提交评论异常: " << e.what() << std::endl;
        return server_error("服务器内部错误");
    }
}

http::response<http::string_body> RouteHandler::handle_api_view(const std::string& id) {
    try {
        if (id.empty()) {
            return bad_request("评论ID不能为空");
        }
        
        // 增加浏览次数
        db_manager->increment_view_count(id);
        
        // 获取评论
        auto post_opt = db_manager->get_post(id);
        if (!post_opt) {
            return not_found("评论不存在");
        }
        
        db::Post post = *post_opt;
        
        // 构造JSON响应
        std::ostringstream json_data;
        json_data << "{"
                  << "\"id\":\"" << utils::JsonUtils::escape_json_string(post.id) << "\","
                  << "\"content\":\"" << utils::JsonUtils::escape_json_string(post.content) << "\","
                  << "\"created_at\":\"" << utils::JsonUtils::escape_json_string(post.created_at) << "\","
                  << "\"view_count\":" << post.view_count << ","
                  << "\"like_count\":" << post.like_count << ","
                  << "\"images\":[";
        
        for (size_t i = 0; i < post.image_paths.size(); ++i) {
            if (i > 0) json_data << ",";
            json_data << "\"" << utils::JsonUtils::escape_json_string(post.image_paths[i]) << "\"";
        }
        json_data << "]}";
        
        return ok_response(utils::JsonUtils::create_success_response(json_data.str()));
        
    } catch (const std::exception& e) {
        std::cerr << "查看评论异常: " << e.what() << std::endl;
        return server_error("服务器内部错误");
    }
}

http::response<http::string_body> RouteHandler::handle_api_like(const std::string& id) {
    try {
        if (id.empty()) {
            return bad_request("评论ID不能为空");
        }
        
        // 增加点赞次数
        if (!db_manager->increment_like_count(id)) {
            return not_found("评论不存在");
        }
        
        return ok_response(utils::JsonUtils::create_success_response());
        
    } catch (const std::exception& e) {
        std::cerr << "点赞异常: " << e.what() << std::endl;
        return server_error("服务器内部错误");
    }
}

template<class Body, class Allocator>
http::message_generator RouteHandler::serve_file(
    const http::request<Body, http::basic_fields<Allocator>>& req,
    const std::string& path) {
    
    // 处理路径
    std::string target = path;
    if (target == "/") {
        target = "/index.html";
    }
    
    // 去掉开头的斜杠
    if (target.front() == '/') {
        target.erase(0, 1);
    }
    
    // 构造完整文件路径
    std::string full_path;
    if (target.starts_with("uploads/")) {
        full_path = target;  // uploads目录直接访问
    } else {
        full_path = "frontend/" + target;  // 前端文件
    }
    
    // 读取文件
    std::ifstream file(full_path, std::ios::binary);
    if (!file.is_open()) {
        return not_found(target);
    }
    
    // 读取文件内容
    std::string content((std::istreambuf_iterator<char>(file)),
                       std::istreambuf_iterator<char>());
    file.close();
    
    // 创建响应
    http::response<http::string_body> res{http::status::ok, req.version()};
    res.set(http::field::server, "CommentFree/1.0");
    res.set(http::field::content_type, server::mime_type(full_path));
    res.body() = std::move(content);
    res.prepare_payload();
    
    add_cors_headers(res);
    
    return res;
}

std::string RouteHandler::extract_post_id_from_path(const std::string& path) {
    // 从路径中提取ID，例如 "/api/view/word42" -> "word42"
    size_t last_slash = path.find_last_of('/');
    if (last_slash != std::string::npos && last_slash < path.length() - 1) {
        return path.substr(last_slash + 1);
    }
    return "";
}

bool RouteHandler::parse_multipart_form(const std::string& body, const std::string& boundary,
                                       std::string& content, std::vector<std::string>& image_files) {
    // 简化的multipart解析（实际项目中建议使用专门的库）
    std::string delimiter = "--" + boundary;
    size_t pos = 0;
    
    while (pos < body.length()) {
        size_t start = body.find(delimiter, pos);
        if (start == std::string::npos) break;
        
        start += delimiter.length();
        size_t end = body.find(delimiter, start);
        if (end == std::string::npos) end = body.length();
        
        std::string part = body.substr(start, end - start);
        
        // 分离headers和content
        size_t header_end = part.find("\r\n\r\n");
        if (header_end == std::string::npos) {
            pos = end;
            continue;
        }
        
        std::string headers = part.substr(0, header_end);
        std::string part_content = part.substr(header_end + 4);
        
        // 移除末尾的\r\n
        if (part_content.size() >= 2 && part_content.substr(part_content.size() - 2) == "\r\n") {
            part_content = part_content.substr(0, part_content.size() - 2);
        }
        
        // 解析headers
        if (headers.find("name=\"content\"") != std::string::npos) {
            content = part_content;
        } else if (headers.find("name=\"images\"") != std::string::npos && 
                  headers.find("filename=") != std::string::npos) {
            
            // 提取文件名
            size_t filename_pos = headers.find("filename=\"");
            if (filename_pos != std::string::npos) {
                filename_pos += 10;
                size_t filename_end = headers.find("\"", filename_pos);
                if (filename_end != std::string::npos) {
                    std::string filename = headers.substr(filename_pos, filename_end - filename_pos);
                    
                    // 验证文件格式
                    if (utils::FileHandler::validate_image_format(filename)) {
                        // 保存文件
                        std::string saved_path = utils::FileHandler::save_uploaded_file(part_content, filename);
                        if (!saved_path.empty()) {
                            image_files.push_back(saved_path);
                        }
                    }
                }
            }
        }
        
        pos = end;
    }
    
    return !content.empty();
}

std::string RouteHandler::create_json_response(const std::string& status, const std::string& message, 
                                             const std::string& data) {
    std::ostringstream json;
    json << "{\"status\":\"" << status << "\"";
    if (!message.empty()) {
        json << ",\"message\":\"" << utils::JsonUtils::escape_json_string(message) << "\"";
    }
    if (!data.empty()) {
        json << ",\"data\":" << data;
    }
    json << "}";
    return json.str();
}

http::response<http::string_body> RouteHandler::bad_request(const std::string& why) {
    http::response<http::string_body> res{http::status::bad_request, 11};
    res.set(http::field::server, "CommentFree/1.0");
    res.set(http::field::content_type, "application/json");
    res.body() = utils::JsonUtils::create_error_response(why);
    res.prepare_payload();
    add_cors_headers(res);
    return res;
}

http::response<http::string_body> RouteHandler::not_found(const std::string& target) {
    http::response<http::string_body> res{http::status::not_found, 11};
    res.set(http::field::server, "CommentFree/1.0");
    res.set(http::field::content_type, "application/json");
    res.body() = utils::JsonUtils::create_error_response("资源不存在: " + target);
    res.prepare_payload();
    add_cors_headers(res);
    return res;
}

http::response<http::string_body> RouteHandler::server_error(const std::string& what) {
    http::response<http::string_body> res{http::status::internal_server_error, 11};
    res.set(http::field::server, "CommentFree/1.0");
    res.set(http::field::content_type, "application/json");
    res.body() = utils::JsonUtils::create_error_response(what);
    res.prepare_payload();
    add_cors_headers(res);
    return res;
}

http::response<http::string_body> RouteHandler::ok_response(const std::string& content, 
                                                          const std::string& content_type) {
    http::response<http::string_body> res{http::status::ok, 11};
    res.set(http::field::server, "CommentFree/1.0");
    res.set(http::field::content_type, content_type);
    res.body() = content;
    res.prepare_payload();
    add_cors_headers(res);
    return res;
}

template<class Body>
void RouteHandler::add_cors_headers(http::response<Body>& res) {
    res.set(http::field::access_control_allow_origin, "*");
    res.set(http::field::access_control_allow_methods, "GET, POST, OPTIONS");
    res.set(http::field::access_control_allow_headers, "Content-Type");
}

// 显式实例化模板
template http::message_generator RouteHandler::handle_request<http::string_body, std::allocator<char>>(
    http::request<http::string_body, http::basic_fields<std::allocator<char>>>&& req,
    const std::string& doc_root);

template http::message_generator RouteHandler::serve_file<http::string_body, std::allocator<char>>(
    const http::request<http::string_body, http::basic_fields<std::allocator<char>>>& req,
    const std::string& path);

} // namespace routes