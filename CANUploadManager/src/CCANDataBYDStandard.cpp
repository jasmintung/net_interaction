#include "CCANDataBYDStandard.h"
#include "json.h"
#include "Message.h"
#include <sys/time.h>
#include "SystemConfig.h"
#include "StreamBuffer.h"

using namespace std;
using namespace Common;
using namespace SysConfig;

#define NET_CONNECT_TEST
const char IPADD[] = "121.15.172.104\n";
//const char IPADD[] = "192.168.50.119\n";
//const short PORT = 5566;
const short PORT = 8225;
const int CONSEC = 3;
const int CONUSEC = 0;
const int DATA_MAXLEN = (4*1024);	/*数据长度*/
const int MAX_MSG_NUMBER = 1024;		/*最大缓存补传数据条数*/
const int CAN_DATA_TYPE = 0x0d01;

static void *TcpConnectThread(void *args)
{
	if(NULL != args)
	{
		CCANDataBYDStandard *pCanData = (CCANDataBYDStandard *)args;
		char readBuf[1024] = {0};
		int length = 0;
		int ret = -1;
		printf("TcpConnectProc[%d]pid:%d\n",__LINE__, getpid());
		while(1)
		{
			ret = pCanData->NetStart();
			if(ret < 0)
			{
				
			}
			else
			{
				length = 0;
				pCanData->DataFromNet(readBuf,&length);
				if(length < 0)
				{
					pCanData->SetSystemStats(ITSNETSTATSTYPE_INIT);
					pCanData->SetSocketFd(-1);
				}
				else
				{
					if(length > 0)
					{
						//printf("recv data:");
						//for(int i = 0;i < length;i++)
						//{
						//	printf("%02x ",readBuf[i]);
						//}
						//pCanData->AnalyseReceiveDatas(readBuf,length);
					}
					pCanData->AnalyseReceiveDatas(readBuf,length);
					
				}
			}
			
			usleep(200*1000);
		}
	}
	return NULL;
}
static void *E6CanDatasProcThread(void *args)
{
	if(NULL != args)
	{
		CCANDataBYDStandard *pCanData = (CCANDataBYDStandard *)args;
		while(1)
		{	
			
			pCanData->AnalyseE6CanDatas();
			usleep(50*1000);
		}
	}
	return NULL;
}

CCANDataBYDStandard::CCANDataBYDStandard()
{
	pthread_mutex_init(&m_stMagMutex,NULL);
	pthread_mutex_init(&m_stE6DataMutex,NULL);
	m_pConnectCtrl = new ConnectCtrl();
	SetSystemStats(ITSNETSTATSTYPE_INIT);
	m_BYDCanDataShm_read = NULL;
	m_CanSavedContainer.canNumbers = 0;
	memset(&m_CanId,0,100);
	m_CanSavedContainer.ofsVector = vector<CanDatas_S>();
	exist = false;
	m_NetDatas = vector<Circular_t>();
	m_Circular = vector<Circular_t>();
	m_currentMsgNumber = 0;
	m_MsgNum = 0;
	InitBYDCanDataShmBuf();
}

CCANDataBYDStandard::~CCANDataBYDStandard()
{
}

