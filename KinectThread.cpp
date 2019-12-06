#include "KinectThread.h"
#include "DataQueue.h"

#include <stdio.h>

#include <QDir>
#include <QDebug>

#include <QTextStream>

#include <QtConcurrent/QtConcurrent>
#include <QFuture>

#include <QMessageBox>
#include<QPushButton>

#define _USE_MATH_DEFINES
#include <math.h>


#define COLOR_FRAME_SAVE_THREAD_NUM 2

#include <QAtomicInteger>
QAtomicInteger<int> atoColorFrameIndex;

using namespace std;


k4a_device_configuration_t K4ADeviceConfiguration::ToK4ADeviceConfiguration() const
{
    k4a_device_configuration_t deviceConfig;

    deviceConfig.color_format = ColorFormat;
    deviceConfig.color_resolution = EnableColorCamera ? ColorResolution : K4A_COLOR_RESOLUTION_OFF;
    deviceConfig.depth_mode = EnableDepthCamera ? DepthMode : K4A_DEPTH_MODE_OFF;
    deviceConfig.camera_fps = Framerate;

    deviceConfig.depth_delay_off_color_usec = DepthDelayOffColorUsec;
    deviceConfig.wired_sync_mode = WiredSyncMode;
    deviceConfig.subordinate_delay_off_master_usec = SubordinateDelayOffMasterUsec;

    deviceConfig.disable_streaming_indicator = DisableStreamingIndicator;
    deviceConfig.synchronized_images_only = SynchronizedImagesOnly;

    return deviceConfig;
}


void KinectThread::updateDevicesVector(k4a_device_t &&device)
{
    m_devices.push_back(device);
}


void KinectThread::startCamera(K4ADeviceConfiguration &config)
{
    m_config = config.ToK4ADeviceConfiguration();

	switch (m_config.color_resolution)
	{
	case K4A_COLOR_RESOLUTION_720P:
		m_colorWidth = 1280;
		m_colorHeight = 720;
		break;
	case K4A_COLOR_RESOLUTION_1080P:
		m_colorWidth = 1920;
		m_colorHeight = 1080;
		break;
	case K4A_COLOR_RESOLUTION_1440P:
		m_colorWidth = 2560;
		m_colorHeight = 1440;
		break;
	case K4A_COLOR_RESOLUTION_1536P:
		m_colorWidth = 2548;
		m_colorHeight = 1536;
		break;
	case K4A_COLOR_RESOLUTION_2160P:
		m_colorWidth = 3840;
		m_colorHeight = 2160;
		break;
	case K4A_COLOR_RESOLUTION_3072P:
		m_colorWidth = 4096;
		m_colorHeight = 3072;
		break;
	default:
		break;
	}

    if(K4A_RESULT_SUCCEEDED != k4a_device_start_cameras(m_devices.back(), &m_config))
        qDebug()<<"Failed to start device\n";

    //get calibration
    if(K4A_RESULT_SUCCEEDED != k4a_device_get_calibration(m_devices.back(), m_config.depth_mode, m_config.color_resolution, &m_calibration))
    {
        qDebug()<<"Get depth camera calibration failed!";
        return;
    }

    // Set Tracker Configuration
    k4abt_tracker_configuration_t tracker_config = K4ABT_TRACKER_CONFIG_DEFAULT;
    tracker_config.sensor_orientation = K4ABT_SENSOR_ORIENTATION_DEFAULT;
    tracker_config.cpu_only_mode      = false;

    // Create Tracker with Configuration
    m_bodyTracker = nullptr;
    k4abt_tracker_create(&m_calibration, tracker_config, &m_bodyTracker);
    if( !m_bodyTracker )
	{
        qDebug() << "Failed to create tracker!";
    }

    startWork();
}


void KinectThread::stopCamera()
{
	if (m_bodyTracker)
		k4abt_tracker_destroy(m_bodyTracker);

    k4a_device_t device = m_devices.back();
    if(device)
    {
        k4a_device_stop_cameras(device);
    }
}


