// Aether core: temperature-result CSV parser.
// Schema: node_id,x_m,y_m,z_m,temperature_C  (columns in any order, all five required, no extras).
// Lines starting with '#' are comments. Quoted fields are not supported.
#pragma once

#include <cstddef>
#include <iosfwd>
#include <string>
#include <string_view>
#include <vector>

namespace aether::csv {

constexpr std::size_t kMaxCsvFileBytes = 100ull * 1024 * 1024;  // 100 MiB
constexpr std::size_t kMaxCsvRows = 2'000'000;
constexpr std::size_t kMaxReportedErrors = 20;

struct Node {
    long long id = 0;
    double x = 0, y = 0, z = 0;
    double temperature = 0;
};

struct ParseError {
    std::size_t line = 0;  // 1-based, 0 = whole file
    std::string message;
    std::string format() const;
};

struct Stats {
    std::size_t count = 0;
    double min = 0, max = 0, mean = 0;
    std::size_t indexOfMin = 0, indexOfMax = 0;
};

struct TemperatureData {
    std::vector<Node> nodes;
    Stats stats;
    std::vector<std::string> comments;  // '#' lines, without the marker
};

struct CsvResult {
    bool ok = false;
    TemperatureData data;
    std::vector<ParseError> errors;
    bool errorsTruncated = false;
};

CsvResult parseTemperatureCsv(std::string_view text);
CsvResult loadTemperatureCsvFile(const std::string& path);

void writeTemperatureCsv(std::ostream& os, const TemperatureData& d);
void writeStatsSummary(std::ostream& os, const TemperatureData& d);

}  // namespace aether::csv
