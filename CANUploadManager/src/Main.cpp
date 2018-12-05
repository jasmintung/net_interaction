#include "CCANDataBYDStandard.h"

int main()
{

	CCANDataBYDStandard *pBydCanDatas = new CCANDataBYDStandard();
	if(pBydCanDatas)
	{
		pBydCanDatas->TcpConnectProc();
		pBydCanDatas->E6CanDatasProc();
	}
	pBydCanDatas->TransCanDatasToNet();
	while(1)
	{
		usleep(100*1000);
	}
	return 0;
}

