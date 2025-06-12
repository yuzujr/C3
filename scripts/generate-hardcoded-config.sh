#!/bin/bash
# 硬编码配置生成脚本
# 将指定的配置预设硬编码到C++头文件中

# 颜色定义
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
GRAY='\033[0;37m'
NC='\033[0m' # No Color

# 默认值
PRESET=""
LIST=false
HELP=false

# 项目根目录
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_ROOT="$(dirname "$SCRIPT_DIR")"
CONFIG_DIR="$PROJECT_ROOT/config"
SRC_DIR="$PROJECT_ROOT/src"
CORE_INCLUDE_DIR="$SRC_DIR/core/include/core"

# 文件路径
PRESETS_FILE="$CONFIG_DIR/build-presets.json"
OUTPUT_HEADER_FILE="$CORE_INCLUDE_DIR/HardcodedConfig.h"

# 显示帮助信息
show_help() {
    echo -e "${GREEN}硬编码配置生成工具${NC}"
    echo -e "${GREEN}========================${NC}"
    echo ""
    echo "用法:"
    echo "  ./generate-hardcoded-config.sh [选项]"
    echo ""
    echo "选项:"
    echo "  -p, --preset <name>   指定要硬编码的配置预设 (默认: development)"
    echo "  -l, --list            列出所有可用的配置预设"
    echo "  -h, --help            显示此帮助信息"
    echo ""
    echo "示例:"
    echo "  ./generate-hardcoded-config.sh -p production"
    echo "  ./generate-hardcoded-config.sh --preset production"
    echo "  ./generate-hardcoded-config.sh -l"
    echo "  ./generate-hardcoded-config.sh --list"
    echo ""
}

# 检查先决条件
test_prerequisites() {
    if [[ ! -f "$PRESETS_FILE" ]]; then
        echo -e "${RED}错误: 配置预设文件不存在: $PRESETS_FILE${NC}" >&2
        return 1
    fi
    
    if [[ ! -d "$CORE_INCLUDE_DIR" ]]; then
        echo -e "${RED}错误: 头文件目录不存在: $CORE_INCLUDE_DIR${NC}" >&2
        return 1
    fi
    
    # 检查 jq 是否可用
    if ! command -v jq &> /dev/null; then
        echo -e "${RED}错误: 需要安装 jq 工具来解析 JSON 文件${NC}" >&2
        echo "在 Ubuntu/Debian 上: sudo apt install jq"
        echo "在 CentOS/RHEL 上: sudo yum install jq"
        echo "在 macOS 上: brew install jq"
        return 1
    fi
    
    return 0
}

# 获取预设数据
get_presets() {
    if [[ ! -f "$PRESETS_FILE" ]]; then
        echo -e "${RED}错误: 无法读取配置预设文件${NC}" >&2
        return 1
    fi
    
    if ! jq . "$PRESETS_FILE" > /dev/null 2>&1; then
        echo -e "${RED}错误: 无法解析配置预设文件 JSON 格式${NC}" >&2
        return 1
    fi
    
    return 0
}

# 显示预设列表
show_preset_list() {
    if ! get_presets; then
        return 1
    fi
    
    echo -e "${GREEN}可用的配置预设:${NC}"
    echo -e "${GREEN}==================${NC}"
    echo ""
    
    # 获取所有预设名称
    preset_names=$(jq -r '.presets | keys[]' "$PRESETS_FILE")
    
    for preset_name in $preset_names; do
        preset_data=$(jq -r ".presets[\"$preset_name\"]" "$PRESETS_FILE")
        name=$(echo "$preset_data" | jq -r '.name')
        description=$(echo "$preset_data" | jq -r '.description')
        server_url=$(echo "$preset_data" | jq -r '.config.server_url')
        ws_url=$(echo "$preset_data" | jq -r '.config.ws_url')
        interval_seconds=$(echo "$preset_data" | jq -r '.config.interval_seconds')
        
        echo -e "${YELLOW}[$preset_name]${NC}"
        echo -e "  名称: $name"
        echo -e "${GRAY}  描述: $description${NC}"
        echo -e "${CYAN}  服务器: $server_url${NC}"
        echo -e "${CYAN}  WebSocket: $ws_url${NC}"
        echo -e "${CYAN}  截图间隔: ${interval_seconds}秒${NC}"
        echo ""
    done
    
    default_preset=$(jq -r '.default_preset' "$PRESETS_FILE")
    echo -e "${GREEN}默认预设: $default_preset${NC}"
}

