#include <QApplication>
#include <QDesktopServices>
#include <QFile>
#include <QFileDialog>
#include <QFileInfo>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSpinBox>
#include <QTabWidget>
#include <QTableWidget>
#include <QTextBrowser>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <atomic>
#include <fstream>
#include <thread>

#include "../../core/neural/backprop.hpp"
#include "../../core/performance/batch.hpp"
#include "../../core/verification/selfcheck.hpp"
#include "../widgets/plot_widget.hpp"
#include "../widgets/ui_common.hpp"
#include "pages.hpp"

using namespace ui;
namespace hw = aether::hardware;
namespace perf = aether::performance;
namespace nn = aether::neural;

namespace {
QTableWidget* makeTable(const QStringList& headers) {
    auto* t = new QTableWidget(0, headers.size());
    t->setHorizontalHeaderLabels(headers);
    t->setEditTriggers(QAbstractItemView::NoEditTriggers);
    t->setSelectionBehavior(QAbstractItemView::SelectRows);
    t->setAlternatingRowColors(true);
    t->verticalHeader()->setVisible(false);
    t->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    return t;
}
void addRow(QTableWidget* t, const QStringList& cells, const QColor& lastColor = QColor()) {
    const int r = t->rowCount();
    t->insertRow(r);
    for (int c = 0; c < cells.size(); ++c) {
        auto* it = new QTableWidgetItem(cells[c]);
        if (lastColor.isValid() && c == cells.size() - 1) it->setForeground(lastColor);
        t->setItem(r, c, it);
    }
}
QWidget* pageRoot(QWidget* self, QVBoxLayout*& root, const QString& title, const QString& desc) {
    self->setObjectName("Page");
    root = new QVBoxLayout(self);
    root->setContentsMargins(24, 20, 24, 16);
    root->setSpacing(10);
    root->addWidget(pageHeader(title, desc));
    return self;
}
}  // namespace

// ============================================================ Overview
OverviewPage::OverviewPage(AppContext* ctx, QWidget* parent) : QWidget(parent), ctx_(ctx) {
    QVBoxLayout* root;
    pageRoot(this, root, "Overview", "Workspace status and module state. Every value below reflects what has actually been done in this session.");
    auto* top = new QHBoxLayout;
    auto* info = panel();
    auto* g = new QGridLayout(info);
    g->setContentsMargins(14, 12, 14, 12);
    g->setHorizontalSpacing(18);
    g->setVerticalSpacing(6);
    auto addKV = [&](int row, const QString& k, QLabel*& v) {
        g->addWidget(label(k, "Secondary"), row, 0);
        v = label("", "Mono");
        v->setWordWrap(true);
        g->addWidget(v, row, 1);
    };
    addKV(0, "Project", project_);
    addKV(1, "Application version", version_);
    addKV(2, "Workspace", workspace_);
    addKV(3, "Last successful analysis", lastAnalysis_);
    addKV(4, "Verification", verification_);
    addKV(5, "Launch-to-ready", launchReady_);
    g->setColumnStretch(1, 1);
    top->addWidget(info, 3);
    auto* actions = panel();
    auto* al = new QVBoxLayout(actions);
    al->setContentsMargins(14, 12, 14, 12);
    al->addWidget(label("OPEN MODULE", "SectionTitle"));
    const QList<QPair<QString, int>> btns = {{"Geometry", PageGeometry}, {"Thermal Analysis", PageThermal}, {"Performance", PagePerformance}, {"Hardware", PageHardware}};
    for (const auto& b : btns) {
        auto* pb = new QPushButton(b.first);
        connect(pb, &QPushButton::clicked, this, [this, b] { emit ctx_->navigate(b.second); });
        al->addWidget(pb);
    }
    al->addStretch();
    top->addWidget(actions, 1);
    root->addLayout(top);

    root->addWidget(label("MODULES", "SectionTitle"));
    modules_ = makeTable({"Module", "State"});
    modules_->setMinimumHeight(262);
    modules_->setMaximumHeight(280);
    modules_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    modules_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    root->addWidget(modules_);
    root->addWidget(label("RECENT ACTIVITY", "SectionTitle"));
    activity_ = new QListWidget;
    root->addWidget(activity_, 1);
    connect(ctx_, &AppContext::stateChanged, this, &OverviewPage::refresh);
    connect(&ThemeManager::instance(), &ThemeManager::changed, this, &OverviewPage::refresh);
    refresh();
}

