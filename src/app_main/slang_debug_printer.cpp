// copied from https://github.com/shader-slang/slangpy/blob/main/src/sgl/device/print.cpp
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
#include "pch.h"
#include "slang_debug_printer.h"

#include <assert.h>
#include <exception>
#include <fmt/args.h>
#include <map>
#include <span>

#define SGL_THROW(msg) throw std::runtime_error(msg)
#define SGL_ASSERT(cond) assert(cond)


namespace sgl {
namespace print_buffer {

/// Argument value kind.
/// This needs to be in sync with the enum in print.slang.
enum class Kind {
  scalar,
  vector,
  matrix,
};
static_assert(int(Kind::matrix) <= 16, "Kind is encoded in 4 bits");

/// Argument value type.
/// This needs to be in sync with the enum in print.slang.
enum class Type {
  boolean,
  int8,
  int16,
  int32,
  int64,
  uint8,
  uint16,
  uint32,
  uint64,
  float16,
  float32,
  float64,
  printable_string,
  count,
};
static_assert(int(Type::count) <= 16, "Type is encoded in 4 bits");

/// Argument value layout.
struct Layout {
  Kind kind;
  Type type;
  uint32_t rows;
  uint32_t cols;
};

/// Storage for a single argument value.
template<typename T>
struct Value {
  static constexpr size_t MAX_ELEMENT_COUNT = 16;
  Layout layout;
  T elements[MAX_ELEMENT_COUNT];
  uint32_t element_count;
};
} // namespace print_buffer
} // namespace sgl

/// Custom formatter for argument values.
/// Supports printing scalars, vectors and matrices.
/// Takes into account the formatting specifiers of the underlying element type.
template<typename T>
struct fmt::formatter<sgl::print_buffer::Value<T>> : formatter<T> {
  template<typename FormatContext>
  auto format(const sgl::print_buffer::Value<T>& v, FormatContext& ctx) const
  {
    auto out = ctx.out();
    switch (v.layout.kind) {
      case sgl::print_buffer::Kind::scalar:
        out = formatter<T>::format(v.elements[0], ctx);
        break;
      case sgl::print_buffer::Kind::vector:
        for (uint32_t i = 0; i < v.element_count; ++i) {
          out = fmt::format_to(out, "{}", (i == 0) ? "{" : ", ");
          out = formatter<T>::format(v.elements[i], ctx);
        }
        out = fmt::format_to(out, "}}");
        break;
      case sgl::print_buffer::Kind::matrix:
        for (uint32_t r = 0; r < v.layout.rows; ++r) {
          out = fmt::format_to(out, "{}", (r == 0) ? "{" : ", ");
          for (uint32_t c = 0; c < v.layout.cols; ++c) {
            out = fmt::format_to(out, "{}", (c == 0) ? "{" : ", ");
            out = formatter<T>::format(v.elements[r * v.layout.cols + c], ctx);
          }
          out = fmt::format_to(out, "}}");
        }
        out = fmt::format_to(out, "}}");
        break;
    }
    return out;
  }
};

