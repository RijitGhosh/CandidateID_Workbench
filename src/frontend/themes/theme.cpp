#include "theme.hpp"

#include <QApplication>
#include <QCoreApplication>
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QPalette>
#include <QSettings>
#include <QStandardPaths>

Palette darkPalette() {
    Palette p;
    p.dark = true;
    p.bg = "#101217"; p.sidebar = "#171A22"; p.panel = "#1D222C"; p.elevated = "#252B37";
    p.text = "#F3F4F8"; p.textSecondary = "#A6ADBD"; p.border = "#343B49";
    p.primary = "#8B5CF6"; p.secondary = "#A78BFA";
    p.success = "#34D399"; p.warning = "#FBBF24"; p.error = "#F87171";
    return p;
}
Palette lightPalette() {
    Palette p;
    p.dark = false;
    p.bg = "#F5F6FA"; p.sidebar = "#FFFFFF"; p.panel = "#FFFFFF"; p.elevated = "#F0F1F7";
    p.text = "#202330"; p.textSecondary = "#656B7B"; p.border = "#DCE0E9";
    p.primary = "#7048E8"; p.secondary = "#8B5CF6";
    p.success = "#047857"; p.warning = "#B45309"; p.error = "#B91C1C";
    return p;
}

ThemeManager& ThemeManager::instance() {
    static ThemeManager t;
    return t;
}

QString ThemeManager::settingsPath() {
    // Relative to the application (portable); fall back to the per-user location if that folder is read-only.
    const QString portable = QCoreApplication::applicationDirPath() + "/aether_settings.ini";
    QFileInfo dir(QCoreApplication::applicationDirPath());
    if (dir.isWritable()) return portable;
    const QString fallback = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation);
    QDir().mkpath(fallback);
    return fallback + "/aether_settings.ini";
}

void ThemeManager::init(QApplication& app) {
    app_ = &app;
    systemDark_ = app.palette().color(QPalette::Window).lightness() < 128;
    QSettings s(settingsPath(), QSettings::IniFormat);
    const QString m = s.value("theme/mode", "dark").toString();
    mode_ = m == "light" ? ThemeMode::Light : m == "system" ? ThemeMode::System : ThemeMode::Dark;
    apply();
}

void ThemeManager::setMode(ThemeMode m) {
    mode_ = m;
    QSettings s(settingsPath(), QSettings::IniFormat);
    s.setValue("theme/mode", m == ThemeMode::Light ? "light" : m == ThemeMode::System ? "system" : "dark");
    s.sync();
    apply();
}

QFont ThemeManager::uiFont(double pt) const {
    QFont f;
    f.setFamilies({"Segoe UI", "Inter", "Noto Sans", "DejaVu Sans", "Sans Serif"});
    f.setPointSizeF(pt);
    return f;
}
QFont ThemeManager::monoFont(double pt) const {
    QFont f;
    f.setFamilies({"Consolas", "JetBrains Mono", "DejaVu Sans Mono", "Monospace"});
    f.setStyleHint(QFont::Monospace);
    f.setPointSizeF(pt);
    return f;
}

void ThemeManager::apply() {
    const bool dark = mode_ == ThemeMode::Dark || (mode_ == ThemeMode::System && systemDark_);
    palette_ = dark ? darkPalette() : lightPalette();
    if (app_) {
        app_->setFont(uiFont(10.0));
        QPalette qp;
        qp.setColor(QPalette::Window, palette_.bg);
        qp.setColor(QPalette::WindowText, palette_.text);
        qp.setColor(QPalette::Base, palette_.panel);
        qp.setColor(QPalette::AlternateBase, palette_.elevated);
        qp.setColor(QPalette::Text, palette_.text);
        qp.setColor(QPalette::Button, palette_.elevated);
        qp.setColor(QPalette::ButtonText, palette_.text);
        qp.setColor(QPalette::Highlight, palette_.primary);
        qp.setColor(QPalette::HighlightedText, QColor("#FFFFFF"));
        qp.setColor(QPalette::ToolTipBase, palette_.elevated);
        qp.setColor(QPalette::ToolTipText, palette_.text);
        qp.setColor(QPalette::PlaceholderText, palette_.textSecondary);
        app_->setPalette(qp);
        app_->setStyleSheet(buildStyleSheet());
    }
    emit changed();
}

