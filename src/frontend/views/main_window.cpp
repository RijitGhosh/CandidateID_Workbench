#include "main_window.hpp"

#include <QActionGroup>
#include <QApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QMenu>
#include <QMessageBox>
#include <QPainter>
#include <QPixmap>
#include <QTimer>
#include <QVBoxLayout>

#include "../widgets/plot_widget.hpp"
#include "../widgets/ui_common.hpp"
#include "../widgets/viewport3d.hpp"

using namespace ui;

namespace {
const char* kNavNames[] = {"Overview", "Geometry", "Thermal Analysis", "Performance", "Hardware", "Neural Network", "Verification", "Reports and Documentation"};

QIcon logoIcon() {
    QPixmap pm(64, 64);
    pm.fill(Qt::transparent);
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setBrush(QColor("#8B5CF6"));
    p.setPen(Qt::NoPen);
    p.drawRoundedRect(QRectF(4, 4, 56, 56), 10, 10);
    p.setPen(QPen(Qt::white, 5, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    p.drawLine(QPointF(18, 46), QPointF(32, 16));
    p.drawLine(QPointF(32, 16), QPointF(46, 46));
    p.drawLine(QPointF(23, 36), QPointF(41, 36));
    return QIcon(pm);
}
}  // namespace

MainWindow::MainWindow(double, QWidget* parent) : QMainWindow(parent), ctx_(new AppContext) {
    setWindowTitle("Aether Engineering Workbench");
    setWindowIcon(logoIcon());
    resize(1440, 900);
    setMinimumSize(1100, 700);
    auto* central = new QWidget;
    setCentralWidget(central);
    auto* v = new QVBoxLayout(central);
    v->setContentsMargins(0, 0, 0, 0);
    v->setSpacing(0);

    // ---- top bar
    auto* top = new QWidget;
    top->setObjectName("TopBar");
    top->setFixedHeight(52);
    auto* th = new QHBoxLayout(top);
    th->setContentsMargins(16, 0, 12, 0);
    th->setSpacing(10);
    auto* logo = new QLabel;
    logo->setPixmap(logoIcon().pixmap(26, 26));
    th->addWidget(logo);
    th->addWidget(label("Aether Engineering Workbench", "ProductName"));
    th->addSpacing(18);
    th->addWidget(label("Project", "Caption"));
    projectLabel_ = label("", "Mono");
    th->addWidget(projectLabel_);
    auto* openB = new QPushButton("Open project...");
    auto* saveB = new QPushButton("Save project...");
    th->addWidget(openB);
    th->addWidget(saveB);
    th->addStretch();
    appStatus_ = label("", "Mono");
    th->addWidget(appStatus_);
    themeBtn_ = new QToolButton;
    themeBtn_->setObjectName("ThemeToggle");
    themeBtn_->setToolTip("Theme: Dark / Light / System");
    themeBtn_->setPopupMode(QToolButton::InstantPopup);
    themeBtn_->setIconSize(QSize(20, 20));
    themeBtn_->setFixedSize(38, 34);
    auto* menu = new QMenu(themeBtn_);
    auto* grp = new QActionGroup(menu);
    struct M { const char* t; ThemeMode m; };
    for (M m : {M{"Dark", ThemeMode::Dark}, M{"Light", ThemeMode::Light}, M{"System", ThemeMode::System}}) {
        QAction* a = menu->addAction(m.t);
        a->setCheckable(true);
        a->setActionGroup(grp);
        a->setChecked(ThemeManager::instance().mode() == m.m);
        connect(a, &QAction::triggered, this, [m] { ThemeManager::instance().setMode(m.m); });
    }
    themeBtn_->setMenu(menu);
    themeBtn_->setPopupMode(QToolButton::InstantPopup);
    th->addWidget(themeBtn_);
    v->addWidget(top);

    // ---- body
    auto* body = new QHBoxLayout;
    body->setContentsMargins(0, 0, 0, 0);
    body->setSpacing(0);
    auto* side = new QWidget;
    side->setObjectName("Sidebar");
    side->setFixedWidth(220);
    auto* sl = new QVBoxLayout(side);
    sl->setContentsMargins(0, 12, 0, 12);
    sl->setSpacing(1);
    auto* grpNav = new QButtonGroup(this);
    grpNav->setExclusive(true);
    for (int i = 0; i < 8; ++i) {
        auto* b = new QPushButton(kNavNames[i]);
        b->setObjectName("NavButton");
        b->setCheckable(true);
        b->setCursor(Qt::PointingHandCursor);
        grpNav->addButton(b, i);
        sl->addWidget(b);
        navButtons_.push_back(b);
        connect(b, &QPushButton::clicked, this, [this, i] { showPage(i); });
    }
    sl->addStretch();
    auto* ver = label(QString("v%1").arg(aether::application::kAppVersion), "Caption");
    ver->setContentsMargins(16, 0, 0, 0);
    sl->addWidget(ver);
    body->addWidget(side);

    stack_ = new QStackedWidget;
    overview_ = new OverviewPage(ctx_);
    geometry_ = new GeometryPage(ctx_);
    thermal_ = new ThermalPage(ctx_);
    performance_ = new PerformancePage(ctx_);
    hardware_ = new HardwarePage(ctx_);
    neural_ = new NeuralPage(ctx_);
    verification_ = new VerificationPage(ctx_);
    reports_ = new ReportsPage(ctx_);
    for (QWidget* w : std::initializer_list<QWidget*>{overview_, geometry_, thermal_, performance_, hardware_, neural_, verification_, reports_}) stack_->addWidget(w);
    body->addWidget(stack_, 1);
    v->addLayout(body, 1);

    // ---- status bar
    auto* sb = new QWidget;
    sb->setObjectName("StatusBar");
    sb->setFixedHeight(34);
    auto* sh = new QHBoxLayout(sb);
    sh->setContentsMargins(14, 0, 14, 0);
    sh->setSpacing(12);
    auto* ready = label("", "Mono");
    ready->setObjectName("Mono");
    sh->addWidget(ready);
    taskLabel_ = label("Ready", "Secondary");
    sh->addWidget(taskLabel_, 1);
    progress_ = new QProgressBar;
    progress_->setFixedWidth(220);
    progress_->setRange(0, 1000);
    progress_->setVisible(false);
    sh->addWidget(progress_);
    cancel_ = new QPushButton("Cancel");
    cancel_->setVisible(false);
    sh->addWidget(cancel_);
    v->addWidget(sb);
    // appStatus_ in the top bar mirrors readiness
    connect(cancel_, &QPushButton::clicked, ctx_, &AppContext::cancelRequested);

    connect(ctx_, &AppContext::taskStarted, this, [this](const QString& t, bool c) {
        taskLabel_->setText(t);
        progress_->setVisible(true);
        progress_->setRange(0, 0);
        cancel_->setVisible(c);
        setStatus("Busy", "busy");
    });
    connect(ctx_, &AppContext::taskProgress, this, [this](double f) {
        if (f < 0) { progress_->setRange(0, 0); return; }
        progress_->setRange(0, 1000);
        progress_->setValue(static_cast<int>(f * 1000));
    });
    connect(ctx_, &AppContext::taskFinished, this, [this](const QString& m) {
        taskLabel_->setText(m);
        progress_->setVisible(false);
        cancel_->setVisible(false);
        setStatus("Ready", "ok");
    });
    connect(ctx_, &AppContext::taskFailed, this, [this](const QString& m) {
        taskLabel_->setText(m);
        progress_->setVisible(false);
        cancel_->setVisible(false);
        setStatus("Error", "error");
    });
    connect(ctx_, &AppContext::navigate, this, &MainWindow::showPage);
    connect(ctx_, &AppContext::stateChanged, this, [this] {
        projectLabel_->setText(QString::fromStdString(ctx_->state.projectName));
    });
    connect(openB, &QPushButton::clicked, this, &MainWindow::openProject);
    connect(saveB, &QPushButton::clicked, this, &MainWindow::saveProject);
    connect(&ThemeManager::instance(), &ThemeManager::changed, this, &MainWindow::onThemeChanged);

    projectLabel_->setText(QString::fromStdString(ctx_->state.projectName));
    ready->setText("");
    setStatus("Ready", "ok");
    onThemeChanged();
    showPage(0);
    ctx_->log("Application started");
}

void MainWindow::markReady(double ms) {
    overview_->setLaunchToReadyMs(ms);
    performance_->setLaunchToReadyMs(ms);
}

void MainWindow::showPage(int i) {
    stack_->setCurrentIndex(i);
    for (int k = 0; k < static_cast<int>(navButtons_.size()); ++k) navButtons_[k]->setChecked(k == i);
    if (i == PageOverview) overview_->refresh();
}

void MainWindow::setStatus(const QString& text, const QString& kind) {
    const Palette& p = ThemeManager::instance().palette();
    const QColor c = kind == "ok" ? p.success : kind == "busy" ? p.warning : p.error;
    appStatus_->setText(QString("\xE2\x97\x8F %1").arg(text));
    appStatus_->setStyleSheet(QString("color:%1;").arg(c.name()));
}

void MainWindow::onThemeChanged() {
    updateThemeIcon();
    const bool ok = appStatus_->text().contains("Ready");
    if (ok) setStatus("Ready", "ok");
}

void MainWindow::updateThemeIcon() {
    const Palette& p = ThemeManager::instance().palette();
    const ThemeMode m = ThemeManager::instance().mode();
    themeBtn_->setIcon(m == ThemeMode::System ? systemIcon(p.text) : p.dark ? moonIcon(p.text) : sunIcon(p.text));
    if (auto* menu = themeBtn_->menu()) {
        const auto acts = menu->actions();
        for (int i = 0; i < acts.size(); ++i) acts[i]->setChecked((m == ThemeMode::Dark && i == 0) || (m == ThemeMode::Light && i == 1) || (m == ThemeMode::System && i == 2));
    }
}

void MainWindow::openProject() {
    const QString p = QFileDialog::getOpenFileName(this, "Open project", QString(), "Aether project (*.aewb)");
    if (p.isEmpty()) return;
    const std::string err = aether::application::loadProject(ctx_->state, p.toStdString());
    if (!err.empty()) {
        QMessageBox::warning(this, "Open project", QString::fromStdString(err));
        emit ctx_->taskFailed("Open project failed: " + QString::fromStdString(err));
        return;
    }
    if (!ctx_->state.stlPath.empty() && QFile::exists(QString::fromStdString(ctx_->state.stlPath))) geometry_->loadFile(QString::fromStdString(ctx_->state.stlPath));
    if (!ctx_->state.csvPath.empty() && QFile::exists(QString::fromStdString(ctx_->state.csvPath))) thermal_->loadCsv(QString::fromStdString(ctx_->state.csvPath));
    ctx_->log("Opened project " + p);
    emit ctx_->taskFinished("Project opened");
}

void MainWindow::saveProject() {
    const QString p = QFileDialog::getSaveFileName(this, "Save project", QString::fromStdString(ctx_->state.projectName) + ".aewb", "Aether project (*.aewb)");
    if (p.isEmpty()) return;
    ctx_->state.projectName = QFileInfo(p).completeBaseName().toStdString();
    const std::string err = aether::application::saveProject(ctx_->state, p.toStdString());
    if (!err.empty()) {
        QMessageBox::warning(this, "Save project", QString::fromStdString(err));
        emit ctx_->taskFailed("Save failed: " + QString::fromStdString(err));
        return;
    }
    ctx_->state.projectFile = p.toStdString();
    ctx_->log("Saved project " + p);
    emit ctx_->taskFinished("Project saved");
}

namespace {
void pump(int ms) {
    QElapsedTimer t;
    t.start();
    while (t.elapsed() < ms) QApplication::processEvents(QEventLoop::AllEvents, 20);
}
QPushButton* findButton(QWidget* root, const QString& text) {
    for (auto* b : root->findChildren<QPushButton*>())
        if (b->text() == text) return b;
    return nullptr;
}
bool waitUntilEnabled(QPushButton* b, int timeoutMs) {
    QElapsedTimer t;
    t.start();
    while (!b->isEnabled() && t.elapsed() < timeoutMs) QApplication::processEvents(QEventLoop::AllEvents, 20);
    return b->isEnabled();
}
}  // namespace

bool MainWindow::captureScreenshots(const QString& outDir) {
    QDir().mkpath(outDir);
    bool ok = true;
    auto shot = [&](const QString& name) {
        pump(150);
        const bool w = grab().save(outDir + "/" + name + ".png");
        if (!w) ok = false;
    };
    const QString dataRoot = dataDir();
    for (ThemeMode mode : {ThemeMode::Dark, ThemeMode::Light}) {
        ThemeManager::instance().setMode(mode);
        const QString t = mode == ThemeMode::Dark ? "dark" : "light";
        show();
        showPage(PageOverview);
        shot("01_overview_empty_" + t);
        showPage(PageGeometry);
        geometry_->loadFile(dataRoot + "/geometry/example_bracket.stl");
        shot("02_geometry_" + t);
        showPage(PageThermal);
        thermal_->tabs()->setCurrentIndex(0);
        thermal_->solveDefault();
        if (auto* b = findButton(thermal_, "Mesh study N = 10, 20, 40, 80")) b->click();
        shot("03_thermal_fe_" + t);
        thermal_->tabs()->setCurrentIndex(1);
        thermal_->loadCsv(dataRoot + "/temperature/example_temperature.csv");
        shot("04_temperature_cloud_" + t);
        showPage(PagePerformance);
        if (auto* b = findButton(performance_, "Run benchmark")) {
            b->click();
            ok = waitUntilEnabled(b, 180000) && ok;
        }
        shot("05_performance_" + t);
        showPage(PageHardware);
        for (const char* label : {"Discover + capabilities", "Connect", "Read one telemetry message", "Inject timeout fault", "Read one telemetry message", "Recover", "Read one telemetry message"})
            for (auto* b : hardware_->findChildren<QPushButton*>())
                if (b->text() == label) b->click();
        shot("06_hardware_" + t);
        showPage(PageNeural);
        neural_->compute();
        shot("07_neural_" + t);
        showPage(PageVerification);
        verification_->runChecks();
        if (auto* b = findButton(verification_, "Run self-checks")) ok = waitUntilEnabled(b, 60000) && ok;
        shot("08_verification_" + t);
        showPage(PageReports);
        shot("09_reports_" + t);
        showPage(PageOverview);
        shot("10_overview_active_" + t);
        {   // plots rendered by the same PlotWidget, saved as standalone images
            QDir().mkpath(outDir + "/plots");
            int k = 0;
            const QStringList names = {"thermal_profile", "thermal_convergence"};
            for (auto* pw : thermal_->findChildren<PlotWidget*>()) { pw->resize(760, 460); ok = pw->saveImage(QString("%1/plots/%2_%3.png").arg(outDir, names.value(k++, "thermal_plot"), t)) && ok; }
            for (auto* pw : neural_->findChildren<PlotWidget*>()) { pw->resize(760, 460); ok = pw->saveImage(QString("%1/plots/backprop_loss_%2.png").arg(outDir, t)) && ok; }
            for (auto* pw : performance_->findChildren<PlotWidget*>()) { pw->resize(760, 460); ok = pw->saveImage(QString("%1/plots/benchmark_speedup_%2.png").arg(outDir, t)) && ok; }
        }
    }
    return ok;
}
