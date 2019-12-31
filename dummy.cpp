#include "dummy.h"

Dummy::Dummy(QObject *parent) : QObject(parent)
{

}

void Dummy::emitLogin(const CameraDeviceImf &imf)
{
    emit dhLogin(imf);
}
