#ifndef CORE_H
#define CORE_H

#include "core/CommandDispatcher.h"
#include "core/Config.h"
#include "core/IDGenerator.h"
#include "core/ImageEncoder.h"
#include "core/Logger.h"
#include "core/PtyManager.h"
#include "core/RawImage.h"
#include "core/ScreenCapturer.h"
#include "core/SystemUtils.h"
#include "core/UploadController.h"

#ifdef USE_HARDCODED_CONFIG
#include "core/HardcodedConfig.h"
#endif

#if defined(_WIN32)
#include "core/platform/windows/GDIRAIIClasses.h"
#elif defined(__linux__)
#include "core/platform/linux/X11RAIIClasses.h"
#endif

#endif  // CORE_H