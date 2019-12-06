#include "MainWindow.h"
#include "KinectThread.h"
#include "DataQueue.h"

#include <QVBoxLayout>
#include <QDesktopWidget>
#include <QStyle>
#include <QTimer>
#include <QDebug>
#include <QDateTime>
#include <QFileDialog>
#include <QDateTime>
#include <QComboBox>
#include <QRadioButton>
#include <QButtonGroup>
#include <QGridLayout>
#include <QMessageBox>

#include <k4arecord/playback.hpp>



MainWindow::MainWindow(QWidget* parent)
    : QWidget(parent)
{
    initWindow();

    refreshDevices();

    this->setLayout(m_mainLayout);

    setMinimumSize(1564, 880);
	   
    QDesktopWidget * desktop = QApplication::desktop();
    setGeometry(QStyle::alignedRect(Qt::LeftToRight,
                                    Qt::AlignCenter,
                                    this->size(),
                                    desktop->availableGeometry()
                                    ));

    QString qss = QString("background-color: black");
    this->setStyleSheet(qss);

    QTimer *timer = new QTimer(this);
    connect(timer, SIGNAL(timeout()), this, SLOT(update()));
    timer->start();
}


MainWindow::~MainWindow()
{

}


void MainWindow::initWindow()
{
    m_mainLayout = new QHBoxLayout;

    initSettingsGroup();
    m_mainLayout->addWidget(m_gbSettings);

    QVBoxLayout *vlayGLWgts = new QVBoxLayout;

    QHBoxLayout *hlayGLWgts = new QHBoxLayout;
    m_depthWgt = new GLWidget(this, ViewType::DepthView);
    m_colorWgt = new GLWidget(this, ViewType::ColorView);
    hlayGLWgts->addWidget(m_depthWgt);
    hlayGLWgts->addWidget(m_colorWgt);
    vlayGLWgts->addLayout(hlayGLWgts);

    QString strQss = QString("background-color: green; color : white");
    m_btnChooseDir = new QPushButton("Save Dir");
    m_btnChooseDir->setStyleSheet(strQss);
    m_btnStartRecord = new QPushButton("Start Record");
    m_btnStartRecord->setStyleSheet(strQss);
    strQss = QString("background-color: red; color: white");
    m_btnStopRecord = new QPushButton("Stop Record");
    m_btnStopRecord->setStyleSheet(strQss);
    m_btnStartRecord->setEnabled(false);
    m_btnStopRecord->setEnabled(false);
    QHBoxLayout *btnHLay = new QHBoxLayout;
    btnHLay->addStretch(5);
    btnHLay->addWidget(m_btnChooseDir);
    btnHLay->addStretch(1);
    btnHLay->addWidget(m_btnStartRecord);
    btnHLay->addWidget(m_btnStopRecord);
    vlayGLWgts->addLayout(btnHLay);

    m_mainLayout->addLayout(vlayGLWgts);

    connect(m_btnChooseDir, SIGNAL(clicked()), this, SLOT(handleChooseDir()));
    connect(m_btnStartRecord, SIGNAL(clicked()), this, SLOT(handleStartRecord()));
    connect(m_btnStopRecord, SIGNAL(clicked()), this, SLOT(handleStopRecord()));
}


void MainWindow::updateDeviceList()
{
    if(m_selectedDevice >= 0)
    {
        m_cbSerialNums->clear();
        for(int i=0; i<m_connectedDevices.size(); i++)
            m_cbSerialNums->addItem(QString::fromStdString(m_connectedDevices[i].second));

        m_pbOpenDevice->setEnabled(true);
    }
}


