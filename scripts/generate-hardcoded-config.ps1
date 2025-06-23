# 硬编码配置生成脚本
# 将指定的配置预设硬编码到C++头文件中

param(
    [Parameter(Mandatory=$false)]
    [Alias("p")]
    [string]$Preset,
      [Parameter(Mandatory=$false)]
    [Alias("l")]
    [switch]$List,
    
    [Parameter(Mandatory=$false)]
    [Alias("h")]
    [switch]$Help
)

# 项目根目录
$ProjectRoot = Split-Path -Parent $PSScriptRoot
$ConfigDir = Join-Path $ProjectRoot "config"
$SrcDir = Join-Path $ProjectRoot "src"
$CoreIncludeDir = Join-Path $SrcDir "core\include\core"

# 文件路径
$PresetsFile = Join-Path $ConfigDir "build-presets.json"
$OutputHeaderFile = Join-Path $CoreIncludeDir "HardcodedConfig.h"

function Generate-ClientId {
    # 生成一个短格式的客户端ID
    # 格式：xxxxxxxx-xxxx-xxxx
    $guid = [System.Guid]::NewGuid()
    $fullId = $guid.ToString().ToLower()
    # 只取前3段：xxxxxxxx-xxxx-xxxx
    $parts = $fullId.Split('-')
    return "$($parts[0])-$($parts[1])-$($parts[2])"
}

function Show-Help {
    Write-Host "硬编码配置生成工具" -ForegroundColor Green
    Write-Host "========================" -ForegroundColor Green
    Write-Host ""
    Write-Host "用法:"
    Write-Host "  .\generate-hardcoded-config.ps1 [选项]"
    Write-Host ""
    Write-Host "选项:"
    Write-Host "  -p, --preset <n>   指定要硬编码的配置预设 (默认: development)"
    Write-Host "  -l, --list            列出所有可用的配置预设"
    Write-Host "  -h, --help            显示此帮助信息"
    Write-Host ""
    Write-Host "示例:"
    Write-Host "  .\generate-hardcoded-config.ps1 -p production"
    Write-Host "  .\generate-hardcoded-config.ps1 --preset production"
    Write-Host "  .\generate-hardcoded-config.ps1 -l"
    Write-Host "  .\generate-hardcoded-config.ps1 --list"
    Write-Host ""
    Write-Host "注意: 每次执行都会自动生成新的 client_id"
}

function Test-Prerequisites {
    if (-not (Test-Path $PresetsFile)) {
        Write-Error "配置预设文件不存在: $PresetsFile"
        return $false
    }
    
    if (-not (Test-Path $CoreIncludeDir)) {
        Write-Error "头文件目录不存在: $CoreIncludeDir"
        return $false
    }
    
    return $true
}

function Get-Presets {
    try {
        $content = Get-Content $PresetsFile -Raw -Encoding UTF8
        $presetsData = $content | ConvertFrom-Json
        return $presetsData
    }
    catch {
        Write-Error "无法解析配置预设文件: $_"
        return $null
    }
}

function Show-PresetList {
    $presetsData = Get-Presets
    if (-not $presetsData) { return }
    
    Write-Host "可用的配置预设:" -ForegroundColor Green
    Write-Host "==================" -ForegroundColor Green
    Write-Host ""
    
    foreach ($presetName in $presetsData.presets.PSObject.Properties.Name) {
        $preset = $presetsData.presets.$presetName
        Write-Host "[$presetName]" -ForegroundColor Yellow
        Write-Host "  名称: $($preset.name)" -ForegroundColor White
        Write-Host "  描述: $($preset.description)" -ForegroundColor Gray        
        $serverInfo = "$($preset.config.hostname):$($preset.config.port)"
        if ($preset.config.use_ssl) {
            $serverInfo += " (HTTPS/WSS)"
        } else {
            $serverInfo += " (HTTP/WS)"
        }
        Write-Host "  服务器: $serverInfo" -ForegroundColor Cyan
        if ($preset.config.use_ssl -and $preset.config.skip_ssl_verification) {
            Write-Host "  SSL: 启用 (跳过证书验证)" -ForegroundColor Yellow
        } elseif ($preset.config.use_ssl) {
            Write-Host "  SSL: 启用 (验证证书)" -ForegroundColor Green
        } else {
            Write-Host "  SSL: 禁用" -ForegroundColor Gray
        }
        Write-Host "  截图间隔: $($preset.config.interval_seconds)秒" -ForegroundColor Cyan
        Write-Host ""
    }
    
    Write-Host "默认预设: $($presetsData.default_preset)" -ForegroundColor Green
}

