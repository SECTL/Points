add_rules("mode.debug","mode.release")
set_project("points")

-- 版本号唯一来源：set_version 自动拆出 VERSION/VERSION_MAJOR/VERSION_MINOR/VERSION_ALTER
-- 供 version.rc.in 生成；APP_VERSION 宏供 C++ 源码使用
local APP_VERSION = "1.0.0"
set_version(APP_VERSION)

set_languages("c++20")
-- gcc 工具链下不声明包：xmake 包缓存会把 MSVC 编译的 libcurl 串给 gcc（__GSHandlerCheck 链接失败），
-- 且 points_modernize（模块/反射目标）不用任何包；old 目标保持 clang-cl 编译
-- 本地仓库（Slint 与 Material 组件库）
add_repositories("local-repo repo")
add_requires("slint")
add_requires("slint-material")
if is_plat("windows") then
    add_requires("nlohmann_json", "libcurl")
end

if is_plat("windows") then
target("points_main")
    set_kind("binary")
    add_files("old/main.cpp")
    add_packages("nlohmann_json", "libcurl")
    add_defines(string.format('APP_VERSION="%s"', APP_VERSION))
    -- /utf-8 是 MSVC/clang-cl 专用；GCC 默认按 UTF-8 读源码
    add_cxflags("/utf-8", {force = true})  -- main.cpp 是 UTF-8 无 BOM，MSVC 默认按 GBK 读

    -- 1. 生成 version.rc：set_version 自动提供 VERSION/VERSION_MAJOR/VERSION_MINOR/VERSION_ALTER
    add_configfiles("resource/version.rc.in", {pattern = "@([%w_]+)@"})
    add_files("$(builddir)/version.rc", {always_added = true})
end


target("points_modernize")
    set_kind("binary")
    add_files("src/storage_management.ixx")
    add_files("src/exceptions.ixx")
    add_files("src/data_storage.ixx")
    add_files("src/storage_management.cpp")
    add_files("src/data_storage.cpp")
    add_files("src/main_test_storage.cpp")
    add_includedirs("include")
    add_defines(string.format('APP_VERSION="%s"', APP_VERSION))
    -- /utf-8 是 MSVC/clang-cl 专用；GCC 默认按 UTF-8 读源码
    if is_plat("windows") then
        add_cxflags("/utf-8", {force = true})
    end
    -- GCC 16.1+ 启用 C++26 反射（P2996，-freflection）；源码侧用 __cpp_impl_reflection 自动检测
    -- 注：xmake 3.0 DSL 无进程执行/版本探测 API，不解析 gcc 版本；gcc < 16 会因 -freflection 明确报错
    if get_config("toolchain") == "gcc" then
        add_cxflags("-std=c++26", "-freflection", {force = true})
    end

-- ── Slint 集成 ─────────────────────────────────────────────────
--
-- 依赖：repo/packages/s/slint/xmake.lua（本地仓库包）
--   add_requires("slint") → 自动下载对应平台的 Slint C++ SDK：
--     Windows x86-64: Slint-cpp-*.exe（NSIS 安装包）→ 7z 解压
--     Linux x86-64:   Slint-cpp-*-Linux-x86_64.tar.gz
--     Linux arm64:    Slint-cpp-*-Linux-arm64.tar.gz
--   全部解压到 xmake 包缓存，不运行安装程序，不污染全局环境。
--   暴露 slint-compiler（编译器）、include/slint/（头文件）、lib/（运行时：.dll/.so）
--
-- 构建规则：rule("slint.compile")
--   target 内 add_files("xxx.slint") → 自动调用 slint-compiler 生成 xxx.h + xxx.cpp，
--   生成的头文件目录加入 include 路径，生成的 .cpp 自动编译并链接。
--   一次 xmake 调用即可在 C++ 源码中 #include "xxx.h"。
--
-- 用法示例：
--   target("my_app")
--       set_kind("binary")
--       add_packages("slint")              -- 拉取 Slint SDK
--       add_rules("slint.compile")         -- 启用 .slint 编译规则
--       add_files("ui/main.slint")         -- → 生成 main.h + main.cpp
--       add_files("src/main.cpp")          -- #include "main.h" 即可直接用
--
--   可选：指定 C++ 命名空间（默认取 target 名）
--       add_rules("slint.compile", {namespace = "ui"})
--       -- 生成的类在 ui:: 命名空间下
--
--   平台支持：Windows MSVC / clang-cl，Linux gcc / clang
--   （gcc/C++26 反射路径不声明 slint 依赖）
--   运行时共享库（slint_cpp.dll / libslint_cpp.so）已在 after_build 中自动拷贝到 targetdir
if get_config("toolchain") ~= "gcc" then
rule("slint.material")
    on_load(function (target)
        target:add("packages", "slint-material")
    end)

