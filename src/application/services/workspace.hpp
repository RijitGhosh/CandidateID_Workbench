// Aether application layer (Qt-free): project state, activity log, project file I/O.
#pragma once

#include <chrono>
#include <string>
#include <vector>

namespace aether::application {

constexpr const char* kAppVersion = "1.0.0";

struct ActivityEntry {
    std::string timestamp;  // local time, "YYYY-MM-DD HH:MM:SS"
    std::string message;
};

// Everything shown on the Overview page comes from this state; nothing is pre-filled.
struct WorkspaceState {
    std::string projectName = "Untitled project";
    std::string projectFile;   // empty if never saved
    std::string stlPath;       // last imported STL (empty = none)
    std::string csvPath;       // last imported CSV (empty = none)
    std::string lastAnalysis;  // e.g. "1-D thermal FE, N=80 (max midpoint error 1.2e-3 C)"; empty = none run yet
    std::string lastAnalysisTime;
    std::string verificationSummary;  // empty = not run in this session
    std::vector<ActivityEntry> activity;

    void log(const std::string& message);
};

std::string nowString();

// Plain-text "key=value" project file (.aewb). Returns an empty string on success or an error message.
std::string saveProject(const WorkspaceState& s, const std::string& path);
std::string loadProject(WorkspaceState& s, const std::string& path);

}  // namespace aether::application
