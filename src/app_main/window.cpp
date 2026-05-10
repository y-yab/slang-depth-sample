#include "pch.h"

#include "window.h"
#include "window_event_handler.h"
#include <vector>

using namespace yab;

struct Window::Impl {
  std::wstring name_;
  uint32_t width_;
  uint32_t height_;
  HWND hwhd_;
  std::vector<std::shared_ptr<IWindowEventHandler>> handlers_;

  Impl(const std::wstring& name, uint32_t width, uint32_t height)
    : name_{ name }, width_{ width }, height_{ height }
  {
    WNDCLASSEX wc{};
    wc.style = CS_HREDRAW | CS_VREDRAW | CS_OWNDC | CS_DBLCLKS;
    wc.lpfnWndProc = (WNDPROC)Impl::WindProc;
    wc.hInstance = ::GetModuleHandle(NULL);
    wc.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wc.hbrBackground = (HBRUSH)::GetStockObject(BLACK_BRUSH);
    wc.lpszClassName = name.c_str();
    wc.cbSize = sizeof(WNDCLASSEX);
    ::RegisterClassEx(&wc);

    constexpr auto WINDOW_STYLE = WS_OVERLAPPEDWINDOW;

    RECT rect{ 0, 0, static_cast<long>(width), static_cast<long>(height) };
    ::AdjustWindowRect(&rect, WINDOW_STYLE, true);

    hwhd_ = ::CreateWindowEx(
        WS_EX_APPWINDOW,
        name.c_str(),
        name.c_str(),
        WINDOW_STYLE,
        0, 0, width, height,
        NULL, NULL, wc.hInstance, this
    );
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

  LRESULT ProcessMessage(HWND hwnd, UINT msg, WPARAM wparam, LPARAM lparam) {
    LRESULT res = 0;
    UINT x, y;

    switch (msg) {
      case WM_SIZE:
        break;
      case WM_CLOSE:
        for (auto& it : handlers_) {
          it->OnWindowClose();
        }
        ::DestroyWindow(hwnd);
        break;
      case WM_DESTROY:
        ::PostQuitMessage(0);
        break;
      case WM_KEYDOWN:
      case WM_KEYUP:
        for (auto& it : handlers_) {
          it->OnKeyboard((uint32_t)wparam, (msg == WM_KEYDOWN));
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
};

Window::Window(const std::wstring& name, uint32_t width, uint32_t height)
  : impl_{ std::make_unique<Impl>(name, width, height) }
{
}

Window::~Window() {
}

HWND Window::GetWindowHandle() const {
  return impl_->hwhd_;
}

uint32_t Window::GetWidth() const {
  return impl_->width_;
}

uint32_t Window::GetHeight() const {
  return impl_->height_;
}

void Window::SetTitle(const std::wstring& title) {
  ::SetWindowText(impl_->hwhd_, title.c_str());
}

void Window::Show() {
  if (impl_->hwhd_) {
    ::ShowWindow(impl_->hwhd_, SW_SHOW);
    ::UpdateWindow(impl_->hwhd_);
  }
}

void Window::Hide() {
  if (impl_->hwhd_) {
    ::ShowWindow(impl_->hwhd_, SW_HIDE);
    ::UpdateWindow(impl_->hwhd_);
  }
}

void Window::Close() {
  if (impl_->hwhd_) {
    ::PostMessage(impl_->hwhd_, WM_CLOSE, 0, 0);
  }
}

void Window::ProcessEvent() {
  MSG msg;
  if (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
    ::TranslateMessage(&msg);
    ::DispatchMessage(&msg);
  }
}

void Window::AddWindowEventHandler(std::shared_ptr<IWindowEventHandler> handler) {
  impl_->handlers_.push_back(handler);
}
