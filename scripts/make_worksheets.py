#!/usr/bin/env python3
"""Documentation helper: typesets the handwriting worksheets from results/raw/backprop_derivation_values.txt
(values computed by the C++ program). It performs no engineering calculation of its own."""
import sys, pathlib
from playwright.sync_api import sync_playwright

root = pathlib.Path(__file__).resolve().parent.parent
v = dict(l.strip().split("=") for l in (root / "results/raw/backprop_derivation_values.txt").read_text().splitlines() if "=" in l)
f = lambda k, d=9: f"{float(v[k]):.{d}f}"

CSS = """
@page{size:A4;margin:14mm}
body{font-family:'DejaVu Sans',Arial,sans-serif;color:#111;font-size:13.5pt;line-height:1.55}
h1{font-size:18pt;margin:0 0 2mm}h2{font-size:14.5pt;margin:5mm 0 1mm;border-bottom:1.5px solid #333}
.sub{font-size:10.5pt;color:#444}.eq{font-family:'DejaVu Sans Mono',monospace;font-size:13pt;margin:1.2mm 0}
.num{font-weight:bold}.page{page-break-after:always}.line{border-bottom:1px solid #999;height:9.5mm}
.box{border:1.5px solid #333;padding:2mm 3mm;margin:2mm 0}.small{font-size:10pt;color:#444}
table{border-collapse:collapse;width:100%}td,th{border:1px solid #444;padding:2mm;font-size:12pt}.blank{height:9mm}
"""

def lines(n): return "".join('<div class="line"></div>' for _ in range(n))

def copy_sheet():
    p1 = f"""<div class="page"><h1>Backpropagation by hand: page 1 of 2</h1>
<div class="sub">Copy this page by hand onto your own paper (do not submit this printout as the handwritten work). Name / ID / date at the end of page 2.</div>
<div class="box">Given: x = 1, y = 2, w = 0.5, b = 0, v = 1, c = 0, learning rate 0.1<br>Model: z = wx + b, h = tanh(z), y&#770; = vh + c, J = 0.5 (y&#770; - y)&sup2;</div>
<h2>1. Forward pass</h2>
<div class="eq">z = 0.5(1) + 0 = <span class="num">{f('z',4)}</span></div>
<div class="eq">h = tanh(0.5) = <span class="num">{f('h')}</span></div>
<div class="eq">y&#770; = 1(h) + 0 = <span class="num">{f('y_hat')}</span></div>
<div class="eq">J = 0.5 (y&#770; - 2)&sup2; = 0.5 ({f('error')})&sup2; = <span class="num">{f('J')}</span></div>
<h2>2. Chain rule, backwards</h2>
<div class="eq">dJ/dy&#770; = y&#770; - y = <span class="num">{f('error')}</span></div>
<div class="eq">dJ/dc = dJ/dy&#770; &middot; 1 = <span class="num">{f('dJ_dc')}</span></div>
<div class="eq">dJ/dv = dJ/dy&#770; &middot; h = <span class="num">{f('dJ_dv')}</span></div>
<div class="eq">dJ/dh = dJ/dy&#770; &middot; v = <span class="num">{f('dJ_dh')}</span></div>
<div class="eq">dh/dz = 1 - h&sup2; = <span class="num">{f('tanh_prime')}</span></div>
<div class="eq">dJ/dz = dJ/dh &middot; (1 - h&sup2;) = <span class="num">{f('dJ_dz')}</span></div>
<div class="eq">dJ/dw = dJ/dz &middot; x = <span class="num">{f('dJ_dw')}</span></div>
<div class="eq">dJ/db = dJ/dz &middot; 1 = <span class="num">{f('dJ_db')}</span></div>
<div class="small">All gradients are negative: the prediction is below the target.</div></div>"""
    p2 = f"""<div class="page"><h1>Backpropagation by hand: page 2 of 2</h1>
<h2>3. One simultaneous update (all four use the ORIGINAL w, b, v, c)</h2>
<div class="eq">w&#8321; = 0.5 - 0.1({f('dJ_dw')}) = <span class="num">{f('w1')}</span></div>
<div class="eq">b&#8321; = 0 - 0.1({f('dJ_db')}) = <span class="num">{f('b1')}</span></div>
<div class="eq">v&#8321; = 1 - 0.1({f('dJ_dv')}) = <span class="num">{f('v1')}</span></div>
<div class="eq">c&#8321; = 0 - 0.1({f('dJ_dc')}) = <span class="num">{f('c1')}</span></div>
<div class="eq">New loss J&#8321; = <span class="num">{f('J1')}</span> (was {f('J')}: decreased)</div>
<h2>4. Gradient direction, in words</h2>
<div class="box">The gradient points toward increasing loss. Gradient descent moves each parameter the opposite way, so w, b, v and c all increase, which raises y&#770; toward the target 2 and lowers J. The loss falls from {f('J',4)} to {f('J1',4)} after one step.</div>
<h2>5. Check against the C++ program (central differences, &epsilon; = 1e-6)</h2>
<table><tr><th></th><th>analytic</th><th>finite difference</th></tr>
<tr><td>dJ/dw</td><td>{f('dJ_dw',8)}</td><td>{f('fd_dw',8)}</td></tr><tr><td>dJ/db</td><td>{f('dJ_db',8)}</td><td>{f('fd_db',8)}</td></tr>
<tr><td>dJ/dv</td><td>{f('dJ_dv',8)}</td><td>{f('fd_dv',8)}</td></tr><tr><td>dJ/dc</td><td>{f('dJ_dc',8)}</td><td>{f('fd_dc',8)}</td></tr></table>
<div class="small">Largest absolute error from the program: {float(v['max_abs_gradient_error']):.2e} (limit 1e-5). Loss after 101 updates: {float(v['J_after_101_updates']):.2e}.</div>
<h2>Sign-off</h2><div class="eq">Name: ____________________ ID: ____________ Date: ____________ Signature: ____________</div></div>"""
    return p1 + p2

