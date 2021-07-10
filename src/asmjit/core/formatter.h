// This file is part of AsmJit project <https://asmjit.com>
//
// See asmjit.h or LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#ifndef ASMJIT_CORE_FORMATTER_H_INCLUDED
#define ASMJIT_CORE_FORMATTER_H_INCLUDED

#include "../core/globals.h"
#include "../core/inst.h"
#include "../core/string.h"
#include "../core/support.h"

ASMJIT_BEGIN_NAMESPACE

//! \addtogroup asmjit_logging
//! \{

class BaseBuilder;
class BaseEmitter;
class BaseNode;
struct Operand_;

//! Format flags used by \ref Logger and \ref FormatOptions.
enum class FormatFlags : uint32_t {
  //! No formatting flags.
  kNone = 0u,

  //! Show also binary form of each logged instruction (Assembler).
  kMachineCode = 0x00000001u,
  //! Show a text explanation of some immediate values.
  kExplainImms = 0x00000002u,
  //! Use hexadecimal notation of immediate values.
  kHexImms = 0x00000004u,
  //! Use hexadecimal notation of address offsets.
  kHexOffsets = 0x00000008u,
  //! Show casts between virtual register types (Compiler).
  kRegCasts = 0x00000010u,
  //! Show positions associated with nodes (Compiler).
  kPositions = 0x00000020u,
  //! Annotate nodes that are lowered by passes.
  kAnnotations = 0x00000040u,

  // TODO: These must go, keep this only for formatting.
  //! Show an additional output from passes.
  kDebugPasses = 0x00000080u,
  //! Show an additional output from RA.
  kDebugRA = 0x00000100u
};
ASMJIT_DEFINE_ENUM_FLAGS(FormatFlags)

//! Format indentation group, used by \ref FormatOptions.
enum class FormatIndentationGroup : uint32_t {
  //! Indentation used for instructions and directives.
  kCode = 0u,
  //! Indentation used for labels and function nodes.
  kLabel = 1u,
  //! Indentation used for comments (not inline comments).
  kComment = 2u,

  //! \cond INTERNAL
  //! Reserved for future use.
  kReserved = 3u,
  //! \endcond

  //! Maximum value of `FormatIndentationGroup`.
  kMaxValue = kReserved
};

//! Formatting options used by \ref Logger and \ref Formatter.
class FormatOptions {
public:
  //! Format flags.
  FormatFlags _flags;
  //! Indentation by type.
  uint8_t _indentation[uint32_t(FormatIndentationGroup::kMaxValue) + 1];

  //! \name Construction & Destruction
  //! \{

  //! Creates a default-initialized FormatOptions.
  inline constexpr FormatOptions() noexcept
    : _flags(FormatFlags::kNone),
      _indentation { 0, 0, 0, 0 } {}

  inline constexpr FormatOptions(const FormatOptions& other) noexcept = default;
  inline FormatOptions& operator=(const FormatOptions& other) noexcept = default;

  //! Resets FormatOptions to its default initialized state.
  inline void reset() noexcept {
    _flags = FormatFlags::kNone;
    _indentation[0] = 0;
    _indentation[1] = 0;
    _indentation[2] = 0;
    _indentation[3] = 0;
  }

  //! \}

  //! \name Accessors
  //! \{

  //! Returns format flags.
  inline constexpr FormatFlags flags() const noexcept { return _flags; }
  //! Tests whether the given `flag` is set in format flags.
  inline constexpr bool hasFlag(FormatFlags flag) const noexcept { return Support::test(_flags, flag); }

  //! Resets all format flags to `flags`.
  inline void setFlags(FormatFlags flags) noexcept { _flags = flags; }
  //! Adds `flags` to format flags.
  inline void addFlags(FormatFlags flags) noexcept { _flags |= flags; }
  //! Removes `flags` from format flags.
  inline void clearFlags(FormatFlags flags) noexcept { _flags &= ~flags; }

