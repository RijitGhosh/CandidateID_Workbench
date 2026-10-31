// Aether core: system information and the serial-vs-parallel benchmark.
#pragma once

#include <functional>
#include <string>
#include <vector>

#include "batch.hpp"

namespace aether::performance {

struct SystemInfo {
    std::string cpuModel;
    int physicalCores = 0;   // 0 = could not be determined
    int logicalCores = 0;
    double ramGiB = 0.0;     // 0 = could not be determined
    std::string os;
    std::string compiler;
    std::string compilerVersion;
    std::string buildType;
    std::string optimisationFlags;
};
SystemInfo querySystemInfo();

struct BenchmarkConfig {
    std::size_t cases = 2000;
    int elements = 2000;
    std::uint64_t seed = 20260101;
    int warmups = 1;
    int repetitions = 5;
    std::vector<unsigned> workerCounts = {1, 2, 3, 4};
};

struct BenchmarkRow {
    unsigned workersRequested = 0;
    unsigned workersUsed = 0;
    bool measured = false;
    std::string note;                // e.g. "skipped: exceeds logical CPU count"
    std::vector<double> timesSeconds;
    double medianSeconds = 0.0;
    double speedup = 0.0;            // T1 / Tp
    double efficiency = 0.0;         // speedup / p
    double maxAbsDiffVsSerial_C = 0.0;
};

struct BenchmarkReport {
    SystemInfo system;
    BenchmarkConfig config;
    std::vector<BenchmarkRow> rows;
    double serialReferenceSeconds = 0.0;
    bool speedupAssessable = false;  // false when only one logical CPU
    bool agreementOk = false;        // all rows within 1e-8 C of the serial reference
    bool cancelled = false;
};

double median(std::vector<double> v);

// progress(fraction in [0,1], description). Compute-only timing; no file I/O or UI work inside timed regions.
BenchmarkReport runBenchmark(const BenchmarkConfig& cfg, CancelToken* cancel = nullptr,
                             const std::function<void(double, const std::string&)>& progress = {});

std::string toMarkdown(const BenchmarkReport& r);
std::string toJson(const BenchmarkReport& r);

}  // namespace aether::performance
