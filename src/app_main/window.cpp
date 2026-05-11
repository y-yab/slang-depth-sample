#include "pch.h"

#include "window.h"
#include "window_event_handler.h"

#include <atomic>
#include <vector>
#include <windowsx.h>

using namespace yab;

namespace {

constexpr auto kWindowStyle = WS_OVERLAPPEDWINDOW;

} // namespace (anonymous)

struct Window::Impl {
  std::atomic<bool> closed_{ false };
  std::atomic<bool> destroyed_{ false };
  std::wstring name_;
  HWND hwnd_;
  Rect window_rect_;
  Rect restore_rect_;
  bool has_restore_rect_{ false };
  bool is_fullscreen_{ false };
  bool is_topmost_{ false };
  bool is_shown_{ false };
  bool hook_f10key_{ false };
  LONG_PTR windowed_style_{};
  LONG_PTR windowed_ex_style_{};
  std::vector<std::shared_ptr<IWindowEventHandler>> handlers_;

  Impl(const CreationDesc& desc)
    : name_{ desc.name }, window_rect_{ desc.rect }, hook_f10key_{ desc.hook_f10key }
  {
    WNDCLASSEX wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = (WNDPROC)Impl::WindProc;
    wc.hInstance = ::GetModuleHandle(NULL);
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = name_.c_str();
    wc.cbSize = sizeof(WNDCLASSEX);
    ::RegisterClassEx(&wc);

    hwnd_ = ::CreateWindowEx(
      WS_EX_APPWINDOW,
      name_.c_str(),
      name_.c_str(),
      kWindowStyle,
      window_rect_.x, window_rect_.y, window_rect_.width, window_rect_.height,
      NULL, NULL, wc.hInstance, this
    );

    SwitchFullscreen(desc.fullscreen);
    SwitchTopmost(desc.topmost);
  }

  ~Impl() {
    if (hwnd_ && !closed_.load()) {
      ::PostMessage(hwnd_, WM_CLOSE, 0, 0);
    }
    while (hwnd_ && !destroyed_.load()) {
      ProcessEvents();
    }
  }

  void ProcessEvents() {
    MSG msg;
    while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
  }

  static LRESULT WINAPI WindProc(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    Window::Impl* impl{};

    if (WM_CREATE == msg) {
      auto cs = reinterpret_cast<CREATESTRUCT*>(lparam);
      impl = reinterpret_cast<Window::Impl*>(cs->lpCreateParams);
      ::SetWindowLongPtr(hwnd, GWLP_USERDATA, (LONG_PTR)impl);
    }
    else {
      impl = reinterpret_cast<Window::Impl*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    }

    if (impl) {
      auto ret = impl->ProcessMessage(hwnd, msg, wparam, lparam);

      if (WM_DESTROY == msg) {
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, 0);
      }

      return ret;
    }

