#ifndef _MAIN_WINDOW_H
#define _MAIN_WINDOW_H

#include "GLWidget.h"

#include <QApplication>
#include <QWidget>
#include <QGridLayout>
#include <QPushButton>
#include <QLabel>
#include <QGroupBox>
#include <QCheckBox>
#include <QLineEdit>



///* tm_zip contain date/time info */
//typedef struct tm_zip_s
//{
//    uint tm_sec;            /* seconds after the minute - [0,59] */
//    uint tm_min;            /* minutes after the hour - [0,59] */
//    uint tm_hour;           /* hours since midnight - [0,23] */
//    uint tm_mday;           /* day of the month - [1,31] */
//    uint tm_mon;            /* months since January - [0,11] */
//    uint tm_year;           /* years - [1980..2044] */
//} tm_zip;


//typedef struct
//{
//    tm_zip      tmz_date;       /* date in understandable format           */
//    ulong       dosDate;       /* if dos_date == 0, tmu_date is used      */
///*    uLong       flag;        */   /* general purpose bit flag        2 bytes */

//    ulong       internal_fa;    /* internal file attributes        2 bytes */
//    ulong       external_fa;    /* external file attributes        4 bytes */
//} zip_fileinfo;


class MainWindow : public QWidget
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = 0);
	~MainWindow();

private:
    void initWindow();
	void initSettingsGroup();
    void initDepthCamSettings();
    void initColorCamSettings();
    void initFramerateSettings();
    void initSyncSettings();
	void saveInfoTxt(const QString &);
    void updateSettingsGroup();
    void initDetailSettings();
    void updateDeviceList();

signals:
    void startRecord(const QString &);
    void stopRecord();

public slots:
    void enableStopRecordBtn();
    void handleRecordStopped();

private slots:
    void paintEvent(QPaintEvent *) override;
    void handleChooseDir();
    void handleStartRecord();
    void handleStopRecord();
    void refreshDevices();
    void initializeDevice();
    void activeDeviceChanged(int);
    void handleStartCamera();
    void handleStopCamera();
    void handleCloseCamera();
	void depthModeChanged(int);
	void colorFormatChanged(int);

private:
    QHBoxLayout *m_mainLayout;

    GLWidget *m_depthWgt;
    GLWidget *m_colorWgt;

    QGroupBox *m_gbSettings;
    QVBoxLayout *m_vlaySettings;
    QGroupBox *m_gbOpenDevices;
    QGroupBox *m_gbEnableDepth;
    QGroupBox *m_gbEnableColor;
    QGroupBox *m_gbFramerate;
    QGroupBox *m_gbSync;

    QButtonGroup *m_bgDepthMode;
    QButtonGroup *m_bgColorFormat;
    QButtonGroup *m_bgRatio;
    QButtonGroup *m_bgFps;
    QButtonGroup *m_bgIO;
    QButtonGroup *m_bgMode;

	int m_installedDevices = 0;

    QCheckBox *m_cbLedControl;
    QCheckBox *m_cbSyncImg;

    QLineEdit *m_leDepthDelay;
    QLineEdit *m_leMasterDelay;

    QComboBox *m_cbSerialNums;
    QPushButton *m_pbOpenDevice;

    QLabel *m_lbDeviceNum;
    QPushButton *m_pbCloseDevice;

    QPushButton *m_pbStartCamera;
    QPushButton *m_pbStopCamera;

    k4a_device_t m_device;
    bool m_camerasStarted = false;
    bool m_imuStarted = false;

    int m_selectedDevice = -1;
    std::string m_deviceSerialNumber;

    int m_activeDevice;

    std::vector<std::pair<int, std::string>> m_connectedDevices;

    QPushButton *m_btnChooseDir;
    QPushButton *m_btnStartRecord;
    QPushButton *m_btnStopRecord;

    QString m_strSaveDir;
    QString m_strCurrentTime;
};

#endif //_MAIN_WINDOW_H
