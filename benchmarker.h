#pragma once

#include <chrono>
#include <fstream>
#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include "compressor.h"

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
    void add_test(const std::string& dataset_path) {
        datasets.push_back(dataset_path);
    }

    void run(Compressor& compressor, const std::string& algo_name) {
        for (const auto& dataset : datasets) {
            std::string compressed_file = dataset + ".tmp.compressed";
            std::string decompressed_file = dataset + ".tmp.decompressed";

            auto start = std::chrono::high_resolution_clock::now();
            compressor.compress(dataset, compressed_file);
            auto end = std::chrono::high_resolution_clock::now();
            double comp_time = std::chrono::duration<double, std::milli>(end - start).count();

            start = std::chrono::high_resolution_clock::now();
            compressor.decompress(compressed_file, decompressed_file);
            end = std::chrono::high_resolution_clock::now();
            double decomp_time = std::chrono::duration<double, std::milli>(end - start).count();

            size_t orig_size = std::filesystem::file_size(dataset);
            size_t comp_size = std::filesystem::file_size(compressed_file);

            results.push_back({algo_name, dataset, orig_size, comp_size, comp_time, decomp_time});

            // Cleanup
            std::filesystem::remove(compressed_file);
            std::filesystem::remove(decompressed_file);
        }
    }

    void save_to_csv(const std::string& filename) {
        std::ofstream out(filename);
        out << "Algorithm,Dataset,OriginalSize,CompressedSize,CompressionRatio,CompressionTimeMS,DecompressionTimeMS,CompSpeedMBs,DecompSpeedMBs\n";
        for (const auto& r : results) {
            double ratio = static_cast<double>(r.original_size) / r.compressed_size;
            double comp_speed = (r.original_size / 1024.0 / 1024.0) / (r.compression_time_ms / 1000.0);
            double decomp_speed = (r.original_size / 1024.0 / 1024.0) / (r.decompression_time_ms / 1000.0);
            
            out << r.algorithm << "," << r.dataset << "," << r.original_size << "," << r.compressed_size << ","
                << ratio << "," << r.compression_time_ms << "," << r.decompression_time_ms << ","
                << comp_speed << "," << decomp_speed << "\n";
        }
    }

private:
    std::vector<std::string> datasets;
    std::vector<BenchmarkResult> results;
};
