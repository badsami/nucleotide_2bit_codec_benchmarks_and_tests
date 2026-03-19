#if defined(LOGS_ENABLED) && (LOGS_ENABLED != 0)
#include "logs.h"
#include "to_str_utilities.h"

#define _WIN32_WINNT 0x0501 // ATTACH_PARENT_PROCESS
#include <Windows.h>


///////////////////////////////////////////////////////////////////////////////////////////////////
//// Private helpers
static void logs_close_output(logs_output_idx output_idx)
{
  u32 output_mask          = (1 << output_idx);
  u32 output_was_enabled   = logs.outputs_state_bits & output_mask;
  logs.outputs_state_bits &= ~output_mask;
  
  if (logs.buffer_end_idx != 0 && output_was_enabled)
  {
    // Flush buffered content to the output before closing it
    WriteFile(logs.outputs[output_idx], logs.buffer, (u32)logs.buffer_end_idx, 0, 0);
    
    // If there are no other enabled outputs, the content of the log buffer is no longer needed
    u32 any_output_open = (logs.outputs_state_bits != 0);
    logs.buffer_end_idx = any_output_open * logs.buffer_end_idx;
  }
  
  CloseHandle(logs.outputs[output_idx]);
  logs.outputs[output_idx] = 0;
}


// memcpy is automatically inserted to replace bits of code by compilers, even when the standard
// standard library and C runtime are explicitely excluded through compiler flags. A minimal version
// of memcpy is enough for this library, and is called instead of the code that would otherwise be
// replaced
#pragma function(memcpy)
void* memcpy(void* dest, const void* src, u64 byte_count)
{
  const u8* src_u8  = src;
  u8*       dest_u8 = dest;

  const u64       u8_x8_count   = byte_count & ~7;
  const u8* const src_u8_x8_end = src_u8 + u8_x8_count;
  const u8* const src_u8_end    = src_u8 + byte_count;

  while (src_u8 < src_u8_x8_end)
  {
    *(u64*)dest_u8 = *(u64*)src_u8;

    dest_u8 += 8;
    src_u8  += 8;
  }
  
  while (src_u8 < src_u8_end)
  {
    *dest_u8 = *src_u8;
    dest_u8 += 1;
    src_u8  += 1;
  }

  return dest_u8;
}



///////////////////////////////////////////////////////////////////////////////////////////////////
//// Global
struct logs logs =
{
  .buffer             = {0},
  .outputs            = {0, 0},
  .buffer_end_idx     = 0,
  .outputs_state_bits = 0
};




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Output management
// Console output
void logs_open_console_output(void)
{
  if (logs.outputs[LOGS_CONSOLE_OUTPUT] == 0)
  {
    const BOOL success    = AttachConsole(ATTACH_PARENT_PROCESS);
    const u32  last_error = GetLastError();
    if ((success == 0) && (last_error != ERROR_ACCESS_DENIED))
    {
      // There is no console to borrow, or something went wrong. Create a new console
      AllocConsole();
      SetConsoleTitleA("Logs");

      logs.console_original_output_code_page = 0u;
    }
    else
    {
      // An existing console is attached
      logs.console_original_output_code_page = GetConsoleOutputCP();
    }

    SetConsoleOutputCP(CP_UTF8);

    const u32 SHARE_MODE = FILE_SHARE_READ | FILE_SHARE_WRITE;
    logs.outputs[LOGS_CONSOLE_OUTPUT] = CreateFileA("\\\\?\\CONOUT$", // lpFileName
                                                    GENERIC_WRITE,    // dwDesiredAccess
                                                    SHARE_MODE,       // dwShareMode
                                                    0,                // lpSecurityAttributes
                                                    OPEN_EXISTING,    // dwCreationDisposition
                                                    0,                // dwFlagsAndAttributes
                                                    0);               // hTemplateFile
    logs_enable_output(LOGS_CONSOLE_OUTPUT);
  }
}


// NOTE (Sami): this breaks Windows Terminal (the default Windows 11 console)
void logs_close_console_output(void)
{
  if (logs.outputs[LOGS_CONSOLE_OUTPUT] != 0)
  {
    logs_close_output(LOGS_CONSOLE_OUTPUT);

    if (logs.console_original_output_code_page != 0u)
    {
      SetConsoleOutputCP(logs.console_original_output_code_page);
    }
    
    // Free the console of this process
    FreeConsole();
  }
}