void OverviewPage::setLaunchToReadyMs(double ms) {
    launchReady_->setText(QString("%1 ms (process start to main window shown, measured on this run)").arg(ms, 0, 'f', 0));
}

void OverviewPage::refresh() {
    const auto& s = ctx_->state;
    const Palette& p = ThemeManager::instance().palette();
    project_->setText(QString::fromStdString(s.projectName) + (s.projectFile.empty() ? "  (unsaved)" : ""));
    version_->setText(aether::application::kAppVersion);
    const bool any = !s.stlPath.empty() || !s.csvPath.empty() || !s.lastAnalysis.empty();
    workspace_->setText(any ? "Active: data loaded or analysis run in this session" : "Empty: nothing loaded yet");
    lastAnalysis_->setText(s.lastAnalysis.empty() ? "None run yet" : QString::fromStdString(s.lastAnalysis + "  [" + s.lastAnalysisTime + "]"));
    verification_->setText(s.verificationSummary.empty() ? "Not run in this session" : QString::fromStdString(s.verificationSummary));
    modules_->setRowCount(0);
    auto row = [&](const QString& name, const QString& state, bool ok) { addRow(modules_, {name, state}, ok ? p.success : p.textSecondary); };
    row("Geometry (ASCII STL)", s.stlPath.empty() ? "No model loaded" : "Loaded: " + QFileInfo(QString::fromStdString(s.stlPath)).fileName(), !s.stlPath.empty());
    row("Thermal FE solver", s.lastAnalysis.empty() ? "Available, not run" : "Last run: " + QString::fromStdString(s.lastAnalysisTime), !s.lastAnalysis.empty());
    row("Temperature results (CSV)", s.csvPath.empty() ? "No result file loaded" : "Loaded: " + QFileInfo(QString::fromStdString(s.csvPath)).fileName(), !s.csvPath.empty());
    row("Performance", "Benchmark available (run it on the Performance page)", false);
    row("Hardware", "Simulated devices only; no physical hardware", false);
    row("Neural network", "Backpropagation available", false);
    row("Verification", s.verificationSummary.empty() ? "Not run in this session" : QString::fromStdString(s.verificationSummary), !s.verificationSummary.empty());
    activity_->clear();
    for (auto it = s.activity.rbegin(); it != s.activity.rend(); ++it)
        activity_->addItem(QString::fromStdString(it->timestamp + "   " + it->message));
    if (s.activity.empty()) activity_->addItem("No activity yet");
}

// ============================================================ Performance
struct PerformancePage::Impl {
    QFutureWatcher<perf::BenchmarkReport> watcher;
    perf::CancelToken cancel;
    perf::BenchmarkReport report;
    bool have = false;
    bool running = false;
    std::shared_ptr<std::atomic<int>> progressPermille = std::make_shared<std::atomic<int>>(0);
};

