# High-Performance LZ77 Variants

This project explores the implementation of a high-performance LZ77 derivative. It incorporates modern architectural optimizations and novel adaptive heuristics to balance compression density against computational throughput. The custom LZ77 implementation is compared against LZ4 to evaluate the trade-offs inherent in dictionary-based compression. For full details on the implementation and performance evaluation, please refer to the [project report](./Report.pdf).

## Key Features

*   **LZSS-Style Bitstream Encoding:** Utilizes a 1-bit prefix to distinguish between literal bytes and match references, eliminating redundant fields and improving compression ratios. Uses variable-width bit packing.
*   **Search Acceleration with Hash Chains:** Employs a 24-bit hash (based on 3-byte prefixes) mapped to `head` and `prev` arrays forming a linked list to significantly reduce search complexity.
*   **Adaptive Heuristics (Dynamic Search Termination):** Implements an **Exponential Moving Average (EMA)** based "Good Enough" match threshold. This allows the compressor to "learn" local redundancy and adaptively terminate searches, boosting speed with minimal impact on compression ratio. Includes a Mean Absolute Deviation (MAD) filter for outlier rejection.
*   **Lazy Parsing Strategy:** Speculatively searches for longer matches at the next position before committing to a current match, which is critical for breaking local maxima.
*   **Comparative Analysis:** Benchmarked against the highly-optimized LZ4 compressor.

## Building the Project

### Requirements
*   CMake (minimum version 3.5)
*   A C++ compiler supporting the C++20 standard

### Build Instructions
Run the following commands in the root of the project directory:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build
```

This will produce the executable `csc2525` inside the `build` directory.

## Benchmarking Instructions

The project uses the *Silesia Corpus* and *enwik8* for evaluation. A script is provided to download these datasets.

1.  **Download Datasets:**
    Run the provided shell script from the project root to fetch the benchmark corpora.
    ```bash
    ./download_datasets.sh
    ```
    This will create a `datasets` directory and extract the files there.

2.  **Configuration:**
    The benchmarking parameters (datasets to use, LZ77 configurations, window sizes, heuristic parameters, etc.) are controlled via `config.json`. You can modify this file to test different configurations (e.g., toggling the adaptive heuristic or lazy parsing).

3.  **Run the Benchmark:**
    Execute the compiled program, passing the path to the configuration file. From the project root, run:
    ```bash
    ./build/csc2525 ./config.json
    ```
    *Note: The benchmarker automatically verifies the correctness of the compression/decompression cycle by comparing the decompressed output bit-for-bit against the original input file.*

4.  **Results:**
    After the execution completes, the benchmarking results (including compression ratios and throughput in MB/s) will be saved to `benchmark_results.csv` in the same directory as the config file.
