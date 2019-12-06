#include "GLWidget.h"
#include "KinectThread.h"

#include <QDebug>


GLWidget::GLWidget(QWidget *parent, const int viewType)
    : QGLWidget(parent)
{
	m_viewType = viewType;
	if (m_viewType == 0)
	{
		m_width = 640;
		m_height = 576;
	}
	else
	{
		m_width = 1280;
		m_height = 720;
	}

	m_format = GL_RGB;

    this->resize(parent->size());
}


GLWidget::~GLWidget()
{
    m_data = NULL;
}


void GLWidget::setData(const cv::Mat &data)
{
    if (!data.empty())
    {
        m_data = std::move(data);
        updateGL();
    }
}


void GLWidget::initializeGL()
{
	GLenum result = glewInit();

//	char msgBuf[4096];
//	sprintf_s(msgBuf, "init glew result is %d\n", result);
//	OutputDebugStringA(msgBuf);

    glClearColor(0.5f, 0.5f, 0.5f, 1.0f);

    glEnable(GL_DEPTH_TEST);
    glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

    glClearDepth(2000.0);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnable(GL_TEXTURE_2D);

    glGenTextures(1, &m_texture);
    glBindTexture(GL_TEXTURE_2D, m_texture);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}


void GLWidget::resizeGL(int w, int h)
{
    glViewport(0, 0, w, h);

    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();

    glOrtho(0, m_width, m_height, 0.0f, 0.0f, 1.0f);

    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
}


void GLWidget::paintGL()
{
    if (!m_data.empty())
    {
        glClearColor(.5f, .5f, .5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        int alignment = (m_data.step & 3) ? 1 : 4;
        glPixelStorei(GL_UNPACK_ALIGNMENT, alignment);
        glPixelStorei(GL_UNPACK_ROW_LENGTH, m_data.step / m_data.elemSize());
        glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
        glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

        glTexImage2D(GL_TEXTURE_2D,
			0,
			m_format,
            m_data.cols,
            m_data.rows,
            0,
			m_format,
            GL_UNSIGNED_BYTE,
            m_data.data);

        glLoadIdentity();

        glEnable(GL_TEXTURE_2D);

		/*
		glBegin(GL_POLYGON);
		glTexCoord2f(0.0f, 0.0f); glVertex2f(0, 0);
		glTexCoord2f(0.0f, 1.0f); glVertex2f(0, m_height);
		glTexCoord2f(1.0f, 1.0f); glVertex2f(m_width, m_height);
		glTexCoord2f(1.0f, 0.0f); glVertex2f(m_width, 0);
		glEnd();
		*/
		// /*
        glBegin(GL_TRIANGLE_FAN);
        glColor4f(255.0f, 255.0f, 255.0f, 255.0f);
        glTexCoord2f(0, 0); glVertex3f(0, 0, 0);
        glTexCoord2f(1, 0); glVertex3f(m_width, 0, 0);
        glTexCoord2f(1, 1); glVertex3f(m_width, m_height, 0);
        glTexCoord2f(0, 1); glVertex3f(0, m_height, 0);
        glEnd();
		//*/
    }
}
