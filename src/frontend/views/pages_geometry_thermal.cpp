#include <QApplication>
#include <QComboBox>
#include <QFileInfo>
#include <QVector3D>
#include <cmath>
#include <QDesktopServices>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFutureWatcher>
#include <QGridLayout>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QListWidget>
#include <QMessageBox>
#include <QPushButton>
#include <QSpinBox>
#include <QSplitter>
#include <QTabWidget>
#include <QTableWidget>
#include <QToolButton>
#include <QUrl>
#include <QVBoxLayout>
#include <QtConcurrent>
#include <fstream>

#include "../../core/csv/temperature_csv.hpp"
#include "../../core/geometry/stl.hpp"
#include "../../core/thermal/thermal.hpp"
#include "../widgets/plot_widget.hpp"
#include "../widgets/ui_common.hpp"
#include "../widgets/viewport3d.hpp"
#include "pages.hpp"

using namespace ui;
namespace geo = aether::geometry;
namespace th = aether::thermal;
namespace csvns = aether::csv;

namespace {
QToolButton* toolBtn(const QString& text, bool checkable = false) {
    auto* b = new QToolButton;
    b->setObjectName("ViewTool");
    b->setText(text);
    b->setCheckable(checkable);
    return b;
}
QDoubleSpinBox* dspin(double v, double lo, double hi, int dec, const QString& suffix) {
    auto* s = new QDoubleSpinBox;
    s->setDecimals(dec);
    s->setRange(lo, hi);
    s->setValue(v);
    s->setSuffix(suffix);
    s->setButtonSymbols(QAbstractSpinBox::NoButtons);
    s->setMinimumWidth(130);
    return s;
}
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
void addRow(QTableWidget* t, const QStringList& cells) {
    const int r = t->rowCount();
    t->insertRow(r);
    for (int c = 0; c < cells.size(); ++c) t->setItem(r, c, new QTableWidgetItem(cells[c]));
}
}  // namespace

// ============================================================ Geometry
GeometryPage::GeometryPage(AppContext* ctx, QWidget* parent) : QWidget(parent), ctx_(ctx) {
    setObjectName("Page");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 16);
    root->setSpacing(10);
    root->addWidget(pageHeader("Geometry", "Import and inspect ASCII STL triangle meshes. Binary STL and CAD solids are not supported."));

    auto* bar = new QHBoxLayout;
    auto* open = new QPushButton("Open STL...");
    open->setObjectName("Primary");
    auto* ex = new QPushButton("Load example");
    auto* fit = toolBtn("Fit to view");
    auto* reset = toolBtn("Reset camera");
    auto* shot = toolBtn("Save image");
    bar->addWidget(open);
    bar->addWidget(ex);
    bar->addSpacing(12);
    bar->addWidget(fit);
    bar->addWidget(reset);
    bar->addWidget(shot);
    bar->addStretch();
    bar->addWidget(label("Model units:", "Secondary"));
    units_ = new QComboBox;
    units_->addItems({"unspecified", "mm", "cm", "m", "in"});
    bar->addWidget(units_);
    root->addLayout(bar);

    auto* split = new QSplitter;
    view_ = new Viewport3D;
    view_->setPlaceholder("No STL loaded.\nOpen an ASCII STL file or load the example.");
    split->addWidget(view_);
    auto* side = panel();
    side->setMinimumWidth(300);
    side->setMaximumWidth(360);
    auto* sl = new QVBoxLayout(side);
    sl->setContentsMargins(12, 12, 12, 12);
    sl->setSpacing(8);
    sl->addWidget(label("MODEL", "SectionTitle"));
    file_ = label("No file loaded", "Mono");
    file_->setWordWrap(true);
    sl->addWidget(file_);
    triangles_ = new StatTile("Triangles (accepted)");
    facets_ = new StatTile("Facets in file");
    skipped_ = new StatTile("Skipped: degenerate / duplicate");
    bbox_ = new StatTile("Bounding box min \xE2\x86\x92 max");
    size_ = new StatTile("Extent X \xC3\x97 Y \xC3\x97 Z");
    for (auto* t : {triangles_, facets_, skipped_, bbox_, size_}) sl->addWidget(t);
    auto* note = label("STL files do not encode units. The unit selector above only labels the axes; it does not scale the model.", "Caption");
    note->setWordWrap(true);
    sl->addWidget(note);
    messages_ = label("", "Secondary");
    messages_->setWordWrap(true);
    messages_->setTextFormat(Qt::RichText);
    sl->addWidget(messages_);
    sl->addStretch();
    split->addWidget(side);
    split->setStretchFactor(0, 1);
    split->setCollapsible(0, false);
    root->addWidget(split, 1);
    auto* hint = label("Drag: rotate  |  Shift+drag or right-drag: pan  |  Wheel: zoom  |  Double-click: fit", "Caption");
    root->addWidget(hint);

    connect(open, &QPushButton::clicked, this, &GeometryPage::openDialog);
    connect(ex, &QPushButton::clicked, this, &GeometryPage::loadExample);
    connect(fit, &QToolButton::clicked, view_, &Viewport3D::fitToView);
    connect(reset, &QToolButton::clicked, view_, &Viewport3D::resetCamera);
    connect(shot, &QToolButton::clicked, this, [this] {
        const QString p = QFileDialog::getSaveFileName(this, "Save viewport image", "geometry.png", "PNG (*.png)");
        if (!p.isEmpty() && !view_->saveImage(p)) QMessageBox::warning(this, "Save image", "Could not write " + p);
    });
    connect(units_, &QComboBox::currentTextChanged, this, [this](const QString& u) { view_->setAxisUnit(u == "unspecified" ? QString() : u); });
}