PerformancePage::PerformancePage(AppContext* ctx, QWidget* parent) : QWidget(parent), ctx_(ctx), d_(std::make_shared<Impl>()) {
    QVBoxLayout* root;
    pageRoot(this, root, "Performance", "Serial versus parallel execution of independent 1-D thermal solves. Times are measured on this machine; nothing is estimated.");
    auto* bar = new QHBoxLayout;
    cases_ = new QSpinBox;
    cases_->setRange(1, 200000);
    cases_->setValue(2000);
    mesh_ = new QSpinBox;
    mesh_->setRange(1, 100000);
    mesh_->setValue(2000);
    bar->addWidget(label("Cases", "Secondary"));
    bar->addWidget(cases_);
    bar->addWidget(label("Mesh (elements)", "Secondary"));
    bar->addWidget(mesh_);
    bar->addWidget(label("Workers: 1, 2, 3, 4 (capped at logical CPUs)", "Secondary"));
    bar->addStretch();
    runBtn_ = new QPushButton("Run benchmark");
    runBtn_->setObjectName("Primary");
    saveBtn_ = new QPushButton("Save report to results/benchmarks");
    saveBtn_->setEnabled(false);
    bar->addWidget(runBtn_);
    bar->addWidget(saveBtn_);
    root->addLayout(bar);
    status_ = label("Not run yet. Protocol: 1 warm-up + 5 measured repetitions per worker count; median reported; fixed random seed.", "Secondary");
    status_->setWordWrap(true);
    root->addWidget(status_);
    launch_ = label("", "Mono");
    root->addWidget(launch_);
    auto* mid = new QHBoxLayout;
    table_ = makeTable({"Workers", "Median [s]", "Speedup T1/Tp", "Efficiency", "Max |diff| vs serial [\xC2\xB0""C]", "Note"});
    mid->addWidget(table_, 3);
    plot_ = new PlotWidget;
    plot_->setTitle("Speedup vs workers");
    plot_->setAxisTitles("Workers p", "Speedup T1 / Tp");
    plot_->setPlaceholder("Run the benchmark");
    mid->addWidget(plot_, 2);
    root->addLayout(mid, 2);
    info_ = new QPlainTextEdit;
    info_->setObjectName("Log");
    info_->setReadOnly(true);
    root->addWidget(info_, 2);
    connect(runBtn_, &QPushButton::clicked, this, &PerformancePage::run);
    connect(saveBtn_, &QPushButton::clicked, this, &PerformancePage::saveReport);
    connect(ctx_, &AppContext::cancelRequested, this, [this] { if (d_->running) d_->cancel.cancel(); });
    connect(&d_->watcher, &QFutureWatcher<perf::BenchmarkReport>::finished, this, [this] {
        d_->running = false;
        runBtn_->setEnabled(true);
        const auto rep = d_->watcher.result();
        if (rep.cancelled) {
            status_->setText("Benchmark cancelled.");
            ctx_->log("Benchmark cancelled");
            emit ctx_->taskFinished("Benchmark cancelled");
            return;
        }
        showReport(rep);
        ctx_->log(QString("Benchmark finished (%1 cases, N=%2)").arg(rep.config.cases).arg(rep.config.elements));
        emit ctx_->taskFinished("Benchmark complete");
    });
}

void PerformancePage::setLaunchToReadyMs(double ms) {
    launch_->setText(QString("Launch-to-ready (reported separately, not part of any timed region): %1 ms").arg(ms, 0, 'f', 0));
}

void PerformancePage::runBenchmarkNow() { run(); }

void PerformancePage::run() {
    if (d_->running) return;
    perf::BenchmarkConfig cfg;
    cfg.cases = static_cast<std::size_t>(cases_->value());
    cfg.elements = mesh_->value();
    if (cfg.cases * (static_cast<std::size_t>(cfg.elements) + 1) > perf::kMaxResultDoubles) {
        QMessageBox::warning(this, "Benchmark", "cases x (mesh+1) exceeds the 25M-value memory limit. Reduce the case count or mesh size.");
        return;
    }
    d_->cancel.cancelled = false;
    d_->running = true;
    runBtn_->setEnabled(false);
    status_->setText("Running... (UI stays responsive; use Cancel in the status bar)");
    emit ctx_->taskStarted("Benchmark", true);
    auto prog = d_->progressPermille;
    auto* ctxp = ctx_;
    d_->watcher.setFuture(QtConcurrent::run([cfg, prog, this] {
        return perf::runBenchmark(cfg, &d_->cancel, [prog](double f, const std::string&) { prog->store(static_cast<int>(f * 1000)); });
    }));
    auto* timer = new QTimer(this);
    connect(timer, &QTimer::timeout, this, [this, timer, prog, ctxp] {
        if (!d_->running) { timer->deleteLater(); return; }
        emit ctxp->taskProgress(prog->load() / 1000.0);
    });
    timer->start(100);
}

