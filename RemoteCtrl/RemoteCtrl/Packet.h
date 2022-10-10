#pragma once
#include "pch.h"
#include "framework.h"
#pragma pack(push)
#pragma pack(1)
class CPacket   //�������ݰ�����
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
	CPacket(WORD nCmd, BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)(strData.c_str()), pData, nSize);
		}
		else strData.clear();
		sSum = 0;
		for (size_t j = 0; j < strData.size(); j++) {
			sSum += BYTE(strData[j]) & 0xFF;
		}
	}
	CPacket(const CPacket& packet) {
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSum = packet.sSum;

	}
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;   //���ݶε�ƫ����
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {  //�����ݿ��ܲ�ȫ�����ͷδ��ȫ�����գ�����ʧ��
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) {  //����ֻ����һ����������δ��ȫ���յ��ͷ��أ�����ʧ��
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(pData + i); i += 2;
		//memcpy(&sCmd, &pData + i, sizeof(WORD)); i += 2;

		TRACE("command=%d\r\n", sCmd);
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum = 0;  //����У��
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) { //�����ɹ�
			nSize = i;  //��ʱ��i�ǰ���ʵ�ʳ���
			return;
		}
		nSize = 0;  //����ʧ��
	}
	~CPacket() {}
	CPacket& operator=(const CPacket& pack) {
		if (this != &pack) {
			sHead = pack.sHead;
			nLength = pack.nLength;
			sCmd = pack.sCmd;
			strData = pack.strData;
			sSum = pack.sSum;
		}
		return *this;
	}
	int Size() {  //��ð����ݵĴ�С
		return nLength + 6;  //nLength�ǰ�����
	}
	const char* Data() {
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();  //����һ��ָ�룬���ں����޸��ַ�������
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
public:
	WORD sHead=0;  //��ͷ���̶�λFEFF
	DWORD nLength=0;  //�����ȣ��������ʼ����У�����
	WORD sCmd=0; //��������
	std::string strData;  //����
	WORD sSum=0;  //��У��
	std::string strOut; //�����������ݱ�����strOut��
};

#pragma pack(pop)

typedef struct MouseEvent {
	MouseEvent() {//Ĭ�Ϲ��캯��
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//����������������ƶ���˫��
	WORD nButton;//������Ҽ�������
	POINT ptXY;//����
}MOUSEEV, * PMOUSEEV;
typedef struct file_info
{
	file_info() { //���캯��
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid; //�Ƿ���Ч
	BOOL IsDirectory;//�Ƿ�ΪĿ¼ 0�� 1��
	BOOL HasNext; //�Ƿ��к����ļ� 0û�� 1��  ����ʵ���ҵ�һ������һ��
	char szFileName[260];
}FILEINFO, * PFILEINFO;