# 生成硬编码配置
generate_hardcoded_config() {
    local preset_name="$1"
    
    if ! get_presets; then
        return 1
    fi
    
    # 检查预设是否存在
    if ! jq -e ".presets[\"$preset_name\"]" "$PRESETS_FILE" > /dev/null; then
        echo -e "${RED}错误: 配置预设 '$preset_name' 不存在${NC}" >&2
        available_presets=$(jq -r '.presets | keys | join(", ")' "$PRESETS_FILE")
        echo -e "${YELLOW}可用预设: $available_presets${NC}"
        return 1
    fi
    
    # 获取预设数据
    preset_data=$(jq -r ".presets[\"$preset_name\"]" "$PRESETS_FILE")
    preset_display_name=$(echo "$preset_data" | jq -r '.name')
    preset_description=$(echo "$preset_data" | jq -r '.description')
    
    # 获取配置数据
    config_data=$(echo "$preset_data" | jq -r '.config')
    server_url=$(echo "$config_data" | jq -r '.server_url')
    ws_url=$(echo "$config_data" | jq -r '.ws_url')
    interval_seconds=$(echo "$config_data" | jq -r '.interval_seconds')
    max_retries=$(echo "$config_data" | jq -r '.max_retries')
    retry_delay_ms=$(echo "$config_data" | jq -r '.retry_delay_ms')
    add_to_startup=$(echo "$config_data" | jq -r '.add_to_startup')
    client_id=$(echo "$config_data" | jq -r '.client_id')
    
    # 转换布尔值
    if [[ "$add_to_startup" == "true" ]]; then
        add_to_startup_cpp="true"
    else
        add_to_startup_cpp="false"
    fi
    
    # 生成时间戳
    timestamp=$(date '+%Y-%m-%d %H:%M:%S')
    
    # 生成头文件内容
    cat > "$OUTPUT_HEADER_FILE" << EOF
// 自动生成的硬编码配置文件
// 生成时间: $timestamp
// 配置预设: $preset_name ($preset_display_name)
// 描述: $preset_description
//
// 警告: 此文件是自动生成的，请勿手动编辑！
// 要修改配置，请编辑 config/build-presets.json 并重新运行生成脚本

#ifndef HARDCODED_CONFIG_H
#define HARDCODED_CONFIG_H

#include <string_view>

// 硬编码配置命名空间
namespace HardcodedConfig {
    
    // 构建信息
    constexpr std::string_view BUILD_PRESET = "$preset_name";
    constexpr std::string_view BUILD_PRESET_NAME = "$preset_display_name";
    constexpr std::string_view BUILD_PRESET_DESC = "$preset_description";
    constexpr std::string_view BUILD_TIMESTAMP = "$timestamp";
    
    // API 配置
    constexpr std::string_view SERVER_URL = "$server_url";
    constexpr std::string_view WS_URL = "$ws_url";
    constexpr int INTERVAL_SECONDS = $interval_seconds;
    constexpr int MAX_RETRIES = $max_retries;
    constexpr int RETRY_DELAY_MS = $retry_delay_ms;
    constexpr bool ADD_TO_STARTUP = $add_to_startup_cpp;
    constexpr std::string_view CLIENT_ID = "$client_id";
    
    // 编译时配置检查
    static_assert(INTERVAL_SECONDS > 0, "INTERVAL_SECONDS must be positive");
    static_assert(MAX_RETRIES >= 0, "MAX_RETRIES must be non-negative");
    static_assert(RETRY_DELAY_MS >= 0, "RETRY_DELAY_MS must be non-negative");
    
    // 配置信息结构
    struct ConfigInfo {
        std::string_view preset;
        std::string_view preset_name;
        std::string_view preset_desc;
        std::string_view build_timestamp;
        std::string_view server_url;
        std::string_view ws_url;
        int interval_seconds;
        int max_retries;
        int retry_delay_ms;
        bool add_to_startup;
        std::string_view client_id;
    };
    
    // 获取硬编码配置信息
    inline constexpr ConfigInfo getConfigInfo() {
        return ConfigInfo{
            BUILD_PRESET,
            BUILD_PRESET_NAME,
            BUILD_PRESET_DESC,
            BUILD_TIMESTAMP,
            SERVER_URL,
            WS_URL,
            INTERVAL_SECONDS,
            MAX_RETRIES,
            RETRY_DELAY_MS,
            ADD_TO_STARTUP,
            CLIENT_ID
        };
    }
}

#endif // HARDCODED_CONFIG_H
EOF

    if [[ $? -eq 0 ]]; then
        echo -e "${GREEN}✅ 成功生成硬编码配置!${NC}"
        echo -e "   ${YELLOW}预设名称: $preset_name ($preset_display_name)${NC}"
        echo -e "   ${YELLOW}描述: $preset_description${NC}"
        echo -e "   ${YELLOW}输出文件: $OUTPUT_HEADER_FILE${NC}"

        echo -e "   ${CYAN}配置内容：${NC}"
        echo -e "   ${GRAY}http地址: $server_url${NC}"
        echo -e "   ${GRAY}WebSocket地址: $ws_url${NC}"
        echo -e "   ${GRAY}截图间隔: ${interval_seconds}秒${NC}"
        echo -e "   ${GRAY}最大重试次数: $max_retries${NC}"
        echo -e "   ${GRAY}重试延迟: ${retry_delay_ms}毫秒${NC}"
        echo -e "   ${GRAY}启动时添加到自启动: $add_to_startup${NC}"
        
        return 0
    else
        echo -e "${RED}错误: 生成硬编码配置失败${NC}" >&2
        return 1
    fi
}

