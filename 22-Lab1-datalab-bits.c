/* 
 * CS:APP Data Lab 
 * 
 * Adrian Borup, adbo
 * 
 * bits.c - Source file with your solutions to the Lab.
 *          This is the file you will hand in to your instructor.
 *
 * WARNING: Do not include the <stdio.h> header; it confuses the dlc
 * compiler. You can still use printf for debugging without including
 * <stdio.h>, although you might get a compiler warning. In general,
 * it's not good practice to ignore compiler warnings, but in this
 * case it's OK.  
 */

#if 0
/*
 * Instructions to Students:
 *
 * STEP 1: Read the following instructions carefully.
 */

You will provide your solution to the Data Lab by
editing the collection of functions in this source file.

INTEGER CODING RULES:
 
  Replace the "return" statement in each function with one
  or more lines of C code that implements the function. Your code 
  must conform to the following style:
 
  int Funct(arg1, arg2, ...) {
      /* brief description of how your implementation works */
      int var1 = Expr1;
      ...
      int varM = ExprM;

      varJ = ExprJ;
      ...
      varN = ExprN;
      return ExprR;
  }

  Each "Expr" is an expression using ONLY the following:
  1. Integer constants 0 through 255 (0xFF), inclusive. You are
      not allowed to use big constants such as 0xffffffff.
  2. Function arguments and local variables (no global variables).
  3. Unary integer operations ! ~
  4. Binary integer operations & ^ | + << >>
    
  Some of the problems restrict the set of allowed operators even further.
  Each "Expr" may consist of multiple operators. You are not restricted to
  one operator per line.

  You are expressly forbidden to:
  1. Use any control constructs such as if, do, while, for, switch, etc.
  2. Define or use any macros.
  3. Define any additional functions in this file.
  4. Call any functions.
  5. Use any other operations, such as &&, ||, -, or ?:
  6. Use any form of casting.
  7. Use any data type other than int.  This implies that you
     cannot use arrays, structs, or unions.

 
  You may assume that your machine:
  1. Uses 2s complement, 32-bit representations of integers.
  2. Performs right shifts arithmetically.
  3. Has unpredictable behavior when shifting if the shift amount
     is less than 0 or greater than 31.


EXAMPLES OF ACCEPTABLE CODING STYLE:
  /*
   * pow2plus1 - returns 2^x + 1, where 0 <= x <= 31
   */
  int pow2plus1(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     return (1 << x) + 1;
  }

  /*
   * pow2plus4 - returns 2^x + 4, where 0 <= x <= 31
   */
  int pow2plus4(int x) {
     /* exploit ability of shifts to compute powers of 2 */
     int result = (1 << x);
     result += 4;
     return result;
  }

FLOATING POINT CODING RULES

For the problems that require you to implement floating-point operations,
the coding rules are less strict.  You are allowed to use looping and
conditional control.  You are allowed to use both ints and unsigneds.
You can use arbitrary integer and unsigned constants. You can use any arithmetic,
logical, or comparison operations on int or unsigned data.

You are expressly forbidden to:
  1. Define or use any macros.
  2. Define any additional functions in this file.
  3. Call any functions.
  4. Use any form of casting.
  5. Use any data type other than int or unsigned.  This means that you
     cannot use arrays, structs, or unions.
  6. Use any floating point data types, operations, or constants.


NOTES:
  1. Use the dlc (data lab checker) compiler (described in the handout) to 
     check the legality of your solutions.
  2. Each function has a maximum number of operations (integer, logical,
     or comparison) that you are allowed to use for your implementation
     of the function.  The max operator count is checked by dlc.
     Note that assignment ('=') is not counted; you may use as many of
     these as you want without penalty.
  3. Use the btest test harness to check your functions for correctness.
  4. Use the BDD checker to formally verify your functions
  5. The maximum number of ops for each function is given in the
     header comment for each function. If there are any inconsistencies 
     between the maximum ops in the writeup and in this file, consider
     this file the authoritative source.

/*
 * STEP 2: Modify the following functions according the coding rules.
 * 
 *   IMPORTANT. TO AVOID GRADING SURPRISES:
 *   1. Use the dlc compiler to check that your solutions conform
 *      to the coding rules.
 *   2. Use the BDD checker to formally verify that your solutions produce 
 *      the correct answers.
 */


#endif
/* Copyright (C) 1991-2020 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */
/* This header is separate from features.h so that the compiler can
   include it implicitly at the start of every compilation.  It must
   not itself include <features.h> or any other header that includes
   <features.h> because the implicit include comes before any feature
   test macros that may be defined in a source file before it first
   explicitly includes a system header.  GCC knows the name of this
   header in order to preinclude it.  */
