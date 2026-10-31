// Aether frontend: 3-D viewport (software-rendered with QPainter: no GPU/driver dependency, works on any Windows 11 machine).
// Draws a triangle mesh (flat shaded) or a coloured point cloud with orbit / pan / zoom / fit / reset / picking.
// This file only DRAWS data it is given; all geometry parsing and statistics live in the C++ core.
#pragma once

#include <QColor>
#include <QMatrix4x4>
#include <QVector3D>
#include <QVector>
#include <QWidget>

class Viewport3D : public QWidget {
    Q_OBJECT
public:
    enum class Mode { Empty, Mesh, Points };
    explicit Viewport3D(QWidget* parent = nullptr);

    void setMesh(const QVector<QVector3D>& triVertices /* 3 per triangle */);
    void setPoints(const QVector<QVector3D>& pts, const QVector<float>& scalar, float vmin, float vmax, const QString& unit);
    void clearScene();
    void setSelected(int index) { selected_ = index; update(); }
    void setPlaceholder(const QString& t) { placeholder_ = t; update(); }
    void setAxisUnit(const QString& u) { axisUnit_ = u; update(); }
    int selectedIndex() const { return selected_; }
    Mode mode() const { return mode_; }
    bool saveImage(const QString& path) const;

public slots:
    void fitToView();
    void resetCamera();

signals:
    void pointPicked(int index);  // -1 if nothing was hit

protected:
    void paintEvent(QPaintEvent*) override;
    void mousePressEvent(QMouseEvent*) override;
    void mouseMoveEvent(QMouseEvent*) override;
    void mouseReleaseEvent(QMouseEvent*) override;
    void wheelEvent(QWheelEvent*) override;
    void mouseDoubleClickEvent(QMouseEvent*) override { fitToView(); }

private:
    struct Cam {
        float yaw = 35.f, pitch = 25.f, dist = 3.f;
        QVector3D target{0, 0, 0};
    };
    QMatrix4x4 viewMatrix() const;
    bool project(const QVector3D& w, QPointF& out, float& depth) const;
    void computeBounds();
    QColor colormap(float t) const;
    void drawAxes(QPainter& p);
    void drawLegend(QPainter& p);

    Mode mode_ = Mode::Empty;
    QVector<QVector3D> tris_, pts_;
    QVector<float> scalar_;
    float vmin_ = 0, vmax_ = 1;
    QString unit_, placeholder_ = "No data loaded", axisUnit_;
    QVector3D bmin_, bmax_;
    float radius_ = 1.f;
    Cam cam_;
    int selected_ = -1;
    QPoint last_, press_;
    bool dragging_ = false;
    Qt::MouseButton button_ = Qt::NoButton;
};
