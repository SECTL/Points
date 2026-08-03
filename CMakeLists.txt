cmake_minimum_required(VERSION 3.10)
project(Points VERSION 1.0.0)

set(CMAKE_BUILD_TYPE Debug)

# 指定 C++ 标准
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# 导出编译命令，让 VS Code 智能提示能正确识别宏
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# 将版本号拆分成整数，供 .rc 文件使用
string(REPLACE "." ";" VERSION_LIST ${PROJECT_VERSION})
list(GET VERSION_LIST 0 VERSION_MAJOR)
list(GET VERSION_LIST 1 VERSION_MINOR)
list(GET VERSION_LIST 2 VERSION_PATCH)


# 告诉 CMake，我们的源代码都在 src 目录下
file(GLOB SOURCES "src/*.cpp")

# 生成可执行文件，名字叫 Points
add_executable(Points ${SOURCES})
# 配置 .rc 文件
configure_file(resource/version.rc.in ${CMAKE_CURRENT_BINARY_DIR}/version.rc @ONLY)
target_sources(Points PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/version.rc)

# 设置 Debug 模式下定义 _DEBUG，Release 模式下不定义
target_compile_definitions(Points PRIVATE
    $<$<CONFIG:Debug>:_DEBUG>
    $<$<CONFIG:Release>:NDEBUG>
)

# 把版本号作为宏传给 C++ 源码，这样 main.cpp 也能拿到
target_compile_definitions(Points PRIVATE APP_VERSION="${PROJECT_VERSION}")

# 链接配置
target_include_directories(Points PRIVATE include)
target_link_directories(Points PRIVATE lib)
target_link_libraries(Points
    libcurl
    ws2_32
    wldap32
    crypt32
)