void PerformancePage::showReport(const perf::BenchmarkReport& r) {
    d_->report = r;
    d_->have = true;
    saveBtn_->setEnabled(true);
    const Palette& p = ThemeManager::instance().palette();
    table_->setRowCount(0);
    QPlainTextEdit* i = info_;
    PlotSeries meas;
    meas.name = "Measured";
    meas.color = p.primary;
    meas.markers = true;
    PlotSeries ideal;
    ideal.name = "Ideal (speedup = p)";
    ideal.color = p.textSecondary;
    ideal.dashed = true;
    for (const auto& row : r.rows) {
        if (row.measured) {
            addRow(table_, {QString::number(row.workersRequested), fmt(row.medianSeconds, 6), fmt(row.speedup, 4), fmt(row.efficiency, 4), QString::number(row.maxAbsDiffVsSerial_C, 'e', 2), ""});
            meas.x.push_back(row.workersRequested);
            meas.y.push_back(row.speedup);
            ideal.x.push_back(row.workersRequested);
            ideal.y.push_back(row.workersRequested);
        } else {
            addRow(table_, {QString::number(row.workersRequested), "-", "-", "-", "-", QString::fromStdString(row.note)});
        }
    }
    plot_->setSeries({ideal, meas});
    const auto& s = r.system;
    QString t;
    t += QString("CPU model        : %1\n").arg(QString::fromStdString(s.cpuModel));
    t += QString("Physical cores   : %1\n").arg(s.physicalCores ? QString::number(s.physicalCores) : "not reported by the OS");
    t += QString("Logical cores    : %1\n").arg(s.logicalCores);
    t += QString("RAM              : %1 GiB\n").arg(s.ramGiB, 0, 'f', 2);
    t += QString("OS               : %1\n").arg(QString::fromStdString(s.os));
    t += QString("Compiler         : %1 %2\n").arg(QString::fromStdString(s.compiler), QString::fromStdString(s.compilerVersion));
    t += QString("Build / flags    : %1 / %2\n").arg(QString::fromStdString(s.buildType), QString::fromStdString(s.optimisationFlags).trimmed());
    t += QString("Workload         : independent 1-D thermal FE solves, %1 cases, %2 elements, seed %3\n").arg(r.config.cases).arg(r.config.elements).arg(r.config.seed);
    t += "Timing policy    : 1 warm-up, 5 repetitions, median; compute only (no file I/O, rendering or UI)\n";
    t += QString("Agreement check  : max|T_serial - T_parallel| <= 1e-8 C : %1\n").arg(r.agreementOk ? "PASS" : "FAIL");
    if (!r.speedupAssessable) t += "NOTE: only one logical CPU is available; the parallel path ran but speedup cannot be assessed.\n";
    t += "\nWhy speedup can be lower than p: thread start/join cost, work-queue contention, small workloads that finish before all\n"
         "workers are busy, shared memory bandwidth and cache, hyper-threads sharing one physical core, and other processes on the machine.\n"
         "Memory: results are stored case-major in one array of cases x (mesh+1) doubles (capped at 25M values).";
    i->setPlainText(t);
    status_->setText(QString("<span style='color:%1'>Benchmark complete.</span>").arg(r.agreementOk ? p.success.name() : p.error.name()));
}

void PerformancePage::saveReport() {
    if (!d_->have) return;
    const QString dir = resultsDir() + "/benchmarks";
    QDir().mkpath(dir);
    std::ofstream(QString(dir + "/benchmark_gui.md").toStdString()) << perf::toMarkdown(d_->report);
    std::ofstream(QString(dir + "/benchmark_gui.json").toStdString()) << perf::toJson(d_->report);
    ctx_->log("Saved benchmark report to " + dir);
    emit ctx_->taskFinished("Saved benchmark report to " + dir);
}

// ============================================================ Hardware
struct HardwarePage::Card {
    QLabel *title, *board, *state;
    QPlainTextEdit* caps;
};

