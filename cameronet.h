#ifndef CAMERONET_H
#define CAMERONET_H
#include<QObject>
#include<QWidget>
#include<QMultiMap>
#include<QThread>
#include<QMap>

#include "HCNetSDK.h"
#include "cameronet_global.h"
#include "enums.h"
#include "cameradeviceimf.h"
#include "screen.h"
#include "soundpushbutton.h"
#include "dhnetsdk.h"
#include "dhwork.h"
#include "dummy.h"


class CAMERONETSHARED_EXPORT CameroNet : public QObject
{

    Q_OBJECT
private:

    explicit CameroNet(QObject *p =nullptr);


public:
    static CameroNet* getInstance()
    {
        static CameroNet * instance;
        if(instance == nullptr)
        {
            instance = new CameroNet();
        }
        return instance;
    }

     ~CameroNet();
public slots:
    /*登陆函数*/
    void login(CameraDeviceImf &i);

    /*播放函数，默认采用主码流，TCP连接方式*/
    LLONG realPlay(CameraDeviceImf *info, LONG channel, Screen &screen);

    /*停止播放，函数的错误信息留给调用模块处理*/
    DWORD stopPlay(CameraDeviceImf &info, Screen *screen);

    DWORD stopPlay(LONG realHandle);

    /*登出，函数的错误信息留给调用模块处理*/
    DWORD loginOut(CameraDeviceImf *info);

    /*抓图函数*/
    DWORD capPic(CameraDeviceImf &info);

    /*声音控制函数*/
    DWORD changeSoundState(CameraDeviceImf& info,SoundCommond c);
    DWORD opendSound(CameraDeviceImf *info);
    DWORD closeSound(CameraDeviceImf *info);

    /*只录像，不播放*/
    LONG recordVedio(CameraDeviceImf *info,LONG channel,QString *fileName,Screen *screen = nullptr);

    /*云台控制*/
    bool PtzControl(CameraDeviceImf &info,Camero::PTZcommond c,int start,int step = 4,int channal = 1);//start = 1 开始，start = 0，结束

    //DH
private slots:
    void dhLoginResut(const CameraDeviceImf& info,int state,int err);

signals:
    void signalDeviceStatusChanged(CameraDeviceImf* camero,Camero::LineState state);
    void dhLogin(const CameraDeviceImf* imf);
protected:
    bool event(QEvent *event);
private:
    static QMap<LONG,QString> fileNameMap;
    //HK
    static void  CALLBACK hkLoginResutCallback(LONG lUserID,DWORD dwResult,
                                               LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo,void *pUser);
    static void CALLBACK g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType,
                                                BYTE *pBuffer,DWORD dwBufSize,void* dwUser);
    static void CALLBACK fExceptionCallBack(DWORD dwType,LONG lUserID, LONG lHandle,void *pUser);

    //DH
    static void CALLBACK autoConnectFunc(LONG lLoginID,char *pchDVRIP,LONG nDVRPort,DWORD dwUser);

    static void CALLBACK fDisConnect( LLONG  lLoginID,char  *pchDVRIP,LONG   nDVRPort,LDWORD dwUser);//设备断线回调函数

    static QList<CameraDeviceImf *> HKPinfoList;//存储登陆信息，析构函数使用
    static QMap<LLONG,CameraDeviceImf *>  pImfByUserIdMap;
    static QMultiMap<LONG,Screen *> pScreenByUserMap;

    //DH
    static QMap<LLONG,CameraDeviceImf *> dhImfByUserIdMap;

    static void CALLBACK fRealDataCallBack(  LLONG  lRealHandle,DWORD  dwDataType,  BYTE  *pBuffer,  DWORD  dwBufsize,  LDWORD dwUser);
};

#endif // CAMERONET_H
