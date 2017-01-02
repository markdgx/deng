// DtsDevice.cpp: implementation of the CDtsDevice class.
//
//////////////////////////////////////////////////////////////////////

#include "stdafx.h"
#include "DTSDriverTest.h"
#include "DtsDevice.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

#define RECV_BUF_SIZE (1024*1024)
#define SEND_BUF_SIZE (1024*1024)
static unsigned char s_cRecvBuf[RECV_BUF_SIZE];

#define PACKET_MAGIC 0xC5CC5C55			// 包标志

#define CMD_ID_READ_TEMPERATURE	0xA5	// 读温度命令

// 包头结构
#pragma pack(push, 1)
typedef struct
{
	int nMagic;			// 包标志
	int nPacketLength;	// 包长度, 不包括包头大小，指后续数据长度
}SCmdPacketHead;
#pragma pack(pop)


//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

CDtsDevice::CDtsDevice()
{
	m_pBuffer = (char *)malloc(MAX_RESPONSE_PACKET_LENGTH);
}

CDtsDevice::~CDtsDevice()
{
	if(m_pBuffer != NULL)
	{
		free(m_pBuffer);
		m_pBuffer = NULL;
	}
}

BOOL CDtsDevice::ReadPacket(void **pData, int *pDataLen, unsigned int nTimeOut)
{
	int nRet;
	char *pBuf=NULL;

	// 读取起始标志和包长度
	SCmdPacketHead packetHead;
	nRet = Read((char *)&packetHead, sizeof(packetHead), nTimeOut);
	if(nRet != sizeof(packetHead))
	{
		goto err;
	}

	// 检验包头
	if(packetHead.nMagic != PACKET_MAGIC)
	{
		goto err;
	}

	// 读取包的数据
	/*
	if(packetHead.nPacketLength>0)
	{
		// 分配内存
		pBuf = (char *)malloc(packetHead.nPacketLength);
		if(pBuf == NULL)
		{
			goto err;
		}
		nRet = Read(pBuf, packetHead.nPacketLength, nTimeOut);
		if(nRet != (int)packetHead.nPacketLength)
		{
			free(pBuf);
			pBuf = NULL;
			goto err;
		}
	}
	*/

	if(packetHead.nPacketLength>0 )
	{
		if(packetHead.nPacketLength<=MAX_RESPONSE_PACKET_LENGTH && m_pBuffer != NULL)
		{
			pBuf = m_pBuffer;
			nRet = Read(pBuf, packetHead.nPacketLength, nTimeOut);
			if(nRet != (int)packetHead.nPacketLength)
			{
				goto err;
			}
		}
		else
		{
			goto err;
		}
	}

	// 提取命令包数据
	*pData = pBuf;
	*pDataLen = packetHead.nPacketLength;
	return TRUE;

err:
	DisConnect();
	return FALSE;
}

BOOL CDtsDevice::SendPacket(void *pData, int nDataLen, unsigned int nTimeOut)
{
	// 发送包头
	SCmdPacketHead sHead;
	sHead.nMagic = PACKET_MAGIC;
	sHead.nPacketLength = nDataLen;
	if(Send((char*)&sHead, sizeof(sHead), nTimeOut) != sizeof(sHead))
	{
		return FALSE;
	}

	// 发送命令数据
	if(Send((char*)pData, nDataLen, nTimeOut) != nDataLen)
	{
		return FALSE;
	}

	return TRUE;
}

// 读取温度命令
// 返回温度值和温升速率
// pResponse指向的内存是类内部的，用户使用调用函数后必须把数据拷贝走
BOOL CDtsDevice::ReadTemperature(int nChannel, SReadTemperature_Response **pResponse, int *pResponseLen, unsigned int nTimeOut)
{
	SReadTemperature_Request req;
	SReadTemperature_Response *pRes;
	void *pPacketData = NULL;
	int nPacketDataLen=0;

	req.nCmdID = CMD_ID_READ_TEMPERATURE;
	req.nChannel = (unsigned char)nChannel;
	if(!SendPacket(&req, sizeof(req), nTimeOut))
	{
		return FALSE;
	}

	if(!ReadPacket(&pPacketData, &nPacketDataLen, nTimeOut))
	{
		return FALSE;
	}

	if(nPacketDataLen < sizeof(SReadTemperature_Response) || pPacketData == NULL)
	{
		return FALSE;
	}

	pRes = (SReadTemperature_Response*)pPacketData;
	if(pRes->nCmdID != req.nCmdID 
	|| pRes->nChannel != req.nChannel
	|| (int)(sizeof(SReadTemperature_Response)+pRes->nTemperaturePointCount*sizeof(int)) != nPacketDataLen)
	{
		return FALSE;
	}
	
	*pResponse = pRes;
	*pResponseLen = nPacketDataLen;

	return TRUE;
}