    return ::DefWindowProc(hwnd, msg, wparam, lparam);
  }

  Rect GetCurrentWindowRect() const {
    RECT rect{};
    if (::GetClientRect(hwnd_, &rect)) {
      return {
        .x = static_cast<int32_t>(rect.left),
        .y = static_cast<int32_t>(rect.top),
        .width = static_cast<uint32_t>(std::abs(rect.right - rect.left)),
        .height = static_cast<uint32_t>(std::abs(rect.bottom - rect.top)) };
    }
    return {};
  }

  LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    LRESULT res = 0;
    UINT x, y;

    switch (msg) {
      case WM_MOVE: {
        window_rect_.x = GET_X_LPARAM(lparam);
        window_rect_.y = GET_Y_LPARAM(lparam);
        for (auto& it : handlers_) {
          it->OnWindowMove(window_rect_.x, window_rect_.y);
        }
        break;
      }
      case WM_SIZE:
        window_rect_ = GetCurrentWindowRect();
        for (auto& it : handlers_) {
          it->OnWindowResize(window_rect_.width, window_rect_.height);
        }
        break;
      case WM_CLOSE:
        for (auto& it : handlers_) {
          it->OnWindowClose();
        }
        ::DestroyWindow(hwnd);
        closed_.store(true);
        closed_.notify_one();
        break;
      case WM_DESTROY:
        ::PostQuitMessage(0);
        destroyed_.store(true);
        destroyed_.notify_one();
        break;
      case WM_SYSKEYDOWN:
      case WM_SYSKEYUP:
        if (hook_f10key_ && static_cast<uint32_t>(wparam) == VK_F10) {
          // Prevent F10 from activating the menu bar.
          for (auto& it : handlers_) {
            it->OnKeyboard(VK_F10, (msg == WM_SYSKEYDOWN));
          }
          break;
        }
      case WM_KEYDOWN:
      case WM_KEYUP:
        for (auto& it : handlers_) {
          it->OnKeyboard(static_cast<uint32_t>(wparam), (msg == WM_KEYDOWN));
        }
        break;
      case WM_LBUTTONDOWN:
      case WM_LBUTTONUP:
      case WM_RBUTTONDOWN:
      case WM_RBUTTONUP:
      case WM_MBUTTONDOWN:
      case WM_MBUTTONUP:
        x = LOWORD(lparam);
        y = HIWORD(lparam);
        for (auto& it : handlers_) {
          IWindowEventHandler::MouseButton button;
          if (msg == WM_LBUTTONDOWN || msg == WM_LBUTTONUP) {
            button = IWindowEventHandler::MouseButton::Left;
          }
          if (msg == WM_RBUTTONDOWN || msg == WM_RBUTTONUP) {
            button = IWindowEventHandler::MouseButton::Right;
          }
          if (msg == WM_MBUTTONDOWN || msg == WM_MBUTTONUP) {
            button = IWindowEventHandler::MouseButton::Center;
          }
          IWindowEventHandler::MouseAction action = IWindowEventHandler::MouseAction::Release;
          if (msg == WM_LBUTTONDOWN || msg == WM_RBUTTONDOWN || msg == WM_MBUTTONDOWN) {
            action = IWindowEventHandler::MouseAction::Press;
          }
          it->OnMouseButton(button, action, (float)x, (float)y);
        }
        break;
      case WM_MOUSEMOVE:
        x = LOWORD(lparam);
        y = HIWORD(lparam);
        for (auto& it : handlers_) {
          it->OnMouseMove((float)x, (float)y);
        }
        break;
      case WM_MOUSEWHEEL:
        for (auto& it : handlers_) {
          auto delta = GET_WHEEL_DELTA_WPARAM(wparam);
          delta = delta > 1 ? 1 : delta < -1 ? -1 : 0;
          it->OnMouseWheel((float)delta);
        }
        break;
      default:
        res = ::DefWindowProc(hwnd, msg, wparam, lparam);
        break;
    }
    return res;
  }

  void SwitchFullscreen(bool fullscreen) {
    if (is_fullscreen_ == fullscreen) {
      return;
    }

    const HWND z_order = is_topmost_ ? HWND_TOPMOST : HWND_TOP;

    if (fullscreen) { // windowed -> fullscreen
      restore_rect_ = window_rect_;
      has_restore_rect_ = true;

      windowed_style_ = ::GetWindowLong(hwnd_, GWL_STYLE);
      auto fullscreen_style = (windowed_style_ & ~kWindowStyle) | WS_POPUP;
      ::SetWindowLongPtr(hwnd_, GWL_STYLE, fullscreen_style);

      RECT monitor_rect{ 0, 0, GetSystemMetrics(SM_CXSCREEN), GetSystemMetrics(SM_CYSCREEN) };
      MONITORINFO mi{};
      mi.cbSize = sizeof(mi);
      if (::GetMonitorInfo(::MonitorFromWindow(hwnd_, MONITOR_DEFAULTTONEAREST), &mi)) {
        monitor_rect = mi.rcMonitor;
      }

      ::SetWindowPos(hwnd_, z_order,
        monitor_rect.left,
        monitor_rect.top,
        monitor_rect.right - monitor_rect.left,
        monitor_rect.bottom - monitor_rect.top,
        SWP_FRAMECHANGED);
    }
    else { // fullscreen -> windowed
      windowed_style_ = windowed_style_ ? windowed_style_ : kWindowStyle;
      ::SetWindowLongPtr(hwnd_, GWL_STYLE, windowed_style_);

      if (has_restore_rect_) {
        ::SetWindowPos(hwnd_, z_order,
           restore_rect_.x,
           restore_rect_.y,
           static_cast<int>(restore_rect_.width),
           static_cast<int>(restore_rect_.height),
           SWP_FRAMECHANGED);
      }
      else {
        ::SetWindowPos(hwnd_, z_order,
           0,
           0,
           static_cast<int>(window_rect_.width),
           static_cast<int>(window_rect_.height),
           SWP_FRAMECHANGED);
      }
      ::ShowWindow(hwnd_, is_shown_ ? SW_SHOW : SW_HIDE);
    }

    is_fullscreen_ = fullscreen;
  }

  void SwitchTopmost(bool topmost) {
    if (is_topmost_ == topmost) {
      return;
    }

    ::SetWindowPos(hwnd_,
      topmost ? HWND_TOPMOST : HWND_NOTOPMOST,
      0, 0, 0, 0,
      SWP_NOMOVE | SWP_NOSIZE);

    is_topmost_ = topmost;
  }

  void Locate(int32_t x, int32_t y) {
    auto prev_is_fullscreen = is_fullscreen_;

    if (is_fullscreen_) {
      SwitchFullscreen(false);
    }

    ::SetWindowPos(hwnd_,
      is_topmost_ ? HWND_TOPMOST : HWND_NOTOPMOST,
      x, y, 0, 0,
      SWP_NOSIZE | SWP_NOZORDER);

    if (prev_is_fullscreen != is_fullscreen_) {
      SwitchFullscreen(prev_is_fullscreen);
    }
  }
};

