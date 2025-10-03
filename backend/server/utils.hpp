#pragma once

#include <string>
#include <vector>
#include <random>
#include <boost/filesystem.hpp>

namespace utils {

// ID生成器 - 生成易记的ID（如 word42）
class IdGenerator {
private:
    static const std::vector<std::string> words;
    std::mt19937 generator;
    std::uniform_int_distribution<> word_dist;
    std::uniform_int_distribution<> num_dist;

public:
    IdGenerator();
    std::string generate();
};

// 文件处理工具
class FileHandler {
public:
    // 验证文件大小（最大1MB）
    static bool validate_file_size(const std::string& filepath, size_t max_size = 1024 * 1024);
    
    // 验证文件格式（只允许常见图片格式）
    static bool validate_image_format(const std::string& filename);
    
    // 保存上传的文件
    static std::string save_uploaded_file(const std::string& content, const std::string& filename);
    
    // 创建目录（如果不存在）
    static bool ensure_directory(const std::string& path);
};

// 字符串工具
class StringUtils {
public:
    // URL编码/解码
    static std::string url_encode(const std::string& value);
    static std::string url_decode(const std::string& value);
    
    // 去除首尾空白
    static std::string trim(const std::string& str);
    
    // 验证文本长度（至少50字）
    static bool validate_content_length(const std::string& content, size_t min_length = 50);
};

// JSON工具
class JsonUtils {
public:
    // 转义JSON字符串
    static std::string escape_json_string(const std::string& input);
    
    // 创建错误响应JSON
    static std::string create_error_response(const std::string& message);
    
    // 创建成功响应JSON
    static std::string create_success_response(const std::string& data = "");
};

} // namespace utils