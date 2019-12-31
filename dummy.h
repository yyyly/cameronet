#ifndef DUMMY_H
#define DUMMY_H

#include <QObject>
#include "cameradeviceimf.h"

class Dummy : public QObject
{
    Q_OBJECT
public:
    explicit Dummy(QObject *parent = nullptr);
    void emitLogin(const CameraDeviceImf& imf);
signals:
    void dhLogin(const CameraDeviceImf& imf);

public slots:
};

#endif // DUMMY_H
