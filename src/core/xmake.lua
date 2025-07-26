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
