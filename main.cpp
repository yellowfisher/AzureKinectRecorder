#include "MainWindow.h"
#include "KinectThread.h"
#include "WorkThread.h"

#include <QDebug>
#include <QDir>
#include <QFile>
#include <QDateTime>
#include <QMutex>

#define MAX_FILE_SIZE 1024*1024*5

void myMessageOutput(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    QByteArray localMsg = msg.toLocal8Bit();

    QString strDateTime = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss");

    QString strMsg("");
    switch (type)
    {
    case QtDebugMsg:
        strMsg = QString("%1 [Debug:] ").arg(strDateTime);
        break;
    case QtInfoMsg:
        strMsg = QString("%1 [Info:] ").arg(strDateTime);
        break;
    case QtWarningMsg:
        strMsg = QString("%1 [Warning:] ").arg(strDateTime);
        break;
    case QtCriticalMsg:
        strMsg = QString("%1 [Critical:] ").arg(strDateTime);
        break;
    case QtFatalMsg:
        strMsg = QString("%1 [Fatal:] ").arg(strDateTime);
        break;
    }
    strMsg += QString("%1 (%2:%3, %4)").arg(localMsg.constData()).
            arg(context.file).
            arg(context.line).
            arg(context.function);

    strDateTime.chop(9);

    QString strLogDir = "./logs";
    QDir dirLog(strLogDir);
    if(!dirLog.exists())
        dirLog.mkpath(".");

    QString strLogFile = strLogDir + QDir::separator() + strDateTime + ".log";

    static QMutex mutex;
    mutex.lock();

    QFile logFile(strLogFile);
    QFileInfo fileInfo(strLogFile);
    if(fileInfo.size() > MAX_FILE_SIZE)
        logFile.remove();
    logFile.open(QIODevice::WriteOnly | QIODevice::Append);

    QTextStream stream(&logFile);
    stream << strMsg << endl;

    mutex.unlock();
}


int main(int argc, char *argv[])
{
    //qInstallMessageHandler(myMessageOutput);

    QApplication app(argc, argv);

    MainWindow mainWindow;


    QObject::connect(&mainWindow, &MainWindow::startRecord, &KinectThread::getInstance(), &KinectThread::handleRecord);
    QObject::connect(&mainWindow, &MainWindow::stopRecord, &KinectThread::getInstance(), &KinectThread::handleStopRecord);
    QObject::connect(&KinectThread::getInstance(), &KinectThread::enableStopBtn, &mainWindow, &MainWindow::enableStopRecordBtn);
    QObject::connect(&KinectThread::getInstance(), &KinectThread::recordStopped, &mainWindow, &MainWindow::handleRecordStopped);

    mainWindow.show();

	_CrtDumpMemoryLeaks();
    return app.exec();
}