// File output
void logs_open_file_output_ascii(const char* file_path)
{
  if (logs.outputs[LOGS_FILE_OUTPUT] == 0)
  {
    const u32 SHARE_MODE = FILE_SHARE_READ | FILE_SHARE_WRITE;
    logs.outputs[LOGS_FILE_OUTPUT] = CreateFileA(file_path,        // lpFileName
                                                 FILE_APPEND_DATA, // dwDesiredAccess
                                                 SHARE_MODE,       // dwShareMode
                                                 0,                // lpSecurityAttributes
                                                 OPEN_ALWAYS,      // dwCreationDisposition
                                                 0,                // dwFlagsAndAttributes
                                                 0);               // hTemplateFile
    logs_enable_output(LOGS_FILE_OUTPUT);
  }
}


void logs_open_file_output_utf16(const WCHAR* file_path)
{
  if (logs.outputs[LOGS_FILE_OUTPUT] == 0)
  {
    const u32 SHARE_MODE = FILE_SHARE_READ | FILE_SHARE_WRITE;
    logs.outputs[LOGS_FILE_OUTPUT] = CreateFileW(file_path,        // lpFileName
                                                 FILE_APPEND_DATA, // dwDesiredAccess
                                                 SHARE_MODE,       // dwShareMode
                                                 0,                // lpSecurityAttributes
                                                 OPEN_ALWAYS,      // dwCreationDisposition
                                                 0,                // dwFlagsAndAttributes
                                                 0);               // hTemplateFile
    logs_enable_output(LOGS_FILE_OUTPUT);
  }
}


void logs_close_file_output(void)
{
  if (logs.outputs[LOGS_FILE_OUTPUT] != 0)
  {
    logs_close_output(LOGS_FILE_OUTPUT);
  }
}


// All outputs
void logs_disable_output(logs_output_idx output_idx)
{
  logs.outputs_state_bits &= ~(1 << output_idx);
}


void logs_enable_output(logs_output_idx output_idx)
{
  u32 is_open = (logs.outputs[output_idx] != 0);
  logs.outputs_state_bits |= (is_open << output_idx);
}


