#pragma once

#include <memory>

namespace yab {

class App {
public:
  App();
  ~App();

  int Run();

private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

} // namespace yab
