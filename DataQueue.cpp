#include "DataQueue.h"
#include <QDebug>

void DataQueue::updateDepthDisplayFrame(const cv::Mat &&newFrame)
{
    QMutexLocker locker(&m_depthDisplayMutex);

    if(m_depthDisplayFrames.size() < 5)
        m_depthDisplayFrames.push_back(newFrame);
    else
        m_depthDisplayFrames.pop_front();
}


void DataQueue::updateDepthFrame(const frame_t &&newFrame)
{
    QMutexLocker locker(&m_depthMutex);

    if(m_depthFrames.size() < 5)
        m_depthFrames.push_back(newFrame);
    else
        m_depthFrames.pop_front();
}


void DataQueue::updateColorDisplayFrame(const cv::Mat &&newFrame)
{
    QMutexLocker locker(&m_colorDisplayMutex);

    if(m_colorDisplayFrames.size() < 5)
        m_colorDisplayFrames.push_back(newFrame);
    else
        m_colorDisplayFrames.pop_front();
}


void DataQueue::updateColorFrame(const frame_t &&newFrame)
{
    QMutexLocker locker(&m_colorMutex);

    if(m_colorFrames.size() < 5)
        m_colorFrames.push_back(newFrame);
    else
        m_colorFrames.pop_front();
}


void DataQueue::updateBodyFrame(const body_frame_t &&newFrame)
{
    QMutexLocker locker(&m_bodyMutex);

    if(m_bodyFrames.size() < 5)
        m_bodyFrames.push_back(newFrame);
    else
        m_bodyFrames.pop_front();
}


cv::Mat DataQueue::getDepthDisplayFrame()
{
    QMutexLocker locker(&m_depthDisplayMutex);

    cv::Mat frame;
    if(!m_depthDisplayFrames.empty())
    {
        frame = m_depthDisplayFrames.front();
        m_depthDisplayFrames.pop_front();
    }
    return frame;
}


frame_t DataQueue::getDepthFrame()
{
    QMutexLocker locker(&m_depthMutex);

    frame_t frame = {"", cv::Mat()};
    if(!m_depthFrames.empty())
    {
        frame.time = m_depthFrames.front().time;
        frame.frame = m_depthFrames.front().frame;
        m_depthFrames.pop_front();
    }
    
    return frame;
}


cv::Mat DataQueue::getColorDisplayFrame()
{
    QMutexLocker locker(&m_colorDisplayMutex);

    cv::Mat frame;
    if(!m_colorDisplayFrames.empty())
    {
        frame = m_colorDisplayFrames.front();
        m_colorDisplayFrames.pop_front();
    }

    return frame;
}


frame_t DataQueue::getColorFrame()
{
    QMutexLocker locker(&m_colorMutex);

    frame_t frame = {"", cv::Mat()};
    if(!m_colorFrames.empty())
    {
        frame.time = m_colorFrames.front().time;
        frame.frame = m_colorFrames.front().frame;
        m_colorFrames.pop_front();
    }
    
    return frame;
}


body_frame_t DataQueue::getBodyFrame()
{
    QMutexLocker locker(&m_bodyMutex);

    body_frame_t frame = {"", ""};
    if(!m_bodyFrames.empty())
    {
        frame.time = m_bodyFrames.front().time;
        frame.jointData = m_bodyFrames.front().jointData;
        m_bodyFrames.pop_front();
    }
    return frame;
}


