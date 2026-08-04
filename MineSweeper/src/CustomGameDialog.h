#ifndef CUSTOMGAMEDIALOG_H
#define CUSTOMGAMEDIALOG_H

#include <QDialog>

class QSpinBox;

// 自定义难度对话框：行数、列数、地雷数
class CustomGameDialog : public QDialog {
    Q_OBJECT
public:
    explicit CustomGameDialog(QWidget* parent = nullptr);

    int rows() const;
    int cols() const;
    int mines() const;
    void setValues(int rows, int cols, int mines);

protected:
    void accept() override;

private:
    QSpinBox* rowsBox_ = nullptr;
    QSpinBox* colsBox_ = nullptr;
    QSpinBox* minesBox_ = nullptr;
};

#endif  // CUSTOMGAMEDIALOG_H
