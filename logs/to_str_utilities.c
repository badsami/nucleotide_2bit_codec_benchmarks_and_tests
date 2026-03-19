#include "to_str_utilities.h"


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Intrinsics
// Declare the specific intrinsics used below as extern, rather than including the 1025-line-long
// intrin.h header 
extern unsigned char _BitScanReverse(unsigned long* msb_idx, unsigned long mask);
extern unsigned char _BitScanReverse64(unsigned long* msb_idx, unsigned __int64 mask);


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Intrisics wrapper
u64 bsr32(u32 num)
{
  unsigned long msb_idx;
  _BitScanReverse(&msb_idx, num);
  return msb_idx;
}


u64 bsr64(u64 num)
{
  unsigned long msb_idx;
  _BitScanReverse64(&msb_idx, num);
  return msb_idx;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Numerals count in a number
u64 u32_digit_count(u32 num)
{
  // Similar to https://commaok.xyz/post/lookup_tables/, but:
  // - lzcnt replaces bsr
  // - table is reversed
  // - an extra entry is added for num = 0, so this function returns 1
  // - the bitwise OR and substraction performed by int_log2() no longer happen
  static const u64 offsets[33] =
  {
    42949672960ull, 42949672960ull,                                 // (10 << 32)
    41949672960ull, 41949672960ull, 41949672960ull,                 // (10 << 32) - 1000000000
    38554705664ull, 38554705664ull, 38554705664ull,                 // (9  << 32) - 100000000
    34349738368ull, 34349738368ull, 34349738368ull, 34349738368ull, // (8  << 32) - 10000000
    30063771072ull, 30063771072ull, 30063771072ull,                 // (7  << 32) - 1000000
    25769703776ull, 25769703776ull, 25769703776ull,                 // (6  << 32) - 100000
    21474826480ull, 21474826480ull, 21474826480ull, 21474826480ull, // (5  << 32) - 10000
    17179868184ull, 17179868184ull, 17179868184ull,                 // (4  << 32) - 1000
    12884901788ull, 12884901788ull, 12884901788ull,                 // (3  << 32) - 100
    8589934582ull,  8589934582ull,  8589934582ull,                  // (2  << 32) - 10
    4294967296ull,                                                  // (1  << 32)
    4294967296ull                                                   // (1  << 32)
  };

  u64 lzcnt       = __lzcnt(num);
  u64 offset      = offsets[lzcnt];
  u64 digit_count = (num + offset) >> 32;

  return digit_count;
}


u64 u64_digit_count(u64 num)
{
  // https://lemire.me/blog/2025/01/07/counting-the-digits-of-64-bit-integers/
  static u64 thresholds[19] =
  {
    9ull,
    99ull,
    999ull,
    9999ull,
    99999ull,
    999999ull,
    9999999ull,
    99999999ull,
    999999999ull,
    9999999999ull,
    99999999999ull,
    999999999999ull,
    9999999999999ull,
    99999999999999ull,
    999999999999999ull,
    9999999999999999ull,
    99999999999999999ull,
    999999999999999999ull,
    9999999999999999999ull
  };
  u64 msb_idx       = bsr64(num | 0b1);
  u64 threshold_idx = (19ull * msb_idx) >> 6;
  u64 threshold     = thresholds[threshold_idx];
  u64 is_greater    = num > threshold;
  u64 digit_count   = threshold_idx + is_greater + 1ull;

  return digit_count;
}


u64 u32_bit_count(u32 num)
{
  return bsr32(num | 0b1) + 1ull;
}


u64 u64_bit_count(u64 num)
{
  return bsr64(num | 0b1) + 1ull;
}


u64 u32_nibble_count(u32 num)
{
  u64 msb_idx      = bsr32(num | 0b1);
  u64 nibble_count = 1ull + (msb_idx >> 2);

  return nibble_count;
}


u64 u64_nibble_count(u64 num)
{
  u64 msb_idx      = bsr64(num | 0b1);
  u64 nibble_count = 1ull + (msb_idx >> 2);
  
  return nibble_count;
}


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Floating-point specific values
u32 f32_is_a_number(f32 num)
{
  const u32 EXPONENT_ALL_ONE = 0x7F800000;
  
  u32 num_bits = *(u32*)&num;
  
  return (num_bits & EXPONENT_ALL_ONE) != EXPONENT_ALL_ONE;
}
