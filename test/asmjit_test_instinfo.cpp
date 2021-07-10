// This file is part of AsmJit project <https://asmjit.com>
//
// See asmjit.h or LICENSE.md for license and copyright information
// SPDX-License-Identifier: Zlib

#include <asmjit/core.h>

#if !defined(ASMJIT_NO_X86)
#include <asmjit/x86.h>
#endif

#include <stdio.h>

using namespace asmjit;

static char accessLetter(bool r, bool w) noexcept {
  return r && w ? 'X' : r ? 'R' : w ? 'W' : '_';
}

static void printInfo(Arch arch, const BaseInst& inst, const Operand_* operands, size_t opCount) {
  StringTmp<512> sb;

  // Read & Write Information
  // ------------------------

  InstRWInfo rw;
  InstAPI::queryRWInfo(arch, inst, operands, opCount, &rw);

#ifndef ASMJIT_NO_LOGGING
  Formatter::formatInstruction(sb, FormatFlags::kNone, nullptr, arch, inst, operands, opCount);
#else
  sb.append("<Logging-Not-Available>");
#endif
  sb.append("\n");

  sb.append("  Operands:\n");
  for (uint32_t i = 0; i < rw.opCount(); i++) {
    const OpRWInfo& op = rw.operand(i);

    sb.appendFormat("    [%u] Op=%c Read=%016llX Write=%016llX Extend=%016llX",
                    i,
                    accessLetter(op.isRead(), op.isWrite()),
                    op.readByteMask(),
                    op.writeByteMask(),
                    op.extendByteMask());

    if (op.isMemBaseUsed()) {
      sb.appendFormat(" Base=%c", accessLetter(op.isMemBaseRead(), op.isMemBaseWrite()));
      if (op.isMemBasePreModify())
        sb.appendFormat(" <PRE>");
      if (op.isMemBasePostModify())
        sb.appendFormat(" <POST>");
    }

    if (op.isMemIndexUsed()) {
      sb.appendFormat(" Index=%c", accessLetter(op.isMemIndexRead(), op.isMemIndexWrite()));
    }

    sb.append("\n");
  }

  if (rw.readFlags() | rw.writeFlags()) {
    sb.append("  Flags: \n");

    struct FlagMap {
      uint32_t flag;
      char name[4];
    };

    static const FlagMap flagMap[] = {
      { x86::Status::kCF, "CF" },
      { x86::Status::kOF, "OF" },
      { x86::Status::kSF, "SF" },
      { x86::Status::kZF, "ZF" },
      { x86::Status::kAF, "AF" },
      { x86::Status::kPF, "PF" },
      { x86::Status::kDF, "DF" },
      { x86::Status::kIF, "IF" },
      { x86::Status::kAC, "AC" },
      { x86::Status::kC0, "C0" },
      { x86::Status::kC1, "C1" },
      { x86::Status::kC2, "C2" },
      { x86::Status::kC3, "C3" }
    };

    sb.append("    ");
    for (uint32_t f = 0; f < 13; f++) {
      char c = accessLetter((rw.readFlags() & flagMap[f].flag) != 0,
                            (rw.writeFlags() & flagMap[f].flag) != 0);
      if (c != '_')
        sb.appendFormat("%s=%c ", flagMap[f].name, c);
    }

    sb.append("\n");
  }

  // CPU Features
  // ------------

  CpuFeatures features;
  InstAPI::queryFeatures(arch, inst, operands, opCount, &features);

#ifndef ASMJIT_NO_LOGGING
  if (!features.empty()) {
    sb.append("  Features:\n");
    sb.append("    ");

    bool first = true;
    CpuFeatures::Iterator it(features.iterator());
    while (it.hasNext()) {
      uint32_t featureId = uint32_t(it.next());
      if (!first)
        sb.append(" & ");
      Formatter::formatFeature(sb, arch, featureId);
      first = false;
    }
    sb.append("\n");
  }
#endif

  printf("%s\n", sb.data());
}

template<typename... Args>
static void printInfoSimple(Arch arch,InstId instId, InstOptions options, Args&&... args) {
  BaseInst inst(instId);
  inst.addOptions(options);
  Operand_ opArray[] = { std::forward<Args>(args)... };
  printInfo(arch, inst, opArray, sizeof...(args));
}

template<typename... Args>
static void printInfoExtra(Arch arch, InstId instId, InstOptions options, const BaseReg& extraReg, Args&&... args) {
  BaseInst inst(instId);
  inst.addOptions(options);
  inst.setExtraReg(extraReg);
  Operand_ opArray[] = { std::forward<Args>(args)... };
  printInfo(arch, inst, opArray, sizeof...(args));
}

static void testX86Arch() {
#if !defined(ASMJIT_NO_X86)
  using namespace x86;
  Arch arch = Arch::kX64;

  printInfoSimple(arch, Inst::kIdAdd, InstOptions::kNone, eax, ebx);
  printInfoSimple(arch, Inst::kIdLods, InstOptions::kNone, eax, dword_ptr(rsi));

  printInfoSimple(arch, Inst::kIdPshufd, InstOptions::kNone, xmm0, xmm1, imm(0));
  printInfoSimple(arch, Inst::kIdPabsb, InstOptions::kNone, mm1, mm2);
  printInfoSimple(arch, Inst::kIdPabsb, InstOptions::kNone, xmm1, xmm2);
  printInfoSimple(arch, Inst::kIdPextrw, InstOptions::kNone, eax, mm1, imm(0));
  printInfoSimple(arch, Inst::kIdPextrw, InstOptions::kNone, eax, xmm1, imm(0));
  printInfoSimple(arch, Inst::kIdPextrw, InstOptions::kNone, ptr(rax), xmm1, imm(0));

  printInfoSimple(arch, Inst::kIdVpdpbusd, InstOptions::kNone, xmm0, xmm1, xmm2);
  printInfoSimple(arch, Inst::kIdVpdpbusd, InstOptions::kX86_Vex, xmm0, xmm1, xmm2);

  printInfoSimple(arch, Inst::kIdVaddpd, InstOptions::kNone, ymm0, ymm1, ymm2);
  printInfoSimple(arch, Inst::kIdVaddpd, InstOptions::kNone, ymm0, ymm30, ymm31);
  printInfoSimple(arch, Inst::kIdVaddpd, InstOptions::kNone, zmm0, zmm1, zmm2);

  printInfoExtra(arch, Inst::kIdVaddpd, InstOptions::kNone, k1, zmm0, zmm1, zmm2);
  printInfoExtra(arch, Inst::kIdVaddpd, InstOptions::kX86_ZMask, k1, zmm0, zmm1, zmm2);
#endif
}

int main() {
  printf("AsmJit Instruction Info Test-Suite v%u.%u.%u\n",
    unsigned((ASMJIT_LIBRARY_VERSION >> 16)       ),
    unsigned((ASMJIT_LIBRARY_VERSION >>  8) & 0xFF),
    unsigned((ASMJIT_LIBRARY_VERSION      ) & 0xFF));
  printf("\n");

  testX86Arch();

  return 0;
}
