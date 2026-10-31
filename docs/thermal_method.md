# Thermal method: 1-D steady-state conduction by finite elements

## Problem
-d/dx(kA dT/dx) = qA on 0 <= x <= L, with L = 1 m, A = 1e-4 m^2, k = 10 W/(m K), q = 10000 W/m^3, T(0) = 100 C, T(L) = 20 C.
Assumptions: constant properties, uniform cross-section, steady state, insulated lateral surface, uniform volumetric generation, one-dimensional heat flow.

## Discretisation
Uniform mesh of N two-node linear elements, h = L/N.
- Element stiffness: Ke = (kA/h) [[1, -1], [-1, 1]]
- Element load: fe = (qA h / 2) [1, 1]
- Assembly adds element blocks into a tridiagonal global K (stored as three diagonals) and load vector f.
- Dirichlet conditions are applied by moving known values to the right-hand side of the neighbouring rows (f1 -= K10 T(0); f(n-2) -= K(n-2,n-1) T(L)), then replacing the two constrained rows by identity rows.
- The system is solved with the Thomas algorithm (O(N)). A pivot smaller than 1e-12 times the largest diagonal raises `SingularSystemError`; so does a non-finite result. Invalid inputs raise `InvalidParameterError` naming the offending parameter.

## Verification
Analytical solution: T(x) = 100 - 80 x/L + q/(2k) x (L - x).
- **Nodes:** linear elements with a uniform source are nodally exact in 1-D, so nodal error is at round-off level (about 1e-13 C for N = 20). This is why the assignment asks for the error at element midpoints.
- **Element midpoints:** the FE solution is linear inside an element; the error at the midpoint is exactly q/(2k) (h/2)^2.

Measured by `build/aether_results` (file `results/raw/thermal_convergence.csv`):

| N | h [m] | max abs error at midpoints [C] | observed order |
|---|---|---|---|
| 10 | 0.1 | 1.25 | n/a |
| 20 | 0.05 | 0.3125 | 2.00 |
| 40 | 0.025 | 0.078125 | 2.00 |
| 80 | 0.0125 | 0.01953125 | 2.00 |

These equal the closed form q/(8k) h^2 (= 125 h^2): 125 x 0.1^2 = 1.25. Halving h divides the error by four: second-order convergence of the interpolated solution.

## Debugging notes (how problems in such a solver show up)
These are the failure modes the tests are designed to expose; they describe diagnosis, not bugs that were found in this code.
1. **Singular matrix.** Without boundary conditions, K has a rigid-body mode (adding a constant to T changes nothing). `Assembly.SingularWithoutBoundaryConditions` shows the solver reports it instead of returning garbage.
2. **Forgetting to move the known temperature to the right-hand side.** The solution then has the right end values but a wrong interior; `BoundaryConditions.ApplyDirichletKeepsNeighbourRowsConsistent` checks row 1 explicitly.
3. **Wrong load factor.** Using qA h instead of qA h / 2 per node doubles the heating; the analytical comparison and `ElementMatrices.LoadIsQAleOver2OnBothNodes` catch it.
4. **Checking error only at nodes** hides discretisation error completely (it is ~1e-13 here), so convergence would look perfect or flat. The midpoint test (`MidpointErrorMatchesClosedForm`) guards against this.
5. **Row-sum test.** Each row of Ke must sum to zero (a constant field carries no flux): `StiffnessRowsSumToZero`.
