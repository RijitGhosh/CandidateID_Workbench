// Aether frontend: lightweight theme-aware 2-D plot (QPainter). No numerical algorithms here, only drawing.
#pragma once

#include <QString>
#include <QVector>
#include <QWidget>

struct PlotSeries {
    QString name;
    QVector<double> x, y;
    QColor color;
    bool line = true;
    bool markers = false;
    bool dashed = false;
};

class PlotWidget : public QWidget {
    Q_OBJECT
public:
    explicit PlotWidget(QWidget* parent = nullptr);
    void setTitle(const QString& t) { title_ = t; update(); }
    void setAxisTitles(const QString& x, const QString& y) { xTitle_ = x; yTitle_ = y; update(); }
    void setLog(bool logX, bool logY) { logX_ = logX; logY_ = logY; update(); }
    void setSeries(const QVector<PlotSeries>& s) { series_ = s; update(); }
    void clear() { series_.clear(); update(); }
    void setPlaceholder(const QString& t) { placeholder_ = t; update(); }
    QSize minimumSizeHint() const override { return {320, 220}; }
    bool saveImage(const QString& path) const;

protected:
    void paintEvent(QPaintEvent*) override;

private:
    QString title_, xTitle_, yTitle_, placeholder_ = "No data";
    bool logX_ = false, logY_ = false;
    QVector<PlotSeries> series_;
};