QString ThemeManager::buildStyleSheet() const {
    const Palette& p = palette_;
    auto c = [](const QColor& col) { return col.name(); };
    const QString mono = "'Consolas','JetBrains Mono','DejaVu Sans Mono',monospace";
    QString q;
    q += QString(R"(
QWidget { color: %1; background: transparent; }
QMainWindow, QDialog, QStackedWidget, #Page { background: %2; }
QToolTip { background: %3; color: %1; border: 1px solid %4; padding: 4px; }
#TopBar { background: %5; border-bottom: 1px solid %4; }
#Sidebar { background: %5; border-right: 1px solid %4; }
#StatusBar { background: %5; border-top: 1px solid %4; }
#Panel { background: %6; border: 1px solid %4; border-radius: 4px; }
#PanelAlt { background: %3; border: 1px solid %4; border-radius: 4px; }
#Banner { background: %3; border: 1px solid %7; border-radius: 4px; }
#PageTitle { font-size: 20pt; font-weight: 600; }
#PageDesc, #Secondary { color: %8; }
#SectionTitle { font-size: 10pt; font-weight: 600; color: %8; letter-spacing: 1px; }
#Mono, #Value { font-family: %9; }
#Value { font-size: 12pt; }
#Caption { color: %8; font-size: 8.5pt; }
#ProductName { font-size: 11pt; font-weight: 600; }
#NavButton { text-align: left; padding: 9px 14px 9px 16px; border: none; border-left: 3px solid transparent; border-radius: 0; color: %8; background: transparent; }
#NavButton:hover { background: %3; color: %1; }
#NavButton:checked { background: %3; color: %1; border-left: 3px solid %7; font-weight: 600; }
QPushButton { background: %3; border: 1px solid %4; border-radius: 4px; padding: 6px 14px; color: %1; }
QPushButton:hover { border-color: %7; }
QPushButton:pressed { background: %4; }
QPushButton:disabled { color: %8; background: %6; border-color: %4; }
QPushButton#Primary { background: %7; border: 1px solid %7; color: #FFFFFF; font-weight: 600; }
QPushButton#Primary:hover { background: %10; border-color: %10; }
QPushButton#Primary:disabled { background: %4; border-color: %4; color: %8; }
QToolButton { background: transparent; border: 1px solid transparent; border-radius: 4px; padding: 4px; }
QToolButton:hover { background: %3; border-color: %4; }
QToolButton#ThemeToggle { border: 1px solid %4; background: %3; }
QToolButton#ViewTool { border: 1px solid %4; background: %3; padding: 4px 10px; }
QToolButton#ViewTool:hover { border-color: %7; }
QToolButton#ViewTool:checked { background: %7; color: #FFFFFF; border-color: %7; }
QMenu { background: %3; border: 1px solid %4; padding: 4px; }
QMenu::item { padding: 6px 22px; border-radius: 3px; }
QMenu::item:selected { background: %7; color: #FFFFFF; }
QLineEdit, QSpinBox, QDoubleSpinBox, QComboBox { background: %6; border: 1px solid %4; border-radius: 4px; padding: 4px 8px; selection-background-color: %7; font-family: %9; }
QLineEdit:focus, QSpinBox:focus, QDoubleSpinBox:focus, QComboBox:focus { border-color: %7; }
QComboBox QAbstractItemView { background: %3; border: 1px solid %4; selection-background-color: %7; selection-color: #FFFFFF; }
QTabWidget::pane { border: 1px solid %4; border-radius: 4px; top: -1px; background: %2; }
QTabBar::tab { padding: 8px 16px; border: none; color: %8; border-bottom: 2px solid transparent; }
QTabBar::tab:selected { color: %1; border-bottom: 2px solid %7; }
QTabBar::tab:hover { color: %1; }
QTableWidget, QListWidget, QTreeWidget { background: %6; border: 1px solid %4; border-radius: 4px; gridline-color: %4; alternate-background-color: %3; font-family: %9; selection-background-color: %7; selection-color: #FFFFFF; }
QListWidget#DocList { font-family: inherit; }
QHeaderView::section { background: %3; color: %8; border: none; border-right: 1px solid %4; border-bottom: 1px solid %4; padding: 5px 8px; font-weight: 600; }
QTextBrowser, QPlainTextEdit, QTextEdit { background: %6; border: 1px solid %4; border-radius: 4px; selection-background-color: %7; }
QPlainTextEdit#Log { font-family: %9; }
QScrollBar:vertical { background: transparent; width: 10px; margin: 0; }
QScrollBar::handle:vertical { background: %4; border-radius: 5px; min-height: 28px; }
QScrollBar::handle:vertical:hover { background: %8; }
QScrollBar:horizontal { background: transparent; height: 10px; margin: 0; }
QScrollBar::handle:horizontal { background: %4; border-radius: 5px; min-width: 28px; }
QScrollBar::add-line, QScrollBar::sub-line { width: 0; height: 0; }
QProgressBar { background: %3; border: 1px solid %4; border-radius: 3px; text-align: center; height: 14px; font-size: 8pt; }
QProgressBar::chunk { background: %7; border-radius: 2px; }
QCheckBox::indicator { width: 14px; height: 14px; border: 1px solid %4; border-radius: 3px; background: %6; }
QCheckBox::indicator:checked { background: %7; border-color: %7; }
QSplitter::handle { background: %4; }
QScrollArea { border: none; }
)")
             .arg(c(p.text), c(p.bg), c(p.elevated), c(p.border), c(p.sidebar))   // 1..5
             .arg(c(p.panel), c(p.primary), c(p.textSecondary), mono, c(p.secondary));  // 6..10
    return q;
}
