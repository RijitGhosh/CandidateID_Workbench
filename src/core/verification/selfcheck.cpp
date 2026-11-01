#include "selfcheck.hpp"

#include <algorithm>
#include <cmath>
#include <sstream>

#include "../csv/temperature_csv.hpp"
#include "../geometry/stl.hpp"
#include "../hardware/mock_devices.hpp"
#include "../neural/backprop.hpp"
#include "../performance/batch.hpp"
#include "../thermal/thermal.hpp"

namespace aether::verification {

namespace {
std::string sci(double v) {
    std::ostringstream o;
    o.precision(3);
    o << std::scientific << v;
    return o.str();
}
}  // namespace

std::vector<CheckResult> runAllChecks(const std::function<void(double, const std::string&)>& progress) {
    std::vector<CheckResult> out;
    const int total = 8;
    int n = 0;
    auto step = [&](const char* what) { if (progress) progress(static_cast<double>(n++) / total, what); };

    // 1. FE nodal values equal the analytical solution (exact for linear elements and uniform source).
    step("FE vs analytical (nodes)");
    {
        thermal::RodParameters p;
        p.elements = 40;
        const auto s = thermal::solve(p);
        out.push_back({"FE nodal temperatures vs analytical (N=40)", s.maxNodalError <= 1e-9, "max nodal error " + sci(s.maxNodalError) + " C (criterion <= 1e-9)"});
    }
    // 2. Midpoint error + convergence order
    step("Mesh convergence");
    {
        thermal::RodParameters p;
        const auto rows = thermal::convergenceStudy(p, {10, 20, 40, 80});
        bool mono = true;
        for (std::size_t i = 1; i < rows.size(); ++i) mono = mono && rows[i].maxMidpointError_C < rows[i - 1].maxMidpointError_C;
        const double order = rows.back().observedOrder;
        out.push_back({"Mesh convergence (N=10,20,40,80) midpoint error",
                       mono && std::fabs(order - 2.0) < 0.05,
                       "error at N=80: " + sci(rows.back().maxMidpointError_C) + " C, observed order " + sci(order) + " (criterion: 2.0 +/- 0.05)"});
    }
    // 3. Boundary conditions satisfied
    step("Boundary conditions");
    {
        thermal::RodParameters p;
        p.elements = 20;
        const auto s = thermal::solve(p);
        const double e = std::max(std::fabs(s.tFe.front() - 100.0), std::fabs(s.tFe.back() - 20.0));
        out.push_back({"Prescribed temperatures T(0)=100, T(L)=20", e <= 1e-12, "boundary deviation " + sci(e) + " C"});
    }
    // 4. Invalid parameters rejected
    step("Invalid parameter handling");
    {
        thermal::RodParameters p;
        p.conductivity_W_mK = -1.0;
        bool threw = false;
        try { thermal::solve(p); } catch (const thermal::InvalidParameterError&) { threw = true; }
        out.push_back({"Invalid parameters rejected (k < 0)", threw, threw ? "InvalidParameterError raised" : "no error raised"});
    }
    // 5. Parallel agreement
    step("Serial vs parallel agreement");
    {
        const auto cases = performance::generateCases(200, 42);
        const auto a = performance::runSerial(cases, 200);
        const auto b = performance::runParallel(cases, 200, 4);
        const double d = performance::maxAbsDifference(a.temperatures, b.temperatures);
        out.push_back({"Serial vs parallel (200 cases, N=200, " + std::to_string(b.workersUsed) + " worker(s))", d <= 1e-8,
                       "max |T_serial - T_parallel| = " + sci(d) + " C (criterion <= 1e-8)"});
    }
    // 6. Gradient check
    step("Gradient check");
    {
        const auto g = neural::checkGradients({}, {}, 1e-6, 1e-5);
        out.push_back({"Backprop gradients vs central differences", g.pass, "max abs error " + sci(g.maxAbsError) + " (criterion <= 1e-5, eps = 1e-6)"});
    }
    // 7. Parsers reject bad input
    step("Parser rejection");
    {
        const bool stlBad = !geometry::parseAsciiStl("").ok && !geometry::parseAsciiStl("solid x\nfacet normal 0 0 1\nouter loop\nvertex nan 0 0\n").ok;
        const bool csvBad = !csv::parseTemperatureCsv("").ok && !csv::parseTemperatureCsv("node_id,x_m\n1,2\n").ok;
        out.push_back({"STL/CSV parsers reject empty, non-finite and malformed input", stlBad && csvBad, std::string("STL rejects: ") + (stlBad ? "yes" : "no") + ", CSV rejects: " + (csvBad ? "yes" : "no")});
    }
    // 8. Mock hardware recovery
    step("Mock hardware recovery");
    {
        bool ok = true;
        std::string detail = "simulated devices: ";
        for (auto& d : hardware::makeMockDeviceSet()) {
            d->discover();
            d->connect();
            d->armTimeoutFault();
            hardware::Telemetry t;
            const auto fail = d->readTelemetry(t);
            const auto rec = d->recover();
            const auto good = d->readTelemetry(t);
            ok = ok && fail.code == hardware::ErrorCode::Timeout && rec.ok() && good.ok();
            detail += std::string(hardware::toString(d->info().kind)) + " ";
        }
        out.push_back({"Mock devices recover from a timeout (SIMULATED)", ok, detail + "(timeout -> recover -> telemetry)"});
    }
    if (progress) progress(1.0, "done");
    return out;
}

}  // namespace aether::verification