void MainWindow::initSettingsGroup()
{
	QString strQss = QString("color:white");

    m_gbSettings = new QGroupBox(tr("Settings"));
	m_gbSettings->setStyleSheet(strQss);
    m_gbSettings->setCheckable(false);
    m_gbSettings->setMaximumWidth(380);

    m_vlaySettings = new QVBoxLayout;

    m_gbOpenDevices = new QGroupBox(tr("Open Device"));
    m_gbOpenDevices->setCheckable(false);

    QVBoxLayout *vlayOpenDevice = new QVBoxLayout;

    QHBoxLayout *hlaySerialNum = new QHBoxLayout;
    m_cbSerialNums = new QComboBox();

    QLabel *lbDescribe = new QLabel(tr("Device S/N"));
    hlaySerialNum->addWidget(m_cbSerialNums);
    hlaySerialNum->addWidget(lbDescribe);
    vlayOpenDevice->addLayout(hlaySerialNum);

    QHBoxLayout *hlayDeviceControl = new QHBoxLayout;
    QPushButton *pbRefreshDevice = new QPushButton("Refresh Devices");
	strQss = QString("background-color:rgb(100,149,237); color: white");
	pbRefreshDevice->setStyleSheet(strQss);
    m_pbOpenDevice = new QPushButton("Open Device");
    hlayDeviceControl->addWidget(pbRefreshDevice);
    hlayDeviceControl->addWidget(m_pbOpenDevice);
    vlayOpenDevice->addLayout(hlayDeviceControl);
	vlayOpenDevice->addStretch(1);

    QString qss = QString("background-color: green; color : white");
    m_pbOpenDevice->setStyleSheet(qss);

    m_gbOpenDevices->setLayout(vlayOpenDevice);

    m_vlaySettings->addWidget(m_gbOpenDevices);

    initDetailSettings();

    m_lbDeviceNum->hide();
    m_pbCloseDevice->hide();
    m_gbEnableDepth->hide();
    m_gbEnableColor->hide();
    m_gbFramerate->hide();
    m_gbSync->hide();
    m_pbStartCamera->hide();

    m_gbSettings->setLayout(m_vlaySettings);

    if(m_selectedDevice < 0)
    {
        m_cbSerialNums->addItem(tr("No Available devices"));
        m_pbOpenDevice->setEnabled(false);
    }
    else
    {
        updateDeviceList();
    }

    connect(m_cbSerialNums, SIGNAL(currentIndexChanged(int)), this, SLOT(activeDeviceChanged(int)));
    connect(pbRefreshDevice, SIGNAL(clicked()), this, SLOT(refreshDevices()));
    connect(m_pbOpenDevice, SIGNAL(clicked()), this, SLOT(initializeDevice()));
}


void MainWindow::updateSettingsGroup()
{
    m_gbOpenDevices->hide();

    m_lbDeviceNum->show();
    m_pbCloseDevice->show();
    m_gbEnableDepth->show();
    m_gbEnableColor->show();
    m_gbFramerate->show();
    m_gbSync->show();
    m_pbStartCamera->show();

}


void MainWindow::activeDeviceChanged(int activeDevice)
{
	if(m_installedDevices == m_connectedDevices.size())
		m_selectedDevice = activeDevice;
}


void MainWindow::initDetailSettings()
{
    QHBoxLayout *hlayDeviceInfo = new QHBoxLayout;
    //QString strCurrentSerialNum = QString::fromStdString(m_connectedDevices[m_selectedDevice].second);
    m_lbDeviceNum = new QLabel("Device S/N: ");
    hlayDeviceInfo->addWidget(m_lbDeviceNum);
    m_pbCloseDevice = new QPushButton("Close deivce");
    QString qss = QString("background-color: red");
    m_pbCloseDevice->setStyleSheet(qss);
    hlayDeviceInfo->addWidget(m_pbCloseDevice);
    m_vlaySettings->addLayout(hlayDeviceInfo);
    connect(m_pbCloseDevice, SIGNAL(clicked()), this, SLOT(handleCloseCamera()));

    initDepthCamSettings();
    initColorCamSettings();
    initFramerateSettings();
    initSyncSettings();

    m_vlaySettings->addWidget(m_gbEnableDepth);
    m_vlaySettings->addWidget(m_gbEnableColor);
    m_vlaySettings->addWidget(m_gbFramerate);
    m_vlaySettings->addWidget(m_gbSync);

    QHBoxLayout *hlayStartBtn = new QHBoxLayout;
    m_pbStartCamera = new QPushButton(tr("Start"));
    qss = QString("background-color: green");
    m_pbStartCamera->setStyleSheet(qss);
    m_pbStopCamera = new QPushButton(tr("Stop"));
    m_pbStopCamera->hide();
    qss = QString("background-color: red");
    m_pbStopCamera->setStyleSheet(qss);
    hlayStartBtn->addWidget(m_pbStartCamera);
    hlayStartBtn->addWidget(m_pbStopCamera);
    m_vlaySettings->addLayout(hlayStartBtn);

    connect(m_pbStartCamera, SIGNAL(clicked()), this, SLOT(handleStartCamera()));
    connect(m_pbStopCamera, SIGNAL(clicked()), this, SLOT(handleStopCamera()));
}

