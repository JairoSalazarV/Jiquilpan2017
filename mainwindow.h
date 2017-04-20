#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

private slots:
    void on_actionConnect_triggered();

    void updateLastConnection();

    QString serialPortReadLine(quint64 timeToWait);

    bool saveFile( QString fileName, QString contain );

    QString readAllFile( QString filePath );

    void loadLastConnection();

    void on_actionSpray_triggered();

    void QtDelay( unsigned int ms );

    bool saveBinFile(QString fileName, char *dataPtr, unsigned long datasize);

    void on_comboBoxResol_currentIndexChanged(int index);

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
