#pragma once

#include "non_copyable.h"

#include <memory>

namespace yab {

class App : NonCopyable {
public:
  App();
  ~App();

  int Run();

private:
  struct Impl;
  std::shared_ptr<Impl> impl_;
};

} // namespace yab
