// Aether core: batches of independent 1-D thermal solves, run serially or on a bounded worker pool.
#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <vector>

namespace aether::performance {

constexpr std::size_t kMaxCases = 1'000'000;
constexpr std::size_t kMaxResultDoubles = 25'000'000;  // ~200 MB of stored temperatures

struct BatchCase {
    double conductivity_W_mK;
    double heatGeneration_W_m3;
};

// Deterministic: k in [1, 50] W/(m K), q in [1e3, 5e4] W/m^3 from a fixed-seed mt19937_64.
std::vector<BatchCase> generateCases(std::size_t count, std::uint64_t seed);

struct CancelToken {
    std::atomic<bool> cancelled{false};
    void cancel() { cancelled.store(true, std::memory_order_relaxed); }
    bool isCancelled() const { return cancelled.load(std::memory_order_relaxed); }
};

enum class RunStatus { Completed, Cancelled };

struct BatchResult {
    RunStatus status = RunStatus::Completed;
    std::size_t casesCompleted = 0;
    std::size_t casesTotal = 0;
    int elements = 0;
    unsigned workersUsed = 0;
    double seconds = 0.0;          // compute only (includes thread start/join for parallel runs)
    std::vector<double> temperatures;  // casesTotal * (elements+1), case-major, input order
};

struct RunOptions {
    CancelToken* cancel = nullptr;
    std::atomic<std::size_t>* progress = nullptr;  // incremented once per finished case
};

// Number of workers actually used: clamp(requested, 1, logical CPUs) and never more than the case count.
unsigned effectiveWorkers(unsigned requested, std::size_t cases);

BatchResult runSerial(const std::vector<BatchCase>& cases, int elements, const RunOptions& opt = {});
BatchResult runParallel(const std::vector<BatchCase>& cases, int elements, unsigned workers, const RunOptions& opt = {});

double maxAbsDifference(const std::vector<double>& a, const std::vector<double>& b);

}  // namespace aether::performance
