#include <gtest/gtest.h>

#include <algorithm>
#include <cmath>
#include <limits>
#include <sstream>

#include "core/thermal/thermal.hpp"

using namespace aether::thermal;

TEST(ElementMatrices, StiffnessMatchesKAOverLeTimesPattern) {
    const auto ke = elementStiffness(10.0, 1e-4, 0.1);  // kA/le = 0.01
    EXPECT_NEAR(ke[0], 0.01, 1e-15);
    EXPECT_NEAR(ke[1], -0.01, 1e-15);
    EXPECT_NEAR(ke[2], -0.01, 1e-15);
    EXPECT_NEAR(ke[3], 0.01, 1e-15);
}
TEST(ElementMatrices, StiffnessRowsSumToZero) {
    const auto ke = elementStiffness(3.7, 2e-4, 0.0123);
    EXPECT_NEAR(ke[0] + ke[1], 0.0, 1e-18) << "a constant temperature field must produce zero nodal flux";
}
TEST(ElementMatrices, LoadIsQAleOver2OnBothNodes) {
    const auto fe = elementLoad(10000.0, 1e-4, 0.1);  // 10000*1e-4*0.1/2 = 0.05
    EXPECT_NEAR(fe[0], 0.05, 1e-15);
    EXPECT_NEAR(fe[1], 0.05, 1e-15);
}
TEST(ElementMatrices, RejectsNonPositiveLength) {
    EXPECT_THROW(elementStiffness(10, 1e-4, 0.0), InvalidParameterError);
    EXPECT_THROW(elementLoad(1, 1e-4, -1.0), InvalidParameterError);
}

TEST(Assembly, TwoElementGlobalMatrixAndLoad) {
    RodParameters p;
    p.elements = 2;  // le = 0.5, kA/le = 0.002, qA le/2 = 0.25
    TridiagonalSystem s;
    assemble(p, s);
    ASSERT_EQ(s.size(), 3u);
    EXPECT_NEAR(s.diag[0], 0.002, 1e-15);
    EXPECT_NEAR(s.diag[1], 0.004, 1e-15) << "interior node receives contributions from both elements";
    EXPECT_NEAR(s.diag[2], 0.002, 1e-15);
    EXPECT_NEAR(s.upper[0], -0.002, 1e-15);
    EXPECT_NEAR(s.lower[1], -0.002, 1e-15);
    EXPECT_NEAR(s.rhs[0], 0.25, 1e-12);  // qA le/2 = 10000*1e-4*0.5/2
    EXPECT_NEAR(s.rhs[1], 0.5, 1e-12);
    EXPECT_NEAR(s.rhs[2], 0.25, 1e-12);
}
TEST(Assembly, SingularWithoutBoundaryConditions) {
    RodParameters p;
    p.elements = 5;
    Scratch sc;
    assemble(p, sc.sys);
    std::vector<double> x;
    EXPECT_THROW(solveTridiagonal(sc.sys, sc.cprime, x), SingularSystemError)
        << "pure-Neumann conduction has a rigid-body mode; the solver must report it, not return garbage";
    // The pivot check must be the one that fires (it names the cause); a later "non-finite result" guard is only a backstop.
    try {
        solveTridiagonal(sc.sys, sc.cprime, x);
        FAIL() << "expected SingularSystemError";
    } catch (const SingularSystemError& e) {
        EXPECT_NE(std::string(e.what()).find("pivot"), std::string::npos) << "message was: " << e.what();
    }
}

TEST(BoundaryConditions, EndpointsAreExactlyPrescribed) {
    for (int n : {1, 2, 10, 80}) {
        RodParameters p;
        p.elements = n;
        const auto s = solve(p);
        EXPECT_DOUBLE_EQ(s.tFe.front(), 100.0) << "N=" << n;
        EXPECT_DOUBLE_EQ(s.tFe.back(), 20.0) << "N=" << n;
    }
}
TEST(BoundaryConditions, DifferentValuesAndNoHeatGeneration) {
    RodParameters p;
    p.tLeft_C = -5.0;
    p.tRight_C = 35.0;
    p.heatGeneration_W_m3 = 0.0;
    p.elements = 8;
    const auto s = solve(p);
    for (std::size_t i = 0; i < s.x.size(); ++i) EXPECT_NEAR(s.tFe[i], -5.0 + 40.0 * s.x[i], 1e-12) << "pure conduction must be linear";
}
TEST(BoundaryConditions, ApplyDirichletKeepsNeighbourRowsConsistent) {
    RodParameters p;
    p.elements = 3;
    TridiagonalSystem s;
    assemble(p, s);
    const double kOverH = s.diag[0];  // kA/le
    const double f1 = s.rhs[1];
    applyDirichlet(s, 100.0, 20.0);
    EXPECT_DOUBLE_EQ(s.diag[0], 1.0);
    EXPECT_DOUBLE_EQ(s.rhs[0], 100.0);
    EXPECT_DOUBLE_EQ(s.diag[3], 1.0);
    EXPECT_DOUBLE_EQ(s.rhs[3], 20.0);
    EXPECT_NEAR(s.rhs[1], f1 + kOverH * 100.0, 1e-12) << "known T(0) must move to the RHS of row 1";
}

