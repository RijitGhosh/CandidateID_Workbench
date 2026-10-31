#include "thermal.hpp"

#include <algorithm>
#include <cmath>
#include <limits>
#include <ostream>
#include <iomanip>
#include <sstream>

namespace aether::thermal {

namespace {
void requirePositiveFinite(double v, const char* name) {
    if (!std::isfinite(v) || v <= 0.0) {
        std::ostringstream m;
        m << name << " must be a finite positive number (got " << v << ")";
        throw InvalidParameterError(m.str());
    }
}
}  // namespace

void validate(const RodParameters& p) {
    requirePositiveFinite(p.length_m, "length_m");
    requirePositiveFinite(p.area_m2, "area_m2");
    requirePositiveFinite(p.conductivity_W_mK, "conductivity_W_mK");
    if (!std::isfinite(p.heatGeneration_W_m3))
        throw InvalidParameterError("heatGeneration_W_m3 must be finite");
    if (!std::isfinite(p.tLeft_C) || !std::isfinite(p.tRight_C))
        throw InvalidParameterError("boundary temperatures must be finite");
    if (p.elements < 1 || p.elements > kMaxElements) {
        std::ostringstream m;
        m << "elements must be in [1, " << kMaxElements << "] (got " << p.elements << ")";
        throw InvalidParameterError(m.str());
    }
}

Matrix2 elementStiffness(double k, double A, double le) {
    if (!(le > 0.0) || !std::isfinite(le)) throw InvalidParameterError("element length must be positive and finite");
    const double c = k * A / le;
    return {c, -c, -c, c};
}

std::array<double, 2> elementLoad(double q, double A, double le) {
    if (!(le > 0.0) || !std::isfinite(le)) throw InvalidParameterError("element length must be positive and finite");
    const double f = q * A * le / 2.0;
    return {f, f};
}

void assemble(const RodParameters& p, TridiagonalSystem& sys) {
    validate(p);
    const std::size_t n = static_cast<std::size_t>(p.elements) + 1;
    sys.diag.assign(n, 0.0);
    sys.lower.assign(n - 1, 0.0);
    sys.upper.assign(n - 1, 0.0);
    sys.rhs.assign(n, 0.0);
    const double le = p.length_m / p.elements;
    const Matrix2 ke = elementStiffness(p.conductivity_W_mK, p.area_m2, le);
    const auto fe = elementLoad(p.heatGeneration_W_m3, p.area_m2, le);
    for (std::size_t e = 0; e + 1 < n; ++e) {
        sys.diag[e] += ke[0];
        sys.upper[e] += ke[1];
        sys.lower[e] += ke[2];
        sys.diag[e + 1] += ke[3];
        sys.rhs[e] += fe[0];
        sys.rhs[e + 1] += fe[1];
    }
}

void applyDirichlet(TridiagonalSystem& sys, double tLeft, double tRight) {
    const std::size_t n = sys.size();
    if (n < 2) throw InvalidParameterError("system needs at least two nodes");
    // Move known values to the RHS of the neighbouring rows first...
    sys.rhs[1] -= sys.lower[0] * tLeft;
    sys.rhs[n - 2] -= sys.upper[n - 2] * tRight;
    // ...then replace the constrained rows by identity rows.
    sys.diag[0] = 1.0;
    sys.upper[0] = 0.0;
    sys.rhs[0] = tLeft;
    sys.diag[n - 1] = 1.0;
    sys.lower[n - 2] = 0.0;
    sys.rhs[n - 1] = tRight;
    // Zero the coupling of the neighbours toward the constrained nodes (symmetry kept).
    if (n >= 3) {
        sys.lower[0] = 0.0;      // K(1,0)
        sys.upper[n - 2] = 0.0;  // K(n-2,n-1)
    }
}

void solveTridiagonal(const TridiagonalSystem& sys, std::vector<double>& cprime, std::vector<double>& x) {
    const std::size_t n = sys.size();
    if (n == 0) throw SingularSystemError("empty system");
    double scale = 0.0;
    for (double d : sys.diag) scale = std::max(scale, std::fabs(d));
    const double tol = 1e-12 * scale;
    cprime.assign(n, 0.0);
    x.assign(n, 0.0);
    if (std::fabs(sys.diag[0]) <= tol) throw SingularSystemError("singular system: zero pivot in row 0");
    if (n > 1) cprime[0] = sys.upper[0] / sys.diag[0];
    x[0] = sys.rhs[0] / sys.diag[0];
    for (std::size_t i = 1; i < n; ++i) {
        const double m = sys.diag[i] - sys.lower[i - 1] * cprime[i - 1];
        if (!(std::fabs(m) > tol)) {
            std::ostringstream msg;
            msg << "singular system: pivot " << m << " in row " << i
                << " is ~0. The model probably lacks a prescribed temperature (rigid-body mode).";
            throw SingularSystemError(msg.str());
        }
        if (i + 1 < n) cprime[i] = sys.upper[i] / m;
        x[i] = (sys.rhs[i] - sys.lower[i - 1] * x[i - 1]) / m;
    }
    for (std::size_t i = n - 1; i-- > 0;) x[i] -= cprime[i] * x[i + 1];
    for (double v : x)
        if (!std::isfinite(v)) throw SingularSystemError("solution contains non-finite values");
}

void solveInto(const RodParameters& p, Scratch& s, std::vector<double>& t) {
    assemble(p, s.sys);
    applyDirichlet(s.sys, p.tLeft_C, p.tRight_C);
    solveTridiagonal(s.sys, s.cprime, t);
}

double analyticalTemperature(const RodParameters& p, double x) {
    const double L = p.length_m;
    return p.tLeft_C + (p.tRight_C - p.tLeft_C) * x / L +
           (p.heatGeneration_W_m3 / (2.0 * p.conductivity_W_mK)) * x * (L - x);
}

Solution solve(const RodParameters& p) {
    validate(p);
    Solution s;
    s.params = p;
    Scratch scratch;
    solveInto(p, scratch, s.tFe);
    const std::size_t n = s.tFe.size();
    const double le = p.length_m / p.elements;
    s.x.resize(n);
    s.tExact.resize(n);
    for (std::size_t i = 0; i < n; ++i) {
        s.x[i] = le * static_cast<double>(i);
        s.tExact[i] = analyticalTemperature(p, s.x[i]);
        s.maxNodalError = std::max(s.maxNodalError, std::fabs(s.tFe[i] - s.tExact[i]));
    }
    s.x[n - 1] = p.length_m;
    const std::size_t ne = n - 1;
    s.xMid.resize(ne);
    s.tFeMid.resize(ne);
    s.tExactMid.resize(ne);
    s.midError.resize(ne);
    for (std::size_t e = 0; e < ne; ++e) {
        s.xMid[e] = 0.5 * (s.x[e] + s.x[e + 1]);
        s.tFeMid[e] = 0.5 * (s.tFe[e] + s.tFe[e + 1]);  // linear shape-function interpolation
        s.tExactMid[e] = analyticalTemperature(p, s.xMid[e]);
        s.midError[e] = std::fabs(s.tFeMid[e] - s.tExactMid[e]);
        s.maxMidpointError = std::max(s.maxMidpointError, s.midError[e]);
    }
    return s;
}

std::vector<ConvergenceRow> convergenceStudy(const RodParameters& base, const std::vector<int>& counts) {
    std::vector<ConvergenceRow> rows;
    for (int n : counts) {
        RodParameters p = base;
        p.elements = n;
        const Solution s = solve(p);
        ConvergenceRow r;
        r.elements = n;
        r.h_m = p.length_m / n;
        r.maxMidpointError_C = s.maxMidpointError;
        r.observedOrder = std::numeric_limits<double>::quiet_NaN();
        if (!rows.empty() && rows.back().maxMidpointError_C > 0.0 && r.maxMidpointError_C > 0.0)
            r.observedOrder = std::log(rows.back().maxMidpointError_C / r.maxMidpointError_C) /
                              std::log(rows.back().h_m / r.h_m);
        rows.push_back(r);
    }
    return rows;
}

void writeNodesCsv(std::ostream& os, const Solution& s) {
    os << std::setprecision(12);
    os << "node,x_m,T_fe_C,T_analytical_C,abs_error_C\n";
    for (std::size_t i = 0; i < s.x.size(); ++i)
        os << i << ',' << s.x[i] << ',' << s.tFe[i] << ',' << s.tExact[i] << ',' << std::fabs(s.tFe[i] - s.tExact[i]) << '\n';
}

void writeMidpointsCsv(std::ostream& os, const Solution& s) {
    os << std::setprecision(12);
    os << "element,x_mid_m,T_fe_interp_C,T_analytical_C,abs_error_C\n";
    for (std::size_t e = 0; e < s.xMid.size(); ++e)
        os << e << ',' << s.xMid[e] << ',' << s.tFeMid[e] << ',' << s.tExactMid[e] << ',' << s.midError[e] << '\n';
}

void writeSummaryText(std::ostream& os, const Solution& s) {
    os << std::setprecision(12);
    os << "elements=" << s.params.elements << '\n'
       << "length_m=" << s.params.length_m << '\n'
       << "area_m2=" << s.params.area_m2 << '\n'
       << "conductivity_W_mK=" << s.params.conductivity_W_mK << '\n'
       << "heat_generation_W_m3=" << s.params.heatGeneration_W_m3 << '\n'
       << "T_left_C=" << s.params.tLeft_C << '\n'
       << "T_right_C=" << s.params.tRight_C << '\n'
       << "max_abs_error_midpoints_C=" << s.maxMidpointError << '\n'
       << "max_abs_error_nodes_C=" << s.maxNodalError << '\n';
}

void writeConvergenceCsv(std::ostream& os, const std::vector<ConvergenceRow>& rows) {
    os << std::setprecision(12);
    os << "elements,h_m,max_midpoint_error_C,observed_order\n";
    for (const auto& r : rows) {
        os << r.elements << ',' << r.h_m << ',' << r.maxMidpointError_C << ',';
        if (std::isnan(r.observedOrder)) os << "n/a"; else os << r.observedOrder;
        os << '\n';
    }
}

}  // namespace aether::thermal
