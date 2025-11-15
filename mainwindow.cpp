#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QStringList>
#include <QFileDialog>

// OpenSSL
#include <openssl/evp.h>

namespace
{

// Преобразуем hex-строку в сырые байты
QByteArray hexToBytes(const QString &hex)
{
    QByteArray result;
    QString clean = hex.trimmed();
    if (clean.isEmpty())
        return result;

    if (clean.size() % 2 != 0) {
        // нечётное число символов — ошибка
        return QByteArray();
    }

    result.reserve(clean.size() / 2);
    for (int i = 0; i < clean.size(); i += 2) {
        QString byteStr = clean.mid(i, 2);
        bool ok = false;
        int val = byteStr.toInt(&ok, 16);
        if (!ok) {
            return QByteArray();
        }
        result.append(static_cast<char>(val));
    }
    return result;
}

// Расшифровка AES-256-CBC через OpenSSL
QByteArray decryptAes256Cbc(const QByteArray &cipher,
                            const QByteArray &key,
                            const QByteArray &iv,
                            bool *ok = nullptr)
{
    if (ok)
        *ok = false;

    if (key.size() != 32 || iv.size() != 16 || cipher.isEmpty()) {
        return QByteArray();
    }

    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (!ctx) {
        return QByteArray();
    }

    QByteArray plain;
    plain.resize(cipher.size() + EVP_MAX_BLOCK_LENGTH);

    int len = 0;
    int plainLen = 0;

    if (1 != EVP_DecryptInit_ex(ctx,
                                EVP_aes_256_cbc(),
                                nullptr,
                                reinterpret_cast<const unsigned char*>(key.constData()),
                                reinterpret_cast<const unsigned char*>(iv.constData()))) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    if (1 != EVP_DecryptUpdate(ctx,
                               reinterpret_cast<unsigned char*>(plain.data()),
                               &len,
                               reinterpret_cast<const unsigned char*>(cipher.constData()),
                               cipher.size())) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    plainLen = len;

    if (1 != EVP_DecryptFinal_ex(ctx,
                                 reinterpret_cast<unsigned char*>(plain.data()) + len,
                                 &len)) {
        EVP_CIPHER_CTX_free(ctx);
        return QByteArray();
    }

    plainLen += len;
    EVP_CIPHER_CTX_free(ctx);

    plain.resize(plainLen);

    if (ok)
        *ok = true;

    return plain;
}

} // anonymous namespace

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Настраиваем таблицу: 4 столбца
    ui->tableWidget->setColumnCount(4);

    QStringList headers;
    headers << tr("Хеш")
            << tr("Счёт списания")
            << tr("Счёт зачисления")
            << tr("Дата/время (ISO 8601)");
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::ResizeToContents);

    setWindowTitle(tr("Банковские транзакции — вариант 2"));
    resize(900, 400);

    // НИЧЕГО НЕ ОТКРЫВАЕМ АВТОМАТИЧЕСКИ.
    // Пользователь сам нажимает "Открыть..." и выбирает файл.
}

MainWindow::~MainWindow()
{
    delete ui;
}

// Слот для кнопки "Открыть..."
void MainWindow::on_openButton_clicked()
{
    const QString appDir = QCoreApplication::applicationDirPath();

    QString fileName = QFileDialog::getOpenFileName(
        this,
        tr("Выбор файла с транзакциями"),
        appDir,
        tr("Зашифрованные файлы (*.enc);;Все файлы (*.*)")
        );

    if (fileName.isEmpty())
        return;

    // Ищем aes_key.txt рядом с приложением
    QString keyPath = appDir + "/aes_key.txt";
    if (!QFile::exists(keyPath)) {
        keyPath = appDir + "/../aes_key.txt";
    }

    if (!QFile::exists(keyPath)) {
        QMessageBox::warning(
            this,
            tr("Ошибка"),
            tr("Файл ключа aes_key.txt не найден рядом с приложением.")
            );
        return;
    }

    // Очищаем таблицу и загружаем выбранный файл
    ui->tableWidget->setRowCount(0);
    loadTransactionsEncrypted(fileName, keyPath);
}

// Чтение aes_key.txt, расшифровка AES-256-CBC и разбор записей
void MainWindow::loadTransactionsEncrypted(const QString &encFileName,
                                           const QString &keyFileName)
{
    // 1. Читаем ключ/iv
    QFile keyFile(keyFileName);
    if (!keyFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this,
                             tr("Ошибка"),
                             tr("Не удалось открыть файл ключа:\n%1")
                                 .arg(keyFileName));
        return;
    }

    QTextStream keyStream(&keyFile);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    keyStream.setCodec("UTF-8");
#endif

    QString keyHex;
    QString ivHex;

    while (!keyStream.atEnd() && keyHex.isEmpty()) {
        keyHex = keyStream.readLine().trimmed();
    }
    while (!keyStream.atEnd() && ivHex.isEmpty()) {
        ivHex = keyStream.readLine().trimmed();
    }

    if (keyHex.isEmpty() || ivHex.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Ошибка"),
                             tr("В файле ключа должно быть две непустые строки:\n"
                                "ключ и IV в hex."));
        return;
    }

    QByteArray keyBytes = hexToBytes(keyHex);
    QByteArray ivBytes  = hexToBytes(ivHex);

    if (keyBytes.size() != 32 || ivBytes.size() != 16) {
        QMessageBox::warning(
            this,
            tr("Ошибка"),
            tr("Неверные размеры key/iv.\n"
               "Ожидается 32 байта (64 hex символа) для ключа\n"
               "и 16 байт (32 hex символа) для IV."));
        return;
    }

    // 2. Читаем зашифрованный файл
    QFile encFile(encFileName);
    if (!encFile.open(QIODevice::ReadOnly)) {
        QMessageBox::warning(this,
                             tr("Ошибка"),
                             tr("Не удалось открыть зашифрованный файл:\n%1")
                                 .arg(encFileName));
        return;
    }

    QByteArray cipher = encFile.readAll();
    if (cipher.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Ошибка"),
                             tr("Зашифрованный файл пустой или ошибка чтения."));
        return;
    }

    // 3. Расшифровываем
    bool ok = false;
    QByteArray plain = decryptAes256Cbc(cipher, keyBytes, ivBytes, &ok);
    if (!ok || plain.isEmpty()) {
        QMessageBox::warning(this,
                             tr("Ошибка"),
                             tr("Не удалось расшифровать файл. "
                                "Проверьте ключ, IV и режим AES-256-CBC."));
        return;
    }

    // 4. Разбираем расшифрованный текст как обычный transactions.txt
    ui->tableWidget->setRowCount(0);

    QTextStream in(plain);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif

    QStringList buffer;
    int row = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.isEmpty()) {
            continue;
        }

        buffer << line;

        // Одна транзакция = 4 непустые строки: hash, from, to, datetime
        if (buffer.size() == 4) {
            ui->tableWidget->insertRow(row);

            ui->tableWidget->setItem(row, 0,
                                     new QTableWidgetItem(buffer.at(0)));
            ui->tableWidget->setItem(row, 1,
                                     new QTableWidgetItem(buffer.at(1)));
            ui->tableWidget->setItem(row, 2,
                                     new QTableWidgetItem(buffer.at(2)));
            ui->tableWidget->setItem(row, 3,
                                     new QTableWidgetItem(buffer.at(3)));

            buffer.clear();
            ++row;
        }
    }

    if (!buffer.isEmpty() && buffer.size() != 4) {
        QMessageBox::information(
            this,
            tr("Предупреждение"),
            tr("Последняя запись в файле неполная и была пропущена."));
    }
}
