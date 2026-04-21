#pragma once

#include <chrono>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>
#include "compressor.h"
#include "mapped_file.h"

struct BenchmarkResult {
    std::string algorithm;
    std::string dataset;
    size_t original_size;
    size_t compressed_size;
    double compression_time_ms;
    double decompression_time_ms;
};

class Benchmarker {
public:
    void add_test(const std::string &dataset_path) { datasets.push_back(dataset_path); }

    void run(Compressor &compressor, const std::string &algo_name) {
        std::cout << "\n=== Running " << algo_name << " ===" << std::endl;
        for (const auto &dataset: datasets) {
            std::cout << "Benchmarking " << std::filesystem::path(dataset).filename().string() << "... " << std::flush;
            std::string compressed_file = dataset + ".tmp.compressed";
            std::string decompressed_file = dataset + ".tmp.decompressed";

            auto start = std::chrono::high_resolution_clock::now();
            compressor.compress(dataset, compressed_file);
            auto end = std::chrono::high_resolution_clock::now();
            const double comp_time = std::chrono::duration<double, std::milli>(end - start).count();

            start = std::chrono::high_resolution_clock::now();
            compressor.decompress(compressed_file, decompressed_file);
            end = std::chrono::high_resolution_clock::now();
            const double decomp_time = std::chrono::duration<double, std::milli>(end - start).count();

            const size_t orig_size = std::filesystem::file_size(dataset);
            const size_t comp_size = std::filesystem::file_size(compressed_file);
            const size_t decomp_size = std::filesystem::file_size(decompressed_file);

            bool passed = true;
            if (orig_size != decomp_size) {
                passed = false;
            } else {
                MappedFile<mode::read> orig_map(dataset);
                MappedFile<mode::read> decomp_map(decompressed_file);
                if (orig_map.is_valid() && decomp_map.is_valid()) {
                    if (std::memcmp(orig_map.data(), decomp_map.data(), orig_size) != 0) {
                        passed = false;
                    }
                } else {
                    std::cerr << " [Map Error] ";
                    passed = false;
                }
            }

            const double ratio = static_cast<double>(orig_size) / comp_size;
            const double comp_speed = (orig_size / 1024.0 / 1024.0) / (comp_time / 1000.0);
            const double decomp_speed = (orig_size / 1024.0 / 1024.0) / (decomp_time / 1000.0);

            std::cout << (passed ? "[PASS] " : "[FAIL] ") << std::fixed << std::setprecision(2) << "Ratio: " << ratio
                      << "x | "
                      << "Comp: " << comp_speed << " MB/s | "
                      << "Decomp: " << decomp_speed << " MB/s\n";

            results.push_back({algo_name, dataset, orig_size, comp_size, comp_time, decomp_time});

            // Cleanup
            std::filesystem::remove(compressed_file);
            std::filesystem::remove(decompressed_file);
        }
    }

    void save_to_csv(const std::string &filename) const {
        std::ofstream out(filename);
        out << "Algorithm,Dataset,OriginalSize,CompressedSize,CompressionRatio,CompressionTimeMS,DecompressionTimeMS,"
               "CompSpeedMBs,DecompSpeedMBs\n";
        for (const auto &r: results) {
            const double ratio = static_cast<double>(r.original_size) / r.compressed_size;
            const double comp_speed = (r.original_size / 1024.0 / 1024.0) / (r.compression_time_ms / 1000.0);
            const double decomp_speed = (r.original_size / 1024.0 / 1024.0) / (r.decompression_time_ms / 1000.0);

            out << r.algorithm << "," << r.dataset << "," << r.original_size << "," << r.compressed_size << "," << ratio
                << "," << r.compression_time_ms << "," << r.decompression_time_ms << "," << comp_speed << ","
                << decomp_speed << "\n";
        }
    }

private:
    std::vector<std::string> datasets;
    std::vector<BenchmarkResult> results;
};