HardwarePage::HardwarePage(AppContext* ctx, QWidget* parent) : QWidget(parent), ctx_(ctx), devices_(hw::makeMockDeviceSet()) {
    QVBoxLayout* root;
    pageRoot(this, root, "Hardware", "Modular host-connected system: microcontroller board, FPGA board and JTAG debug adapter.");
    auto* banner = panel("Banner");
    auto* bl = new QVBoxLayout(banner);
    bl->setContentsMargins(12, 8, 12, 8);
    bl->addWidget(label("SIMULATED: every device on this page is a software mock. No physical hardware is connected or was tested.", "ProductName"));
    auto* b2 = label("The boards named below are the documented example parts the mocks model; electrical details are in docs/embedded_design.md.", "Caption");
    b2->setWordWrap(true);
    bl->addWidget(b2);
    root->addWidget(banner);
    auto* row = new QHBoxLayout;
    for (std::size_t i = 0; i < devices_.size(); ++i) {
        auto* c = new Card;
        auto* box = panel();
        auto* l = new QVBoxLayout(box);
        l->setContentsMargins(12, 12, 12, 12);
        l->setSpacing(6);
        c->title = label(QString::fromUtf8(hw::toString(devices_[i]->info().kind)) + "  (simulated)", "ProductName");
        c->board = label("Models: " + QString::fromStdString(devices_[i]->info().modelsBoard), "Caption");
        c->board->setWordWrap(true);
        c->state = label("", "Mono");
        c->caps = new QPlainTextEdit;
        c->caps->setReadOnly(true);
        c->caps->setObjectName("Log");
        c->caps->setMaximumHeight(110);
        l->addWidget(c->title);
        l->addWidget(c->board);
        l->addWidget(c->state);
        l->addWidget(c->caps);
        auto mk = [&](const QString& text, std::function<void()> fn) {
            auto* b = new QPushButton(text);
            connect(b, &QPushButton::clicked, this, [fn] { fn(); });
            l->addWidget(b);
        };
        const int idx = static_cast<int>(i);
        mk("Discover + capabilities", [this, idx] {
            auto& d = *devices_[idx];
            d.discover();
            hw::Capabilities cp;
            const auto s = d.capabilities(cp);
            logLine(QString("[%1] discover: %2").arg(QString::fromStdString(d.info().id), s.ok() ? "ok" : QString::fromStdString(s.message)));
            QString t = "Interfaces:\n";
            for (const auto& x : cp.interfaces) t += "  " + QString::fromStdString(x) + "\n";
            t += "Features:\n";
            for (const auto& x : cp.features) t += "  " + QString::fromStdString(x) + "\n";
            cards_[idx]->caps->setPlainText(t);
            refreshCard(idx);
        });
        mk("Connect", [this, idx] { const auto s = devices_[idx]->connect(); logLine(QString("[%1] connect: %2").arg(QString::fromStdString(devices_[idx]->info().id), s.ok() ? "ok (simulated)" : QString::fromStdString(s.message))); refreshCard(idx); });
        mk("Read one telemetry message", [this, idx] {
            hw::Telemetry t;
            const auto s = devices_[idx]->readTelemetry(t);
            if (s.ok()) {
                QString m = QString("[%1] telemetry #%2 [SIMULATED]:").arg(QString::fromStdString(t.deviceId)).arg(t.sequence);
                for (const auto& ch : t.channels) m += QString("  %1=%2").arg(QString::fromStdString(ch.first)).arg(ch.second);
                logLine(m);
            } else {
                logLine(QString("[%1] telemetry FAILED: %2 (%3)").arg(QString::fromStdString(devices_[idx]->info().id), hw::toString(s.code), QString::fromStdString(s.message)));
                emit ctx_->taskFailed("Simulated device error: " + QString::fromStdString(s.message));
            }
            refreshCard(idx);
        });
        mk("Inject timeout fault", [this, idx] { devices_[idx]->armTimeoutFault(); logLine(QString("[%1] timeout armed: next telemetry read will time out").arg(QString::fromStdString(devices_[idx]->info().id))); });
        mk("Recover", [this, idx] { const auto s = devices_[idx]->recover(); logLine(QString("[%1] recover: %2").arg(QString::fromStdString(devices_[idx]->info().id), s.ok() ? "ok, link restored (simulated)" : QString::fromStdString(s.message))); refreshCard(idx); if (s.ok()) emit ctx_->taskFinished("Simulated device recovered"); });
        mk("Disconnect", [this, idx] { const auto s = devices_[idx]->disconnect(); logLine(QString("[%1] disconnect: %2").arg(QString::fromStdString(devices_[idx]->info().id), s.ok() ? "ok" : QString::fromStdString(s.message))); refreshCard(idx); });
        l->addStretch();
        row->addWidget(box);
        cards_.push_back(c);
    }
    root->addLayout(row, 3);
    root->addWidget(label("DEVICE LOG", "SectionTitle"));
    log_ = new QPlainTextEdit;
    log_->setObjectName("Log");
    log_->setReadOnly(true);
    root->addWidget(log_, 2);
    for (int i = 0; i < static_cast<int>(devices_.size()); ++i) refreshCard(i);
}

