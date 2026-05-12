#pragma once

#include "non_copyable.h"

#include <filesystem>
#include <memory>

namespace yab {

class SlangContext;

class SlangRendererMain : public NonCopyable {
public:
  SlangRendererMain() = delete;
  SlangRendererMain(std::shared_ptr<SlangContext> context);
  ~SlangRendererMain();

  void Render();
  void ReloadShader();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace yab
