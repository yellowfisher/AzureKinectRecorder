#ifndef _GL_WIDGET_H
#define _GL_WIDGET_H

#include <GL/glew.h>

#include <QGLWidget>

#include "KinectThread.h"

enum ViewType
{
    DepthView,
    ColorView
};


class GLWidget : public QGLWidget
{
    Q_OBJECT

public:
    GLWidget(QWidget *parent, const int viewType);
    ~GLWidget();

	inline void setSize(const unsigned int &width, const unsigned int &height) { m_width = width; m_height = height; }
    void setData(const cv::Mat &);

protected:
    void initializeGL() Q_DECL_OVERRIDE;
    void paintGL() Q_DECL_OVERRIDE;
    void resizeGL(const int, const int) Q_DECL_OVERRIDE;

signals:
    void updateDepthFpsLabel();
    void updateColorFpsLabel();

private:
    int m_viewType;

    int m_width;
    int m_height;

    int m_format;

    GLuint m_texture;

    cv::Mat m_data;
};


#endif