void HardwarePage::logLine(const QString& s) {
    log_->appendPlainText(s);
    ctx_->log("Hardware (simulated): " + s.left(70));
}

void HardwarePage::refreshCard(int i) {
    const auto st = devices_[i]->state();
    const Palette& p = ThemeManager::instance().palette();
    const QColor c = st == hw::DeviceState::Connected ? p.success : st == hw::DeviceState::Faulted ? p.error : p.textSecondary;
    cards_[i]->state->setText(QString("State: %1").arg(hw::toString(st)));
    cards_[i]->state->setStyleSheet(QString("color:%1;").arg(c.name()));
}

// ============================================================ Neural
NeuralPage::NeuralPage(AppContext* ctx, QWidget* parent) : QWidget(parent), ctx_(ctx) {
    QVBoxLayout* root;
    pageRoot(this, root, "Neural Network", "One-sample regression, h = tanh(wx + b), \xC5\xB7 = vh + c, J = \xC2\xBD(\xC5\xB7 \xE2\x88\x92 y)\xC2\xB2. Hand-derived backpropagation, no automatic differentiation.");
    auto* bar = new QHBoxLayout;
    auto* run = new QPushButton("Compute gradients and train");
    run->setObjectName("Primary");
    bar->addWidget(run);
    verdict_ = label("", "Secondary");
    bar->addWidget(verdict_, 1);
    root->addLayout(bar);
    auto* split = new QHBoxLayout;
    auto* left = new QVBoxLayout;
    left->addWidget(label("GRADIENTS AT INITIAL PARAMETERS  (w=0.5, b=0, v=1, c=0; x=1, y=2)", "SectionTitle"));
    grad_ = makeTable({"Parameter", "Analytic", "Central difference (\xCE\xB5=1e-6)", "|error|", "\xE2\x89\xA4 1e-5"});
    left->addWidget(grad_);
    left->addWidget(label("ONE SIMULTANEOUS UPDATE (learning rate 0.1)", "SectionTitle"));
    step_ = makeTable({"Parameter", "Before", "After"});
    left->addWidget(step_);
    split->addLayout(left, 3);
    loss_ = new PlotWidget;
    loss_->setTitle("Loss vs update step");
    loss_->setAxisTitles("Training step", "Loss J");
    loss_->setLog(false, true);
    loss_->setPlaceholder("Press Compute");
    split->addWidget(loss_, 3);
    root->addLayout(split, 3);
    text_ = new QTextBrowser;
    root->addWidget(text_, 2);
    connect(run, &QPushButton::clicked, this, &NeuralPage::compute);
}

