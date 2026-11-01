#include "benchmark.hpp"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <thread>

#if defined(_WIN32)
#define WIN32_LEAN_AND_MEAN
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <unistd.h>
#if defined(__APPLE__)
#include <sys/sysctl.h>
#include <sys/types.h>
#endif
#endif
#include <cstdlib>
#include <cstring>
#if defined(__x86_64__) || defined(_M_X64) || defined(__i386__) || defined(_M_IX86)
#if defined(_MSC_VER)
#include <intrin.h>
#else
#include <cpuid.h>
#endif
#define AETHER_X86 1
#endif

#ifndef AETHER_BUILD_TYPE
#define AETHER_BUILD_TYPE "unknown"
#endif
#ifndef AETHER_CXX_FLAGS
#define AETHER_CXX_FLAGS "unknown"
#endif

namespace aether::performance {

namespace {

std::string cpuBrandFromCpuid() {
#ifdef AETHER_X86
    unsigned regs[4] = {0, 0, 0, 0};
    char brand[49] = {0};
#if defined(_MSC_VER)
    int r[4];
    __cpuid(r, 0x80000000);
    if (static_cast<unsigned>(r[0]) < 0x80000004u) return {};
    for (unsigned i = 0; i < 3; ++i) {
        __cpuid(r, 0x80000002 + i);
        std::copy_n(reinterpret_cast<char*>(r), 16, brand + 16 * i);
    }
#else
    if (__get_cpuid_max(0x80000000, nullptr) < 0x80000004u) return {};
    for (unsigned i = 0; i < 3; ++i) {
        __cpuid(0x80000002 + i, regs[0], regs[1], regs[2], regs[3]);
        std::copy_n(reinterpret_cast<char*>(regs), 16, brand + 16 * i);
    }
#endif
    std::string s(brand);
    const auto b = s.find_first_not_of(' ');
    return b == std::string::npos ? std::string{} : s.substr(b);
#else
    return {};
#endif
}

}  // namespace

SystemInfo querySystemInfo() {
    SystemInfo s;
    s.logicalCores = static_cast<int>(std::thread::hardware_concurrency());
    s.cpuModel = cpuBrandFromCpuid();
#if defined(_WIN32)
    s.os = "Windows";
    MEMORYSTATUSEX m;
    m.dwLength = sizeof(m);
    if (GlobalMemoryStatusEx(&m)) s.ramGiB = static_cast<double>(m.ullTotalPhys) / (1024.0 * 1024.0 * 1024.0);
    DWORD len = 0;
    GetLogicalProcessorInformation(nullptr, &len);
    if (len > 0) {
        std::vector<SYSTEM_LOGICAL_PROCESSOR_INFORMATION> buf(len / sizeof(SYSTEM_LOGICAL_PROCESSOR_INFORMATION));
        if (GetLogicalProcessorInformation(buf.data(), &len)) {
            int cores = 0;
            for (const auto& e : buf) if (e.Relationship == RelationProcessorCore) ++cores;
            s.physicalCores = cores;
        }
    }
#elif defined(__APPLE__)
    s.os = "macOS";
    auto sysStr = [](const char* name) {
        std::size_t n = 0;
        if (sysctlbyname(name, nullptr, &n, nullptr, 0) != 0 || n == 0) return std::string();
        std::string v(n, '\0');
        if (sysctlbyname(name, v.data(), &n, nullptr, 0) != 0) return std::string();
        v.resize(std::strlen(v.c_str()));
        return v;
    };
    auto sysInt = [](const char* name) -> long long {
        long long v = 0;
        std::size_t n = sizeof(v);
        if (sysctlbyname(name, &v, &n, nullptr, 0) != 0) return 0;
        return v;
    };
    const std::string brand = sysStr("machdep.cpu.brand_string");
    if (!brand.empty()) s.cpuModel = brand;
    s.physicalCores = static_cast<int>(sysInt("hw.physicalcpu"));
    const long long mem = sysInt("hw.memsize");
    if (mem > 0) s.ramGiB = static_cast<double>(mem) / (1024.0 * 1024.0 * 1024.0);
#else
    s.os = "Linux";
    const long pages = sysconf(_SC_PHYS_PAGES), psz = sysconf(_SC_PAGE_SIZE);
    if (pages > 0 && psz > 0) s.ramGiB = static_cast<double>(pages) * static_cast<double>(psz) / (1024.0 * 1024.0 * 1024.0);
    std::ifstream f("/proc/cpuinfo");
    std::string line;
    int cpuCores = 0;
    while (std::getline(f, line)) {
        if (s.cpuModel.empty() && line.rfind("model name", 0) == 0) {
            const auto p = line.find(':');
            if (p != std::string::npos) s.cpuModel = line.substr(p + 2);
        }
        if (line.rfind("cpu cores", 0) == 0 && cpuCores == 0) {
            const auto p = line.find(':');
            if (p != std::string::npos) cpuCores = std::atoi(line.c_str() + p + 1);
        }
    }
    s.physicalCores = cpuCores;  // 0 if not reported (e.g. some virtual machines)
#endif
    if (s.cpuModel.empty()) s.cpuModel = "unknown";
#if defined(__clang__)
    s.compiler = "Clang";
    s.compilerVersion = __clang_version__;
#elif defined(__GNUC__)
    s.compiler = "GCC";
    s.compilerVersion = __VERSION__;
#elif defined(_MSC_VER)
    s.compiler = "MSVC";
    s.compilerVersion = std::to_string(_MSC_VER);
#else
    s.compiler = "unknown";
#endif
    s.buildType = AETHER_BUILD_TYPE;
    s.optimisationFlags = AETHER_CXX_FLAGS;
    return s;
}

double median(std::vector<double> v) {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    const std::size_t n = v.size();
    return n % 2 ? v[n / 2] : 0.5 * (v[n / 2 - 1] + v[n / 2]);
}

BenchmarkReport runBenchmark(const BenchmarkConfig& cfg, CancelToken* cancel,
                             const std::function<void(double, const std::string&)>& progress) {
    BenchmarkReport rep;
    rep.system = querySystemInfo();
    rep.config = cfg;
    const unsigned logical = static_cast<unsigned>(std::max(1, rep.system.logicalCores));
    rep.speedupAssessable = logical > 1;

    const auto cases = generateCases(cfg.cases, cfg.seed);  // generated outside every timed region
    RunOptions ro;
    ro.cancel = cancel;

    // Serial reference result (identical cases, identical numerical routine) for the agreement check.
    if (progress) progress(0.0, "serial reference run");
    BatchResult ref = runSerial(cases, cfg.elements, ro);
    if (ref.status == RunStatus::Cancelled) { rep.cancelled = true; return rep; }

    const int totalRuns = static_cast<int>(cfg.workerCounts.size()) * (cfg.warmups + cfg.repetitions);
    int doneRuns = 0;
    rep.agreementOk = true;
    for (unsigned p : cfg.workerCounts) {
        BenchmarkRow row;
        row.workersRequested = p;
        if (p > logical) {
            row.note = "not measured: " + std::to_string(p) + " workers exceeds " + std::to_string(logical) + " logical CPU(s)";
            rep.rows.push_back(row);
            doneRuns += cfg.warmups + cfg.repetitions;
            continue;
        }
        row.workersUsed = effectiveWorkers(p, cases.size());
        auto once = [&](BatchResult& out) {
            out = (p == 1) ? runSerial(cases, cfg.elements, ro) : runParallel(cases, cfg.elements, p, ro);
        };
        BatchResult res;
        for (int w = 0; w < cfg.warmups; ++w) {
            once(res);
            if (res.status == RunStatus::Cancelled) { rep.cancelled = true; return rep; }
            if (progress) progress(static_cast<double>(++doneRuns) / totalRuns, "warm-up, " + std::to_string(p) + " worker(s)");
        }
        for (int i = 0; i < cfg.repetitions; ++i) {
            once(res);
            if (res.status == RunStatus::Cancelled) { rep.cancelled = true; return rep; }
            row.timesSeconds.push_back(res.seconds);
            if (progress) progress(static_cast<double>(++doneRuns) / totalRuns,
                                   "repetition " + std::to_string(i + 1) + ", " + std::to_string(p) + " worker(s)");
        }
        row.measured = true;
        row.medianSeconds = median(row.timesSeconds);
        row.maxAbsDiffVsSerial_C = maxAbsDifference(ref.temperatures, res.temperatures);
        if (!(row.maxAbsDiffVsSerial_C <= 1e-8)) rep.agreementOk = false;
        rep.rows.push_back(row);
    }

    // T1 = median of the 1-worker row
    double t1 = 0.0;
    for (const auto& r : rep.rows) if (r.measured && r.workersRequested == 1) t1 = r.medianSeconds;
    rep.serialReferenceSeconds = t1;
    for (auto& r : rep.rows) {
        if (!r.measured || t1 <= 0.0) continue;
        r.speedup = t1 / r.medianSeconds;
        r.efficiency = r.speedup / static_cast<double>(r.workersUsed);
    }
    return rep;
}

std::string toMarkdown(const BenchmarkReport& r) {
    std::ostringstream o;
    o << std::fixed;
    o << "# Benchmark report (measured on this machine)\n\n";
    o << "| Item | Value |\n|---|---|\n";
    o << "| CPU model | " << r.system.cpuModel << " |\n";
    o << "| Physical cores | " << (r.system.physicalCores ? std::to_string(r.system.physicalCores) : "not reported by OS") << " |\n";
    o << "| Logical cores | " << r.system.logicalCores << " |\n";
    o << "| RAM | " << std::setprecision(2) << r.system.ramGiB << " GiB |\n";
    o << "| OS | " << r.system.os << " |\n";
    o << "| Compiler | " << r.system.compiler << " " << r.system.compilerVersion << " |\n";
    o << "| Build type / flags | " << r.system.buildType << " / `" << r.system.optimisationFlags << "` |\n";
    o << "| Workload | independent 1-D thermal FE solves (L=1 m, A=1e-4 m^2, T(0)=100 C, T(L)=20 C) |\n";
    o << "| Case count | " << r.config.cases << " |\n";
    o << "| Mesh size | " << r.config.elements << " elements |\n";
    o << "| Random seed | " << r.config.seed << " (k in [1,50], q in [1e3,5e4]) |\n";
    o << "| Warm-up policy | " << r.config.warmups << " untimed run per worker count |\n";
    o << "| Repetitions | " << r.config.repetitions << " timed runs; median reported |\n";
    o << "| Timed region | compute only (excludes file I/O, rendering, compilation, UI); includes thread start/join |\n\n";
    o << "| Workers | Median time [s] | Speedup T1/Tp | Efficiency | max abs diff vs serial [C] | Note |\n|---|---|---|---|---|---|\n";
    for (const auto& row : r.rows) {
        o << "| " << row.workersRequested << " | ";
        if (row.measured) {
            o << std::setprecision(6) << row.medianSeconds << " | " << std::setprecision(3) << row.speedup << " | " << row.efficiency
              << " | " << std::scientific << std::setprecision(2) << row.maxAbsDiffVsSerial_C << std::fixed << " | |\n";
        } else {
            o << "- | - | - | - | " << row.note << " |\n";
        }
    }
    o << "\nAgreement check (<= 1e-8 C): " << (r.agreementOk ? "PASS" : "FAIL") << "\n";
    if (!r.speedupAssessable) o << "\nOnly one logical CPU is available: the parallel path ran, but speedup cannot be assessed.\n";
    return o.str();
}

std::string toJson(const BenchmarkReport& r) {
    std::ostringstream o;
    o << std::setprecision(12);
    auto esc = [](const std::string& s) {
        std::string e;
        for (char c : s) { if (c == '"' || c == '\\') e += '\\'; e += c; }
        return e;
    };
    o << "{\n  \"cpu_model\": \"" << esc(r.system.cpuModel) << "\",\n  \"physical_cores\": " << r.system.physicalCores
      << ",\n  \"logical_cores\": " << r.system.logicalCores << ",\n  \"ram_gib\": " << r.system.ramGiB
      << ",\n  \"os\": \"" << r.system.os << "\",\n  \"compiler\": \"" << esc(r.system.compiler + " " + r.system.compilerVersion)
      << "\",\n  \"build_type\": \"" << r.system.buildType << "\",\n  \"flags\": \"" << esc(r.system.optimisationFlags)
      << "\",\n  \"cases\": " << r.config.cases << ",\n  \"elements\": " << r.config.elements << ",\n  \"seed\": " << r.config.seed
      << ",\n  \"warmups\": " << r.config.warmups << ",\n  \"repetitions\": " << r.config.repetitions
      << ",\n  \"speedup_assessable\": " << (r.speedupAssessable ? "true" : "false")
      << ",\n  \"agreement_ok\": " << (r.agreementOk ? "true" : "false") << ",\n  \"rows\": [\n";
    for (std::size_t i = 0; i < r.rows.size(); ++i) {
        const auto& w = r.rows[i];
        o << "    {\"workers\": " << w.workersRequested << ", \"measured\": " << (w.measured ? "true" : "false");
        if (w.measured) {
            o << ", \"times_s\": [";
            for (std::size_t k = 0; k < w.timesSeconds.size(); ++k) o << (k ? ", " : "") << w.timesSeconds[k];
            o << "], \"median_s\": " << w.medianSeconds << ", \"speedup\": " << w.speedup << ", \"efficiency\": " << w.efficiency
              << ", \"max_abs_diff_c\": " << w.maxAbsDiffVsSerial_C;
        } else {
            o << ", \"note\": \"" << esc(w.note) << "\"";
        }
        o << "}" << (i + 1 < r.rows.size() ? "," : "") << "\n";
    }
    o << "  ]\n}\n";
    return o.str();
}

}  // namespace aether::performance