void KinectThread::closeDevice()
{
    if(m_timer.isActive())
        m_timer.stop();

    if(m_record)
        handleStopRecord();

    if(!m_devices.empty())
    {
        k4abt_tracker_destroy(m_bodyTracker);

        k4a_device_t device = m_devices.back();
        if(device)
        {
            k4a_device_stop_cameras(device);
            k4a_device_close(device);
        }

        m_devices.pop_back();
    }
}


void KinectThread::startWork()
{
    if(!m_timer.isActive())
        m_timer.start(0, this);
}


void KinectThread::timerEvent(QTimerEvent *event)
{
    if(event->timerId() == m_timer.timerId())
    {
        qint64 currentTime = QDateTime::currentMSecsSinceEpoch();

        k4a_device_t device = m_devices.back();

		k4a_capture_t capture;
        k4a_wait_result_t get_capture_result = k4a_device_get_capture(device, &capture, K4A_WAIT_INFINITE);
		if (get_capture_result == K4A_WAIT_RESULT_SUCCEEDED)
		{
            getBodyData(capture);

			getDepthData(capture);

			getColorData(capture);

			k4a_capture_release(capture);
		}

        //qDebug()<<"time cost : "<<QDateTime::currentMSecsSinceEpoch() - currentTime;
        m_currentTime = currentTime;
    }
    else
        return;
}


void KinectThread::handleRecord(const QString &strSaveDir)
{
    m_strSaveDir = strSaveDir;

    m_skeletonFrameIndex = 0;
    QString strSkeletonOut = m_strSaveDir + "skeleton.txt";
    m_skeletonFile.setFileName(strSkeletonOut);
    m_bSkeletonFileOpened = m_skeletonFile.open(QIODevice::WriteOnly | QIODevice::Append);

    QString strColorDir = m_strSaveDir + "color";
    QDir dirSave(strColorDir);
    if(!dirSave.exists())
        dirSave.mkpath(".");
    QString strColorOut = strColorDir + QDir::separator() + "color.avi";
    m_colorVideoWriter.open(strColorOut.toStdString(), CV_FOURCC('M','J','P','G'), 30, cv::Size(m_colorWidth, m_colorHeight));

	QString strColorTimeStamp = strColorDir + QDir::separator() + "time_stamp.txt";
	m_colorTimeStampFile.setFileName(strColorTimeStamp);
	m_bColorTimeFileOpened = m_colorTimeStampFile.open(QIODevice::WriteOnly | QIODevice::Append);

    QString strDepthDir = m_strSaveDir + "depth";
    QDir dirDepth(strDepthDir);
    if(!dirDepth.exists())
        dirDepth.mkpath(".");
    QString strDepthVideoFile = strDepthDir + QDir::separator() + "depth.data";
    m_depthFrameIndex = 0;
    m_compressionParams.clear();
    m_compressionParams.push_back(cv::IMWRITE_PNG_COMPRESSION);
    m_compressionParams.push_back(0);
	
	QByteArray ba = strDepthVideoFile.toLocal8Bit();
	const char *c_str2 = ba.data();
    m_depthDataFile = new ofstream(c_str2, ios::binary | ios::out);

	QString strDepthTimeStampFile = strDepthDir + QDir::separator() + "time_stamp.txt";
	m_depthTimeStampFile.setFileName(strDepthTimeStampFile);
	m_bDepthTimeFileOpened = m_depthTimeStampFile.open(QIODevice::WriteOnly | QIODevice::Append);

    m_record = true;

    emit enableStopBtn();
}


void KinectThread::handleStopRecord()
{
    m_record = false;

    if(m_bSkeletonFileOpened)
    {
        m_skeletonFile.close();
        m_bSkeletonFileOpened = false;
    }

	m_colorVideoWriter.release();

	if(m_depthDataFile->is_open())
    {
		m_depthDataFile->close();
    }

	if (m_bColorTimeFileOpened)
    {
		m_colorTimeStampFile.close();
        m_bColorTimeFileOpened = false;
    }

	if (m_bDepthTimeFileOpened)
    {
		m_depthTimeStampFile.close();
        m_bDepthTimeFileOpened = false;
    }

    emit recordStopped();
}


