#include <gtest/gtest.h>

#include <cmath>

#include "core/neural/backprop.hpp"

using namespace aether::neural;

TEST(Backprop, ForwardPassValues) {
    const auto f = forward({}, {});
    EXPECT_DOUBLE_EQ(f.z, 0.5);
    EXPECT_NEAR(f.h, std::tanh(0.5), 1e-15);
    EXPECT_NEAR(f.h, 0.46211715726, 1e-10);
    EXPECT_NEAR(f.yHat, 0.46211715726, 1e-10);
    EXPECT_NEAR(f.loss, 0.5 * (0.46211715726 - 2.0) * (0.46211715726 - 2.0), 1e-9);
}

TEST(Backprop, AnalyticGradientsMatchHandDerivation) {
    const auto t = trace({}, {});
    const double h = std::tanh(0.5), e = h - 2.0;
    EXPECT_NEAR(t.error, e, 1e-15);
    EXPECT_NEAR(t.grad.dc, e, 1e-15);
    EXPECT_NEAR(t.grad.dv, e * h, 1e-15);
    EXPECT_NEAR(t.grad.db, e * 1.0 * (1 - h * h), 1e-15);
    EXPECT_NEAR(t.grad.dw, e * 1.0 * (1 - h * h) * 1.0, 1e-15);
    EXPECT_LT(t.grad.dc, 0.0) << "prediction is below the target, so dJ/dc must be negative";
}

TEST(Backprop, CentralDifferenceAgreesWithinSpecForEveryGradient) {
    const auto c = checkGradients({}, {}, 1e-6, 1e-5);
    EXPECT_LE(c.absErrDw, 1e-5) << "dJ/dw: analytic " << c.analytic.dw << " vs numeric " << c.numeric.dw;
    EXPECT_LE(c.absErrDb, 1e-5) << "dJ/db";
    EXPECT_LE(c.absErrDv, 1e-5) << "dJ/dv";
    EXPECT_LE(c.absErrDc, 1e-5) << "dJ/dc";
    EXPECT_TRUE(c.pass);
}

TEST(Backprop, GradientCheckAlsoPassesAtOtherPoints) {
    for (Params p : {Params{0.1, -0.3, 2.0, 0.5}, Params{-1.0, 0.7, -0.5, 1.5}, Params{2.0, 2.0, 1.0, -2.0}})
        EXPECT_TRUE(checkGradients(p, {}).pass);
}

TEST(Backprop, CheckDetectsAWrongGradient) {
    // sanity check of the checker itself: a deliberately wrong analytic value must exceed tolerance
    auto g = analyticGradients({}, {});
    const auto num = centralDifferenceGradients({}, {}, 1e-6);
    EXPECT_GT(std::fabs((g.dw * 1.01) - num.dw), 1e-5);
}

TEST(Backprop, UpdateIsSimultaneousUsingOriginalParameters) {
    const Params p0;
    const auto g = analyticGradients(p0, {});
    const auto p1 = gradientStep(p0, g, 0.1);
    EXPECT_DOUBLE_EQ(p1.w, 0.5 - 0.1 * g.dw);
    EXPECT_DOUBLE_EQ(p1.b, 0.0 - 0.1 * g.db);
    EXPECT_DOUBLE_EQ(p1.v, 1.0 - 0.1 * g.dv);
    EXPECT_DOUBLE_EQ(p1.c, 0.0 - 0.1 * g.dc);
    // dJ/dw used v = 1 (the ORIGINAL v). A sequential update would have used the new v instead.
    const double sequentialDw = (forward(p0, {}).yHat - 2.0) * p1.v * (1 - std::pow(forward(p0, {}).h, 2));
    EXPECT_NE(sequentialDw, g.dw);
}

TEST(Backprop, FirstUpdateDecreasesLoss) {
    const auto h = train({}, {}, 1);
    ASSERT_EQ(h.loss.size(), 2u);
    EXPECT_LT(h.loss[1], h.loss[0]);
}

TEST(Backprop, TrainingIsMonotoneAndConvergesTowardZero) {
    const auto h = train({}, {}, 101);  // 1 hand-calculated update + 100 further steps
    ASSERT_EQ(h.loss.size(), 102u);
    for (std::size_t i = 1; i < h.loss.size(); ++i) EXPECT_LE(h.loss[i], h.loss[i - 1] + 1e-15) << "step " << i;
    EXPECT_LT(h.loss.back(), 0.1 * h.loss.front()) << "loss should drop by at least 10x in 101 updates";
}

TEST(Backprop, NegativeUpdateCountThrows) { EXPECT_THROW(train({}, {}, -1), std::invalid_argument); }
