#include "mainwindow.h"
#include "ui_mainwindow.h"

#include <QSerialPort>
#include <QDebug>
#include <QFile>
#include <QTime>
#include <QGraphicsPixmapItem>

#include <QMessageBox>

#define _PATH_LAST_CONN_SETT                "lastConnection.arduCAM"
#define _PATH_REC_IMG                       "default.jpg"
#define _PATH_PROC_IMG                      "imgProc.jpg"
#define _FILE_NOT_EXIST                     "-1"
#define _FILE_UNKNOW_ERROR                  "-2"
#define _MAX_SIZE_IMAGE                     250000
#define _SERIAL_BUFF_SIZE                   80
#define _IMG_W                              320
#define _IMG_H                              240

#define _FLAG_PROC_COL_IDENTIFY             1
#define _FLAG_PROC_NDVI                     2

#define _MOTOR_STEP_LEN                     8

QSerialPort* serialPort;

bool flagConnected;


MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->comboBoxResol->addItems(QStringList({"160x120","176x144","320x240","352x288","640x480","800x600","1024x768","1280x1024","1600x1200"}));
    ui->comboBoxBaud->addItems( QStringList({"9600","19200","38400","57600","256000","115200","921600"}) );
    ui->comboBoxPort->addItems( QStringList({"ttyACM0","ttyACM1","ttyACM2"}) );

    serialPort = new QSerialPort();

    loadLastConnection();

    flagConnected = false;
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
        ui->mainToolBar->actions().at(0)->setIcon(QIcon(":/new/imagenes/connect.png"));
        ui->mainToolBar->actions().at(0)->setToolTip("Connect");
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
            ui->mainToolBar->actions().at(0)->setIcon(QIcon(":/new/imagenes/disconnect.png"));
            ui->mainToolBar->actions().at(0)->setToolTip("Disconnect");

            //Save last connection
            updateLastConnection();

            //Set arduCAM resolution
            serialPort->write( QString::number(ui->comboBoxResol->currentIndex()).toStdString().c_str(), 1 );

            //Red messages from arduino if existes
            QString tmp;
            tmp = serialPortReadLine(20);
            if( !tmp.isEmpty() )qDebug() << tmp;
            tmp = serialPortReadLine(20);
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

        if( numReadTotal > 500 )//Heuristic
        {
            saveBinFile(_PATH_REC_IMG, receivedFileData, numReadTotal);

            processImage( _PATH_REC_IMG, _FLAG_PROC_COL_IDENTIFY );

            displayImage(_PATH_PROC_IMG);
        }
        qDebug() << "Received numReadTotal: " << numReadTotal;
    }
    else
        qDebug() << "SerialPort not connected";
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
    img.save(_PATH_PROC_IMG);
}

void MainWindow::displayImage(QString imgName )
{
    //Prepare image size
    QPixmap itemPix(imgName);
    itemPix = itemPix.scaled(_IMG_W-2, _IMG_H-2, Qt::IgnoreAspectRatio, Qt::FastTransformation);

    //Add image to scene
    QGraphicsScene* scene = new QGraphicsScene();
    QGraphicsPixmapItem* item = new QGraphicsPixmapItem(itemPix);
    scene->addItem(item);
    ui->graphicsView->setScene(scene);
    ui->graphicsView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    ui->graphicsView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
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

    displayImage( _PATH_PROC_IMG );
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
        int i;
        for(i=1; i<=_MOTOR_STEP_LEN; i++)
        {
            serialPort->write(msg.toStdString().c_str(),len);
            serialPort->waitForBytesWritten(10);
            QtDelay(50);
        }
    }
}
