#pragma once

#include "non_copyable.h"
#include <windows.h>
#include <memory>
#include <stdint.h>
#include <string>

namespace yab {

class IWindowEventHandler;

class Window : NonCopyable {
public:
  Window() = delete;
  Window(const std::wstring& name, uint32_t width, uint32_t height);
  ~Window();

  HWND GetWindowHandle() const;
  uint32_t GetWidth() const;
  uint32_t GetHeight() const;

  void SetTitle(const std::wstring& title);

  void Show();
  void Hide();
  void Close();
  void ProcessEvent();

  void AddWindowEventHandler(std::shared_ptr<IWindowEventHandler> handler);

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}
