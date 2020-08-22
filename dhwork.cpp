#include "dhwork.h"
#include <QThread>
#include<QCoreApplication>

DHwork::DHwork(QObject *parent) : QObject(parent)
{

}

void DHwork::login(const CameraDeviceImf *info)
{
    CameraDeviceImf *inform = const_cast<CameraDeviceImf*>(info);
    QByteArray array = info->ip.toLatin1();
    char *cIp = array.data();
    int port = info->port;
    QByteArray array1 = info->accoumt.toLatin1();
    char *pUserAccount = array1.data();
    QByteArray array2 = info->passWord.toLatin1();
    char *pPassWord = array2.data();
    int err;

    //此函数是同步接口，会阻塞线程,成功返回非0，为登陆句柄。
    LLONG loginIndex = CLIENT_Login(cIp,port,pUserAccount,pPassWord,NULL,&err);
    if(loginIndex != 0)//成功
    {
        inform->luserId = loginIndex;
        CameraDeviceImf *imf = const_cast<CameraDeviceImf *>(info);
        LogEvent *event = new LogEvent(imf,Camero::OnLine);
        QCoreApplication::postEvent(reciveObject,event);
    }
    else {
        inform->luserId = -1;
        CameraDeviceImf *imf = const_cast<CameraDeviceImf *>(info);
        LogEvent *event = new LogEvent(imf,Camero::OffLine);
        QCoreApplication::postEvent(reciveObject,event);
    }

}
