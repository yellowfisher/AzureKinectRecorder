#ifndef _DATAQUEUE_H
#define _DATAQUEUE_H

#include <QMutex>
#include <QMutexLocker>

#include <deque>

#include "KinectThread.h"

struct frame_t
{
    QString time;
    cv::Mat frame;
};


struct body_t
{
    uint trackState;
    float x;
    float y;
    float z;
};


struct body_frame_t
{
    QString time;
    QString jointData;
};


class DataQueue
{
public:
    static DataQueue& getInstance()
    {
        static DataQueue instance;
        return instance;
    }

    DataQueue(DataQueue const&) = delete;
    DataQueue &operator=(DataQueue const&) = delete;

    void updateDepthDisplayFrame(const cv::Mat &&);
    void updateDepthFrame(const frame_t &&);
    void updateColorDisplayFrame(const cv::Mat &&);
    void updateColorFrame(const frame_t &&);
    void updateBodyFrame(const body_frame_t &&);
//    void updateBodyFrame(const QString &&);

    cv::Mat getDepthDisplayFrame();
    frame_t getDepthFrame();
    cv::Mat getColorDisplayFrame();
    frame_t getColorFrame();
    body_frame_t getBodyFrame();

private:
    DataQueue(){}

private:
    QMutex m_depthDisplayMutex;
    QMutex m_depthMutex;
    QMutex m_colorDisplayMutex;
    QMutex m_colorMutex;
    QMutex m_bodyMutex;

    std::deque<cv::Mat> m_depthDisplayFrames;
    std::deque<frame_t> m_depthFrames;
    std::deque<cv::Mat> m_colorDisplayFrames;
    std::deque<frame_t> m_colorFrames;
    std::deque<body_frame_t> m_bodyFrames;
//    std::deque<QString> m_bodyFrames;
};

#endif //_DATAQUEUE_H