namespace sgl {
namespace print_buffer {

template<typename T>
inline Value<T> decode_value(
    std::span<const uint8_t> data,
    const Layout layout,
    const std::map<uint32_t, std::string>* hashed_strings = nullptr
)
{
  Value<T> value;
  value.layout = layout;

  switch (layout.kind) {
    case Kind::scalar:
      value.element_count = 1;
      break;
    case Kind::vector:
      value.element_count = layout.rows;
      break;
    case Kind::matrix:
      value.element_count = layout.rows * layout.cols;
      break;
    default:
      SGL_THROW("Invalid argument kind in print()");
  }

  SGL_ASSERT(value.element_count <= Value<T>::MAX_ELEMENT_COUNT);

  if constexpr (std::is_same_v<T, std::string_view>) {
    SGL_ASSERT(hashed_strings);
    SGL_ASSERT(layout.type == Type::printable_string);
    for (uint32_t i = 0; i < value.element_count; ++i) {
      const int32_t& string_hash = *reinterpret_cast<const int32_t*>(data.data() + i * 4);
      auto it = hashed_strings->find(string_hash);
      if (it != hashed_strings->end())
        value.elements[i] = it->second;
      else
        value.elements[i] = "<unknown string>";
    }
    return value;
  }
  else {
    // Elements are aligned to 4 bytes.
    uint32_t element_size = ((uint32_t(sizeof(T)) + 3) / 4) * 4;
    SGL_ASSERT(data.size() >= element_size * value.element_count);

    for (uint32_t i = 0; i < value.element_count; ++i)
      std::memcpy(&value.elements[i], data.data() + i * element_size, element_size);

    return value;
  }
}

inline void decode_arg(
    std::span<const uint8_t> data,
    fmt::dynamic_format_arg_store<fmt::format_context>& arg_store,
    const std::map<uint32_t, std::string>& hashed_strings
)
{
  SGL_ASSERT(data.size() >= 4);
  uint32_t header = *reinterpret_cast<const uint32_t*>(data.data());
  uint32_t arg_size = header & 0xffff;
  SGL_ASSERT(data.size() == arg_size);
  Layout layout;
  layout.kind = Kind((header >> 28) & 0xf);
  layout.type = Type((header >> 24) & 0xf);
  layout.rows = (header >> 20) & 0xf;
  layout.cols = (header >> 16) & 0xf;

  switch (layout.type) {
    case Type::boolean:
      arg_store.push_back(decode_value<bool>(data.subspan(4), layout));
      break;
    case Type::int8:
      arg_store.push_back(decode_value<int8_t>(data.subspan(4), layout));
      break;
    case Type::int16:
      arg_store.push_back(decode_value<int16_t>(data.subspan(4), layout));
      break;
    case Type::int32:
      arg_store.push_back(decode_value<int32_t>(data.subspan(4), layout));
      break;
    case Type::int64:
      arg_store.push_back(decode_value<int64_t>(data.subspan(4), layout));
      break;
    case Type::uint8:
      arg_store.push_back(decode_value<uint8_t>(data.subspan(4), layout));
      break;
    case Type::uint16:
      arg_store.push_back(decode_value<uint16_t>(data.subspan(4), layout));
      break;
    case Type::uint32:
      arg_store.push_back(decode_value<uint32_t>(data.subspan(4), layout));
      break;
    case Type::uint64:
      arg_store.push_back(decode_value<uint64_t>(data.subspan(4), layout));
      break;
      // case Type::float16:
      //     arg_store.push_back(decode_value<float16_t>(data.subspan(4), layout));
      //     break;
    case Type::float32:
      arg_store.push_back(decode_value<float>(data.subspan(4), layout));
      break;
    case Type::float64:
      arg_store.push_back(decode_value<double>(data.subspan(4), layout));
      break;
    case Type::printable_string:
      arg_store.push_back(decode_value<std::string_view>(data.subspan(4), layout, &hashed_strings));
      break;
    default:
      SGL_THROW("Invalid argument type in print()");
  }
}

template<typename Output>
inline void
decode_msg(std::span<const uint8_t> data, const std::map<uint32_t, std::string>& hashed_strings, Output output)
{
  const uint8_t* ptr = data.data();

  // Decode message header.
  SGL_ASSERT(data.size() >= 12);
  uint32_t msg_size = *reinterpret_cast<const uint32_t*>(ptr);
  SGL_ASSERT(data.size() == msg_size);
  uint32_t fmt_hash = *reinterpret_cast<const uint32_t*>(ptr + 4);
  uint32_t arg_count = *reinterpret_cast<const uint32_t*>(ptr + 8);
  const uint8_t* end = ptr + msg_size;
  ptr += 12;

  // Decode arguments and append to argument store.
  fmt::dynamic_format_arg_store<fmt::format_context> arg_store;
  for (uint32_t i = 0; i < arg_count; ++i) {
    uint32_t arg_size = *reinterpret_cast<const uint32_t*>(ptr) & 0xffff;
    SGL_ASSERT(ptr + arg_size <= end);
    decode_arg(std::span(ptr, arg_size), arg_store, hashed_strings);
    ptr += arg_size;
  }

  /// Lookup format string.
  std::string_view fmt;
  if (auto it = hashed_strings.find(fmt_hash); it != hashed_strings.end())
    fmt = it->second;

  /// Output formatted string.
  try {
    output(fmt::vformat(fmt, arg_store));
  }
  catch (const fmt::format_error& e) {
    SGL_THROW(std::format("Invalid shader print() formatting: {}\nFormat string:\n{}", e.what(), fmt));
  }
}

template<typename Output>
inline void
decode_buffer(const void* data, size_t size, const std::map<uint32_t, std::string>& hashed_strings, Output output)
{
  const uint8_t* ptr = reinterpret_cast<const uint8_t*>(data);
  ptr += 4; // Skip header (buffer capacity).
  uint32_t buffer_size = *reinterpret_cast<const uint32_t*>(ptr);
  ptr += 4; // Skip header (buffer size).
  // Clamp buffer size to the actual size of the buffer (in case the device has overflown the buffer).
  buffer_size = std::min(buffer_size, uint32_t(size) - 8);
  const uint8_t* end = ptr + buffer_size;

  size_t count = 0;
  while (ptr < end) {
    uint32_t msg_size = *reinterpret_cast<const uint32_t*>(ptr);
    if (msg_size == 0xffffffff) {
      SPDLOG_WARN("Print buffer overflow!");
      break;
    }
    SGL_ASSERT(ptr + msg_size <= end);
    decode_msg(std::span(ptr, msg_size), hashed_strings, output);
    ++count;
    ptr += msg_size;
  }
}

} // namespace print_buffer

using namespace sgl;

struct DebugPrinter::Impl {
  uint64_t buf_size_;
  Slang::ComPtr<rhi::IBuffer> buffer_;
  std::map<uint32_t, std::string> hashed_strings_;

