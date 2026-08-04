#include "BoardWidget.h"

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

BoardWidget::BoardWidget(MineField* field, QWidget* parent)
    : QWidget(parent), field_(field) {
    setFocusPolicy(Qt::NoFocus);
    setContextMenuPolicy(Qt::PreventContextMenu);

    // 格子尺寸自适应：大棋盘自动缩小，保证窗口大致不超出屏幕
    int px = 28;
    if (field_->cols() > 0 && field_->cols() * px > 1500)
        px = qMax(1, 1500 / field_->cols());
    if (field_->rows() > 0 && field_->rows() * px > 850)
        px = qMax(1, 850 / field_->rows());
    cellPx_ = px;
    setFixedSize(cellPx_ * field_->cols(), cellPx_ * field_->rows());
}

QRect BoardWidget::cellRect(int row, int col) const {
    return QRect(col * cellPx_, row * cellPx_, cellPx_, cellPx_);
}

void BoardWidget::refresh(const std::vector<int>& dirtyCells) {
    if (dirtyCells.empty())
        return;
    // 变化的格子太多时整板刷新一次，避免海量 update() 调用
    if (dirtyCells.size() > 256 || !field_) {
        update();
        return;
    }
    for (int idx : dirtyCells) {
        const int r = idx / field_->cols();
        const int c = idx % field_->cols();
        update(cellRect(r, c));
    }
}

void BoardWidget::paintEvent(QPaintEvent* event) {
    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, cellPx_ >= 12);

    // 只绘制暴露/变化的区域，超大棋盘下也保持流畅
    const QRect exposed = event->rect();
    const int r0 = qMax(0, exposed.top() / cellPx_);
    const int r1 = qMin(field_->rows() - 1, exposed.bottom() / cellPx_);
    const int c0 = qMax(0, exposed.left() / cellPx_);
    const int c1 = qMin(field_->cols() - 1, exposed.right() / cellPx_);
    for (int r = r0; r <= r1; ++r)
        for (int c = c0; c <= c1; ++c)
            paintCell(p, r, c);
}

void BoardWidget::paintCell(QPainter& p, int row, int col) {
    const QRect rect = cellRect(row, col);
    const Cell& cell = field_->cell(row, col);
    const bool hidden = (cell.state == CellState::Hidden ||
                         cell.state == CellState::Flagged);

    // 极小格子：只画基础色块，保证超大棋盘刷新速度
    if (cellPx_ < 6) {
        if (hidden) {
            p.fillRect(rect, kFaceColor);
        } else {
            p.fillRect(rect, kRevealedFace);
        }
        if (cell.state == CellState::Flagged) {
            p.fillRect(rect.adjusted(rect.width() / 3, rect.height() / 3,
                                     -rect.width() / 3, -rect.height() / 3),
                       QColor(208, 0, 0));
        } else if (!hidden && cell.detonated) {
            p.fillRect(rect, QColor(255, 0, 0));
        } else if (!hidden && cell.mine) {
            p.fillRect(rect, QColor(25, 25, 25));
        }
        return;
    }

    if (hidden) {
        drawRaisedBevel(p, rect);
        if (cell.state == CellState::Flagged)
            drawFlag(p, rect);
        return;
    }

    // 已翻开
    p.fillRect(rect, kRevealedFace);
    p.setPen(QColor(128, 128, 128));
    p.drawRect(rect.adjusted(0, 0, -1, -1));

    if (cell.detonated) {
        p.fillRect(rect.adjusted(1, 1, -1, -1), QColor(255, 0, 0));
    }
    if (cell.wrongFlag) {
        drawWrongFlag(p, rect);
    } else if (cell.mine) {
        drawMine(p, rect);
    } else if (cell.adjacent > 0) {
        drawNumber(p, rect, cell.adjacent);
    }
}

void BoardWidget::mousePressEvent(QMouseEvent* event) {
    if (!field_)
        return;
    const int r = event->y() / cellPx_;
    const int c = event->x() / cellPx_;
    if (!field_->inBounds(r, c))
        return;

    if (event->button() == Qt::LeftButton) {
        if (field_->cell(r, c).state == CellState::Revealed)
            emit chordRequested(r, c);
        else
            emit leftClicked(r, c);
    } else if (event->button() == Qt::RightButton) {
        emit rightClicked(r, c);
    } else if (event->button() == Qt::MiddleButton) {
        emit chordRequested(r, c);
    }
}
