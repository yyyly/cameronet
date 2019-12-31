#ifndef LOGEVENT_H
#define LOGEVENT_H

#include <QObject>
#include <QEvent>
#include "enums.h"
#include "cameradeviceimf.h"

class LogEvent : public QEvent
{
public:
    LogEvent(CameraDeviceImf* c,Camero::LineState s);
    static QEvent::Type getEventType();
    CameraDeviceImf* getCamera(){return camera;}
    Camero::LineState getState(){return state;}
private:
    static QEvent::Type type;
    CameraDeviceImf* camera;
    Camero::LineState state;
};

#endif // LOGEVENT_H
