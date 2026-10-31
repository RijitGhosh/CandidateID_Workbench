#include <gtest/gtest.h>

#include <cstdio>
#include <filesystem>
#include <fstream>

#include "application/services/workspace.hpp"
#include "core/verification/selfcheck.hpp"

using namespace aether::application;

TEST(Workspace, ProjectRoundTrip) {
    WorkspaceState a;
    a.projectName = "Bracket study";
    a.stlPath = "data/geometry/example_bracket.stl";
    a.csvPath = "data/temperature/example_temperature.csv";
    const auto path = (std::filesystem::temp_directory_path() / "aether_test.aewb").string();
    ASSERT_EQ(saveProject(a, path), "");
    WorkspaceState b;
    ASSERT_EQ(loadProject(b, path), "");
    EXPECT_EQ(b.projectName, "Bracket study");
    EXPECT_EQ(b.stlPath, a.stlPath);
    EXPECT_EQ(b.csvPath, a.csvPath);
    std::remove(path.c_str());
}
TEST(Workspace, RejectsForeignFilesAndMissingFiles) {
    const auto path = (std::filesystem::temp_directory_path() / "aether_bad.aewb").string();
    std::ofstream(path) << "hello\n";
    WorkspaceState s;
    EXPECT_NE(loadProject(s, path), "");
    EXPECT_NE(loadProject(s, "/no/such/file.aewb"), "");
    std::remove(path.c_str());
}
TEST(Workspace, ActivityLogIsBoundedAndTimestamped) {
    WorkspaceState s;
    for (int i = 0; i < 250; ++i) s.log("event " + std::to_string(i));
    EXPECT_EQ(s.activity.size(), 200u);
    EXPECT_EQ(s.activity.back().message, "event 249");
    EXPECT_EQ(s.activity.back().timestamp.size(), 19u);
}
TEST(SelfCheck, AllInAppChecksExecuteAndPass) {
    const auto r = aether::verification::runAllChecks();
    ASSERT_EQ(r.size(), 8u);
    for (const auto& c : r) EXPECT_TRUE(c.passed) << c.name << " -> " << c.detail;
}