void GeometryPage::showMessages(const QStringList& errors, const QStringList& warnings) {
    const Palette& p = ThemeManager::instance().palette();
    QString h;
    for (const auto& e : errors) h += QString("<p style='color:%1'><b>Error</b> &mdash; %2</p>").arg(p.error.name(), htmlEscape(e));
    for (const auto& w : warnings) h += QString("<p style='color:%1'><b>Note</b> &mdash; %2</p>").arg(p.warning.name(), htmlEscape(w));
    messages_->setText(h);
}

void GeometryPage::openDialog() {
    const QString p = QFileDialog::getOpenFileName(this, "Open ASCII STL", dataDir() + "/geometry", "STL files (*.stl);;All files (*)");
    if (!p.isEmpty()) loadFile(p);
}
void GeometryPage::loadExample() { loadFile(dataDir() + "/geometry/example_bracket.stl"); }

bool GeometryPage::loadFile(const QString& path) {
    emit ctx_->taskStarted("Importing STL: " + QFileInfo(path).fileName(), false);
    emit ctx_->taskProgress(-1);
    QApplication::processEvents();
    const auto r = geo::loadAsciiStlFile(path.toStdString());
    if (!r.ok) {
        QStringList errs;
        for (const auto& e : r.errors) errs << QString::fromStdString(e.format());
        showMessages(errs, {});
        file_->setText(path);
        view_->clearScene();
        for (auto* t : {triangles_, facets_, skipped_, bbox_, size_}) t->clearValue();
        ctx_->log("STL import failed: " + QFileInfo(path).fileName());
        emit ctx_->taskFailed("STL import failed: " + errs.value(0));
        return false;
    }
    QVector<QVector3D> v;
    v.reserve(static_cast<int>(r.mesh.triangles.size() * 3));
    for (const auto& t : r.mesh.triangles)
        for (const auto& p : t.v) v.push_back(QVector3D(float(p.x), float(p.y), float(p.z)));
    view_->setMesh(v);
    const auto& b = r.mesh.bbox;
    file_->setText(QFileInfo(path).fileName() + "\nsolid: " + QString::fromStdString(r.mesh.name));
    triangles_->setValue(QString::number(r.mesh.triangles.size()));
    facets_->setValue(QString::number(r.mesh.facetsInFile));
    skipped_->setValue(QString("%1 / %2").arg(r.mesh.degenerateSkipped).arg(r.mesh.duplicateSkipped));
    bbox_->setValue(QString("(%1, %2, %3)\n\xE2\x86\x92 (%4, %5, %6)").arg(fmt(b.min.x, 5), fmt(b.min.y, 5), fmt(b.min.z, 5), fmt(b.max.x, 5), fmt(b.max.y, 5), fmt(b.max.z, 5)));
    size_->setValue(QString("%1 \xC3\x97 %2 \xC3\x97 %3").arg(fmt(b.max.x - b.min.x, 5), fmt(b.max.y - b.min.y, 5), fmt(b.max.z - b.min.z, 5)));
    QStringList warns;
    for (const auto& w : r.warnings) warns << QString::fromStdString(w);
    showMessages({}, warns);
    ctx_->state.stlPath = path.toStdString();
    ctx_->log(QString("Imported STL %1 (%2 triangles)").arg(QFileInfo(path).fileName()).arg(r.mesh.triangles.size()));
    emit ctx_->taskFinished(QString("STL loaded: %1 triangles").arg(r.mesh.triangles.size()));
    return true;
}

