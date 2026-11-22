//
// Created by Dimitar Anastassov on 21.11.26.
//

#ifndef BOMI_EMIT_H
#define BOMI_EMIT_H

#include <stdio.h>
#include "backend/mir.h"

/* Prints the MIR as an ARM64 .s file for macOS (clang out.s -o prog).
 * Runs after frame_layout: there must be no [slot N] left. */
void m_print(const MProgram* m, FILE* out);

#endif //BOMI_EMIT_H
