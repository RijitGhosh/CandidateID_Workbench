// Aether frontend: shared small widgets and helpers. Presentation only.
#pragma once

#include <QColor>
#include <QDir>
#include <QFrame>
#include <QLabel>
#include <QString>
#include <QVBoxLayout>

#include "../themes/theme.hpp"

namespace ui {

// ---- resource lookup relative to the executable (no absolute paths) ----
QString appDir();
QString findDir(const QString& name);   // searches appDir and up to four parent levels
inline QString dataDir() { return findDir("data"); }
inline QString docsDir() { return findDir("docs"); }
inline QString resultsDir() { return findDir("results"); }

QLabel* label(const QString& text, const QString& objectName = {}, QWidget* parent = nullptr);
QFrame* panel(const QString& objectName = "Panel");

// Page header: title + one-line description
QWidget* pageHeader(const QString& title, const QString& description);

// Small "caption over value" tile; value uses the monospaced font.
class StatTile : public QFrame {
public:
    explicit StatTile(const QString& caption, QWidget* parent = nullptr);
    void setValue(const QString& text, const QColor& color = QColor());
    void clearValue();
private:
    QLabel* caption_;
    QLabel* value_;
};

QString fmt(double v, int digits = 6);   // 'g' formatting
QString htmlEscape(const QString& s);

QIcon sunIcon(const QColor& c);
QIcon moonIcon(const QColor& c);
QIcon systemIcon(const QColor& c);

}  // namespace ui
