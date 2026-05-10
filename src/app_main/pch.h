#pragma once

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <Windows.h>

#pragma warning(push)
#pragma warning(disable:4275)
#define SPDLOG_WCHAR_TO_UTF8_SUPPORT
#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#pragma warning(pop)

#include <magic_enum/magic_enum.hpp>

#include <slang.h>
#include <slang-rhi.h>
#pragma warning(push)
#pragma warning(disable:4267)
#include <slang-rhi/shader-cursor.h>
#pragma warning(pop)

#include <winrt/Windows.Graphics.Display.h>
#include <winrt/Windows.Foundation.h>
#include <winrt/Windows.Foundation.Collections.h>
