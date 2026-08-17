package("slint-material")
    set_kind("binary")
    set_homepage("https://material.slint.dev/")
    set_description("Material 3 component library for Slint")
    set_license("MIT")

    set_urls("https://material.slint.dev/zip/material-$(version).zip")
    add_versions("1.0", "69fafbb428d9e09c50de285c2e1aaf5b069e69cb97983032e55981bf9a00a2ab")

    local function find_material_library(installdir)
        for _, candidate in ipairs(os.files(path.join(installdir, "**", "material.slint"))) do
            return candidate
        end
    end

    on_fetch(function (package)
        local library = find_material_library(package:installdir())
        if not library then
            return
        end
        return {
            librarypaths = { material = library },
        }
    end)

    on_install(function (package)
        import("lib.detect.find_tool")
        local z = find_tool("7z")
        if not z then
            raise("slint-material package: 7z not found — ensure it is on PATH")
        end

        local installdir = package:installdir()
        os.vrunv(z.program, { "x", package:originfile(), "-o" .. installdir, "-y" })
        if not find_material_library(installdir) then
            raise("slint-material package: material.slint not found after extraction")
        end
    end)

    on_test(function (package)
        if not find_material_library(package:installdir()) then
            raise("material.slint not found")
        end
    end)

