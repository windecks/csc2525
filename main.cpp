#include <filesystem>
#include <fstream>
#include <iostream>
#include <vector>
#include "benchmarker.h"
#include "json.hpp"
#include "lz4.h"
#include "lz77.h"

using json = nlohmann::json;

int main(int argc, char *argv[]) {
    std::string config_path = "./config.json";
    if (argc > 1) {
        config_path = argv[1];
    }

    std::ifstream f(config_path);
    if (!f.is_open()) {
        std::cerr << "Config not found: " << config_path << std::endl;
        return 1;
    }

    json config = json::parse(f);

    std::filesystem::path config_dir = std::filesystem::path(config_path).parent_path();

    Benchmarker benchmarker;
    for (const auto &ds: config["datasets"]) {
        std::filesystem::path ds_path = ds.get<std::string>();
        if (ds_path.is_relative()) {
            ds_path = config_dir / ds_path;
        }
        if (std::filesystem::exists(ds_path)) {
            benchmarker.add_test(ds_path.string());
        }
    }

    // Run LZ4
    LZ4 lz4;
    benchmarker.run(lz4, "LZ4");

    // Run LZ77 with various configs
    for (auto &lz77_cfg: config["lz77_configs"]) {
        LZ77Config cfg;
        if (lz77_cfg.contains("search_buffer"))
            cfg.search_buffer_size = lz77_cfg["search_buffer"];
        if (lz77_cfg.contains("lookahead_buffer"))
            cfg.lookahead_buffer_size = lz77_cfg["lookahead_buffer"];
        if (lz77_cfg.contains("max_chain_attempts"))
            cfg.max_chain_attempts = lz77_cfg["max_chain_attempts"];
        if (lz77_cfg.contains("decay_rate"))
            cfg.decay_rate = lz77_cfg["decay_rate"];
        if (lz77_cfg.contains("good_enough_multiplier"))
            cfg.good_enough_multiplier = lz77_cfg["good_enough_multiplier"];
        if (lz77_cfg.contains("outlier_k"))
            cfg.outlier_k = lz77_cfg["outlier_k"];
        if (lz77_cfg.contains("lazy_parsing"))
            cfg.lazy_parsing = lz77_cfg["lazy_parsing"];

        LZ77 lz77(cfg);
        std::string name =
                "LZ77_s" + std::to_string(cfg.search_buffer_size) + "_l" + std::to_string(cfg.lookahead_buffer_size);
        benchmarker.run(lz77, name);
    }

    auto csv_path = (config_dir / "benchmark_results.csv").string();
    benchmarker.save_to_csv(csv_path);
    std::cout << "Benchmarking complete. Results saved to " << csv_path << std::endl;

    return 0;
}
