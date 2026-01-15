/* silence unused-variable warnings for GCC/Clang when needed */
#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic ignored "-Wunused-variable"
#endif

/* WARNING: Do not include any other libraries here,
 * otherwise you will get an error while running test.py
 * You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.
 *
 * Using printf will interfere with our script capturing the execution results.
 * At this point, you can only test correctness with ./btest.
 * After confirming everything is correct in ./btest, remove the printf
 * and run the complete tests with test.py.
 */

 /*
 * bitAnd - x & y using only ~ and |
 * Example: bitAnd(4, 5) = 4
 * Legal ops: ~ |
 * Max ops: 7
 * Difficulty: 1
 */
int bitAnd(int x, int y) {
    return ~(~x | ~y);
}

/*
 * bitXor - x ^ y using only ~ and &
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 7
 *   Difficulty: 1
 */
int bitXor(int x, int y) {
    return (~(~x & ~y) & ~(x & y));
}

/*
 * samesign - Determines if two integers have the same sign.
 *   0 is not positive, nor negative
 *   Example: samesign(0, 1) = 0, samesign(0, 0) = 1
 *            samesign(-4, -5) = 1, samesign(-4, 5) = 0
 *   Legal ops: >> << ! ^ && if else &
 *   Max ops: 12
 *   Difficulty: 2
 *
 * Parameters:
 *   x - The first integer.
 *   y - The second integer.
 *
 * Returns:
 *   1 if x and y have the same sign , 0 otherwise.
 */
int samesign(int x, int y) {
    if(x&&y){
        x=x>>31;
        y=y>>31;         
        return !(x^y);
    }
    else{
        if(x) return 0;
        if(y) return 0;
        return 1;
    };
}

/*
 * logtwo - Calculate the base-2 logarithm of a positive integer using bit
 *   shifting. (Think about bitCount)
 *   Note: You may assume that v > 0
 *   Example: logtwo(32) = 5
 *   Legal ops: > < >> << |
 *   Max ops: 25
 *   Difficulty: 4
 */
int logtwo(int v) {
    int r = 0;
    int s;
    s = (v >> (r | 16)) > 0; r = r | (s << 4);
    s = (v >> (r | 8 )) > 0; r = r | (s << 3);
    s = (v >> (r | 4 )) > 0; r = r | (s << 2);
    s = (v >> (r | 2 )) > 0; r = r | (s << 1);
    s = (v >> (r | 1 )) > 0; r = r |  s;
    return r;
}

/*
 *  byteSwap - swaps the nth byte and the mth byte
 *    Examples: byteSwap(0x12345678, 1, 3) = 0x56341278
 *              byteSwap(0xDEADBEEF, 0, 2) = 0xDEEFBEAD
 *    Note: You may assume that 0 <= n <= 3, 0 <= m <= 3
 *    Legal ops: ! ~ & ^ | + << >>
 *    Max ops: 17
 *    Difficulty: 2
 */
int byteSwap(int x, int n, int m) {
    int move1=(n<<3);
    int move2=(m<<3);
    int byte1=(x>>move1)&(0xFF);
    int byte2=(x>>move2)&(0xFF);
    int xor=(byte1 ^ byte2);
    int change=((xor<<move1)|(xor<<move2));
    return (x^change);
}

/*
 * reverse - Reverse the bit order of a 32-bit unsigned integer.
 *   Example: reverse(0xFFFF0000) = 0x0000FFFF reverse(0x80000000)=0x1 reverse(0xA0000000)=0x5
 *   Note: You may assume that an unsigned integer is 32 bits long.
 *   Legal ops: << | & - + >> for while ! ~ (You can define unsigned in this function)
 *   Max ops: 30
 *   Difficulty: 3
 */
unsigned reverse(unsigned v) {
    v=(v >> 16) | (v << 16);
    v=((v & 0xFF00FF00) >> 8) | ((v & 0x00FF00FF) << 8);
    v=((v & 0xF0F0F0F0) >> 4) | ((v & 0x0F0F0F0F) << 4);
    v=((v & 0xCCCCCCCC) >> 2) | ((v & 0x33333333) << 2);
    v=((v & 0xAAAAAAAA) >> 1) | ((v & 0x55555555) << 1);
    return v;
}

/*
 * logicalShift - shift x to the right by n, using a logical shift
 *   Examples: logicalShift(0x87654321,4) = 0x08765432
 *   Note: You can assume that 0 <= n <= 31
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 20
 *   Difficulty: 3
 */
int logicalShift(int x, int n) {
    int u=0x7FFFFFFF;
    u=((u >> n) << 1) | 1;
    x=(x >> n) & u;
    return x;
}

/*
 * leftBitCount - returns count of number of consective 1's in left-hand (most) end of word.
 *   Examples: leftBitCount(-1) = 32, leftBitCount(0xFFF0F0F0) = 12,
 *             leftBitCount(0xFE00FF0F) = 7
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 50
 *   Difficulty: 4
 */
int leftBitCount(int x) {
    int y = ~x; 
    int n = 0;
    int s;
    s = !(y >> 16) << 4; n = n + s; y = y << s;
    s = !(y >> 24) << 3; n = n + s; y = y << s;
    s = !(y >> 28) << 2; n = n + s; y = y << s;
    s = !(y >> 30) << 1; n = n + s; y = y << s;
    s = !(y >> 31);      n = n + s;
    return n + !y;    /* x==-1 时应返回32；此时 y 一直为0，上面得到31，再补1 */
}

