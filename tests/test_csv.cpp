#include <gtest/gtest.h>

#include <sstream>

#include "core/csv/temperature_csv.hpp"

using namespace aether::csv;

namespace {
const char* kHeader = "node_id,x_m,y_m,z_m,temperature_C\n";
bool hasError(const CsvResult& r, std::size_t line, const std::string& fragment) {
    for (const auto& e : r.errors)
        if (e.line == line && e.message.find(fragment) != std::string::npos) return true;
    return false;
}
}  // namespace

TEST(Csv, ParsesAndComputesStatistics) {
    const auto r = parseTemperatureCsv(std::string(kHeader) + "1,0,0,0,10\n2,1,0,0,30\n3,2,0,0,20\n");
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.data.stats.count, 3u);
    EXPECT_DOUBLE_EQ(r.data.stats.min, 10.0);
    EXPECT_DOUBLE_EQ(r.data.stats.max, 30.0);
    EXPECT_DOUBLE_EQ(r.data.stats.mean, 20.0);
    EXPECT_EQ(r.data.stats.indexOfMax, 1u);
}
TEST(Csv, ColumnsMayBeReorderedAndCrlfAndBomAndBlankLinesWork) {
    const auto r = parseTemperatureCsv("\xEF\xBB\xBFtemperature_C,node_id,z_m,y_m,x_m\r\n\r\n55,7,3,2,1\r\n");
    ASSERT_TRUE(r.ok) << r.errors[0].format();
    EXPECT_EQ(r.data.nodes[0].id, 7);
    EXPECT_DOUBLE_EQ(r.data.nodes[0].x, 1.0);
    EXPECT_DOUBLE_EQ(r.data.nodes[0].temperature, 55.0);
}
TEST(Csv, CommentsAreCollectedAndIgnored) {
    const auto r = parseTemperatureCsv(std::string("# Synthetic example generated for this prototype.\n") + kHeader + "1,0,0,0,1\n");
    ASSERT_TRUE(r.ok);
    ASSERT_EQ(r.data.comments.size(), 1u);
    EXPECT_NE(r.data.comments[0].find("Synthetic"), std::string::npos);
}
TEST(Csv, EmptyAndWhitespaceFilesRejected) {
    EXPECT_TRUE(hasError(parseTemperatureCsv(""), 0, "empty"));
    EXPECT_TRUE(hasError(parseTemperatureCsv("  \n\n"), 0, "empty"));
}
TEST(Csv, HeaderOnlyRejected) { EXPECT_TRUE(hasError(parseTemperatureCsv(kHeader), 0, "no data rows")); }
TEST(Csv, MissingColumnListedByName) {
    const auto r = parseTemperatureCsv("node_id,x_m,y_m,temperature_C\n1,0,0,5\n");
    EXPECT_FALSE(r.ok);
    EXPECT_TRUE(hasError(r, 1, "z_m"));
}
TEST(Csv, UnexpectedAndDuplicateColumnsRejected) {
    EXPECT_FALSE(parseTemperatureCsv("node_id,x_m,y_m,z_m,temperature_C,extra\n1,0,0,0,1,2\n").ok);
    EXPECT_FALSE(parseTemperatureCsv("node_id,x_m,x_m,y_m,z_m,temperature_C\n1,0,0,0,0,1\n").ok);
}
TEST(Csv, MalformedRowReportsLineNumber) {
    const auto r = parseTemperatureCsv(std::string(kHeader) + "1,0,0,0,1\n2,0,0,5\n3,0,0,0,1\n");
    EXPECT_FALSE(r.ok);
    EXPECT_TRUE(hasError(r, 3, "expected 5 fields but found 4"));
}
TEST(Csv, NonNumericRejectedWithColumnName) {
    const auto r = parseTemperatureCsv(std::string(kHeader) + "1,0,abc,0,1\n");
    EXPECT_TRUE(hasError(r, 2, "y_m"));
}
TEST(Csv, NonFiniteValuesRejected) {
    for (const char* bad : {"nan", "inf", "-inf", "NaN"}) {
        EXPECT_TRUE(hasError(parseTemperatureCsv(std::string(kHeader) + "1," + bad + ",0,0,1\n"), 2, "not finite")) << bad;
        EXPECT_TRUE(hasError(parseTemperatureCsv(std::string(kHeader) + "1,0,0,0," + bad + "\n"), 2, "not finite")) << bad;
    }
}
TEST(Csv, DuplicateNodeIdReportsBothLines) {
    const auto r = parseTemperatureCsv(std::string(kHeader) + "5,0,0,0,1\n6,0,0,0,1\n5,1,1,1,2\n");
    EXPECT_FALSE(r.ok);
    EXPECT_TRUE(hasError(r, 4, "duplicate node_id 5 (first seen on line 2)"));
}
TEST(Csv, BadNodeIdsRejected) {
    EXPECT_FALSE(parseTemperatureCsv(std::string(kHeader) + "1.5,0,0,0,1\n").ok);
    EXPECT_FALSE(parseTemperatureCsv(std::string(kHeader) + "-3,0,0,0,1\n").ok);
    EXPECT_FALSE(parseTemperatureCsv(std::string(kHeader) + ",0,0,0,1\n").ok);
}
TEST(Csv, ManyErrorsAreCappedButFailureIsReported) {
    std::string s = kHeader;
    for (int i = 0; i < 100; ++i) s += "x,0,0,0,1\n";
    const auto r = parseTemperatureCsv(s);
    EXPECT_FALSE(r.ok);
    EXPECT_EQ(r.errors.size(), kMaxReportedErrors);
    EXPECT_TRUE(r.errorsTruncated);
}
TEST(Csv, RoundTripExport) {
    const auto r = parseTemperatureCsv(std::string(kHeader) + "1,0.5,0.25,0,12.5\n2,1,2,3,-4\n");
    ASSERT_TRUE(r.ok);
    std::ostringstream o;
    writeTemperatureCsv(o, r.data);
    const auto r2 = parseTemperatureCsv(o.str());
    ASSERT_TRUE(r2.ok);
    EXPECT_EQ(r2.data.nodes.size(), 2u);
    EXPECT_DOUBLE_EQ(r2.data.nodes[1].temperature, -4.0);
}
TEST(Csv, MissingFileReportsError) {
    const auto r = loadTemperatureCsvFile("/no/such/file.csv");
    EXPECT_FALSE(r.ok);
    EXPECT_NE(r.errors[0].message.find("cannot open"), std::string::npos);
}
TEST(Csv, FiftyThousandRowsParse) {
    std::string s = kHeader;
    for (int i = 0; i < 50000; ++i) s += std::to_string(i) + "," + std::to_string(i) + ",0,0," + std::to_string(i % 100) + "\n";
    const auto r = parseTemperatureCsv(s);
    ASSERT_TRUE(r.ok);
    EXPECT_EQ(r.data.stats.count, 50000u);
}
