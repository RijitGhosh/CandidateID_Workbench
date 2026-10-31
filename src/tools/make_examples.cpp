// make_examples: writes the self-authored example data files (see data/provenance.md).
//   usage: make_examples <data_dir>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>

namespace fs = std::filesystem;

namespace {
struct V { double x, y, z; };
void facet(std::ofstream& f, V a, V b, V c) {
    const V u{b.x - a.x, b.y - a.y, b.z - a.z}, w{c.x - a.x, c.y - a.y, c.z - a.z};
    V n{u.y * w.z - u.z * w.y, u.z * w.x - u.x * w.z, u.x * w.y - u.y * w.x};
    const double l = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    n = {n.x / l, n.y / l, n.z / l};
    f << "  facet normal " << n.x << ' ' << n.y << ' ' << n.z << "\n    outer loop\n";
    for (V p : {a, b, c}) f << "      vertex " << p.x << ' ' << p.y << ' ' << p.z << '\n';
    f << "    endloop\n  endfacet\n";
}
// Axis-aligned box [x0,x1]x[y0,y1]x[z0,z1], 12 triangles, outward normals (counter-clockwise from outside).
void box(std::ofstream& f, double x0, double x1, double y0, double y1, double z0, double z1) {
    const V p000{x0, y0, z0}, p100{x1, y0, z0}, p110{x1, y1, z0}, p010{x0, y1, z0};
    const V p001{x0, y0, z1}, p101{x1, y0, z1}, p111{x1, y1, z1}, p011{x0, y1, z1};
    facet(f, p000, p010, p110); facet(f, p000, p110, p100);  // -z
    facet(f, p001, p101, p111); facet(f, p001, p111, p011);  // +z
    facet(f, p000, p100, p101); facet(f, p000, p101, p001);  // -y
    facet(f, p010, p011, p111); facet(f, p010, p111, p110);  // +y
    facet(f, p000, p001, p011); facet(f, p000, p011, p010);  // -x
    facet(f, p100, p110, p111); facet(f, p100, p111, p101);  // +x
}
}  // namespace

int main(int argc, char** argv) {
    if (argc < 2) { std::cerr << "usage: make_examples <data_dir>\n"; return 2; }
    const fs::path dir = argv[1];
    fs::create_directories(dir / "geometry");
    fs::create_directories(dir / "temperature");

    {   // L-shaped bracket made of two boxes, 12 + 12 = 24 triangles. Units: millimetres (by convention only).
        std::ofstream f(dir / "geometry" / "example_bracket.stl");
        f << std::setprecision(6);
        f << "solid aether_example_bracket\n";
        box(f, 0, 40, 0, 20, 0, 5);    // base plate 40 x 20 x 5
        box(f, 0, 5, 0, 20, 5, 30);    // upright   5 x 20 x 25 on top
        f << "endsolid aether_example_bracket\n";
    }
    {   // Synthetic temperature field on a regular 21 x 3 x 3 grid of a 1 m rod (0.1 m x 0.1 m cross-section).
        std::ofstream f(dir / "temperature" / "example_temperature.csv");
        f << "# Synthetic example generated for this prototype.\n# Not an actual ANSYS export.\n"
          << "# T = 100 - 80x + 500 x (1 - x) + 40 (y^2 + z^2), x,y,z in metres, y,z in [-0.05, 0.05]\n"
          << std::setprecision(10) << "node_id,x_m,y_m,z_m,temperature_C\n";
        int id = 1;
        for (int i = 0; i <= 20; ++i)
            for (int j = -1; j <= 1; ++j)
                for (int k = -1; k <= 1; ++k) {
                    const double x = i / 20.0, y = 0.05 * j, z = 0.05 * k;
                    f << id++ << ',' << x << ',' << y << ',' << z << ',' << 100 - 80 * x + 500 * x * (1 - x) + 40 * (y * y + z * z) << '\n';
                }
    }
    std::cout << "examples written to " << dir << '\n';
    return 0;
}
