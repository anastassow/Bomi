//
// Created by Dimitar Anastassov on 30.09.26.
//

#ifndef BOMI_CODEGEN_H
#define BOMI_CODEGEN_H

// MProgram                      the whole output file
//  └── MFunc                    one function            (name, nslots, frame_size)
//       └── MBlock              one basic block         (label)
//            └── MInstr         one ARM instruction     (op, ops[4], nops, prev, next)
//                 └── MOperand  one operand             (kind + union: reg / imm / slot)

#include <stdio.h>
#include "ir/ir.h"

typedef struct MProgram{ } MProgram;

void codegen_arm64(const IrProgram* p, FILE* out);

#endif //BOMI_CODEGEN_H
