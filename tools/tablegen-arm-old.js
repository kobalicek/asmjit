// [AsmJit]
// Machine Code Generation for C++.
//
// [License]
// ZLIB - See LICENSE.md file in the package.

// ============================================================================
// tablegen-arm.js
// ============================================================================

"use strict";

const core = require("./tablegen.js");
const hasOwn = Object.prototype.hasOwnProperty;

const asmdb = core.asmdb;
const kIndent = core.kIndent;
const IndexedArray = core.IndexedArray;
const StringUtils = core.StringUtils;

// ============================================================================
// [ArmDB]
// ============================================================================

// Create ARM ISA.
const isa = new asmdb.arm.ISA();

/*
console.log(
  isa.query({
    filter: function(inst) {
      return !!inst.extensions.ASIMD;
    }
  })
);
//console.log(JSON.stringify(isa.instructionMap, null, 2));
*/

function makeReverseMap(input) {
  const output = Object.create(null);
  for (var value in input) {
    const keys = input[value];
    for (var i = 0; i < keys.length; i++)
      output[keys[i]] = value;
  }
  return output;
}

// ============================================================================
// [tablegen.arm.InstructionToCategoryMap]
// ============================================================================

// Maps each instruction to its category that is then used by ARM assembler.
const InstructionToCategoryMap = makeReverseMap({
  "Adr": ["adr"],
  "ArithBase": ["adc", "adcs", "and", "ands", "bic", "bics", "eor", "eors", "orr", "orrs", "rsb", "rsbs", "rsc", "rscs", "sbc", "sbcs"],
  "ArithBitOps": ["clz", "rbit", "rev", "rev16", "revsh", "rrx", "rrxs"],
  "ArithAdd": ["add", "adds"],
  "ArithRot": ["asr", "asrs", "lsl", "lsls", "lsr", "lsrs", "ror", "rors"],
  "ArithSub": ["sub", "subs"],
  "B": ["b", "bl"],
  "CmpTest": ["cmn", "cmp", "teq", "tst"],
  "MovMvn": ["mov", "movs", "mvn", "mvns"],
  "MovtMovW": ["movt", "movw"],
  "PopPush": ["pop", "push"],

  "Unknown": [
    "crc32b", "crc32cb", "crc32ch", "crc32cw", "crc32h", "crc32w",
    "mul", "muls",
    "qadd", "qadd8", "qadd16", "qasx", "qdadd", "qdsub", "qsax", "qsub", "qsub16", "qsub8",
    "sadd16", "sadd8", "sasx", "sdiv", "sel", "shadd16", "shadd8", "shasx", "shsax", "shsub16", "shsub8",
    "smmul", "smmulr", "smuad", "smuadx", "smusd", "smusdx", "ssax",
    "ssub16", "ssub8",
    "uadd16", "uadd8", "uasx",
    "udiv",
    "uhadd16", "uhadd8", "uhasx", "uhsax", "uhsub16", "uhsub8", "uqadd16", "uqadd8", "uqasx", "uqsax", "uqsub16", "uqsub8", "usad8", "usax",
    "usub16", "usub8"
  ]
});

// ============================================================================
// [tablegen.arm.GenUtils]
// ============================================================================

class GenUtils {
  // Get a list of instructions based on `name` and optional `mode`.
  static query(name, mode) {
    const insts = isa.query(name);
    return !mode ? insts : insts.filter(function(inst) { return inst.arch === mode; });
  }

  static archOf(records) {
    var t16Arch = false;
    var t32Arch = false;
    var a32Arch = false;
    var a64Arch = false;

    for (var i = 0; i < records.length; i++) {
      const record = records[i];
      if (record.encoding === "T16") t16Arch = true;
      if (record.encoding === "T32") t32Arch = true;
      if (record.encoding === "A32") a32Arch = true;
      if (record.encoding === "A64") a64Arch = true;
    }

    var s = (t16Arch && !t32Arch) ? "T16" :
            (t32Arch && !t16Arch) ? "T32" :
            (t16Arch &&  t32Arch) ? "Txx" : "---";
    s += " ";
    s += (a32Arch) ? "A32" : "---";
    s += " ";
    s += (a64Arch) ? "A64" : "---";

    return `[${s}]`;
  }

  static featuresOf(records) {
    const exts = Object.create(null);
    for (var i = 0; i < records.length; i++) {
      const record = records[i];
      for (var k in record.extensions)
        exts[k] = true;
    }
    const arr =  Object.keys(exts);
    arr.sort();
    return arr;
  }
}

// ============================================================================
// [tablegen.arm.ArmTableGen]
// ============================================================================

class ArmTableGen extends core.TableGen {
  constructor() {
    super("Arm");
  }

