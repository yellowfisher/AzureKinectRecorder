#ifndef _KINECT_THREAD_H
#define _KINECT_THREAD_H

#include <QObject>
#include <QBasicTimer>
#include <QTimerEvent>
#include <QFile>

#include <opencv2/opencv.hpp>

#include <GL/glew.h>

#include <k4a/k4a.hpp>
#include <k4arecord/record.h>
#include <k4abt.h>


#include <fstream>

#define DEPTH_WIDTH 512
#define DEPTH_HEIGHT 512

#define COLOR_WIDTH 1280
#define COLOR_HEIGHT 720


#define VERIFY(result)                                                                                                 \
    if (result != K4A_RESULT_SUCCEEDED)                                                                                \
    {                                                                                                                  \
        printf("%s \n - (File: %s, Function: %s, Line: %d)\n", #result " failed", __FILE__, __FUNCTION__, __LINE__);   \
        exit(1);                                                                                                       \
    }


struct K4ADeviceConfiguration
{
    // Fields that convert to k4a_device_configuration_t
    //
    bool EnableColorCamera = true;
    bool EnableDepthCamera = true;
    k4a_image_format_t ColorFormat = K4A_IMAGE_FORMAT_COLOR_BGRA32;
    k4a_color_resolution_t ColorResolution = K4A_COLOR_RESOLUTION_720P;
    k4a_depth_mode_t DepthMode = K4A_DEPTH_MODE_NFOV_UNBINNED;
    k4a_fps_t Framerate = K4A_FRAMES_PER_SECOND_30;

    int32_t DepthDelayOffColorUsec = 0;
    k4a_wired_sync_mode_t WiredSyncMode = K4A_WIRED_SYNC_MODE_STANDALONE;
    uint32_t SubordinateDelayOffMasterUsec = 0;
    bool DisableStreamingIndicator = false;
    bool SynchronizedImagesOnly = true;

    // UI-only fields that do not map to k4a_device_configuration_t
    //
    bool EnableImu = false;
    bool EnableMicrophone = false;

    k4a_device_configuration_t ToK4ADeviceConfiguration() const;
};


class KinectThread : public QObject
{
    Q_OBJECT

public:
    static KinectThread& getInstance()
    {
        static KinectThread instance;
        return instance;
    }

    void destroyInstance();

    KinectThread(KinectThread const &) = delete;
    KinectThread &operator = (KinectThread const&) = delete;

    void updateDevicesVector(k4a_device_t &&);

    void startCamera(K4ADeviceConfiguration &);
    void stopCamera();
    void closeDevice();

private:
    KinectThread(){}
    void saveBodyFile();
	void getBodyData(k4a_capture_t);
    void getDepthData(k4a_capture_t);
    void getColorData(k4a_capture_t);
	void createTimeStampTexture();

signals:
    void enableStopBtn();
    void recordStopped();

public slots:
    void startWork();
    void handleRecord(const QString &);
    void handleStopRecord();

private slots:
    void timerEvent(QTimerEvent *);

private:
    void drawBody(const _k4abt_skeleton_t &, const char *);
    void drawSkeleton(const k4abt_joint_t &, const k4abt_joint_t &, const char *);

private:
    QBasicTimer m_timer;

	int m_colorWidth = COLOR_WIDTH;
	int m_colorHeight = COLOR_HEIGHT;

    QString m_strSaveDir;

    qint64 m_currentTime;

    std::vector<k4a_device_t > m_devices;
    k4a_device_configuration_t m_config;

    cv::Mat m_colorImg;
    cv::Mat m_depthImg;

    quint64 m_skeletonFrameIndex = 0;
    QFile m_skeletonFile;
    k4a_calibration_t m_calibration;
    k4abt_tracker_t m_bodyTracker;
    //k4abt_frame_t m_bodyFrame;
    std::vector<k4abt_body_t> m_bodies;

    bool m_record;

    std::vector<int> m_compressionParams;
    quint64 m_depthFrameIndex = 0;

    bool m_bSkeletonFileOpened = false;
    bool m_bColorTimeFileOpened = false;
    bool m_bDepthTimeFileOpened = false;

	QString m_strColorTimeStamp;
	QString m_strDepthTimeStamp;

	QFile m_colorTimeStampFile;
	QFile m_depthTimeStampFile;

	std::ofstream *m_depthDataFile;

    cv::VideoWriter m_colorVideoWriter;
};

#endif // !_KINECT_THREAD_H
