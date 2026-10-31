#include <gtest/gtest.h>

#include <string>

#include "core/geometry/stl.hpp"

using namespace aether::geometry;

namespace {
std::string facet(const std::string& a, const std::string& b, const std::string& c) {
    return "facet normal 0 0 1\n outer loop\n  vertex " + a + "\n  vertex " + b + "\n  vertex " + c + "\n endloop\nendfacet\n";
}
const std::string kTri = facet("0 0 0", "1 0 0", "0 1 0");
}  // namespace

TEST(Stl, ParsesSingleTriangleAndBoundingBox) {
    const auto r = parseAsciiStl("solid t\n" + facet("0 0 0", "2 0 0", "0 3 1") + "endsolid t\n");
    ASSERT_TRUE(r.ok) << (r.errors.empty() ? "" : r.errors[0].format());
    EXPECT_EQ(r.mesh.triangles.size(), 1u);
    EXPECT_EQ(r.mesh.name, "t");
    EXPECT_DOUBLE_EQ(r.mesh.bbox.max.x, 2.0);
    EXPECT_DOUBLE_EQ(r.mesh.bbox.max.y, 3.0);
    EXPECT_DOUBLE_EQ(r.mesh.bbox.max.z, 1.0);
}
TEST(Stl, KeywordsAreCaseInsensitiveAndScientificNotationWorks) {
    const auto r = parseAsciiStl("SOLID x\nFACET NORMAL 0 0 1\nOUTER LOOP\nVERTEX 0 0 0\nVERTEX 1e0 0 0\nVERTEX 0 1.5E+0 0\nENDLOOP\nENDFACET\nENDSOLID x\n");
    ASSERT_TRUE(r.ok);
    EXPECT_DOUBLE_EQ(r.mesh.bbox.max.y, 1.5);
}
TEST(Stl, EmptyFileIsRejected) {
    const auto r = parseAsciiStl("");
    EXPECT_FALSE(r.ok);
    ASSERT_FALSE(r.errors.empty());
    EXPECT_NE(r.errors[0].message.find("empty"), std::string::npos);
}
TEST(Stl, WhitespaceOnlyIsRejected) { EXPECT_FALSE(parseAsciiStl("  \n\t \n").ok); }
TEST(Stl, MissingSolidKeyword) {
    const auto r = parseAsciiStl("hello world\n");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.errors[0].line, 1u);
}
TEST(Stl, SolidWithoutFacetsIsRejected) { EXPECT_FALSE(parseAsciiStl("solid x\nendsolid x\n").ok); }
TEST(Stl, TruncatedFileReportsMissingEndsolid) {
    const auto r = parseAsciiStl("solid x\n" + kTri);
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.errors[0].message.find("endsolid"), std::string::npos);
}
TEST(Stl, TruncatedInsideFacet) {
    const auto r = parseAsciiStl("solid x\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex 1 0 0\n");
    EXPECT_FALSE(r.ok);
}
TEST(Stl, NonFiniteCoordinatesRejectedWithLineNumber) {
    for (const char* bad : {"nan", "NaN", "inf", "-inf", "infinity"}) {
        const auto r = parseAsciiStl("solid x\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertex " + std::string(bad) + " 0 0\nvertex 0 1 0\nendloop\nendfacet\nendsolid x\n");
        ASSERT_FALSE(r.ok) << bad;
        EXPECT_EQ(r.errors[0].line, 5u) << bad;
        EXPECT_NE(r.errors[0].message.find("non-finite"), std::string::npos) << bad;
    }
}
TEST(Stl, NonNumericCoordinateRejected) {
    const auto r = parseAsciiStl("solid x\nfacet normal 0 0 1\nouter loop\nvertex 0 0 abc\n");
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.errors[0].line, 4u);
}
TEST(Stl, WrongKeywordReportsExpectedAndFound) {
    const auto r = parseAsciiStl("solid x\nfacet normal 0 0 1\nouter loop\nvertex 0 0 0\nvertx 1 0 0\n");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.errors[0].message.find("expected 'vertex'"), std::string::npos);
    EXPECT_NE(r.errors[0].message.find("vertx"), std::string::npos);
}
TEST(Stl, DegenerateTrianglesAreSkippedAndCounted) {
    const auto r = parseAsciiStl("solid x\n" + kTri + facet("0 0 0", "1 1 1", "2 2 2") + facet("5 5 5", "5 5 5", "6 6 6") + "endsolid x\n");
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.mesh.triangles.size(), 1u);
    EXPECT_EQ(r.mesh.degenerateSkipped, 2u);
    EXPECT_EQ(r.mesh.facetsInFile, 3u);
    EXPECT_FALSE(r.warnings.empty());
}
TEST(Stl, DuplicateTrianglesIncludingPermutedOnesAreSkipped) {
    const auto r = parseAsciiStl("solid x\n" + kTri + kTri + facet("0 1 0", "0 0 0", "1 0 0") + "endsolid x\n");
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.mesh.triangles.size(), 1u);
    EXPECT_EQ(r.mesh.duplicateSkipped, 2u);
}
TEST(Stl, OnlyDegenerateFacetsIsAnError) { EXPECT_FALSE(parseAsciiStl("solid x\n" + facet("0 0 0", "1 1 1", "2 2 2") + "endsolid x\n").ok); }
TEST(Stl, BinaryFileIsRejectedWithClearMessage) {
    std::string bin(80, ' ');
    bin += std::string("\x01\x00\x00\x00", 4);
    bin += std::string(50, '\0');
    const auto r = parseAsciiStl(bin);
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.errors[0].message.find("binary"), std::string::npos);
}
TEST(Stl, BinaryDetectionWorksWhenHeaderStartsWithSolid) {
    std::string bin = "solid exported by cad";
    bin.resize(80, ' ');
    bin += std::string("\x01\x00\x00\x00", 4) + std::string(50, '\x01');
    EXPECT_FALSE(parseAsciiStl(bin).ok);
}
TEST(Stl, ContentAfterEndsolidIsRejected) { EXPECT_FALSE(parseAsciiStl("solid x\n" + kTri + "endsolid x\nsolid y\n").ok); }
TEST(Stl, MissingFileReportsError) {
    const auto r = loadAsciiStlFile("/definitely/not/here.stl");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.errors[0].message.find("cannot open"), std::string::npos);
}
TEST(Stl, NormalIsUnitAndZeroForDegenerate) {
    Triangle t{{{0, 0, 0}, {1, 0, 0}, {0, 1, 0}}};
    EXPECT_DOUBLE_EQ(triangleNormal(t).z, 1.0);
    Triangle d{{{0, 0, 0}, {1, 1, 1}, {2, 2, 2}}};
    EXPECT_DOUBLE_EQ(triangleNormal(d).z, 0.0);
}
TEST(Stl, ModerateSizeParsesQuickly) {
    // 20,000 distinct triangles (about 2.6 MB of text) -- the tested size range is documented in data/provenance.md
    std::string s = "solid big\n";
    for (int i = 0; i < 20000; ++i) s += facet(std::to_string(i) + " 0 0", std::to_string(i) + " 1 0", std::to_string(i) + " 0 1");
    s += "endsolid big\n";
    const auto r = parseAsciiStl(s);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.mesh.triangles.size(), 20000u);
}
