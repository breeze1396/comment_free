#include "db.hpp"
#include <iostream>
#include <sstream>

namespace db {

DatabaseManager::DatabaseManager(const std::string& conn_str) 
    : connection_string(conn_str) {
}

DatabaseManager::~DatabaseManager() {
    disconnect();
}

bool DatabaseManager::connect() {
    try {
        conn = std::make_unique<pqxx::connection>(connection_string);
        if (conn->is_open()) {
            std::cout << "数据库连接成功: " << conn->dbname() << std::endl;
            return initialize_tables();
        }
    } catch (const std::exception& e) {
        std::cerr << "数据库连接失败: " << e.what() << std::endl;
        conn.reset();
    }
    return false;
}

void DatabaseManager::disconnect() {
    if (conn && conn->is_open()) {
        conn->close();
        conn.reset();
    }
}

bool DatabaseManager::is_connected() const {
    return conn && conn->is_open();
}

bool DatabaseManager::save_post(const Post& post) {
    if (!is_connected()) {
        return false;
    }
    
    try {
        pqxx::work txn(*conn);
        
        // 插入评论主体
        std::string query = "INSERT INTO posts (id, content, created_at) VALUES ($1, $2, NOW())";
        txn.exec_params(query, post.id, post.content);
        
        // 插入图片路径
        for (const auto& image_path : post.image_paths) {
            std::string img_query = "INSERT INTO post_images (post_id, path) VALUES ($1, $2)";
            txn.exec_params(img_query, post.id, image_path);
        }
        
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "保存评论失败: " << e.what() << std::endl;
        return false;
    }
}

std::optional<Post> DatabaseManager::get_post(const std::string& id) {
    if (!is_connected()) {
        return std::nullopt;
    }
    
    try {
        pqxx::nontransaction txn(*conn);
        
        // 获取评论主体
        std::string query = "SELECT id, content, created_at, view_count, like_count FROM posts WHERE id = $1";
        pqxx::result result = txn.exec_params(query, id);
        
        if (result.empty()) {
            return std::nullopt;
        }
        
        Post post;
        auto row = result[0];
        post.id = row["id"].as<std::string>();
        post.content = row["content"].as<std::string>();
        post.created_at = row["created_at"].as<std::string>();
        post.view_count = row["view_count"].as<int>(0);
        post.like_count = row["like_count"].as<int>(0);
        
        // 获取图片路径
        std::string img_query = "SELECT path FROM post_images WHERE post_id = $1 ORDER BY id";
        pqxx::result img_result = txn.exec_params(img_query, id);
        
        for (const auto& img_row : img_result) {
            post.image_paths.push_back(img_row["path"].as<std::string>());
        }
        
        return post;
    } catch (const std::exception& e) {
        std::cerr << "获取评论失败: " << e.what() << std::endl;
        return std::nullopt;
    }
}

bool DatabaseManager::increment_view_count(const std::string& id) {
    if (!is_connected()) {
        return false;
    }
    
    try {
        pqxx::work txn(*conn);
        std::string query = "UPDATE posts SET view_count = COALESCE(view_count, 0) + 1 WHERE id = $1";
        auto result = txn.exec_params(query, id);
        txn.commit();
        return result.affected_rows() > 0;
    } catch (const std::exception& e) {
        std::cerr << "增加浏览次数失败: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::increment_like_count(const std::string& id) {
    if (!is_connected()) {
        return false;
    }
    
    try {
        pqxx::work txn(*conn);
        std::string query = "UPDATE posts SET like_count = COALESCE(like_count, 0) + 1 WHERE id = $1";
        auto result = txn.exec_params(query, id);
        txn.commit();
        return result.affected_rows() > 0;
    } catch (const std::exception& e) {
        std::cerr << "增加点赞次数失败: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::initialize_tables() {
    if (!is_connected()) {
        return false;
    }
    
    try {
        pqxx::work txn(*conn);
        
        // 创建posts表
        std::string create_posts = R"(
            CREATE TABLE IF NOT EXISTS posts (
                id VARCHAR(16) PRIMARY KEY,
                content TEXT NOT NULL,
                created_at TIMESTAMP DEFAULT NOW(),
                view_count INTEGER DEFAULT 0,
                like_count INTEGER DEFAULT 0
            )
        )";
        txn.exec(create_posts);
        
        // 创建post_images表
        std::string create_images = R"(
            CREATE TABLE IF NOT EXISTS post_images (
                id SERIAL PRIMARY KEY,
                post_id VARCHAR(16) REFERENCES posts(id) ON DELETE CASCADE,
                path TEXT NOT NULL
            )
        )";
        txn.exec(create_images);
        
        // 创建索引以提高查询性能
        txn.exec("CREATE INDEX IF NOT EXISTS idx_posts_created_at ON posts(created_at)");
        txn.exec("CREATE INDEX IF NOT EXISTS idx_post_images_post_id ON post_images(post_id)");
        
        txn.commit();
        std::cout << "数据库表初始化成功" << std::endl;
        return true;
    } catch (const std::exception& e) {
        std::cerr << "数据库表初始化失败: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::execute_query(const std::string& query) {
    if (!is_connected()) {
        return false;
    }
    
    try {
        pqxx::work txn(*conn);
        txn.exec(query);
        txn.commit();
        return true;
    } catch (const std::exception& e) {
        std::cerr << "执行查询失败: " << e.what() << std::endl;
        return false;
    }
}

bool DatabaseManager::table_exists(const std::string& table_name) {
    if (!is_connected()) {
        return false;
    }
    
    try {
        pqxx::nontransaction txn(*conn);
        std::string query = "SELECT EXISTS (SELECT FROM information_schema.tables WHERE table_name = $1)";
        pqxx::result result = txn.exec_params(query, table_name);
        return result[0][0].as<bool>();
    } catch (const std::exception& e) {
        std::cerr << "检查表存在性失败: " << e.what() << std::endl;
        return false;
    }
}

} // namespace db