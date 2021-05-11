// AsmJit - Machine code generation for C++
//
//  * Official AsmJit Home Page: https://asmjit.com
//  * Official Github Repository: https://github.com/asmjit/asmjit
//
// Copyright (c) 2008-2020 The AsmJit Authors
//
// This software is provided 'as-is', without any express or implied
// warranty. In no event will the authors be held liable for any damages
// arising from the use of this software.
//
// Permission is granted to anyone to use this software for any purpose,
// including commercial applications, and to alter it and redistribute it
// freely, subject to the following restrictions:
//
// 1. The origin of this software must not be misrepresented; you must not
//    claim that you wrote the original software. If you use this software
//    in a product, an acknowledgment in the product documentation would be
//    appreciated but is not required.
// 2. Altered source versions must be plainly marked as such, and must not be
//    misrepresented as being the original software.
// 3. This notice may not be removed or altered from any source distribution.

#include <asmjit/core.h>

#if !defined(ASMJIT_NO_ARM) && ASMJIT_ARCH_ARM
#include <asmjit/a64.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

using namespace asmjit;

// Signature of the generated function.
typedef void (*SumIntsFunc)(int* dst, const int* a, const int* b);

// This function works with both a64::Assembler and a64::Builder. It shows how
// `a64::Emitter` can be used to make your code more generic.
static void makeRawFunc(a64::Emitter* emitter) noexcept {
  // Decide which registers will be mapped to function arguments. Try changing
  // registers of `dst`, `src_a`, and `src_b` and see what happens in function's
  // prolog and epilog.
  a64::Gp dst = a64::x0;
  a64::Gp src_a = a64::x1;
  a64::Gp src_b = a64::x2;

  // Decide which vector registers to use. We use these to keep the code generic,
  // you can switch to any other registers when needed.
  a64::Vec vec0 = a64::v0;
  a64::Vec vec1 = a64::v1;
  a64::Vec res = a64::v2;

  // Create and initialize `FuncDetail` and `FuncFrame`.
  FuncDetail func;
  func.init(FuncSignatureT<void, int*, const int*, const int*>(CallConv::kIdHost), emitter->environment());

  FuncFrame frame;
  frame.init(func);

  // Make v0 and v1 dirty. VEC group includes v0|v1|v2 registers.
  frame.addDirtyRegs(vec0, vec1);

  FuncArgsAssignment args(&func);         // Create arguments assignment context.
  args.assignAll(dst, src_a, src_b);      // Assign our registers to arguments.
  args.updateFuncFrame(frame);            // Reflect our args in FuncFrame.
  frame.finalize();

  // Emit prolog and allocate arguments to registers.
  emitter->emitProlog(frame);
  emitter->emitArgsAssignment(frame, args);

  emitter->ldr(vec0.b16(), a64::ptr(src_a)); // Load 4 ints from [src_a] to v0.
  emitter->ldr(vec1.b16(), a64::ptr(src_b)); // Load 4 ints from [src_b] to v1.

  emitter->add(res.b16(), vec0.b16(), vec1.b16());       // Add 4 ints in v0 to 4 ints in v1 and store to v2.
  emitter->str(res.b16(), a64::ptr(dst));   // Store the result from v2 to [dst].

  // Emit epilog and return.
  emitter->emitEpilog(frame);
}

#ifndef ASMJIT_NO_COMPILER
// This function works with a64::Compiler, provided for comparison.
static void makeCompiledFunc(a64::Compiler* cc) noexcept {
  a64::Gp dst   = cc->newIntPtr();
  a64::Gp src_a = cc->newIntPtr();
  a64::Gp src_b = cc->newIntPtr();

  a64::Vec vec0 = cc->newVecQ();
  a64::Vec vec1 = cc->newVecQ();
  a64::Vec res = cc->newVecQ();

  cc->addFunc(FuncSignatureT<void, int*, const int*, const int*>(CallConv::kIdHost));
  cc->setArg(0, dst);
  cc->setArg(1, src_a);
  cc->setArg(2, src_b);

  cc->ldr(vec0.b16(), a64::ptr(src_a));
  cc->ld1(vec1.b16(), a64::ptr(src_b));
  cc->add(res.b16(), vec0.b16(), vec1.b16());
  cc->str(res.b16(), a64::ptr(dst));
  cc->endFunc();
}
#endif

static uint32_t testFunc(JitRuntime& rt, uint32_t emitterType) noexcept {
#ifndef ASMJIT_NO_LOGGING
  FileLogger logger(stdout);
  logger.setIndentation(FormatOptions::kIndentationCode, 2);
#endif

  CodeHolder code;
  code.init(rt.environment());

#ifndef ASMJIT_NO_LOGGING
  code.setLogger(&logger);
#endif

  Error err = kErrorOk;
  switch (emitterType) {
    case BaseEmitter::kTypeAssembler: {
      printf("Using a64::Assembler:\n");
      a64::Assembler a(&code);
      makeRawFunc(a.as<a64::Emitter>());
      break;
    }

#ifndef ASMJIT_NO_BUILDER
    case BaseEmitter::kTypeBuilder: {
      printf("Using a64::Builder:\n");
      a64::Builder cb(&code);
      makeRawFunc(cb.as<a64::Emitter>());

      err = cb.finalize();
      if (err) {
        printf("a64::Builder::finalize() failed: %s\n", DebugUtils::errorAsString(err));
        return 1;
      }
      break;
    }
#endif

#ifndef ASMJIT_NO_COMPILER
    case BaseEmitter::kTypeCompiler: {
      printf("Using a64::Compiler:\n");
      a64::Compiler cc(&code);
      makeCompiledFunc(&cc);

      err = cc.finalize();
      if (err) {
        printf("a64::Compiler::finalize() failed: %s\n", DebugUtils::errorAsString(err));
        return 1;
      }
      break;
    }
#endif
  }

  // Add the code generated to the runtime.
  SumIntsFunc fn;
  err = rt.add(&fn, &code);

  if (err) {
    printf("JitRuntime::add() failed: %s\n", DebugUtils::errorAsString(err));
    return 1;
  }

  // Execute the generated function.
  int inA[4] = { 4, 3, 2, 1 };
  int inB[4] = { 1, 5, 2, 8 };
  int out[4];
  fn(out, inA, inB);

  // Should print {5 8 4 9}.
  printf("Result = { %d %d %d %d }\n\n", out[0], out[1], out[2], out[3]);

  rt.release(fn);
  return !(out[0] == 5 && out[1] == 8 && out[2] == 4 && out[3] == 9);
}

int main() {
  printf("AsmJit A64 Emitter Test\n\n");

  JitRuntime rt;
  unsigned nFailed = 0;

  nFailed += testFunc(rt, BaseEmitter::kTypeAssembler);

#ifndef ASMJIT_NO_BUILDER
  nFailed += testFunc(rt, BaseEmitter::kTypeBuilder);
#endif

#ifndef ASMJIT_NO_COMPILER
  nFailed += testFunc(rt, BaseEmitter::kTypeCompiler);
#endif

  if (!nFailed)
    printf("Success:\n  All tests passed\n");
  else
    printf("Failure:\n  %u %s failed\n", nFailed, nFailed == 1 ? "test" : "tests");

  return nFailed ? 1 : 0;
}
#else
int main() {
  printf("AsmJit A64 Emitter Test is disabled on non-a64 host\n\n");
  return 0;
}
#endif
