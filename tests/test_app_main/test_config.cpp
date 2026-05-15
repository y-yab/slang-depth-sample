#include <gtest/gtest.h>
#include "config.h"
#include "utils.h"

using namespace yab;

TEST(ConfigTest, LoadConfig) {
  auto exe_dir = Util::GetExecutionDir();
  auto yaml_file = exe_dir / "slang_depth_sample.yaml";

  auto& config = Config::GetInstance();
  bool result = config.Load(yaml_file);
  EXPECT_TRUE(result);

  EXPECT_FALSE(config.is_reverse_z_);
  EXPECT_TRUE(config.is_texture_composition_);
}
