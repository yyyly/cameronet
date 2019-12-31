#ifndef CAMERADEVICEIMF_H
#define CAMERADEVICEIMF_H
#include "enums.h"
#include<QString>
#include<QMap>
#include<QList>
#include "HCNetSDK.h"
#include "dhnetsdk.h"

struct CameraDeviceImf
{
 CameraDeviceImf & operator = (const CameraDeviceImf &right)
    {
        mold = right.mold;
        name = right.name;
        ip = right.ip;
        port = right.port;
        accoumt = right.accoumt;
        passWord = right.passWord;
        type = right.type;
        channalAmount = right.channalAmount;
        channelNameMap = right.channelNameMap;
        luserId = right.luserId;
        playId = right.playId;
        LoginWarming = right.LoginWarming;
        playWarming = right.playWarming;
        return  *this;
    }

    friend bool operator ==(const CameraDeviceImf &,const CameraDeviceImf &);

    Camero::Mold mold;  //设备厂家
    QString name;  //设备名称
    QString ip;  //设备的IP地址
    unsigned short port;  //设备端口号
    QString accoumt;  //设备账户
    QString passWord;  //访问密码
    Camero::TYPE type;  //设备种类（路线机，摄像头）
    int channalAmount;  //（通道号）
    QMap<int,QString> channelNameMap;  //设备的通道名称
    LONG luserId;  //设备登陆后的句柄
    LLONG playId;  //播放句柄
    DWORD LoginWarming;  //登陆时错误码
    DWORD playWarming;  //播放时错误码
    Camero::LineState lineState;//在线
};
inline bool operator == (const CameraDeviceImf &imf1,const CameraDeviceImf &imf2)
{

 return imf1.name == imf2.name &&
        imf1.mold == imf2.mold &&
        imf1.ip == imf2.ip &&
        imf1.port == imf2.port&&
        imf1.type == imf2.type;
}

#endif // CAMERADEVICEIMF_H