int CCANDataBYDStandard::TcpConnectProc(void)
{
	printf("[%s][%d]\n",__FUNCTION__,__LINE__);
	pthread_t TcpConnect_th;
	pthread_attr_t pthread_attr;
	pthread_attr_init( &pthread_attr );
	pthread_attr_setscope( &pthread_attr, PTHREAD_SCOPE_SYSTEM );
	pthread_attr_setdetachstate( &pthread_attr, PTHREAD_CREATE_DETACHED );
 	if ( pthread_create( &TcpConnect_th, &pthread_attr, TcpConnectThread, this) != 0 )
	{			   
		pthread_attr_destroy( &pthread_attr );
		printf("[%s][%d]\n",__FUNCTION__,__LINE__);
		return -1;
 	}	
	pthread_attr_destroy( &pthread_attr );

	return 0;
}
int CCANDataBYDStandard::E6CanDatasProc(void)
{
	printf("[%s][%d]\n",__FUNCTION__,__LINE__);
	pthread_t TcpConnect_th;
	pthread_attr_t pthread_attr;
	pthread_attr_init( &pthread_attr );
	pthread_attr_setscope( &pthread_attr, PTHREAD_SCOPE_SYSTEM );
	pthread_attr_setdetachstate( &pthread_attr, PTHREAD_CREATE_DETACHED );
 	if ( pthread_create( &TcpConnect_th, &pthread_attr, E6CanDatasProcThread, this) != 0 )
	{			   
		pthread_attr_destroy( &pthread_attr );
		printf("[%s][%d]\n",__FUNCTION__,__LINE__);
		return -1;
 	}	
	pthread_attr_destroy( &pthread_attr );

	return 0;
}
int CCANDataBYDStandard::NetStart(void)
{

	int socket = 0;
	if(ITSNETSTATSTYPE_INIT == GetSystemStats())
	{
		socket = m_pConnectCtrl->Tcp_Connect(IPADD,PORT,CONSEC,CONUSEC);
		if(socket <= 0)
		{
			return -1;
		}
		SetSocketFd(socket);
		SetSystemStats(ITSNETSTATSTYPE_CONNECTED);
	}
	
	return 0;	
}
int CCANDataBYDStandard::DataFromNet(char *buf,int *length)
{
	if(GetSystemStats() == ITSNETSTATSTYPE_CONNECTED)
	{
		*length = m_pConnectCtrl->Tcp_ReadData(GetSocketFd(),3,0,DATA_MAXLEN,DATA_MAXLEN,buf);
	
	}
	return 0;
}

