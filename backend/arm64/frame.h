//
// Created by Dimitar Anastassov on 21.11.26.
//

#ifndef BOMI_FRAME_H
#define BOMI_FRAME_H

#include "backend/mir.h"

/* Gives every function its stack frame: sets frame_size, turns [slot N] into [sp, #4*N],
 * adds the prologue at the start and the epilogue before each ret. Runs after select. */
void frame_layout(MProgram* m);

#endif //BOMI_FRAME_H