  //! Returns indentation for the given indentation `group`.
  inline constexpr uint8_t indentation(FormatIndentationGroup group) const noexcept { return _indentation[uint32_t(group)]; }
  //! Sets indentation for the given indentation `group`.
  inline void setIndentation(FormatIndentationGroup group, uint32_t n) noexcept { _indentation[uint32_t(group)] = uint8_t(n); }
  //! Resets indentation for the given indentation `group` to zero.
  inline void resetIndentation(FormatIndentationGroup group) noexcept { _indentation[uint32_t(group)] = uint8_t(0); }

  //! \}
};

//! Provides formatting functionality to format operands, instructions, and nodes.
namespace Formatter {

#ifndef ASMJIT_NO_LOGGING

//! Appends a formatted `typeId` to the output string `sb`.
ASMJIT_API Error formatTypeId(
  String& sb,
  TypeId typeId) noexcept;

//! Appends a formatted `featureId` to the output string `sb`.
//!
//! See \ref CpuFeatures.
ASMJIT_API Error formatFeature(
  String& sb,
  Arch arch,
  uint32_t featureId) noexcept;

//! Appends a formatted register to the output string `sb`.
//!
//! \note Emitter is optional, but it's required to format virtual registers, which won't be formatted properly
//! if the `emitter` is not provided.
ASMJIT_API Error formatRegister(
  String& sb,
  FormatFlags formatFlags,
  const BaseEmitter* emitter,
  Arch arch,
  RegType regType,
  uint32_t regId) noexcept;

//! Appends a formatted label to the output string `sb`.
//!
//! \note Emitter is optional, but it's required to format named labels properly, otherwise the formatted as
//! it is an anonymous label.
ASMJIT_API Error formatLabel(
  String& sb,
  FormatFlags formatFlags,
  const BaseEmitter* emitter,
  uint32_t labelId) noexcept;

//! Appends a formatted operand to the output string `sb`.
//!
//! \note Emitter is optional, but it's required to format named labels and virtual registers. See
//! \ref formatRegister() and \ref formatLabel() for more details.
ASMJIT_API Error formatOperand(
  String& sb,
  FormatFlags formatFlags,
  const BaseEmitter* emitter,
  Arch arch,
  const Operand_& op) noexcept;

//! Appends a formatted data-type to the output string `sb`.
ASMJIT_API Error formatDataType(
  String& sb,
  FormatFlags formatFlags,
  Arch arch,
  TypeId typeId) noexcept;

//! Appends a formatted data to the output string `sb`.
ASMJIT_API Error formatData(
  String& sb,
  FormatFlags formatFlags,
  Arch arch,
  TypeId typeId, const void* data, size_t itemCount, size_t repeatCount = 1) noexcept;

//! Appends a formatted instruction to the output string `sb`.
//!
//! \note Emitter is optional, but it's required to format named labels and virtual registers. See
//! \ref formatRegister() and \ref formatLabel() for more details.
ASMJIT_API Error formatInstruction(
  String& sb,
  FormatFlags formatFlags,
  const BaseEmitter* emitter,
  Arch arch,
  const BaseInst& inst, const Operand_* operands, size_t opCount) noexcept;

#ifndef ASMJIT_NO_BUILDER
//! Appends a formatted node to the output string `sb`.
//!
//! The `node` must belong to the provided `builder`.
ASMJIT_API Error formatNode(
  String& sb,
  FormatFlags formatFlags,
  const BaseBuilder* builder,
  const BaseNode* node) noexcept;

//! Appends formatted nodes to the output string `sb`.
//!
//! All nodes that are part of the given `builder` will be appended.
ASMJIT_API Error formatNodeList(
  String& sb,
  FormatFlags formatFlags,
  const BaseBuilder* builder) noexcept;

//! Appends formatted nodes to the output string `sb`.
//!
//! This function works the same as \ref formatNode(), but appends more nodes to the output string,
//! separating each node with a newline '\n' character.
ASMJIT_API Error formatNodeList(
  String& sb,
  FormatFlags formatFlags,
  const BaseBuilder* builder,
  const BaseNode* begin,
  const BaseNode* end) noexcept;
#endif

#endif

} // {Formatter}

//! \}

ASMJIT_END_NAMESPACE

#endif // ASMJIT_CORE_FORMATTER_H_INCLUDED
