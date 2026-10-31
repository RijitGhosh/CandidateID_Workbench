// aether_results: regenerates every file in results/ from the real C++ core. No values are typed in by hand.
//   usage: aether_results <output_dir> [--quick]
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "../core/neural/backprop.hpp"
#include "../core/performance/benchmark.hpp"
#include "../core/thermal/thermal.hpp"

namespace fs = std::filesystem;
using namespace aether;

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: aether_results <output_dir> [--quick]\n"; return 2; }
    const fs::path out = argv[1];
    const bool quick = argc > 2 && std::string(argv[2]) == "--quick";
    fs::create_directories(out / "raw");
    fs::create_directories(out / "benchmarks");

    // --- thermal: solutions for N = 10, 20, 40, 80 and the convergence table
    thermal::RodParameters base;
    for (int n : {10, 20, 40, 80}) {
        base.elements = n;
        const auto s = thermal::solve(base);
        std::ofstream(out / "raw" / ("thermal_nodes_N" + std::to_string(n) + ".csv")) << [&] { std::ostringstream o; thermal::writeNodesCsv(o, s); return o.str(); }();
        std::ofstream(out / "raw" / ("thermal_midpoints_N" + std::to_string(n) + ".csv")) << [&] { std::ostringstream o; thermal::writeMidpointsCsv(o, s); return o.str(); }();
        std::ofstream(out / "raw" / ("thermal_summary_N" + std::to_string(n) + ".txt")) << [&] { std::ostringstream o; thermal::writeSummaryText(o, s); return o.str(); }();
    }
    {
        const auto rows = thermal::convergenceStudy(base, {10, 20, 40, 80});
        std::ofstream f(out / "raw" / "thermal_convergence.csv");
        thermal::writeConvergenceCsv(f, rows);
        std::cout << "convergence:\n";
        thermal::writeConvergenceCsv(std::cout, rows);
    }

    // --- backpropagation
    {
        neural::Params p0;
        neural::Problem pr;
        const auto t = neural::trace(p0, pr);
        const auto chk = neural::checkGradients(p0, pr);
        const auto h = neural::train(p0, pr, 101);  // 1 hand update + 100 subsequent steps
        std::ofstream l(out / "raw" / "backprop_loss.csv");
        l << std::setprecision(15) << "update,loss,w,b,v,c\n";
        for (std::size_t i = 0; i < h.loss.size(); ++i)
            l << i << ',' << h.loss[i] << ',' << h.params[i].w << ',' << h.params[i].b << ',' << h.params[i].v << ',' << h.params[i].c << '\n';
        std::ofstream d(out / "raw" / "backprop_derivation_values.txt");
        d << std::setprecision(12)
          << "z=" << t.fwd.z << "\nh=" << t.fwd.h << "\ny_hat=" << t.fwd.yHat << "\nJ=" << t.fwd.loss
          << "\nerror=" << t.error << "\ndJ_dh=" << t.dJ_dh << "\ntanh_prime=" << t.tanhPrime << "\ndJ_dz=" << t.dJ_dz
          << "\ndJ_dw=" << t.grad.dw << "\ndJ_db=" << t.grad.db << "\ndJ_dv=" << t.grad.dv << "\ndJ_dc=" << t.grad.dc
          << "\nfd_dw=" << chk.numeric.dw << "\nfd_db=" << chk.numeric.db << "\nfd_dv=" << chk.numeric.dv << "\nfd_dc=" << chk.numeric.dc
          << "\nmax_abs_gradient_error=" << chk.maxAbsError
          << "\nw1=" << h.params[1].w << "\nb1=" << h.params[1].b << "\nv1=" << h.params[1].v << "\nc1=" << h.params[1].c
          << "\nJ1=" << h.loss[1] << "\nJ_after_101_updates=" << h.loss.back() << '\n';
        std::cout << "gradient check max abs error: " << chk.maxAbsError << "  final loss: " << h.loss.back() << '\n';
    }

    // --- benchmark
    {
        performance::BenchmarkConfig cfg;
        if (quick) { cfg.cases = 200; cfg.elements = 200; }
        const auto rep = performance::runBenchmark(cfg, nullptr, [](double f, const std::string& s) {
            std::fprintf(stderr, "\r[benchmark %3.0f%%] %-40s", f * 100.0, s.c_str());
        });
        std::fprintf(stderr, "\n");
        std::ofstream(out / "benchmarks" / "benchmark.md") << performance::toMarkdown(rep);
        std::ofstream(out / "benchmarks" / "benchmark.json") << performance::toJson(rep);
        std::cout << performance::toMarkdown(rep);
    }
    return 0;
}