# 解析命令行参数
while [[ $# -gt 0 ]]; do
    case $1 in
        -p|--preset)
            PRESET="$2"
            shift 2
            ;;
        -l|--list)
            LIST=true
            shift
            ;;
        -h|--help)
            HELP=true
            shift
            ;;
        *)
            echo -e "${RED}错误: 未知选项 $1${NC}" >&2
            show_help
            exit 1
            ;;
    esac
done

# 主逻辑
# 如果没有任何参数，显示帮助
if [[ $# -eq 0 && -z "$PRESET" && "$LIST" == false && "$HELP" == false ]]; then
    show_help
    exit 0
fi

if [[ "$HELP" == true ]]; then
    show_help
    exit 0
fi

if ! test_prerequisites; then
    exit 1
fi

if [[ "$LIST" == true ]]; then
    show_preset_list
    exit 0
fi

# 如果没有指定预设，使用默认值
if [[ -z "$PRESET" ]]; then
    PRESET="development"
fi

if generate_hardcoded_config "$PRESET"; then
    echo ""
    echo -e "${GREEN}🎯 下一步:${NC}"
    echo ""
    echo -e "${YELLOW}使用 CMake 构建${NC}"
    echo -e "   ${CYAN}mkdir build-hardcoded${NC}"
    echo -e "   ${CYAN}cmake -S . -B build-hardcoded -DUSE_HARDCODED_CONFIG=ON -DCMAKE_BUILD_TYPE=Release${NC}"
    echo -e "   ${CYAN}cmake --build build-hardcoded --config Release -j${NC}"
    exit 0
else
    exit 1
fi
