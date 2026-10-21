//
// Created by Dimitar Anastassov on 30.09.26.
//

#ifndef CC_CODEGEN_H
#define CC_CODEGEN_H

#include <stdio.h>
#include "ir.h"

/* IR → x86-64 assembly (AT&T syntax). Naive: every vreg lives in its own stack slot. */
void codegen_x86(const IrProgram *p, FILE *out);

void codegen_arm(const IrProgram *P, FILE *out);

#endif //CC_CODEGEN_H