void NeuralPage::compute() {
    nn::Params p0;
    nn::Problem pr;
    const auto chk = nn::checkGradients(p0, pr, 1e-6, 1e-5);
    const Palette& pal = ThemeManager::instance().palette();
    grad_->setRowCount(0);
    struct R { const char* n; double a, f, e; };
    const R rows[] = {{"dJ/dw", chk.analytic.dw, chk.numeric.dw, chk.absErrDw}, {"dJ/db", chk.analytic.db, chk.numeric.db, chk.absErrDb},
                      {"dJ/dv", chk.analytic.dv, chk.numeric.dv, chk.absErrDv}, {"dJ/dc", chk.analytic.dc, chk.numeric.dc, chk.absErrDc}};
    for (const auto& r : rows) addRow(grad_, {r.n, fmt(r.a, 12), fmt(r.f, 12), QString::number(r.e, 'e', 2), r.e <= 1e-5 ? "pass" : "FAIL"}, r.e <= 1e-5 ? pal.success : pal.error);
    const auto h = nn::train(p0, pr, 101);
    step_->setRowCount(0);
    const char* names[] = {"w", "b", "v", "c"};
    const double before[] = {h.params[0].w, h.params[0].b, h.params[0].v, h.params[0].c};
    const double after[] = {h.params[1].w, h.params[1].b, h.params[1].v, h.params[1].c};
    for (int i = 0; i < 4; ++i) addRow(step_, {names[i], fmt(before[i], 12), fmt(after[i], 12)});
    addRow(step_, {"Loss J", fmt(h.loss[0], 12), fmt(h.loss[1], 12)});
    PlotSeries s;
    s.name = "Loss";
    s.color = pal.primary;
    s.markers = false;
    for (std::size_t i = 0; i < h.loss.size(); ++i) {
        s.x.push_back(static_cast<double>(i));
        s.y.push_back(std::max(h.loss[i], 1e-30));
    }
    loss_->setSeries({s});
    verdict_->setText(QString("<span style='color:%1'>%2: max gradient error %3 (tolerance 1e-5).</span>").arg(chk.pass ? pal.success.name() : pal.error.name(), chk.pass ? "Gradient check PASS" : "Gradient check FAIL", QString::number(chk.maxAbsError, 'e', 2)));
    const auto t = nn::trace(p0, pr);
    text_->setHtml(QString(
        "<p><b>Forward:</b> z = wx + b = %1, h = tanh(z) = %2, \xC5\xB7 = vh + c = %3, J = %4.</p>"
        "<p><b>Backward (chain rule):</b> \xE2\x88\x82J/\xE2\x88\x82\xC5\xB7 = \xC5\xB7 \xE2\x88\x92 y = %5; \xE2\x88\x82J/\xE2\x88\x82v = (\xC5\xB7 \xE2\x88\x92 y)h; \xE2\x88\x82J/\xE2\x88\x82c = \xC5\xB7 \xE2\x88\x92 y; "
        "\xE2\x88\x82J/\xE2\x88\x82z = (\xC5\xB7 \xE2\x88\x92 y) v (1 \xE2\x88\x92 h\xC2\xB2) = %6; \xE2\x88\x82J/\xE2\x88\x82w = \xE2\x88\x82J/\xE2\x88\x82z \xC2\xB7 x; \xE2\x88\x82J/\xE2\x88\x82b = \xE2\x88\x82J/\xE2\x88\x82z.</p>"
        "<p><b>Direction:</b> the prediction (%2) is below the target (2), so \xE2\x88\x82J/\xE2\x88\x82\xC5\xB7 is negative. Gradient descent moves each parameter opposite its gradient, which increases \xC5\xB7 and lowers J. "
        "All four updates use the <i>original</i> parameter values.</p>"
        "<p><b>Convergence:</b> after the 101 updates shown (1 hand-checked update + 100 further steps) the loss is %7. tanh saturates as w and b grow, so the gradients through h shrink; "
        "the output bias c and weight v keep a non-vanishing gradient, which is why the loss still falls smoothly.</p>")
        .arg(fmt(t.fwd.z, 8), fmt(t.fwd.h, 10), fmt(t.fwd.yHat, 10), fmt(t.fwd.loss, 10), fmt(t.error, 10), fmt(t.dJ_dz, 10), fmt(h.loss.back(), 6)));
    ctx_->log("Backprop gradient check + 101 training updates");
    emit ctx_->taskFinished("Backpropagation computed");
}

// ============================================================ Verification
struct VerificationPage::Impl {
    QFutureWatcher<std::vector<aether::verification::CheckResult>> watcher;
};

