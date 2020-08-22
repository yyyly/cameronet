#include<QCoreApplication>
#include<QByteArray>
#include<QDebug>
#include<string>
#include<QTextCodec>
#include<QDir>
#include<QDateTime>
#include<QFile>
#include<QList>
#include <QMetaType>

#include "cameronet.h"
#include "logevent.h"
#include "globaloption.h"

#include "log4qt/logger.h"
#include "log4qt/patternlayout.h"
#include "log4qt/dailyrollingfileappender.h"


QString G_PicSaveAdd = "";
QString G_VedioSaveAdd = "";
int G_Port = 0;
int G_RecordTime = 0;
int G_Stream = 0;

QMap<LLONG,QString> CameroNet::fileNameMap = {};
static QObject *gpRecive =nullptr;
QList<CameraDeviceImf *> CameroNet::HKPinfoList;
QMultiMap<LONG,Screen *> CameroNet::pScreenByUserMap = {};
QMap<LONG,CameraDeviceImf *> CameroNet::pImfByUserIdMap = {};

//DH
QMap<LLONG,CameraDeviceImf *> CameroNet::dhImfByUserIdMap = {};

CameroNet::CameroNet(QObject *p):
    QObject(p)
{
    gpRecive = this;
    qRegisterMetaType<CameraDeviceImf>("CameraDeviceImf");
    //HK
    if(!NET_DVR_Init())
    {
        DWORD err = GetLastError();
    }
    //注册事件回调函数
    if(!NET_DVR_SetExceptionCallBack_V30(WM_NULL,nullptr,fExceptionCallBack,nullptr));
    {
        DWORD err = GetLastError();

    }

    //DH
    if(CLIENT_Init(fDisConnect,0))
    {
        DWORD err = CLIENT_GetLastError();
    }
    //DH
    CLIENT_SetAutoReconnect(autoConnectFunc,0);
    DHwork *dhWork = new DHwork;
    dhWork->setRecive(this);
    QThread *dhThread = new QThread(this);
    connect(this,SIGNAL(dhLogin(const CameraDeviceImf *)),dhWork,SLOT(login(const CameraDeviceImf *)));
    //connect(dhWork,SIGNAL(loginResult(const CameraDeviceImf&,int,int)),this,SLOT(dhLoginResut(const CameraDeviceImf&,int,int)),Qt::QueuedConnection);
    dhWork->moveToThread(dhThread);
    dhThread->start();

    //日志系统配置
    Log4Qt::PatternLayout *p_layout = new Log4Qt::PatternLayout(this);
    p_layout->setConversionPattern("%d{yyyy-MM-dd HH:mm:ss} [%c] - %m%n");
    p_layout->activateOptions();
    const QString rFileName =QCoreApplication::applicationDirPath() + "/logs/CameroNetlog.log";
    const QString datePattern = "'.'yyyy-MM-dd";
    Log4Qt::DailyRollingFileAppender *appender = new Log4Qt::DailyRollingFileAppender(p_layout,rFileName,datePattern,this);
    appender->setAppendFile(true);
    appender->activateOptions();

    Log4Qt::Logger::rootLogger()->addAppender(appender);
}

CameroNet::~CameroNet()
{
    //停止播放打开的视频
    CameraDeviceImf *info;
    foreach (info, HKPinfoList) {
       loginOut(info);
    }
}

void CameroNet::   login(CameraDeviceImf &info)
{
    switch(info.mold){
    case Camero::HK:
    {
        NET_DVR_USER_LOGIN_INFO hk_info;
        memset(&hk_info,0,sizeof (hk_info));
        //获取访问IP
        QByteArray array = info.ip.toLatin1();
        char *str = array.data();
        strcpy(hk_info.sDeviceAddress,str);
        //获取端口
        hk_info.wPort = info.port;
        //获取访问账户
        array = info.accoumt.toLatin1();
        str = array.data();
        strcpy(hk_info.sUserName,str);
        //获取访问密码
        array = info.passWord.toLatin1();
        str = array.data();
        strcpy(hk_info.sPassword,str);
        hk_info.bUseAsynLogin = 1;//异步登陆
        hk_info.cbLoginResult = hkLoginResutCallback;//登陆结果通过回调函数返回
        hk_info.pUser = static_cast<void *>(&info);
        NET_DVR_DEVICEINFO_V40 dev_info;
        memset(&dev_info,0,sizeof(NET_DVR_DEVICEINFO_V40));
        info.luserId =NET_DVR_Login_V40(&hk_info,&dev_info);
    }
        break;
    case Camero::DH:
    {
        emit dhLogin(&info);
    }
        break;
    }
}