// ============================================================ Thermal
struct ThermalPage::Impl {
    th::Solution solution;
    bool haveSolution = false;
    std::vector<th::ConvergenceRow> convergence;
    csvns::TemperatureData data;
    bool haveCsv = false;
};

ThermalPage::ThermalPage(AppContext* ctx, QWidget* parent) : QWidget(parent), ctx_(ctx), d_(std::make_shared<Impl>()) {
    setObjectName("Page");
    auto* root = new QVBoxLayout(this);
    root->setContentsMargins(24, 20, 24, 16);
    root->setSpacing(10);
    root->addWidget(pageHeader("Thermal Analysis", "1-D steady-state conduction by finite elements, and 3-D temperature-result inspection."));
    tabs_ = new QTabWidget;
    root->addWidget(tabs_, 1);

    // ---------------- FE tab
    auto* fe = new QWidget;
    auto* feLay = new QHBoxLayout(fe);
    feLay->setContentsMargins(10, 10, 10, 10);
    auto* left = panel();
    left->setFixedWidth(300);
    auto* ll = new QVBoxLayout(left);
    ll->setContentsMargins(12, 12, 12, 12);
    ll->setSpacing(6);
    ll->addWidget(label("PROBLEM  -d/dx(kA dT/dx) = qA", "SectionTitle"));
    auto* form = new QGridLayout;
    len_ = dspin(1.0, 1e-6, 1e6, 4, " m");
    area_ = dspin(1e-4, 1e-12, 1e3, 8, " m\xC2\xB2");
    k_ = dspin(10.0, -1e6, 1e6, 4, " W/(m\xC2\xB7K)");
    q_ = dspin(10000.0, -1e12, 1e12, 2, " W/m\xC2\xB3");
    tl_ = dspin(100.0, -273.15, 1e5, 3, " \xC2\xB0""C");
    tr_ = dspin(20.0, -273.15, 1e5, 3, " \xC2\xB0""C");
    elems_ = new QSpinBox;
    elems_->setRange(0, 1000000);
    elems_->setValue(20);
    const QList<QPair<QString, QWidget*>> rows = {{"Length L", len_}, {"Area A", area_}, {"Conductivity k", k_}, {"Heat generation q", q_}, {"T(0)", tl_}, {"T(L)", tr_}, {"Elements N", elems_}};
    int r = 0;
    for (const auto& row : rows) {
        form->addWidget(label(row.first, "Secondary"), r, 0);
        form->addWidget(row.second, r, 1);
        ++r;
    }
    ll->addLayout(form);
    auto* solve = new QPushButton("Solve");
    solve->setObjectName("Primary");
    auto* study = new QPushButton("Mesh study N = 10, 20, 40, 80");
    auto* exp = new QPushButton("Export results...");
    auto* reset = new QPushButton("Reset to prescribed problem");
    ll->addWidget(solve);
    ll->addWidget(study);
    ll->addWidget(exp);
    ll->addWidget(reset);
    auto* assumptions = label("Assumptions: constant properties, uniform section, steady state, insulated lateral surface, uniform volumetric heat generation, 1-D heat flow. Two-node linear elements.", "Caption");
    assumptions->setWordWrap(true);
    ll->addWidget(assumptions);
    feStatus_ = label("", "Secondary");
    feStatus_->setWordWrap(true);
    feStatus_->setTextFormat(Qt::RichText);
    ll->addWidget(feStatus_);
    ll->addStretch();
    feLay->addWidget(left);

    auto* right = new QVBoxLayout;
    auto* tiles = new QHBoxLayout;
    nElem_ = new StatTile("Elements");
    maxMid_ = new StatTile("Max |error| at element midpoints [\xC2\xB0""C]");
    maxNode_ = new StatTile("Max |error| at nodes [\xC2\xB0""C]");
    midTemp_ = new StatTile("T at x = L/2 (FE) [\xC2\xB0""C]");
    for (auto* t : {nElem_, maxMid_, maxNode_, midTemp_}) tiles->addWidget(t);
    right->addLayout(tiles);
    auto* plots = new QHBoxLayout;
    profile_ = new PlotWidget;
    profile_->setTitle("Temperature profile");
    profile_->setAxisTitles("Position x [m]", "Temperature [\xC2\xB0""C]");
    profile_->setPlaceholder("Press Solve");
    conv_ = new PlotWidget;
    conv_->setTitle("Mesh convergence (midpoint error)");
    conv_->setAxisTitles("Element size h [m]", "Max |error| [\xC2\xB0""C]");
    conv_->setLog(true, true);
    conv_->setPlaceholder("Press 'Mesh study'");
    plots->addWidget(profile_, 3);
    plots->addWidget(conv_, 2);
    right->addLayout(plots, 3);
    table_ = makeTable({"Element", "x_mid [m]", "T_FE [\xC2\xB0""C]", "T_exact [\xC2\xB0""C]", "|error| [\xC2\xB0""C]"});
    right->addWidget(table_, 2);
    feLay->addLayout(right, 1);
    tabs_->addTab(fe, "Rod FE analysis");

    // ---------------- CSV tab
    auto* cs = new QWidget;
    auto* csRoot = new QVBoxLayout(cs);
    csRoot->setContentsMargins(10, 10, 10, 10);
    auto* banner = panel("Banner");
    auto* bl = new QVBoxLayout(banner);
    bl->setContentsMargins(10, 6, 10, 6);
    auto* b1 = label("Synthetic example generated for this prototype. Not an actual ANSYS export.", "Secondary");
    auto* b2 = label("Temperature nodes are shown in their own coordinate system. They are not registered to, and not mapped onto, the STL mesh.", "Caption");
    b2->setWordWrap(true);
    bl->addWidget(b1);
    bl->addWidget(b2);
    csRoot->addWidget(banner);
    auto* cbar = new QHBoxLayout;
    auto* openCsv = new QPushButton("Open CSV...");
    openCsv->setObjectName("Primary");
    auto* exCsv = new QPushButton("Load synthetic example");
    auto* fit = toolBtn("Fit to view");
    auto* rst = toolBtn("Reset camera");
    auto* expCsv = new QPushButton("Export results...");
    cbar->addWidget(openCsv);
    cbar->addWidget(exCsv);
    cbar->addSpacing(12);
    cbar->addWidget(fit);
    cbar->addWidget(rst);
    cbar->addStretch();
    cbar->addWidget(expCsv);
    csRoot->addLayout(cbar);
    auto* split = new QSplitter;
    cloud_ = new Viewport3D;
    cloud_->setAxisUnit("m");
    cloud_->setPlaceholder("No temperature file loaded.\nSchema: node_id,x_m,y_m,z_m,temperature_C");
    split->addWidget(cloud_);
    auto* side = panel();
    side->setFixedWidth(300);
    auto* sl = new QVBoxLayout(side);
    sl->setContentsMargins(12, 12, 12, 12);
    sl->setSpacing(6);
    sl->addWidget(label("RESULT FILE", "SectionTitle"));
    csvFile_ = label("No file loaded", "Mono");
    csvFile_->setWordWrap(true);
    sl->addWidget(csvFile_);
    nodes_ = new StatTile("Node count");
    tmin_ = new StatTile("Minimum [\xC2\xB0""C]");
    tmax_ = new StatTile("Maximum [\xC2\xB0""C]");
    tmean_ = new StatTile("Mean [\xC2\xB0""C]");
    for (auto* t : {nodes_, tmin_, tmax_, tmean_}) sl->addWidget(t);
    sl->addWidget(label("SELECTED NODE  (click a point)", "SectionTitle"));
    selId_ = new StatTile("Node ID");
    selPos_ = new StatTile("Coordinates x, y, z [m]");
    selT_ = new StatTile("Temperature [\xC2\xB0""C]");
    for (auto* t : {selId_, selPos_, selT_}) sl->addWidget(t);
    csvMessages_ = label("", "Secondary");
    csvMessages_->setWordWrap(true);
    csvMessages_->setTextFormat(Qt::RichText);
    sl->addWidget(csvMessages_);
    sl->addStretch();
    split->addWidget(side);
    split->setStretchFactor(0, 1);
    csRoot->addWidget(split, 1);
    tabs_->addTab(cs, "Temperature result (CSV)");

    connect(solve, &QPushButton::clicked, this, &ThermalPage::solveOne);
    connect(study, &QPushButton::clicked, this, &ThermalPage::runStudy);
    connect(exp, &QPushButton::clicked, this, &ThermalPage::exportFe);
    connect(reset, &QPushButton::clicked, this, [this] {
        len_->setValue(1.0); area_->setValue(1e-4); k_->setValue(10.0); q_->setValue(10000.0); tl_->setValue(100.0); tr_->setValue(20.0); elems_->setValue(20);
    });
    connect(openCsv, &QPushButton::clicked, this, &ThermalPage::openCsvDialog);
    connect(exCsv, &QPushButton::clicked, this, &ThermalPage::loadExampleCsv);
    connect(fit, &QToolButton::clicked, cloud_, &Viewport3D::fitToView);
    connect(rst, &QToolButton::clicked, cloud_, &Viewport3D::resetCamera);
    connect(expCsv, &QPushButton::clicked, this, &ThermalPage::exportCsvResults);
    connect(cloud_, &Viewport3D::pointPicked, this, &ThermalPage::onPicked);
    connect(&ThemeManager::instance(), &ThemeManager::changed, this, [this] { if (d_->haveSolution) solveOne(); });
}

