#include "utils.hpp"
#include <sstream>
#include <iomanip>
#include <fstream>
#include <algorithm>
#include <cctype>
#include <chrono>
#include <filesystem>
#include <iostream>

namespace utils {

const std::vector<std::string> IdGenerator::words = {
    "apple", "beach", "cloud", "dream", "eagle", "flame", "grace", "happy",
    "ideal", "joker", "knife", "light", "magic", "night", "ocean", "peace",
    "queen", "river", "smile", "tiger", "unity", "voice", "water", "xenon",
    "youth", "zebra", "brave", "clean", "dance", "earth", "fresh", "green",
    "heart", "inbox", "juice", "kind", "lucky", "money", "noble", "order",
    "piano", "quiet", "rapid", "sweet", "trust", "upper", "vital", "world"
};

IdGenerator::IdGenerator() 
    : generator(std::chrono::steady_clock::now().time_since_epoch().count()),
      word_dist(0, words.size() - 1),
      num_dist(1, 999) {
}

std::string IdGenerator::generate() {
    std::string word = words[word_dist(generator)];
    int number = num_dist(generator);
    return word + std::to_string(number);
}

bool FileHandler::validate_file_size(const std::string& filepath, size_t max_size) {
    try {
        std::filesystem::path p(filepath);
        if (!std::filesystem::exists(p)) {
            return false;
        }
        return std::filesystem::file_size(p) <= max_size;
    } catch (const std::exception&) {
        return false;
    }
}

bool FileHandler::validate_image_format(const std::string& filename) {
    // 转换为小写进行比较
    std::string lower_filename = filename;
    std::transform(lower_filename.begin(), lower_filename.end(), lower_filename.begin(), ::tolower);
    
    // 支持的图片格式
    std::vector<std::string> valid_extensions = {".jpg", ".jpeg", ".png", ".gif", ".bmp", ".webp"};
    
    for (const auto& ext : valid_extensions) {
        if (lower_filename.size() >= ext.size() && 
            lower_filename.substr(lower_filename.size() - ext.size()) == ext) {
            return true;
        }
    }
    return false;
}

std::string FileHandler::save_uploaded_file(const std::string& content, const std::string& filename) {
    try {
        // 确保uploads目录存在
        if (!ensure_directory("uploads")) {
            return "";
        }
        
        // 生成唯一的文件名
        auto now = std::chrono::system_clock::now();
        auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count();
        
        std::string file_extension = "";
        size_t dot_pos = filename.find_last_of('.');
        if (dot_pos != std::string::npos) {
            file_extension = filename.substr(dot_pos);
        }
        
        std::string new_filename = "img_" + std::to_string(timestamp) + file_extension;
        std::string filepath = "uploads/" + new_filename;
        
        // 写入文件
        std::ofstream file(filepath, std::ios::binary);
        if (!file.is_open()) {
            return "";
        }
        
        file.write(content.c_str(), content.size());
        file.close();
        
        return filepath;
    } catch (const std::exception&) {
        return "";
    }
}

bool FileHandler::ensure_directory(const std::string& path) {
    try {
        std::filesystem::path dir(path);
        if (!std::filesystem::exists(dir)) {
            return std::filesystem::create_directories(dir);
        }
        return std::filesystem::is_directory(dir);
    } catch (const std::exception& e) {
        std::cout << "mkdir error: "<< "path=" << path << ", error=" << e.what() << std::endl;
        return false;
    }
}

std::string StringUtils::url_encode(const std::string& value) {
    std::ostringstream escaped;
    escaped.fill('0');
    escaped << std::hex;

    for (std::string::const_iterator i = value.begin(), n = value.end(); i != n; ++i) {
        std::string::value_type c = (*i);
        
        // Keep alphanumeric and other accepted characters intact
        if (isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~') {
            escaped << c;
            continue;
        }
        
        // Any other characters are percent-encoded
        escaped << std::uppercase;
        escaped << '%' << std::setw(2) << int((unsigned char) c);
        escaped << std::nouppercase;
    }

    return escaped.str();
}

std::string StringUtils::url_decode(const std::string& value) {
    std::string result;
    result.reserve(value.length());
    
    for (size_t i = 0; i < value.length(); ++i) {
        if (value[i] == '%' && i + 2 < value.length()) {
            std::string hex = value.substr(i + 1, 2);
            char c = static_cast<char>(std::stoul(hex, nullptr, 16));
            result += c;
            i += 2;
        } else if (value[i] == '+') {
            result += ' ';
        } else {
            result += value[i];
        }
    }
    
    return result;
}

std::string StringUtils::trim(const std::string& str) {
    size_t start = str.find_first_not_of(" \t\r\n");
    if (start == std::string::npos) {
        return "";
    }
    size_t end = str.find_last_not_of(" \t\r\n");
    return str.substr(start, end - start + 1);
}

bool StringUtils::validate_content_length(const std::string& content, size_t min_length) {
    std::string trimmed = trim(content);
    return trimmed.length() >= min_length;
}

std::string JsonUtils::escape_json_string(const std::string& input) {
    std::string output;
    output.reserve(input.length() * 2);
    
    for (char c : input) {
        switch (c) {
            case '"':  output += "\\\""; break;
            case '\\': output += "\\\\"; break;
            case '\b': output += "\\b";  break;
            case '\f': output += "\\f";  break;
            case '\n': output += "\\n";  break;
            case '\r': output += "\\r";  break;
            case '\t': output += "\\t";  break;
            default:
                if (c < 0x20) {
                    output += "\\u";
                    output += "0000";
                    std::ostringstream oss;
                    oss << std::hex << std::setw(4) << std::setfill('0') << static_cast<int>(c);
                    std::string hex = oss.str();
                    output.replace(output.length() - 4, 4, hex);
                } else {
                    output += c;
                }
                break;
        }
    }
    
    return output;
}

std::string JsonUtils::create_error_response(const std::string& message) {
    return "{\"status\":\"error\",\"message\":\"" + escape_json_string(message) + "\"}";
}

std::string JsonUtils::create_success_response(const std::string& data) {
    if (data.empty()) {
        return "{\"status\":\"success\"}";
    }
    return "{\"status\":\"success\",\"data\":" + data + "}";
}

} // namespace utils