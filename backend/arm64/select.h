//
// Created by Dimitar Anastassov on 21.11.26.
//

#ifndef BOMI_SELECT_H
#define BOMI_SELECT_H

#include "ir/ir.h"
#include "backend/mir.h"

/* IR -> MIR. Every IR vreg %vN lives in stack slot N,
 * so the result still has [slot N] operands and no prologue / epilogue. */
MProgram* select_program(const IrProgram* p);

#endif //BOMI_SELECT_H
