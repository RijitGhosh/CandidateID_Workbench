// Aether core: hand-derived backpropagation for the one-sample regression
//   h = tanh(w x + b),  y_hat = v h + c,  J = 0.5 (y_hat - y)^2
// No automatic differentiation is used anywhere.
#pragma once

#include <vector>

namespace aether::neural {

struct Params {
    double w = 0.5, b = 0.0, v = 1.0, c = 0.0;
};
struct Problem {
    double x = 1.0, y = 2.0, learningRate = 0.1;
};
struct Forward {
    double z = 0, h = 0, yHat = 0, loss = 0;
};
struct Gradients {
    double dw = 0, db = 0, dv = 0, dc = 0;
};

// Intermediate quantities of the chain rule, for the derivation worksheet.
struct Trace {
    Forward fwd;
    double error = 0;       // y_hat - y
    double dJ_dyhat = 0;    // = error
    double dJ_dh = 0;       // = error * v
    double tanhPrime = 0;   // 1 - h^2
    double dJ_dz = 0;       // = dJ_dh * tanhPrime
    Gradients grad;
};

Forward forward(const Params& p, const Problem& pr);
Trace trace(const Params& p, const Problem& pr);
Gradients analyticGradients(const Params& p, const Problem& pr);
Gradients centralDifferenceGradients(const Params& p, const Problem& pr, double epsilon);
// Simultaneous update: every new value uses the ORIGINAL parameters' gradients.
Params gradientStep(const Params& p, const Gradients& g, double learningRate);

struct GradientCheck {
    Gradients analytic, numeric;
    double absErrDw = 0, absErrDb = 0, absErrDv = 0, absErrDc = 0;
    double maxAbsError = 0;
    double epsilon = 1e-6, tolerance = 1e-5;
    bool pass = false;
};
GradientCheck checkGradients(const Params& p, const Problem& pr, double epsilon = 1e-6, double tol = 1e-5);

struct History {
    std::vector<double> loss;     // loss[0] = initial, loss[k] = after k updates
    std::vector<Params> params;   // same indexing
};
// Performs `updates` gradient-descent updates. The assignment's "one hand update + 100 more" is updates = 101.
History train(const Params& initial, const Problem& pr, int updates);

}  // namespace aether::neural