void KinectThread::getBodyData(k4a_capture_t capture)
{
    qint64 current = QDateTime::currentMSecsSinceEpoch();

	k4abt_frame_t bodyFrame;
    k4abt_tracker_enqueue_capture(m_bodyTracker, capture, K4A_WAIT_INFINITE);
    k4abt_tracker_pop_result(m_bodyTracker, &bodyFrame, K4A_WAIT_INFINITE);

    qint64 interval = QDateTime::currentMSecsSinceEpoch() - current;

    // Get Bodies
    const size_t numBodies = static_cast<size_t>(k4abt_frame_get_num_bodies(bodyFrame));
    if(numBodies > 1)
    {
		QString strQss = QString("background-color:black; color:white");
		QMessageBox msgBox;
		msgBox.setStyleSheet(strQss);

		msgBox.setWindowFlags(Qt::WindowTitleHint | Qt::Dialog | Qt::WindowMaximizeButtonHint | Qt::CustomizeWindowHint);

		msgBox.setWindowTitle("Warning");

		msgBox.setText("Only support One body tracking, please check.");
		QAbstractButton *btnOk = msgBox.addButton(tr("Ok"), QMessageBox::AcceptRole);

		strQss = QString("background-color:rgb(100,149,237); color:white");
		btnOk->setStyleSheet(strQss);

		//msgBox.exec();
    }

    QString strContent = QString::number(m_skeletonFrameIndex) + ":{\n";

    for( size_t i = 0; i < numBodies; i++ )
    {
        k4abt_skeleton_t skeleton;
        k4abt_frame_get_body_skeleton(bodyFrame, i, &skeleton);

        //do record here
        if(m_record)
        {
            strContent += "body" + QString::number(i) + ":{\n";
            for(const k4abt_joint_t& joint : skeleton.joints)
            {
                strContent += "(" + QString::number(joint.position.xyz.x) + "," + \
                        QString::number(joint.position.xyz.y) + "," + \
                        QString::number(joint.position.xyz.z) + "," + \
                        QString::number(joint.confidence_level) + ")\n";
            }
            strContent += "}\n";
        }

        drawBody(skeleton, "color");
        drawBody(skeleton, "depth");
    }

    strContent += "}\n";

    if(m_bSkeletonFileOpened)
    {
        QTextStream tsBody(&m_skeletonFile);
        tsBody << strContent;
    }

    if(m_record || strContent.size() > 6)
        m_skeletonFrameIndex += 1;

	k4abt_frame_release(bodyFrame);
}


void KinectThread::getDepthData(k4a_capture_t capture)
{
    qint64 current = QDateTime::currentMSecsSinceEpoch();

    k4a_image_t depthImage = k4a_capture_get_depth_image(capture); // get image metadata
    if (depthImage != NULL)
    {
        // get raw buffer
        uint8_t* buffer = k4a_image_get_buffer(depthImage);
        size_t bufferSize = k4a_image_get_size(depthImage);

		if (buffer != NULL && bufferSize > 0)
		{
			//get time stamp in ms
			uint64_t timestamp_ns = k4a_image_get_device_timestamp_usec(depthImage) / 1000;

            // convert the raw buffer to cv::Mat
            int rows = k4a_image_get_height_pixels(depthImage);
            int cols = k4a_image_get_width_pixels(depthImage);
            cv::Mat depthMat(rows, cols, CV_16U, (void*)buffer, cv::Mat::AUTO_STEP);

            if (m_record && m_depthDataFile->is_open())
            {
				//m_depthDataFile->write((char *)buffer, sizeof(uint8_t) * bufferSize);
                QString strFilePath = m_strSaveDir + "depth" + QDir::separator() + \
					QString::number(m_depthFrameIndex) + ".png";
                QTextCodec *codec = QTextCodec::codecForName("gb2312");
                std::string fileName = codec->fromUnicode(strFilePath).data();
                cv::imwrite(fileName, depthMat, m_compressionParams);

                QTextStream ts(&m_colorTimeStampFile);
                ts << QString::number(timestamp_ns) << "\n";

                m_depthFrameIndex += 1;
            }

			cv::Mat mat8c1;
			depthMat.convertTo(mat8c1, CV_8UC1, 255.0 / 5000.0, 0);

            cv::applyColorMap(mat8c1, m_depthImg, cv::COLORMAP_JET);
            DataQueue::getInstance().updateDepthDisplayFrame(std::move(m_depthImg));
		}
        k4a_image_release(depthImage);
    }
}


