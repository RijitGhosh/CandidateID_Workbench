#include "viewport3d.hpp"

#include <QMouseEvent>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>
#include <QWheelEvent>
#include <algorithm>
#include <cmath>
#include <numeric>

#include "../themes/theme.hpp"

Viewport3D::Viewport3D(QWidget* parent) : QWidget(parent) {
    setMinimumSize(360, 300);
    setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);
    setMouseTracking(false);
    setFocusPolicy(Qt::StrongFocus);
    connect(&ThemeManager::instance(), &ThemeManager::changed, this, qOverload<>(&QWidget::update));
}

bool Viewport3D::saveImage(const QString& path) const {
    QPixmap pm(size());
    const_cast<Viewport3D*>(this)->render(&pm);
    return pm.save(path);
}

void Viewport3D::clearScene() {
    mode_ = Mode::Empty;
    tris_.clear(); pts_.clear(); scalar_.clear();
    selected_ = -1;
    update();
}

void Viewport3D::computeBounds() {
    const QVector<QVector3D>& src = mode_ == Mode::Mesh ? tris_ : pts_;
    if (src.isEmpty()) { bmin_ = bmax_ = {}; radius_ = 1; return; }
    bmin_ = bmax_ = src[0];
    for (const auto& v : src) {
        bmin_ = {std::min(bmin_.x(), v.x()), std::min(bmin_.y(), v.y()), std::min(bmin_.z(), v.z())};
        bmax_ = {std::max(bmax_.x(), v.x()), std::max(bmax_.y(), v.y()), std::max(bmax_.z(), v.z())};
    }
    radius_ = std::max(1e-12f, 0.5f * (bmax_ - bmin_).length());
}

void Viewport3D::setMesh(const QVector<QVector3D>& v) {
    tris_ = v; pts_.clear(); scalar_.clear(); selected_ = -1;
    mode_ = v.isEmpty() ? Mode::Empty : Mode::Mesh;
    computeBounds();
    resetCamera();
}
void Viewport3D::setPoints(const QVector<QVector3D>& pts, const QVector<float>& s, float vmin, float vmax, const QString& unit) {
    pts_ = pts; scalar_ = s; vmin_ = vmin; vmax_ = vmax; unit_ = unit; tris_.clear(); selected_ = -1;
    mode_ = pts.isEmpty() ? Mode::Empty : Mode::Points;
    computeBounds();
    resetCamera();
}

void Viewport3D::fitToView() {
    cam_.target = 0.5f * (bmin_ + bmax_);
    cam_.dist = radius_ * 3.2f;
    update();
}
void Viewport3D::resetCamera() {
    cam_ = Cam{};
    cam_.target = 0.5f * (bmin_ + bmax_);
    cam_.dist = radius_ * 3.2f;
    update();
}

QMatrix4x4 Viewport3D::viewMatrix() const {
    QMatrix4x4 m;
    m.translate(0, 0, -cam_.dist);
    m.rotate(cam_.pitch - 90.f, 1, 0, 0);  // Z up
    m.rotate(cam_.yaw, 0, 0, 1);
    m.translate(-cam_.target);
    return m;
}

bool Viewport3D::project(const QVector3D& w, QPointF& out, float& depth) const {
    const QMatrix4x4 v = viewMatrix();
    const QVector3D e = v.map(w);
    depth = -e.z();
    const float fovScale = 1.0f / std::tan(qDegreesToRadians(25.f));
    const float near = 0.05f * cam_.dist;
    if (depth < near) return false;
    const float s = std::min(width(), height()) * 0.5f * fovScale;
    out = QPointF(width() * 0.5 + e.x() / depth * s, height() * 0.5 - e.y() / depth * s);
    return true;
}

QColor Viewport3D::colormap(float t) const {
    // Viridis control points (perceptually uniform, colour-blind friendly)
    static const float stops[][3] = {{0.267f, 0.005f, 0.329f}, {0.283f, 0.141f, 0.458f}, {0.254f, 0.265f, 0.530f},
                                     {0.207f, 0.372f, 0.553f}, {0.164f, 0.471f, 0.558f}, {0.128f, 0.567f, 0.551f},
                                     {0.135f, 0.659f, 0.518f}, {0.267f, 0.749f, 0.441f}, {0.478f, 0.821f, 0.318f},
                                     {0.741f, 0.873f, 0.150f}, {0.993f, 0.906f, 0.144f}};
    t = std::clamp(t, 0.f, 1.f);
    const float f = t * 10.f;
    const int i = std::min(9, static_cast<int>(f));
    const float a = f - i;
    return QColor::fromRgbF(stops[i][0] * (1 - a) + stops[i + 1][0] * a, stops[i][1] * (1 - a) + stops[i + 1][1] * a,
                            stops[i][2] * (1 - a) + stops[i + 1][2] * a);
}