  // --------------------------------------------------------------------------
  // [Parse / Merge]
  // --------------------------------------------------------------------------

  parse() {
    this.addInst({
      id: 0,
      name: "",
      enum: "None",
      records: [],
      encoding: "Unknown",

      familyType: "kFamilyNone",
      familyIndex: 0,

      nameIndex: -1,
      commonIndex: -1
    });

    var names = isa.instructionNames;
    for (var i = 0; i < names.length; i++) {
      const name = names[i];

      const enum_ = StringUtils.upFirst(name);
      const records = GenUtils.query(names[i]);

      const encoding = InstructionToCategoryMap[name] || "0";

      this.addInst({
        id            : 0,             // Instruction id (numeric value).
        name          : name,          // Instruction name.
        enum          : enum_,         // Instruction enum without `kId` prefix.
        records       : records,       // Instruction records (asmdb query).
        encoding      : encoding,

        familyType    : "kFamilyNone", // Family type.
        familyIndex   : 0,             // Index to a family-specific data.

        nameIndex     : -1,            // Instruction name-index.
        commonIndex   : -1
      });
    }

    console.log("Number of Instructions: " + this.insts.length);
  }

  merge() {
    var s = StringUtils.format(this.insts, "", true, function(inst) {
      return "INST(" +
        String(inst.enum            ).padEnd(17) + ", " +
        String(inst.encoding        ).padEnd(19) + ", " +
        String(0                    ).padEnd( 3) + ", " +
        String(inst.nameIndex       ).padEnd( 5) + ")";
    }) + "\n";
    return this.inject("InstInfo", s, this.insts.length * 4);
  }

  // --------------------------------------------------------------------------
  // [Hooks]
  // --------------------------------------------------------------------------

  onBeforeRun() {
    this.load([
      "src/asmjit/arm/armemitter.h",
      "src/asmjit/arm/armglobals.h",
      "src/asmjit/arm/arminstdb.cpp",
      "src/asmjit/arm/arminstdb.h",
      "src/asmjit/arm/arminstdb_p.h"
    ]);
    this.parse();
  }

  onAfterRun() {
    this.merge();
    this.save();
    this.dumpTableSizes();
  }
}

// ============================================================================
// [tablegen.arm.IdEnum]
// ============================================================================

class IdEnum extends core.IdEnum {
  constructor() {
    super("IdEnum");
  }

  comment(inst) {
    var dbInsts = inst.dbInsts;
    if (!dbInsts) return "";

    var text = GenUtils.archOf(dbInsts);
    var features = GenUtils.featuresOf(dbInsts);

    if (features.length)
      text += " {" + features.join("|") + "}";
    return text;
  }
}

// ============================================================================
// [tablegen.arm.NameTable]
// ============================================================================

class NameTable extends core.NameTable {
  constructor() {
    super("NameTable");
  }
}

// ============================================================================
// [tablegen.arm.TestTable]
// ============================================================================

function simplifyPattern(pattern) {
  return pattern.replace(/\?\|\?/g, "??").replace(/\?\|\?/g, "??");
}

function hashFromMachineInst(mi) {
  //return "{" + simplifyPattern(mi.opcodePattern) + "} " + mi.operands.map((op) => {
  //  return op.def.replace(/ /g, "").replace(/\!=PC\b/g, "").replace(/\b(R|V|D|S)(d|n|n2|n3|n4|m|s|x|x2)\b/g, "$1");
  //}).join(",");

  return "{" + simplifyPattern(mi.opcodePattern) + "} " + mi.operands.map((op) => {
    return op.def.replace(/ /g, "").replace(/\!=PC\b/g, "");
  }).join(",");
}

function compareMachineInstName(a, b) {
  return (a.name < b.name) ? -1 : (a.name > b.name) ? 1 : 0;
}

function isArrayEq(a, b) {
  if (a.length !== b.length)
    return false;
  for (var i = 0; i < a.length; i++)
    if (a[i] != b[i])
      return false;
  return true;
}

class TestTable extends core.Task {
  constructor() {
    super("TestTable");
  }

  run() {
    const insts = this.ctx.insts;
    const map = {};

    insts.forEach((inst) => {
      var arr = [];
      inst.records.forEach((mi) => {
        if (mi.arch == "A32") {
          const hash = hashFromMachineInst(mi);
          arr.push(hash);
        }
      });
      arr.sort(compareMachineInstName);
      // nameToStr[inst.name] = arr;
      var sign = arr.join("~");
      if (!hasOwn.call(map, sign))
        map[sign] = [];
      map[sign].push(inst);

      //if (!hasOwn.call(nameToStr, mi.name))
      //  map[mi.name] = [];
      //map[mi.name].push(h);
      // console.log(h);
    });

    console.log(map);

    for (var k in map) {
      console.log("[" + (map[k].map((inst) => { return JSON.stringify(inst.name); })).join(", ") + "]");
      console.log("  " + k.split("~").join("\n  "));
    }
    console.log("Categories: " + Object.keys(map).length);
    console.log("-----");

    console.log("Unhandled:");
    insts.forEach((inst) => {
      if (inst.name && !(inst.name in InstructionToCategoryMap))
        console.log("  " + inst.name);
    })

    //for (var k in map)
      //map[k].sort(compareMachineInstName);

  }
}