/*
 * float_i2f - Return bit-level equivalent of expression (float) x
 *   Result is returned as unsigned int, but it is to be interpreted as
 *   the bit-level representation of a single-precision floating point values.
 *   Legal ops: if else while for & | ~ + - >> << < > ! ==
 *   Max ops: 30
 *   Difficulty: 4
 */
unsigned float_i2f(int x) {
    if (x == 0) {
        return 0;
    }
    int s = x < 0;
    unsigned us = s << 31;
    unsigned ax = x;
    if (s) {
        ax = (~ax) + 1;
    }

    int p = 0;
    unsigned y = ax;
    while (y >> 1) {
        y = y >> 1;
        p = p + 1;
    }
    unsigned exp  = ((p + 127) << 23);
    unsigned rem = ax - (1 << p);
    unsigned frac;
    if (p <= 23) {
        frac = rem << (23 - p);
    } 
    else {
        unsigned shift = p - 23;
        unsigned frac_base = rem >> shift; // 未舍入的尾数
        unsigned half = 1 << (shift - 1); // 判断与“半”的大小关系
        frac = (rem + (half - 1) + (frac_base & 1)) >> shift;
        if (frac >> 23) { // 处理溢出
            frac = 0; 
            exp = exp + 0x00800000; 
        }
    }
    return us | exp | frac;
}

/*
 * floatScale2 - Return bit-level equivalent of expression 2*f for
 *   floating point argument f.
 *   Both the argument and result are passed as unsigned int's, but
 *   they are to be interpreted as the bit-level representation of
 *   single-precision floating point values.
 *   When argument is NaN, return argument
 *   Legal ops: & >> << | if > < >= <= ! ~ else + ==
 *   Max ops: 30
 *   Difficulty: 4
 */
unsigned floatScale2(unsigned uf) {
    unsigned exp = (uf << 1) >> 24 ;
    unsigned s = uf >> 31 ;
    unsigned frac = (uf << 9) >> 9 ;
    if(exp == 255) { // 特殊值
        return uf;
    }
    if(exp <=254){
        s = s << 31;
        if(exp == 0) { // 非规格化
            frac = frac << 1;
            return s | frac;
        }
        else { // 规格化
            exp = (exp + 1) << 23;
            if(exp == 255){
                return s | exp; // 无穷
            }
            else {
                return (s | exp) | frac;
            }
        }
    }
    return 0;
}

/*
 * float64_f2i - Convert a 64-bit IEEE 754 floating-point number to a 32-bit signed integer.
 *   The conversion rounds towards zero.
 *   Note: Assumes IEEE 754 representation and standard two's complement integer format.
 *   Parameters:
 *     uf1 - The lower 32 bits of the 64-bit floating-point number.
 *     uf2 - The higher 32 bits of the 64-bit floating-point number.
 *   Returns:
 *     The converted integer value, or 0x80000000 on overflow, or 0 on underflow.
 *   Legal ops: >> << | & ~ ! + - > < >= <= if else
 *   Max ops: 60
 *   Difficulty: 3
 */
// 64：s1 exp11 frac52
// 1 11 20     32
int float64_f2i(unsigned uf1, unsigned uf2) {
    unsigned s = uf2 >> 31;
    int exp = (uf2 >> 20) & 0x7FF;              
    unsigned frac_high = uf2 & 0xFFFFF;         
    unsigned frac_low = uf1; 
    int E = exp + (-1023);                   
    if ((exp + 1) >> 11 | (E > 31)) {
        return 0x80000000; //上溢
    }
    if (!exp | (E < 0)) {
        return 0; // 下溢
    }
    unsigned M_hi = frac_high | (1 << 20); // 变为1.xxx
    unsigned M_lo = frac_low;
    int shift = 52 - E;  // [21,52]
    unsigned mag;
    if (shift >= 32) { // 获得整数部分
        mag = M_hi >> (shift - 32);
    } else {
        mag = (M_lo >> shift) | (M_hi << (32 - shift));
    }
    if (!s) { // 正数情况
        if (mag >> 31) {
            return 0x80000000;
        }
        return mag;
    }
    return (~mag) + 1; // 负数情况
}

/*
 * floatPower2 - Return bit-level equivalent of the expression 2.0^x
 *   (2.0 raised to the power x) for any 32-bit integer x.
 *
 *   The unsigned value that is returned should have the identical bit
 *   representation as the single-precision floating-point number 2.0^x.
 *   If the result is too small to be represented as a denorm, return
 *   0. If too large, return +INF.
 *
 *   Legal ops: < > <= >= << >> + - & | ~ ! if else &&
 *   Max ops: 30
 *   Difficulty: 4
 */
unsigned floatPower2(int x) {
    if(x > 127) { // 上溢
        return 0x7F800000;
    }
    if(x < -149) { // 下溢
        return 0;
    }
    if(x > -127) { // 规格化
        unsigned exp = x + 127;
        return exp << 23;
    }
    unsigned shift = -127 - x; // 非规格化
    return 1 << (22 - shift);
}