void MainWindow::depthModeChanged(int id)
{
	QRadioButton *rb30Fps = qobject_cast<QRadioButton *>(m_bgFps->button(0));
	if (id == 3)
	{
		if (rb30Fps->isChecked())
		{
			m_bgFps->button(1)->setChecked(true);
			rb30Fps->setEnabled(false);
		}
	}
	else
		m_bgFps->button(0)->setEnabled(true);
}

void MainWindow::initDepthCamSettings()
{
	QString strQss = QString("background-color:rgb(0,0,128)");

	m_gbEnableDepth = new QGroupBox(tr("Enable Depth Camera"));
    m_gbEnableDepth->setCheckable(true);
    m_gbEnableDepth->setChecked(true);

    QVBoxLayout *vlayDepthCamSettings = new QVBoxLayout;

    QLabel *lbDepthConfig = new QLabel(tr("Depth Configuration"));
    vlayDepthCamSettings->addWidget(lbDepthConfig);

    QLabel *lbDepthMode = new QLabel(tr("Depth Mode"));
    vlayDepthCamSettings->addWidget(lbDepthMode);

    QRadioButton *rbDepthN2X2Binned = new QRadioButton("NFOV Binned");
    QRadioButton *rbDepthNUnbinned  = new QRadioButton("NFOV Unbinned");
    QRadioButton *rbDepthW2X2Binned = new QRadioButton("WFOV Binned");
    QRadioButton *rbDepthWUnbinned = new QRadioButton("WFOV Unbinned");
    QRadioButton *rbDepthPassiveIR = new QRadioButton("Passive IR");
    m_bgDepthMode = new QButtonGroup;
    m_bgDepthMode->addButton(rbDepthN2X2Binned, 0);
    m_bgDepthMode->addButton(rbDepthNUnbinned, 1);
    m_bgDepthMode->addButton(rbDepthW2X2Binned, 2);
    m_bgDepthMode->addButton(rbDepthWUnbinned, 3);
    m_bgDepthMode->addButton(rbDepthPassiveIR, 4);
    rbDepthNUnbinned->setChecked(true);

    QGridLayout *grdDepthMode = new QGridLayout;
    grdDepthMode->addWidget(rbDepthN2X2Binned, 0, 0);
    grdDepthMode->addWidget(rbDepthNUnbinned,  0, 1);
    grdDepthMode->addWidget(rbDepthW2X2Binned, 1, 0);
    grdDepthMode->addWidget(rbDepthWUnbinned,  1, 1);
    grdDepthMode->addWidget(rbDepthPassiveIR,  2, 0);
    vlayDepthCamSettings->addLayout(grdDepthMode);

    m_gbEnableDepth->setLayout(vlayDepthCamSettings);

	connect(m_bgDepthMode, SIGNAL(buttonClicked(int)), this, SLOT(depthModeChanged(int)));
}


void MainWindow::colorFormatChanged(int id)
{
	if (id == 2 || id == 3)
	{
		m_bgRatio->button(0)->setChecked(true);
		m_bgRatio->button(1)->setEnabled(false);
		m_bgRatio->button(2)->setEnabled(false);
		m_bgRatio->button(3)->setEnabled(false);
		m_bgRatio->button(4)->setEnabled(false);
		m_bgRatio->button(5)->setEnabled(false);
	}
	else
	{
		m_bgRatio->button(1)->setEnabled(true);
		m_bgRatio->button(2)->setEnabled(true);
		m_bgRatio->button(3)->setEnabled(true);
		m_bgRatio->button(4)->setEnabled(true);
		m_bgRatio->button(5)->setEnabled(true);
	}
}


