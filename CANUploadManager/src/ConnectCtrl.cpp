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

	fd_set rdfds; /* 先申明一个 fd_set 集合来保存我们要检测的 socket句柄 */
	struct timeval tv; /* 申明一个时间变量来保存时间 */ 
	int ret = -1; /* 保存返回值 */

	FD_ZERO(&rdfds); /* 用select函数之前先把集合清零 */  
	FD_SET(socketFd, &rdfds); /* 把要检测的句柄socket加入到集合里 */  
  
	tv.tv_sec = selectSec;  
	tv.tv_usec = selectUsec; /* 设置select等待的最大时间为1秒加500毫秒 */  
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
	/* 检测我们上面设置到集合rdfds里的句柄是否有可读信息 */  
  
	if(ret < 0)
	{		
		printf("select");/*select函数出错 */  
		
	}
	else if(ret == 0)
	{
		printf("超时\n"); /*在我们设定的时间值1秒加500毫秒的时间内，socket的状态没有发生变化 */  
  
	}
	else 
	{/* 说明等待时间还未到1秒加500毫秒，socket的状态发生了变化 */  
    	printf("ret=%d\n", ret);/* ret这个返回值记录了发生状态变化的句柄的数目，由于我们只监视了socket这一个句柄，所以这里一定ret=1，如果同时有多个句柄发生变化返回的就是句柄的总和了 */  
    	
	}
	return ret;
}
int ConnectCtrl::Tcp_Connect(const char *peerIp,const short peerPort,const int conSec,const int conUsec)
{
	
	int sockfd = -1; 
    struct sockaddr_in server_addr; //描述服务器的地址
	bzero(&server_addr,sizeof(server_addr)); // 初始化,置0
	//struct hostent *host; 
	char errorInfo[128] = {0};
    int nbytes;
	int connectRet = 0;
	unsigned long ul = 1;
	socklen_t sockLen = sizeof(int);
	int	getError 	= 0;
    /*开始建立 sockfd描述符 */ 
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
    /*填充服务端的信息*/ 
	server_addr.sin_addr.s_addr = inet_addr(peerIp);
	if (INADDR_NONE == server_addr.sin_addr.s_addr)
	{
		printf( "[%s]ipaddr error, invalid ipaddr[%s]\n", 
			__FUNCTION__, 
			peerIp );

		return	-1;
	}
    
    server_addr.sin_family = AF_INET; // IPV4
    server_addr.sin_port = htons(peerPort); // (将本机器上的short数据转化为网络上的short数据)端口号
    //server_addr.sin_addr =*((struct in_addr *)peerIp); // IP地址
    /*发起连接请求 */ 
	printf("socket  = %d \n",sockfd);
	int setRet = -1;
	int curSet = fcntl( sockfd, F_GETFL, 0 );
	setRet = fcntl( sockfd, F_SETFL, ( curSet | O_NONBLOCK ) );	//设置为非阻塞
	connectRet = connect(sockfd,(struct sockaddr *)(&server_addr),sizeof(struct sockaddr));
	
    if(connectRet < 0) 
    {//connect调用会立即返回,连接过程有延时,所以再通过select来分析socket状态是否变化

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
			printf("11connect failed \n");		//服务器关闭
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
			/*对端关机*/
			printf("[recv close by peer]:");
			close(sockFd);
			return	-1;
		}
	}
	
	return	readRet;
}

