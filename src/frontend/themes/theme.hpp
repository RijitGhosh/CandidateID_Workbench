// Aether frontend: dark/light/system theme manager. Presentation only.
#pragma once

#include <QColor>
#include <QFont>
#include <QObject>
#include <QString>

class QApplication;

struct Palette {
    QColor bg, sidebar, panel, elevated, text, textSecondary, border;
    QColor primary, secondary, success, warning, error;
    bool dark = true;
};

enum class ThemeMode { Dark, Light, System };

class ThemeManager : public QObject {
    Q_OBJECT
public:
    static ThemeManager& instance();
    void init(QApplication& app);  // loads the saved mode and applies it
    ThemeMode mode() const { return mode_; }
    bool isDark() const { return palette_.dark; }
    const Palette& palette() const { return palette_; }
    void setMode(ThemeMode m);     // applies immediately, persists to the settings file
    QFont uiFont(double pt = 10.0) const;
    QFont monoFont(double pt = 10.0) const;
    static QString settingsPath();

signals:
    void changed();

private:
    ThemeManager() = default;
    void apply();
    QString buildStyleSheet() const;
    QApplication* app_ = nullptr;
    ThemeMode mode_ = ThemeMode::Dark;
    Palette palette_;
    bool systemDark_ = true;
};

Palette darkPalette();
Palette lightPalette();
