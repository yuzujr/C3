-- src/net/xmake.lua
target("net")
    set_kind("static")
    set_languages("cxx20")
    set_warnings("all", "error")

    add_includedirs("include", {public = true})

    -- 网络模块源码
    add_files(
        "Uploader.cpp",
        "WebSocketClient.cpp",
        "URLBuilder.cpp"
    )

    -- 依赖 core 模块
    add_deps("core")

    -- 第三方静态库，public 表示也会传递给 dependents（如 app、可执行文件）
    add_packages(
        "cpr",
        "nlohmann_json",
        "ixwebsocket",
        {public = true}
    )
