win32:TEMPLATE = vcapp
win32:RC_ICONS += KinectRecorder.ico
unix:TEMPLATE = app

CONFIG += qt debug_and_release thread
QT += gui core opengl

win32 {
        contains(QMAKE_HOST.arch, x86):{
                QMAKE_LFLAGS *= /MACHINE:X86
        }

        contains(QMAKE_HOST.arch, x86_64):{
                QMAKE_LFLAGS *= /MACHINE:X64
        }

        INCLUDEPATH += "."                                                         \
                       "C:/3rd/glew-2.1.0/include"                                     \
                       "C:/3rd/opencv/build/include"                                   \
                       "C:/Program Files/Azure Kinect SDK v1.3.0/sdk/include"          \
                       "C:/Program Files/Azure Kinect Body Tracking SDK/sdk/include"

        LIBS += -L"C:/Program Files/Azure Kinect SDK v1.3.0/sdk/windows-desktop/amd64/release/lib"  \
                -L"C:/3rd/glew-2.1.0/lib/Release/x64"                      \
                -L"C:/3rd/opencv/build/x64/vc14/lib"                       \
                -L"C:/Program Files/Azure Kinect Body Tracking SDK/sdk/windows-desktop/amd64/release/lib" \
                -lk4a -lk4arecord -lk4abt -lglew32 -lopengl32 -lopencv_world310
}

HEADERS += MainWindow.h GLWidget.h KinectThread.h DataQueue.h WorkThread.h

SOURCES += main.cpp MainWindow.cpp GLWidget.cpp KinectThread.cpp DataQueue.cpp WorkThread.cpp

TARGET = AzureKinectRecorder
DESTDIR = ./output
