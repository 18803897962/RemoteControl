#pragma once
#include "pch.h"
#include "framework.h"
#include "Packet.h"
#include<list>
#include "Tools.h"
typedef void(*SOCKET_CALLBACK)(void*,int, std::list<CPacket>&,CPacket&);  //定义回调函数指针
class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {  //静态函数没有this指针,无法直接访问私有成员变量
			m_instance = new CServerSocket();
		}
		return m_instance;
	}  //设置静态函数，用于后续调用类的私有成员函数
	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527) {
		bool ret = InitSocket(port);
		if (ret == false) return -1;
		std::list<CPacket> listCPacket;
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (true)
		{
			if (AcceptClient() == false) {
				if (count >= 3) return -2;
				count++;
			}
			int ret = DealCommand();
			if (ret > 0) {
				m_callback(m_arg, ret, listCPacket, m_packet);
				while (listCPacket.size() > 0) {//有数据要发
					Send(listCPacket.front());  //发包
					listCPacket.pop_front(); //把发过的包丢弃
				}
			}
			CloseClient();
		}

		return 0;
	}
protected:
	bool InitSocket(short port) {
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(port);
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) 	return false;
		if (listen(m_sock, 1) == -1) return false;
		
		return true;
	}
	
	bool AcceptClient() {
		TRACE("AcceptClient called\r\n");
		//char buffer[1024];
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client =accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		TRACE("m_client=%d\r\n", m_client);
		if (m_client == -1) return false;
		return true;
	}
#define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_client == -1) return -1;
		//char buffer[1024]="";
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL) {
			TRACE("内存不足\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer, BUFFER_SIZE-index, 0);
			if (len <= 0) {
				delete[]buffer;
				return -1;
			}
			TRACE("recv:%d\r\n", len);
			//TODO:处理命令
			index +=len;  //这个len是收到数据包的长度
			len = index;
			m_packet=CPacket((BYTE*)buffer, index);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE-len);  //这个len是实际上有效的数据包长度
				index -= len;
				delete[]buffer;
				return m_packet.sCmd;
			}
		}
		delete[]buffer;
		return -1;
	}
	bool Send(const char* pData, size_t nSize) {
		return send(m_client, pData, nSize, 0)>0?true:false;
	}
	bool Send(CPacket& pack) {
		CTools::Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_client, pack.Data(), pack.Size(), 0) > 0 ? true : false;
	}
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd >= 2)&&(m_packet.sCmd <= 4)||(m_packet.sCmd==9)) {  //当前命令为获取文件列表时，此时数据段strData为所需路径
			strPath = m_packet.strData;
			return true;
		}
		return false;
	}
	bool GetMouseEvent(MOUSEEV& mouse) {
		if (m_packet.sCmd == 5) {
			memcpy(&mouse, m_packet.strData.c_str(), sizeof(mouse));  //将鼠标点击的指令信息复制给mouse
			return true;
		}
		return false;
	}
	CPacket& GetPacket() {

		return m_packet;
	}
	void CloseClient() {
		if (m_client != INVALID_SOCKET) {
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	}
private:
	SOCKET_CALLBACK m_callback=NULL;
	void* m_arg=NULL;
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