/*设置网络连接状态*/
int CCANDataBYDStandard::SetSystemStats(ITSNETSTATSTYPE type)
{
	m_gItsNetStats = type;
	return 0;
}
int CCANDataBYDStandard::SetSocketFd(int fd)
{
	m_SocketFd = fd;
	return 0;
}
/*获取网络连接状态*/
ITSNETSTATSTYPE CCANDataBYDStandard::GetSystemStats(void)
{
	int type = 0;
	type = m_gItsNetStats;
	return (ITSNETSTATSTYPE)type;
}
int CCANDataBYDStandard::GetSocketFd(void)
{
	return m_SocketFd;
}
int CCANDataBYDStandard::ProcessEvent(int event, const char *pMessage, int msgLen)
{
	return 0;
}
void CCANDataBYDStandard::InitBYDCanDataShmBuf(void)
{
	int				retValue = 0;
	char			buf_name[32] = {0};	
	char 			tmp_name[128] = {0};
	unsigned int	buf_size = 0;
	unsigned int	buf_cnt = 0;

	memset(buf_name, 0, sizeof(buf_name));
	buf_size = 0;
	buf_cnt = 0;

	retValue = N9M_GetStreamBufferName(BUFFER_TYPE_CAN, 0,buf_name,buf_size, buf_cnt);
	printf("222InitBYDCanDataShmBuf retValue === %d \n",retValue);
	if(retValue == 0)
	{

		memset(tmp_name, 0, sizeof(tmp_name));
		sprintf(tmp_name, "BYD_CAN_DATA_SHMRead_%d", getpid());	
		m_BYDCanDataShm_read = N9M_SHCreateShareBuffer(tmp_name,buf_name,buf_size,buf_cnt,SHAREMEM_READ_MODE);
	}
	
}
void CCANDataBYDStandard::TransCanDatasToNet()
{
	char *pReadBuf = NULL;
	int readLen = -1;
	
	while(m_BYDCanDataShm_read)
	{
		readLen = N9M_SHGetOneFrame(m_BYDCanDataShm_read,&pReadBuf,NULL);
		
		
		if((pReadBuf == NULL)||(readLen <= 0))
		{
			usleep(50*1000);
		}
		else
		{
			if(readLen > 0)
			{
				
				BYDCanDataShm_transform(pReadBuf,readLen);
			}
			
		}
		
		
		
	}
	

	
}
void CCANDataBYDStandard::BYDCanDataShm_transform(const char *data,int len)
{
	
	//获取设备系统时间
	datetime_t curTime;
	N9M_TMGetShareTime(&curTime);
	char tmpBuf[1024];
	int canId = 0;
	canId |= data[0];
	canId |= data[1]<<8;
	canId |= data[2]<<16;
	canId |= data[3]<<24;
	static int strIndex = 0;
	static int curSec = 0;
	static char flag = 0;
	int length = 0;
	int canIdNumbers[100] = {0};
	int index = 0;
	bool isExist = false;
	if(curTime.second % 15 == 0)
	{
		
		flag = 1;
		curSec = curTime.second;
		CanDatas_S s_CanDatas;
		memset(&s_CanDatas,0,sizeof(CanDatas_S));
		s_CanDatas.canId = canId;
		s_CanDatas.length = len;
		printf("readLen == %d\n",len);
		for(int i = 0;i < len;i++)
		{
			printf("%02x ",data[i]);
		}
		memcpy(s_CanDatas.canDatas,data,len);
		m_CanSavedContainer.canNumbers += 1;
		for(int i = 0;i < m_CanSavedContainer.canNumbers;i++)
		{
			length = len;
			printf("222 s_CanDatas.canId = %d m_CanId[%d] = %d\n",s_CanDatas.canId, i,m_CanId[i]);
			if(s_CanDatas.canId != m_CanId[i])
			{//槽里没有这个ID
				
				isExist = false;
				index += 1;
				if(((index == m_CanSavedContainer.canNumbers)&&(!isExist))||(m_CanSavedContainer.canNumbers == 0))
				{
					printf("333 canID %d m_CanId[%d] %d\n",s_CanDatas.canId, i,m_CanId[i]);
					m_CanId[i] = s_CanDatas.canId;
					
					printf("加入新的CAN ID数据 %d \n",s_CanDatas.canId);
					m_CanSavedContainer.ofsVector.push_back(s_CanDatas);
				}
				
			}
			else
			{//覆盖已经存在的CAN ID的数据
				if(m_CanSavedContainer.canNumbers > 0)
				{
					m_CanSavedContainer.canNumbers -= 1;
				
				}
				isExist = true;
				printf("刷新CAN ID 数据 %d \n",s_CanDatas.canId);
				PackageCanDatas(canId,s_CanDatas.canDatas,&s_CanDatas.length,false);
			}
		}
		strIndex += len;
	}
	else
	{
		memset(&m_CanId,0,100);
		if(flag == 1)
		{
			if(strIndex > 1024)
			{
				strIndex = 1024;
			}
			if(curTime.second > curSec)
			{
				curSec = 60;
				m_CanSavedContainer.canNumbers = 0;
				printf("最后发送调用 \n");
				PackageCanDatas(0,NULL,NULL,true);
				flag = 0;
			}
		}
	}
	
}
void CCANDataBYDStandard::PackageCanDatas(int canId,char *canMsg,int *length,bool needPackage)
{
	char tmpBuf[1024] = {0};
	std::vector<CanDatas_S>::iterator it;
	int strIndex = 0;
	for(it = m_CanSavedContainer.ofsVector.begin();it!= m_CanSavedContainer.ofsVector.end();)
	{
		if(needPackage)
		{
			printf("111 PackageCanDatas \n");
			memcpy(&tmpBuf[strIndex],(*it).canDatas,(*it).length);
			strIndex += (*it).length;
			m_CanSavedContainer.ofsVector.erase(it);
		}
		else
		{
			if(canId == (*it).canId)
			{
				printf("222 PackageCanDatas \n");
				memset((*it).canDatas,0,32);
				memcpy((*it).canDatas,canMsg,*length);
			}
			it++;
		}
		
	}
	if(needPackage)
	{
		printf("CAN ID DATA SEND:\n");
		for(int i = 0;i < strIndex;i++)
		{
			printf("%02x ",tmpBuf[i]);
		}
		unsigned short cmdType = 0x0D01;
		unsigned static int  msgNumber = 0;
		msgNumber += 1;
		Circular_t circular;
		memset(&circular,0,sizeof(Circular_t));
		circular.length = strIndex;
		memcpy(&circular.NetData,tmpBuf,strIndex);
		circular.msgNum = msgNumber;
		circular.msgID = cmdType;
		pthread_mutex_lock(&m_stE6DataMutex);
		m_Circular.push_back(circular);
		pthread_mutex_unlock(&m_stE6DataMutex);
		
	}
}

