cmake_minimum_required(VERSION 3.20)

project(CommentFree 
    VERSION 1.0.0
    DESCRIPTION "A simple comment system with C++ backend and HTML frontend"
    LANGUAGES CXX
)

# 设置C++20标准
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 设置构建类型
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# 编译选项
if(MSVC)
    add_compile_options(/W4)
else()
    add_compile_options(-Wall -Wextra -Wpedantic)
endif()


# 添加子项目
add_subdirectory(backend)

# 创建安装目录结构
install(DIRECTORY frontend/ DESTINATION share/commentfree/frontend)
install(DIRECTORY sql/ DESTINATION share/commentfree/sql)
install(DIRECTORY uploads/ DESTINATION share/commentfree/uploads OPTIONAL)
install(DIRECTORY data/ DESTINATION share/commentfree/data OPTIONAL)
install(FILES README.md DESTINATION share/commentfree)

# 打包配置
set(CPACK_PACKAGE_NAME "CommentFree")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "${PROJECT_DESCRIPTION}")
set(CPACK_PACKAGE_VENDOR "CommentFree Team")
set(CPACK_PACKAGE_CONTACT "admin@commentfree.com")

# 平台特定的打包设置
if(WIN32)
    set(CPACK_GENERATOR "NSIS;ZIP")
    set(CPACK_NSIS_DISPLAY_NAME "CommentFree")
    set(CPACK_NSIS_PACKAGE_NAME "CommentFree")
    set(CPACK_NSIS_CONTACT "admin@commentfree.com")
    set(CPACK_NSIS_HELP_LINK "https://github.com/commentfree/commentfree")
    set(CPACK_NSIS_URL_INFO_ABOUT "https://github.com/commentfree/commentfree")
else()
    set(CPACK_GENERATOR "TGZ;DEB;RPM")
    set(CPACK_DEBIAN_PACKAGE_DEPENDS "libboost-all-dev, libpqxx-dev, postgresql-client")
    set(CPACK_RPM_PACKAGE_REQUIRES "boost-devel, libpqxx-devel, postgresql")
endif()

include(CPack)

# 自定义目标
add_custom_target(run
    COMMAND ${CMAKE_BINARY_DIR}/backend/bin/commentfree_server
    DEPENDS comment_free_backend
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    COMMENT "Running CommentFree server..."
)

add_custom_target(clean-all
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${CMAKE_BINARY_DIR}
    COMMENT "Cleaning all build files..."
)

file(COPY ${CMAKE_SOURCE_DIR}/frontend DESTINATION ${CMAKE_BINARY_DIR})

# 显示配置信息
message(STATUS "")
message(STATUS "=== CommentFree Project Configuration ===")
message(STATUS "Project: ${PROJECT_NAME}")
message(STATUS "Version: ${PROJECT_VERSION}")
message(STATUS "Build Type: ${CMAKE_BUILD_TYPE}")
message(STATUS "C++ Standard: ${CMAKE_CXX_STANDARD}")
message(STATUS "Install Prefix: ${CMAKE_INSTALL_PREFIX}")
message(STATUS "Source Directory: ${CMAKE_SOURCE_DIR}")
message(STATUS "Binary Directory: ${CMAKE_BINARY_DIR}")