VerificationPage::VerificationPage(AppContext* ctx, QWidget* parent) : QWidget(parent), ctx_(ctx), d_(std::make_shared<Impl>()) {
    QVBoxLayout* root;
    pageRoot(this, root, "Verification", "Runs the numerical self-checks now. The full GoogleTest suite is a separate executable (see docs/testing.md); its results are not shown here.");
    auto* bar = new QHBoxLayout;
    runBtn_ = new QPushButton("Run self-checks");
    runBtn_->setObjectName("Primary");
    bar->addWidget(runBtn_);
    summary_ = label("Not run in this session.", "Secondary");
    bar->addWidget(summary_, 1);
    root->addLayout(bar);
    table_ = makeTable({"Check", "Measured value and criterion", "Result"});
    table_->horizontalHeader()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    table_->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    table_->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
    root->addWidget(table_, 1);
    connect(runBtn_, &QPushButton::clicked, this, &VerificationPage::runChecks);
    connect(&d_->watcher, &QFutureWatcher<std::vector<aether::verification::CheckResult>>::finished, this, [this] {
        const auto res = d_->watcher.result();
        const Palette& p = ThemeManager::instance().palette();
        table_->setRowCount(0);
        int pass = 0;
        for (const auto& c : res) {
            addRow(table_, {QString::fromStdString(c.name), QString::fromStdString(c.detail), c.passed ? "PASS" : "FAIL"}, c.passed ? p.success : p.error);
            pass += c.passed;
        }
        const QString msg = QString("%1 of %2 self-checks passed (run %3)").arg(pass).arg(res.size()).arg(QString::fromStdString(aether::application::nowString()));
        summary_->setText(QString("<span style='color:%1'>%2</span>").arg(pass == static_cast<int>(res.size()) ? p.success.name() : p.error.name(), msg));
        ctx_->state.verificationSummary = msg.toStdString();
        ctx_->log(msg);
        runBtn_->setEnabled(true);
        if (pass == static_cast<int>(res.size())) emit ctx_->taskFinished(msg); else emit ctx_->taskFailed(msg);
    });
}

void VerificationPage::runChecks() {
    runBtn_->setEnabled(false);
    emit ctx_->taskStarted("Running self-checks", false);
    emit ctx_->taskProgress(-1);
    d_->watcher.setFuture(QtConcurrent::run([] { return aether::verification::runAllChecks(); }));
}

// ============================================================ Reports
ReportsPage::ReportsPage(AppContext* ctx, QWidget* parent) : QWidget(parent), ctx_(ctx) {
    QVBoxLayout* root;
    pageRoot(this, root, "Reports and Documentation", "Project documentation shipped with the application.");
    auto* bar = new QHBoxLayout;
    auto* folder = new QPushButton("Open docs folder");
    auto* results = new QPushButton("Open results folder");
    bar->addWidget(folder);
    bar->addWidget(results);
    bar->addStretch();
    root->addLayout(bar);
    auto* split = new QHBoxLayout;
    list_ = new QListWidget;
    list_->setObjectName("DocList");
    list_->setMaximumWidth(280);
    view_ = new QTextBrowser;
    view_->setOpenExternalLinks(true);
    split->addWidget(list_);
    split->addWidget(view_, 1);
    root->addLayout(split, 1);
    const QStringList docs = {"README.md", "architecture.md", "thermal_method.md", "embedded_design.md", "testing.md", "limitations.md", "report.md", "ai_use_log.md", "ownership_statement.md", "handwritten_derivation_worksheet.md"};
    for (const auto& d : docs) list_->addItem(d);
    connect(list_, &QListWidget::currentTextChanged, this, [this](const QString& f) { if (!f.isEmpty()) showDoc(f); });
    connect(folder, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl::fromLocalFile(docsDir())); });
    connect(results, &QPushButton::clicked, this, [] { QDesktopServices::openUrl(QUrl::fromLocalFile(resultsDir())); });
    list_->setCurrentRow(0);
}

void ReportsPage::showDoc(const QString& fileName) {
    QString path = docsDir() + "/" + fileName;
    if (fileName == "README.md") path = QFileInfo(docsDir()).dir().filePath("README.md");
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        view_->setPlainText("Could not open " + fileName + " (looked in " + path + ").");
        return;
    }
    view_->setMarkdown(QString::fromUtf8(f.readAll()));
}
