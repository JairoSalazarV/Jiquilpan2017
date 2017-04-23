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

    void processImage(QString imgName , int idProc );

    void identifyColorPixels(QString imgName);

    void displayImage(QString imgName );

    void QtDelay( unsigned int ms );

    bool saveBinFile(QString fileName, char *dataPtr, unsigned long datasize);

    void on_comboBoxResol_currentIndexChanged(int index);

    void on_actioncolorIdentification_triggered();

    void funcShowMsg(QString title, QString msg);

    void sendMessageBySerial(QString msg, int len);

    void on_pbUp_clicked();

    void on_pbDown_clicked();

    void on_pbRight_clicked();

    void on_pbLeft_clicked();

    void on_pbZero_clicked();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