void ThermalPage::solveDefault() {
    elems_->setValue(20);
    solveOne();
}

void ThermalPage::solveOne() {
    th::RodParameters p;
    p.length_m = len_->value();
    p.area_m2 = area_->value();
    p.conductivity_W_mK = k_->value();
    p.heatGeneration_W_m3 = q_->value();
    p.tLeft_C = tl_->value();
    p.tRight_C = tr_->value();
    p.elements = elems_->value();
    const Palette& pal = ThemeManager::instance().palette();
    try {
        d_->solution = th::solve(p);
        d_->haveSolution = true;
    } catch (const std::exception& e) {
        feStatus_->setText(QString("<span style='color:%1'><b>Cannot solve:</b> %2</span>").arg(pal.error.name(), htmlEscape(e.what())));
        emit ctx_->taskFailed(QString("Thermal solve rejected: ") + e.what());
        return;
    }
    const auto& s = d_->solution;
    feStatus_->setText(QString("<span style='color:%1'>Solved: %2 elements, %3 nodes.</span>").arg(pal.success.name()).arg(p.elements).arg(s.x.size()));
    nElem_->setValue(QString::number(p.elements));
    maxMid_->setValue(fmt(s.maxMidpointError, 8));
    maxNode_->setValue(fmt(s.maxNodalError, 3));
    midTemp_->setValue(fmt(s.tFe[s.tFe.size() / 2], 8));
    QVector<PlotSeries> ser;
    PlotSeries exact;
    exact.name = "Analytical";
    exact.color = pal.secondary;
    exact.dashed = true;
    const int dense = 200;
    for (int i = 0; i <= dense; ++i) {
        const double x = p.length_m * i / dense;
        exact.x.push_back(x);
        exact.y.push_back(th::analyticalTemperature(p, x));
    }
    PlotSeries fe;
    fe.name = QString("FE (N=%1)").arg(p.elements);
    fe.color = pal.dark ? QColor("#F3F4F8") : QColor("#202330");
    fe.markers = p.elements <= 100;
    for (std::size_t i = 0; i < s.x.size(); ++i) { fe.x.push_back(s.x[i]); fe.y.push_back(s.tFe[i]); }
    ser << exact << fe;
    profile_->setSeries(ser);
    table_->setRowCount(0);
    const std::size_t shown = std::min<std::size_t>(s.xMid.size(), 2000);
    for (std::size_t e = 0; e < shown; ++e)
        addRow(table_, {QString::number(e), fmt(s.xMid[e], 8), fmt(s.tFeMid[e], 10), fmt(s.tExactMid[e], 10), fmt(s.midError[e], 6)});
    ctx_->state.lastAnalysis = QString("1-D thermal FE, N=%1, max midpoint error %2 C").arg(p.elements).arg(fmt(s.maxMidpointError, 4)).toStdString();
    ctx_->state.lastAnalysisTime = aether::application::nowString();
    ctx_->log(QString("Thermal solve N=%1 (max midpoint error %2 C)").arg(p.elements).arg(fmt(s.maxMidpointError, 4)));
    emit ctx_->taskFinished("Thermal solve complete");
}