void CameroNet::dhLoginResut(const CameraDeviceImf &info, int state, int err)
{
    CameraDeviceImf *imf = const_cast<CameraDeviceImf *>(&info);
    if(state != 0)
    {

        imf->luserId = state;
        emit this->signalDeviceStatusChanged(imf,Camero::OnLine);
    }
    else {
        imf->luserId = -1;
        emit this->signalDeviceStatusChanged(imf,Camero::OffLine);
    }
}

void CameroNet::hkLoginResutCallback(LONG lUserID, DWORD dwResult,LPNET_DVR_DEVICEINFO_V30 lpDeviceInfo, void *pUser)
{
    CameraDeviceImf *info = static_cast<CameraDeviceImf *>(pUser);
    Camero::LineState state = Camero::OffLine;
    switch (dwResult) {
    case 0://异步登陆失败
    {
        DWORD err = GetLastError();
        info->luserId = -1;
        info->LoginWarming = err;
        state = Camero::OffLine;
    }
        break;
    case 1://异步登陆成功
    {
       state = Camero::OnLine;
       info->luserId = lUserID;
       HKPinfoList << info;
       pImfByUserIdMap[lUserID] = info;
       if(lpDeviceInfo->byStartDChan == 0)//表示是IPC或DVR
       {
           info->type = Camero::IPC;
       }
       else
       {
           info->type = Camero::NVR;
       }
    }
        break;
    }
    LogEvent *event = new LogEvent(info,state);
    QCoreApplication::postEvent(gpRecive,event);
}



void CameroNet::fExceptionCallBack(DWORD dwType, LONG lUserID, LONG lHandle, void *pUser)
{
    switch (dwType) {
    case EXCEPTION_EXCHANGE://注册的心跳包超时
    {
        LogEvent *event = new LogEvent(pImfByUserIdMap[lUserID],Camero::OffLine);
        QCoreApplication::postEvent(gpRecive,event);
    }
        break;
    case RESUME_EXCHANGE://心跳包恢复
    {
        LogEvent *event = new LogEvent(pImfByUserIdMap[lUserID],Camero::OnLine);
        QCoreApplication::postEvent(gpRecive,event);
    }
        break;
    case EXCEPTION_PREVIEW:
    {

    }
        break;
    case EXCEPTION_RECONNECT:
    {
        QList<Screen*> list = pScreenByUserMap.values(lUserID);
        for(int i = 0;i < list.size();i++)
        {
            list.at(i)->setToolTip("正在重连。。。");
        }
    }
        break;
    case PREVIEW_RECONNECTSUCCESS:
    {
        QList<Screen*> list = pScreenByUserMap.values(lUserID);
        for(int i = 0;i < list.size();i++)
        {
            //list.at(i)->setToolTip("重连成功！");
            list.at(i)->setToolTip("");
        }
    }
    default:
        break;
    }
}

bool CameroNet::event(QEvent *event)
{
    if(event->type() == LogEvent::getEventType())
    {
        LogEvent *logevent = static_cast<LogEvent *>(event);
        CameraDeviceImf *imf = logevent->getCamera();
        if(imf->mold == Camero::DH)
        {
            if(!dhImfByUserIdMap.contains(imf->luserId))
            {
                dhImfByUserIdMap.insert(imf->luserId,imf);
            }
        }
        emit this->signalDeviceStatusChanged(imf,logevent->getState());
        return true;
    }
    return false;
}


