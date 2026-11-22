//
// Created by Dimitar Anastassov on 30.09.26.
//

#include "codegen.h"

#include "backend/mir.h"
#include "backend/arm64/select.h"
#include "backend/arm64/frame.h"
#include "backend/arm64/emit.h"

/* select:        IR -> MIR, every vreg in its stack slot
 * frame_layout:  frame size, [slot N] -> [sp, #4*N], prologue and epilogues
 * m_print:       MIR -> .s text */
void codegen_arm64(const IrProgram* p, FILE* out) {
    MProgram* m = select_program(p);

    frame_layout(m);
    m_print(m, out);

    m_program_free(m);
}
