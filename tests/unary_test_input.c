/*
 * unary_test_input.c: every unary expression the parser can build.
 *
 * A test for the parser and the AST print. Lowering does not handle
 * NODE_UNARY yet, so bomi writes the tree to compiled/unary_test_input.ast,
 * then stops with "Unsupported expression type".
 *
 * The comment after each line shows how it should group.
 * Rules this file follows, because the parser needs them for now:
 * only block comments, declarations first, one variable per declaration,
 * no sizeof, casts, calls, arrays or comparisons (not supported yet).
 */


/* ---- 1. each operator once, on a variable ------------------------------ */
int each_operator() {
    int x = 1;
    int *p = &x;        /* bomi keeps only "int p" for now, but this is real C */

    -x;                 /* OP_NEG      */
    +x;                 /* OP_POS      */
    ~x;                 /* OP_BNOT     */
    !x;                 /* OP_LNOT     */
    &x;                 /* OP_ADDR     */
    *p;                 /* OP_DEREF    */
    ++x;                /* OP_PRE_INC  */
    --x;                /* OP_PRE_DEC  */
    x++;                /* OP_POST_INC */
    x--;                /* OP_POST_DEC */
    return 0;
}


/* ---- 2. on constants and on brackets ------------------------------------ */
int operands() {
    int x = 1;

    -5;
    +5;
    ~5;
    !0;
    -(x + 1);           /* the operand is the whole + tree */
    ~(x * 2);
    !(x - 1);
    -(x);               /* brackets make no node: operand is var x */
    return 0;
}


/* ---- 3. unary inside unary ----------------------------------------------- */
int nesting() {
    int x = 1;
    int *p = &x;        /* bomi keeps only "int p" for now, but this is real C */

    - -x;               /* -(-x): two tokens, not a decrement */
    --x;                /* one DEC_OP token: a decrement */
    - - -x;             /* -(-(-x)) */
    -~x;                /* -(~x) */
    ~-x;                /* ~(-x) */
    !!x;                /* !(!x) */
    !-x;                /* !(-x) */
    -!x;                /* -(!x) */
    -+x;                /* -(+x) */
    +-x;                /* +(-x) */
    *&x;                /* *(&x) */
    &*p;                /* &(*p) */
    return 0;
}


/* ---- 4. prefix and postfix together -------------------------------------- */
int prefix_postfix() {
    int x = 1;
    int *p = &x;        /* bomi keeps only "int p" for now, but this is real C */

    -x++;               /* -(x++): postfix binds tighter */
    -x--;               /* -(x--) */
    ~x++;               /* ~(x++) */
    !x--;               /* !(x--) */
    -++x;               /* -(++x) */
    - --x;              /* -(--x) */
    ++*p;               /* ++(*p) */
    --*p;               /* --(*p) */
    *p++;               /* *(p++) */
    *p--;               /* *(p--) */
    return 0;
}


/* ---- 5. next to binary operators ------------------------------------------ */
int with_binary() {
    int a = 1;
    int b = 2;
    int x = 3;

    -a + b;             /* (-a) + b */
    -(a + b);           /* -(a + b) */
    a - -b;             /* a - (-b) */
    a * -b;             /* a * (-b) */
    -a * b;             /* (-a) * b */
    -a / -b;            /* (-a) / (-b) */
    -x % 3;             /* (-x) % 3 */
    ~a & b;             /* (~a) & b */
    !a | b;             /* (!a) | b */
    a ^ ~b;             /* a ^ (~b) */
    a + +b;             /* a + (+b) */
    a+++b;              /* (a++) + b: the lexer takes the longest token */
    a---b;              /* (a--) - b */
    a++ * b--;          /* (a++) * (b--) */
    -a + -b * ~x;       /* (-a) + ((-b) * (~x)) */
    return 0;
}


/* ---- 6. in every place an expression can be ------------------------------ */
int places() {
    int x = 1;
    int y = -x;         /* in a declaration */
    int z = ++x;

    y = ~x;             /* right of = */
    y += -x;            /* right of += */
    y = -(x = 3);       /* the operand is an assignment */
    z = y = !x;         /* in a chain */
    return -y;          /* in return */
}


/* ---- 7. parse, but the analysis pass must reject them later --------------- */
int rejected_later() {
    int x = 1;
    int a = 2;

    &(10 + 10);         /* a sum has no address */
    &5;                 /* a number has no address */
    &-x;                /* -x has no address */
    &x++;               /* &(x++): x++ is a value, not a variable */
    ++5;                /* nothing to write back to */
    5++;
    --(a + x);
    (a + x)++;
    ++-x;               /* -x is not a variable */
    ++++x;              /* ++(++x): in C, ++x is not a variable */
    x++++;              /* (x++)++ */
    return 0;
}


int main() {
    int x = 5;

    return -x;
}


/* ---- not in this file --------------------------------------------------------
 *
 * Not supported yet:
 *   sizeof x, sizeof(int)
 *   (int)x          casts
 *
 * These stop the parser:
 *   ++(int)x        syntax error: ++ never takes a cast (in real C too)
 *   &&x             syntax error: the lexer reads && as one token, not & &
 *   -x = 3          lvalue required as left operand of assignment
 *                   (the check that is still in parser.y)
 */
