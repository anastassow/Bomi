/*
 * unary_test_input.c: every unary operator bomi compiles:  - + ~ ++ --
 * (! & * are not supported yet: see docs/partly_implemented).
 *
 * The comment after a line shows how it groups.
 * Only block comments, declarations first, one variable per declaration.
 */


/* ---- 1. each operator once ------------------------------------------------- */
int each_operator() {
    int x = 1;

    -x;
    +x;
    ~x;
    ++x;
    --x;
    x++;
    x--;
    return 0;
}


/* ---- 2. on constants and brackets ------------------------------------------ */
int operands() {
    int x = 1;

    -5;
    +5;
    ~5;
    -(x + 1);           /* the operand is the whole + tree */
    ~(x * 2);
    -(x);               /* brackets make no node */
    return 0;
}


/* ---- 3. unary inside unary ------------------------------------------------- */
int nesting() {
    int x = 1;

    - -x;               /* -(-x): two tokens, not a decrement */
    --x;                /* one token: a decrement */
    - - -x;             /* -(-(-x)) */
    -~x;                /* -(~x) */
    ~-x;                /* ~(-x) */
    -+x;                /* -(+x) */
    +-x;                /* +(-x) */
    return 0;
}


/* ---- 4. prefix and postfix together ---------------------------------------- */
int prefix_postfix() {
    int x = 1;

    -x++;               /* -(x++): postfix binds tighter */
    -x--;               /* -(x--) */
    ~x++;               /* ~(x++) */
    -++x;               /* -(++x) */
    - --x;              /* -(--x) */
    return 0;
}


/* ---- 5. next to binary operators ------------------------------------------- */
int with_binary() {
    int a = 1;
    int b = 2;
    int x = 3;

    -a + b;             /* (-a) + b */
    -(a + b);           /* -(a + b) */
    a - -b;             /* a - (-b) */
    a * -b;             /* a * (-b) */
    -a / -b;            /* (-a) / (-b) */
    -x % 3;             /* (-x) % 3 */
    ~a & b;             /* (~a) & b */
    a ^ ~b;             /* a ^ (~b) */
    a+++b;              /* (a++) + b: the lexer takes the longest token */
    a---b;              /* (a--) - b */
    a++ * b--;          /* (a++) * (b--) */
    -a + -b * ~x;       /* (-a) + ((-b) * (~x)) */
    return 0;
}


/* ---- 6. in every place an expression can be -------------------------------- */
int places() {
    int x = 1;
    int y = -x;         /* in a declaration */
    int z = ++x;

    y = ~x;             /* right of = */
    y += -x;            /* right of += */
    y = -(x = 3);       /* the operand is an assignment */
    z = y = x++;        /* in a chain */
    return -y;          /* in return */
}


int main() {
    int x = 5;

    return -x;          /* answer: 251 (-5 in 8 bits) */
}


/* ---- errors ---------------------------------------------------------------------
 *
 * ++ and -- need a variable:  "Expression is not assignable: it must be a variable"
 *   ++5   5++   --(a + x)   (a + x)++   ++-x   ++++x   x++++
 *
 * Stop the parser:
 *   sizeof x, sizeof(int)   sizeof not supported yet
 *   (int)x                  casts not supported yet
 *   ++(int)x                syntax error (++ never takes a cast, in C too)
 */
