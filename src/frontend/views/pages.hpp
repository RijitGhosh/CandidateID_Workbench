// Aether frontend: page widgets. They call the C++ core and display what it returns; no algorithms live here.
#pragma once

#include <QWidget>
#include <memory>
#include <vector>

#include "../../application/controllers/app_context.hpp"
#include "../../core/hardware/mock_devices.hpp"
#include "../../core/performance/benchmark.hpp"

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTableWidget;
class QComboBox;
class QDoubleSpinBox;
class QSpinBox;
class QTabWidget;
class QTextBrowser;
class QListWidget;
class PlotWidget;
class Viewport3D;
namespace ui { class StatTile; }

enum PageIndex { PageOverview, PageGeometry, PageThermal, PagePerformance, PageHardware, PageNeural, PageVerification, PageReports };

class OverviewPage : public QWidget {
    Q_OBJECT
public:
    explicit OverviewPage(AppContext* ctx, QWidget* parent = nullptr);
    void setLaunchToReadyMs(double ms);
public slots:
    void refresh();
private:
    AppContext* ctx_;
    QLabel *project_, *version_, *workspace_, *lastAnalysis_, *verification_, *launchReady_;
    QTableWidget* modules_;
    QListWidget* activity_;
};

class GeometryPage : public QWidget {
    Q_OBJECT
public:
    explicit GeometryPage(AppContext* ctx, QWidget* parent = nullptr);
    bool loadFile(const QString& path);
    Viewport3D* viewport() { return view_; }
private slots:
    void openDialog();
    void loadExample();
private:
    AppContext* ctx_;
    Viewport3D* view_;
    QLabel *file_, *messages_;
    QComboBox* units_;
    ui::StatTile *triangles_, *facets_, *skipped_, *bbox_, *size_;
    void showMessages(const QStringList& errors, const QStringList& warnings);
};

class ThermalPage : public QWidget {
    Q_OBJECT
public:
    explicit ThermalPage(AppContext* ctx, QWidget* parent = nullptr);
    bool loadCsv(const QString& path);
    void solveDefault();
    Viewport3D* viewport() { return cloud_; }
    QTabWidget* tabs() { return tabs_; }
private slots:
    void solveOne();
    void runStudy();
    void exportFe();
    void openCsvDialog();
    void loadExampleCsv();
    void exportCsvResults();
    void onPicked(int idx);
private:
    AppContext* ctx_;
    QTabWidget* tabs_;
    // FE tab
    QDoubleSpinBox *len_, *area_, *k_, *q_, *tl_, *tr_;
    QSpinBox* elems_;
    PlotWidget *profile_, *conv_;
    ui::StatTile *maxMid_, *maxNode_, *nElem_, *midTemp_;
    QTableWidget* table_;
    QLabel* feStatus_;
    // CSV tab
    Viewport3D* cloud_;
    QLabel *csvFile_, *csvMessages_;
    ui::StatTile *nodes_, *tmin_, *tmax_, *tmean_, *selId_, *selPos_, *selT_;
    struct Impl;
    std::shared_ptr<Impl> d_;
};

class PerformancePage : public QWidget {
    Q_OBJECT
public:
    explicit PerformancePage(AppContext* ctx, QWidget* parent = nullptr);
    void setLaunchToReadyMs(double ms);
    void runBenchmarkNow();
private slots:
    void run();
    void saveReport();
private:
    void showReport(const aether::performance::BenchmarkReport& r);
    AppContext* ctx_;
    QSpinBox *cases_, *mesh_;
    QPushButton *runBtn_, *saveBtn_;
    QTableWidget* table_;
    QPlainTextEdit* info_;
    PlotWidget* plot_;
    QLabel *status_, *launch_;
    struct Impl;
    std::shared_ptr<Impl> d_;
};

class HardwarePage : public QWidget {
    Q_OBJECT
public:
    explicit HardwarePage(AppContext* ctx, QWidget* parent = nullptr);
private:
    struct Card;
    void refreshCard(int i);
    void logLine(const QString& s);
    AppContext* ctx_;
    std::vector<std::unique_ptr<aether::hardware::MockDevice>> devices_;
    std::vector<Card*> cards_;
    QPlainTextEdit* log_;
};

class NeuralPage : public QWidget {
    Q_OBJECT
public:
    explicit NeuralPage(AppContext* ctx, QWidget* parent = nullptr);
    void compute();
private:
    AppContext* ctx_;
    QTableWidget *grad_, *step_;
    PlotWidget* loss_;
    QLabel* verdict_;
    QTextBrowser* text_;
};

class VerificationPage : public QWidget {
    Q_OBJECT
public:
    explicit VerificationPage(AppContext* ctx, QWidget* parent = nullptr);
    void runChecks();
private:
    AppContext* ctx_;
    QTableWidget* table_;
    QLabel* summary_;
    QPushButton* runBtn_;
    struct Impl;
    std::shared_ptr<Impl> d_;
};

class ReportsPage : public QWidget {
    Q_OBJECT
public:
    explicit ReportsPage(AppContext* ctx, QWidget* parent = nullptr);
    void showDoc(const QString& fileName);
private:
    AppContext* ctx_;
    QListWidget* list_;
    QTextBrowser* view_;
};
