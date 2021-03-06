// sudo apt-get install libqt4-dev libqt4-dev-bin libqt4-opengl-dev libqtwebkit-dev qt4-linguist-tools qt4-qmake


#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSerialPort>
#include <QDebug>
#include <QFile>
#include <QTime>
#include <QGraphicsPixmapItem>

#include <QMessageBox>
#include <QFileDialog>
#include <math.h>

#define _PATH_LAST_CONN_SETT                "lastConnection.arduCAM"
#define _PATH_REC_IMG                       "imgReceived.jpg"
#define _PATH_IMAGE_TO_DISPLAY              "imgToDisplay.jpg"
#define _FILE_NOT_EXIST                     "-1"
#define _FILE_UNKNOW_ERROR                  "-2"
#define _MAX_SIZE_IMAGE                     250000
#define _SERIAL_BUFF_SIZE                   80
#define _IMG_W                              320
#define _IMG_H                              240

#define _FLAG_PROC_COL_IDENTIFY             1
#define _FLAG_PROC_NDVI                     2

#define _MOTOR_STEP_LEN                     8
#define _MOTOR_STEP_CENTERING               1
#define _MOTOR_STEP_DELAY                   10 //ms
#define _MOTOR_STEP_PIXELS                  7 //pixels

#define _TIMER_DELAY                        1
#define _TIMER_UPDATE_VIEW                  1

#define _HIST_VER_NUM                       30
#define _HIST_VER_COVER                     0.22
#define _HIST_HORIZ_NUM                     40
#define _HIST_HORIZ_COVER                   0.3

#define _NDVI_THRESHOLD                     0.0     //[-1,1]

#define _STRIKE_AREA_THRESHOLD              0.15    //(0,1)

#define _IMG_MIN_SIZE                       3000
#define _IMG_MAX_SIZE                       7000
#define _IMG_MAX_CORRUPTED                  3

#define _ERROR                              -1
#define _OK                                 1

int numImgCorrupted;

QSerialPort* serialPort;

bool flagConnected;
bool flagTrackinOn;

QGraphicsRectItem* strikeAreaScene;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboBoxResol->addItems(QStringList({"160x120","176x144","320x240","352x288","640x480","800x600","1024x768","1280x1024","1600x1200"}));
    ui->comboBoxBaud->addItems( QStringList({"9600","19200","38400","57600","256000","115200","921600"}) );
    ui->comboBoxPort->addItems( QStringList({"ttyACM0","ttyACM1","ttyACM2","ttyACM3","ttyAC4","ttyACM5","ttyACM6","ttyACM7"}) );

    serialPort = new QSerialPort();

    loadLastConnection();

    flagConnected = false;

    displayImage( _PATH_IMAGE_TO_DISPLAY );

    numImgCorrupted = 0;

    flagTrackinOn = false;

}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::on_actionConnect_triggered()
{

    if( flagConnected == true )
    {
        serialPort->close();
        ui->mainToolBar->actions().at(1)->setIcon(QIcon(":/new/prefix1/imagenes/connect.png"));
        ui->mainToolBar->actions().at(1)->setToolTip("Connect");
        flagConnected = false;
        qDebug() << "serialPort close()";
    }
    else
    {
        qDebug() << "Pix: " << ui->comboBoxResol->currentText().toStdString().c_str();
        qDebug() << "Baudrate: " << ui->comboBoxBaud->currentText().toInt(0);
        qDebug() << "Port: " << ui->comboBoxPort->currentText().toStdString().c_str();

        serialPort->setBaudRate( ui->comboBoxBaud->currentText().toInt(0) );
        serialPort->setPortName( ui->comboBoxPort->currentText().toStdString().c_str() );
        if( serialPort->open(QIODevice::ReadWrite) )
        {
            qDebug() << "Connected to ArduCAM Successfully";
            flagConnected = true;
            ui->mainToolBar->actions().at(1)->setIcon(QIcon(":/new/prefix1/imagenes/disconnect.png"));
            ui->mainToolBar->actions().at(1)->setToolTip("Disconnect");

            //Save last connection
            updateLastConnection();

            //Set arduCAM resolution
            serialPort->write( QString::number(ui->comboBoxResol->currentIndex()).toStdString().c_str(), 1 );

            //Red messages from arduino if existes
            QString tmp;
            tmp = serialPortReadLine(10);
            if( !tmp.isEmpty() )qDebug() << tmp;
            tmp = serialPortReadLine(10);
            if( !tmp.isEmpty() )qDebug() << tmp;
        }
        else
        {
            qDebug() << "Error: connecting to ArduCAM " << ui->comboBoxPort->currentText().toStdString().c_str();
        }
    }
}

