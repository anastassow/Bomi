//
// Created by Dimitar Anastassov on 22.11.26.
//
// Runs both checks on bomi's output in one command:
//
//   1. scripts/check_ir.sh   bomi -> <name>.ll -> opt: is it valid LLVM IR?
//   2. scripts/run.sh        bomi -> <name>.s -> clang -c: is it valid ARM64 assembly?
//                            then link, run it and print the answer
//
//   bomi_tests                 checks tests/input.txt
//   bomi_tests a.c b.c ...     checks the given files
//
// In CLion: pick bomi_tests and press Run (bomi is built first).
// Exit code: 0 if every check passed, 1 otherwise.
//
// BOMI_REPO and BOMI_BUILD_DIR are set by CMakeLists.txt.
//

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/wait.h>

#ifndef BOMI_REPO
#error "BOMI_REPO must be defined (path to the bomi repo), see CMakeLists.txt"
#endif

#define CHECK_IR BOMI_REPO "/scripts/check_ir.sh"
#define RUN      BOMI_REPO "/scripts/run.sh"
#define DEFAULT  BOMI_REPO "/tests/input.txt"

/* bash <script> <files...>; returns 1 if it ran and exited with 0 */
static int run_script(const char* script, char* const files[], int nfiles) {
    char* argv[nfiles + 3];
    argv[0] = "bash";
    argv[1] = (char*) script;
    for (int i = 0; i < nfiles; i++) argv[i + 2] = files[i];
    argv[nfiles + 2] = NULL;

    fflush(stdout);     /* our output first, then the script's */

    pid_t pid = fork();
    if (pid < 0) {
        perror("fork");
        return 0;
    }
    if (pid == 0) {
        execvp("bash", argv);
        perror("bash");
        _exit(127);
    }

    int status;
    if (waitpid(pid, &status, 0) < 0) {
        perror("waitpid");
        return 0;
    }
    return WIFEXITED(status) && WEXITSTATUS(status) == 0;
}

int main(int argc, char** argv) {
#ifdef BOMI_BUILD_DIR
    /* the scripts build bomi in this dir (an existing BOMI_BUILD_DIR wins) */
    setenv("BOMI_BUILD_DIR", BOMI_BUILD_DIR, 0);
#endif

    char* default_file[] = { DEFAULT };
    char** files = argc > 1 ? argv + 1 : default_file;
    int nfiles   = argc > 1 ? argc - 1 : 1;

    printf("== LLVM IR ==================================================\n");
    int ir_ok = run_script(CHECK_IR, files, nfiles);

    /* run.sh takes one file at a time */
    printf("\n== ARM64: assemble and run =================================\n");
    int asm_ok = 1;
    for (int i = 0; i < nfiles; i++) {
        if (!run_script(RUN, &files[i], 1)) asm_ok = 0;
    }

    printf("\n== summary =================================================\n");
    printf("%s  LLVM IR\n",                  ir_ok  ? "OK  " : "FAIL");
    printf("%s  ARM64 assembly and run\n",   asm_ok ? "OK  " : "FAIL");

    return ir_ok && asm_ok ? 0 : 1;
}
