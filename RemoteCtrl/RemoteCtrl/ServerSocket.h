#pragma once
#include "pch.h"
#include "framework.h"
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
		if (m_sock == -1) return false;
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(9527);
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)	return false;
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
//             recv(serv_sock, buffer, sizeof(buffer), 0);
//             send(serv_sock, buffer, sizeof(buffer), 0);
	}
	int DealCommand() {
		if (m_client == -1) return -1;
		char buffer[1024];
		while (true) {
			int len = recv(m_client, buffer, sizeof(buffer), 0);
			if (len <= 0) return -1;
			//TODO:处理命令
		}
	}
	bool Send(const char* pData, size_t nSize) {
		return send(m_client, pData, nSize, 0)>0?true:false;
	}
private:
	SOCKET m_sock;
	SOCKET m_client;
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
		SOCKET m_sock = socket(PF_INET, SOCK_STREAM, 0);  //使用tcp
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

