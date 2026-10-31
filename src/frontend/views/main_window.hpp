#pragma once

#include <QButtonGroup>
#include <QLabel>
#include <QMainWindow>
#include <QProgressBar>
#include <QPushButton>
#include <QStackedWidget>
#include <QToolButton>
#include <vector>

#include "../../application/controllers/app_context.hpp"
#include "pages.hpp"

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(double launchStartMsSinceEpoch, QWidget* parent = nullptr);
    void showPage(int index);
    // Used by --screenshots: drives each page into a realistic state and writes PNGs.
    bool captureScreenshots(const QString& outDir);
    void markReady(double elapsedMs);

private slots:
    void openProject();
    void saveProject();
    void setStatus(const QString& text, const QString& kind);
    void onThemeChanged();

private:
    void buildTopBar(QWidget* parent, QLayout* layout);
    void updateThemeIcon();
    AppContext* ctx_;
    QStackedWidget* stack_;
    std::vector<QPushButton*> navButtons_;
    QLabel *projectLabel_, *appStatus_, *taskLabel_;
    QProgressBar* progress_;
    QPushButton* cancel_;
    QToolButton* themeBtn_;
    OverviewPage* overview_;
    GeometryPage* geometry_;
    ThermalPage* thermal_;
    PerformancePage* performance_;
    HardwarePage* hardware_;
    NeuralPage* neural_;
    VerificationPage* verification_;
    ReportsPage* reports_;
};