void Viewport3D::drawAxes(QPainter& p) {
    const Palette& pal = ThemeManager::instance().palette();
    // orientation gizmo (bottom-left) rotates with the camera
    QMatrix4x4 rot = viewMatrix();
    rot.setColumn(3, QVector4D(0, 0, 0, 1));
    const QPointF c(52, height() - 52);
    struct Ax { QVector3D d; QColor col; const char* n; };
    const Ax axes[3] = {{{1, 0, 0}, QColor("#F87171"), "X"}, {{0, 1, 0}, QColor("#34D399"), "Y"}, {{0, 0, 1}, QColor("#60A5FA"), "Z"}};
    QVector<std::pair<float, int>> order;
    for (int i = 0; i < 3; ++i) order.push_back({rot.map(axes[i].d).z(), i});
    std::sort(order.begin(), order.end());
    QFont f = ThemeManager::instance().monoFont(8.5);
    f.setBold(true);
    p.setFont(f);
    for (auto [z, i] : order) {
        const QVector3D e = rot.map(axes[i].d);
        const QPointF tip(c.x() + e.x() * 34, c.y() - e.y() * 34);
        p.setPen(QPen(axes[i].col, 2.4, Qt::SolidLine, Qt::RoundCap));
        p.drawLine(c, tip);
        p.drawText(QRectF(tip.x() - 8 + e.x() * 8, tip.y() - 8 - e.y() * 8, 16, 16), Qt::AlignCenter, axes[i].n);
    }
    p.setBrush(pal.textSecondary);
    p.setPen(Qt::NoPen);
    p.drawEllipse(c, 2.5, 2.5);

    // model-space axes at the bounding-box corner
    if (mode_ == Mode::Empty) return;
    const QVector3D o = bmin_;
    const QVector3D ext = bmax_ - bmin_;
    for (int i = 0; i < 3; ++i) {
        QVector3D end = o;
        end[i] += ext[i];
        QPointF a, b;
        float da, db;
        if (project(o, a, da) && project(end, b, db)) {
            p.setPen(QPen(axes[i].col, 1.6));
            p.drawLine(a, b);
            p.setPen(axes[i].col);
            QString t = QString("%1 %2").arg(axes[i].n).arg(QString::number(end[i], 'g', 4));
            if (!axisUnit_.isEmpty()) t += " " + axisUnit_;
            p.setFont(ThemeManager::instance().monoFont(8));
            p.drawText(b + QPointF(5, -4), t);
        }
    }
}

void Viewport3D::drawLegend(QPainter& p) {
    if (mode_ != Mode::Points) return;
    const Palette& pal = ThemeManager::instance().palette();
    const int w = 16, h = std::min(220, height() - 120);
    const QRectF bar(width() - 80, 40, w, h);
    for (int i = 0; i < h; ++i) {
        p.setPen(colormap(1.f - static_cast<float>(i) / (h - 1)));
        p.drawLine(QPointF(bar.left(), bar.top() + i), QPointF(bar.right(), bar.top() + i));
    }
    p.setPen(QPen(pal.border, 1));
    p.setBrush(Qt::NoBrush);
    p.drawRect(bar);
    p.setFont(ThemeManager::instance().monoFont(8.5));
    p.setPen(pal.text);
    for (int i = 0; i <= 4; ++i) {
        const double v = vmax_ - (vmax_ - vmin_) * i / 4.0;
        const double y = bar.top() + h * i / 4.0;
        p.drawLine(QPointF(bar.right(), y), QPointF(bar.right() + 4, y));
        p.drawText(QPointF(bar.right() + 7, y + 4), QString::number(v, 'f', 1));
    }
    p.setFont(ThemeManager::instance().uiFont(8.5));
    p.setPen(pal.textSecondary);
    p.drawText(QRectF(bar.left() - 30, bar.top() - 22, 120, 16), Qt::AlignLeft, "Temperature [" + unit_ + "]");
}

