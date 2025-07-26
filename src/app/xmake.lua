-- src/app/xmake.lua
target("app")
    set_kind("static")
    set_languages("cxx20")
    set_warnings("all", "error")

    add_includedirs("include", {public = true})

    -- 依赖 core 和 net
    add_deps("core", "net")

    -- 应用模块源码
    add_files("C3App.cpp")

    -- 硬编码配置宏
    if has_config("hardcode_config") then
        add_defines("USE_HARDCODED_CONFIG")
    end