function Generate-HardcodedConfig {
    param($PresetName)
    
    $presetsData = Get-Presets
    if (-not $presetsData) { return $false }
    
    # 检查预设是否存在
    if (-not $presetsData.presets.PSObject.Properties.Name -contains $PresetName) {
        Write-Error "配置预设 '$PresetName' 不存在"
        Write-Host "可用预设: $($presetsData.presets.PSObject.Properties.Name -join ', ')" -ForegroundColor Yellow
        return $false
    }    $preset = $presetsData.presets.$PresetName
    $config = $preset.config
    
    # 生成新的 client_id（每次执行都生成新的）
    $clientId = Generate-ClientId
    Write-Host "生成Client ID: $clientId" -ForegroundColor Green
    
    # 生成时间戳
    $timestamp = Get-Date -Format "yyyy-MM-dd HH:mm:ss"
    
    # 生成头文件内容
    $headerContent = @"
// 自动生成的硬编码配置文件
// 生成时间: $timestamp
// 配置预设: $PresetName ($($preset.name))
// 描述: $($preset.description)
//
// 警告: 此文件是自动生成的，请勿手动编辑！
// 要修改配置，请编辑 config/build-presets.json 并重新运行生成脚本

#ifndef HARDCODED_CONFIG_H
#define HARDCODED_CONFIG_H

#include <string_view>

// 硬编码配置命名空间
namespace HardcodedConfig {
    
    // 构建信息
    constexpr std::string_view BUILD_PRESET = "$PresetName";
    constexpr std::string_view BUILD_PRESET_NAME = "$($preset.name)";
    constexpr std::string_view BUILD_PRESET_DESC = "$($preset.description)";
    constexpr std::string_view BUILD_TIMESTAMP = "$timestamp";      // API 配置
    constexpr std::string_view HOSTNAME = "$($config.hostname)";
    constexpr int PORT = $($config.port);
    constexpr bool USE_SSL = $($config.use_ssl.ToString().ToLower());
    constexpr bool SKIP_SSL_VERIFICATION = $($config.skip_ssl_verification.ToString().ToLower());
    constexpr int INTERVAL_SECONDS = $($config.interval_seconds);
    constexpr int MAX_RETRIES = $($config.max_retries);
    constexpr int RETRY_DELAY_MS = $($config.retry_delay_ms);
    constexpr bool ADD_TO_STARTUP = $($config.add_to_startup.ToString().ToLower());
    constexpr std::string_view CLIENT_ID = "$clientId";
    
    // 编译时配置检查
    static_assert(INTERVAL_SECONDS > 0, "INTERVAL_SECONDS must be positive");
    static_assert(MAX_RETRIES >= 0, "MAX_RETRIES must be non-negative");
    static_assert(RETRY_DELAY_MS >= 0, "RETRY_DELAY_MS must be non-negative");      // 配置信息结构
    struct ConfigInfo {
        std::string_view preset;
        std::string_view preset_name;
        std::string_view preset_desc;
        std::string_view build_timestamp;
        std::string_view hostname;
        int port;
        bool use_ssl;
        bool skip_ssl_verification;
        int interval_seconds;
        int max_retries;
        int retry_delay_ms;
        bool add_to_startup;
        std::string_view client_id;
    };      // 获取硬编码配置信息
    inline constexpr ConfigInfo getConfigInfo() {
        return ConfigInfo{
            BUILD_PRESET,
            BUILD_PRESET_NAME,
            BUILD_PRESET_DESC,
            BUILD_TIMESTAMP,
            HOSTNAME,
            PORT,
            USE_SSL,
            SKIP_SSL_VERIFICATION,
            INTERVAL_SECONDS,
            MAX_RETRIES,
            RETRY_DELAY_MS,
            ADD_TO_STARTUP,
            CLIENT_ID
        };
    }
}

#endif // HARDCODED_CONFIG_H
"@

    try {
        # 写入头文件
        $headerContent | Out-File -FilePath $OutputHeaderFile -Encoding UTF8
        
        Write-Host "✅ 成功生成硬编码配置!" -ForegroundColor Green
        Write-Host "   预设名称: $PresetName ($($preset.name))" -ForegroundColor Yellow
        Write-Host "   描述: $($preset.description)" -ForegroundColor Yellow
        Write-Host "   输出文件: $OutputHeaderFile" -ForegroundColor Yellow        
        Write-Host "   配置内容：" -ForegroundColor Cyan
        $serverInfo = "$($config.hostname):$($config.port)"
        if ($config.use_ssl) {
            $serverInfo += " (HTTPS/WSS)"
        } else {
            $serverInfo += " (HTTP/WS)"
        }
        Write-Host "   服务器地址: $serverInfo" -ForegroundColor Gray
        if ($config.use_ssl) {
            if ($config.skip_ssl_verification) {
                Write-Host "   SSL验证: 跳过证书验证" -ForegroundColor Gray
            } else {
                Write-Host "   SSL验证: 启用证书验证" -ForegroundColor Gray
            }
        }
        Write-Host "   截图间隔: $($config.interval_seconds)秒" -ForegroundColor Gray
        Write-Host "   最大重试次数: $($config.max_retries)" -ForegroundColor Gray
        Write-Host "   重试延迟: $($config.retry_delay_ms)毫秒" -ForegroundColor Gray
        Write-Host "   启动时添加到自启动: $($config.add_to_startup)" -ForegroundColor Gray
        Write-Host "   客户端ID: $clientId" -ForegroundColor Gray
        
        return $true
    }
    catch {
        Write-Error "生成硬编码配置失败: $_"
        return $false
    }
}

# 主逻辑
# 如果没有任何参数，显示帮助
if ($PSBoundParameters.Count -eq 0) {
    Show-Help
    exit 0
}

if ($Help) {
    Show-Help
    exit 0
}

if (-not (Test-Prerequisites)) {
    exit 1
}

if ($List) {
    Show-PresetList
    exit 0
}

# 如果没有指定预设，使用默认值
if (-not $Preset) {
    $Preset = "development"
}

if (Generate-HardcodedConfig -PresetName $Preset) {
    Write-Host ""
    Write-Host "🎯 下一步:" -ForegroundColor Green
    Write-Host ""
    Write-Host "使用 CMake 构建" -ForegroundColor Yellow
    Write-Host "   mkdir build-hardcoded" -ForegroundColor Cyan
    Write-Host "   cmake -S . -B build-hardcoded -DUSE_HARDCODED_CONFIG=ON -DCMAKE_BUILD_TYPE=Release" -ForegroundColor Cyan
    Write-Host "   cmake --build build-hardcoded --config Release -j" -ForegroundColor Cyan
    exit 0
} else {
    exit 1
}