Window::Window(const CreationDesc& desc) : impl_{ std::make_unique<Impl>(desc) } {
}

Window::~Window() {
}

HWND Window::GetWindowHandle() const {
  return impl_->hwnd_;
}

Window::Rect Window::GetWindowRect() const {
  return impl_->window_rect_;
}

void Window::SetTitle(const std::wstring& title) {
  ::SetWindowText(impl_->hwnd_, title.c_str());
}

bool Window::IsFullscreen() const {
  return impl_->is_fullscreen_;
}

void Window::SwitchFullscreen(bool fullscreen) {
  impl_->SwitchFullscreen(fullscreen);
}

bool Window::IsTopmost() const {
  return impl_->is_topmost_;
}

void Window::SwitchTopmost(bool topmost) {
  impl_->SwitchTopmost(topmost);
}

void Window::Show() {
  if (impl_->hwnd_) {
    impl_->is_shown_ = true;
    ::ShowWindow(impl_->hwnd_, SW_SHOW);
    ::UpdateWindow(impl_->hwnd_);
  }
}

void Window::Hide() {
  if (impl_->hwnd_) {
    impl_->is_shown_ = false;
    ::ShowWindow(impl_->hwnd_, SW_HIDE);
    ::UpdateWindow(impl_->hwnd_);
  }
}

void Window::Close() {
  if (impl_->hwnd_) {
    ::PostMessage(impl_->hwnd_, WM_CLOSE, 0, 0);
  }
}

void Window::ProcessEvent() {
  impl_->ProcessEvents();
}

void Window::AddWindowEventHandler(std::shared_ptr<IWindowEventHandler> handler) {
  impl_->handlers_.push_back(handler);
}

void Window::RemoveWindowEventHandler(std::shared_ptr<IWindowEventHandler> handler) {
  auto it = std::find(impl_->handlers_.begin(), impl_->handlers_.end(), handler);
  if (it != impl_->handlers_.end()) {
    impl_->handlers_.erase(it);
  }
}
