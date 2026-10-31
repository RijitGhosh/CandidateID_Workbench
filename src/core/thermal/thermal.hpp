// Aether core: 1-D steady-state thermal finite-element solver.
//   -d/dx( kA dT/dx ) = qA     (Dirichlet temperatures at both ends)
// Two-node linear elements, uniform mesh, tridiagonal global system.
#pragma once

#include <array>
#include <cstddef>
#include <iosfwd>
#include <stdexcept>
#include <vector>

namespace aether::thermal {

struct RodParameters {
    double length_m = 1.0;
    double area_m2 = 1.0e-4;
    double conductivity_W_mK = 10.0;
    double heatGeneration_W_m3 = 10000.0;
    double tLeft_C = 100.0;
    double tRight_C = 20.0;
    int elements = 10;
};

class InvalidParameterError : public std::invalid_argument {
public:
    using std::invalid_argument::invalid_argument;
};
class SingularSystemError : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};

constexpr int kMaxElements = 1'000'000;

// Throws InvalidParameterError with an actionable message.
void validate(const RodParameters& p);

// 2x2 row-major: {K00, K01, K10, K11}
using Matrix2 = std::array<double, 4>;
Matrix2 elementStiffness(double conductivity, double area, double elementLength);
std::array<double, 2> elementLoad(double heatGeneration, double area, double elementLength);

// Global system K T = f stored as three diagonals.
//   lower[i] = K(i+1, i), upper[i] = K(i, i+1)   (size n-1)
struct TridiagonalSystem {
    std::vector<double> lower, diag, upper, rhs;
    std::size_t size() const { return diag.size(); }
};

// Assemble K and f from element contributions (no boundary conditions yet).
void assemble(const RodParameters& p, TridiagonalSystem& sys);
// Apply T(0)=tLeft, T(L)=tRight by symmetric elimination (RHS corrected, rows replaced).
void applyDirichlet(TridiagonalSystem& sys, double tLeft, double tRight);

struct Scratch {
    TridiagonalSystem sys;
    std::vector<double> cprime;
};

// Thomas algorithm. Throws SingularSystemError on a (near-)zero pivot or non-finite result.
void solveTridiagonal(const TridiagonalSystem& sys, std::vector<double>& cprime, std::vector<double>& x);

// Complete numerical path used by BOTH the interactive solver and the batch runner.
void solveInto(const RodParameters& p, Scratch& scratch, std::vector<double>& temperatures);

double analyticalTemperature(const RodParameters& p, double x);

struct Solution {
    RodParameters params;
    std::vector<double> x;          // node coordinates [m]
    std::vector<double> tFe;        // FE nodal temperature [C]
    std::vector<double> tExact;     // analytical at nodes [C]
    std::vector<double> xMid;       // element midpoints [m]
    std::vector<double> tFeMid;     // FE interpolation at midpoints [C]
    std::vector<double> tExactMid;  // analytical at midpoints [C]
    std::vector<double> midError;   // |FE - exact| at midpoints [C]
    double maxMidpointError = 0.0;
    double maxNodalError = 0.0;
};

Solution solve(const RodParameters& p);

struct ConvergenceRow {
    int elements = 0;
    double h_m = 0.0;
    double maxMidpointError_C = 0.0;
    double observedOrder = 0.0;  // NaN for the first row
};
std::vector<ConvergenceRow> convergenceStudy(const RodParameters& base, const std::vector<int>& counts);

void writeNodesCsv(std::ostream& os, const Solution& s);
void writeMidpointsCsv(std::ostream& os, const Solution& s);
void writeSummaryText(std::ostream& os, const Solution& s);
void writeConvergenceCsv(std::ostream& os, const std::vector<ConvergenceRow>& rows);

}  // namespace aether::thermal
