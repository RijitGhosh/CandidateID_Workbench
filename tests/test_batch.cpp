#include <gtest/gtest.h>

#include <atomic>
#include <thread>

#include "core/performance/batch.hpp"
#include "core/performance/benchmark.hpp"
#include "core/thermal/thermal.hpp"

using namespace aether::performance;

TEST(Cases, DeterministicForFixedSeedAndInRange) {
    const auto a = generateCases(500, 123), b = generateCases(500, 123), c = generateCases(500, 124);
    for (std::size_t i = 0; i < a.size(); ++i) {
        EXPECT_DOUBLE_EQ(a[i].conductivity_W_mK, b[i].conductivity_W_mK);
        EXPECT_DOUBLE_EQ(a[i].heatGeneration_W_m3, b[i].heatGeneration_W_m3);
        EXPECT_GE(a[i].conductivity_W_mK, 1.0);
        EXPECT_LE(a[i].conductivity_W_mK, 50.0);
        EXPECT_GE(a[i].heatGeneration_W_m3, 1e3);
        EXPECT_LE(a[i].heatGeneration_W_m3, 5e4);
    }
    EXPECT_NE(a[0].conductivity_W_mK, c[0].conductivity_W_mK);
}

TEST(Batch, SerialMatchesDirectSolverForEachCase) {
    const auto cases = generateCases(10, 7);
    const auto r = runSerial(cases, 50);
    ASSERT_EQ(r.status, RunStatus::Completed);
    for (std::size_t i = 0; i < cases.size(); ++i) {
        aether::thermal::RodParameters p;
        p.conductivity_W_mK = cases[i].conductivity_W_mK;
        p.heatGeneration_W_m3 = cases[i].heatGeneration_W_m3;
        p.elements = 50;
        const auto s = aether::thermal::solve(p);
        for (std::size_t j = 0; j < s.tFe.size(); ++j) EXPECT_DOUBLE_EQ(r.temperatures[i * 51 + j], s.tFe[j]) << "case " << i << " node " << j;
    }
}

TEST(Batch, ParallelAgreesWithSerialWithinSpec) {
    const auto cases = generateCases(300, 99);
    const auto s = runSerial(cases, 400);
    for (unsigned w : {1u, 2u, 3u, 4u, 8u}) {
        const auto p = runParallel(cases, 400, w);
        ASSERT_EQ(p.status, RunStatus::Completed) << w;
        EXPECT_LE(maxAbsDifference(s.temperatures, p.temperatures), 1e-8) << "workers requested=" << w;
        EXPECT_EQ(p.casesCompleted, cases.size());
    }
}

TEST(Batch, ResultOrderMatchesInputOrder) {
    auto cases = generateCases(64, 5);
    const auto fwd = runParallel(cases, 20, 4);
    std::vector<BatchCase> rev(cases.rbegin(), cases.rend());
    const auto bwd = runParallel(rev, 20, 4);
    const std::size_t stride = 21;
    for (std::size_t i = 0; i < cases.size(); ++i)
        for (std::size_t j = 0; j < stride; ++j)
            EXPECT_DOUBLE_EQ(fwd.temperatures[i * stride + j], bwd.temperatures[(cases.size() - 1 - i) * stride + j]);
}

TEST(Batch, WorkerCountIsClampedToLogicalCpusAndCases) {
    const unsigned hw = std::max(1u, std::thread::hardware_concurrency());
    EXPECT_EQ(effectiveWorkers(1000, 100000), hw);
    EXPECT_EQ(effectiveWorkers(0, 100), 1u);
    EXPECT_EQ(effectiveWorkers(hw, 1), 1u);
}

TEST(Batch, ProgressCountsEveryCase) {
    const auto cases = generateCases(50, 1);
    std::atomic<std::size_t> prog{0};
    RunOptions o;
    o.progress = &prog;
    runParallel(cases, 30, 4, o);
    EXPECT_EQ(prog.load(), 50u);
}

TEST(Batch, CancellationStopsEarlyAndIsReported) {
    const auto cases = generateCases(20000, 3);
    CancelToken tok;
    std::atomic<std::size_t> prog{0};
    RunOptions o;
    o.cancel = &tok;
    o.progress = &prog;
    std::thread canceller([&] {
        while (prog.load() < 5) std::this_thread::yield();
        tok.cancel();
    });
    const auto r = runParallel(cases, 1000, 4, o);
    canceller.join();
    EXPECT_EQ(r.status, RunStatus::Cancelled);
    EXPECT_LT(r.casesCompleted, cases.size());
    EXPECT_GE(r.casesCompleted, 5u);
}

TEST(Batch, PreCancelledSerialRunDoesNoWork) {
    CancelToken tok;
    tok.cancel();
    RunOptions o;
    o.cancel = &tok;
    const auto r = runSerial(generateCases(10, 1), 10, o);
    EXPECT_EQ(r.status, RunStatus::Cancelled);
    EXPECT_EQ(r.casesCompleted, 0u);
}

TEST(Batch, InvalidInputsThrow) {
    EXPECT_THROW(runSerial({}, 10), std::invalid_argument);
    EXPECT_THROW(runSerial(generateCases(5, 1), 0), std::invalid_argument);
    EXPECT_THROW(runSerial(generateCases(5, 1), 1'000'000), std::invalid_argument) << "exceeds mesh limit";
    EXPECT_THROW(runParallel({{-1.0, 5.0}}, 10, 2), std::invalid_argument);
    EXPECT_THROW(runParallel(generateCases(1000, 1), 50000, 2), std::invalid_argument) << "memory limit";
}

TEST(Benchmark, MedianHandlesOddAndEven) {
    EXPECT_DOUBLE_EQ(median({3, 1, 2}), 2.0);
    EXPECT_DOUBLE_EQ(median({4, 1, 3, 2}), 2.5);
    EXPECT_DOUBLE_EQ(median({}), 0.0);
}

TEST(Benchmark, SmallRunProducesConsistentReport) {
    BenchmarkConfig cfg;
    cfg.cases = 40;
    cfg.elements = 50;
    const auto rep = runBenchmark(cfg);
    ASSERT_EQ(rep.rows.size(), 4u);
    EXPECT_TRUE(rep.agreementOk);
    const unsigned logical = static_cast<unsigned>(std::max(1, rep.system.logicalCores));
    for (const auto& r : rep.rows) {
        if (r.workersRequested > logical) {
            EXPECT_FALSE(r.measured) << "workers above logical CPU count must not be measured";
        } else {
            EXPECT_TRUE(r.measured);
            EXPECT_EQ(r.timesSeconds.size(), 5u);
            EXPECT_GT(r.medianSeconds, 0.0);
            EXPECT_NEAR(r.efficiency, r.speedup / r.workersUsed, 1e-12);
        }
    }
    EXPECT_NEAR(rep.rows[0].speedup, 1.0, 1e-12) << "Speedup of the 1-worker row is T1/T1";
    EXPECT_FALSE(toMarkdown(rep).empty());
    EXPECT_NE(toJson(rep).find("\"rows\""), std::string::npos);
}
