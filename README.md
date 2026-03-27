# Nucleotide 2-bit encoding benchmarks & tests
Benchmarks and tests for [nucleotide_2bit_codec](https://github.com/badsami/nucleotide_2bit_codec).

## Compiling
All benchmarks and tests are currently Windows-specific.

1. Download or clone [nucleotide_2bit_codec](https://github.com/badsami/nucleotide_2bit_codec)
2. Download or clone this repository next to nucleotide_2bit_codec's directory
3. Make sure you have either Visual Studio Build Tools or Visual Studio's Native Desktop workload 2017 or greater installed
4. Open either a Windows terminal, or a x64 Native Tools Command Prompt for VS in nucleotide_2bit_codec_benchmarks_and_tests
5. Run `build.bat` to compile benchmarks and tests, for instance `build debug benchmark_decode`:
```
Usage: build [main] [build_type] [target_arch]

main, build_type and target_arch can appear in any order:

- main: the benchmark/test to compile. A program entry point (main) is picked based on its value
  - Valid values:  1 of: benchmark_encode benchmark_decode test_encode test_decode
  - Default value: benchmark_encode

- build_type: the type of build to perform
  - Valid values:  1 of: debug, dev, release
  - Default value: release

- target_arch: the target CPU architecture to build for
  - Valid values:  1 of: x64, arm64
  - Default value: the architecture of the CPU in use

"build" is the same as "build benchmark_decode release x64"
```

## Benchmarks
Benchmarks encode/decode 1.00 MiB-worth of unencoded/encoded bases 1024 times, on a single core.  
  
The numbers reported below are from benchmarks compiled with MSVC using compilers flags `-O2 -arch:AVX2` and with clang using compiler flags `-O2 -mavx2 -mbmi2`, and run on an AMD Zen 3 5800H CPU (released January 2021) on Windows. For all 4 benchmark scenarii, measurements were within ±5% of each others. So the results reported below apply benchmarks compiled with both MSVC and clang.  
   
The benchmarks were ran 10 times each, without any open application running in the background. The best of the 10 runs are reported below.

#### Encoding
- CPU clock boost disabled
  Type   | Minimum bandwidth | Maximum bandwidth | Average bandwidth 
  -------|-------------------|-------------------|------------------
  Scalar |        3.43 GiB/s |        4.32 GiB/s |    **4.26 GiB/s**
  BMI2   |        7.71 GiB/s |       15.09 GiB/s |   **14.54 GiB/s**
  SSE4.1 |        9.10 GiB/s |       31.00 GiB/s |   **29.67 GiB/s**
  AVX2   |       13.37 GiB/s |       38.44 GiB/s |   **36.78 GiB/s**

- CPU clock boost enabled
  Type   | Minimum bandwidth | Maximum bandwidth | Average bandwidth
  -------|-------------------|-------------------|------------------
  Scalar |        4.81 GiB/s |        6.05 GiB/s |    **5.94 GiB/s**
  BMI2   |        9.29 GiB/s |       20.86 GiB/s |   **19.84 GiB/s**
  SSE4.1 |       15.62 GiB/s |       42.64 GiB/s |   **39.98 GiB/s**
  AVX2   |       17.25 GiB/s |       53.65 GiB/s |   **49.65 GiB/s**

#### Decoding
- CPU clock boost disabled
  Type   | Minimum bandwidth | Maximum bandwidth | Average bandwidth
  -------|-------------------|-------------------|------------------
  Scalar |        9.72 GiB/s |       13.45 GiB/s |   **13.30 GiB/s**
  SSE4.1 |       13.16 GiB/s |       22.24 GiB/s |   **21.87 GiB/s**
  AVX2   |       19.14 GiB/s |       41.37 GiB/s |   **40.16 GiB/s**

- CPU clock boost enabled
  Type   | Minimum bandwidth | Maximum bandwidth | Average bandwidth
  -------|-------------------|-------------------|------------------
  Scalar |       10.26 GiB/s |       18.74 GiB/s |   **17.24 GiB/s**
  SSE4.1 |       16.41 GiB/s |       30.14 GiB/s |   **28.75 GiB/s**
  AVX2   |       21.51 GiB/s |       53.65 GiB/s |   **51.42 GiB/s**
