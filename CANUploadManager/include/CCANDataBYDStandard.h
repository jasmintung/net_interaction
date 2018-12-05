#ifndef __CCANDATASTANDARDSTANDARD_H__
#define __CCANDATASTANDARDSTANDARD_H__

#include "ConnectCtrl.h"
#include "SystemTime.h"
#include <iostream>
#include <vector>
#include <pthread.h>
#include <unistd.h>
#include "DbgPrint.h"

using namespace std;

/*网络连接状态*/
typedef enum   tagITSNETSTATSTYPE_E
{
    ITSNETSTATSTYPE_INIT=0x0,           /*初始化*/
    ITSNETSTATSTYPE_CONNECTED,          /*成功连接网络*/
    ITSNETSTATSTYPE_REGISTERED,         /*成功注册服务器*/
    ITSNETSTATSTYPE_SESSION,            /*正在通话*/
    ITSNETSTATSTYPE_UNKNOWN,            /*未知状态*/
}ITSNETSTATSTYPE;
typedef struct _Circular_
{
	int length;
	int msgID;			/*消息ID*/
	int msgNum;			/*消息流水号*/
	char sendCount;		/*网传发送次数*/
	char historyFlag;	/*是否历史数据*/
	char NetData[1024];
}Circular_t;
typedef struct _PlatFormRsp_
{
	int msgID;			/*消息ID*/
	int msgNum;			/*消息流水号*/
	char result;		/*BYD平台应答结果:1表示正确,0表示不正确*/
}PlatFormRsp;
typedef struct _canDatas_
{
	int canId;
	int length;
	char canDatas[32];
}CanDatas_S;
typedef struct _CTBoxSavedCommandContainer_
{
	int canNumbers;
	vector<CanDatas_S> ofsVector;
}CANSavedContainer;

class CCANDataBYDStandard
{
public:
	CCANDataBYDStandard();
	~CCANDataBYDStandard();
	int ProtocolAnalyseStart();
	int ProtocolAnalyseStop();
	int ProcessProtocol(const char *cmd, int len);
	int ProcessEvent(int event, const char *pMessage, int msgLen);
	bool isExist();
	int NetStart(void);
	int DataFromNet(char *buf,int *length);	
	int SetSystemStats(ITSNETSTATSTYPE type);
	int GetSocketFd(void);
	int SetSocketFd(int fd);
	int ProcessRecPlatFormDatas(char *buf,int length,PlatFormRsp *responseInfo);
	void AnalyseReceiveDatas(char *recData,int recLen);
	void AnalyseE6CanDatas();
	void enouthCircular(Circular_t circular);
	int TcpConnectProc(void);
	int E6CanDatasProc(void);
	void InitBYDCanDataShmBuf(void);
	void TransCanDatasToNet();
	void BYDCanDataShm_transform(const char *data,int len);
	void PackageCanDatas(int canId,char *canMsg,int *length,bool needPackage);
public:
	int m_currentMsgNumber;		/*当前缓存数据条数*/
	ConnectCtrl *m_pConnectCtrl;
	vector<Circular_t> m_NetDatas;
	vector<Circular_t> m_Circular;
private:
	void *m_BYDCanDataShm_read;		/*BYD CAN数据读共享内存句柄*/ 
	pthread_mutex_t m_stMagMutex;                       /*线程锁*/
	pthread_mutex_t m_stE6DataMutex;					/*线程锁*/
	bool exist;		/* 设备是否存在 */
	int m_gItsNetStats;	/*网络连接状态*/
	int m_SocketFd;	/*网络连接socket*/
	int m_MsgNum;	/*消息流水号*/
	int m_MsgId;	/*消息ID*/
	CanDatas_S m_CanDatas;
	CANSavedContainer m_CanSavedContainer;
	int m_CanId[100];
private:
	int MakeComplete905Packet(unsigned short cmdtype, int msgDataLen,  char *msgBuf, unsigned short msgNum);
	int SendTransferredMeaning(char *inBuf, int inLen, char *outBuf, int *outLen);
	ITSNETSTATSTYPE GetSystemStats(void);
	
	
};

#endif

