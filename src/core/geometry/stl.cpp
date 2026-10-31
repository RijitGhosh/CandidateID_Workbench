#include "stl.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <cstdlib>
#include <fstream>
#include <set>
#include <sstream>

namespace aether::geometry {

std::string StlError::format() const {
    std::ostringstream o;
    if (line) o << "line " << line << ": "; else o << "file: ";
    o << message;
    return o.str();
}

namespace {

struct Token {
    std::string_view text;
    std::size_t line;
};

std::vector<Token> tokenize(std::string_view s) {
    std::vector<Token> out;
    std::size_t line = 1, i = 0;
    while (i < s.size()) {
        const char c = s[i];
        if (c == '\n') { ++line; ++i; continue; }
        if (std::isspace(static_cast<unsigned char>(c))) { ++i; continue; }
        const std::size_t b = i;
        while (i < s.size() && !std::isspace(static_cast<unsigned char>(s[i]))) ++i;
        out.push_back({s.substr(b, i - b), line});
    }
    return out;
}

bool ieq(std::string_view a, std::string_view b) {
    if (a.size() != b.size()) return false;
    for (std::size_t i = 0; i < a.size(); ++i)
        if (std::tolower(static_cast<unsigned char>(a[i])) != std::tolower(static_cast<unsigned char>(b[i]))) return false;
    return true;
}

bool looksBinary(std::string_view t) {
    if (t.find('\0') != std::string_view::npos) return true;
    if (t.size() >= 84) {  // 80-byte header + uint32 count + 50 bytes per triangle
        std::uint32_t n = 0;
        for (int i = 0; i < 4; ++i) n |= static_cast<std::uint32_t>(static_cast<unsigned char>(t[80 + i])) << (8 * i);
        if (84ull + 50ull * n == t.size() && n > 0) return true;
    }
    return false;
}

struct Cursor {
    const std::vector<Token>& toks;
    std::size_t pos = 0;
    bool eof() const { return pos >= toks.size(); }
};

}  // namespace

Vec3 triangleNormal(const Triangle& t) {
    const double ax = t.v[1].x - t.v[0].x, ay = t.v[1].y - t.v[0].y, az = t.v[1].z - t.v[0].z;
    const double bx = t.v[2].x - t.v[0].x, by = t.v[2].y - t.v[0].y, bz = t.v[2].z - t.v[0].z;
    Vec3 n{ay * bz - az * by, az * bx - ax * bz, ax * by - ay * bx};
    const double len = std::sqrt(n.x * n.x + n.y * n.y + n.z * n.z);
    if (len <= 0.0) return {0, 0, 0};
    return {n.x / len, n.y / len, n.z / len};
}

StlResult parseAsciiStl(std::string_view text) {
    StlResult r;
    auto fail = [&](std::size_t line, std::string msg) {
        r.ok = false;
        r.errors.push_back({line, std::move(msg)});
        return r;
    };

    if (text.empty()) return fail(0, "file is empty");
    if (text.size() > kMaxStlFileBytes) return fail(0, "file exceeds the 256 MiB limit");
    if (looksBinary(text))
        return fail(0, "binary STL detected. Only ASCII STL ('solid ... endsolid') is supported; re-export the model as ASCII STL.");

    const std::vector<Token> toks = tokenize(text);
    if (toks.empty()) return fail(0, "file contains only whitespace");
    Cursor c{toks};

    if (!ieq(toks[0].text, "solid"))
        return fail(toks[0].line, "expected 'solid' at start of file but found '" + std::string(toks[0].text) + "'");
    const std::size_t headerLine = toks[0].line;
    ++c.pos;
    {
        std::string name;
        while (!c.eof() && toks[c.pos].line == headerLine) {
            if (!name.empty()) name += ' ';
            name += std::string(toks[c.pos].text);
            ++c.pos;
        }
        r.mesh.name = name;
    }

    auto need = [&](std::string_view kw) -> bool {
        if (c.eof()) {
            r.errors.push_back({0, "unexpected end of file: expected '" + std::string(kw) + "' (file truncated?)"});
            return false;
        }
        if (!ieq(toks[c.pos].text, kw)) {
            r.errors.push_back({toks[c.pos].line, "expected '" + std::string(kw) + "' but found '" + std::string(toks[c.pos].text) + "'"});
            return false;
        }
        ++c.pos;
        return true;
    };
    auto number = [&](double& out, const char* what) -> bool {
        if (c.eof()) {
            r.errors.push_back({0, std::string("unexpected end of file: expected a number for ") + what});
            return false;
        }
        const Token& t = toks[c.pos];
        const std::string tmp(t.text);
        char* end = nullptr;
        out = std::strtod(tmp.c_str(), &end);
        if (end == tmp.c_str() || *end != '\0') {
            r.errors.push_back({t.line, "invalid number '" + tmp + "' for " + what});
            return false;
        }
        if (!std::isfinite(out)) {
            r.errors.push_back({t.line, "non-finite value '" + tmp + "' for " + what + " (NaN/Inf are rejected)"});
            return false;
        }
        ++c.pos;
        return true;
    };

    std::set<std::array<double, 9>> seen;
    bool closed = false;
    while (!c.eof()) {
        if (ieq(toks[c.pos].text, "endsolid")) {
            ++c.pos;
            const std::size_t l = toks[c.pos - 1].line;
            while (!c.eof() && toks[c.pos].line == l) ++c.pos;  // optional name
            if (!c.eof()) return fail(toks[c.pos].line, "unexpected content after 'endsolid' (multi-solid files are not supported)");
            closed = true;
            break;
        }
        if (r.mesh.facetsInFile >= kMaxStlTriangles) return fail(toks[c.pos].line, "triangle limit of 5,000,000 exceeded");
        if (!need("facet")) return (r.ok = false, r);
        if (!need("normal")) return (r.ok = false, r);
        double nrm[3];
        for (double& v : nrm) if (!number(v, "facet normal")) return (r.ok = false, r);
        if (!need("outer") || !need("loop")) return (r.ok = false, r);
        Triangle tri;
        for (auto& vert : tri.v) {
            if (!need("vertex")) return (r.ok = false, r);
            if (!number(vert.x, "vertex x") || !number(vert.y, "vertex y") || !number(vert.z, "vertex z")) return (r.ok = false, r);
        }
        if (!need("endloop") || !need("endfacet")) return (r.ok = false, r);
        ++r.mesh.facetsInFile;

        // degenerate: zero (or numerically negligible) area
        double maxEdge2 = 0;
        for (int i = 0; i < 3; ++i) {
            const Vec3& a = tri.v[i];
            const Vec3& b = tri.v[(i + 1) % 3];
            maxEdge2 = std::max(maxEdge2, (a.x - b.x) * (a.x - b.x) + (a.y - b.y) * (a.y - b.y) + (a.z - b.z) * (a.z - b.z));
        }
        const double ax = tri.v[1].x - tri.v[0].x, ay = tri.v[1].y - tri.v[0].y, az = tri.v[1].z - tri.v[0].z;
        const double bx = tri.v[2].x - tri.v[0].x, by = tri.v[2].y - tri.v[0].y, bz = tri.v[2].z - tri.v[0].z;
        const double cx = ay * bz - az * by, cy = az * bx - ax * bz, cz = ax * by - ay * bx;
        const double area2 = std::sqrt(cx * cx + cy * cy + cz * cz);
        if (area2 <= 1e-12 * maxEdge2 || maxEdge2 == 0.0) { ++r.mesh.degenerateSkipped; continue; }

        // duplicate: same three vertices in any order
        std::array<std::array<double, 3>, 3> vs{{{tri.v[0].x, tri.v[0].y, tri.v[0].z},
                                                 {tri.v[1].x, tri.v[1].y, tri.v[1].z},
                                                 {tri.v[2].x, tri.v[2].y, tri.v[2].z}}};
        std::sort(vs.begin(), vs.end());
        std::array<double, 9> key{};
        for (int i = 0; i < 3; ++i) for (int j = 0; j < 3; ++j) key[i * 3 + j] = vs[i][j];
        if (!seen.insert(key).second) { ++r.mesh.duplicateSkipped; continue; }

        r.mesh.triangles.push_back(tri);
    }

    if (!r.errors.empty()) { r.ok = false; return r; }
    if (!closed) return fail(0, "unexpected end of file: missing 'endsolid' (file truncated?)");
    if (r.mesh.facetsInFile == 0) return fail(0, "solid contains no facets");
    if (r.mesh.triangles.empty()) return fail(0, "all facets are degenerate or duplicates; nothing to display");

    BoundingBox bb;
    bb.valid = true;
    bb.min = bb.max = r.mesh.triangles[0].v[0];
    for (const auto& t : r.mesh.triangles)
        for (const auto& v : t.v) {
            bb.min = {std::min(bb.min.x, v.x), std::min(bb.min.y, v.y), std::min(bb.min.z, v.z)};
            bb.max = {std::max(bb.max.x, v.x), std::max(bb.max.y, v.y), std::max(bb.max.z, v.z)};
        }
    r.mesh.bbox = bb;
    if (r.mesh.degenerateSkipped)
        r.warnings.push_back(std::to_string(r.mesh.degenerateSkipped) + " degenerate (zero-area) triangle(s) skipped");
    if (r.mesh.duplicateSkipped)
        r.warnings.push_back(std::to_string(r.mesh.duplicateSkipped) + " duplicate triangle(s) skipped");
    r.ok = true;
    return r;
}

StlResult loadAsciiStlFile(const std::string& path) {
    std::ifstream f(path, std::ios::binary | std::ios::ate);
    if (!f) {
        StlResult r;
        r.errors.push_back({0, "cannot open file '" + path + "'"});
        return r;
    }
    const std::streamoff size = f.tellg();
    if (size < 0 || static_cast<std::size_t>(size) > kMaxStlFileBytes) {
        StlResult r;
        r.errors.push_back({0, "file exceeds the 256 MiB limit"});
        return r;
    }
    std::string buf(static_cast<std::size_t>(size), '\0');
    f.seekg(0);
    f.read(buf.data(), size);
    return parseAsciiStl(buf);
}

}  // namespace aether::geometry