void MainWindow::initColorCamSettings()
{
    m_gbEnableColor = new QGroupBox(tr("Enable Color Camera"));
    m_gbEnableColor->setCheckable(true);
    m_gbEnableColor->setChecked(true);
    QVBoxLayout *vlayColorConfig = new QVBoxLayout;

    QGroupBox *gbColorConfig = new QGroupBox("Color Configuration");
    gbColorConfig->setCheckable(false);

    QVBoxLayout *vlayColorCamSettings = new QVBoxLayout;

    QGroupBox *gbFormat = new QGroupBox(tr("Format"));
    gbFormat->setCheckable(false);
    QHBoxLayout *hlayColorFormat = new QHBoxLayout;
    QRadioButton *rbBGRA = new QRadioButton("BGRA");
    QRadioButton *rbMJPG = new QRadioButton("MJPG");
    QRadioButton *rbNV12 = new QRadioButton("NV12");
    QRadioButton *rbYUV2 = new QRadioButton("YUV2");
    hlayColorFormat->addWidget(rbBGRA);
    hlayColorFormat->addWidget(rbMJPG);
    hlayColorFormat->addWidget(rbNV12);
    hlayColorFormat->addWidget(rbYUV2);
    m_bgColorFormat = new QButtonGroup;
    m_bgColorFormat->addButton(rbBGRA, 0);
    m_bgColorFormat->addButton(rbMJPG, 1);
    m_bgColorFormat->addButton(rbNV12, 2);
    m_bgColorFormat->addButton(rbYUV2, 3);
    gbFormat->setLayout(hlayColorFormat);
    rbBGRA->setChecked(true);

    vlayColorCamSettings->addWidget(gbFormat);

    QGroupBox *gbResolution = new QGroupBox(tr("Resolution"));
    gbResolution->setCheckable(0);
    QVBoxLayout *vlaySizeRatio = new QVBoxLayout;

    QGroupBox *gbRatio0 = new QGroupBox("16:9");
    QGridLayout *grdRatio0 = new QGridLayout;
    QRadioButton *rb720P = new QRadioButton("720P");
    QRadioButton *rb1080P = new QRadioButton("1080P");
    QRadioButton *rb1440P = new QRadioButton("1440P");
    QRadioButton *rb2160P = new QRadioButton("2160P");
    grdRatio0->addWidget(rb720P,  0, 0);
    grdRatio0->addWidget(rb1080P, 0, 1);
    grdRatio0->addWidget(rb1440P, 1, 0);
    grdRatio0->addWidget(rb2160P, 1, 1);
    m_bgRatio = new QButtonGroup;
    m_bgRatio->addButton(rb720P,  0);
    m_bgRatio->addButton(rb1080P, 1);
    m_bgRatio->addButton(rb1440P, 2);
    m_bgRatio->addButton(rb2160P, 3);
    gbRatio0->setLayout(grdRatio0);
    vlaySizeRatio->addWidget(gbRatio0);
    rb720P->setChecked(true);

    QGroupBox *gbRatio1 = new QGroupBox("4:3");
    gbRatio1->setCheckable(false);
    QHBoxLayout *hlayRatio1 = new QHBoxLayout;
    QRadioButton *rb1536p = new QRadioButton("1536P");
    QRadioButton *rb3072p = new QRadioButton("3072P");
    hlayRatio1->addWidget(rb1536p);
    hlayRatio1->addWidget(rb3072p);
    m_bgRatio->addButton(rb1536p, 4);
    m_bgRatio->addButton(rb3072p, 5);
    gbRatio1->setLayout(hlayRatio1);
    vlaySizeRatio->addWidget(gbRatio1);

    gbResolution->setLayout(vlaySizeRatio);

    vlayColorCamSettings->addWidget(gbResolution);

    gbColorConfig->setLayout(vlayColorCamSettings);
    vlayColorConfig->addWidget(gbColorConfig);
    m_gbEnableColor->setLayout(vlayColorConfig);
	
	connect(m_bgColorFormat, SIGNAL(buttonClicked(int)), this, SLOT(colorFormatChanged(int)));
}


