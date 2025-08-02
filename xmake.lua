-- xmake.lua (放在项目根目录)

-- 项目基础配置
set_project("C3")
set_version("1.10.0")
set_languages("cxx23")
set_warnings("all", "error")

-- 编译模式
add_rules("mode.debug", "mode.release")

-- 构建选项
option("gui")
    set_showmenu(true)
    set_description("是否构建 Windows GUI 程序")
option_end()

option("hardcode_config")
    set_showmenu(true)
    set_description("是否使用硬编码配置（USE_HARDCODED_CONFIG）")
option_end()

-- 第三方依赖，全部静态链接
add_requires(
    "cpr", 
    "nlohmann_json", 
    "spdlog", 
    "ixwebsocket", 
    "libjpeg-turbo", 
    "gtest",
    {configs = {shared = false}}
)

includes("src")
includes("test")