// Aether core: ASCII STL importer (binary STL is NOT supported and is rejected explicitly).
#pragma once

#include <cstddef>
#include <string>
#include <string_view>
#include <vector>

namespace aether::geometry {

constexpr std::size_t kMaxStlFileBytes = 256ull * 1024 * 1024;  // 256 MiB
constexpr std::size_t kMaxStlTriangles = 5'000'000;

struct Vec3 {
    double x = 0, y = 0, z = 0;
};
struct Triangle {
    Vec3 v[3];
};
struct BoundingBox {
    Vec3 min, max;
    bool valid = false;
};

struct StlError {
    std::size_t line = 0;  // 1-based; 0 = whole file
    std::string message;
    std::string format() const;
};

struct StlMesh {
    std::string name;
    std::vector<Triangle> triangles;  // accepted triangles only
    BoundingBox bbox;
    std::size_t facetsInFile = 0;
    std::size_t degenerateSkipped = 0;
    std::size_t duplicateSkipped = 0;
};

struct StlResult {
    bool ok = false;
    StlMesh mesh;
    std::vector<StlError> errors;
    std::vector<std::string> warnings;
};

StlResult parseAsciiStl(std::string_view text);
StlResult loadAsciiStlFile(const std::string& path);

Vec3 triangleNormal(const Triangle& t);  // unit normal; zero vector for degenerate input

}  // namespace aether::geometry
