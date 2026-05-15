#pragma once

#include "non_copyable.h"

#include <filesystem>

namespace yab {

class Config : NonCopyable {
public:
  ~Config();
  static Config& GetInstance();
  bool Load(const std::filesystem::path& yaml_file);

  bool is_box1_reverse_z_{};
  bool is_box2_reverse_z_{};
  bool is_render_target_reverse_z_{};
  bool is_texture_composition_{};

private:
  Config();
};

}
