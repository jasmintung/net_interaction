#ifndef CONNECT_CTRL_H
#define CONNECT_CTRL_H

enum NetSelectType_E
{
	NetSelectType_Read,
	NetSelectType_Write,
	NetSelectType_Excpt,
};
enum BlockType_E
{
	BlockType_Block,		//��Ҫ����
	BlockType_NonBlock,		//����Ҫ����
};

class ConnectCtrl
{
	public:
		 ConnectCtrl();
		 ~ConnectCtrl();
		 int Net_Select(int socketFd,const int selectSec,const int selectUsec,const NetSelectType_E selectType);
		 int Tcp_Connect(const char *peerIp,const short peerPort,const int conSec,const int conUsec);
		 int Tcp_Write(int socketFd,const int timeoSec,
		 			      const int timeoUsec,const int bufLen,
		 			      const int sendLen,const char *senBuf,
		 			      const BlockType_E blockType);
		 int Tcp_ReadData(
					int sockFd,const int timeoSec, 
					const int	timeoUsec,const int	wantRecvLen, 
					const int bufLen,char* recvBuf );
};
#endif