void ThermalPage::runStudy() {
    th::RodParameters p;
    p.length_m = len_->value();
    p.area_m2 = area_->value();
    p.conductivity_W_mK = k_->value();
    p.heatGeneration_W_m3 = q_->value();
    p.tLeft_C = tl_->value();
    p.tRight_C = tr_->value();
    const Palette& pal = ThemeManager::instance().palette();
    try {
        d_->convergence = th::convergenceStudy(p, {10, 20, 40, 80});
    } catch (const std::exception& e) {
        feStatus_->setText(QString("<span style='color:%1'><b>Cannot run study:</b> %2</span>").arg(pal.error.name(), htmlEscape(e.what())));
        return;
    }
    PlotSeries s;
    s.name = "FE midpoint error";
    s.color = pal.primary;
    s.markers = true;
    QString orders;
    for (const auto& r : d_->convergence) {
        s.x.push_back(r.h_m);
        s.y.push_back(r.maxMidpointError_C);
        if (!std::isnan(r.observedOrder)) orders += QString(" %1").arg(r.observedOrder, 0, 'f', 3);
    }
    PlotSeries ref;  // reference slope-2 line anchored on the first point (drawing aid only)
    ref.name = "slope 2 (reference)";
    ref.color = pal.textSecondary;
    ref.dashed = true;
    if (!d_->convergence.empty()) {
        const auto& f = d_->convergence.front();
        for (const auto& r : d_->convergence) { ref.x.push_back(r.h_m); ref.y.push_back(f.maxMidpointError_C * std::pow(r.h_m / f.h_m, 2.0)); }
    }
    conv_->setSeries({s, ref});
    feStatus_->setText(QString("<span style='color:%1'>Mesh study done. Observed order:%2</span>").arg(pal.success.name(), orders));
    ctx_->log("Mesh convergence study N = 10, 20, 40, 80");
    emit ctx_->taskFinished("Mesh study complete");
}

