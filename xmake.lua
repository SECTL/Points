add_rules("mode.debug","mode.release")
set_project("points")

-- 版本号唯一来源：set_version 自动拆出 VERSION/VERSION_MAJOR/VERSION_MINOR/VERSION_ALTER
-- 供 version.rc.in 生成；APP_VERSION 宏供 C++ 源码使用
local APP_VERSION = "1.0.0"
set_version(APP_VERSION)

set_languages("c++20")
add_requires("nlohmann_json","libcurl")

target("points_main_old")
    set_kind("binary")
    add_files("old/main.cpp")
    add_packages("nlohmann_json", "libcurl")
    add_defines(string.format('APP_VERSION="%s"', APP_VERSION))
    if is_plat("windows") then
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
    if is_plat("windows") then
        add_cxflags("/utf-8", {force = true})  -- main.cpp 是 UTF-8 无 BOM，MSVC 默认按 GBK 读
    end