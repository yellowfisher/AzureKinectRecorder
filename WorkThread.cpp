#include "WorkThread.h"

#include <QAbstractEventDispatcher>
#include <QCoreApplication>
#include <QTimer>
#include <QVariant>

WorkThread::~WorkThread()
{
    m_atomicDestructor.store(1);
    requestInterruption();
    quit();
    wait();
}

void WorkThread::run()
{
    connect(QAbstractEventDispatcher::instance(this),
            &QAbstractEventDispatcher::aboutToBlock,
            this,
            &WorkThread::aboutToBlock);

    QThread::run();
}


void WorkThread::takeObject(QObject *obj)
{
    static constexpr char kRegistered[] = "__ThreadRegistered";
    static constexpr char kMoved[] = "__Moved";

    if (!obj->property(kRegistered).isValid())
    {
        QObject::connect(this, &WorkThread::finished, obj, [this, obj]{
            if (!m_atomicDestructor.load() || obj->thread() != this)
                return;

            Q_ASSERT(obj->thread() == QThread::currentThread());

            obj->setProperty(kMoved, true);
            obj->moveToThread(this->thread());
        }, Qt::DirectConnection);

        QObject::connect(this, &QObject::destroyed, obj, [obj]{
            if (!obj->thread())
            {
                obj->moveToThread(QThread::currentThread());
                obj->setProperty(kRegistered, {});
            }
            else if (obj->thread() == QThread::currentThread() && obj->property(kMoved).isValid())
            {
                obj->setProperty(kMoved, {});
                QCoreApplication::sendPostedEvents(obj, QEvent::MetaCall);
            }
            else if (obj->thread()->eventDispatcher())
                QTimer::singleShot(0, obj, [obj]{obj->setProperty(kRegistered, {}); });
        }, Qt::DirectConnection);

        obj->setProperty(kRegistered, true);
    }
    obj->moveToThread(this);
}
