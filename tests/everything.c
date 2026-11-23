/*
 * everything.c: one example of everything bomi compiles fully, from C to
 * running ARM64.
 *
 *   scripts/run.sh tests/everything.c       ->  answer: 0
 *
 * main checks every example against the value C gives it:
 *     bad |= (got) ^ expected;
 * a ^ b is 0 only when a == b, so bad stays 0 while every example is right.
 * The answer is 0 if everything works, anything else if an example is wrong.
 * (bomi has no if or == yet, so this is how main checks.)
 *
 * Rules this file has to follow, because bomi does not support more yet:
 *   - only block comments: a line comment is a syntax error
 *   - all declarations first in a block, then the statements (C89)
 *   - one variable per declaration:  int x, y;  is an error
 *   - no unary minus: -5 is written 0 - 5
 */


/* ---- functions ---------------------------------------------------------
 * Several int functions in one file. bomi cannot call functions yet, so
 * these are compiled (see everything.s) but only main runs.
 */

/* return with an expression; x is local, it is not main's x */
int square() {
    int x = 3;
    return x * x;
}

/* falling off the end returns 0 */
int falls_off() {
    int x = 5;
    x = x * 2;
}

/* everything after the first return is skipped: returns 1 */
int returns_early() {
    return 1;
    return 2;
}

/* an empty function returns 0 */
int empty() {
}


int main()
{
    /* ---- declarations: all of them before the first statement ---------- */
    int bad = 0;            /* int x = constant */
    int a = 20;
    int b = 3;
    int neg = 0 - 7;        /* int x = expression */
    int x;                  /* int x;  no value yet */
    int y;
    int t;

    /* ---- integer constants --------------------------------------------- */
    x = 42;           bad |= x ^ 42;               /* decimal */
    x = 0x1F;         bad |= x ^ 31;               /* hex */
    x = 7u + 5L;      bad |= x ^ 12;               /* u and L suffixes */
    x = 70000;        bad |= (x - 69999) ^ 1;      /* over 65535: movz + movk */
    x = 2147483647;   bad |= x ^ 0x7FFFFFFF;       /* the biggest int */
    x = 0 - 5;        bad |= (x + 5) ^ 0;          /* a negative number */

    /* ---- reading variables ---------------------------------------------- */
    bad |= a ^ 20;
    bad |= (a + b) ^ 23;                           /* two variables */
    bad |= (a + b * neg) ^ (0 - 1);                /* three: 20 + 3 * -7 */

    /* ---- + - * ----------------------------------------------------------- */
    bad |= (a + b) ^ 23;
    bad |= (a - b) ^ 17;
    bad |= (b - a) ^ (0 - 17);                     /* negative result */
    bad |= (a * b) ^ 60;
    bad |= (neg * b) ^ (0 - 21);                   /* negative times positive */

    /* ---- / and %: rounded toward zero, like C ---------------------------- */
    bad |= (a / b) ^ 6;
    bad |= (a % b) ^ 2;
    bad |= (neg / 2) ^ (0 - 3);                    /* -7 / 2 = -3, not -4 */
    bad |= (neg % 2) ^ (0 - 1);                    /* -7 % 2 = -1 */

    /* ---- & | ^ ----------------------------------------------------------- */
    bad |= (12 & 10) ^ 8;
    bad |= (12 | 3) ^ 15;
    bad |= (12 ^ 10) ^ 6;

    /* ---- precedence and brackets ----------------------------------------- */
    bad |= (1 + 2 * 3) ^ 7;                        /* * before + */
    bad |= (1 + 2 * 3 - 8 / 4 % 3) ^ 5;            /* 1 + 6 - (2 % 3) */
    bad |= (1 | 6 ^ 3 & 5) ^ 7;                    /* & then ^ then |: 1 | (6 ^ 1) */
    bad |= ((1 + 2) * (10 - 4)) ^ 18;              /* brackets first */
    bad |= (a - b - 2) ^ 15;                       /* left to right: (20 - 3) - 2 */
    bad |= (a / b / 2) ^ 3;                        /* left to right: (20 / 3) / 2 */

    /* ---- = --------------------------------------------------------------- */
    x = 9;            bad |= x ^ 9;
    x = a + b;        bad |= x ^ 23;
    x = x * 2;        bad |= x ^ 46;               /* x on both sides */

    /* ---- the 10 compound assignments ------------------------------------ */
    x = 5;            x += 3;     bad |= x ^ 8;
    x = 5;            x -= 3;     bad |= x ^ 2;
    x = 5;            x *= 3;     bad |= x ^ 15;
    x = 50;           x /= 3;     bad |= x ^ 16;
    x = 50;           x %= 7;     bad |= x ^ 1;
    x = 5;            x <<= 3;    bad |= x ^ 40;
    x = 0 - 64;       x >>= 2;    bad |= x ^ (0 - 16);   /* keeps the sign */
    x = 14;           x &= 7;     bad |= x ^ 6;
    x = 8;            x |= 3;     bad |= x ^ 11;
    x = 15;           x ^= 5;     bad |= x ^ 10;
    x = 2;            x += a * b; bad |= x ^ 62;          /* an expression on the right */

    /* ---- an assignment is also a value ----------------------------------- */
    y = (x = 5) + 1;  bad |= (x * 10 + y) ^ 56;    /* x = 5, y = 6 */
    x = y = 7;        bad |= (x + y) ^ 14;         /* chained */

    /* ---- expression statements and the empty statement ------------------- */
    x = 4;
    x + 1;                                         /* worked out, then dropped */
    3 * 3;
    ;
    bad |= x ^ 4;                                  /* x is still 4 */

    /* ---- the answer -------------------------------------------------------
     * Only the low 8 bits of main's answer survive, so first fold all 32 bits
     * of bad into them: a wrong bit anywhere still gives a non-zero answer.
     */
    t = bad;          t >>= 16;   bad |= t;
    t = bad;          t >>= 8;    bad |= t;
    return bad & 255;
}


/* ---- errors -----------------------------------------------------------------
 * Each of these stops bomi with exit code 1. To see one, put it into main
 * (a declaration goes before the first statement).
 *
 *   int x = 1;  int x = 2;      Redeclaration of 'x'
 *   return y;                   Use of undeclared variable 'y'
 *   z = 3;                      Assignment to undeclared variable 'z'
 *   return 3000000000;          Integer constant 3000000000 does not fit in an int
 *   5 = 3;                      lvalue required as left operand of assignment
 *   return 1 }                  syntax error
 *   x = x + 1;  about 520 times frame_layout: function 'main' needs ... bytes of
 *                               stack, at most 4095 are supported
 */
