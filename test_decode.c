#include "logs/logs.h"
#include "../nucleotide_2bit_codec/decoded_tables.h"
#include "../nucleotide_2bit_codec/nucleotide_2bit_codec.h"
#include "../nucleotide_2bit_codec/types.h"

// Win32 external references
extern void ExitProcess(u32 exit_code);

s32 _fltused;

typedef void (*decode_func)(const u8* restrict encoded_bases,
                            char*     restrict decoded_bases,
                            u64                base_count,
                            const char         decoded_table[256 * 4]);

typedef struct DecodeFunction DecodeFunction;
struct DecodeFunction
{
  const char*       name;
  const decode_func ptr;
};

void program_start(void)
{
  const DecodeFunction functions[] =
  {
    {.name = "Scalar   ", .ptr = decode_bases_scalar},
    {.name = "SSE4.1   ", .ptr = decode_bases_sse4_1},
    {.name = "AVX2     ", .ptr = decode_bases_avx2}
  };

  logs_open_console_output();
  
  #define BASE_COUNT         (256 * 4 - 6)
  #define ENCODED_BYTE_COUNT ((BASE_COUNT + 3) / 4)

  char decoded_bases[BASE_COUNT];

  const u8 encoded_bases[ENCODED_BYTE_COUNT] =
  {
    0b00000000, 0b00000001, 0b00000010, 0b00000011,
    0b00000100, 0b00000101, 0b00000110, 0b00000111,
    0b00001000, 0b00001001, 0b00001010, 0b00001011,
    0b00001100, 0b00001101, 0b00001110, 0b00001111,
    0b00010000, 0b00010001, 0b00010010, 0b00010011,
    0b00010100, 0b00010101, 0b00010110, 0b00010111,
    0b00011000, 0b00011001, 0b00011010, 0b00011011,
    0b00011100, 0b00011101, 0b00011110, 0b00011111,
    0b00100000, 0b00100001, 0b00100010, 0b00100011,
    0b00100100, 0b00100101, 0b00100110, 0b00100111,
    0b00101000, 0b00101001, 0b00101010, 0b00101011,
    0b00101100, 0b00101101, 0b00101110, 0b00101111,
    0b00110000, 0b00110001, 0b00110010, 0b00110011,
    0b00110100, 0b00110101, 0b00110110, 0b00110111,
    0b00111000, 0b00111001, 0b00111010, 0b00111011,
    0b00111100, 0b00111101, 0b00111110, 0b00111111,

    0b01000000, 0b01000001, 0b01000010, 0b01000011,
    0b01000100, 0b01000101, 0b01000110, 0b01000111,
    0b01001000, 0b01001001, 0b01001010, 0b01001011,
    0b01001100, 0b01001101, 0b01001110, 0b01001111,
    0b01010000, 0b01010001, 0b01010010, 0b01010011,
    0b01010100, 0b01010101, 0b01010110, 0b01010111,
    0b01011000, 0b01011001, 0b01011010, 0b01011011,
    0b01011100, 0b01011101, 0b01011110, 0b01011111,
    0b01100000, 0b01100001, 0b01100010, 0b01100011,
    0b01100100, 0b01100101, 0b01100110, 0b01100111,
    0b01101000, 0b01101001, 0b01101010, 0b01101011,
    0b01101100, 0b01101101, 0b01101110, 0b01101111,
    0b01110000, 0b01110001, 0b01110010, 0b01110011,
    0b01110100, 0b01110101, 0b01110110, 0b01110111,
    0b01111000, 0b01111001, 0b01111010, 0b01111011,
    0b01111100, 0b01111101, 0b01111110, 0b01111111,

    0b10000000, 0b10000001, 0b10000010, 0b10000011,
    0b10000100, 0b10000101, 0b10000110, 0b10000111,
    0b10001000, 0b10001001, 0b10001010, 0b10001011,
    0b10001100, 0b10001101, 0b10001110, 0b10001111,
    0b10010000, 0b10010001, 0b10010010, 0b10010011,
    0b10010100, 0b10010101, 0b10010110, 0b10010111,
    0b10011000, 0b10011001, 0b10011010, 0b10011011,
    0b10011100, 0b10011101, 0b10011110, 0b10011111,
    0b10100000, 0b10100001, 0b10100010, 0b10100011,
    0b10100100, 0b10100101, 0b10100110, 0b10100111,
    0b10101000, 0b10101001, 0b10101010, 0b10101011,
    0b10101100, 0b10101101, 0b10101110, 0b10101111,
    0b10110000, 0b10110001, 0b10110010, 0b10110011,
    0b10110100, 0b10110101, 0b10110110, 0b10110111,
    0b10111000, 0b10111001, 0b10111010, 0b10111011,
    0b10111100, 0b10111101, 0b10111110, 0b10111111,

    0b11000000, 0b11000001, 0b11000010, 0b11000011,
    0b11000100, 0b11000101, 0b11000110, 0b11000111,
    0b11001000, 0b11001001, 0b11001010, 0b11001011,
    0b11001100, 0b11001101, 0b11001110, 0b11001111,
    0b11010000, 0b11010001, 0b11010010, 0b11010011,
    0b11010100, 0b11010101, 0b11010110, 0b11010111,
    0b11011000, 0b11011001, 0b11011010, 0b11011011,
    0b11011100, 0b11011101, 0b11011110, 0b11011111,
    0b11100000, 0b11100001, 0b11100010, 0b11100011,
    0b11100100, 0b11100101, 0b11100110, 0b11100111,
    0b11101000, 0b11101001, 0b11101010, 0b11101011,
    0b11101100, 0b11101101, 0b11101110, 0b11101111,
    0b11110000, 0b11110001, 0b11110010, 0b11110011,
    0b11110100, 0b11110101, 0b11110110, 0b11110111,
    0b11111000, 0b11111001, 0b11111010, 0b11111011,
    0b11111100, 0b11111101, 0b1110
  };

  #pragma warning(disable: 4295)
  const char expected_bases[BASE_COUNT] =
    "aaaa" "caaa" "gaaa" "uaaa"
    "acaa" "ccaa" "gcaa" "ucaa"
    "agaa" "cgaa" "ggaa" "ugaa"
    "auaa" "cuaa" "guaa" "uuaa"
    "aaca" "caca" "gaca" "uaca"
    "acca" "ccca" "gcca" "ucca"
    "agca" "cgca" "ggca" "ugca"
    "auca" "cuca" "guca" "uuca"
    "aaga" "caga" "gaga" "uaga"
    "acga" "ccga" "gcga" "ucga"
    "agga" "cgga" "ggga" "ugga"
    "auga" "cuga" "guga" "uuga"
    "aaua" "caua" "gaua" "uaua"
    "acua" "ccua" "gcua" "ucua"
    "agua" "cgua" "ggua" "ugua"
    "auua" "cuua" "guua" "uuua"

    "aaac" "caac" "gaac" "uaac"
    "acac" "ccac" "gcac" "ucac"
    "agac" "cgac" "ggac" "ugac"
    "auac" "cuac" "guac" "uuac"
    "aacc" "cacc" "gacc" "uacc"
    "accc" "cccc" "gccc" "uccc"
    "agcc" "cgcc" "ggcc" "ugcc"
    "aucc" "cucc" "gucc" "uucc"
    "aagc" "cagc" "gagc" "uagc"
    "acgc" "ccgc" "gcgc" "ucgc"
    "aggc" "cggc" "gggc" "uggc"
    "augc" "cugc" "gugc" "uugc"
    "aauc" "cauc" "gauc" "uauc"
    "acuc" "ccuc" "gcuc" "ucuc"
    "aguc" "cguc" "gguc" "uguc"
    "auuc" "cuuc" "guuc" "uuuc"

    "aaag" "caag" "gaag" "uaag"
    "acag" "ccag" "gcag" "ucag"
    "agag" "cgag" "ggag" "ugag"
    "auag" "cuag" "guag" "uuag"
    "aacg" "cacg" "gacg" "uacg"
    "accg" "cccg" "gccg" "uccg"
    "agcg" "cgcg" "ggcg" "ugcg"
    "aucg" "cucg" "gucg" "uucg"
    "aagg" "cagg" "gagg" "uagg"
    "acgg" "ccgg" "gcgg" "ucgg"
    "aggg" "cggg" "gggg" "uggg"
    "augg" "cugg" "gugg" "uugg"
    "aaug" "caug" "gaug" "uaug"
    "acug" "ccug" "gcug" "ucug"
    "agug" "cgug" "ggug" "ugug"
    "auug" "cuug" "guug" "uuug"

    "aaau" "caau" "gaau" "uaau"
    "acau" "ccau" "gcau" "ucau"
    "agau" "cgau" "ggau" "ugau"
    "auau" "cuau" "guau" "uuau"
    "aacu" "cacu" "gacu" "uacu"
    "accu" "cccu" "gccu" "uccu"
    "agcu" "cgcu" "ggcu" "ugcu"
    "aucu" "cucu" "gucu" "uucu"
    "aagu" "cagu" "gagu" "uagu"
    "acgu" "ccgu" "gcgu" "ucgu"
    "aggu" "cggu" "gggu" "uggu"
    "augu" "cugu" "gugu" "uugu"
    "aauu" "cauu" "gauu" "uauu"
    "acuu" "ccuu" "gcuu" "ucuu"
    "aguu" "cguu" "gguu" "uguu"
    "auuu" "cuuu" "gu";
#pragma warning(default: 4295)

  const u64 func_count = sizeof(functions) / sizeof(functions[0]);
  for (u64 func_idx = 0; func_idx < func_count; func_idx += 1)
  {
    log_literal_str(functions[func_idx].name);
    logs_flush();

    functions[func_idx].ptr(encoded_bases, decoded_bases, BASE_COUNT, decoded_acgu);

    const char* const decoded_bases_end    = decoded_bases + BASE_COUNT;
    const char* const decoded_bases_x8_end = decoded_bases + (BASE_COUNT & ~7);
    const char* decoded_bases_ptr  = decoded_bases;
    const char* expected_bases_ptr = expected_bases;
    while (decoded_bases_ptr < decoded_bases_x8_end)
    {
      u64 decoded_bases_x8  = *(u64*)decoded_bases_ptr;
      u64 expected_bases_x8 = *(u64*)expected_bases_ptr;
      if (decoded_bases_x8 != expected_bases_x8)
      {
        const u64 base_idx = decoded_bases_ptr - decoded_bases;
        log_literal_str("Failed somewhere between bases ");
        log_dec_u64(base_idx);
        log_literal_str(" and ");
        log_dec_u64(base_idx + 8);
        log_literal_str("\n- expected: ");
        log_sized_str((char*)expected_bases_ptr, 8);
        log_literal_str("\n- actual:   ");
        log_sized_str((char*)decoded_bases_ptr, 8);
        log_character('\n');
        logs_flush();
        goto next_func;
      }

      decoded_bases_ptr  += 8;
      expected_bases_ptr += 8;
    }

    while (decoded_bases_ptr < decoded_bases_end)
    {
      char decoded_base  = *decoded_bases_ptr;
      char expected_base = *expected_bases_ptr;
      if (decoded_base != expected_base)
      {
        const u64 base_idx = decoded_bases_ptr - decoded_bases;
        log_literal_str("Failed at base ");
        log_dec_u64(base_idx);
        log_literal_str("\n- expected: ");
        log_character(decoded_base);
        log_literal_str("\n- actual:   ");
        log_character(decoded_base);
        log_character('\n');
        logs_flush();
        goto next_func;
      }

      decoded_bases_ptr  += 1;
      expected_bases_ptr += 1;
    }

    log_literal_str("OK\n");
    logs_flush();
    next_func:;
  }
  
  logs_close_console_output();
  ExitProcess(0);
}
