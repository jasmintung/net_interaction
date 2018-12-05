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

/*��������״̬*/
typedef enum   tagITSNETSTATSTYPE_E
{
    ITSNETSTATSTYPE_INIT=0x0,           /*��ʼ��*/
    ITSNETSTATSTYPE_CONNECTED,          /*�ɹ���������*/
    ITSNETSTATSTYPE_REGISTERED,         /*�ɹ�ע�������*/
    ITSNETSTATSTYPE_SESSION,            /*����ͨ��*/
    ITSNETSTATSTYPE_UNKNOWN,            /*δ֪״̬*/
}ITSNETSTATSTYPE;
typedef struct _Circular_
{
	int length;
	int msgID;			/*��ϢID*/
	int msgNum;			/*��Ϣ��ˮ��*/
	char sendCount;		/*�������ʹ���*/
	char historyFlag;	/*�Ƿ���ʷ����*/
	char NetData[1024];
}Circular_t;
typedef struct _PlatFormRsp_
{
	int msgID;			/*��ϢID*/
	int msgNum;			/*��Ϣ��ˮ��*/
	char result;		/*BYDƽ̨Ӧ����:1��ʾ��ȷ,0��ʾ����ȷ*/
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
	int m_currentMsgNumber;		/*��ǰ������������*/
	ConnectCtrl *m_pConnectCtrl;
	vector<Circular_t> m_NetDatas;
	vector<Circular_t> m_Circular;
private:
	void *m_BYDCanDataShm_read;		/*BYD CAN���ݶ������ڴ���*/ 
	pthread_mutex_t m_stMagMutex;                       /*�߳���*/
	pthread_mutex_t m_stE6DataMutex;					/*�߳���*/
	bool exist;		/* �豸�Ƿ���� */
	int m_gItsNetStats;	/*��������״̬*/
	int m_SocketFd;	/*��������socket*/
	int m_MsgNum;	/*��Ϣ��ˮ��*/
	int m_MsgId;	/*��ϢID*/
	CanDatas_S m_CanDatas;
	CANSavedContainer m_CanSavedContainer;
	int m_CanId[100];
private:
	int MakeComplete905Packet(unsigned short cmdtype, int msgDataLen,  char *msgBuf, unsigned short msgNum);
	int SendTransferredMeaning(char *inBuf, int inLen, char *outBuf, int *outLen);
	ITSNETSTATSTYPE GetSystemStats(void);
	
	
};

#endif

