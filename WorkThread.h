#ifndef _WORK_THREAD_H
#define _WORK_THREAD_H

#include <QThread>
#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QTimer>
#include <QVariant>

class WorkThread : public QThread
{
	Q_OBJECT

public:
    ~WorkThread() override;
    void takeObject(QObject *);

    void run() override;

signals:
    void aboutToBlock();

private:
    QAtomicInt m_atomicDestructor;
};


#endif //_WORK_THREAD_H
