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

#define PACKET_MAGIC 0xC5CC5C55			// ����־

#define CMD_ID_READ_TEMPERATURE	0xA5	// ���¶�����

// ��ͷ�ṹ
#pragma pack(push, 1)
typedef struct
{
	int nMagic;			// ����־
	int nPacketLength;	// ������, ��������ͷ��С��ָ�������ݳ���
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

	// ��ȡ��ʼ��־�Ͱ�����
	SCmdPacketHead packetHead;
	nRet = Read((char *)&packetHead, sizeof(packetHead), nTimeOut);
	if(nRet != sizeof(packetHead))
	{
		goto err;
	}

	// �����ͷ
	if(packetHead.nMagic != PACKET_MAGIC)
	{
		goto err;
	}

	// ��ȡ��������
	/*
	if(packetHead.nPacketLength>0)
	{
		// �����ڴ�
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

	// ��ȡ���������
	*pData = pBuf;
	*pDataLen = packetHead.nPacketLength;
	return TRUE;

err:
	DisConnect();
	return FALSE;
}

BOOL CDtsDevice::SendPacket(void *pData, int nDataLen, unsigned int nTimeOut)
{
	// ���Ͱ�ͷ
	SCmdPacketHead sHead;
	sHead.nMagic = PACKET_MAGIC;
	sHead.nPacketLength = nDataLen;
	if(Send((char*)&sHead, sizeof(sHead), nTimeOut) != sizeof(sHead))
	{
		return FALSE;
	}

	// ������������
	if(Send((char*)pData, nDataLen, nTimeOut) != nDataLen)
	{
		return FALSE;
	}

	return TRUE;
}

// ��ȡ�¶�����
// �����¶�ֵ����������
// pResponseָ����ڴ������ڲ��ģ��û�ʹ�õ��ú������������ݿ�����
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