void KinectThread::getColorData(k4a_capture_t capture)
{
    k4a_image_t colorImage = k4a_capture_get_color_image(capture); // get image metadata

	uint64_t timestamp_ns = k4a_image_get_device_timestamp_usec(colorImage) / 1000;

    if (colorImage != NULL)
    {
        // get raw buffer
        uint8_t* buffer = k4a_image_get_buffer(colorImage);

        // convert the raw buffer to cv::Mat
        int rows = k4a_image_get_height_pixels(colorImage);
        int cols = k4a_image_get_width_pixels(colorImage);
        m_colorImg = cv::Mat(rows, cols, CV_8UC4, (void*)buffer, cv::Mat::AUTO_STEP);

		cv::Mat img4Save;
		cvtColor(m_colorImg, img4Save, CV_BGRA2BGR);

        if(m_record)
        {
            m_colorVideoWriter.write(img4Save);

			QTextStream ts(&m_depthTimeStampFile);
			ts << QString::number(timestamp_ns) << "\n";
        }

		cvtColor(m_colorImg, m_colorImg, CV_BGRA2RGB);
		DataQueue::getInstance().updateColorDisplayFrame(std::move(m_colorImg));

        k4a_image_release(colorImage);
    }
}


void KinectThread::drawBody(const _k4abt_skeleton_t &skeleton, const char *imgType)
{
    if(strcmp("color", imgType) == 0)
    {
        if(m_colorImg.empty())
            return;
    }
    else
    {
        if(m_depthImg.empty())
            return;
    }

    drawSkeleton(skeleton.joints[K4ABT_JOINT_EAR_LEFT],       skeleton.joints[K4ABT_JOINT_EYE_LEFT],        imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_EAR_RIGHT],      skeleton.joints[K4ABT_JOINT_EYE_RIGHT],       imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_NOSE],           skeleton.joints[K4ABT_JOINT_EYE_LEFT],        imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_NOSE],           skeleton.joints[K4ABT_JOINT_EYE_RIGHT],       imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_NOSE],           skeleton.joints[K4ABT_JOINT_HEAD],            imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_HEAD],           skeleton.joints[K4ABT_JOINT_NECK],            imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_NECK],           skeleton.joints[K4ABT_JOINT_SPINE_CHEST],     imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_SPINE_CHEST],    skeleton.joints[K4ABT_JOINT_CLAVICLE_LEFT],   imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_CLAVICLE_LEFT],  skeleton.joints[K4ABT_JOINT_SHOULDER_LEFT],   imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_SHOULDER_LEFT],  skeleton.joints[K4ABT_JOINT_ELBOW_LEFT],      imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_ELBOW_LEFT],     skeleton.joints[K4ABT_JOINT_WRIST_LEFT],      imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_WRIST_LEFT],     skeleton.joints[K4ABT_JOINT_HAND_LEFT],       imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_HAND_LEFT],      skeleton.joints[K4ABT_JOINT_THUMB_LEFT],      imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_HAND_LEFT],      skeleton.joints[K4ABT_JOINT_HANDTIP_LEFT],    imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_SPINE_CHEST],    skeleton.joints[K4ABT_JOINT_CLAVICLE_RIGHT],  imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_CLAVICLE_RIGHT], skeleton.joints[K4ABT_JOINT_SHOULDER_RIGHT],  imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_SHOULDER_RIGHT], skeleton.joints[K4ABT_JOINT_ELBOW_RIGHT],     imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_ELBOW_RIGHT],    skeleton.joints[K4ABT_JOINT_WRIST_RIGHT],     imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_WRIST_RIGHT],    skeleton.joints[K4ABT_JOINT_HAND_RIGHT],      imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_HAND_RIGHT],     skeleton.joints[K4ABT_JOINT_THUMB_RIGHT],     imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_HAND_RIGHT],     skeleton.joints[K4ABT_JOINT_HANDTIP_RIGHT],   imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_SPINE_CHEST],    skeleton.joints[K4ABT_JOINT_SPINE_NAVAL],     imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_SPINE_NAVAL],    skeleton.joints[K4ABT_JOINT_PELVIS],          imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_PELVIS],         skeleton.joints[K4ABT_JOINT_HIP_LEFT],        imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_PELVIS],         skeleton.joints[K4ABT_JOINT_HIP_RIGHT],       imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_HIP_LEFT],       skeleton.joints[K4ABT_JOINT_KNEE_LEFT],       imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_KNEE_LEFT],      skeleton.joints[K4ABT_JOINT_ANKLE_LEFT],      imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_ANKLE_LEFT],     skeleton.joints[K4ABT_JOINT_FOOT_LEFT],       imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_HIP_RIGHT],      skeleton.joints[K4ABT_JOINT_KNEE_RIGHT],      imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_KNEE_RIGHT],     skeleton.joints[K4ABT_JOINT_ANKLE_RIGHT],     imgType);
    drawSkeleton(skeleton.joints[K4ABT_JOINT_ANKLE_RIGHT],    skeleton.joints[K4ABT_JOINT_FOOT_RIGHT],      imgType);
}


