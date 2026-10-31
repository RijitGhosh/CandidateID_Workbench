#include <QApplication>
#include <QCommandLineParser>
#include <QElapsedTimer>
#include <QTimer>
#include <chrono>
#include <cstdio>

#include "themes/theme.hpp"
#include "views/main_window.hpp"

int main(int argc, char** argv) {
    QElapsedTimer launchTimer;
    launchTimer.start();  // launch-to-ready is measured from here (first line of main; process start-up by the OS is not included)
    QApplication app(argc, argv);
    app.setApplicationName("Aether Engineering Workbench");
    app.setApplicationVersion(aether::application::kAppVersion);
    app.setStyle("Fusion");

    QCommandLineParser cli;
    cli.addHelpOption();
    cli.addOption({"screenshots", "Render every page in both themes into <dir> and exit.", "dir"});
    cli.addOption({"selftest-ui", "Construct the window, show it, then exit with code 0 (used to verify launch)."});
    cli.process(app);

    ThemeManager::instance().init(app);
    MainWindow w(0);
    w.show();
    QApplication::processEvents();
    w.markReady(static_cast<double>(launchTimer.nsecsElapsed()) / 1.0e6);

    if (cli.isSet("screenshots")) {
        const bool ok = w.captureScreenshots(cli.value("screenshots"));
        std::printf("screenshots %s\n", ok ? "written" : "FAILED");
        return ok ? 0 : 1;
    }
    if (cli.isSet("selftest-ui")) {
        QTimer::singleShot(500, &app, &QApplication::quit);
        app.exec();
        std::printf("ui started and exited normally\n");
        return 0;
    }
    return app.exec();
}
