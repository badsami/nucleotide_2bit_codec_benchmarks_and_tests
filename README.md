# Nucleotide 2-bit encoding benchmarks & tests
Benchmarks and tests for [nucleotide_2bit_codec](https://github.com/badsami/nucleotide_2bit_codec).

## Compiling
All benchmarks and tests are currently Windows-specific.

1. Download or clone [nucleotide_2bit_codec](https://github.com/badsami/nucleotide_2bit_codec)
2. Download or clone this repository next to nucleotide_2bit_codec's directory
3. Make sure you have either Visual Studio Build Tools or Visual Studio's Native Desktop workload 2017 or greater installed
4. Open either a Windows terminal, or a x64 Native Tools Command Prompt for VS in nucleotide_2bit_codec_benchmarks_and_tests
5. Use `build.bat` to compile and run benchmarks and tests, for instance `build debug benchmark_decode`:
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