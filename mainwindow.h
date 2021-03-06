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

    int takeAPhotoSerial();

    void processImage(QString imgName , int idProc );

    void identifyColorPixels(QString imgName);

    int displayImage(QString imgName );

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

    qreal drawMaxLine(float *lstHistLens, bool vertical);

    void on_actionVertical_histogram_triggered();

    void on_actionHorizontal_histogram_triggered();

    int getMaxHist(float* lstHistLens, int numLens );

    float* calcVerticalHistogram(float *lstBarsLens, bool drawHist);

    float* calcHorizontalHistogram( float* lstBarsLens, bool drawHist);

    void on_actionClear_triggered();

    void on_actionNDVI_drawing_triggered();

    void on_actionLoad_file_triggered();

    void on_actionstrikeArea_triggered();

    int on_actionTracking_triggered();

    int centeringCamera(qreal x, qreal y);

    void motorJumpToThe( QString side, int degrees );

    void on_actionTraking_timer_triggered();

private:
    Ui::MainWindow *ui;
};

#endif // MAINWINDOW_H
