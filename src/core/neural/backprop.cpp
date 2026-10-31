#include "backprop.hpp"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace aether::neural {

Forward forward(const Params& p, const Problem& pr) {
    Forward f;
    f.z = p.w * pr.x + p.b;
    f.h = std::tanh(f.z);
    f.yHat = p.v * f.h + p.c;
    const double e = f.yHat - pr.y;
    f.loss = 0.5 * e * e;
    return f;
}

Trace trace(const Params& p, const Problem& pr) {
    Trace t;
    t.fwd = forward(p, pr);
    t.error = t.fwd.yHat - pr.y;
    t.dJ_dyhat = t.error;
    t.dJ_dh = t.dJ_dyhat * p.v;
    t.tanhPrime = 1.0 - t.fwd.h * t.fwd.h;
    t.dJ_dz = t.dJ_dh * t.tanhPrime;
    t.grad.dv = t.dJ_dyhat * t.fwd.h;
    t.grad.dc = t.dJ_dyhat;
    t.grad.dw = t.dJ_dz * pr.x;
    t.grad.db = t.dJ_dz;
    return t;
}

Gradients analyticGradients(const Params& p, const Problem& pr) { return trace(p, pr).grad; }

Gradients centralDifferenceGradients(const Params& p, const Problem& pr, double eps) {
    auto d = [&](double Params::*member) {
        Params hi = p, lo = p;
        hi.*member += eps;
        lo.*member -= eps;
        return (forward(hi, pr).loss - forward(lo, pr).loss) / (2.0 * eps);
    };
    return {d(&Params::w), d(&Params::b), d(&Params::v), d(&Params::c)};
}

Params gradientStep(const Params& p, const Gradients& g, double lr) {
    return {p.w - lr * g.dw, p.b - lr * g.db, p.v - lr * g.dv, p.c - lr * g.dc};
}

GradientCheck checkGradients(const Params& p, const Problem& pr, double eps, double tol) {
    GradientCheck r;
    r.epsilon = eps;
    r.tolerance = tol;
    r.analytic = analyticGradients(p, pr);
    r.numeric = centralDifferenceGradients(p, pr, eps);
    r.absErrDw = std::fabs(r.analytic.dw - r.numeric.dw);
    r.absErrDb = std::fabs(r.analytic.db - r.numeric.db);
    r.absErrDv = std::fabs(r.analytic.dv - r.numeric.dv);
    r.absErrDc = std::fabs(r.analytic.dc - r.numeric.dc);
    r.maxAbsError = std::max({r.absErrDw, r.absErrDb, r.absErrDv, r.absErrDc});
    r.pass = r.maxAbsError <= tol;
    return r;
}

History train(const Params& initial, const Problem& pr, int updates) {
    if (updates < 0) throw std::invalid_argument("updates must be >= 0");
    History h;
    Params p = initial;
    h.loss.push_back(forward(p, pr).loss);
    h.params.push_back(p);
    for (int i = 0; i < updates; ++i) {
        p = gradientStep(p, analyticGradients(p, pr), pr.learningRate);
        h.loss.push_back(forward(p, pr).loss);
        h.params.push_back(p);
    }
    return h;
}

}  // namespace aether::neural