void logs_flush(void)
{
  // Trust that the caller knows the log buffer is not empty
  // If an output is enabled, it is open. Parse the output state bits to select destination outputs
  u32 outputs_mask = logs.outputs_state_bits;
  u32 idx          = 0;
  while (outputs_mask != 0)
  {
    if (outputs_mask & 0b1)
    {
      WriteFile(logs.outputs[idx], logs.buffer, (u32)logs.buffer_end_idx, 0, 0);
    }
    
    idx++;
    outputs_mask >>= 1;
  }

  logs.buffer_end_idx = 0;
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Memory
u64 logs_buffer_remaining_bytes(void)
{
  s64 difference      = LOGS_BUFFER_SIZE - logs.buffer_end_idx;
  u64 remaining_bytes = (difference > 0ll) ? difference : 0ll;

  return remaining_bytes;
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Characters & strings logging
void log_utf8_character(char character)
{
  logs.buffer[logs.buffer_end_idx] = character;
  logs.buffer_end_idx += 1u;
}


void log_utf16_character(WCHAR character)
{
  log_sized_utf16_str(&character, 1u);
}


void log_sized_utf8_str(const char* str, u64 char_count)
{
  char* dest = (char*)(logs.buffer + logs.buffer_end_idx);
  memcpy(dest, str, char_count);
  logs.buffer_end_idx += char_count;
}


void log_sized_utf16_str(const WCHAR* str, s32 wchar_count)
{
  // None of the functions appending content to the logs buffer make sure there is enough space to
  // write to. Lie about the available space in the logs buffer to remain consistent
  const s32 AVAILABLE_BYTES = ~(1 << 31);

  // TODO: implement our own WideCharToMultiByte()
  char* dest = (char*)(logs.buffer + logs.buffer_end_idx);
  s32 bytes_written = WideCharToMultiByte(CP_UTF8,         // CodePage,
                                          0,               // dwFlags,
                                          str,             // lpWideCharStr
                                          wchar_count,     // cchWideChar 
                                          dest,            // lpMultiByteStr
                                          AVAILABLE_BYTES, // cbMultiByte
                                          0,               // lpDefaultChar
                                          0);              // lpUsedDefaultChar
  logs.buffer_end_idx += bytes_written;
}


void log_null_terminated_utf8_str(const char* str)
{
  char character = *str;
  while (character != '\0')
  {
    logs.buffer[logs.buffer_end_idx] = character;
    logs.buffer_end_idx += 1ull;
    
    str  += 1;
    character = *str;
  }
}


void log_null_terminated_utf16_str(const WCHAR* str)
{
  log_sized_utf16_str(str, -1);
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Non-alphanumeric types logging
static const char* bool_str = "truefalse";

void log_bool(u32 boolean)
{
  const u64 is_false = (boolean == 0);
  const u64 offset   = is_false << 2; // 4 if (boolean == 0), 0 otherwise

  // Starts either at the 't' of "true" or the 'f' of "false"
  const char* bool_str_start = bool_str + offset;

  // length("true") = 4, length("false") = 5, is_false = 0 or 1
  const u64 char_count  = 4 + is_false;   
  log_sized_utf8_str(bool_str_start, char_count);
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Compounds logging
static const u64 unit_multipliers[] =
{
  // Maybe it's a road. Perhaps a flame? Or a witch hat! A reverse lightning bolt even!
  1,
  1000, // K
  1000000, // M
  1000000000, // G
  1000000000000ull, // T
  1000000000000000ull, // P
  1000000000000000000ull // E
};

static const char dec_unit_prefixes[7] = {0, 'K', 'M', 'G', 'T', 'P', 'E'};

void log_byte_count_dec_unit(u64 byte_count)
{
  // Log the integer part
  const u64 digit_count          = u64_digit_count(byte_count); // in [1; 20]
  const u64 unit_idx             = (digit_count - 1) / 3; // in [0; 6]
  const u64 unit_mul             = unit_multipliers[unit_idx];
  const u64 int_byte_count       = byte_count / unit_mul; // has 1 to 3 digits
  const u64 int_byte_digit_count = digit_count - (unit_idx * 3);

  log_sized_dec_u64(int_byte_count, int_byte_digit_count);

  // Log the fractional part (if necessary)
  const u32 byte_count_ge_1000 = byte_count >= 1000;
  if (byte_count_ge_1000)
  {
    const u64 byte_count_remainder = byte_count - (int_byte_count * unit_mul);
    const u64 frac_byte_count      = (BYTE_COUNT_FRAC_DIV * byte_count_remainder) / unit_mul;
    log_character('.');
    log_sized_dec_u64(frac_byte_count, BYTE_COUNT_FRAC_SIZE);
  }

  // Log the unit prefix and the unit itself
  // ' ' [+ unit prefix] + 'B' = 2 mandatory + 1 optional characters
  u8* const dest              = logs.buffer + logs.buffer_end_idx;
  const u64 b_char_idx        = 1 + byte_count_ge_1000;
  const u64 suffix_char_count = 2 + byte_count_ge_1000;

  dest[0]          = ' ';
  dest[1]          = dec_unit_prefixes[unit_idx]; // overwritten if unnecessary
  dest[b_char_idx] = 'B';

  logs.buffer_end_idx += suffix_char_count;
}


void log_byte_count_bin_unit(u64 byte_count)
{
  // Log the integer part
  const u64 msb_idx        = bsr64(byte_count | 0b1);
  const u64 prefix_idx     = msb_idx / 10;
  const u8  mul_shift      = (u8)(prefix_idx * 10);
  const u32 int_byte_count = (u32)(byte_count >> mul_shift);
  log_dec_u32(int_byte_count);

  // Log the fractional part (if necessary)
  const u32 byte_count_ge_1024 = byte_count >= 1024;
  if (byte_count_ge_1024)
  {
    const u64 byte_count_remainder = byte_count - (int_byte_count << mul_shift);
    const u64 unit_mul             = 1ull << mul_shift;
    const u64 frac_byte_count      = (BYTE_COUNT_FRAC_DIV * byte_count_remainder) / unit_mul;
    log_character('.');
    log_sized_dec_u64(frac_byte_count, BYTE_COUNT_FRAC_SIZE);
  }

  // Log the unit prefix and the unit itself
  // ' ' [+ unit prefix + 'i'] + 'B' = 2 mandatory + 2 optional characters
  u8* const dest              = logs.buffer + logs.buffer_end_idx;
  const u64 b_char_offset     = byte_count_ge_1024 * 2;
  const u64 suffix_char_count = 2 + b_char_offset;
  const u64 i_char_idx        = 1 + byte_count_ge_1024;
  const u64 b_char_idx        = 1 + b_char_offset;

  dest[0]          = ' ';
  dest[1]          = dec_unit_prefixes[prefix_idx]; // overwritten if unnecessary
  dest[i_char_idx] = 'i'; // overwritten if unnecessary
  dest[b_char_idx] = 'B';

  logs.buffer_end_idx += suffix_char_count;
}


void log_last_windows_error(void)
{
  const DWORD flags = FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS;

  // Can be obtained either from winnt.h, or through "Windows Language Code Identifier (LCID)
  // Reference", version 16.0 (23rd April 2024), page 14:
  // https://winprotocoldoc.z19.web.core.windows.net/MS-LCID/[MS-LCID].pdf#page=14
  const WORD en_us_lang_id = 0x0409;

  // None of the functions appending content to the logs buffer make sure there is enough space to
  // write to. Lie about the available space in the logs buffer to remain consistent
  const DWORD max_bytes = 64000; // maximum allowed by FormatMessage()

  u32 last_error = GetLastError();
  log_literal_str("Last Windows error: ");
  log_dec_u32(last_error);
  log_literal_str(", ");

  char* dest = (char*)(logs.buffer + logs.buffer_end_idx);

  DWORD char_written = FormatMessageA(flags,         // dwFlags
                                      0,             // lpSource
                                      last_error,    // dwMessageId
                                      en_us_lang_id, // dwLanguageId
                                      dest,          // lpBuffer
                                      max_bytes,     // nSize
                                      0);            // Arguments
  if (char_written == 0)
  {
    // Something went wrong, fallback
    log_literal_str("couldn't get error description)");
  }
  else
  {
    // Messages finish with a carriage return + linefeed. Both are removed
    logs.buffer_end_idx += char_written - 2;
  }
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Binary number logging
void log_sized_bin_s8 (s8  num, u64 bit_to_write_count) { log_sized_bin_u64((u64)num, bit_to_write_count); }
void log_sized_bin_s16(s16 num, u64 bit_to_write_count) { log_sized_bin_u64((u64)num, bit_to_write_count); }
void log_sized_bin_s32(s32 num, u64 bit_to_write_count) { log_sized_bin_u64((u64)num, bit_to_write_count); }
void log_sized_bin_s64(s64 num, u64 bit_to_write_count) { log_sized_bin_u64((u64)num, bit_to_write_count); }
void log_sized_bin_u8 (u8  num, u64 bit_to_write_count) { log_sized_bin_u64(num,      bit_to_write_count); }
void log_sized_bin_u16(u16 num, u64 bit_to_write_count) { log_sized_bin_u64(num,      bit_to_write_count); }
void log_sized_bin_u32(u32 num, u64 bit_to_write_count) { log_sized_bin_u64(num,      bit_to_write_count); }


void log_sized_bin_u64(u64 num, u64 bit_to_write_count)
{
  u8* const num_str_start = logs.buffer + logs.buffer_end_idx;
  u8*       dest          = num_str_start + bit_to_write_count;
  while (dest > num_str_start)
  {
    u8 bit = num & 0b1;
    
    dest -= 1;
    *dest = '0' + bit;
    
    num >>= 1;
  }

  logs.buffer_end_idx += bit_to_write_count;
}


void log_sized_bin_f32(f32 num, u64 bit_to_write_count) { log_sized_bin_u64(*(u32*)(&num), bit_to_write_count); }


void log_bin_s8 (s8  num) { log_sized_bin_u64((u64)num,      u32_bit_count((u32)num));    }
void log_bin_s16(s16 num) { log_sized_bin_u64((u64)num,      u32_bit_count((u32)num));    }
void log_bin_s32(s32 num) { log_sized_bin_u64((u64)num,      u32_bit_count((u32)num));    }
void log_bin_s64(s64 num) { log_sized_bin_u64((u64)num,      u64_bit_count((u64)num));    }
void log_bin_u8 (u8  num) { log_sized_bin_u64(num,           u32_bit_count(num));         }
void log_bin_u16(u16 num) { log_sized_bin_u64(num,           u32_bit_count(num));         }
void log_bin_u32(u32 num) { log_sized_bin_u64(num,           u32_bit_count(num));         }
void log_bin_u64(u64 num) { log_sized_bin_u64(num,           u64_bit_count(num));         }
void log_bin_f32(f32 num) { log_sized_bin_u64(*(u32*)(&num), u32_bit_count(*(u32*)&num)); }




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Decimal number logging
void log_sized_dec_s8 (s8  num, u64 digit_to_write_count) { log_sized_dec_u64((u64)num, digit_to_write_count); }
void log_sized_dec_s16(s16 num, u64 digit_to_write_count) { log_sized_dec_u64((u64)num, digit_to_write_count); }
void log_sized_dec_s32(s32 num, u64 digit_to_write_count) { log_sized_dec_u64((u64)num, digit_to_write_count); }


void log_sized_dec_s64(s64 num, u64 digit_to_write_count)
{
  u64 is_neg  = num < 0ull;
  u64 pos_num = is_neg ? -num : num;

  logs.buffer[logs.buffer_end_idx] = '-'; // overwritten if unnecessary
  logs.buffer_end_idx += is_neg;
  
  log_sized_dec_u64(pos_num, digit_to_write_count);
}


void log_sized_dec_u8 (u8  num, u64 digit_to_write_count) { log_sized_dec_u64(num, digit_to_write_count); }
void log_sized_dec_u16(u16 num, u64 digit_to_write_count) { log_sized_dec_u64(num, digit_to_write_count); }
void log_sized_dec_u32(u32 num, u64 digit_to_write_count) { log_sized_dec_u64(num, digit_to_write_count); }


void log_sized_dec_u64(u64 num, u64 digit_to_write_count)
{
  u8* const num_str_start = logs.buffer + logs.buffer_end_idx;
  u8*       dest          = num_str_start + digit_to_write_count;
  while (dest > num_str_start)
  {
    u64 quotient = num / 10ull;
    u8  digit    = (u8)(num - (quotient * 10ull));

    dest -= 1;
    *dest = '0' + digit;
    
    num = quotient;
  }
  
  logs.buffer_end_idx += digit_to_write_count;
}


static const f32 f32_frac_size_to_mul[F32_DEC_FRAC_MAX_STR_SIZE + 1] =
{
  1.f,
  10.f,
  100.f,
  1000.f,
  10000.f,
  100000.f,
  1000000.f,
  10000000.f,
  100000000.f,
  1000000000.f
};

void log_sized_dec_f32_number(f32 num, u64 frac_digit_to_write_count)
{
  u32 is_neg = num < 0.f;
  
  logs.buffer[logs.buffer_end_idx] = '-'; // overwritten if unnecessary
  logs.buffer_end_idx += is_neg;

  // Absolute values equal or greater to 8 388 608 are likely better represented as a s32 or s64
  // than as a 32-bit floating-point value (thereafter referred to as "f32"), for two reasons:
  // 
  // 1. Range of values: f32 values can be as large as +/- 3.4 x 10^38. I've never needed to
  // represent values that large as a f32. If they are ever necessary to you, I believe s32 and s64
  // are better fits because they can represent values up to +/- 2.15 x 10^9 and +/- 9.22 x 10^18
  // respectively, which might cover your needs. Otherwise, changing scale (i.e. meter to lightyear)
  // might be a better option because of reason #2.
  // 
  // 2. Precision: starting at 8 388 608 and beyond, f32 values cannot have a fractional part.
  // Starting at 16 777 216 and beyond, adding 1 to a f32 value doesn't change it. The fractional
  // part is not very significant at this magnitude, but s32 and s64 retain a precision of 1 on the
  // full range of values they cover.
  //
  // Nonetheless, 8 388 608 needs 23 bits to be represented, which would require a u32. So the full
  // range of the u32 type (+/- [0; 4 294 967 296]) may as well be supported, adding an extra
  // margin
  num = is_neg ? -num : num;
  if (num < 4294967296.f)
  {
    u32 num_int = (u32)num;
    log_dec_u32(num_int);
    
    if (num < 8388608.f)
    {
      // The same way values I've never needed to represent very large f32 values, I believe 
      // representing values under 0.000001 (without changing units) should never be needed
      f32 num_rounded = (f32)num_int;
      f32 num_frac    = num - num_rounded;
      if (num_frac >= 0.000001f)
      {
        log_character('.');

        // Fun fact: for floating-point values with exponent n (n < 23), the maximum count of decimal
        // fractional digit is 23 - n. Using the unbiased exponent u, this is the same as 150 - u.
        //
        //   u32 num_bits             = *(u32*)&num;
        //   s8  unbiased_exp         = (s8)((num_bits & 0x7F800000) >> 23);
        //   u8  max_frac_digit_count = 150u - unbiased_exp;

        // There can never be more fractional digits than the configured maximum allowed.
        // F32_DEC_FRAC_MAX_STR_SIZE is defined in logs.h
        u64 num_frac_digit_to_write_count = (frac_digit_to_write_count < F32_DEC_FRAC_MAX_STR_SIZE) ?
                                            frac_digit_to_write_count : F32_DEC_FRAC_MAX_STR_SIZE;
        f32 num_frac_ext = num_frac * f32_frac_size_to_mul[num_frac_digit_to_write_count];
        u32 num_frac_int = (u32)num_frac_ext;

        log_sized_dec_u32(num_frac_int, num_frac_digit_to_write_count);
      }
    }
  }
  else
  {
    u8* dest = logs.buffer + logs.buffer_end_idx;
    dest[0] = 'b';
    dest[1] = 'i';
    dest[2] = 'g';

    logs.buffer_end_idx += 3u;
  }
}


void log_sized_dec_f32(f32 num, u64 frac_size)
{
  u32 is_a_number = f32_is_a_number(num);
  if (is_a_number)
  {
    log_sized_dec_f32_number(num, frac_size);
  }
  else
  {
    log_dec_f32_nan_or_inf(num);
  }
}


void log_dec_s8 (s8  num) { log_dec_s32(num); }
void log_dec_s16(s16 num) { log_dec_s32(num); }


void log_dec_s32(s32 num)
{
  u32 is_neg  = num < 0ll;
  u32 pos_num = (u32)(is_neg ? -num : num);

  u8* num_str_start        = logs.buffer + logs.buffer_end_idx;
  u64 digit_to_write_count = u32_digit_count(pos_num);
  u64 char_to_write_count  = digit_to_write_count + is_neg;
  u8* dest                 = num_str_start + char_to_write_count;
  
  *num_str_start = '-'; // will be overwritten if not needed
  num_str_start += is_neg;
  while (dest > num_str_start)
  {
    u32 quotient = pos_num / 10u;
    u8  digit    = (u8)(pos_num - (quotient * 10u));

    dest -= 1;
    *dest = '0' + digit;
    
    pos_num = quotient;
  }
  
  logs.buffer_end_idx += char_to_write_count;
}


void log_dec_s64(s64 num)
{
  u64 is_neg  = num < 0ll;
  u64 pos_num = (u64)(is_neg ? -num : num);

  u8* const num_str_start        = logs.buffer + logs.buffer_end_idx;
  u64       digit_to_write_count = u64_digit_count(pos_num);
  u64       char_to_write_count  = digit_to_write_count + is_neg;
  u8*       dest                 = num_str_start + char_to_write_count;
  
  *num_str_start = '-'; // will be overwritten if not needed
  while (dest > num_str_start)
  {
    u64 quotient = pos_num / 10ull;
    u8  digit    = (u8)(pos_num - (quotient * 10ull));

    dest -= 1;
    *dest = '0' + digit;
    
    pos_num = quotient;
  }
  
  logs.buffer_end_idx += char_to_write_count;
}


void log_dec_u8 (u8  num) { log_sized_dec_u64(num, u32_digit_count(num)); }
void log_dec_u16(u16 num) { log_sized_dec_u64(num, u32_digit_count(num)); }
void log_dec_u32(u32 num) { log_sized_dec_u64(num, u32_digit_count(num)); }
void log_dec_u64(u64 num) { log_sized_dec_u64(num, u64_digit_count(num)); }


void log_dec_f32_nan_or_inf(f32 num)
{
  u32 num_bits = *(u32*)&num;
  
  // num is +infinity, -infinity, qnan, -qnan, snan or -snan
  // The negative sign is overwritten if it is unnecessary
  u8* const num_str_start = logs.buffer + logs.buffer_end_idx;
  num_str_start[0] = '-'; // overwritten if unnecessary

  u8* dest = num_str_start;
  dest += num_bits >> 31;
 
  const u32 MANTISSA_MASK = 0x007FFFFF;
  u32 mantissa_bits = num_bits & MANTISSA_MASK;
  if (mantissa_bits == 0)
  {
    dest[0] = 'i';
    dest[1] = 'n';
    dest[2] = 'f';
    dest += 3;
  }
  else
  {
    // A nan is a float with all the exponent bits set and at least one fraction bit set.
    // A quiet nan (qnan) is a nan with the leftmost, highest fraction bit set.
    // A signaling nan (qnan) is a nan with the leftmost, highest fraction bit clear.
    // 's' - 'q' is 2, which is a multiple of 2. We can use this to change 's' to a 'q' by
    // offsetting 's' to 'q' without ifs
    u8 is_quiet_offset = (u8)((num_bits & 0x00400000) >> 21);
    
    dest[0] = 's' - is_quiet_offset;
    dest[1] = 'n';
    dest[2] = 'a';
    dest[3] = 'n';
    dest += 4;
  }

  logs.buffer_end_idx += (u64)(dest - num_str_start);
}


void log_dec_f32_number(f32 num)
{
  u64 is_neg = num < 0.f;
  
  logs.buffer[logs.buffer_end_idx] = '-'; // overwritten if unnecessary
  logs.buffer_end_idx += is_neg;

  // Absolute values equal or greater to 8 388 608 are likely better represented as a s32 or s64
  // than as a 32-bit floating-point value (thereafter referred to as "f32"), for two reasons:
  // 
  // 1. Range of values: f32 values can be as large as +/- 3.4 x 10^38. I've never needed to
  // represent values that large as a f32. If they are ever necessary to you, I believe s32 and s64
  // are better fits because they can represent values up to +/- 2.15 x 10^9 and +/- 9.22 x 10^18
  // respectively, which might cover your needs. Otherwise, changing scale (i.e. meter to lightyear)
  // might be a better option because of reason #2.
  // 
  // 2. Precision: starting at 8 388 608 and beyond, f32 values cannot have a fractional part.
  // Starting at 16 777 216 and beyond, adding 1 to a f32 value doesn't change it. The fractional
  // part is not very significant at this magnitude, but s32 and s64 retain a precision of 1 on the
  // full range of values they cover.
  //
  // Nonetheless, 8 388 608 needs 23 bits to be represented, which would require a u32. So the full
  // range of the u32 type (+/- [0; 4 294 967 296]) may as well be supported, adding an extra
  // margin
  num = is_neg ? -num : num;
  if (num < 4294967296.f)
  {
    u32 num_int = (u32)num;
    log_dec_u32(num_int);
    
    if (num < 8388608.f)
    {
      // The same way values I've never needed to represent very large f32 values, I believe 
      // representing values under 0.000001 (without changing units) should never be needed
      f32 num_rounded = (f32)num_int;
      f32 num_frac    = num - num_rounded;
      if (num_frac >= 0.000001f)
      {
        log_character('.');

        // Fun fact: for floating-point values with exponent n (n < 23), the maximum count of decimal
        // fractional digit is 23 - n. Using the unbiased exponent u, this is the same as 150 - u.
        //
        //   u32 num_bits             = *(u32*)&num;
        //   s8  unbiased_exp         = (s8)((num_bits & 0x7F800000) >> 23);
        //   u8  max_frac_digit_count = 150u - unbiased_exp;

        // F32_DEC_FRAC_MULT and F32_DEC_FRAC_DEFAULT_STR_SIZE are defined in logs.h
        u32 num_frac_int = (u32)(num_frac * F32_DEC_FRAC_MULT);
        log_sized_dec_u32(num_frac_int, F32_DEC_FRAC_DEFAULT_STR_SIZE);
      }
    }
  }
  else
  {
    u8* dest = logs.buffer + logs.buffer_end_idx;
    dest[0] = 'b';
    dest[1] = 'i';
    dest[2] = 'g';

    logs.buffer_end_idx += 3u;
  }
}


void log_dec_f32(f32 num)
{
  u32 is_a_number = f32_is_a_number(num);
  if (is_a_number)
  {
    log_dec_f32_number(num);
  }
  else
  {
    log_dec_f32_nan_or_inf(num);
  }
}




///////////////////////////////////////////////////////////////////////////////////////////////////
//// Hexadecimal number logging
static const char hex_digits[] = "0123456789ABCDEF";

void log_sized_hex_s8 (s8  num, u64 nibble_to_write_count) { log_sized_hex_u64((u64)num, nibble_to_write_count); }
void log_sized_hex_s16(s16 num, u64 nibble_to_write_count) { log_sized_hex_u64((u64)num, nibble_to_write_count); }
void log_sized_hex_s32(s32 num, u64 nibble_to_write_count) { log_sized_hex_u64((u64)num, nibble_to_write_count); }
void log_sized_hex_s64(s64 num, u64 nibble_to_write_count) { log_sized_hex_u64((u64)num, nibble_to_write_count); }
void log_sized_hex_u8 (u8  num, u64 nibble_to_write_count) { log_sized_hex_u64(num,      nibble_to_write_count); }
void log_sized_hex_u16(u16 num, u64 nibble_to_write_count) { log_sized_hex_u64(num,      nibble_to_write_count); }
void log_sized_hex_u32(u32 num, u64 nibble_to_write_count) { log_sized_hex_u64(num,      nibble_to_write_count); }


void log_sized_hex_u64(u64 num, u64 nibble_to_write_count)
{
  u8* const num_str_start = logs.buffer + logs.buffer_end_idx;
  u8*       dest          = num_str_start + nibble_to_write_count;
  while (dest > num_str_start)
  {
    u8 nibble_idx = num & 0xF;
    
    dest -= 1;
    *dest = hex_digits[nibble_idx];
    
    num >>= 4;
  }

  logs.buffer_end_idx += nibble_to_write_count;
}


void log_sized_hex_f32(f32 num, u64 nibble_to_write_count) { log_sized_hex_u64(*(u32*)&num, nibble_to_write_count); }


void log_hex_s8 (s8  num) { log_sized_hex_u64((u64)num,    u32_nibble_count((u32)num));    }
void log_hex_s16(s16 num) { log_sized_hex_u64((u64)num,    u32_nibble_count((u32)num));    }
void log_hex_s32(s32 num) { log_sized_hex_u64((u64)num,    u32_nibble_count((u32)num));    }
void log_hex_s64(s64 num) { log_sized_hex_u64((u64)num,    u64_nibble_count((u64)num));    }
void log_hex_u8 (u8  num) { log_sized_hex_u64(num,         u32_nibble_count(num));         }
void log_hex_u16(u16 num) { log_sized_hex_u64(num,         u32_nibble_count(num));         }
void log_hex_u32(u32 num) { log_sized_hex_u64(num,         u32_nibble_count(num));         }
void log_hex_u64(u64 num) { log_sized_hex_u64(num,         u64_nibble_count(num));         }
void log_hex_f32(f32 num) { log_sized_hex_u64(*(u32*)&num, u32_nibble_count(*(u32*)&num)); }

#elif defined(_MSC_VER)
#  pragma warning(disable: 4206) // Disable "empty translation unit" warning
#endif // defined(LOGS_ENABLED) && (LOGS_ENABLED != 0)

