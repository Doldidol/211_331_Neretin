#include "pindialog.h"
#include "ui_pindialog.h"

PinDialog::PinDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::PinDialog)
{
    ui->setupUi(this);

    setWindowTitle(tr("Ввод пин-кода"));
    // Фокус сразу в поле ввода
    ui->lineEditPin->setEchoMode(QLineEdit::Password);
    ui->lineEditPin->setFocus();
}

PinDialog::~PinDialog()
{
    delete ui;
}

QString PinDialog::pin() const
{
    return ui->lineEditPin->text();
}
