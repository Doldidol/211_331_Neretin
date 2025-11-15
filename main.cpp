#include <QApplication>
#include <QMessageBox>
#include <QCryptographicHash>

#include "mainwindow.h"
#include "pindialog.h"

namespace
{
// Хешируем строку через SHA-256 и возвращаем hex-строку
QString sha256Hex(const QString &text)
{
    QCryptographicHash hash(QCryptographicHash::Sha256);
    hash.addData(text.toUtf8());
    return QString::fromLatin1(hash.result().toHex());
}
} // anonymous namespace

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);

    // Хеш пин-кода "1234" (SHA-256, hex)
    const QString kStoredPinHash =
        QStringLiteral("03ac674216f3e15c761ee1a5e255f067953623c8b388b4459e13f978d7c846f4");

    // Показываем диалог ввода пин-кода
    PinDialog dlg;
    if (dlg.exec() != QDialog::Accepted) {
        // Пользователь нажал "Отмена" или закрыл окно
        return 0;
    }

    const QString enteredPin  = dlg.pin();
    const QString enteredHash = sha256Hex(enteredPin);

    if (enteredHash != kStoredPinHash) {
        QMessageBox::warning(
            nullptr,
            QObject::tr("Ошибка"),
            QObject::tr("Неверный пин-код. Приложение будет закрыто.")
            );
        return 0;  // неправильный пин — завершаем работу
    }

    // Пин-код верный — открываем основное окно
    MainWindow w;
    w.show();

    return a.exec();
}
