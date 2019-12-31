#include "logevent.h"

QEvent::Type  LogEvent::type = (Type)QEvent::registerEventType(-1);

LogEvent::LogEvent(CameraDeviceImf *c, Camero::LineState s)
    :QEvent(type),
      camera(c),
      state(s)
{

}

QEvent::Type LogEvent::getEventType()
{
    return type;
}
