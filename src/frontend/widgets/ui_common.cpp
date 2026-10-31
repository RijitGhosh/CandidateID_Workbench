#include "ui_common.hpp"

#include <QCoreApplication>
#include <QHBoxLayout>
#include <QIcon>
#include <QPainter>
#include <QPainterPath>
#include <QPixmap>

namespace ui {

QString appDir() { return QCoreApplication::applicationDirPath(); }

QString findDir(const QString& name) {
    QDir d(appDir());
    for (int i = 0; i < 5; ++i) {
        if (d.exists(name)) return d.filePath(name);
        if (!d.cdUp()) break;
    }
    return appDir() + "/" + name;  // may not exist; callers handle that
}

QLabel* label(const QString& text, const QString& objectName, QWidget* parent) {
    auto* l = new QLabel(text, parent);
    if (!objectName.isEmpty()) l->setObjectName(objectName);
    return l;
}

QFrame* panel(const QString& objectName) {
    auto* f = new QFrame;
    f->setObjectName(objectName);
    return f;
}

QWidget* pageHeader(const QString& title, const QString& description) {
    auto* w = new QWidget;
    auto* l = new QVBoxLayout(w);
    l->setContentsMargins(0, 0, 0, 4);
    l->setSpacing(2);
    l->addWidget(label(title, "PageTitle"));
    auto* d = label(description, "PageDesc");
    d->setWordWrap(true);
    l->addWidget(d);
    return w;
}

StatTile::StatTile(const QString& caption, QWidget* parent) : QFrame(parent) {
    setObjectName("PanelAlt");
    auto* l = new QVBoxLayout(this);
    l->setContentsMargins(10, 7, 10, 7);
    l->setSpacing(1);
    caption_ = label(caption, "Caption");
    value_ = label(QString::fromUtf8("\xE2\x80\x94"), "Value");
    value_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    l->addWidget(caption_);
    l->addWidget(value_);
}
void StatTile::setValue(const QString& text, const QColor& color) {
    value_->setText(text);
    value_->setStyleSheet(color.isValid() ? QString("color:%1;").arg(color.name()) : QString());
}
void StatTile::clearValue() { setValue(QString::fromUtf8("\xE2\x80\x94")); }

QString fmt(double v, int digits) { return QString::number(v, 'g', digits); }
QString htmlEscape(const QString& s) { return s.toHtmlEscaped(); }

static QPixmap canvas() {
    QPixmap pm(32, 32);
    pm.fill(Qt::transparent);
    return pm;
}
QIcon sunIcon(const QColor& c) {
    QPixmap pm = canvas();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(c, 2.2, Qt::SolidLine, Qt::RoundCap));
    p.setBrush(Qt::NoBrush);
    p.drawEllipse(QPointF(16, 16), 6, 6);
    for (int i = 0; i < 8; ++i) {
        p.save();
        p.translate(16, 16);
        p.rotate(i * 45);
        p.drawLine(QPointF(0, -10), QPointF(0, -13));
        p.restore();
    }
    return QIcon(pm);
}
QIcon moonIcon(const QColor& c) {
    QPixmap pm = canvas();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(c);
    QPainterPath moon;
    moon.addEllipse(QPointF(15, 16), 10, 10);
    QPainterPath cut;
    cut.addEllipse(QPointF(21, 12), 9, 9);
    p.drawPath(moon.subtracted(cut));
    return QIcon(pm);
}
QIcon systemIcon(const QColor& c) {
    QPixmap pm = canvas();
    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(QPen(c, 2.0));
    p.setBrush(Qt::NoBrush);
    p.drawRoundedRect(QRectF(5, 7, 22, 15), 2, 2);
    p.drawLine(QPointF(12, 26), QPointF(20, 26));
    return QIcon(pm);
}

}  // namespace ui
