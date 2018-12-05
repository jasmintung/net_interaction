#include "ConnectCtrl.h"
#include <stdlib.h> 
#include <stdio.h> 
#include <errno.h> 
#include <string.h> 
#include <netdb.h> 
#include <sys/types.h> 
#include <netinet/in.h> 
#include <sys/socket.h> 
#include <arpa/inet.h>
#include <sys/select.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <netinet/tcp.h>
#include <linux/if.h>

ConnectCtrl::ConnectCtrl()
{

}
ConnectCtrl::~ConnectCtrl()
{

}
int ConnectCtrl::Net_Select(int socketFd,const int selectSec,const int selectUsec,const NetSelectType_E selectType)
{

	fd_set rdfds; /* ������һ�� fd_set ��������������Ҫ���� socket��� */
	struct timeval tv; /* ����һ��ʱ�����������ʱ�� */ 
	int ret = -1; /* ���淵��ֵ */

	FD_ZERO(&rdfds); /* ��select����֮ǰ�ȰѼ������� */  
	FD_SET(socketFd, &rdfds); /* ��Ҫ���ľ��socket���뵽������ */  
  
	tv.tv_sec = selectSec;  
	tv.tv_usec = selectUsec; /* ����select�ȴ������ʱ��Ϊ1���500���� */  
	switch( selectType )
	{
		case NetSelectType_Read:
			ret = select(socketFd + 1, &rdfds, NULL, NULL, &tv); 
			break;
		case NetSelectType_Write:
			ret = select(socketFd + 1, NULL, &rdfds, NULL, &tv); 
			break;
		case NetSelectType_Excpt:
			ret = select(socketFd + 1, NULL, NULL, &rdfds, &tv); 
			break;
	}
	/* ��������������õ�����rdfds��ľ���Ƿ��пɶ���Ϣ */  
  
	if(ret < 0)
	{		
		printf("select");/*select�������� */  
		
	}
	else if(ret == 0)
	{
		printf("��ʱ\n"); /*�������趨��ʱ��ֵ1���500�����ʱ���ڣ�socket��״̬û�з����仯 */  
  
	}
	else 
	{/* ˵���ȴ�ʱ�仹δ��1���500���룬socket��״̬�����˱仯 */  
    	printf("ret=%d\n", ret);/* ret�������ֵ��¼�˷���״̬�仯�ľ������Ŀ����������ֻ������socket��һ���������������һ��ret=1�����ͬʱ�ж����������仯���صľ��Ǿ�����ܺ��� */  
    	
	}
	return ret;
}
int ConnectCtrl::Tcp_Connect(const char *peerIp,const short peerPort,const int conSec,const int conUsec)
{
	
	int sockfd = -1; 
    struct sockaddr_in server_addr; //�����������ĵ�ַ
	bzero(&server_addr,sizeof(server_addr)); // ��ʼ��,��0
	//struct hostent *host; 
	char errorInfo[128] = {0};
    int nbytes;
	int connectRet = 0;
	unsigned long ul = 1;
	socklen_t sockLen = sizeof(int);
	int	getError 	= 0;
    /*��ʼ���� sockfd������ */ 
    if((sockfd=socket(AF_INET,SOCK_STREAM,0))==-1) // AF_INET:Internet;SOCK_STREAM:TCP
    { 
        fprintf(stderr,"Socket Error:%s\a\n",strerror(errno)); 
		printf("socket failed \n");
        return -1;
    } 
	int KeepAlive=1; 
	socklen_t KPlen=sizeof(int); 
	if(setsockopt(sockfd,SOL_SOCKET,SO_KEEPALIVE,(char *)&KeepAlive,KPlen)!=0)
	{ 
		printf("setsockopt error!/n"); 
		return -1;
	} 
    /*������˵���Ϣ*/ 
	server_addr.sin_addr.s_addr = inet_addr(peerIp);
	if (INADDR_NONE == server_addr.sin_addr.s_addr)
	{
		printf( "[%s]ipaddr error, invalid ipaddr[%s]\n", 
			__FUNCTION__, 
			peerIp );

		return	-1;
	}
    
    server_addr.sin_family = AF_INET; // IPV4
    server_addr.sin_port = htons(peerPort); // (���������ϵ�short����ת��Ϊ�����ϵ�short����)�˿ں�
    //server_addr.sin_addr =*((struct in_addr *)peerIp); // IP��ַ
    /*������������ */ 
	printf("socket  = %d \n",sockfd);
	int setRet = -1;
	int curSet = fcntl( sockfd, F_GETFL, 0 );
	setRet = fcntl( sockfd, F_SETFL, ( curSet | O_NONBLOCK ) );	//����Ϊ������
	connectRet = connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr));
	
    if(connectRet < 0) 
    {//connect���û���������,���ӹ�������ʱ,������ͨ��select������socket״̬�Ƿ�仯

		if ( (EINPROGRESS == errno) ||
			(EALREADY == errno) )
		{
			connectRet = Net_Select(sockfd, conSec, conUsec,NetSelectType_Write);

			if (connectRet > 0)
			{
				connectRet = -1;
				if (getsockopt(sockfd, SOL_SOCKET, SO_ERROR,(char *)&getError, &sockLen) >= 0)
				{
					if (0 != getError)
					{
						connectRet = -1;
					}
					else
					{
						connectRet = 1;
					}
				}
			}
		}
		if(connectRet <= 0)
		{
			sprintf(errorInfo,"Connect Error:%s\a\n",strerror(errno));
			printf("%s \n",errorInfo);
			close(sockfd); 
			sockfd = -1;
			printf("11connect failed \n");		//�������ر�
	        return -1; 
		}
        
    }
	else if(connectRet == 0)
	{
		sprintf(errorInfo,"Connect Error:%s\a\n",strerror(errno));
		printf("%s \n",errorInfo);
		close(sockfd); 
		sockfd = -1;
		printf("22connect failed \n");
        return -1; 
	}
	if(Net_Select(sockfd,conSec,conUsec,NetSelectType_Write) <= 0)
	{
		
		fprintf(stderr,"Select Error:%s\a\n",strerror(errno));
		printf("Net_Select failed \n");
		return -1;
	}
	//send(sockfd, "Attention: A Client has enter...\n", strlen("Attention: A Client has enter...\n")+1, 0); 
	return sockfd;
}
int ConnectCtrl::Tcp_Write(int socketFd,const int timeoSec,
		 			      const int timeoUsec,const int bufLen,
		 			      const int sendLen,const char *senBuf,
		 			      const BlockType_E blockType)
{
	if(!senBuf)
	{
		return -1;
	}
	if(socketFd <= 0)
	{
		return -1;
	}

	int	sendTotal = 0;
	int	sendRet = -1;
	int times = 0;
	if(blockType == BlockType_Block)
	{
		if (Net_Select(socketFd, timeoSec, timeoUsec,NetSelectType_Write) <= 0)
		{
			printf( "[Net_Select]:error" );
			return	-1;
		}
	}
	while (sendTotal < sendLen)
	{
		sendRet = send(socketFd, (senBuf + sendTotal), (sendLen - sendTotal), 0);
		
		if ( sendRet <= 0 )
		{
			printf( "[send]:error" );
			
			if((errno == EAGAIN) || (errno == EINTR))
			{
				if (times < 5)
				{
					times++;
					continue;
				}
			}
			sendTotal = -1;
			break;
		}
		times = 0;						
		printf("[%s][%d]sendRet:%d\n",__FUNCTION__,__LINE__,sendRet);
		sendTotal += sendRet;
	}
	return sendTotal;
}
int	ConnectCtrl::Tcp_ReadData( 
					int sockFd, 
					const int	timeoSec, 
					const int	timeoUsec,
					const int	wantRecvLen, 
					const int bufLen,
					char* recvBuf )
{
	if ( NULL == recvBuf )
	{
		printf( "[%s]Point Para Error\n", 
			__FUNCTION__ );

		return	( -1 );
	}

	if(sockFd < 0)
	{
		return -1;
	}
	
	if ( wantRecvLen > bufLen )
	{
		printf( "want recv length[%d] big than buflen[%d]\n", 
			wantRecvLen, 
			bufLen );

		return	( -1 );
	}

	int readRet = Net_Select( sockFd, timeoSec, timeoUsec, NetSelectType_Read );
	
	if ( readRet > 0 )
	{
		readRet = recv( sockFd, recvBuf, wantRecvLen, 0 );
		if ((readRet <= 0)&&(errno != EINTR))
		{
			/*�Զ˹ػ�*/
			printf("[recv close by peer]:");
			close(sockFd);
			return	-1;
		}
	}
	
	return	readRet;
}

