cmake_minimum_required(VERSION 3.20)

# 设置C++20标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置CMake策略
if(POLICY CMP0144)
    cmake_policy(SET CMP0144 NEW)
endif()
if(POLICY CMP0167)
    cmake_policy(SET CMP0167 NEW)
endif()

# 定义项目
set(PROJECT_NAME comment_free_backend)
project(${PROJECT_NAME})

# 设置输出目录
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# 平台特定配置
if(WIN32)
    # Windows平台：使用vcpkg或手动配置
    find_package(Boost REQUIRED COMPONENTS system filesystem json)
else()
    # Linux/macOS平台：使用pkg-config
    find_package(PkgConfig REQUIRED)
    find_package(Boost REQUIRED COMPONENTS system filesystem json)
    pkg_check_modules(PQXX REQUIRED libpqxx)
endif()

# 添加源文件
set(SOURCES
    main.cpp
    server/utils.cpp
    server/db.cpp
    server/http_server.cpp
    server/routes.cpp
)

# 添加头文件
set(HEADERS
    server/utils.hpp
    server/db.hpp
    server/http_server.hpp
    server/routes.hpp
)

# 创建可执行文件
add_executable(${PROJECT_NAME} ${SOURCES} ${HEADERS})

# 平台特定的链接配置
if(WIN32)
    # Windows平台链接
    target_link_libraries(${PROJECT_NAME} 
        ${Boost_LIBRARIES}
        ws2_32 
        wsock32
    )
    
    # Windows编译选项
    target_compile_options(${PROJECT_NAME} PRIVATE
        /W4
        /std:c++20
    )
    
    # Windows包含目录
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${Boost_INCLUDE_DIRS}
    )
else()
    # Linux/macOS平台链接
    target_link_libraries(${PROJECT_NAME} 
        ${Boost_LIBRARIES}
        ${PQXX_LIBRARIES}
        pthread
    )
    
    # Unix编译选项
    target_compile_options(${PROJECT_NAME} PRIVATE
        ${PQXX_CFLAGS_OTHER}
        -Wall
        -Wextra
        -O2
    )
    
    # Unix包含目录
    target_include_directories(${PROJECT_NAME} PRIVATE
        ${Boost_INCLUDE_DIRS}
        ${PQXX_INCLUDE_DIRS}
    )
endif()

# 设置输出名称
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME "commentfree_server")

# 安装规则
install(TARGETS ${PROJECT_NAME} DESTINATION bin)

# 打印配置信息
message(STATUS "=== CommentFree Backend Configuration ===")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
if(WIN32)
    message(STATUS "Platform: Windows")
    message(STATUS "Using vcpkg packages")
else()
    message(STATUS "Platform: Unix-like")
    message(STATUS "Boost Version: ${Boost_VERSION}")
    message(STATUS "Boost Include: ${Boost_INCLUDE_DIRS}")
    message(STATUS "Boost Libraries: ${Boost_LIBRARIES}")
    message(STATUS "PQXX Include: ${PQXX_INCLUDE_DIRS}")
    message(STATUS "PQXX Libraries: ${PQXX_LIBRARIES}")
endif()
message(STATUS "Output Directory: ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}")
message(STATUS "===========================================")