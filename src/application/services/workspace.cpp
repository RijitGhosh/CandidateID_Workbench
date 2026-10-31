#include "workspace.hpp"

#include <ctime>
#include <fstream>
#include <sstream>

namespace aether::application {

std::string nowString() {
    const std::time_t t = std::time(nullptr);
    std::tm tmv{};
#if defined(_WIN32)
    localtime_s(&tmv, &t);
#else
    localtime_r(&t, &tmv);
#endif
    char buf[32];
    std::strftime(buf, sizeof(buf), "%Y-%m-%d %H:%M:%S", &tmv);
    return buf;
}

void WorkspaceState::log(const std::string& message) {
    activity.push_back({nowString(), message});
    if (activity.size() > 200) activity.erase(activity.begin());
}

std::string saveProject(const WorkspaceState& s, const std::string& path) {
    std::ofstream f(path, std::ios::trunc);
    if (!f) return "cannot write '" + path + "'";
    f << "aether_project=1\n"
      << "name=" << s.projectName << '\n'
      << "stl=" << s.stlPath << '\n'
      << "csv=" << s.csvPath << '\n';
    return f.good() ? std::string{} : "write error on '" + path + "'";
}

std::string loadProject(WorkspaceState& s, const std::string& path) {
    std::ifstream f(path);
    if (!f) return "cannot open '" + path + "'";
    std::string line;
    if (!std::getline(f, line) || line != "aether_project=1") return "not an Aether project file (missing 'aether_project=1' header)";
    WorkspaceState n;
    n.projectFile = path;
    while (std::getline(f, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        const auto eq = line.find('=');
        if (eq == std::string::npos) continue;
        const std::string k = line.substr(0, eq), v = line.substr(eq + 1);
        if (k == "name") n.projectName = v;
        else if (k == "stl") n.stlPath = v;
        else if (k == "csv") n.csvPath = v;
    }
    if (n.projectName.empty()) return "project file has no name";
    s = std::move(n);
    return {};
}

}  // namespace aether::application
