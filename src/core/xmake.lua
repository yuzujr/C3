-- src/core/xmake.lua
target("core")
    set_kind("static")
    set_languages("cxx20")
    set_warnings("all", "error")

    -- 公共头文件搜索路径，供其他模块或可执行文件使用
    add_includedirs("include", {public = true})

    -- 通用源码
    add_files(
        "Config.cpp",
        "Logger.cpp",
        "IDGenerator.cpp",
        "UploadController.cpp",
        "CommandDispatcher.cpp",
        "ImageEncoder.cpp",
        "RawImage.cpp"
    )

    -- 平台相关源码
    if is_plat("windows") then
        add_files(
            "platform/windows/ScreenCapturer.cpp",
            "platform/windows/SystemUtils.cpp",
            "platform/windows/GDIRAIIClasses.cpp",
            "platform/windows/PtyManager.cpp"
        )
        -- Windows 系统库
        add_syslinks("Shcore")
    elseif is_plat("linux") or is_plat("macosx") then
        add_files(
            "platform/linux/ScreenCapturer.cpp",
            "platform/linux/SystemUtils.cpp",
            "platform/linux/X11RAIIClasses.cpp",
            "platform/linux/PtyManager.cpp"
        )
        -- X11 Xinerama由用户通过包管理器安装，链接时用系统库名
        add_syslinks("X11", "Xinerama")

        -- Wayland wlr-screencopy 可选后端
        if has_config("wlr_screencopy") then
            add_defines("C3_HAS_WLR_SCREENCOPY")
            add_files("platform/linux/WlrScreenCapturer.cpp")
            add_packages("pkgconfig::wayland-client")
            add_syslinks("rt")  -- for shm_open

            -- 使用 wayland-scanner 生成协议代码
            on_load(function (target)
                local find_program = import("lib.detect.find_program")
                local wayland_scanner = find_program("wayland-scanner")
                if not wayland_scanner then
                    raise("wayland-scanner not found. Install wayland-dev or disable wlr_screencopy via `xmake f --wlr_screencopy=n`.")
                end

                local gendir = path.join(os.projectdir(), "build", "wayland-protocols")
                os.mkdir(gendir)

                local protocoldir = path.join(os.projectdir(), "thirdparty", "wayland-protocols", "protocols")
                for _, xml in ipairs(os.files(path.join(protocoldir, "*.xml"))) do
                    local base   = path.basename(xml)
                    local header = path.join(gendir, base .. "-client-protocol.h")
                    local code   = path.join(gendir, base .. "-protocol.c")

                    os.execv(wayland_scanner, {"client-header", xml, header})
                    os.execv(wayland_scanner, {"private-code",  xml, code})
                end

                -- 添加生成的 .c 文件
                local cfiles = os.files(path.join(gendir, "*.c"))
                if #cfiles > 0 then
                    target:add("files", cfiles)
                end
                -- 添加生成目录到头文件搜索路径
                target:add("includedirs", gendir)
            end)
        end
    end

    -- 第三方静态库
    add_packages(
        "nlohmann_json",
        "spdlog",
        "libjpeg-turbo",
        {public = true}
    )

    -- 硬编码配置宏
    if has_config("hardcode_config") then
        add_defines("USE_HARDCODED_CONFIG")
    end
