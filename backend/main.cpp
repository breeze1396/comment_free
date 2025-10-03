#include "server/http_server.hpp"
#include "server/db.hpp"
#include "server/utils.hpp"
#include <iostream>
#include <string>
#include <memory>
#include <signal.h>

// 全局数据库管理器实例（放到server命名空间以供其他翻译单元extern引用）
namespace server {
    std::shared_ptr<db::DatabaseManager> g_db_manager;
}




void print_usage(const char* program_name) {
    std::cout << "用法: " << program_name << " [选项]\n"
              << "选项:\n"
              << "  -h, --help              显示此帮助信息\n"
              << "  -p, --port PORT         设置监听端口 (默认: 8080)\n"
              << "  -a, --address ADDRESS   设置监听地址 (默认: 0.0.0.0)\n"
              << "  -d, --db-conn CONN      设置数据库连接字符串\n"
              << "                          (默认: host=localhost dbname=commentfree user=postgres)\n"
              << "\n示例:\n"
              << "  " << program_name << " -p 9000 -a 127.0.0.1\n"
              << "  " << program_name << " -d \"host=localhost dbname=mydb user=myuser password=mypass\"\n";
}

int main(int argc, char* argv[]) {
    // 默认配置
    std::string address = "0.0.0.0";
    unsigned short port = 8080;
    std::string db_connection = "host=47.108.220.87 dbname=commentfree user=commentfree_user";
    std::string doc_root = "../frontend";
    
    // 解析命令行参数
    for (int i = 1; i < argc; ++i) {
        std::string arg = argv[i];
        
        if (arg == "-h" || arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if (arg == "--port") {
            if (i + 1 < argc) {
                port = static_cast<unsigned short>(std::stoi(argv[++i]));
            } else {
                std::cerr << "错误: 端口参数缺少值" << std::endl;
                return 1;
            }
        } else if (arg == "-a" || arg == "--address") {
            if (i + 1 < argc) {
                address = argv[++i];
            } else {
                std::cerr << "错误: 地址参数缺少值" << std::endl;
                return 1;
            }
        } else if (arg == "-d" || arg == "--db-conn") {
            if (i + 1 < argc) {
                db_connection = argv[++i];
            } else {
                std::cerr << "错误: 数据库连接参数缺少值" << std::endl;
                return 1;
            }
        } else if(arg == "-p") {
            if (i + 1 < argc) {
                db_connection += " password=" + std::string(argv[++i]);
            } else {
                std::cerr << "错误: 数据库连接参数缺少值" << std::endl;
                return 1;
            }
        }
        
        else {
            std::cerr << "错误: 未知参数 " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }
    
    std::cout << "=== CommentFree 评论系统服务器 ===" << std::endl;
    std::cout << "监听地址: " << address << ":" << port << std::endl;
    std::cout << "数据库连接: " << db_connection << std::endl;
    

    
    try {
        // 确保必要目录存在
        if (!utils::FileHandler::ensure_directory("uploads")) {
            std::cerr << "错误: 无法创建uploads目录" << std::endl;
            return 1;
        }
        
        if (!utils::FileHandler::ensure_directory("data")) {
            std::cerr << "错误: 无法创建data目录" << std::endl;
            return 1;
        }
        
    // 初始化数据库连接
    server::g_db_manager = std::make_shared<db::DatabaseManager>(db_connection);
    if (!server::g_db_manager->connect()) {
            std::cerr << "错误: 数据库连接失败" << std::endl;
            std::cerr << "请确保PostgreSQL服务正在运行，并且数据库存在" << std::endl;
            std::cerr << "可以使用以下命令创建数据库:" << std::endl;
            std::cerr << "  createdb commentfree" << std::endl;
            return 1;
        }
        
        std::cout << "数据库连接成功！" << std::endl;
        
        // 创建并启动HTTP服务器
        server::HttpServer http_server(address, port, doc_root);
        
        std::cout << "服务器启动成功！" << std::endl;
        std::cout << "访问地址: http://" << address << ":" << port << std::endl;
        std::cout << "按 Ctrl+C 停止服务器" << std::endl;
        std::cout << std::endl;

        // 设置信号处理
        // 定义静态指针用于信号处理
        static server::HttpServer* g_http_server_ptr = &http_server;
        auto signal_handler = [](int) {
            if (g_http_server_ptr) {
                g_http_server_ptr->stop();
            }
        };
        void (*handler)(int) = [](int) {
            if (g_http_server_ptr) {
                g_http_server_ptr->stop();
            }
        };
        signal(SIGINT, handler);
        signal(SIGTERM, handler);

        // 运行服务器
        http_server.run();
        
    } catch (const std::exception& e) {
        std::cerr << "服务器异常: " << e.what() << std::endl;
        return 1;
    }
    
    // 清理资源
    if (server::g_db_manager) {
        server::g_db_manager->disconnect();
        server::g_db_manager.reset();
    }
    
    std::cout << "服务器已关闭" << std::endl;
    return 0;
}