void MainWindow::initFramerateSettings()
{
    m_gbFramerate = new QGroupBox("Framerate");

    QVBoxLayout *vlayFramerate = new QVBoxLayout;

    QHBoxLayout *hlayFramerate = new QHBoxLayout;
    QRadioButton *rb30 = new QRadioButton("30 FPS");
    QRadioButton *rb15 = new QRadioButton("15 FPS");
    QRadioButton *rb5 = new QRadioButton("5 FPS");
    hlayFramerate->addWidget(rb30);
    hlayFramerate->addWidget(rb15);
    hlayFramerate->addWidget(rb5);
    m_bgFps = new QButtonGroup;
    m_bgFps->addButton(rb30, 0);
    m_bgFps->addButton(rb15, 1);
    m_bgFps->addButton(rb5,  2);
    rb30->setChecked(true);

    vlayFramerate->addLayout(hlayFramerate);

    m_cbLedControl = new QCheckBox(tr("Disable streaming LED"));
    m_cbLedControl->setChecked(false);
    vlayFramerate->addWidget(m_cbLedControl);

    m_gbFramerate->setLayout(vlayFramerate);
}


void MainWindow::initSyncSettings()
{
	QString strQss = QString("background-color: rgb(100,149,237)");

    m_gbSync = new QGroupBox(tr("Sync"));
    m_gbSync->setCheckable(false);

    QVBoxLayout *vlaySync = new QVBoxLayout;

    QGroupBox *gbInSync = new QGroupBox(tr("Internal Sync"));
    gbInSync->setCheckable(false);
    QVBoxLayout *vlayInSync = new QVBoxLayout;
    m_cbSyncImg = new QCheckBox("Syncronized images only");
    m_cbSyncImg->setChecked(true);
    vlayInSync->addWidget(m_cbSyncImg);
    QHBoxLayout *hlayDepthDelay = new QHBoxLayout;
    m_leDepthDelay = new QLineEdit("0");
    QPushButton *pbReduce = new QPushButton(tr("Reduce"));
	pbReduce->setStyleSheet(strQss);
    QPushButton *pbIncrease = new QPushButton(tr("Increase"));
	pbIncrease->setStyleSheet(strQss);
    QLabel *lbDepthDelay = new QLabel("Depth delay(us)");
    hlayDepthDelay->addWidget(m_leDepthDelay);
    hlayDepthDelay->addWidget(pbReduce);
    hlayDepthDelay->addWidget(pbIncrease);
    hlayDepthDelay->addWidget(lbDepthDelay);
    vlayInSync->addLayout(hlayDepthDelay);
    gbInSync->setLayout(vlayInSync);
    vlaySync->addWidget(gbInSync);
    connect(pbReduce, &QPushButton::clicked, [this](){
        QString strText = m_leDepthDelay->text();
        QString strNewValue = QString::number(strText.toInt() - 1);
        m_leDepthDelay->setText(strNewValue);
    });
    connect(pbIncrease, &QPushButton::clicked, [this](){
        QString strText = m_leDepthDelay->text();
        QString strNewValue = QString::number(strText.toInt() + 1);
        m_leDepthDelay->setText(strNewValue);
    });

    QGroupBox *gbExSync = new QGroupBox(tr("External Sync"));
    QVBoxLayout *vlayEx = new QVBoxLayout;
    QGroupBox *gbSyncCableState = new QGroupBox(tr("Sync cable state"));
    QHBoxLayout *hlayIO = new QHBoxLayout;
    QRadioButton *rbIn = new QRadioButton("In");
    QRadioButton *rbOut = new QRadioButton("Out");
    hlayIO->addWidget(rbIn);
    hlayIO->addWidget(rbOut);
    m_bgIO = new QButtonGroup;
    m_bgIO->addButton(rbIn);
    m_bgIO->addButton(rbOut);
    QPushButton *pbRefresh = new QPushButton("Refresh");
    hlayIO->addWidget(pbRefresh);
    gbSyncCableState->setLayout(hlayIO);
    vlayEx->addWidget(gbSyncCableState);
    QHBoxLayout *hlayMode = new QHBoxLayout;
    QRadioButton *rbStandalone = new QRadioButton("Standalone");
    QRadioButton *rbMaster = new QRadioButton("Master");
    QRadioButton *rbSub = new QRadioButton("Sub");
    m_bgMode = new QButtonGroup;
    m_bgMode->addButton(rbStandalone, 0);
    m_bgMode->addButton(rbMaster,     1);
    m_bgMode->addButton(rbSub,        2);
    rbStandalone->setChecked(true);
    hlayMode->addWidget(rbStandalone);
    hlayMode->addWidget(rbMaster);
    hlayMode->addWidget(rbSub);
    vlayEx->addLayout(hlayMode);
    QHBoxLayout *hlayDelayOffMaster = new QHBoxLayout;
    m_leMasterDelay = new QLineEdit("0");
    QPushButton *pbExReduce = new QPushButton(tr("Reduce"));
	pbExReduce->setStyleSheet(strQss);
    QPushButton *pbExIncrease = new QPushButton(tr("Increase"));
	pbExIncrease->setStyleSheet(strQss);
    QLabel *lbDelayOffMatser = new QLabel("Delay off Master(us)");
    hlayDelayOffMaster->addWidget(m_leMasterDelay);
    hlayDelayOffMaster->addWidget(pbExReduce);
    hlayDelayOffMaster->addWidget(pbExIncrease);
    hlayDelayOffMaster->addWidget(lbDelayOffMatser);
    vlayEx->addLayout(hlayDelayOffMaster);
    gbExSync->setLayout(vlayEx);
    vlaySync->addWidget(gbExSync);
    connect(pbExReduce, &QPushButton::clicked, [this](){
        QString strText = m_leMasterDelay->text();
        QString strNewValue = QString::number(strText.toInt() - 1);
        m_leMasterDelay->setText(strNewValue);
    });
    connect(pbExIncrease, &QPushButton::clicked, [this](){
        QString strText = m_leMasterDelay->text();
        QString strNewValue = QString::number(strText.toInt() + 1);
        m_leMasterDelay->setText(strNewValue);
    });

    m_gbSync->setLayout(vlaySync);
}