/*CAN数据组包成905协议数据包透传到BYD平台2016.10.12 by tong cheung*/
int CCANDataBYDStandard::MakeComplete905Packet(unsigned short cmdtype, int msgDataLen,  char *msgBuf, unsigned short msgNum)
{

	char *tmpBuf = msgBuf;
	char completePacket[1024] = {0};
	char blockPacket[1024] = {0};
	char tmpPacket[1024] = {0};
	int completeIndex = 0;
	int blockIndex = 0;
	int size = 0;
	char xorCheck = 0;
	const int msgLen = 26;		//增加vin和时间戳
	datetime_t curTime;
	char *carVin = "12345678912345678";
	//获取设备系统时间
	N9M_TMGetShareTime(&curTime);
	completePacket[completeIndex++] = 0x7e;
	blockPacket[blockIndex++] = (cmdtype>>8)&0xFF;	//消息ID
	blockPacket[blockIndex++] = cmdtype&0xFF;
	blockPacket[blockIndex++] = ((msgLen + msgDataLen)>>8)&0xFF;	//消息包大小
	blockPacket[blockIndex++] = (msgLen + msgDataLen)&0x00FF;
	blockPacket[blockIndex++] = 0x10;
	blockPacket[blockIndex++] = 0x74;			//自定义厂商编号
	blockPacket[blockIndex++] = 0x41;			//自定义设备类型为CAN
	blockPacket[blockIndex++] = 0x31;			//自定义序列号
	blockPacket[blockIndex++] = 0x32;
	blockPacket[blockIndex++] = 0x33;
	blockPacket[blockIndex++] = (msgNum>>8)&0xFF;	//消息体流水号"1"开始累加
	blockPacket[blockIndex++] = msgNum&0xFF;
	if(GetSystemStats() != ITSNETSTATSTYPE_CONNECTED)
	{//写入补传缓存
		printf("connect not pass,data into buffer!\n");
		blockPacket[blockIndex++] = 0x01;
	}
	else
	{
		blockPacket[blockIndex++] = 0x00;
	}
	for(int i = 0;i < 17;i++)
	{
		blockPacket[blockIndex++] = carVin[i];
	}
	blockPacket[blockIndex++] = curTime.year;
	blockPacket[blockIndex++] = curTime.month;
	blockPacket[blockIndex++] = curTime.day;
	blockPacket[blockIndex++] = curTime.hour;
	blockPacket[blockIndex++] = curTime.minute;
	blockPacket[blockIndex++] = curTime.second;
	blockPacket[blockIndex++] = 0;
	blockPacket[blockIndex++] = 0;
	for(int i = 0;i < msgDataLen;i++)
	{	
		
		blockPacket[blockIndex++] = tmpBuf[i];
	}
	for(int j = 0;j < blockIndex;j++)
	{
		xorCheck ^= blockPacket[j];
	}
	blockPacket[blockIndex++] = xorCheck;	//校验
	SendTransferredMeaning(blockPacket,blockIndex,tmpPacket,&size);		//数据转换7e-->7d 02
	for(int k = 0;k < size;k++)
	{
		completePacket[completeIndex++] = tmpPacket[k];
	}
	completePacket[completeIndex++] = 0x7e;
	printf("MakeComplete905Packet \n");
	for(int i = 0; i < completeIndex;i++)
	{
		printf("%02x ",completePacket[i]);
	}
	//printf("================================================\n");
	Circular_t circular;
	memset(&circular,0,sizeof(Circular_t));
	circular.length = completeIndex;
	memcpy(&circular.NetData,&completePacket,completeIndex);
	
	circular.msgNum = msgNum;
	circular.msgID = cmdtype;
	m_currentMsgNumber += 1;
	if(m_currentMsgNumber > 1000)
	{
		printf("buffer full !!!!!!!\n");
		return 0;
	}
	
	if(GetSystemStats() != ITSNETSTATSTYPE_CONNECTED)
	{//写入补传缓存
		printf("connect not pass\n");
		circular.historyFlag = 1;
		enouthCircular(circular);
	}
	else
	{
		printf("send directly !\n");
		circular.historyFlag = 0;
		enouthCircular(circular);
		m_pConnectCtrl->Tcp_Write(GetSocketFd(),CONSEC,CONUSEC,DATA_MAXLEN,completeIndex,completePacket,BlockType_NonBlock);
	}
	return 0;
}
void CCANDataBYDStandard::enouthCircular(Circular_t circular)
{
	pthread_mutex_lock(&m_stMagMutex);
	m_NetDatas.push_back(circular);
	pthread_mutex_unlock(&m_stMagMutex);
}
void CCANDataBYDStandard::AnalyseE6CanDatas()
{
	unsigned short cmdType = 0;
	int msgDataLen = 0;
	char msgBuf[1024] = {0};
	unsigned short msgNum = 0;
	pthread_mutex_lock(&m_stE6DataMutex);
	//vector<Circular> it;
	//it = m_Circular.front();
	if(!m_Circular.empty())
	{
		cmdType = m_Circular.front().msgID;
		msgDataLen = m_Circular.front().length;
		memcpy(msgBuf,m_Circular.front().NetData,m_Circular.front().length);
		msgNum = m_Circular.front().msgNum;
		
		m_Circular.erase(m_Circular.begin());
	}
	//for (it = m_Circular.begin(); it != m_Circular.end();)
	//{
		
		
	//	break;
	//}
	pthread_mutex_unlock(&m_stE6DataMutex);
	if(msgNum > 0)
	{
		MakeComplete905Packet(cmdType,msgDataLen,msgBuf,msgNum);
	}
	
}
void CCANDataBYDStandard::AnalyseReceiveDatas(char *recData,int recLen)
{
	if(recLen > 127)
	{ 
		return;
	}
	char msgBuf[128] = {0};
	int recMsgSerialNum = 0;
	int recMsgId;
	PlatFormRsp platFormRsp;
	memset(&platFormRsp,0,sizeof(PlatFormRsp));
	memcpy(msgBuf,recData,recLen);
	ProcessRecPlatFormDatas(msgBuf,recLen,&platFormRsp);
	printf("response serial number : %04x \n",platFormRsp.msgNum);
	printf("response result : %d \n",platFormRsp.result);
	vector<Circular_t>::iterator it;	
	pthread_mutex_lock(&m_stMagMutex);
	for (it = m_NetDatas.begin(); it != m_NetDatas.end();)
	{
		
		for(int i = 0;i < (*it).length;i++)
		{
			printf("%02x ",(*it).NetData[i]);
		}
		printf("\n");
		if((*it).historyFlag == 0)
		{
			
		}
		else
		{
			m_pConnectCtrl->Tcp_Write(GetSocketFd(),CONSEC,CONUSEC,DATA_MAXLEN,(*it).length,(*it).NetData,BlockType_NonBlock);
	
		}
		printf("(*it).msgNum = %04x \n",(*it).msgNum);
		if(((*it).msgID == platFormRsp.msgID)&&((*it).msgNum == platFormRsp.msgNum))
		{
			printf("response correct!!!\n");
			if(m_currentMsgNumber > 0)
			{
				m_currentMsgNumber -= 1;
			}
			
			m_NetDatas.erase(it);
		}
		else
		{
			
			(*it).historyFlag = 1;
			(*it).NetData[13] = 1;			/*修改第13位数据类型标志为补传*/
			it++;
		}
		
	}
	pthread_mutex_unlock(&m_stMagMutex);
}
int CCANDataBYDStandard::ProcessRecPlatFormDatas(char *buf,int length,PlatFormRsp *responseInfo)
{
	int index = 0;
	char *tmpBuf = buf;

	for(int i = 0;i < 11;i++)
	{
		(*tmpBuf++);
	}
	responseInfo->msgNum |= ((*tmpBuf++)<<8)&0xFF00;
	responseInfo->msgNum |= (*tmpBuf++)&0xFF;
	for(int i = 0;i < 2;i++)
	{
		(*tmpBuf++);
	}
	responseInfo->msgID |=  ((*tmpBuf++)<<8)&0xFF00;
	responseInfo->msgID |= (*tmpBuf++)&0xFF;
	responseInfo->result = (*tmpBuf++);
	
	return 0;
}

int CCANDataBYDStandard::SendTransferredMeaning(char *inBuf, int inLen, char *outBuf, int *outLen)
{
	int  dwI = 0;
	int  outPos = 0;
	
	if((inBuf == NULL) || (inLen <= 0) || (outBuf == NULL) || (outLen == NULL))
	{
		
		return -1;
	}

	for(dwI=0; dwI<inLen; dwI++)
	{
		if(inBuf[dwI] == 0x7e)
		{
			outBuf[outPos++] = 0x7d;
			outBuf[outPos++] = 0x02;
		}
		else if(inBuf[dwI] == 0x7d)
		{
			outBuf[outPos++] = 0x7d;
			outBuf[outPos++] = 0x01;
		}
		else
		{
			outBuf[outPos++] = inBuf[dwI];
		}
	}
	*outLen = outPos;
	return 0;
}

