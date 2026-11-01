# Handwritten derivation worksheet (2 pages) - to be copied by hand, photographed or scanned by the candidate

**This is a printed template with the correct equations and numbers. It is NOT a handwritten scan.** Print-ready versions: `docs/worksheet_to_copy_by_hand.pdf` (full equations and numbers, large type, 2 pages) and `docs/worksheet_blank_practice.pdf` (blank ruled sheets to practise without looking). Copy it by hand onto two sheets and sign it; do not submit this typed text as handwriting.
Numbers below come from the C++ program (`results/raw/backprop_derivation_values.txt`, 12 significant digits); round as you write.

## Page 1 - Forward pass and gradients
Given: x = 1, y = 2, w = 0.5, b = 0, v = 1, c = 0, learning rate eta = 0.1.
Model: z = w x + b, h = tanh(z), y_hat = v h + c, J = 1/2 (y_hat - y)^2.

**Forward**
- z = 0.5 x 1 + 0 = **0.5**
- h = tanh(0.5) = **0.462117157**
- y_hat = 1 x 0.462117157 + 0 = **0.462117157**
- J = 1/2 (0.462117157 - 2)^2 = 1/2 (-1.537882843)^2 = **1.182541819**

**Backward (chain rule)**
1. dJ/dy_hat = y_hat - y = **-1.537882843**
2. dJ/dc = dJ/dy_hat x 1 = **-1.537882843**
3. dJ/dv = dJ/dy_hat x h = -1.537882843 x 0.462117157 = **-0.710682047**
4. dJ/dh = dJ/dy_hat x v = **-1.537882843**
5. dtanh/dz = 1 - h^2 = 1 - 0.213552267 = **0.786447733**
6. dJ/dz = dJ/dh x (1 - h^2) = -1.537882843 x 0.786447733 = **-1.209464475**
7. dJ/dw = dJ/dz x x = **-1.209464475**
8. dJ/db = dJ/dz x 1 = **-1.209464475**

Sign check: y_hat < y, so every gradient that increases y_hat when its parameter increases is negative; gradient descent will increase all four parameters.

**Finite-difference check (eps = 1e-6):** dJ/dtheta ~ [J(theta + eps) - J(theta - eps)] / (2 eps)

| Parameter | Analytic | Central difference (program) | abs error | <= 1e-5 |
|---|---|---|---|---|
| w | -1.209464475 | -1.209464475 | ~1e-10 | yes |
| b | -1.209464475 | -1.209464475 | ~1e-10 | yes |
| v | -0.710682047 | -0.710682047 | ~1e-10 | yes |
| c | -1.537882843 | -1.537882843 | ~1e-10 | yes |

Program maximum absolute error: 1.12e-10. Write your own hand-computed difference for one parameter in the box: ________________________

## Page 2 - One simultaneous update, then training
Update rule (use ORIGINAL values on the right-hand side of all four lines):
- w1 = w - eta dJ/dw = 0.5 - 0.1 (-1.209464475) = **0.620946448**
- b1 = b - eta dJ/db = 0 - 0.1 (-1.209464475) = **0.120946448**
- v1 = v - eta dJ/dv = 1 - 0.1 (-0.710682047) = **1.071068205**
- c1 = c - eta dJ/dc = 0 - 0.1 (-1.537882843) = **0.153788284**

New forward pass: z = 0.620946448 + 0.120946448 = 0.741892895; J1 = **0.685773765** (down from 1.182541819, so the step was in the right direction).

**Training (program output, same learning rate 0.1, 100 further updates)**

| Update k | Loss J |
|---|---|
| 0 (initial) | 1.18254182 |
| 1 (the hand step above) | 0.68577377 |
| 2 | 0.41667962 |
| 100 | 1.7026e-20 |
| 101 | 1.0805e-20 |

Final parameters after 101 updates: w = 0.903729286, b = 0.403729286, v = 1.492947917, c = 0.710643978.
Plot: loss against update step is in `results/plots/backprop_loss_dark.png` (log scale). Sketch it on your page and label the axes.

**Explain (2-3 sentences, in your own words):** the gradient points uphill, so we move opposite to it; the loss decreases monotonically for this small learning rate;
as the prediction reaches the target the error factor (y_hat - y) shrinks, so all gradients shrink and the decrease slows geometrically.

Name: ____________________   ID: ____________   Date: ____________   Signature: ____________
