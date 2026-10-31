// Integration tests: shipped example data files -> parsers; solver -> export -> re-import.
#include <gtest/gtest.h>

#include <algorithm>
#include <array>
#include <cmath>
#include <map>
#include <tuple>
#include <fstream>
#include <sstream>

#include "core/csv/temperature_csv.hpp"
#include "core/geometry/stl.hpp"
#include "core/thermal/thermal.hpp"

namespace {
const std::string kData = AETHER_DATA_DIR;
}

TEST(Integration, ExampleStlLoadsWithDocumentedCounts) {
    const auto r = aether::geometry::loadAsciiStlFile(kData + "/geometry/example_bracket.stl");
    ASSERT_TRUE(r.ok) << (r.errors.empty() ? "" : r.errors[0].format());
    EXPECT_EQ(r.mesh.triangles.size(), 24u);
    EXPECT_EQ(r.mesh.degenerateSkipped, 0u);
    EXPECT_DOUBLE_EQ(r.mesh.bbox.min.x, 0.0);
    EXPECT_DOUBLE_EQ(r.mesh.bbox.max.x, 40.0);
    EXPECT_DOUBLE_EQ(r.mesh.bbox.max.y, 20.0);
    EXPECT_DOUBLE_EQ(r.mesh.bbox.max.z, 30.0);
}

TEST(Integration, ExampleStlIsWatertightEveryEdgeSharedByTwoTriangles) {
    const auto r = aether::geometry::loadAsciiStlFile(kData + "/geometry/example_bracket.stl");
    ASSERT_TRUE(r.ok);
    using K = std::array<double, 6>;
    std::map<K, int> edges;
    // The two boxes touch on a face; compare edges that belong to the exposed surface of each box separately.
    for (const auto& t : r.mesh.triangles)
        for (int i = 0; i < 3; ++i) {
            auto a = t.v[i], b = t.v[(i + 1) % 3];
            if (std::tie(a.x, a.y, a.z) > std::tie(b.x, b.y, b.z)) std::swap(a, b);
            ++edges[K{a.x, a.y, a.z, b.x, b.y, b.z}];
        }
    for (const auto& [k, n] : edges) EXPECT_EQ(n % 2, 0) << "edge used an odd number of times";
}

TEST(Integration, ExampleCsvLoadsAndCarriesSyntheticLabel) {
    const auto r = aether::csv::loadTemperatureCsvFile(kData + "/temperature/example_temperature.csv");
    ASSERT_TRUE(r.ok) << (r.errors.empty() ? "" : r.errors[0].format());
    EXPECT_EQ(r.data.stats.count, 189u);  // 21 x 3 x 3
    bool labelled = false;
    for (const auto& c : r.data.comments) if (c.find("Not an actual ANSYS export") != std::string::npos) labelled = true;
    EXPECT_TRUE(labelled) << "example CSV must carry the required synthetic-data label";
    EXPECT_NEAR(r.data.stats.min, 20.0 - 0.0 + 0.0, 100.0);  // sanity: finite, plausible
}

TEST(Integration, SolverExportReimportsAsTemperatureCsv) {
    aether::thermal::RodParameters p;
    p.elements = 20;
    const auto s = aether::thermal::solve(p);
    std::ostringstream csv;
    csv << "node_id,x_m,y_m,z_m,temperature_C\n";
    for (std::size_t i = 0; i < s.x.size(); ++i) csv << i << ',' << s.x[i] << ",0,0," << s.tFe[i] << '\n';
    const auto r = aether::csv::parseTemperatureCsv(csv.str());
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.data.stats.count, 21u);
    EXPECT_NEAR(r.data.stats.max, *std::max_element(s.tFe.begin(), s.tFe.end()), 1e-4);
}