void MainWindow::updateLastConnection()
{
    QString lastConnSett;
    lastConnSett = QString::number(ui->comboBoxResol->currentIndex());
    lastConnSett.append(",");
    lastConnSett.append( QString::number(ui->comboBoxBaud->currentIndex() ) );
    lastConnSett.append(",");
    lastConnSett.append( QString::number(ui->comboBoxPort->currentIndex()) );
    saveFile(_PATH_LAST_CONN_SETT,lastConnSett);
}


QString MainWindow::serialPortReadLine(quint64 timeToWait)
{
    QByteArray responseData = serialPort->readAll();
    while( serialPort->waitForReadyRead(timeToWait) )
    {
        responseData += serialPort->readAll();
    }
    QString response(responseData);
    return response;
}

bool MainWindow::saveFile( QString fileName, QString contain )
{
    QFile file(fileName);
    if(file.exists()){
        if(!file.remove()){
            return false;
        }
    }
    if (file.open(QIODevice::ReadWrite)) {
        QTextStream stream(&file);
        stream << contain << endl;
        file.close();
    }else{
        return false;
    }
    return true;
}

QString MainWindow::readAllFile( QString filePath )
{
    QFile tmpFile(filePath);
    if( tmpFile.exists() )
    {
        tmpFile.open(QIODevice::ReadOnly);
        QTextStream tmpStream(&tmpFile);
        return tmpStream.readAll();
    }
    else
    {
        return _FILE_NOT_EXIST;
    }
    return _FILE_UNKNOW_ERROR;
}

void MainWindow::loadLastConnection()
{
    QString tmpLastConn = readAllFile( _PATH_LAST_CONN_SETT );

    if( tmpLastConn != _FILE_NOT_EXIST && tmpLastConn != _FILE_UNKNOW_ERROR )
    {
        QStringList lstLastConn = tmpLastConn.split(",");
        if( lstLastConn.size() < 3 )
            qDebug() << "ERROR: Last connection settings coerrupt";
        else
        {
            ui->comboBoxResol->setCurrentIndex( lstLastConn.first().toInt() );
            lstLastConn.removeFirst();

            ui->comboBoxBaud->setCurrentIndex( lstLastConn.first().toInt() );
            lstLastConn.removeFirst();

            ui->comboBoxPort->setCurrentIndex( lstLastConn.first().toInt() );
        }
    }


}

void MainWindow::on_actionSpray_triggered()
{
    takeAPhotoSerial();
}

int MainWindow::takeAPhotoSerial()
{
    if( flagConnected == true )
    {

        serialPort->write("9",1);

        int numRead = 0, numReadTotal = 0;
        char buffer[_SERIAL_BUFF_SIZE];
        char receivedFileData[_MAX_SIZE_IMAGE];


        serialPort->readLine(buffer,_SERIAL_BUFF_SIZE);
        serialPort->waitForReadyRead(300);

        memset(buffer,'\0',_SERIAL_BUFF_SIZE);
        serialPort->readLine(buffer,_SERIAL_BUFF_SIZE);
        serialPort->waitForReadyRead(300);
        qDebug() << "tmpMsg1: " << buffer;

        memset(buffer,'\0',_SERIAL_BUFF_SIZE);
        serialPort->readLine(buffer,_SERIAL_BUFF_SIZE);
        serialPort->waitForReadyRead(300);
        qDebug() << "tmpMsg2: " << buffer;

        //QByteArray responseData;
        memset(receivedFileData,'\0',_MAX_SIZE_IMAGE);
        numReadTotal = 0;
        while( serialPort->waitForReadyRead(10) )
        {
            memset(buffer,'\0',_SERIAL_BUFF_SIZE);
            numRead  = serialPort->read(buffer, _SERIAL_BUFF_SIZE);
            if( numRead != _SERIAL_BUFF_SIZE )
            {
                //qDebug() << "numRead: " << numRead << " of " << _SERIAL_BUFF_SIZE;
            }

            memcpy(&receivedFileData[numReadTotal],buffer,numRead);
            numReadTotal += numRead;

        }

        if( numReadTotal > _IMG_MIN_SIZE && numReadTotal < _IMG_MAX_SIZE )//Heuristic
        {
            saveBinFile(_PATH_REC_IMG, receivedFileData, numReadTotal);

            saveBinFile(_PATH_IMAGE_TO_DISPLAY, receivedFileData, numReadTotal);

            if( displayImage(_PATH_IMAGE_TO_DISPLAY) == _ERROR )
            {
                qDebug() << "ERROR desplaying image";
                return _ERROR;
            }
        }
        qDebug() << "Received numReadTotal: " << numReadTotal;
    }
    else
    {
        funcShowMsg("Alert!","SerialPort not connected");
        return _ERROR;
    }
    return _OK;
}