/* glibc's intent is to support the IEC 559 math functionality, real
   and complex.  If the GCC (4.9 and later) predefined macros
   specifying compiler intent are available, use them to determine
   whether the overall intent is to support these features; otherwise,
   presume an older compiler has intent to support these features and
   define these macros by default.  */
/* wchar_t uses Unicode 10.0.0.  Version 10.0 of the Unicode Standard is
   synchronized with ISO/IEC 10646:2017, fifth edition, plus
   the following additions from Amendment 1 to the fifth edition:
   - 56 emoji characters
   - 285 hentaigana
   - 3 additional Zanabazar Square characters */
//1
/* 
 * bitXor - x^y using only ~ and & 
 *   Example: bitXor(4, 5) = 1
 *   Legal ops: ~ &
 *   Max ops: 14
 *   Rating: 1
 */
int bitXor(int x, int y) {
  // Set all positions where x and y both have a 1 to 0 (and the rest to a 1)
  int bothMask = ~(x & y);

  // Set all positions where x and y both have a 0 to 0 (and the rest to a 1)
  int neitherMask = ~(~x & ~y);

  // This AND will thus give us all bits where bit pairs of x and y were not
  // both 1 and not both 0
  return bothMask & neitherMask;
}
/* 
 * tmin - return minimum two's complement integer 
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 4
 *   Rating: 1
 */
int tmin(void) {
  // By two's complement, the smallest number starts with a 1 followed by 0s
  // only. Thus we can just shift a 1 to the most significant bit's position.
  return 1 << 31;
}
//2
/* 
 * allOddBits - return 1 if all odd-numbered bits in word set to 1
 *   where bits are numbered from 0 (least significant) to 31 (most significant)
 *   Examples allOddBits(0xFFFFFFFD) = 0, allOddBits(0xAAAAAAAA) = 1
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 2
 */
int allOddBits(int x) {
  // Define the pattern to check against - 170 is the decimal form of the
  // bits 10101010
  int pattern = 170; 

  // If the pattern can be AND'ed with each chunk of 8 bits in the 32-bit number
  // without losing a single 1, the pattern is matched.
  int mask = pattern & (x >> 24) & (x >> 16) & (x >> 8) & x;

  // If the pattern didn't lose a single 1, then mask = pattern and their XOR
  // becomes 0. Thus a 1 is returned. However, if the pattern did lose a 1,
  // mask != pattern and the XOR is non-zero, resulting in returning 0.
  return !(mask ^ pattern);
}
/* 
 * negate - return -x 
 *   Example: negate(1) = -1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 5
 *   Rating: 2
 */
int negate(int x) {
  return ~x + 1;
}
//3
/* 
 * conditional - same as x ? y : z 
 *   Example: conditional(2,4,5) = 4
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 16
 *   Rating: 3
 */
int conditional(int x, int y, int z) {
  // We obtain a 0 for any non-zero x and a 1 if x = 0
  int normalised = !x;

  // Using negation and overflow, we turn normalised into a number with all
  // bits set either to 1 or 0. 0 becomes all 0s and 1 becomes all 1s.
  int mask = ~normalised + 1;

  // If x was truthy, ~mask will be a bit string with only 1s - therefore we
  // retain all bits of y. But mask will be the bit string with only 0s, thus
  // all bits of z are discarded. And the other way around if x was falsy.
  return (~mask & y) | (mask & z);
}
/* 
 * isLessOrEqual - if x <= y  then return 1, else return 0 
 *   Example: isLessOrEqual(4,5) = 1.
 *   Legal ops: ! ~ & ^ | + << >>
 *   Max ops: 24
 *   Rating: 3
 */
int isLessOrEqual(int x, int y) {
  // We can determine if a number is smaller or equal to another with this:
  //    x <= y if y - x >= 0
  // However, due to overflow, we have to check some cases where we can avoid
  // doing arithmetic:
  //  - If x is negative and y is positive the answer is always 1
  //  - If y is negative and x is positive the answer is always 0

  // First we find y - x (by implementing subtraction as adding the negation)
  int diff = y + (~x + 1);
  // Then we determine if the sign bit is 1 (indicating a negative number)
  int diffIsNegative = 1 & (diff >> 31);
  // If the difference is not negative, it must be because x <= y.
  int isLessThan = !diffIsNegative;

  // And now, to avoid some overflow issues, we handle the cases described at
  // beginning. First we determine if the signs of x and y are different by
  // XOR'ing their sign bits:
  int isDifferentSign = 1 & ((x >> 31) ^ (y >> 31));
  // Then check if we are dealing with a negative x or negative y:
  int isNegAndPos = isDifferentSign & (x >> 31);
  int isPosAndNeg = isDifferentSign & (y >> 31);

  // Finally, we can produce our answer:
  //  - If x is positive and y is negative, the result will be forced to a 0
  //  - If x is negative and y is positive, the answer will be OR'ed to have a 1
  //  - Otherwise we just use the result of our arithmetic check
  return (isNegAndPos | isLessThan) & (!isPosAndNeg);
}
//4
/* 
 * logicalNeg - implement the ! operator, using all of 
 *              the legal operators except !
 *   Examples: logicalNeg(3) = 0, logicalNeg(0) = 1
 *   Legal ops: ~ & ^ | + << >>
 *   Max ops: 12
 *   Rating: 4 
 */
