// Aether core: in-application verification checks. Each check is EXECUTED when runAllChecks() is called;
// results are never cached or pre-filled. (The GoogleTest suite in tests/ is separate and run with ctest.)
#pragma once

#include <functional>
#include <string>
#include <vector>

namespace aether::verification {

struct CheckResult {
    std::string name;
    bool passed = false;
    std::string detail;  // measured value and criterion
};

std::vector<CheckResult> runAllChecks(const std::function<void(double, const std::string&)>& progress = {});

}  // namespace aether::verification
