#include "CustomGameDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>

CustomGameDialog::CustomGameDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("自定义游戏"));

    rowsBox_ = new QSpinBox(this);
    rowsBox_->setRange(9, 1000);
    rowsBox_->setValue(16);
    colsBox_ = new QSpinBox(this);
    colsBox_->setRange(9, 1000);
    colsBox_->setValue(16);
    minesBox_ = new QSpinBox(this);
    minesBox_->setRange(1, 999999);
    minesBox_->setValue(40);
    connect(rowsBox_, qOverload<int>(&QSpinBox::valueChanged), this,
            &CustomGameDialog::updateMinesMax);
    connect(colsBox_, qOverload<int>(&QSpinBox::valueChanged), this,
            &CustomGameDialog::updateMinesMax);
    updateMinesMax();

    auto* form = new QFormLayout(this);
    form->addRow(QStringLiteral("行数："), rowsBox_);
    form->addRow(QStringLiteral("列数："), colsBox_);
    form->addRow(QStringLiteral("地雷数："), minesBox_);

    auto* buttons = new QDialogButtonBox(QDialogButtonBox::Ok |
                                             QDialogButtonBox::Cancel,
                                         this);
    connect(buttons, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
    form->addRow(buttons);
}

int CustomGameDialog::rows() const {
    return rowsBox_->value();
}

int CustomGameDialog::cols() const {
    return colsBox_->value();
}

int CustomGameDialog::mines() const {
    return minesBox_->value();
}

void CustomGameDialog::setValues(int rows, int cols, int mines) {
    rowsBox_->setValue(rows);
    colsBox_->setValue(cols);
    minesBox_->setValue(mines);
}

void CustomGameDialog::updateMinesMax() {
    const qint64 maxMines =
        qint64(rowsBox_->value()) * colsBox_->value() - 1;
    const int max = static_cast<int>(qMin<qint64>(maxMines, 999999));
    minesBox_->setMaximum(qMax(1, max));
    if (minesBox_->value() > minesBox_->maximum())
        minesBox_->setValue(minesBox_->maximum());
}

void CustomGameDialog::accept() {
    const qint64 maxMines = qint64(rows()) * cols() - 1;
    if (mines() > maxMines)
        minesBox_->setValue(static_cast<int>(maxMines));
    QDialog::accept();
}
