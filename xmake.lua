add_rules("mode.debug","mode.release")
set_project("points")

-- 版本号唯一来源：set_version 自动拆出 VERSION/VERSION_MAJOR/VERSION_MINOR/VERSION_ALTER
-- 供 version.rc.in 生成；APP_VERSION 宏供 C++ 源码使用
local APP_VERSION = "1.0.0"
set_version(APP_VERSION)

set_languages("c++20")
-- gcc 工具链下不声明包：xmake 包缓存会把 MSVC 编译的 libcurl 串给 gcc（__GSHandlerCheck 链接失败），
-- 且 points_modernize（模块/反射目标）不用任何包；old 目标保持 clang-cl 编译
if get_config("toolchain") ~= "gcc" then
    add_requires("nlohmann_json","libcurl")
end

target("points_main_old")
    set_kind("binary")
    add_files("old/main.cpp")
    if get_config("toolchain") ~= "gcc" then
        add_packages("nlohmann_json", "libcurl")
    end
    add_defines(string.format('APP_VERSION="%s"', APP_VERSION))
    -- /utf-8 是 MSVC/clang-cl 专用；GCC 默认按 UTF-8 读源码
    if get_config("toolchain") ~= "gcc" then
        add_cxflags("/utf-8", {force = true})  -- main.cpp 是 UTF-8 无 BOM，MSVC 默认按 GBK 读
    end

    -- 1. 生成 version.rc：set_version 自动提供 VERSION/VERSION_MAJOR/VERSION_MINOR/VERSION_ALTER
    add_configfiles("resource/version.rc.in", {pattern = "@([%w_]+)@"})

    -- 2. 生成的 rc 不会自动编译，必须手动加进源文件
    add_files("$(builddir)/version.rc", {always_added = true})

target("points_modernize")
    set_kind("binary")
    add_files("src/storage_management.ixx")
    add_files("src/exceptions.ixx")
    add_files("src/storage_management.cpp")
    add_defines(string.format('APP_VERSION="%s"', APP_VERSION))
    -- /utf-8 是 MSVC/clang-cl 专用；GCC 默认按 UTF-8 读源码
    if get_config("toolchain") ~= "gcc" then
        add_cxflags("/utf-8", {force = true})
    end
    -- GCC 16.1+ 启用 C++26 反射（P2996，-freflection）；源码侧用 __cpp_impl_reflection 自动检测
    -- 注：xmake 3.0 DSL 无进程执行/版本探测 API，不解析 gcc 版本；gcc < 16 会因 -freflection 明确报错
    if get_config("toolchain") == "gcc" then
        add_cxflags("-std=c++26", "-freflection", {force = true})
    end
