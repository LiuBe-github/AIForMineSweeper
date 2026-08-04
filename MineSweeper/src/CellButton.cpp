#include "CellButton.h"

#include <QMouseEvent>
#include <QPainter>

namespace {

const QColor kFaceColor(192, 192, 192);
const QColor kRevealedFace(200, 200, 200);

QColor numberColor(int n) {
    switch (n) {
        case 1: return QColor(0, 0, 255);      // 蓝
        case 2: return QColor(0, 128, 0);      // 绿
        case 3: return QColor(255, 0, 0);      // 红
        case 4: return QColor(0, 0, 128);      // 深蓝
        case 5: return QColor(128, 0, 0);      // 深红
        case 6: return QColor(0, 128, 128);    // 青
        case 7: return QColor(0, 0, 0);        // 黑
        default: return QColor(128, 128, 128); // 灰
    }
}

void drawRaisedBevel(QPainter& p, const QRect& r) {
    const int s = 2;  // 边框厚度
    // 外圈：左上亮、右下暗
    p.fillRect(QRect(r.left(), r.top(), r.width(), s), QColor(255, 255, 255));
    p.fillRect(QRect(r.left(), r.top(), s, r.height()), QColor(255, 255, 255));
    p.fillRect(QRect(r.left(), r.bottom() - s + 1, r.width(), s), QColor(0, 0, 0));
    p.fillRect(QRect(r.right() - s + 1, r.top(), s, r.height()), QColor(0, 0, 0));
    // 内圈：过渡色
    p.fillRect(QRect(r.left() + s, r.top() + s, r.width() - 2 * s, s), QColor(223, 223, 223));
    p.fillRect(QRect(r.left() + s, r.top() + s, s, r.height() - 2 * s), QColor(223, 223, 223));
    p.fillRect(QRect(r.left() + s, r.bottom() - 2 * s + 1, r.width() - 2 * s, s), QColor(128, 128, 128));
    p.fillRect(QRect(r.right() - 2 * s + 1, r.top() + s, s, r.height() - 2 * s), QColor(128, 128, 128));
    // 中央
    p.fillRect(QRect(r.left() + 2 * s, r.top() + 2 * s, r.width() - 4 * s, r.height() - 4 * s), kFaceColor);
}

void drawFlag(QPainter& p, const QRect& r) {
    const qreal cx = r.center().x();
    const qreal cy = r.center().y();
    const qreal h = r.height() * 0.62;
    const QPointF poleBase(cx - r.width() * 0.22, cy + h * 0.42);
    const QPointF poleTop(cx - r.width() * 0.22, cy - h * 0.35);

    // 旗杆
    p.setPen(QPen(QColor(60, 60, 60), 1.8));
    p.drawLine(QLineF(poleBase, poleTop));

    // 旗面
    QPolygonF flag;
    flag << poleTop
         << QPointF(cx + r.width() * 0.26, cy - h * 0.15)
         << QPointF(cx - r.width() * 0.22, cy + h * 0.05);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(208, 0, 0));
    p.drawPolygon(flag);

    // 底座
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(60, 60, 60));
    p.drawRect(QRectF(cx - r.width() * 0.30, cy + h * 0.42, r.width() * 0.16, 3));
}

void drawMine(QPainter& p, const QRect& r) {
    const QPointF c = r.center();
    const qreal R = r.width() * 0.24;
    const qreal kPi = 3.14159265358979323846;

    // 尖刺
    p.setPen(QPen(QColor(20, 20, 20), 1.2));
    for (int i = 0; i < 8; ++i) {
        const qreal ang = i * kPi / 4.0;
        const QPointF dir(std::cos(ang), std::sin(ang));
        p.drawLine(QLineF(c + dir * (R * 1.10), c + dir * (R * 1.65)));
    }
    // 球体
    p.setPen(Qt::NoPen);
    p.setBrush(QColor(25, 25, 25));
    p.drawEllipse(c, R, R);
    // 高光
    p.setBrush(QColor(160, 160, 160));
    p.drawEllipse(c + QPointF(-R * 0.35, -R * 0.35), R * 0.22, R * 0.22);
}

void drawWrongFlag(QPainter& p, const QRect& r) {
    p.setPen(QPen(QColor(200, 0, 0), 2.4));
    const qreal m = r.width() * 0.22;
    p.drawLine(QLineF(QPointF(r.left() + m, r.top() + m),
                      QPointF(r.right() - m, r.bottom() - m)));
    p.drawLine(QLineF(QPointF(r.right() - m, r.top() + m),
                      QPointF(r.left() + m, r.bottom() - m)));
}

void drawNumber(QPainter& p, const QRect& r, int n) {
    QFont f = p.font();
    f.setBold(true);
    f.setPixelSize(qMax(12, r.height() - 8));
    p.setFont(f);
    p.setPen(numberColor(n));
    p.drawText(r, Qt::AlignCenter, QString::number(n));
}

}  // namespace

CellButton::CellButton(int row, int col, QWidget* parent)
    : QPushButton(parent), row_(row), col_(col) {
    setFocusPolicy(Qt::NoFocus);
    setContextMenuPolicy(Qt::PreventContextMenu);
}

void CellButton::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        if (field_ && field_->cell(row_, col_).state == CellState::Revealed)
            emit chordRequested(row_, col_);
        else
            emit leftClicked(row_, col_);
    } else if (event->button() == Qt::RightButton) {
        emit rightClicked(row_, col_);
    } else if (event->button() == Qt::MiddleButton) {
        emit chordRequested(row_, col_);
    }
    // 不调用基类，避免按钮自身的按下样式覆盖自绘效果
}

void CellButton::paintEvent(QPaintEvent*) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRect r = rect();
    const Cell& cell = field_ ? field_->cell(row_, col_) : Cell();
    const bool hidden = (cell.state == CellState::Hidden ||
                         cell.state == CellState::Flagged);

    if (hidden) {
        drawRaisedBevel(p, r);
        if (cell.state == CellState::Flagged)
            drawFlag(p, r);
        return;
    }

    // 已翻开
    p.fillRect(r, kRevealedFace);
    p.setPen(QColor(128, 128, 128));
    p.drawRect(r.adjusted(0, 0, -1, -1));

    if (cell.detonated) {
        p.fillRect(r.adjusted(1, 1, -1, -1), QColor(255, 0, 0));
    }
    if (cell.wrongFlag) {
        drawWrongFlag(p, r);
    } else if (cell.mine) {
        drawMine(p, r);
    } else if (cell.adjacent > 0) {
        drawNumber(p, r, cell.adjacent);
    }
}