void MainWindow::processImage( QString imgName, int idProc )
{
    //Apply the algorithm over the received image and save into _PATH_PROC_IMG
    switch( idProc )
    {
        case _FLAG_PROC_COL_IDENTIFY:
            identifyColorPixels(imgName);
            break;
        case _FLAG_PROC_NDVI:
            break;
        default:
            break;
    }
}

void MainWindow::identifyColorPixels( QString imgName )
{
    QImage img( imgName );
    img.save(_PATH_IMAGE_TO_DISPLAY);
}

int MainWindow::displayImage(QString imgName )
{
    //Prepare image size
    QPixmap itemPix(imgName);
    if( itemPix.isNull() )
    {
        //numImgCorrupted++;
        qDebug() << "ERROR transfering image";
        return _ERROR;
    }
    //if( numImgCorrupted >= _IMG_MAX_CORRUPTED )
    //{
    //    funcShowMsg("ERROR","Check the camera connection");
    //    numImgCorrupted = 0;
    //    flagTrackinOn = false;
    //    return _ERROR;
    //}

    itemPix = itemPix.scaled(_IMG_W-2, _IMG_H-2, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    //Add image to scene
    QGraphicsScene* scene = new QGraphicsScene();
    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(itemPix);
    scene->addItem(item);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    ui->graphicsView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    //numImgCorrupted = 0;
    return _OK;
}

void MainWindow::QtDelay( unsigned int ms ){
    QTime dieTime= QTime::currentTime().addMSecs(ms);
    while (QTime::currentTime() < dieTime){
        QCoreApplication::processEvents(QEventLoop::AllEvents, 100);
    }
}

bool MainWindow::saveBinFile(QString fileName, char *dataPtr, unsigned long datasize)
{
    QFile DummyFile(fileName);
    if(DummyFile.open(QIODevice::WriteOnly))
    {
        qint64 bytesWritten = DummyFile.write(reinterpret_cast<const char*>(dataPtr), datasize);
        if (bytesWritten < (qint64)datasize)
        {
            return false;
        }
        DummyFile.close();
    }
    return true;
}

void MainWindow::on_comboBoxResol_currentIndexChanged(int index)
{
    if( flagConnected == true )
    {
        serialPort->write( QString::number(index).toStdString().c_str(), 1 );
        updateLastConnection();
        qDebug() << "Resolution changed to: " << ui->comboBoxResol->currentText();
    }
}

void MainWindow::on_actioncolorIdentification_triggered()
{
    processImage( _PATH_REC_IMG, _FLAG_PROC_COL_IDENTIFY );

    displayImage( _PATH_IMAGE_TO_DISPLAY );
}

void MainWindow::funcShowMsg(QString title, QString msg)
{
    QMessageBox yesNoMsgBox;
    yesNoMsgBox.setWindowTitle(title);
    yesNoMsgBox.setText(msg);
    yesNoMsgBox.setDefaultButton(QMessageBox::Ok);
    yesNoMsgBox.exec();
}

void MainWindow::on_pbUp_clicked()
{
    sendMessageBySerial("U",1);
}

void MainWindow::on_pbDown_clicked()
{
    sendMessageBySerial("D",1);
}

void MainWindow::on_pbRight_clicked()
{
    sendMessageBySerial("R",1);
}

void MainWindow::on_pbLeft_clicked()
{
    sendMessageBySerial("L",1);
}

void MainWindow::on_pbZero_clicked()
{
    sendMessageBySerial("Z",1);
}

void MainWindow::sendMessageBySerial(QString msg, int len)
{
    if( flagConnected == false )
    {
        funcShowMsg("Alert","It is not connected to SerialPort");
    }
    else
    {
        if( msg=="U" || msg=="D" || msg=="L" || msg=="R" )
        {
            int i;
            for(i=1; i<=_MOTOR_STEP_LEN; i++)
            {
                serialPort->write(msg.toStdString().c_str(),len);
                serialPort->waitForBytesWritten(10);
                QtDelay(50);
            }
        }
        else
        {
            serialPort->write(msg.toStdString().c_str(),len);
            serialPort->waitForBytesWritten(50);
        }
    }
}

void MainWindow::on_actionVertical_histogram_triggered()
{
    //Display histogram
    float* lstHistLens = (float*)malloc(_HIST_VER_NUM*sizeof(float));
    lstHistLens = calcVerticalHistogram( lstHistLens, true );

    //Display Line
    drawMaxLine( lstHistLens, true );
}

void MainWindow::on_actionHorizontal_histogram_triggered()
{
    //Display histogram
    float* lstHistLens = (float*)malloc(_HIST_HORIZ_NUM*sizeof(float));
    lstHistLens = calcHorizontalHistogram( lstHistLens, true );

    //Display Line
    drawMaxLine( lstHistLens, false );
}

qreal MainWindow::drawMaxLine(float* lstHistLens, bool vertical)
{
    qreal POS;
    if( vertical == true )
    {
        int maxIndex = getMaxHist( lstHistLens, _HIST_VER_NUM );
        qreal x1,y1;
        qreal x2,y2;
        float histW = (float)_IMG_H / (float)_HIST_VER_NUM;
        x1  = 0.0;
        y1  = (maxIndex+0)*histW;
        x2  = (float)_IMG_W;
        y2  = y1;
        POS = y1;
        QGraphicsLineItem* tmpLine = new QGraphicsLineItem();
        tmpLine->setLine(x1,y1,x2,y2);
        tmpLine->setPen(QPen(Qt::yellow));
        ui->graphicsView->scene()->addItem(tmpLine);
    }
    else
    {
        int maxIndex = getMaxHist( lstHistLens, _HIST_HORIZ_NUM );
        qreal x1,y1;
        qreal x2,y2;
        float histW = (float)_IMG_H / (float)_HIST_VER_NUM;
        x1  = (maxIndex+0)*histW;
        y1  = 0.0;
        x2  = x1;
        y2  = (float)_IMG_H;
        POS = x1;
        QGraphicsLineItem* tmpLine = new QGraphicsLineItem();
        tmpLine->setLine(x1,y1,x2,y2);
        tmpLine->setPen(QPen(Qt::yellow));
        ui->graphicsView->scene()->addItem(tmpLine);
    }

    return POS;
}

int MainWindow::getMaxHist( float* lstHistLens, int numLens )
{
    int i;
    int maxIndex = 0;
    int tmpLen, maxLen;
    maxLen = 0;
    for( i=0; i<numLens; i++ )
    {
        if( i==0 )
            tmpLen = (lstHistLens[0]*2) + lstHistLens[1];
        else if( i==(numLens-1) )
            tmpLen = (lstHistLens[numLens-1]*2) + lstHistLens[numLens-2];
        else
            tmpLen = lstHistLens[i-1] + lstHistLens[i] + lstHistLens[i+1];

        if( tmpLen >= maxLen )
        {
            maxLen   = tmpLen;
            maxIndex = i;
        }
        //qDebug() << i << ".- tmpLen: " << tmpLen;
    }
    return maxIndex;
}

float* MainWindow::calcVerticalHistogram( float* lstBarsLens, bool drawHist )
{
    if( ui->checkBoxClearScene->isChecked() )
    {
        ui->graphicsView->scene()->clear();
        displayImage( _PATH_IMAGE_TO_DISPLAY );
    }

    int numHist = _HIST_VER_NUM;//Cuantas barras

    QImage img( _PATH_IMAGE_TO_DISPLAY );
    int barraW = round( (float)img.height() / (float)numHist );
    int idHist, x, y, lineIni, lineEnd, barraAcum, normAcum;
    int lstBars[numHist];
    QColor tmpColor;
    float histMaxLen = _HIST_VER_COVER * (float)ui->graphicsView->width();//%
    //qDebug() << "histMaxLen: " << histMaxLen;

    //Get bars size
    normAcum = 0;
    for( idHist=0; idHist<numHist; idHist++ )
    {
        lineIni     = (idHist*barraW);
        lineEnd     = lineIni+barraW;
        barraAcum   = 0;
        for( y=lineIni; y<lineEnd; y++ )
        {
            for( x=0; x<img.width(); x++ )
            {
                tmpColor = img.pixelColor(x,y);
                //barraAcum += abs( tmpColor.red() - tmpColor.blue() );
                barraAcum += 255.0 * ((float)(tmpColor.red() - tmpColor.green()) / (float)(tmpColor.red() + tmpColor.green()));

            }
        }
        lstBars[idHist] = ((float)barraAcum / (float)(barraW*img.width()));
        lstBars[idHist] = ( lstBars[idHist] > 0 )?lstBars[idHist]:0;
        normAcum += lstBars[idHist];
    }

    int maxHistVal = 0;
    for( idHist=0; idHist<numHist; idHist++ )
    {
        if( lstBars[idHist] > maxHistVal )
            maxHistVal = lstBars[idHist];
    }

    //Normilizing bar size
    qreal rectX,rectY,rectW,rectH;
    qreal widthInScene = (float)_IMG_H/(float)numHist;
    for( idHist=0; idHist<numHist; idHist++ )
    {
        rectX = 0.0;
        rectY = ((float)idHist*widthInScene) + (widthInScene*0.2);
        rectW = ((float)lstBars[idHist] / (float)maxHistVal)*histMaxLen;
        rectH = widthInScene/2.0;
        lstBarsLens[idHist] = rectW;

        //qDebug() << "x: " << rectX << " y: "<<rectY << "w: " << rectW << " h: "<<rectH << " barraW: " << barraW;
        if( drawHist == true )
        {
            QGraphicsRectItem *tmpRect = new QGraphicsRectItem();
            tmpRect->setRect(rectX, rectY, rectW, rectH);
            tmpRect->setPen( QPen(Qt::black) );
            tmpRect->setBrush( QBrush(Qt::red) );
            ui->graphicsView->scene()->addItem(tmpRect);
        }
    }


    return lstBarsLens;
}

float* MainWindow::calcHorizontalHistogram( float* lstBarsLens, bool drawHist )
{
    if( ui->checkBoxClearScene->isChecked() )
    {
        ui->graphicsView->scene()->clear();
        displayImage( _PATH_IMAGE_TO_DISPLAY );
    }

    int numHist = _HIST_HORIZ_NUM;//Cuantas barras

    QImage img( _PATH_IMAGE_TO_DISPLAY );
    int barraW = round( (float)img.width() / (float)numHist );
    int idHist, x, y, lineIni, lineEnd, barraAcum;
    int lstBars[numHist];
    QColor tmpColor;
    float histMaxLen = _HIST_HORIZ_COVER * (float)ui->graphicsView->height();//%
    //qDebug() << "histMaxLen: " << histMaxLen;

    //Get bars size
    for( idHist=0; idHist<numHist; idHist++ )
    {
        lineIni     = (idHist*barraW);
        lineEnd     = lineIni+barraW;
        barraAcum   = 0;
        for( x=lineIni; x<lineEnd; x++ )
        {
            for( y=0; y<img.height(); y++ )
            {
                tmpColor = img.pixelColor(x,y);
                //barraAcum += abs( tmpColor.red() - tmpColor.blue() );
                barraAcum += 255.0 * ((float)(tmpColor.red() - tmpColor.green()) / (float)(tmpColor.red() + tmpColor.green()));
            }
        }
        lstBars[idHist] = ((float)barraAcum / (float)(barraW*img.height()));
        lstBars[idHist] = ( lstBars[idHist] > 0 )?lstBars[idHist]:0;
        //qDebug() << "lstBars: "<<lstBars[idHist];
    }

    int maxHistVal = 0;
    for( idHist=0; idHist<numHist; idHist++ )
    {
        if( lstBars[idHist] > maxHistVal )
            maxHistVal = lstBars[idHist];
    }

    //Normilizing bar size
    qreal rectX,rectY,rectW,rectH;
    qreal widthInScene = (float)_IMG_W/(float)numHist;
    for( idHist=0; idHist<numHist; idHist++ )
    {
        rectH = ((float)lstBars[idHist] / (float)maxHistVal) * histMaxLen;
        rectW = widthInScene/2.0;

        lstBarsLens[idHist] = rectH;

        rectX = ((float)idHist*widthInScene) + (widthInScene*0.15);
        rectX = (rectX<0)?0:rectX;
        rectX = (rectX>_IMG_W)?_IMG_W:rectX;
        rectY = (float)_IMG_H - rectH - 3;
        rectY = (rectY<0)?0:rectY;
        rectY = (rectY>_IMG_H)?_IMG_H:rectY;

        if( drawHist == true )
        {
            QGraphicsRectItem *tmpRect = new QGraphicsRectItem();
            tmpRect->setRect(rectX, rectY, rectW, rectH);
            tmpRect->setPen( QPen(Qt::black) );
            tmpRect->setBrush( QBrush(Qt::red) );
            ui->graphicsView->scene()->addItem(tmpRect);
        }

        //break;

    }

    return lstBarsLens;
}

void MainWindow::on_actionClear_triggered()
{
    ui->graphicsView->scene()->clear();
    displayImage( _PATH_IMAGE_TO_DISPLAY );
}

void MainWindow::on_actionNDVI_drawing_triggered()
{
    QImage img( _PATH_IMAGE_TO_DISPLAY );
    QColor tmpColor;
    int row, col;
    float NDVI;
    for( row=0; row<img.height(); row++ )
    {
        for( col=0; col<img.width(); col++ )
        {
            tmpColor    = img.pixelColor( col, row );
            NDVI        = (float)(tmpColor.red()-tmpColor.green())/(float)(tmpColor.red()+tmpColor.green());

            tmpColor.setRed(0);
            tmpColor.setGreen(0);
            tmpColor.setBlue(0);
            if( NDVI > _NDVI_THRESHOLD )
            {
                //qDebug() << "row: " << row << " col: " << col << "NDVI: " << NDVI;
                tmpColor.setRed(NDVI*255.0);
            }
            img.setPixelColor(col,row,tmpColor);
        }
    }
    img.save( _PATH_IMAGE_TO_DISPLAY );
    displayImage( _PATH_IMAGE_TO_DISPLAY );
}

void MainWindow::on_actionLoad_file_triggered()
{
    //Select image
    //..
    QString auxQstring;
    auxQstring = QFileDialog::getOpenFileName(
                                                        this,
                                                        tr("Select image..."),
                                                        "./snapshots/Calib/",
                                                        "(*.ppm *.RGB888 *.tif *.png *.jpg *.jpeg *.JPEG *.JPG *.bmp);;"
                                                     );
    if( auxQstring.isEmpty() ){
        return (void)NULL;
    }

    QImage tmpImg( auxQstring );
    tmpImg.save( _PATH_IMAGE_TO_DISPLAY );
    displayImage( _PATH_IMAGE_TO_DISPLAY );
}

void MainWindow::on_actionstrikeArea_triggered()
{
    strikeAreaScene = new QGraphicsRectItem();

    qreal strikeX, strikeY, strikeW, strikeH, strikeHalfLenX, strikeHalfLenY;
    strikeHalfLenX = round((float)_IMG_W * ((float)_STRIKE_AREA_THRESHOLD/2.0));
    strikeHalfLenY = round((float)_IMG_H * ((float)_STRIKE_AREA_THRESHOLD/2.0));
    //qDebug() << "strikeHalfLenX: " << strikeHalfLenX << " strikeHalfLenY: " << strikeHalfLenY;

    strikeX = ((float)_IMG_W/2.0)  - strikeHalfLenX;
    strikeY = ((float)_IMG_H/2.0) - strikeHalfLenY;
    //qDebug() << "strikeX: " << strikeX << " strikeY: " << strikeY;
    strikeW = 2.0*strikeHalfLenX;
    strikeH = 2.0*strikeHalfLenY;
    //qDebug() << "strikeW: " << strikeW << " strikeH: " << strikeH;
    strikeAreaScene->setRect(strikeX,strikeY,strikeW,strikeH);
    strikeAreaScene->setPen(QPen(Qt::yellow));
    ui->graphicsView->scene()->addItem( strikeAreaScene );
}

int MainWindow::on_actionTracking_triggered()
{
    int newPhoto = takeAPhotoSerial();
    if( newPhoto == _ERROR )
    {
        qDebug() << "IMG ERROR on tracking";
        return _ERROR;
    }
    ui->graphicsView->update();
    QtDelay(_TIMER_UPDATE_VIEW);

    //Display histogram
    float* histRows = (float*)malloc(_HIST_VER_NUM*sizeof(float));
    float* histCols = (float*)malloc(_HIST_HORIZ_NUM*sizeof(float));

    histRows = calcVerticalHistogram( histRows, false );
    histCols = calcHorizontalHistogram( histCols, false );

    qreal x = drawMaxLine( histCols, false );
    qreal y = drawMaxLine( histRows, true );

    //
    //Centering camera
    //
    if( centeringCamera(x,y) == _ERROR )
        return _ERROR;


    return _OK;
}

int MainWindow::centeringCamera(qreal x, qreal y)
{
    //Get strike-area middle
    strikeAreaScene = new QGraphicsRectItem();
    qreal strikeHalfLenX, strikeHalfLenY;
    qreal strikeX, strikeY;
    qreal errorX, errorY;
    strikeHalfLenX = round((float)_IMG_W  * (_STRIKE_AREA_THRESHOLD/2.0));
    strikeHalfLenY = round((float)_IMG_H * (_STRIKE_AREA_THRESHOLD/2.0));
    strikeX = (float)_IMG_W/2.0;
    strikeY = (float)_IMG_H/2.0;
    errorX  = x - strikeX;
    errorY  = y - strikeY;

    //
    //Centering
    //
    int i;
    qreal absError;

    //x-axis
    absError = abs( errorX );
    if( absError > strikeHalfLenX )
    {
        int stepsX = floor( absError / (float)_MOTOR_STEP_PIXELS );
        qDebug() << "stepsX: " << stepsX;
        for( i=0; i<stepsX; i++ )
        {
            if( errorX > 0 )
                motorJumpToThe( "L", _MOTOR_STEP_CENTERING );
            else
                motorJumpToThe( "R", _MOTOR_STEP_CENTERING );

        }
    }

    //y-axis
    absError = abs( errorY );
    if( absError > strikeHalfLenY )
    {
        int stepsY = floor( absError / (float)_MOTOR_STEP_PIXELS );
        qDebug() << "stepsY: " << stepsY;
        for( i=0; i<stepsY; i++ )
        {
            if( errorY > 0 )
                motorJumpToThe( "D", _MOTOR_STEP_CENTERING );
            else
                motorJumpToThe( "U", _MOTOR_STEP_CENTERING );

        }
    }

    //
    //Refresh image
    //
    QtDelay(_TIMER_UPDATE_VIEW);
    takeAPhotoSerial();

    return _OK;

}

void MainWindow::motorJumpToThe( QString side, int degrees )
{
    int i;
    for( i=0; i<degrees; i++ )
    {
        serialPort->write( side.toStdString().c_str(), 1 );
        QtDelay(_MOTOR_STEP_DELAY);
    }
}

void MainWindow::on_actionTraking_timer_triggered()
{
    while(numImgCorrupted < _IMG_MAX_CORRUPTED )
    {
        if( on_actionTracking_triggered() == _ERROR )
            numImgCorrupted++;
        else
            numImgCorrupted = 0;
        QtDelay( _TIMER_DELAY );
    }
    flagTrackinOn = false;
    numImgCorrupted = 0;
}