void Viewport3D::paintEvent(QPaintEvent*) {
    const Palette& pal = ThemeManager::instance().palette();
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing);
    p.fillRect(rect(), pal.dark ? QColor("#0B0D12") : QColor("#EEF0F6"));
    if (mode_ == Mode::Empty) {
        p.setPen(pal.textSecondary);
        p.setFont(ThemeManager::instance().uiFont(11));
        p.drawText(rect(), Qt::AlignCenter, placeholder_);
        drawAxes(p);
        return;
    }
    // light direction in camera space (headlight, slightly offset)
    const QMatrix4x4 view = viewMatrix();
    if (mode_ == Mode::Mesh) {
        struct Item { float depth; QPolygonF poly; QColor col; };
        QVector<Item> items;
        items.reserve(tris_.size() / 3);
        const QVector3D light = QVector3D(0.3f, 0.5f, 0.8f).normalized();
        for (int i = 0; i + 2 < tris_.size(); i += 3) {
            QVector3D n = QVector3D::crossProduct(tris_[i + 1] - tris_[i], tris_[i + 2] - tris_[i]);
            if (n.lengthSquared() == 0) continue;
            n = view.mapVector(n.normalized());
            QPointF a, b, c;
            float da, db, dc;
            if (!project(tris_[i], a, da) || !project(tris_[i + 1], b, db) || !project(tris_[i + 2], c, dc)) continue;
            const float lit = std::fabs(QVector3D::dotProduct(n, light));  // two-sided shading
            const float k = 0.25f + 0.75f * lit;
            QColor base = pal.dark ? QColor("#A78BFA") : QColor("#7048E8");
            QColor col = QColor::fromRgbF(std::min(1.f, base.redF() * k), std::min(1.f, base.greenF() * k), std::min(1.f, base.blueF() * k));
            items.push_back({(da + db + dc) / 3.f, QPolygonF({a, b, c}), col});
        }
        std::sort(items.begin(), items.end(), [](const Item& l, const Item& r) { return l.depth > r.depth; });  // far to near
        const QColor edge = pal.dark ? QColor(11, 13, 18, 200) : QColor(32, 35, 48, 150);
        p.setPen(QPen(edge, 0.9));
        for (const auto& it : items) {
            p.setBrush(it.col);
            p.drawPolygon(it.poly);
        }
    } else {
        struct Item { float depth; QPointF pt; int idx; };
        QVector<Item> items;
        items.reserve(pts_.size());
        for (int i = 0; i < pts_.size(); ++i) {
            QPointF pt;
            float d;
            if (project(pts_[i], pt, d)) items.push_back({d, pt, i});
        }
        std::sort(items.begin(), items.end(), [](const Item& l, const Item& r) { return l.depth > r.depth; });
        const float span = std::max(1e-12f, vmax_ - vmin_);
        const double r = std::clamp(520.0 / std::sqrt(static_cast<double>(std::max(1, static_cast<int>(pts_.size())))), 2.2, 7.0);
        p.setPen(Qt::NoPen);
        for (const auto& it : items) {
            p.setBrush(colormap((scalar_[it.idx] - vmin_) / span));
            p.drawEllipse(it.pt, r, r);
        }
        if (selected_ >= 0 && selected_ < pts_.size()) {
            QPointF pt;
            float d;
            if (project(pts_[selected_], pt, d)) {
                p.setBrush(Qt::NoBrush);
                p.setPen(QPen(pal.dark ? Qt::white : Qt::black, 2.4));
                p.drawEllipse(pt, r + 5, r + 5);
                p.setPen(QPen(pal.primary, 1.6));
                p.drawEllipse(pt, r + 8, r + 8);
            }
        }
    }
    drawAxes(p);
    drawLegend(p);
}

void Viewport3D::mousePressEvent(QMouseEvent* e) {
    last_ = press_ = e->pos();
    button_ = e->button();
    dragging_ = true;
    setFocus();
}
void Viewport3D::mouseMoveEvent(QMouseEvent* e) {
    if (!dragging_) return;
    const QPoint d = e->pos() - last_;
    last_ = e->pos();
    if (button_ == Qt::LeftButton && !(e->modifiers() & Qt::ShiftModifier)) {
        cam_.yaw += d.x() * 0.5f;
        cam_.pitch = std::clamp(cam_.pitch + d.y() * 0.5f, -89.f, 89.f);
    } else {  // right/middle drag or shift+left = pan
        const float s = cam_.dist * 0.0025f * (500.f / std::max(200, std::min(width(), height())));
        QMatrix4x4 inv = viewMatrix().inverted();
        inv.setColumn(3, QVector4D(0, 0, 0, 1));
        cam_.target += inv.map(QVector3D(-d.x() * s, d.y() * s, 0));
    }
    update();
}
void Viewport3D::mouseReleaseEvent(QMouseEvent* e) {
    dragging_ = false;
    if (e->button() == Qt::LeftButton && (e->pos() - press_).manhattanLength() < 4 && mode_ == Mode::Points) {
        int best = -1;
        double bestD = 14.0 * 14.0, bestDepth = 1e30;
        for (int i = 0; i < pts_.size(); ++i) {
            QPointF pt;
            float d;
            if (!project(pts_[i], pt, d)) continue;
            const double dx = pt.x() - e->pos().x(), dy = pt.y() - e->pos().y();
            const double dist2 = dx * dx + dy * dy;
            if (dist2 < bestD || (dist2 < 36.0 && best >= 0 && d < bestDepth && dist2 <= bestD + 4.0)) {
                if (dist2 < bestD) bestD = dist2;
                bestDepth = d;
                best = i;
            }
        }
        selected_ = best;
        update();
        emit pointPicked(best);
    }
}
void Viewport3D::wheelEvent(QWheelEvent* e) {
    const float steps = e->angleDelta().y() / 120.f;
    cam_.dist = std::clamp(cam_.dist * std::pow(0.88f, steps), radius_ * 0.2f, radius_ * 60.f);
    update();
}
