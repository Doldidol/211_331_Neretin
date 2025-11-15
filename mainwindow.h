#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

private slots:
    // слот для кнопки "Открыть"
    void on_openButton_clicked();

private:
    Ui::MainWindow *ui;

    // загрузка и расшифровка AES-256 файла
    void loadTransactionsEncrypted(const QString &encFileName,
                                   const QString &keyFileName);
};

#endif // MAINWINDOW_H
