#!/bin/bash

# CommentFree 快速构建和运行脚本
# 适用于 Linux/macOS 系统

set -e  # 遇到错误立即退出

echo "========================================"
echo "CommentFree 快速构建脚本"
echo "========================================"

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
NC='\033[0m' # No Color

# 打印彩色消息
print_info() {
    echo -e "${BLUE}[INFO]${NC} $1"
}

print_success() {
    echo -e "${GREEN}[SUCCESS]${NC} $1"
}

print_warning() {
    echo -e "${YELLOW}[WARNING]${NC} $1"
}

print_error() {
    echo -e "${RED}[ERROR]${NC} $1"
}

# 检查依赖
check_dependencies() {
    print_info "检查系统依赖..."
    
    # 检查编译器
    if ! command -v g++ &> /dev/null; then
        print_error "未找到 g++ 编译器，请先安装"
        exit 1
    fi
    
    # 检查CMake
    if ! command -v cmake &> /dev/null; then
        print_error "未找到 cmake，请先安装"
        exit 1
    fi
    
    # 检查PostgreSQL
    if ! command -v psql &> /dev/null; then
        print_warning "未找到 psql 命令，请确保已安装 PostgreSQL"
    fi
    
    print_success "基本依赖检查通过"
}

# 安装依赖（Ubuntu/Debian）
install_dependencies_ubuntu() {
    print_info "安装 Ubuntu/Debian 依赖..."
    sudo apt update
    sudo apt install -y build-essential cmake git
    sudo apt install -y libboost-all-dev
    sudo apt install -y postgresql postgresql-contrib libpqxx-dev
    print_success "依赖安装完成"
}

# 安装依赖（CentOS/RHEL/Fedora）
install_dependencies_centos() {
    print_info "安装 CentOS/RHEL/Fedora 依赖..."
    sudo dnf install -y gcc-c++ cmake git
    sudo dnf install -y boost-devel
    sudo dnf install -y postgresql-server postgresql-contrib libpqxx-devel
    print_success "依赖安装完成"
}

# 安装依赖（macOS）
install_dependencies_macos() {
    print_info "安装 macOS 依赖..."
    if ! command -v brew &> /dev/null; then
        print_error "未找到 Homebrew，请先安装：https://brew.sh/"
        exit 1
    fi
    brew install cmake boost postgresql libpqxx
    print_success "依赖安装完成"
}

# 自动检测并安装依赖
auto_install_dependencies() {
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        if command -v apt &> /dev/null; then
            install_dependencies_ubuntu
        elif command -v dnf &> /dev/null; then
            install_dependencies_centos
        else
            print_warning "未识别的 Linux 发行版，请手动安装依赖"
        fi
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        install_dependencies_macos
    else
        print_warning "未识别的操作系统，请手动安装依赖"
    fi
}

# 配置数据库
setup_database() {
    print_info "配置数据库..."
    
    # 启动PostgreSQL服务
    if [[ "$OSTYPE" == "linux-gnu"* ]]; then
        sudo systemctl start postgresql || true
        sudo systemctl enable postgresql || true
    elif [[ "$OSTYPE" == "darwin"* ]]; then
        brew services start postgresql || true
    fi
    
    # 等待服务启动
    sleep 2
    
    # 创建数据库（如果不存在）
    print_info "创建数据库..."
    if sudo -u postgres psql -lqt | cut -d \| -f 1 | grep -qw commentfree; then
        print_info "数据库 commentfree 已存在"
    else
        sudo -u postgres createdb commentfree
        print_success "数据库 commentfree 创建成功"
    fi
    
    # 初始化表结构
    print_info "初始化数据库表..."
    sudo -u postgres psql -d commentfree -f sql/create_tables.sql
    print_success "数据库表初始化完成"
}

# 编译项目
build_project() {
    print_info "编译项目..."
    
    # 创建构建目录
    if [ -d "build" ]; then
        print_info "清理旧的构建目录..."
        rm -rf build
    fi
    
    mkdir build
    cd build
    
    # 配置项目
    print_info "配置项目..."
    cmake .. -DCMAKE_BUILD_TYPE=Release
    
    # 编译
    print_info "开始编译..."
    make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu 2>/dev/null || echo 4)
    
    cd ..
    print_success "项目编译完成"
}

# 运行服务器
run_server() {
    print_info "启动服务器..."
    
    # 确保目录存在
    mkdir -p uploads data
    
    # 设置数据库连接字符串
    DB_CONN="host=localhost dbname=commentfree user=postgres"
    
    print_success "服务器准备就绪！"
    print_info "访问地址："
    echo "  首页: http://localhost:8080/index.html"
    echo "  写评论: http://localhost:8080/write.html"
    echo ""
    print_info "按 Ctrl+C 停止服务器"
    echo ""
    
    # 运行服务器
    ./build/backend/bin/commentfree_server \
        --port 8080 \
        --address 0.0.0.0 \
        --db-conn "$DB_CONN"
}

# 主菜单
show_menu() {
    echo ""
    echo "请选择要执行的操作："
    echo "1) 安装依赖"
    echo "2) 配置数据库"
    echo "3) 编译项目"
    echo "4) 运行服务器"
    echo "5) 一键安装并运行"
    echo "6) 退出"
    echo ""
    read -p "请输入选项 (1-6): " choice
}

# 一键安装并运行
one_click_setup() {
    print_info "开始一键安装..."
    check_dependencies
    auto_install_dependencies
    setup_database
    build_project
    run_server
}

# 主程序
main() {
    # 检查是否在项目根目录
    if [ ! -f "CMakeLists.txt" ] || [ ! -d "backend" ]; then
        print_error "请在项目根目录下运行此脚本"
        exit 1
    fi
    
    # 如果有命令行参数，直接执行对应操作
    case "$1" in
        "deps")
            auto_install_dependencies
            ;;
        "db")
            setup_database
            ;;
        "build")
            build_project
            ;;
        "run")
            run_server
            ;;
        "all")
            one_click_setup
            ;;
        *)
            # 交互式菜单
            while true; do
                show_menu
                case $choice in
                    1)
                        auto_install_dependencies
                        ;;
                    2)
                        setup_database
                        ;;
                    3)
                        build_project
                        ;;
                    4)
                        run_server
                        ;;
                    5)
                        one_click_setup
                        break
                        ;;
                    6)
                        print_info "再见！"
                        exit 0
                        ;;
                    *)
                        print_error "无效选项，请重新选择"
                        ;;
                esac
                echo ""
                read -p "按 Enter 键继续..."
            done
            ;;
    esac
}

# 捕获中断信号
trap 'print_info "操作被中断"; exit 1' INT

# 执行主程序
main "$@"