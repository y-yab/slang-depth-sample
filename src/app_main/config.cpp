#include "pch.h"
#include "config.h"

#pragma warning(push)
#pragma warning(disable : 4251 4275)
#include <yaml-cpp/yaml.h>
#pragma warning(pop)

#include <string>

namespace fs = std::filesystem;
using namespace yab;

namespace {
namespace detail {

YAML::Node ParseNode(const YAML::Node& node, const std::string& name) {
  auto ret = node[name];
  if (!ret) {
    SPDLOG_WARN("no {} node found", name);
  }

  return ret;
}

template <typename T>
void ParseNode(const YAML::Node& node, const std::string& name, T& out_value) {
  if (node && node[name]) {
    out_value = node[name].as<T>();
  }
  SPDLOG_INFO("{}: {}", name, out_value);
}

} // namespace detail
} // namespace (anonymous)

Config::Config() = default;
Config::~Config() = default;

Config& Config::GetInstance() {
  static Config instance;
  return instance;
}

bool Config::Load(const fs::path& yaml_file) {
  SPDLOG_INFO("{:=^80}", " Configurations ");
  SPDLOG_INFO("Loading config from file: {}", yaml_file.string());
  try {
    auto yaml = YAML::LoadFile(yaml_file.string());
    auto root_node = yaml["slang_depth_sample"];

    detail::ParseNode(root_node, "is_box1_reverse_z", is_box1_reverse_z_);
    detail::ParseNode(root_node, "is_box2_reverse_z", is_box2_reverse_z_);
    detail::ParseNode(root_node, "is_render_target_reverse_z", is_render_target_reverse_z_);
    detail::ParseNode(root_node, "is_texture_composition", is_texture_composition_);
  }
  catch (const std::exception& e) {
    SPDLOG_ERROR("Failed to load config file: {}, error: {}", yaml_file.string(), e.what());
    return false;
  }
  SPDLOG_INFO(std::string(80, '='));

  return true;
}