TEST(Analytical, FormulaValues) {
    RodParameters p;
    EXPECT_DOUBLE_EQ(analyticalTemperature(p, 0.0), 100.0);
    EXPECT_NEAR(analyticalTemperature(p, 1.0), 20.0, 1e-12);
    EXPECT_NEAR(analyticalTemperature(p, 0.5), 100.0 - 40.0 + 500.0 * 0.25, 1e-12);  // = 185
}
TEST(Analytical, NodalValuesMatchForAllRequiredMeshes) {
    for (int n : {10, 20, 40, 80}) {
        RodParameters p;
        p.elements = n;
        EXPECT_LT(solve(p).maxNodalError, 1e-9) << "N=" << n << ": linear FE is nodally exact for this 1-D problem";
    }
}
TEST(Analytical, MidpointErrorMatchesClosedForm) {
    // FE interpolation error at an element midpoint = q/(2k) * (le/2)^2 exactly.
    for (int n : {10, 20, 40, 80}) {
        RodParameters p;
        p.elements = n;
        const double le = 1.0 / n;
        const double expected = 10000.0 / (2.0 * 10.0) * (le / 2.0) * (le / 2.0);
        const auto s = solve(p);
        EXPECT_NEAR(s.maxMidpointError, expected, 1e-9) << "N=" << n;
    }
}
TEST(Analytical, MidpointErrorIsNotJustNodalError) {
    RodParameters p;
    p.elements = 10;
    const auto s = solve(p);
    EXPECT_GT(s.maxMidpointError, 1.0) << "midpoint error for N=10 should be ~1.25 C";
    EXPECT_NEAR(s.maxMidpointError, 1.25, 1e-9);
}

TEST(Convergence, SecondOrderAndMonotone) {
    RodParameters p;
    const auto rows = convergenceStudy(p, {10, 20, 40, 80});
    ASSERT_EQ(rows.size(), 4u);
    for (std::size_t i = 1; i < rows.size(); ++i) {
        EXPECT_LT(rows[i].maxMidpointError_C, rows[i - 1].maxMidpointError_C);
        EXPECT_NEAR(rows[i].observedOrder, 2.0, 0.01);
    }
    EXPECT_TRUE(std::isnan(rows[0].observedOrder));
}

TEST(InvalidParameters, AreRejectedWithMessages) {
    auto bad = [](auto mutate) {
        RodParameters p;
        mutate(p);
        return p;
    };
    const double nan = std::numeric_limits<double>::quiet_NaN();
    EXPECT_THROW(solve(bad([](auto& p) { p.length_m = 0; })), InvalidParameterError);
    EXPECT_THROW(solve(bad([](auto& p) { p.area_m2 = -1; })), InvalidParameterError);
    EXPECT_THROW(solve(bad([&](auto& p) { p.conductivity_W_mK = nan; })), InvalidParameterError);
    EXPECT_THROW(solve(bad([](auto& p) { p.conductivity_W_mK = 0; })), InvalidParameterError);
    EXPECT_THROW(solve(bad([&](auto& p) { p.heatGeneration_W_m3 = nan; })), InvalidParameterError);
    EXPECT_THROW(solve(bad([&](auto& p) { p.tLeft_C = std::numeric_limits<double>::infinity(); })), InvalidParameterError);
    EXPECT_THROW(solve(bad([](auto& p) { p.elements = 0; })), InvalidParameterError);
    try {
        solve(bad([](auto& p) { p.conductivity_W_mK = -3; }));
        FAIL() << "expected InvalidParameterError";
    } catch (const InvalidParameterError& e) {
        EXPECT_NE(std::string(e.what()).find("conductivity_W_mK"), std::string::npos) << "message must name the offending parameter";
    }
}

TEST(Export, CsvContainsHeaderAndAllNodes) {
    RodParameters p;
    p.elements = 10;
    const auto s = solve(p);
    std::ostringstream n, m, sum;
    writeNodesCsv(n, s);
    writeMidpointsCsv(m, s);
    writeSummaryText(sum, s);
    const std::string nodes = n.str(), mids = m.str();  // keep alive: begin()/end() of two temporaries is UB
    EXPECT_NE(nodes.find("node,x_m,T_fe_C,T_analytical_C"), std::string::npos);
    EXPECT_EQ(std::count(nodes.begin(), nodes.end(), '\n'), 12);
    EXPECT_EQ(std::count(mids.begin(), mids.end(), '\n'), 11);
    EXPECT_NE(sum.str().find("elements=10"), std::string::npos);
    EXPECT_NE(sum.str().find("max_abs_error_midpoints_C="), std::string::npos);
}
