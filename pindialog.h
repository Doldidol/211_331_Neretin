#ifndef PINDIALOG_H
#define PINDIALOG_H

#include <QDialog>

QT_BEGIN_NAMESPACE
namespace Ui { class PinDialog; }
QT_END_NAMESPACE

class PinDialog : public QDialog
{
    Q_OBJECT

public:
    explicit PinDialog(QWidget *parent = nullptr);
    ~PinDialog();

    QString pin() const;

private:
    Ui::PinDialog *ui;
};

#endif // PINDIALOG_H
