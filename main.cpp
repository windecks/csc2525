#include <iostream>
#include <fstream>
#include <vector>
#include "json.hpp"
#include "lz77.h"
#include "lz4.h"
#include "benchmarker.h"

using json = nlohmann::json;

int main(int argc, char* argv[]) {
    std::string config_path = "config.json";
    if (argc > 1) {
        config_path = argv[1];
    }

    std::ifstream f(config_path);
    if (!f.is_open()) {
        std::cerr << "Config not found: " << config_path << std::endl;
        return 1;
    }

    json config = json::parse(f);

    Benchmarker benchmarker;
    for (auto& ds : config["datasets"]) {
        benchmarker.add_test(ds.get<std::string>());
    }

    // Run LZ4
    LZ4 lz4;
    benchmarker.run(lz4, "LZ4");

    // Run LZ77 with various configs
    for (auto& lz77_cfg : config["lz77_configs"]) {
        size_t search = lz77_cfg["search_buffer"];
        size_t lookahead = lz77_cfg["lookahead_buffer"];
        LZ77 lz77(search, lookahead);
        std::string name = "LZ77_s" + std::to_string(search) + "_l" + std::to_string(lookahead);
        benchmarker.run(lz77, name);
    }

    benchmarker.save_to_csv("benchmark_results.csv");
    std::cout << "Benchmarking complete. Results saved to benchmark_results.csv" << std::endl;

    return 0;
}