int logicalNeg(int x) {
  // First, we convert x from a number to its corresponding boolean 1 or 0. We
  // do this by finding the sign bit of x and then the sign bit of x's negation.
  // Only 0 will have no 1 in the sign bit in both cases.
  int truthy = (x >> 31) | ((~x + 1) >> 31);

  // And of course, we need the negation step too - and we only want the last
  // bit.
  return (~truthy) & 1;
}
/* howManyBits - return the minimum number of bits required to represent x in
 *             two's complement
 *  Examples: howManyBits(12) = 5
 *            howManyBits(298) = 10
 *            howManyBits(-5) = 4
 *            howManyBits(0)  = 1
 *            howManyBits(-1) = 1
 *            howManyBits(0x80000000) = 32
 *  Legal ops: ! ~ & ^ | + << >>
 *  Max ops: 90
 *  Rating: 4
 */
int howManyBits(int x) {
  // The general idea:
  // - Find the absolute value
  // - How many bits are required to represent that value?
  // - 1 more bit is needed for the sign

  // We find the absolute value of x. However, to avoid overflow issues with
  // -2147483648, we will have to subtract 1 from the absolute value if x is
  // negative - but the result we're looking for will be equally viable since
  // 0 is the most significant bit for positive two's complement numbers.
  int absMask = ~(1 & (x >> 31)) + 1;
  int abs = (~absMask & x) | (absMask & ~x);

  // We can now count how many bits we need to represent the unsigned number.
  // Due to the limited amount of operations we have at our disposal, we can't
  // search the 32 bits linearly - instead we use a binary search.
  //
  // The algorithm is as follows:
  //   1. Take a half-of-the-bits sized chunk out of the number - initially 16
  //      bits because the full number is 32 bits. That is, we will be looking
  //      at these bits first:
  //
  //          00000000 00000000 00000000 00000000
  //          ^^^^^^^^ ^^^^^^^^
  //
  //   2. Use !! to see if the bit chunk is nonzero
  int hasBits1 = !!(abs >> 16);
  //   3. Shift to the left by 4 (because 1 << 4 = 16, which is what we shifted
  //      right by). If there were any bits in the chunk, res1 now holds 16 as
  //      its value, but if no bits were present, its value is 0. If a bit was
  //      in the left half of the 32, we know we need at least 16 bits, so we
  //      hold onto this value for a bit (no pun intended).
  int res1 = hasBits1 << 4;
  //   4. Shift the number right by the amount of bits we know we need so far.
  //      By doing this, we perform the "binary search" aspect of the algorithm.
  //      If there was a bit in the left half, the next iteration will be
  //      looking at the first half of this left half - meaning these:
  //
  //          00000000 00000000 00000000 00000000
  //          ^^^^^^^^
  //
  //      That's thanks to us shifting the number right by 16 and the next
  //      iteration shifting right by 8. However, if no bit was in the left
  //      half, we shift right by 0, and the next iteration shifts right by 8.
  //      Therefore it will be looking in the first half of the right half:
  //
  //          00000000 00000000 00000000 00000000
  //                            ^^^^^^^^
  int abs1 = abs >> res1;
  //   5. Rinse and repeat with smaller and smaller halves until we've found how
  //      many bits we need in each.

  int hasBits2 = !!(abs1 >> 8);
  int res2 = hasBits2 << 3;
  int abs2 = abs1 >> res2;

  int hasBits3 = !!(abs2 >> 4);
  int res3 = hasBits3 << 2;
  int abs3 = abs2 >> res3;

  int hasBits4 = !!(abs3 >> 2);
  int res4 = hasBits4 << 1;
  int abs4 = abs3 >> res4;

  int hasBits5 = !!(abs4 >> 1);
  int res5 = hasBits5;
  int abs5 = abs4 >> res5;

  // 6. Add up all the different "I need at least X bits" results
  int unsignedBitsRequired = res1 + res2 + res3 + res4 + res5 + abs5;

  // And then we just need one more bit to represent signed numbers.
  return unsignedBitsRequired + 1;
}