void MainWindow::refreshDevices()
{
    m_selectedDevice = -1;

     m_installedDevices = k4a_device_get_installed_count();

    m_connectedDevices.clear();

    for (uint32_t i = 0; i < m_installedDevices; i++)
    {
        try
        {
            k4a::device device = k4a::device::open(i);
            m_connectedDevices.emplace_back(std::make_pair(i, device.get_serialnum()));
        }
        catch (const k4a::error &)
        {
            continue;
        }
    }

    if (!m_connectedDevices.empty())
    {
        m_selectedDevice = m_connectedDevices[0].first;
        updateDeviceList();
    }
}


//open k4a device and push it into control thread
void MainWindow::initializeDevice()
{
    if (m_selectedDevice < 0)
    {
        qDebug()<<"No device selected!";
        return;
    }

    k4a_device_t device;
	if (K4A_FAILED(k4a_device_open(static_cast<uint32_t>(m_selectedDevice), &device)))
	{
        QString strQss = QString("background-color:black; color:white");
        QMessageBox msgBox;
        msgBox.setStyleSheet(strQss);

        msgBox.setWindowFlags(Qt::WindowTitleHint | Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::CustomizeWindowHint);

        msgBox.setWindowTitle("Error");

        msgBox.setText("Failed to open device!");
        QAbstractButton *btnDismiss = msgBox.addButton(tr("dismiss"), QMessageBox::AcceptRole);

        strQss = QString("background-color:rgb(100,149,237); color:white");
        btnDismiss->setStyleSheet(strQss);

        msgBox.exec();

        return;
	}

    KinectThread::getInstance().updateDevicesVector(std::move(device));

    updateSettingsGroup();
}


