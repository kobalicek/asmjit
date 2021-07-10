// This file is part of AsmJit project <https://asmjit.com>
//
// See asmjit.h or LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#include "../core/api-build_p.h"
#include "../core/codeholder.h"
#include "../core/codewriter_p.h"

ASMJIT_BEGIN_NAMESPACE

bool CodeWriterUtils::encodeOffset32(uint32_t* dst, int64_t offset64, const OffsetFormat& format) noexcept {
  uint32_t bitCount = format.immBitCount();
  uint32_t bitShift = format.immBitShift();
  uint32_t discardLsb = format.immDiscardLsb();

  if (!bitCount || bitCount > format.valueSize() * 8u)
    return false;

  if (discardLsb) {
    ASMJIT_ASSERT(discardLsb <= 32);
    if ((offset64 & Support::lsbMask<uint32_t>(discardLsb)) != 0)
      return false;
    offset64 >>= discardLsb;
  }

  if (!Support::isInt32(offset64))
    return false;

  int32_t offset32 = int32_t(offset64);
  if (!Support::isEncodableOffset32(offset32, bitCount))
    return false;

  switch (format.type()) {
    case OffsetFormat::kTypeCommon: {
      *dst = (uint32_t(offset32) & Support::lsbMask<uint32_t>(bitCount)) << bitShift;
      return true;
    }

    case OffsetFormat::kTypeAArch64_ADR:
    case OffsetFormat::kTypeAArch64_ADRP: {
      // Sanity checks.
      if (format.valueSize() != 4 || bitCount != 21 || bitShift != 5)
        return false;

      uint32_t immLo = uint32_t(offset32) & 0x3u;
      uint32_t immHi = uint32_t(offset32 >> 2) & Support::lsbMask<uint32_t>(19);

      *dst = (immLo << 29) | (immHi << 5);
      return true;
    }

    default:
      return false;
  }
}

bool CodeWriterUtils::encodeOffset64(uint64_t* dst, int64_t offset64, const OffsetFormat& format) noexcept {
  uint32_t bitCount = format.immBitCount();
  uint32_t discardLsb = format.immDiscardLsb();

  if (!bitCount || bitCount > format.valueSize() * 8u)
    return false;

  if (discardLsb) {
    ASMJIT_ASSERT(discardLsb <= 32);
    if ((offset64 & Support::lsbMask<uint32_t>(discardLsb)) != 0)
      return false;
    offset64 >>= discardLsb;
  }

  if (!Support::isEncodableOffset64(offset64, bitCount))
    return false;

  switch (format.type()) {
    case OffsetFormat::kTypeCommon: {
      *dst = (uint64_t(offset64) & Support::lsbMask<uint64_t>(bitCount)) << format.immBitShift();
      return true;
    }

    default:
      return false;
  }
}

bool CodeWriterUtils::writeOffset(void* dst, int64_t offset64, const OffsetFormat& format) noexcept {
  // Offset the destination by ValueOffset so the `dst` points to the
  // patched word instead of the beginning of the patched region.
  dst = static_cast<char*>(dst) + format.valueOffset();

  switch (format.valueSize()) {
    case 1: {
      uint32_t mask;
      if (!encodeOffset32(&mask, offset64, format))
        return false;

      Support::writeU8(dst, Support::readU8(dst) | mask);
      return true;
    }

    case 2: {
      uint32_t mask;
      if (!encodeOffset32(&mask, offset64, format))
        return false;

      Support::writeU16uLE(dst, Support::readU16uLE(dst) | mask);
      return true;
    }

    case 4: {
      uint32_t mask;
      if (!encodeOffset32(&mask, offset64, format))
        return false;

      Support::writeU32uLE(dst, Support::readU32uLE(dst) | mask);
      return true;
    }

    case 8: {
      uint64_t mask;
      if (!encodeOffset64(&mask, offset64, format))
        return false;

      Support::writeU64uLE(dst, Support::readU64uLE(dst) | mask);
      return true;
    }

    default:
      return false;
  }
}

ASMJIT_END_NAMESPACE