LLONG CameroNet::realPlay(CameraDeviceImf* info, LONG channel,Screen& screen)//在Screen前加const后的问题研究
{
    if(info->luserId == -1)
    {
        return -1;//可以弹出框提示设备不在线
    }
    LLONG playID = -1;
    if(info->mold == Camero::HK)
    {
        NET_DVR_PREVIEWINFO clientInfo;
        memset(&clientInfo,0,sizeof(clientInfo));
        LONG lUserId = info->luserId;
        clientInfo.lChannel = channel;
        clientInfo.hPlayWnd = (HWND)screen.getPlayWidget()->winId();
        playID = NET_DVR_RealPlay_V40(lUserId,&clientInfo,NULL,NULL);
        if(playID == -1)
        {
            info->playWarming = GetLastError();
            screen.setToolTip("播放错误");
            return  -1;
        }
    }
    else if (info->mold == Camero::DH) {
        LLONG lloginID = info->luserId;
        int ichannel = channel;
        HWND w = (HWND)screen.getPlayWidget()->winId();
       playID = CLIENT_RealPlay(lloginID,0,w);
       if(playID == 0)
       {
           info->playWarming = CLIENT_GetLastError();
           return  -1;
       }
       //CLIENT_SaveRealData(playID,"d:/22.mp4");//测试。。。。。。
    }

    info->playId = playID;
    screen.bindDevice(info,channel);
    pScreenByUserMap.insert(info->luserId,&screen);//显示由playId返回的错误
    screen.settoolBarVisible(true);
    screen.setTitle(info->name);
    screen.setPlayState(Screen::PLAY);
    connect(&screen,SIGNAL(cameraClose(CameraDeviceImf&,Screen*)),this,SLOT(stopPlay(CameraDeviceImf&,Screen*)));
    connect(&screen,SIGNAL(capPicture(CameraDeviceImf& )),this,SLOT(capPic(CameraDeviceImf& )));
    connect(&screen,SIGNAL(soundCommond(CameraDeviceImf&,SoundCommond)),
            this,SLOT(changeSoundState(CameraDeviceImf&,SoundCommond)));

    return  playID;
}

void CameroNet::g_RealDataCallBack_V30(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize, void *pUser)
{
    QString *fileName = static_cast<QString *>(pUser);
    if(fileName->isEmpty())
    {
        Log4Qt::Logger::logger("Cameronet")->debug("设置了一个不合法的文件路径");
        return;
    }
    QFile file(*fileName);
    file.open(QIODevice::ReadWrite | QIODevice::Append);

    switch (dwDataType)
        {
        case NET_DVR_STREAMDATA:   //码流数据
            if (dwBufSize > 0)
            {
                QDataStream out(&file);
                out.setVersion(QDataStream::Qt_5_8);
                for(int i = 0;i < dwBufSize;i++)
                {
                    out << *(pBuffer + i);
                }
                file.close();

            }
            break;
        default: //其他数据

            break;
        }
}

void CameroNet::fRealDataCallBack(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufsize, DWORD dwUser)
{
    if(lRealHandle <= 0)
        return;
    QString fileName = fileNameMap.value(lRealHandle);
    QFile file(fileName);
    file.open(QIODevice::ReadWrite | QIODevice::Append);
    if(dwDataType == 0)//原始数据
    {
        if(dwBufsize > 0)
        {
            QDataStream out(&file);
            out.setVersion(QDataStream::Qt_5_8);
            for(int i = 0;i < dwBufsize;i++)
            {
                out << *(pBuffer + i);
            }
            file.close();
        }
    }
}

