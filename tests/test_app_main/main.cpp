#include <gtest/gtest.h>
#include "logger_helper.h"

int main(int argc, char** argv) {
    yab::LogDesc log_desc;
    log_desc.log_base_name_ = "test_slang_depth_sample";
    log_desc.enable_console_logger_ = true;
    auto logger_raii_ = std::make_unique<yab::LoggerRAII>(log_desc);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}
