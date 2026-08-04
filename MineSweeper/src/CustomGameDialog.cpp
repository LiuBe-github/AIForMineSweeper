#include "CustomGameDialog.h"

#include <QDialogButtonBox>
#include <QFormLayout>
#include <QSpinBox>

CustomGameDialog::CustomGameDialog(QWidget* parent) : QDialog(parent) {
    setWindowTitle(QStringLiteral("自定义游戏"));

    rowsBox_ = new QSpinBox(this);
    rowsBox_->setRange(9, 30);
    rowsBox_->setValue(16);
    colsBox_ = new QSpinBox(this);
    colsBox_->setRange(9, 30);
    colsBox_->setValue(16);
    minesBox_ = new QSpinBox(this);
    minesBox_->setRange(1, 899);
    minesBox_->setValue(40);

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

void CustomGameDialog::accept() {
    const int maxMines = rows() * cols() - 1;
    if (mines() > maxMines)
        minesBox_->setValue(maxMines);
    QDialog::accept();
}
