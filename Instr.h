#pragma once

#include <cstdint>
#include <variant>
#include <vector>

namespace Instr {

struct Add {
  std::uint8_t value;
};
struct Shift {
  std::ptrdiff_t offset;
};
struct Print {};
struct Read {};
struct Loop;

using Instruction = std::variant<Add, Shift, Print, Read, Loop>;

struct Loop {
  std::vector<Instruction> instrs;
};

}  // namespace Instr