void MainWindow::paintEvent(QPaintEvent *event)
{
    Q_UNUSED(event);

    m_depthWgt->setData(DataQueue::getInstance().getDepthDisplayFrame());
    m_colorWgt->setData(DataQueue::getInstance().getColorDisplayFrame());
}


void MainWindow::handleChooseDir()
{
    m_strSaveDir = QFileDialog::getExistingDirectory(this, tr("Open Directory"),
                                                    "./",
                                                    QFileDialog::ShowDirsOnly |
                                                    QFileDialog::DontResolveSymlinks);
	if (!m_strSaveDir.endsWith("/") && !m_strSaveDir.endsWith("\\"))
		m_strSaveDir += QDir::separator();

    if(!m_strSaveDir.isEmpty())
        m_btnStartRecord->setEnabled(true);
}


void MainWindow::saveInfoTxt(const QString &saveDir)
{
	QString strFileName = saveDir + QDir::separator() + "settings.txt";
	QFile fDetails(strFileName);
	fDetails.open(QIODevice::WriteOnly);
	QString strContent = QStringLiteral("动作：\n姓名：\n年龄：\n性别：\n身高：\n体重：\n拳龄：\n标准度评分：");
	QTextStream stream(&fDetails);
	stream << strContent;
	fDetails.close();
}


void MainWindow::handleStartRecord()
{
    m_strCurrentTime = QDateTime::currentDateTime().toString("yyyyMMddhhmmss");
    
	QString strSaveDir = m_strSaveDir + m_strCurrentTime;
	
	QDir dirSave(strSaveDir);
	if(!dirSave.exists())
		dirSave.mkpath(".");

	saveInfoTxt(strSaveDir);

	if (!strSaveDir.endsWith("/") && !strSaveDir.endsWith("\\"))
		strSaveDir += QDir::separator();

    KinectThread::getInstance().handleRecord(strSaveDir);
//    emit startRecord(strSaveDir);

    m_btnStartRecord->setEnabled(false);
}


void MainWindow::handleStopRecord()
{
    emit stopRecord();
}


void MainWindow::handleRecordStopped()
{

    m_btnStopRecord->setEnabled(false);
    m_btnStartRecord->setEnabled(true);

    m_btnChooseDir->setEnabled(true);
}


void MainWindow::enableStopRecordBtn()
{
    m_btnStopRecord->setEnabled(true);
    m_btnChooseDir->setEnabled(false);
}