void ThermalPage::exportFe() {
    if (!d_->haveSolution) { QMessageBox::information(this, "Export", "Solve first."); return; }
    const QString dir = QFileDialog::getExistingDirectory(this, "Export thermal results to folder", resultsDir() + "/raw");
    if (dir.isEmpty()) return;
    const QString tag = QString("N%1").arg(d_->solution.params.elements);
    std::ofstream a((dir + "/thermal_nodes_" + tag + ".csv").toStdString()), b((dir + "/thermal_midpoints_" + tag + ".csv").toStdString()), c((dir + "/thermal_summary_" + tag + ".txt").toStdString());
    if (!a || !b || !c) { QMessageBox::warning(this, "Export", "Could not write files into " + dir); return; }
    th::writeNodesCsv(a, d_->solution);
    th::writeMidpointsCsv(b, d_->solution);
    th::writeSummaryText(c, d_->solution);
    if (!d_->convergence.empty()) { std::ofstream f((dir + "/thermal_convergence.csv").toStdString()); th::writeConvergenceCsv(f, d_->convergence); }
    ctx_->log("Exported thermal results to " + dir);
    emit ctx_->taskFinished("Exported thermal results to " + dir);
}

void ThermalPage::openCsvDialog() {
    const QString p = QFileDialog::getOpenFileName(this, "Open temperature CSV", dataDir() + "/temperature", "CSV files (*.csv);;All files (*)");
    if (!p.isEmpty()) loadCsv(p);
}
void ThermalPage::loadExampleCsv() { loadCsv(dataDir() + "/temperature/example_temperature.csv"); }