LLONG CameroNet::recordVedio(CameraDeviceImf *info, LONG channel, QString *fileName, Screen *screen)
{
    Log4Qt::Logger::logger("CameroNet")->debug("准备录像");
    if(info->luserId == -1)
    {
        return -1;//可以弹出框提示设备不在线
    }
    if(info->mold == Camero::HK)
    {
        Log4Qt::Logger::logger("CameroNet")->debug("设备类型：HK");
        NET_DVR_PREVIEWINFO clientInfo;
        memset(&clientInfo,0,sizeof(clientInfo));
        LONG lUserId = info->luserId;
        clientInfo.lChannel = channel;
        clientInfo.hPlayWnd = (HWND)screen->getPlayWidget()->winId();

        LONG playID = NET_DVR_RealPlay_V40(lUserId,&clientInfo,g_RealDataCallBack_V30,fileName);
        Log4Qt::Logger::logger("Cametonet")->debug(QString("错误%1").arg(NET_DVR_GetLastError()));
        if(playID >= 0)
        {
            Log4Qt::Logger::logger("CameroNet")->debug(QString("设备打开成功，句柄为%1").arg(playID));
            info->playId = playID;
            screen->bindDevice(info,channel);
            screen->settoolBarVisible(true);
            screen->setTitle(info->name);
            connect(screen,SIGNAL(cameraClose(CameraDeviceImf&,Screen*)),this,SLOT(stopPlay(CameraDeviceImf&,Screen*)));
            connect(screen,SIGNAL(capPicture(CameraDeviceImf& )),this,SLOT(capPic(CameraDeviceImf& )));
            connect(screen,SIGNAL(soundCommond(CameraDeviceImf&,SoundCommond)),
                    this,SLOT(changeSoundState(CameraDeviceImf&,SoundCommond)));
            return playID;
        }
    }
    else if(info->mold == Camero::DH)
    {
        LLONG playID = realPlay(info,channel,*screen);
        if(playID != 0)
        {
            fileNameMap[playID] = *fileName;
            CLIENT_SetRealDataCallBack(playID,fRealDataCallBack,0);
        }
        return  playID;
    }
    else
    {
        return -8;
    }
}

DWORD CameroNet::stopPlay(CameraDeviceImf &info, Screen *screen)
{
    bool stop = false;
    if(info.mold == Camero::HK)
    {
        stop = NET_DVR_StopRealPlay(info.playId);//成功停止播放
    }
    else if (info.mold == Camero::DH) {
        LLONG playId = info.playId;
        stop = CLIENT_StopRealPlay(playId);
        //CLIENT_StopSaveRealData(playId);
    }
    if(stop)
    {
        info.playId = -1;
        screen->settoolBarVisible(false);
        screen->setToolTip("");
        screen->setPlayState(Screen::UNPLAY);
        screen->update();
        pScreenByUserMap.remove(info.luserId,screen);
        disconnect(screen,SIGNAL(cameraClose(CameraDeviceImf&,Screen*)),this,SLOT(stopPlay(CameraDeviceImf&,Screen*)));
        disconnect(screen,SIGNAL(capPicture(CameraDeviceImf& )),this,SLOT(capPic(CameraDeviceImf& )));
        disconnect(screen,SIGNAL(soundCommond(CameraDeviceImf&,SoundCommond)),
                this,SLOT(changeSoundState(CameraDeviceImf&,SoundCommond)));
        return 0;
    }
    else {
        if(info.mold == Camero::HK)
        {
            return GetLastError();
        }
        else {
            DWORD e = CLIENT_GetLastError();
            return  e;
        }
    }
}

DWORD CameroNet::stopPlay(LONG realHandle)//海康设备接口，需要加一个参数
{
    NET_DVR_StopRealPlay(realHandle);
}

DWORD CameroNet::loginOut(CameraDeviceImf *info)
{
    //登出前先检测是否已经停止播放
    if(info->mold == Camero::HK)
    {
        if(!(info->playId == -1))
        {
            if(stopPlay(*info,nullptr) == 0)
            {
                if(!NET_DVR_Logout(info->luserId))
                {
                    return GetLastError();
                }
            }
            else {
                return GetLastError();
            }
        }
    }
    else if (info->mold == Camero::DH){
        if(info->playId != -1)
        {
            if(stopPlay(*info,nullptr) ==0)
            {
                if(!CLIENT_Logout(info->luserId))
                {
                    return -1;
                }
            }
            else {
                //停止播放不成功，返回原因。
            }
        }

    }
}

DWORD CameroNet::capPic(CameraDeviceImf& info)
{
    if(info.playId == -1)
        return  -1;
    QString fileDir = G_PicSaveAdd + "/" + info.name;
    QDir *dir = new QDir();
    if(!dir->exists(fileDir))
    {
       dir->mkdir(fileDir);
    }
    QString file = QDateTime::currentDateTime().toString("yyyy-MM-dd_hhmmss.zzz") + ".jpg";
    QString fileName = fileDir + "/" + file;
    QTextCodec *code = QTextCodec::codecForName("GBK");
    QByteArray fileArray = code->fromUnicode(fileName);
    char *add = fileArray.data();
    NET_DVR_SetCapturePictureMode(JPEG_MODE);
    if(info.mold == Camero::HK)
    {
        if(!NET_DVR_CapturePicture(info.playId,add))
        {
            DWORD e = GetLastError();
            return  e;
        }
    }
    else if (info.mold == Camero::DH) {
        if(!CLIENT_CapturePictureEx(info.playId,add,NET_CAPTURE_FORMATS::NET_CAPTURE_JPEG))
        {
            DWORD e = CLIENT_GetLastError();
            return e;
        }
    }
}

