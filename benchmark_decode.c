#include "logs/logs.h"
#include "../nucleotide_2bit_codec/decoded_tables.h"
#include "../nucleotide_2bit_codec/nucleotide_2bit_codec.h"
#include "../nucleotide_2bit_codec/types.h"

// Win32 external references
extern void* VirtualAlloc(void* lpAddress, u64 dwSize, u32 flAllocationType, u32 flProtect);
extern u32 QueryPerformanceFrequency(u64* frequency);
extern u32 QueryPerformanceCounter(u64* counter);
extern void ExitProcess(u32 exit_code);
#define MEM_RESERVE             0x00002000
#define MEM_COMMIT              0x00001000
#define PAGE_READWRITE          0x00000004

s32 _fltused;

#define BENCHMARK_BYTE_COUNT (1 << 20)
#define ITERATION_COUNT      ((1ull << 30) / BENCHMARK_BYTE_COUNT)

typedef void (*decode_func)(const u8* restrict encoded_bases,
                            char*     restrict decoded_bases,
                            u64                base_count,
                            const char         lookup_table[256 * 4]);

typedef struct EncodeFunction EncodeFunction;
struct EncodeFunction
{
  const char*       name;
  const decode_func ptr;
};

void program_start(void)
{
  const EncodeFunction functions[] =
  {
    {.name = "Scalar", .ptr = decode_bases_scalar},
    {.name = "SSE4.1", .ptr = decode_bases_sse4_1},
    {.name = "AVX2  ", .ptr = decode_bases_avx2},
  };

  logs_open_console_output();

  // Allocate
  const u64 base_count         = decoded_byte_count_to_base_count(BENCHMARK_BYTE_COUNT);
  const u64 decoded_byte_count = base_count_to_decoded_byte_count(base_count);
  u8*   const encoded = VirtualAlloc(0, BENCHMARK_BYTE_COUNT, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
  char* const decoded = VirtualAlloc(0, decoded_byte_count, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);

  log_literal_str("Benchmarking decoding ");
  log_byte_count_bin_unit(BENCHMARK_BYTE_COUNT);
  log_literal_str("-worth of encoded bases ");
  log_dec_u64(ITERATION_COUNT);
  log_literal_str(" times\n\n       Minimum, maximum, average bandwidth\n");
  logs_flush();

  // Warm-up
  const u64 func_count = sizeof(functions) / sizeof(functions[0]);
  functions[func_count - 1].ptr(encoded, decoded, base_count, decoded_ACGT);

  u64 frequency;
  QueryPerformanceFrequency(&frequency);

  // Benchmark
  for (u64 func_idx = 0; func_idx < func_count; func_idx += 1)
  {
    u64 min_ticks = ~0ull;
    u64 max_ticks = 0;
    u64 all_ticks = 0;
    
    const decode_func target_func = functions[func_idx].ptr;
    for (u64 i = 0; i < ITERATION_COUNT; i += 1)
    {
      u64 t0; QueryPerformanceCounter(&t0);
      target_func(encoded, decoded, base_count, decoded_ACGT);
      u64 t1; QueryPerformanceCounter(&t1);

      const u64 elapsed = t1 - t0;
      min_ticks = (elapsed < min_ticks) ? elapsed : min_ticks;
      max_ticks = (elapsed > max_ticks) ? elapsed : max_ticks;
      all_ticks += elapsed;
    }

    min_ticks = (min_ticks != 0) ? min_ticks : 1;

    const u64 numerator     = BENCHMARK_BYTE_COUNT * frequency;
    const u64 min_bandwidth = numerator / max_ticks;
    const u64 max_bandwidth = numerator / min_ticks;
    const u64 avg_bandwidth = (ITERATION_COUNT * numerator) / all_ticks;
    log_null_terminated_str(functions[func_idx].name);
    log_character(' ');
    log_byte_count_bin_unit(min_bandwidth);
    log_literal_str("/s, ");
    log_byte_count_bin_unit(max_bandwidth);
    log_literal_str("/s, ");
    log_byte_count_bin_unit(avg_bandwidth);
    log_literal_str("/s\n");
    logs_flush();
  }
  
  logs_close_console_output();
  ExitProcess(0);
}