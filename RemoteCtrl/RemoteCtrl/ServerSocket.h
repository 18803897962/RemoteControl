#pragma once
#include "pch.h"
#include "framework.h"

#pragma pack(push)
#pragma pack(1)
class CPacket   //�������ݰ�����
{
public:
	CPacket():sHead(0),nLength(0),sCmd(0),sSum(0) {}
	CPacket(WORD nCmd, const BYTE* pData, size_t nSize) {
		sHead = 0xFEFF;
		nLength = nSize + 4;
		sCmd = nCmd;
		if (nSize > 0) {
			strData.resize(nSize);
			memcpy((void*)strData.c_str(), pData, nSize);
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
		if (i+4+2+2 > nSize) {  //�����ݿ��ܲ�ȫ�����ͷδ��ȫ�����գ�����ʧ��
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) {  //����ֻ����һ����������δ��ȫ���յ��ͷ��أ�����ʧ��
			nSize = 0;
			return;
		}
		sCmd = *(WORD*)(nLength + i); i += 2;
		if (nLength > 4) {
			strData.resize(nLength - 2 - 2);
			memcpy((void*)strData.c_str(), pData + i, nLength - 4);
			i += nLength - 4;
		}
		sSum = *(WORD*)(pData + i); i += 2;
		WORD sum=0;  //����У��
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) { //�����ɹ�
			nSize = i ;  //��ʱ��i�ǰ���ʵ�ʳ���
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
	WORD sHead;  //��ͷ���̶�λFEFF
	DWORD nLength;  //�����ȣ��������ʼ����У�����
	WORD sCmd; //��������
	std::string strData;  //����
	WORD sSum;  //��У��
	std::string strOut; //�����������ݱ�����strOut��
};

#pragma pack(pop)

typedef struct MouseEvent{
	MouseEvent() {//Ĭ�Ϲ��캯��
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//����������������ƶ���˫��
	WORD nButton;//������Ҽ�������
	POINT ptXY;//����
}MOUSEEV,*PMOUSEEV;
class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {  //��̬����û��thisָ��,�޷�ֱ�ӷ���˽�г�Ա����
			m_instance = new CServerSocket();
		}
		return m_instance;
	}  //���þ�̬���������ں����������˽�г�Ա����
	bool InitSocket() {
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(9527);
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) 	return false;
		if (listen(m_sock, 1) == -1) return false;
		return true;
	}
	bool AcceptClient() {
		//char buffer[1024];
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client =accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		if (m_client == -1) return false;
		return true;
	}
#define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_client == -1) return -1;
		//char buffer[1024]="";
		char* buffer = new char[BUFFER_SIZE];
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer, BUFFER_SIZE-index, 0);
			if (len <= 0) return -1;
			//TODO:��������
			index +=len;  //���len���յ����ݰ��ĳ���
			len = index;
			m_packet=CPacket((BYTE*)buffer, index);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE-len);  //���len��ʵ������Ч�����ݰ�����
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}
	bool Send(const char* pData, size_t nSize) {
		return send(m_client, pData, nSize, 0)>0?true:false;
	}
	bool Send(CPacket& pack) {
		return send(m_client, pack.Data(), pack.Size(), 0) > 0 ? true : false;
	}
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd >= 2)&&(m_packet.sCmd <= 4)) {  //��ǰ����Ϊ��ȡ�ļ��б�ʱ����ʱ���ݶ�strDataΪ����·��
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(mouse));  //���������ָ����Ϣ���Ƹ�mouse
			return true;
		}
		return false;
	}
private:
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_client = ss.m_client;
		m_sock = ss.m_sock;
	}
	CServerSocket() {
		m_sock = INVALID_SOCKET;   //INVALID_SOCKET	=-1
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ�����������������"), _T("��ʼ������!"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);  //ʹ��tcp
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
	static CServerSocket* m_instance;  //��̬����û��thisָ��,�޷�ֱ�ӷ���˽�г�Ա��������˽�m_instanceҲ����Ϊ��̬����
	static void releaseInstance()
	{
		if (m_instance != NULL) {
			CServerSocket* temp = m_instance;
			m_instance = NULL;
			delete temp;   //temp ָ��new�����Ķ���deleteʱ���������������
		}
	}
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;   //����һ����̬���Ա�����ڵ����亯��
};
extern CServerSocket server;

