#include "mainwindow.h"
#include "./ui_mainwindow.h"

#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QMessageBox>
#include <QTableWidget>
#include <QHeaderView>
#include <QStringList>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    // Настраиваем таблицу: 4 столбца под поля варианта 2
    ui->tableWidget->setColumnCount(4);

    QStringList headers;
    headers << tr("Хеш")
            << tr("Счёт списания")
            << tr("Счёт зачисления")
            << tr("Дата/время (ISO 8601)");
    ui->tableWidget->setHorizontalHeaderLabels(headers);

    // Растягиваем первый столбец (хеш), остальные по содержимому
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        0, QHeaderView::Stretch);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        1, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        2, QHeaderView::ResizeToContents);
    ui->tableWidget->horizontalHeader()->setSectionResizeMode(
        3, QHeaderView::ResizeToContents);

    // Пытаемся найти файл данных рядом с exe или на уровень выше (корень проекта)
    const QString appDir = QCoreApplication::applicationDirPath();
    QString filePath = appDir + "/transactions.txt";
    if (!QFile::exists(filePath)) {
        filePath = appDir + "/../transactions.txt";
    }

    loadTransactions(filePath);
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::loadTransactions(const QString &fileName)
{
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        QMessageBox::warning(this,
                             tr("Ошибка"),
                             tr("Не удалось открыть файл данных:\n%1")
                                 .arg(fileName));
        return;
    }

    QTextStream in(&file);
#if QT_VERSION < QT_VERSION_CHECK(6, 0, 0)
    in.setCodec("UTF-8");
#endif

    QStringList buffer;
    int row = 0;

    while (!in.atEnd()) {
        QString line = in.readLine().trimmed();

        if (line.isEmpty()) {
            // Пустая строка – просто разделитель, пропускаем
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

    // Если в конце файла оказался не полный блок, просто игнорируем
    if (!buffer.isEmpty() && buffer.size() != 4) {
        QMessageBox::information(
            this,
            tr("Предупреждение"),
            tr("Последняя запись в файле неполная и была пропущена."));
    }
}
