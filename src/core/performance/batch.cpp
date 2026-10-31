#include "batch.hpp"

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <mutex>
#include <random>
#include <stdexcept>
#include <thread>

#include "../thermal/thermal.hpp"

namespace aether::performance {

std::vector<BatchCase> generateCases(std::size_t count, std::uint64_t seed) {
    if (count > kMaxCases) throw std::invalid_argument("case count exceeds limit");
    std::mt19937_64 rng(seed);
    auto unit = [&rng] { return static_cast<double>(rng() >> 11) * (1.0 / 9007199254740992.0); };  // platform independent
    std::vector<BatchCase> v(count);
    for (auto& c : v) {
        c.conductivity_W_mK = 1.0 + 49.0 * unit();
        c.heatGeneration_W_m3 = 1.0e3 + 4.9e4 * unit();
    }
    return v;
}

unsigned effectiveWorkers(unsigned requested, std::size_t cases) {
    unsigned hw = std::thread::hardware_concurrency();
    if (hw == 0) hw = 1;
    unsigned n = std::max(1u, std::min(requested, hw));
    if (cases > 0 && n > cases) n = static_cast<unsigned>(cases);
    return n;
}

namespace {

void checkInputs(const std::vector<BatchCase>& cases, int elements) {
    if (cases.empty()) throw std::invalid_argument("case list is empty");
    if (cases.size() > kMaxCases) throw std::invalid_argument("case count exceeds limit");
    if (elements < 1 || elements > 100000) throw std::invalid_argument("mesh size must be in [1, 100000] elements");
    if (cases.size() * (static_cast<std::size_t>(elements) + 1) > kMaxResultDoubles)
        throw std::invalid_argument("cases x (elements+1) exceeds the 25M-value memory limit; reduce cases or mesh size");
    for (const auto& c : cases)
        if (!(c.conductivity_W_mK > 0.0) || !std::isfinite(c.conductivity_W_mK) || !std::isfinite(c.heatGeneration_W_m3))
            throw std::invalid_argument("every case needs finite, positive conductivity and finite heat generation");
}

// The ONE per-case routine used by both execution paths.
void solveCase(const BatchCase& c, int elements, thermal::Scratch& scratch, std::vector<double>& tmp, double* out) {
    thermal::RodParameters p;  // L=1 m, A=1e-4 m^2, T(0)=100 C, T(L)=20 C
    p.conductivity_W_mK = c.conductivity_W_mK;
    p.heatGeneration_W_m3 = c.heatGeneration_W_m3;
    p.elements = elements;
    thermal::solveInto(p, scratch, tmp);
    std::copy(tmp.begin(), tmp.end(), out);
}

using Clock = std::chrono::steady_clock;

}  // namespace

BatchResult runSerial(const std::vector<BatchCase>& cases, int elements, const RunOptions& opt) {
    checkInputs(cases, elements);
    BatchResult r;
    r.casesTotal = cases.size();
    r.elements = elements;
    r.workersUsed = 1;
    const std::size_t stride = static_cast<std::size_t>(elements) + 1;
    r.temperatures.assign(cases.size() * stride, 0.0);
    thermal::Scratch scratch;
    std::vector<double> tmp;
    const auto t0 = Clock::now();
    for (std::size_t i = 0; i < cases.size(); ++i) {
        if (opt.cancel && opt.cancel->isCancelled()) { r.status = RunStatus::Cancelled; break; }
        solveCase(cases[i], elements, scratch, tmp, &r.temperatures[i * stride]);
        ++r.casesCompleted;
        if (opt.progress) opt.progress->fetch_add(1, std::memory_order_relaxed);
    }
    r.seconds = std::chrono::duration<double>(Clock::now() - t0).count();
    return r;
}

BatchResult runParallel(const std::vector<BatchCase>& cases, int elements, unsigned workers, const RunOptions& opt) {
    checkInputs(cases, elements);
    BatchResult r;
    r.casesTotal = cases.size();
    r.elements = elements;
    r.workersUsed = effectiveWorkers(workers, cases.size());
    const std::size_t stride = static_cast<std::size_t>(elements) + 1;
    r.temperatures.assign(cases.size() * stride, 0.0);

    std::atomic<std::size_t> next{0};
    std::atomic<std::size_t> done{0};
    std::atomic<bool> failed{false};
    std::exception_ptr firstError;
    std::mutex errMutex;
    const std::size_t n = cases.size();
    double* base = r.temperatures.data();

    auto worker = [&] {
        thermal::Scratch scratch;  // thread-private
        std::vector<double> tmp;
        try {
            for (;;) {
                if (failed.load(std::memory_order_relaxed) || (opt.cancel && opt.cancel->isCancelled())) return;
                const std::size_t i = next.fetch_add(1, std::memory_order_relaxed);
                if (i >= n) return;
                solveCase(cases[i], elements, scratch, tmp, base + i * stride);  // each case owns a disjoint slice
                done.fetch_add(1, std::memory_order_relaxed);
                if (opt.progress) opt.progress->fetch_add(1, std::memory_order_relaxed);
            }
        } catch (...) {
            std::lock_guard<std::mutex> lk(errMutex);
            if (!firstError) firstError = std::current_exception();
            failed.store(true);
        }
    };

    const auto t0 = Clock::now();
    {
        std::vector<std::thread> pool;
        pool.reserve(r.workersUsed - 1);
        for (unsigned w = 1; w < r.workersUsed; ++w) pool.emplace_back(worker);
        worker();  // calling thread is worker 0
        for (auto& t : pool) t.join();
    }
    r.seconds = std::chrono::duration<double>(Clock::now() - t0).count();
    if (firstError) std::rethrow_exception(firstError);
    r.casesCompleted = done.load();
    if (r.casesCompleted < n) r.status = RunStatus::Cancelled;
    return r;
}

double maxAbsDifference(const std::vector<double>& a, const std::vector<double>& b) {
    if (a.size() != b.size()) throw std::invalid_argument("result sizes differ");
    double m = 0.0;
    for (std::size_t i = 0; i < a.size(); ++i) m = std::max(m, std::fabs(a[i] - b[i]));
    return m;
}

}  // namespace aether::performance
