#include <gtest/gtest.h>
#include "utils.h"

using namespace yab;

TEST(UtilsTest, GetExecutionDir) {
  auto exe_dir = Util::GetExecutionDir();
  EXPECT_FALSE(exe_dir.empty());
}
