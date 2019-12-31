#ifndef DHWORK_H
#define DHWORK_H

#include <QObject>
#include "cameradeviceimf.h"
#include "dhnetsdk.h"
#include "logevent.h"

class DHwork : public QObject
{
    Q_OBJECT
public:
    explicit DHwork(QObject *parent = nullptr);
    void setRecive(QObject *recive){reciveObject = recive;}
signals:
    void loginResult(const CameraDeviceImf& info,int state,int error);
public slots:

    void login(const CameraDeviceImf *info);
private:
    QObject *reciveObject;
};

#endif // DHWORK_H



