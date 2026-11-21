//
// Created by Dimitar Anastassov on 30.09.26.
//

#ifndef BOMI_CODEGEN_H
#define BOMI_CODEGEN_H

#include <stdio.h>
#include "ir/ir.h"

void codegen_arm64(const IrProgram* p, FILE* out);

#endif //BOMI_CODEGEN_H
