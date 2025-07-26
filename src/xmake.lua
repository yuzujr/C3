-- 包含子模块脚本
includes("core", "net", "app")

-- 可执行文件目标
target("C3")
    set_kind("binary")
    add_deps("core", "net", "app")
    add_files("main.cpp")
    -- Windows GUI 控制
    if is_plat("windows") and has_config("gui") then
        add_files("WinMain.cpp")
        set_kind("binary", {gui = true})
    end
    -- 硬编码配置宏
    if has_config("hardcode_config") then
        add_defines("USE_HARDCODED_CONFIG")
    end
    add_packages("cpr", "nlohmann_json", "spdlog", "ixwebsocket", "libjpeg-turbo")