rule("slint.compile")
    set_extensions(".slint")
    before_buildcmd_file(function (target, batchcmds, sourcefile, opt)
        import("lib.detect.find_tool")

        -- 定位 slint-compiler 和 include 路径
        local slint_pkg = target:pkg("slint")
        local material_pkg = target:pkg("slint-material")
        local compiler
        local slint_inc
        local material_lib
        local is_windows = target:is_plat("windows")
        local compiler_name = is_windows and "slint-compiler.exe" or "slint-compiler"
        local link_name = is_windows and "slint_cpp.dll" or "slint_cpp"
        if slint_pkg then
            local installdir = slint_pkg:installdir()
            if installdir then
                compiler = path.join(installdir, "bin", compiler_name)
                slint_inc = path.join(installdir, "include", "slint")
            end
        end
        if material_pkg then
            local material_root = material_pkg:installdir()
            for _, candidate in ipairs(os.files(path.join(material_root, "**", "material.slint"))) do
                material_lib = candidate
                break
            end
            if material_lib then
                import("core.base.json")
                local settings_file = path.join(os.projectdir(), ".vscode", "settings.json")
                local settings = {}
                if os.isfile(settings_file) then
                    settings = json.loadfile(settings_file) or {}
                end
                local library_paths = settings["slint.libraryPaths"]
                if type(library_paths) ~= "table" then
                    library_paths = {}
                end
                library_paths.material = material_lib
                settings["slint.libraryPaths"] = library_paths
                os.mkdir(path.directory(settings_file))
                json.savefile(settings_file, settings)
            end
        end
        if not compiler or not os.isfile(compiler) then
            local t = find_tool("slint-compiler")
            compiler = t and t.program or "slint-compiler"
        end

        -- 生成路径：$(autogendir)/slint/<basename>.h 和 .cpp
        local gendir = path.join(target:autogendir(), "slint")
        local basename = path.basename(sourcefile)
        local headerfile = path.join(gendir, basename .. ".h")
        local cppfile = path.join(gendir, basename .. ".cpp")
        local objectfile = target:objectfile(cppfile)

        -- 命名空间：取目标名，可被 target:extraconf("rules", "slint.compile", "namespace") 覆盖
        local ns = target:extraconf("rules", "slint.compile", "namespace") or target:name()

        batchcmds:show_progress(opt.progress, "${color.build.object}compiling.slint %s", sourcefile)
        batchcmds:mkdir(gendir)
        local compiler_args = {
            sourcefile,
            "-f", "cpp",
            "-o", headerfile,
            "--cpp-file", cppfile,
            "--cpp-namespace", ns,
        }
        if material_lib then
            table.insert(compiler_args, "-L")
            table.insert(compiler_args, "material=" .. material_lib)
        end
        batchcmds:vrunv(compiler, compiler_args)

        -- 编译生成的 .cpp
        -- slint-compiler 生成的 #include 用项目根相对路径（build\...）→ 需要 os.projectdir()
        -- demo.h 里 #include <slint.h> → 需要 slint 包的 include 目录
        batchcmds:compile(cppfile, objectfile, {configs = {
            includedirs = {gendir, os.projectdir(), slint_inc}
        }})
        table.insert(target:objectfiles(), objectfile)

        -- 把生成头文件所在目录和 slint include 加入 target include 路径
        -- gendir: 供用户源码 #include "xxx.h"
        -- slint_inc: 供用户源码经 demo.h 间接 #include <slint.h>
        target:add("includedirs", gendir)
        if slint_inc then
            target:add("includedirs", slint_inc)
            -- slint 包的 lib 目录和链接库（on_fetch 返回的 links/linkdirs 未自动传播到 target）
            local installdir = slint_pkg:installdir()
            target:add("linkdirs", path.join(installdir, "lib"))
            target:add("links", link_name)
        end

        -- 依赖追踪
        batchcmds:add_depfiles(sourcefile)
        batchcmds:set_depmtime(os.mtime(objectfile))
        batchcmds:set_depcache(target:dependfile(objectfile))
    end)

target("slint_demo")
    set_kind("binary")
    set_default(false)
    add_files("slint_demo/demo.slint")
    add_files("slint_demo/main.cpp")
    add_packages("slint")
    add_rules("slint.compile", "slint.material")
    -- /utf-8 + /bigobj 是 MSVC 专用（Slint 生成代码量大）
    if is_plat("windows") then
        add_cxflags("/utf-8", "/bigobj", {force = true})
    end
    -- GCC 16 的 -Wstringop-overflow 对 Slint vtable 内联产生误报，需禁用
    if is_plat("linux") then
        add_cxflags("-Wno-stringop-overflow", {force = true})
        add_ldflags("-Wl,-rpath,$ORIGIN", {force = true})
    end
    -- 运行时需要找到 slint 共享库，自动拷贝到 targetdir
    after_build(function (target)
        local slint_pkg = target:pkg("slint")
        if slint_pkg then
            local installdir = slint_pkg:installdir()
            local libname = target:is_plat("windows") and "slint_cpp.dll" or "libslint_cpp.so"
            local lib = path.join(installdir, "lib", libname)
            if os.isfile(lib) then
                os.cp(lib, target:targetdir())
            end
        end
    end)
end
-- VS Code Slint 扩展使用 .vscode/settings.json 中的项目级 library path。