bool ThermalPage::loadCsv(const QString& path) {
    emit ctx_->taskStarted("Importing CSV: " + QFileInfo(path).fileName(), false);
    emit ctx_->taskProgress(-1);
    QApplication::processEvents();
    const Palette& pal = ThemeManager::instance().palette();
    const auto r = csvns::loadTemperatureCsvFile(path.toStdString());
    csvFile_->setText(QFileInfo(path).fileName());
    if (!r.ok) {
        QString h;
        for (const auto& e : r.errors) h += QString("<p style='color:%1'><b>Error</b> &mdash; %2</p>").arg(pal.error.name(), htmlEscape(QString::fromStdString(e.format())));
        if (r.errorsTruncated) h += QString("<p style='color:%1'>More errors not shown (first %2 listed).</p>").arg(pal.warning.name()).arg(csvns::kMaxReportedErrors);
        csvMessages_->setText(h);
        cloud_->clearScene();
        d_->haveCsv = false;
        for (auto* t : {nodes_, tmin_, tmax_, tmean_, selId_, selPos_, selT_}) t->clearValue();
        ctx_->log("CSV import failed: " + QFileInfo(path).fileName());
        emit ctx_->taskFailed("CSV import failed: " + (r.errors.empty() ? QString() : QString::fromStdString(r.errors[0].format())));
        return false;
    }
    d_->data = r.data;
    d_->haveCsv = true;
    QVector<QVector3D> pts;
    QVector<float> sc;
    for (const auto& n : r.data.nodes) {
        pts.push_back(QVector3D(float(n.x), float(n.y), float(n.z)));
        sc.push_back(float(n.temperature));
    }
    cloud_->setPoints(pts, sc, float(r.data.stats.min), float(r.data.stats.max), QString::fromUtf8("\xC2\xB0""C"));
    nodes_->setValue(QString::number(r.data.stats.count));
    tmin_->setValue(fmt(r.data.stats.min, 8));
    tmax_->setValue(fmt(r.data.stats.max, 8));
    tmean_->setValue(fmt(r.data.stats.mean, 8));
    for (auto* t : {selId_, selPos_, selT_}) t->clearValue();
    QString notes;
    for (const auto& c : r.data.comments) notes += QString("<p style='color:%1'>%2</p>").arg(pal.textSecondary.name(), htmlEscape(QString::fromStdString(c)));
    csvMessages_->setText(notes);
    ctx_->state.csvPath = path.toStdString();
    ctx_->log(QString("Imported temperature CSV %1 (%2 nodes)").arg(QFileInfo(path).fileName()).arg(r.data.stats.count));
    emit ctx_->taskFinished(QString("CSV loaded: %1 nodes").arg(r.data.stats.count));
    return true;
}

void ThermalPage::onPicked(int idx) {
    if (idx < 0 || !d_->haveCsv || idx >= static_cast<int>(d_->data.nodes.size())) {
        for (auto* t : {selId_, selPos_, selT_}) t->clearValue();
        return;
    }
    const auto& n = d_->data.nodes[idx];
    selId_->setValue(QString::number(n.id));
    selPos_->setValue(QString("%1, %2, %3").arg(fmt(n.x, 7), fmt(n.y, 7), fmt(n.z, 7)));
    selT_->setValue(fmt(n.temperature, 8));
}

void ThermalPage::exportCsvResults() {
    if (!d_->haveCsv) { QMessageBox::information(this, "Export", "Load a temperature CSV first."); return; }
    const QString dir = QFileDialog::getExistingDirectory(this, "Export result summary to folder", resultsDir() + "/raw");
    if (dir.isEmpty()) return;
    std::ofstream a((dir + "/temperature_nodes_export.csv").toStdString()), b((dir + "/temperature_stats.txt").toStdString());
    if (!a || !b) { QMessageBox::warning(this, "Export", "Could not write files into " + dir); return; }
    csvns::writeTemperatureCsv(a, d_->data);
    csvns::writeStatsSummary(b, d_->data);
    ctx_->log("Exported temperature results to " + dir);
    emit ctx_->taskFinished("Exported temperature results to " + dir);
}
