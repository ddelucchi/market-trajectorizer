#pragma once
#include "mt/core/types.hpp"
#include <string>
#include <string_view>

namespace mt::json_io {

// Production JSON text IO + deterministic key readers used by config loaders.
std::string read_text(std::string_view path);
void        write_text(std::string_view path, std::string_view text);

// Lightweight value-readers for engine configs (so headers don't pull nlohmann publicly).
real        read_real(std::string_view json_text, std::string_view key, real def);
int         read_int (std::string_view json_text, std::string_view key, int  def);
bool        read_bool(std::string_view json_text, std::string_view key, bool def);

}  // namespace mt::json_io
