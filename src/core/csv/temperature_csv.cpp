#include "temperature_csv.hpp"

#include <array>
#include <cerrno>
#include <cmath>
#include <cstdlib>
#include <fstream>
#include <iomanip>
#include <ostream>
#include <sstream>
#include <unordered_map>

namespace aether::csv {

std::string ParseError::format() const {
    std::ostringstream o;
    if (line) o << "line " << line << ": "; else o << "file: ";
    o << message;
    return o.str();
}

namespace {

std::string_view trim(std::string_view s) {
    while (!s.empty() && (s.front() == ' ' || s.front() == '\t' || s.front() == '\r')) s.remove_prefix(1);
    while (!s.empty() && (s.back() == ' ' || s.back() == '\t' || s.back() == '\r')) s.remove_suffix(1);
    return s;
}

std::vector<std::string_view> split(std::string_view line) {
    std::vector<std::string_view> f;
    std::size_t b = 0;
    for (std::size_t i = 0; i <= line.size(); ++i)
        if (i == line.size() || line[i] == ',') {
            f.push_back(trim(line.substr(b, i - b)));
            b = i + 1;
        }
    return f;
}

constexpr std::array<const char*, 5> kColumns = {"node_id", "x_m", "y_m", "z_m", "temperature_C"};

bool parseDouble(std::string_view s, double& out, std::string& why) {
    if (s.empty()) { why = "empty value"; return false; }
    const std::string t(s);
    char* end = nullptr;
    errno = 0;
    out = std::strtod(t.c_str(), &end);
    if (end == t.c_str() || *end != '\0') { why = "'" + t + "' is not a number"; return false; }
    if (!std::isfinite(out)) { why = "'" + t + "' is not finite (NaN/Inf rejected)"; return false; }
    return true;
}

bool parseId(std::string_view s, long long& out, std::string& why) {
    if (s.empty()) { why = "empty node_id"; return false; }
    const std::string t(s);
    char* end = nullptr;
    errno = 0;
    out = std::strtoll(t.c_str(), &end, 10);
    if (end == t.c_str() || *end != '\0') { why = "node_id '" + t + "' is not an integer"; return false; }
    if (errno == ERANGE) { why = "node_id '" + t + "' is out of range"; return false; }
    if (out < 0) { why = "node_id '" + t + "' must be non-negative"; return false; }
    return true;
}

}  // namespace

CsvResult parseTemperatureCsv(std::string_view text) {
    CsvResult r;
    auto addError = [&](std::size_t line, std::string msg) {
        if (r.errors.size() < kMaxReportedErrors) r.errors.push_back({line, std::move(msg)});
        else r.errorsTruncated = true;
    };

    if (text.size() > kMaxCsvFileBytes) { addError(0, "file exceeds the 100 MiB limit"); return r; }
    if (text.substr(0, 3) == "\xEF\xBB\xBF") text.remove_prefix(3);  // UTF-8 BOM
    if (text.find_first_not_of(" \t\r\n") == std::string_view::npos) { addError(0, "file is empty"); return r; }

    std::array<int, 5> colIndex{-1, -1, -1, -1, -1};
    std::size_t ncols = 0;
    bool haveHeader = false;
    std::unordered_map<long long, std::size_t> firstLineOfId;

    std::size_t lineNo = 0, pos = 0;
    while (pos <= text.size()) {
        std::size_t nl = text.find('\n', pos);
        if (nl == std::string_view::npos) nl = text.size();
        std::string_view line = text.substr(pos, nl - pos);
        pos = nl + 1;
        ++lineNo;
        std::string_view t = trim(line);
        if (t.empty()) continue;
        if (t.front() == '#') {
            r.data.comments.emplace_back(trim(t.substr(1)));
            continue;
        }
        if (!haveHeader) {
            haveHeader = true;
            const auto f = split(t);
            ncols = f.size();
            std::vector<std::string> missing;
            for (std::size_t k = 0; k < kColumns.size(); ++k) {
                for (std::size_t i = 0; i < f.size(); ++i)
                    if (f[i] == kColumns[k]) {
                        if (colIndex[k] >= 0) addError(lineNo, std::string("duplicate column '") + kColumns[k] + "'");
                        colIndex[k] = static_cast<int>(i);
                    }
                if (colIndex[k] < 0) missing.emplace_back(kColumns[k]);
            }
            for (std::size_t i = 0; i < f.size(); ++i) {
                bool known = false;
                for (const char* c : kColumns) if (f[i] == c) known = true;
                if (!known) addError(lineNo, "unexpected column '" + std::string(f[i]) + "' (schema: node_id,x_m,y_m,z_m,temperature_C)");
            }
            if (!missing.empty()) {
                std::string m = "missing required column(s):";
                for (const auto& s : missing) m += " " + s;
                addError(lineNo, m + ". Expected header: node_id,x_m,y_m,z_m,temperature_C");
            }
            if (!r.errors.empty()) return r;  // cannot interpret rows without a valid header
            continue;
        }
        if (r.data.nodes.size() >= kMaxCsvRows) { addError(lineNo, "row limit of 2,000,000 exceeded"); break; }
        const auto f = split(t);
        if (f.size() != ncols) {
            addError(lineNo, "malformed row: expected " + std::to_string(ncols) + " fields but found " + std::to_string(f.size()));
            continue;
        }
        Node n;
        std::string why;
        bool rowOk = true;
        auto num = [&](int k, double& dst, const char* name) {
            if (!parseDouble(f[colIndex[k]], dst, why)) {
                addError(lineNo, std::string(name) + ": " + why);
                rowOk = false;
            }
        };
        if (!parseId(f[colIndex[0]], n.id, why)) { addError(lineNo, "node_id: " + why); rowOk = false; }
        num(1, n.x, "x_m");
        num(2, n.y, "y_m");
        num(3, n.z, "z_m");
        num(4, n.temperature, "temperature_C");
        if (!rowOk) continue;
        auto ins = firstLineOfId.emplace(n.id, lineNo);
        if (!ins.second) {
            addError(lineNo, "duplicate node_id " + std::to_string(n.id) + " (first seen on line " + std::to_string(ins.first->second) + ")");
            continue;
        }
        r.data.nodes.push_back(n);
    }

    if (!haveHeader) { addError(0, "file has no header row"); return r; }
    if (r.errors.empty() && r.data.nodes.empty()) addError(0, "file has a header but no data rows");
    if (!r.errors.empty()) return r;

    Stats s;
    s.count = r.data.nodes.size();
    s.min = s.max = r.data.nodes[0].temperature;
    long double sum = 0;
    for (std::size_t i = 0; i < r.data.nodes.size(); ++i) {
        const double v = r.data.nodes[i].temperature;
        sum += v;
        if (v < s.min) { s.min = v; s.indexOfMin = i; }
        if (v > s.max) { s.max = v; s.indexOfMax = i; }
    }
    s.mean = static_cast<double>(sum / static_cast<long double>(s.count));
    r.data.stats = s;
    r.ok = true;
    return r;
}

CsvResult loadTemperatureCsvFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        CsvResult r;
        r.errors.push_back({0, "cannot open file '" + path + "'"});
        return r;
    }
    const std::streamoff size = f.tellg();
    if (size < 0 || static_cast<std::size_t>(size) > kMaxCsvFileBytes) {
        CsvResult r;
        r.errors.push_back({0, "file exceeds the 100 MiB limit"});
        return r;
    }
    std::string buf(static_cast<std::size_t>(size), '\0');
    f.seekg(0);
    f.read(buf.data(), size);
    return parseTemperatureCsv(buf);
}

void writeTemperatureCsv(std::ostream& os, const TemperatureData& d) {
    for (const auto& c : d.comments) os << "# " << c << '\n';
    os << std::setprecision(12) << "node_id,x_m,y_m,z_m,temperature_C\n";
    for (const auto& n : d.nodes) os << n.id << ',' << n.x << ',' << n.y << ',' << n.z << ',' << n.temperature << '\n';
}

void writeStatsSummary(std::ostream& os, const TemperatureData& d) {
    os << std::setprecision(12);
    os << "node_count=" << d.stats.count << '\n'
       << "min_temperature_C=" << d.stats.min << '\n'
       << "max_temperature_C=" << d.stats.max << '\n'
       << "mean_temperature_C=" << d.stats.mean << '\n';
}

}  // namespace aether::csv