// ============================================================================
// [tablegen.arm.EncodingIdTask]
// ============================================================================

class EncodingIdTask extends core.Task {
  constructor() {
    super("EncodingIdTask");
  }

  run() {
    const categoriesMap = Object.create(null);
    for (var k in InstructionToCategoryMap)
      categoriesMap[InstructionToCategoryMap[k]] = true;

    const categories = Object.keys(categoriesMap);
    categories.sort();
    categories.splice(0, 0, "None");

    var s = "";
    for (var i = 0; i < categories.length; i++) {
      if (s) s += `,\n`;
      s += `  k${categories[i]}` + (s ? "" : " = 0");
    };

    s = `enum EncodingId : uint32_t {\n${s}\n};\n`;
    return this.ctx.inject("EncodingId", StringUtils.disclaimer(s), 0);
  }
}

// ============================================================================
// [tablegen.arm.CommonTable]
// ============================================================================

class CommonTable extends core.Task {
  constructor() {
    super("CommonTable", [
      "IdEnum",
      "NameTable"
    ]);
  }

  run() {
    //const table = new IndexedArray();

    //for (var i = 0; i < insts.length; i++) {
    //  const inst = insts[i];
    //  const item = "{ " + "0" + "}";
    //  inst.commonIndex = table.addIndexed(item);
    //}

    // return this.ctx.inject("InstInfo", StringUtils.disclaimer(s), 0);
    return 0;
  }
}

// ============================================================================
// [tablegen.arm.EmitCodeTask]
// ============================================================================

function instructionNameToCxxName(name) {
  if (/^(and|and_eq|bitand|bitor|or|or_eq|not|not_eq|xor|xor_eq|if|int)$/.test(name))
    return name + "_";
  else
    return name;
}

class EmitCodeTask extends core.Task {
  constructor() {
    super("EmitCodeTask", [
      "IdEnum"
    ]);
  }

  run() {
    const insts = this.ctx.insts;

    const signatures = Object.create(null);
    var s = "";

    function addSignature(name, enumId, opTypes) {
      var prefix = `ASMJIT_INST_${opTypes.length}`;
      if (opTypes.length > 0 && opTypes[opTypes.length - 1] === "Imm")
        prefix += "i";
      else
        prefix += "x";

      var signature =
        prefix + "(" +
          instructionNameToCxxName(name) + ", " +
          enumId +
          (opTypes.length ? ", " + opTypes.join(", ") : "") +
        ")";

      if (!(signature in signatures)) {
        signatures[signature] = true;
        s += signature + "\n";
      }
    }

    for (var i = 0; i < insts.length; i++) {
      const ai = insts[i];
      for (var j = 0; j < ai.records.length; j++) {
        const mi = ai.records[j];
        if (mi.arch === "A32") {
          var ignore = false;
          var opTypes = [];

          for (var k = 0; k < mi.operands.length; k++) {
            const operand = mi.operands[k];
            var opType = "";
            var isRel = false;

            if (operand.optional && !ignore)
              addSignature(ai.name, ai.enum, opTypes);

            if (operand.isReg()) {
              if (operand.reg.startsWith("R"))
                opType = "Gp";
              else if (operand.reg.startsWith("V"))
                opType = "Vec";
              // console.log(operand)
            }
            else if (operand.isImm()) {
              if (operand.name.startsWith("Rel"))
                isRel = true;
              opType = "Imm";
            }

            if (!opType)
              ignore = true;
            else
              opTypes.push(opType);
          }

          if (!ignore) {
            addSignature(ai.name, ai.enum, opTypes);
          }

          // console.log(mi);
        }
      };
    }

    //var s = `const InstDB::CommonInfo InstDB::commonData[] = {\n${StringUtils.format(table, kIndent, true)}\n};\n`;
    return this.ctx.inject("EmitCode", StringUtils.disclaimer(s), 0);
  }
}

// ============================================================================
// [Main]
// ============================================================================

new ArmTableGen()
  .addTask(new IdEnum())
  .addTask(new NameTable())
  .addTask(new EncodingIdTask())
  .addTask(new TestTable())
  .addTask(new CommonTable())
  .addTask(new EmitCodeTask())
  .run();
