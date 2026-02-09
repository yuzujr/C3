#ifndef WLR_SCREEN_CAPTURER_H
#define WLR_SCREEN_CAPTURER_H

#include <vector>

struct RawImage;

// wlr-screencopy 协议截屏（适用于 wlroots 系 Wayland 合成器）
namespace WlrScreenCapturer {

// 检测当前环境是否支持 wlr-screencopyw
bool isAvailable();

// 通过 wlr-screencopy 协议截取所有显示器，返回 RGB 格式的 RawImage
std::vector<RawImage> captureAllMonitors();

}  // namespace WlrScreenCapturer

#endif  // WLR_SCREEN_CAPTURER_H