DWORD CameroNet::changeSoundState(CameraDeviceImf &info, SoundCommond c)
{
    if(info.playId == -1)
        return -1;
    if(info.mold == Camero::HK)
    {
        if(c == SOUND_PLAY)
            opendSound(&info);
        else {
            closeSound(&info);
        }
    }
    else if (info.mold == Camero::DH) {
        if(c == SOUND_PLAY)
        {
            if(!CLIENT_OpenSound(info.playId))
            {
                return -1;
            }
        }
        else {
            if(!CLIENT_CloseSound())
            {
                return -1;
            }
        }
    }
    return 0;
}

DWORD CameroNet::opendSound(CameraDeviceImf *info)
{
    if(!(info->playId == -1))
    {
        if(!NET_DVR_OpenSound(info->playId))
        {
            return GetLastError();
        }
    }
}

DWORD CameroNet::closeSound(CameraDeviceImf *info)
{
    if(!(info->playId == -1))
    {
        if(!NET_DVR_CloseSound())
        {
            return GetLastError();
        }
    }
}

bool CameroNet::PtzControl(CameraDeviceImf &info, Camero::PTZcommond c, int start, int step, int channal)
{
    switch(info.mold) {
    case Camero::HK:
    {
        DWORD ptzCommand = 0;
        switch (c) {
            case Camero::UP:
        {
            ptzCommand = TILT_UP;
        }
            break;
        case Camero::DOWN:
        {
            ptzCommand = TILT_DOWN;
        }
            break;
        case Camero::RIGHT:
        {
            ptzCommand = PAN_RIGHT;
        }
            break;
        case Camero::LEFT:
        {
            ptzCommand = PAN_LEFT;
        }
            break;
        }
        NET_DVR_PTZControl(info.playId,ptzCommand,start);

    }
        break;
    case Camero::DH:
    {
        DWORD ptzCommand = 0;
        switch (c) {
            case Camero::UP:
        {
            ptzCommand = DH_PTZ_UP_CONTROL;
        }
            break;
        case Camero::DOWN:
        {
            ptzCommand = DH_PTZ_DOWN_CONTROL;
        }
            break;
        case Camero::RIGHT:
        {
            ptzCommand = DH_PTZ_RIGHT_CONTROL;
        }
            break;
        case Camero::LEFT:
        {
            ptzCommand = DH_PTZ_LEFT_CONTROL;
        }
            break;
        }
        if(info.type == Camero::IPC)
        {
            CLIENT_PTZControl(info.luserId,channal - 1,ptzCommand,step,start);
        }
        else
        {
            CLIENT_PTZControl(info.luserId,channal,ptzCommand,step,start);
        }
    }
        break;

    }
}

void  CameroNet::fDisConnect( LLONG  lLoginID,char  *pchDVRIP,LONG   nDVRPort,LDWORD dwUser)
{
    LogEvent *event = new LogEvent(dhImfByUserIdMap[lLoginID],Camero::OffLine);
    QCoreApplication::postEvent(gpRecive,event);
    QList<Screen*> list = pScreenByUserMap.values(lLoginID);
    for(int i = 0;i < list.size();i++)
    {
        list.at(i)->setToolTip("正在重连。。。");
    }
}

void CameroNet::autoConnectFunc(LONG lLoginID, char *pchDVRIP, LONG nDVRPort, DWORD dwUser)
{
    LogEvent *event = new LogEvent(dhImfByUserIdMap[lLoginID],Camero::OnLine);
    QCoreApplication::postEvent(gpRecive,event);
    QList<Screen*> list = pScreenByUserMap.values(lLoginID);
    for(int i = 0;i < list.size();i++)
    {
        list.at(i)->setToolTip("");
    }
}




