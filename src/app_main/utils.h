#pragma once

#include "types.h"

#include <chrono>
#include <filesystem>
#include <string_view>
#include <windows.h>
#include <winrt/base.h>

namespace yab {

class Util {
public:
  static std::string ToStr(const std::wstring_view& wstr) {
    int size_needed = ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), nullptr, 0, nullptr, nullptr);
    std::string str(size_needed, '\0');
    ::WideCharToMultiByte(CP_UTF8, 0, wstr.data(), (int)wstr.size(), str.data(), size_needed, nullptr, nullptr);
    return str;
  }

  static std::filesystem::path GetExecutionDir() {
    std::string app_file_path(_MAX_PATH - 1, '\0');
    ::GetModuleFileNameA(nullptr, app_file_path.data(), _MAX_PATH);
    return std::filesystem::path(app_file_path).parent_path();
  }

  static DirectX::XMMATRIX ToXmMatrix(const Pose& pose) {
    using namespace DirectX;
    XMVECTOR quat = XMVectorSet(pose.orientation.x, pose.orientation.y, pose.orientation.z, pose.orientation.w);
    XMVECTOR pos = XMVectorSet(pose.position.x, pose.position.y, pose.position.z, 1.0f);
    return XMMatrixAffineTransformation({1.f, 1.f, 1.f}, XMVectorZero(), quat, pos);
  }
};

class Timer {
public:
  Timer(uint32_t spin_wait_threshold_ms = 2) : spin_wait_threshold_ms_{ std::chrono::milliseconds(spin_wait_threshold_ms) } {
    waitable_timer_.attach(::CreateWaitableTimerEx(
      nullptr, nullptr, CREATE_WAITABLE_TIMER_MANUAL_RESET | CREATE_WAITABLE_TIMER_HIGH_RESOLUTION, TIMER_ALL_ACCESS));
    Reset();
  }

  virtual ~Timer() {}

  void Reset() {
    tick_count_ = 0;
    t1_ = t0_ = std::chrono::high_resolution_clock::now().time_since_epoch();
  }

  template <typename T = std::chrono::milliseconds>
  T Tick(bool reset = false) {
    t1_ = std::chrono::high_resolution_clock::now().time_since_epoch();
    auto elapsed = t1_ - t0_;
    if (reset) {
      tick_count_ = 0;
      t0_ = t1_;
    }
    tick_count_++;
    return std::chrono::duration_cast<T>(elapsed);
  }

  float GetTickCountPerSecond() const {
    auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(t1_ - t0_).count();
    return elapsed == 0 ? 0.f : tick_count_ / (elapsed / 1000.f);
  }

  template <class Rep, class Period>
  void Wait(const std::chrono::duration<Rep, Period>& wait_time) {
    // Using high-resolution waitable timer to wait for large wait
    if (wait_time > spin_wait_threshold_ms_) {
      LARGE_INTEGER duration;
      duration.QuadPart = -std::chrono::duration_cast<std::chrono::nanoseconds>(wait_time).count() / 100; // 100ns unit; minus means relative

      ::SetWaitableTimer(waitable_timer_.get(), &duration, 0, nullptr, nullptr, false);
      ::WaitForSingleObject(waitable_timer_.get(), INFINITE);
    }
    // Otherwise, we will use busy loop instead. Note that hybrid (sleep until threshold and busy loop) implementation
    // behave worse under (even slight) workload condition.
    else {
      auto t0 = std::chrono::high_resolution_clock::now();
      while (std::chrono::high_resolution_clock::now() - t0 < wait_time) {
        ::YieldProcessor();
      }
    }

  }

protected:
  winrt::handle waitable_timer_;
  std::chrono::milliseconds spin_wait_threshold_ms_;
  uint32_t tick_count_;
  std::chrono::steady_clock::duration t0_;
  std::chrono::steady_clock::duration t1_;
};

}
