#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>
#define WM_SEND_PACK (WM_USER+1) //���Ͱ�����
#define WM_SEND_PACK_ACK (WM_USER+2) //�������ݰ�Ӧ��
#pragma pack(push)
#pragma pack(1)
class CPacket   //�������ݰ�����
{
public:
	CPacket() :sHead(0), nLength(0), sCmd(0), sSum(0) {}
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
	CPacket(const BYTE* pData, size_t& nSize){
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
	const char* Data(std::string& strOut) const{
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

enum {
	CSM_AUTOCLOSE = 1  //CSM===Client Socket Mode,Ϊ1��ʾ�Զ��ر�
};

typedef struct PacketData
{
	std::string strData;//������
	UINT nMod;//ģʽ
	WPARAM wParam;
	PacketData(const char* pData,size_t nLen,UINT mode,WPARAM nParam=0) {
		strData.resize(nLen);
		memcpy((char*)strData.c_str(), pData, nLen);
		nMod = mode;
		wParam = nParam;
	}
	PacketData(const PacketData& pack) {
		strData = pack.strData;
		nMod = pack.nMod;
		wParam = pack.wParam;
	}
	PacketData& operator=(const PacketData& pack) {
		if (this != &pack) {
			strData = pack.strData;
			nMod = pack.nMod;
			wParam = pack.wParam;
		}
		return *this;
	}
}PACKET_DATA;

std::string GetErrorInfo(int wsaErrorCode);
class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		if (m_instance == NULL) {  //��̬����û��thisָ��,�޷�ֱ�ӷ���˽�г�Ա����
			m_instance = new CClientSocket();
			TRACE("CClientSocket size =%d\r\n", sizeof(CClientSocket));
		}
		return m_instance;
	}  //���þ�̬���������ں����������˽�г�Ա����
	bool InitSocket();
#define BUFFER_SIZE 4096000
	int DealCommand() {
		if (m_sock == -1) return -1;
		char* buffer = m_buffer.data();  
		//���̷߳�����������ڷ���ͼƬʱ��ͼƬ��tcp��Ϊ�����������ͼƬ���ղ���ȫ���������Ϣ�����ж�
		static size_t index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer+index, BUFFER_SIZE - index, 0);
			if (((int)len <= 0)&&((int)index<=0)) return -1;
			//TODO:��������
			index += len;  //���len���յ����ݰ��ĳ���
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, index - len);  //���len��ʵ������Ч�����ݰ�����
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}
	
	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPack, bool isAutoclose = true);
	bool SendPacket(HWND hWnd/*���ݰ��յ�����ҪӦ��Ĵ���*/, const CPacket& pack, bool isAutoclose = true, WPARAM wParam=0);
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {  //��ǰ����Ϊ��ȡ�ļ��б�ʱ����ʱ���ݶ�strDataΪ����·��
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
	CPacket& GetPacket() {
		return m_packet;
	}
	void CloseSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
	}
	void UpdateAddress(DWORD nIP, int nPort) {
		if (m_nIP != nIP || m_nPort == nPort) {
			m_nIP = nIP;
			m_nPort = nPort;
		}
	}
private:
	HANDLE m_EventInvoke;//�����¼�
	UINT m_nThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;
	HANDLE m_hThread;
	std::mutex m_lock;
	std::list<CPacket> m_lstSend;  //Ҫ���͵�����
	std::map<HANDLE, std::list<CPacket>&> m_mapAck; //���Ӧ��İ� ѡ��list��ԭ���ǣ�list��Ч���ܰ��Ĵ�СӰ���С����vector��Ч����Ӱ��ϴ�
	DWORD m_nIP;  //��¼IP
	int m_nPort; //��¼�˿�
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	bool m_isAutoClose;
	std::map<HANDLE, bool> m_mapAutoClose;
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss) {
		m_isAutoClose = ss.m_isAutoClose;
		m_hThread = INVALID_HANDLE_VALUE;
		m_sock = ss.m_sock;
		m_nIP = ss.m_nIP;
		m_nPort = ss.m_nPort;
		std::map<UINT, CClientSocket::MSGFUNC>::const_iterator it = ss.m_mapFunc.begin();
		for (; it != ss.m_mapFunc.end(); it++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>(it->first, it->second));
		}
	}
	CClientSocket();
	~CClientSocket() {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		WSACleanup();
	}
	bool Send(const char* pData, size_t nSize) {
		return send(m_sock, pData, nSize, 0) > 0 ? true : false;
	}
	bool Send(const CPacket& pack);
	void SendPack(UINT nMsg, WPARAM wParam/*��������ֵ*/, LPARAM lParam/*�������ĳ���*/);
	static unsigned __stdcall threadEntry(void* arg);
	//void threadFunc();
	void threadFunc2();
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(1, 1), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
	static CClientSocket* m_instance;  //��̬����û��thisָ��,�޷�ֱ�ӷ���˽�г�Ա��������˽�m_instanceҲ����Ϊ��̬����
	static void releaseInstance()
	{
		if (m_instance != NULL) {
			TRACE("CClientSocket delete called\r\n");
			CClientSocket* temp = m_instance;
			m_instance = NULL;
			delete temp;   //temp ָ��new�����Ķ���deleteʱ���������������
			TRACE("CClientSocket delete\r\n");
		}
	}
	class CHelper {
	public:
		CHelper() {
			CClientSocket::getInstance();
		}
		~CHelper() {
			CClientSocket::releaseInstance();
		}
	};
	static CHelper m_helper;   //����һ����̬���Ա�����ڵ����亯��
};
extern CClientSocket server;

