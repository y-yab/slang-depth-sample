#pragma once

#include "non_copyable.h"
#include <windows.h>
#include <memory>
#include <stdint.h>
#include <string>

namespace yab
{

class IWindowEventHandler;

class Window : NonCopyable {
public:
  struct Rect {
    int32_t x;
    int32_t y;
    uint32_t width;
    uint32_t height;
  };

  struct CreationDesc {
    std::wstring name;
    Rect rect;
    bool fullscreen;
    bool topmost;
    bool hook_f10key;
  };

public:
  Window() = delete;
  Window(const CreationDesc& desc);
  ~Window();

  HWND GetWindowHandle() const;
  Rect GetWindowRect() const;

  void SetTitle(const std::wstring& title);

  bool IsFullscreen() const;
  void SwitchFullscreen(bool fullscreen);

  bool IsTopmost() const;
  void SwitchTopmost(bool topmost);

  void Locate(int32_t x, int32_t y);

  void Show();
  void Hide();
  void Close();
  void ProcessEvent();

  void AddWindowEventHandler(std::shared_ptr<IWindowEventHandler> handler);
  void RemoveWindowEventHandler(std::shared_ptr<IWindowEventHandler> handler);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace yab