def blank_sheet():
    def sect(title, n): return f"<h2>{title}</h2>{lines(n)}"
    p1 = f"""<div class="page"><h1>Backpropagation practice sheet (blank): page 1 of 2</h1>
<div class="box">x = 1, y = 2, w = 0.5, b = 0, v = 1, c = 0, learning rate 0.1. z = wx + b, h = tanh z, y&#770; = vh + c, J = 0.5 (y&#770; - y)&sup2;</div>
{sect('1. Forward pass: z, h, y&#770;, J',6)}{sect('2. Chain rule: dJ/dy&#770;, dJ/dc, dJ/dv, dJ/dh, dh/dz, dJ/dz, dJ/dw, dJ/db',12)}</div>"""
    p2 = f"""<div class="page"><h1>Practice sheet (blank): page 2 of 2</h1>
{sect('3. Simultaneous update of w, b, v, c; new loss',8)}{sect('4. Explain the gradient direction',5)}
<div class="small">Check your numbers with docs/handwritten_answer_key.md or results/raw/backprop_derivation_values.txt.</div></div>"""
    return p1 + p2

def render(body, out):
    html = f"<html><head><meta charset='utf-8'><style>{CSS}</style></head><body>{body}</body></html>"
    tmp = root / "build" / "_ws.html"
    tmp.parent.mkdir(exist_ok=True)
    tmp.write_text(html)
    with sync_playwright() as p:
        b = p.chromium.launch(); pg = b.new_page(); pg.goto(tmp.as_uri())
        pg.pdf(path=str(out), format="A4", margin={"top": "14mm", "bottom": "14mm", "left": "14mm", "right": "14mm"}, print_background=True)
        b.close()

render(copy_sheet(), root / "docs/worksheet_to_copy_by_hand.pdf")
render(blank_sheet(), root / "docs/worksheet_blank_practice.pdf")
print("worksheets written")