  Impl(rhi::IDevice* device, uint64_t buf_size) : buf_size_(buf_size) {
    // Create buffer
    {
      rhi::BufferDesc desc{};
      desc.size = buf_size;
      desc.elementSize = sizeof(uint32_t);
      desc.usage = rhi::BufferUsage::UnorderedAccess
        | rhi::BufferUsage::CopySource | rhi::BufferUsage::CopyDestination;
      desc.memoryType = rhi::MemoryType::DeviceLocal;
      desc.label = "DebugPrinter Buffer";
      buffer_ = device->createBuffer(desc);
    }

    // Write buffer header
    {
      auto queue = device->getQueue(rhi::QueueType::Graphics);
      auto command_encoder = queue->createCommandEncoder();

      command_encoder->setBufferState(buffer_, rhi::ResourceState::CopyDestination);
      uint32_t header[2] = { uint32_t(buf_size), 0 };
      command_encoder->uploadBufferData(buffer_, 0, sizeof(header), header);

      queue->submit(command_encoder->finish());
    }
  }
};


DebugPrinter::DebugPrinter(rhi::IDevice* device, uint64_t buf_size)
  : impl_(std::make_unique<Impl>(device, buf_size))
{
}

DebugPrinter::~DebugPrinter() {
}

// copied from https://github.com/shader-slang/slang/blob/master/examples/gpu-printing/gpu-printing.cpp
//
// One of the key ideas in this printing system is that strings
// are not encoded into the buffer of print commands directly,
// but are instead encoded using a hash of the string data.
//
// In order to map from a hash code back to the original string,
// the host side code for the printing system needs a way to
// pre-populate a lookup table with the strings that appear
// in a shader. The Slang reflection API provides a service to
// do exactly that.
//
void DebugPrinter::LoadStrings(slang::ProgramLayout* slang_reflection) {
  auto hashedStringCount = slang_reflection->getHashedStringCount();
  for (SlangUInt ii = 0; ii < hashedStringCount; ++ii)
  {
    // For each string we can fetch its bytes from the Slang
    // reflection data.
    //
    size_t stringSize = 0;
    char const* stringData = slang_reflection->getHashedString(ii, &stringSize);

    // Then we can compute the hash code for that string using
    // another Slang API function.
    //
    // Note: the exact hashing algorithm that Slang uses for
    // string literals is not currently documented, and may
    // change in future releases of the compiler.
    //
    auto hash = spComputeStringHash(stringData, stringSize);

    // The `GPUPrinting` implementation will store the mapping
    // from hash codes back to strings in a simple STL `map`.
    //
    impl_->hashed_strings_.insert(
        std::make_pair(hash, std::string(stringData, stringData + stringSize)));
  }
}

void DebugPrinter::Sink(rhi::IDevice* device, std::function<void(const std::string_view&)> sink) {
  Slang::ComPtr<ISlangBlob> blob;
  device->readBuffer(impl_->buffer_, 0, impl_->buf_size_, blob.writeRef());

  auto data = blob->getBufferPointer();
  auto dataSize = impl_->buf_size_;

  print_buffer::decode_buffer(
    data,
    dataSize,
    impl_->hashed_strings_,
    sink
  );
}

void DebugPrinter::Bind(rhi::ShaderCursor cursor)
{
  if (cursor.isValid()) {
    cursor = cursor.getField("g_debug_printer");
    if (cursor.isValid()) {
      cursor = cursor.getElement(0);
      if (cursor.isValid())
        cursor.setBinding(impl_->buffer_);
    }
  }
}

} // namespace sgl
