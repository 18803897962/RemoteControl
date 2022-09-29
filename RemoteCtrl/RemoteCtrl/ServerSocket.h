#pragma once
#include "pch.h"
#include "framework.h"

class CPacket   //声明数据包的类
{
public:
	CPacket():sHead(0),nLength(0),sCmd(0),sSum(0) {}
	CPacket(const CPacket& packet) {
		sHead = packet.sHead;
		nLength = packet.nLength;
		sCmd = packet.sCmd;
		strData = packet.strData;
		sSum = packet.sSum;
	}
	CPacket(const BYTE* pData, size_t& nSize) {
		size_t i = 0;   //数据段的偏移量
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i+4+2+2 > nSize) {  //包数据可能不全，或包头未能全部接收，解析失败
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) {  //避免只解析一半的情况，包未完全接收到就返回，解析失败
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
		WORD sum=0;  //进行校验
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[i]) & 0xFF;
		}
		if (sum == sSum) { //解析成功
			nSize = i ;  //此时的i是包的实际长度
			return;
		}
		nSize = 0;  //解析失败
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
public:
	WORD sHead;  //包头，固定位FEFF
	DWORD nLength;  //包长度，控制命令开始到和校验结束
	WORD sCmd; //控制命令
	std::string strData;  //数据
	WORD sSum;  //和校验
};
class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {  //静态函数没有this指针,无法直接访问私有成员变量
			m_instance = new CServerSocket();
		}
		return m_instance;
	}  //设置静态函数，用于后续调用类的私有成员函数
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
		char buffer[1024];
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
			//TODO:处理命令
			index +=len;  //这个len是收到数据包的长度
			len = index;
			m_packet=CPacket((BYTE*)buffer, index);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE-len);  //这个len是实际上有效的数据包长度
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}
	bool Send(const char* pData, size_t nSize) {
		return send(m_client, pData, nSize, 0)>0?true:false;
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
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误!"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);  //使用tcp
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
	static CServerSocket* m_instance;  //静态函数没有this指针,无法直接访问私有成员变量，因此将m_instance也设置为静态变量
	static void releaseInstance()
	{
		if (m_instance != NULL) {
			CServerSocket* temp = m_instance;
			m_instance = NULL;
			delete temp;   //temp 指向new出来的对象，delete时会调用其析构函数
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
	static CHelper m_helper;   //声明一个静态类成员，用于调用其函数
};
extern CServerSocket server;