void MainWindow::handleStartCamera()
{
    K4ADeviceConfiguration config;
    if(!m_gbEnableDepth->isChecked())
        config.EnableDepthCamera = false;
    if(!m_gbEnableColor->isChecked())
        config.EnableColorCamera = false;

	unsigned int width = 0;
	unsigned int height = 0;

    switch(m_bgDepthMode->checkedId())
    {
    case 0:
        config.DepthMode = K4A_DEPTH_MODE_NFOV_2X2BINNED;  /**< Depth captured at 320x288. Passive IR is also captured at 320x288. */
		width = 320; height = 288;
        break;
    case 1:
        config.DepthMode = K4A_DEPTH_MODE_NFOV_UNBINNED;  /**< Depth captured at 640x576. Passive IR is also captured at 640x576. */
		width = 640; height = 576;
		break;
    case 2:
        config.DepthMode = K4A_DEPTH_MODE_WFOV_2X2BINNED;  /**< Depth captured at 512x512. Passive IR is also captured at 512x512. */
		width = 512; height = 512;
		break;
    case 3:
        config.DepthMode = K4A_DEPTH_MODE_WFOV_UNBINNED;  /**< Depth captured at 1024x1024. Passive IR is also captured at 1024x1024. */
		width = 1024; height = 1024;
		break;
    case 4:
        config.DepthMode = K4A_DEPTH_MODE_PASSIVE_IR;  /**< Passive IR only, captured at 1024x1024. */
		width = 1024; height = 1024;
		break;
    default:
        break;
    }
	m_depthWgt->setSize(width, height);

    switch(m_bgColorFormat->checkedId())
    {
    case 0:
        config.ColorFormat = K4A_IMAGE_FORMAT_COLOR_BGRA32;
        break;
    case 1:
        config.ColorFormat = K4A_IMAGE_FORMAT_COLOR_MJPG;
        break;
    case 2:
        config.ColorFormat = K4A_IMAGE_FORMAT_COLOR_NV12;
        break;
    case 3:
        config.ColorFormat = K4A_IMAGE_FORMAT_COLOR_YUY2;
        break;
    default:
        break;
    }
    switch(m_bgRatio->checkedId())
    {
    case 0:
        config.ColorResolution = K4A_COLOR_RESOLUTION_720P;  /**< 1280 * 720  16:9 */
		width = 1280; height = 720;
        break;
    case 1:
        config.ColorResolution = K4A_COLOR_RESOLUTION_1080P; /**< 1920 * 1080 16:9 */
		width = 1920; height = 1080;
		break;
    case 2:
        config.ColorResolution = K4A_COLOR_RESOLUTION_1440P; /**< 2560 * 1440 16:9 */
		width = 2560; height = 1440;
		break;
    case 3:
        config.ColorResolution = K4A_COLOR_RESOLUTION_2160P; /**< 2048 * 1536 4:3  */
		width = 2048; height = 1536;
		break;
    case 4:
        config.ColorResolution = K4A_COLOR_RESOLUTION_1536P; /**< 3840 * 2160 16:9 */
		width = 3840; height = 2160;
		break;
    case 5:
        config.ColorResolution = K4A_COLOR_RESOLUTION_3072P; /**< 4096 * 3072 4:3  */
		width = 4096; height = 3072;
		break;
    default:
        break;
    }
	m_colorWgt->setSize(width, height);

    switch(m_bgFps->checkedId())
    {
    case 0:
        config.Framerate = K4A_FRAMES_PER_SECOND_30;
        break;
    case 1:
        config.Framerate = K4A_FRAMES_PER_SECOND_15;
        break;
    case 2:
        config.Framerate = K4A_FRAMES_PER_SECOND_5;
        break;
    default:
        break;
    }

    switch(m_bgMode->checkedId())
    {
    case 0:
        config.WiredSyncMode = K4A_WIRED_SYNC_MODE_STANDALONE;
        break;
    case 1:
        config.WiredSyncMode = K4A_WIRED_SYNC_MODE_MASTER;
        break;
    case 2:
        config.WiredSyncMode = K4A_WIRED_SYNC_MODE_SUBORDINATE;
        break;
    default:
        break;
    }

    config.DisableStreamingIndicator = m_cbLedControl->isChecked() ? true : false;
    config.SynchronizedImagesOnly = m_cbSyncImg->isChecked()?true:false;
    config.DepthDelayOffColorUsec = m_leDepthDelay->text().toInt();
    config.SubordinateDelayOffMasterUsec = m_leMasterDelay->text().toInt();

    KinectThread::getInstance().startCamera(config);

    m_pbStartCamera->hide();
    m_pbStopCamera->show();
    m_gbEnableColor->setEnabled(false);
    m_gbEnableDepth->setEnabled(false);
    m_gbFramerate->setEnabled(false);
    m_gbSync->setEnabled(false);
}


void MainWindow::handleStopCamera()
{
    KinectThread::getInstance().stopCamera();

    m_pbStartCamera->show();
    m_pbStopCamera->hide();
    m_gbEnableColor->setEnabled(true);
    m_gbEnableDepth->setEnabled(true);
    m_gbFramerate->setEnabled(true);
    m_gbSync->setEnabled(true);
}


void MainWindow::handleCloseCamera()
{
    KinectThread::getInstance().closeDevice();

    m_lbDeviceNum->hide();
    m_pbCloseDevice->hide();
    m_gbEnableDepth->hide();
    m_gbEnableColor->hide();
    m_gbFramerate->hide();
    m_gbSync->hide();
    m_pbStartCamera->hide();

    m_gbOpenDevices->show();
}
