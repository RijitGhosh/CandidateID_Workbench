#include "plot_widget.hpp"

#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <cmath>

#include "../themes/theme.hpp"

PlotWidget::PlotWidget(QWidget* parent) : QWidget(parent) {
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    connect(&ThemeManager::instance(), &ThemeManager::changed, this, qOverload<>(&QWidget::update));
}

bool PlotWidget::saveImage(const QString& path) const {
    QPixmap pm(size());
    const_cast<PlotWidget*>(this)->render(&pm);
    return pm.save(path);
}

namespace {
double niceStep(double range, int target) {
    if (range <= 0) return 1;
    const double raw = range / target;
    const double mag = std::pow(10.0, std::floor(std::log10(raw)));
    const double n = raw / mag;
    const double s = n < 1.5 ? 1 : n < 3.5 ? 2 : n < 7.5 ? 5 : 10;
    return s * mag;
}
QString tickLabel(double v) {
    if (v != 0 && (std::fabs(v) >= 1e5 || std::fabs(v) < 1e-3)) return QString::number(v, 'e', 1);
    return QString::number(v, 'g', 6);
}
}  // namespace

void PlotWidget::paintEvent(QPaintEvent*) {
    const Palette& pal = ThemeManager::instance().palette();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), pal.panel);
    const QFont ui = ThemeManager::instance().uiFont(9);
    const QFont mono = ThemeManager::instance().monoFont(8.5);
    p.setFont(ui);

    int top = 14;
    if (!title_.isEmpty()) {
        QFont f = ui;
        f.setBold(true);
        p.setFont(f);
        p.setPen(pal.text);
        p.drawText(QRect(0, 6, width(), 22), Qt::AlignCenter, title_);
        p.setFont(ui);
        top = 34;
    }
    const int left = 80, right = 18, bottom = 48;
    QRectF plot(left, top, width() - left - right, height() - top - bottom);
    if (plot.width() < 40 || plot.height() < 40) return;

    // data range
    double xmin = 1e300, xmax = -1e300, ymin = 1e300, ymax = -1e300;
    bool any = false;
    for (const auto& s : series_)
        for (int i = 0; i < s.x.size() && i < s.y.size(); ++i) {
            if (!std::isfinite(s.x[i]) || !std::isfinite(s.y[i])) continue;
            if ((logX_ && s.x[i] <= 0) || (logY_ && s.y[i] <= 0)) continue;
            xmin = std::min(xmin, s.x[i]); xmax = std::max(xmax, s.x[i]);
            ymin = std::min(ymin, s.y[i]); ymax = std::max(ymax, s.y[i]);
            any = true;
        }
    p.setPen(QPen(pal.border, 1));
    p.setBrush(pal.bg);
    p.drawRect(plot);
    if (!any) {
        p.setPen(pal.textSecondary);
        p.drawText(plot, Qt::AlignCenter, placeholder_);
        return;
    }
    auto tx = [&](double v) { return logX_ ? std::log10(v) : v; };
    auto ty = [&](double v) { return logY_ ? std::log10(v) : v; };
    double x0 = tx(xmin), x1 = tx(xmax), y0 = ty(ymin), y1 = ty(ymax);
    if (x1 - x0 < 1e-300) { x0 -= 0.5; x1 += 0.5; }
    if (y1 - y0 < 1e-300) { y0 -= 0.5; y1 += 0.5; }
    const double padY = (y1 - y0) * 0.06;
    y0 -= padY; y1 += padY;
    if (!logX_) { const double padX = (x1 - x0) * 0.02; x0 -= padX; x1 += padX; }
    else { x0 -= 0.1; x1 += 0.1; }

    auto mapX = [&](double v) { return plot.left() + (tx(v) - x0) / (x1 - x0) * plot.width(); };
    auto mapY = [&](double v) { return plot.bottom() - (ty(v) - y0) / (y1 - y0) * plot.height(); };

    // grid + ticks
    p.setFont(mono);
    auto gridLine = [&](bool vertical, double pos) {
        p.setPen(QPen(pal.border, 1, Qt::DotLine));
        if (vertical) p.drawLine(QPointF(pos, plot.top()), QPointF(pos, plot.bottom()));
        else p.drawLine(QPointF(plot.left(), pos), QPointF(plot.right(), pos));
    };
    auto ticks = [&](double a, double b, bool isLog, QVector<double>& vals) {
        if (isLog) {
            for (int e = static_cast<int>(std::floor(a)); e <= static_cast<int>(std::ceil(b)); ++e)
                if (e >= a && e <= b) vals.push_back(e);
        } else {
            const double st = niceStep(b - a, 6);
            for (double v = std::ceil(a / st) * st; v <= b + st * 1e-9; v += st) vals.push_back(std::fabs(v) < st * 1e-9 ? 0.0 : v);
        }
    };
    QVector<double> xt, yt;
    ticks(x0, x1, logX_, xt);
    ticks(y0, y1, logY_, yt);
    for (double v : xt) {
        const double px = plot.left() + (v - x0) / (x1 - x0) * plot.width();
        gridLine(true, px);
        p.setPen(pal.textSecondary);
        p.drawText(QRectF(px - 36, plot.bottom() + 4, 72, 16), Qt::AlignCenter, tickLabel(logX_ ? std::pow(10.0, v) : v));
    }
    for (double v : yt) {
        const double py = plot.bottom() - (v - y0) / (y1 - y0) * plot.height();
        gridLine(false, py);
        p.setPen(pal.textSecondary);
        p.drawText(QRectF(2, py - 8, left - 8, 16), Qt::AlignRight | Qt::AlignVCenter, tickLabel(logY_ ? std::pow(10.0, v) : v));
    }
    p.setFont(ui);
    p.setPen(pal.text);
    p.drawText(QRectF(plot.left(), height() - 22, plot.width(), 18), Qt::AlignCenter, xTitle_);
    p.save();
    p.translate(13, plot.center().y());
    p.rotate(-90);
    p.drawText(QRectF(-plot.height() / 2, -8, plot.height(), 16), Qt::AlignCenter, yTitle_);
    p.restore();

    // series
    p.setClipRect(plot.adjusted(-1, -1, 1, 1));
    for (const auto& s : series_) {
        QPen pen(s.color, 2.0);
        if (s.dashed) pen.setStyle(Qt::DashLine);
        QPainterPath path;
        bool started = false;
        for (int i = 0; i < s.x.size() && i < s.y.size(); ++i) {
            if (!std::isfinite(s.x[i]) || !std::isfinite(s.y[i]) || (logX_ && s.x[i] <= 0) || (logY_ && s.y[i] <= 0)) continue;
            const QPointF pt(mapX(s.x[i]), mapY(s.y[i]));
            if (!started) { path.moveTo(pt); started = true; } else path.lineTo(pt);
        }
        if (s.line) { p.setPen(pen); p.setBrush(Qt::NoBrush); p.drawPath(path); }
        if (s.markers) {
            p.setPen(QPen(pal.panel, 1));
            p.setBrush(s.color);
            for (int i = 0; i < s.x.size() && i < s.y.size(); ++i) {
                if (!std::isfinite(s.x[i]) || !std::isfinite(s.y[i]) || (logX_ && s.x[i] <= 0) || (logY_ && s.y[i] <= 0)) continue;
                p.drawEllipse(QPointF(mapX(s.x[i]), mapY(s.y[i])), 3.6, 3.6);
            }
        }
    }
    p.setClipping(false);

    // legend
    if (series_.size() > 1 || (series_.size() == 1 && !series_[0].name.isEmpty())) {
        p.setFont(ui);
        int lw = 0;
        for (const auto& s : series_) lw = std::max(lw, p.fontMetrics().horizontalAdvance(s.name));
        const QRectF lg(plot.right() - lw - 46, plot.top() + 8, lw + 38, series_.size() * 18 + 8);
        QColor bg = pal.panel;
        bg.setAlpha(225);
        p.setPen(QPen(pal.border, 1));
        p.setBrush(bg);
        p.drawRoundedRect(lg, 3, 3);
        int i = 0;
        for (const auto& s : series_) {
            const double y = lg.top() + 13 + i * 18;
            QPen pen(s.color, 2.0);
            if (s.dashed) pen.setStyle(Qt::DashLine);
            p.setPen(pen);
            p.drawLine(QPointF(lg.left() + 7, y), QPointF(lg.left() + 25, y));
            if (s.markers) { p.setBrush(s.color); p.drawEllipse(QPointF(lg.left() + 16, y), 3.2, 3.2); }
            p.setPen(pal.text);
            p.drawText(QPointF(lg.left() + 31, y + 4), s.name);
            ++i;
        }
    }
}
