package("slint")

    set_kind("binary")
    set_homepage("https://slint.dev/")
    set_description("Slint is a declarative GUI toolkit to build native user interfaces for desktop and embedded.")
    set_license("GPL-3.0-or-later / Royalty-free")

    -- Windows MSVC x86-64: NSIS .exe（7z 解压，不运行安装程序）
    if is_host("windows") and is_arch("x64", "x86_64") then
        set_urls("https://github.com/slint-ui/slint/releases/download/v$(version)/Slint-cpp-$(version)-win64-MSVC-AMD64.exe")
        add_versions("1.17.1", "f5b537da448c1e3d72a24a774e19518ae412b9706b8ef49bdee64b62b878fe56")
    end

    -- Linux x86-64: tar.gz
    if is_host("linux") and is_arch("x86_64", "x64") then
        set_urls("https://github.com/slint-ui/slint/releases/download/v$(version)/Slint-cpp-$(version)-Linux-x86_64.tar.gz")
        add_versions("1.17.1", "a7ca87bbccfa892ce9cad36794c666f8dfa787fdc86acbb6fa826d909d268b37")
    end

    -- Linux arm64: tar.gz
    if is_host("linux") and is_arch("arm64", "aarch64") then
        set_urls("https://github.com/slint-ui/slint/releases/download/v$(version)/Slint-cpp-$(version)-Linux-arm64.tar.gz")
        add_versions("1.17.1", "7580dca6b2f24d135439dbba33860c041e2db9f17c6cde60000c7af73b420ecf")
    end

    -- on_fetch: 仅在安装目录已存在文件时返回元数据，否则返回 nil 触发 on_install
    on_fetch(function (package)
        if package:is_plat("windows") then
            if not package:is_arch("x64", "x86_64") then return end
            local installdir = package:installdir()
            if not installdir or not os.isfile(path.join(installdir, "bin", "slint-compiler.exe")) then
                return  -- 未安装，返回 nil 让 xmake 触发 on_install
            end
            return {
                includedirs = path.join(installdir, "include", "slint"),
                linkdirs = path.join(installdir, "lib"),
                links = { "slint_cpp" },
                rundirs = path.join(installdir, "lib"),
            }
        elseif package:is_plat("linux") then
            if not package:is_arch("x86_64", "x64", "arm64", "aarch64") then return end
            local installdir = package:installdir()
            if not installdir or not os.isfile(path.join(installdir, "bin", "slint-compiler")) then
                return
            end
            return {
                includedirs = path.join(installdir, "include", "slint"),
                linkdirs = path.join(installdir, "lib"),
                links = { "slint_cpp" },
                rundirs = path.join(installdir, "lib"),
            }
        end
    end)

    -- Windows: NSIS .exe → 7z 解压
    on_install("windows", function (package)
        local exe = package:originfile()
        if not exe or not os.isfile(exe) then
            raise("slint package: origin file not found (NSIS .exe)")
        end

        import("lib.detect.find_tool")
        local z = find_tool("7z")
        if not z then
            raise("slint package: 7z not found — ensure it is on PATH or in xmake winenv")
        end

        local installdir = package:installdir()
        os.vrunv(z.program, { "x", exe, "-o" .. installdir, "-y" })

        -- Verify expected layout
        if not os.isfile(path.join(installdir, "bin", "slint-compiler.exe")) then
            raise("slint package: bin/slint-compiler.exe not found after extraction")
        end
        if not os.isfile(path.join(installdir, "include", "slint", "slint.h")) then
            raise("slint package: include/slint/slint.h not found after extraction")
        end
        if not os.isfile(path.join(installdir, "lib", "slint_cpp.dll.lib")) then
            raise("slint package: lib/slint_cpp.dll.lib not found after extraction")
        end
    end)

    -- Linux: tar.gz → 解压后扁平化（去掉顶层 Slint-cpp-<ver>-<arch>/ 目录）
    on_install("linux", function (package)
        local tarball = package:originfile()
        if not tarball or not os.isfile(tarball) then
            raise("slint package: origin file not found (tar.gz)")
        end

        local installdir = package:installdir()
        -- 解压到临时目录，再把内层目录内容移到 installdir
        local tmpdir = path.join(os.tmpdir(), "slint_extract_" .. os.time())
        os.mkdir(tmpdir)
        os.vrunv("tar", { "xzf", tarball, "-C", tmpdir })

        -- 找到顶层目录（Slint-cpp-<ver>-<arch>/）
        local topdir
        for _, dir in ipairs(os.dirs(path.join(tmpdir, "*"))) do
            if os.isfile(path.join(dir, "bin", "slint-compiler")) then
                topdir = dir
                break
            end
        end
        if not topdir then
            raise("slint package: slint-compiler not found in tarball after extraction")
        end

        -- 把 bin/ include/ lib/ 移到 installdir
        os.cp(path.join(topdir, "bin"), installdir)
        os.cp(path.join(topdir, "include"), installdir)
        os.cp(path.join(topdir, "lib"), installdir)

        -- Verify
        if not os.isfile(path.join(installdir, "bin", "slint-compiler")) then
            raise("slint package: bin/slint-compiler not found after install")
        end
        if not os.isfile(path.join(installdir, "include", "slint", "slint.h")) then
            raise("slint package: include/slint/slint.h not found after install")
        end
        if not os.isfile(path.join(installdir, "lib", "libslint_cpp.so")) then
            raise("slint package: lib/libslint_cpp.so not found after install")
        end

        -- 清理临时目录
        os.rm(tmpdir)
    end)

    on_test(function (package)
        local compiler = path.join(package:installdir(), "bin",
            package:is_plat("windows") and "slint-compiler.exe" or "slint-compiler")
        if not os.isfile(compiler) then
            raise("slint-compiler not found")
        end
        os.vrun(compiler .. " --version")
    end)