void KinectThread::drawSkeleton(const k4abt_joint_t &joint1, const k4abt_joint_t &joint2, const char *imgType)
{
    cv::Mat img;
    k4a_calibration_type_t targetCam;
    if(strcmp("color", imgType) == 0)
    {
        img = m_colorImg;
        targetCam = K4A_CALIBRATION_TYPE_COLOR;
    }
    else
    {
        img = m_depthImg;
        targetCam = K4A_CALIBRATION_TYPE_DEPTH;
    }

    cv::Point pt1, pt2;
    k4abt_joint_confidence_level_t confidenceLevelBegin, confidenceLevelEnd;
    confidenceLevelBegin = joint1.confidence_level;
    confidenceLevelEnd = joint2.confidence_level;

    int valid = 0;
    k4a_float2_t position;
    k4a_calibration_3d_to_2d(&m_calibration, &joint1.position, K4A_CALIBRATION_TYPE_DEPTH, targetCam, &position, &valid);
    if(valid)
    {
        pt1.x = int(position.xy.x + 0.5f);
        pt1.y = int(position.xy.y + 0.5f);
    }

    valid = 0;
    k4a_calibration_3d_to_2d(&m_calibration, &joint2.position, K4A_CALIBRATION_TYPE_DEPTH, targetCam, &position, &valid);
    if(valid)
    {
        pt2.x = int(position.xy.x + 0.5f);
        pt2.y = int(position.xy.y + 0.5f);
    }

	// If we can't find either of these joints, exit
    if ((confidenceLevelBegin == K4ABT_JOINT_CONFIDENCE_NONE) || \
            (confidenceLevelEnd == K4ABT_JOINT_CONFIDENCE_NONE))
		return;

	// Don't draw if both points are inferred
    if ((confidenceLevelBegin == K4ABT_JOINT_CONFIDENCE_LOW) && \
            (confidenceLevelEnd == K4ABT_JOINT_CONFIDENCE_LOW))
        return;

	int pointSize = 3;
	int thickness = 2;

	if (confidenceLevelBegin == K4ABT_JOINT_CONFIDENCE_HIGH)
        cv::circle(img, pt1, pointSize, cv::Scalar(0, 0, 255), -1);
	else
        cv::circle(img, pt1, pointSize, cv::Scalar(0, 255, 0), -1);

	if (confidenceLevelEnd == K4ABT_JOINT_CONFIDENCE_HIGH)
        cv::circle(img, pt2, pointSize, cv::Scalar(0, 0, 255), -1);
	else
        cv::circle(img, pt2, pointSize, cv::Scalar(0, 255, 0), -1);

	// We assume all drawn bones are inferred unless BOTH joints are tracked
    cv::line(img, pt1, pt2, cv::Scalar(255, 0, 255), thickness);
}
