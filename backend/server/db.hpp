#pragma once

#include <string>
#include <vector>
#include <memory>
#include <optional>
#include <pqxx/pqxx>

namespace db {

// 评论数据结构
struct Post {
    std::string id;
    std::string content;
    std::vector<std::string> image_paths;
    std::string created_at;
    int view_count = 0;
    int like_count = 0;
};

// 数据库连接管理器
class DatabaseManager {
private:
    std::string connection_string;
    std::unique_ptr<pqxx::connection> conn;
    
public:
    DatabaseManager(const std::string& conn_str);
    ~DatabaseManager();
    
    // 连接数据库
    bool connect();
    
    // 断开连接
    void disconnect();
    
    // 检查连接状态
    bool is_connected() const;
    
    // 保存评论
    bool save_post(const Post& post);
    
    // 获取评论
    std::optional<Post> get_post(const std::string& id);
    
    // 增加浏览次数
    bool increment_view_count(const std::string& id);
    
    // 增加点赞次数
    bool increment_like_count(const std::string& id);
    
    // 初始化数据库表
    bool initialize_tables();
    
private:
    // 执行SQL查询
    bool execute_query(const std::string& query);
    
    // 检查表是否存在
    bool table_exists(const std::string& table_name);
};

} // namespace db