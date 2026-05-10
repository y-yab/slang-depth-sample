#pragma once

#include <stdint.h>

namespace yab {

class IWindowEventHandler {
public:
  virtual ~IWindowEventHandler() {}

  virtual void OnWindowClose() = 0;
  virtual void OnKeyboard(uint32_t key_code, bool is_keydown) = 0;

  enum class MouseButton {
    Left,
    Right,
    Center,
  };
  enum class MouseAction {
    Release,
    Press,
  };
  virtual void OnMouseButton(MouseButton button, MouseAction action, float x, float y) = 0;
  virtual void OnMouseMove(float x, float y) = 0;
  virtual void OnMouseWheel(float delta) = 0;
};

}
