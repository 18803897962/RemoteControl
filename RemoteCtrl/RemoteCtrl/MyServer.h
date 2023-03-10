#pragma once
#include "MyThread.h"
#include <map>
#include "Queue.h"
#include<MSWSock.h>

enum MyOperator{
	eNone,
	eAccept,
	eRecv,
	eSend,
	eError
};

class CMyClient;
class CMyServer;
typedef std::shared_ptr<CMyClient> PCLIENT;

class CMyOverlapped
{
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;//操作  参见MyOperator
	std::vector<char> m_buffer;//缓冲区
	ThreadWorker m_worker;//处理函数
	CMyServer* m_server;
	CMyClient* m_client;//对应的客户端
	WSABUF m_wsabuffer;
	virtual ~CMyOverlapped() {
		m_buffer.clear();
	}
};

template<MyOperator>class AcceptOverlapped;
typedef AcceptOverlapped<eAccept> ACCEPTOVERLAPPED;
template<MyOperator>class RecvOverlapped;
typedef RecvOverlapped<eRecv> RECVOVERLAPPED;
template<MyOperator>class SendOverlapped;
typedef SendOverlapped<eSend> SENDOVERLAPPED;

class CMyClient:public ThreadFuncBase {
public:
	CMyClient();
	~CMyClient() {
		m_buffer.clear();
		closesocket(m_sock);
		m_recv.reset();
		m_send.reset();
		m_acceptoverlapped.reset();
		m_vecSend.Clear();
	}

	void SetOverlapped(PCLIENT& ptr);

	operator SOCKET() {  //强制类型转换  (SOCKET)CMyClient时返回m_client
		return m_sock;
	}
	operator PVOID() {
		return &m_buffer[0];
	}
	operator LPOVERLAPPED();
	operator LPDWORD() {
		return &m_received;
	}
	
	LPWSABUF RecvWSABuffer();
	LPWSAOVERLAPPED RecvOverlapped();
	LPWSABUF SendWSABuffer();
	LPWSAOVERLAPPED SendOverlapped();
	sockaddr_in* GetLocalAddr() {
		return &m_laddr;
	}
	sockaddr_in* GetRemoteAddr() {
		return &m_raddr;
	}
	size_t GetBufferSize()const {
		return m_buffer.size();
	}
	DWORD& flags() {
		return m_flags;
	}
	int Recv();
	int Send(void* buffer, size_t nSize);
	int Senddata(std::vector<char>& data);
private:

	SOCKET m_sock;
	std::vector<char> m_buffer;
	size_t m_used;//已经使用的缓冲区大小
	DWORD m_flags=0;
	//sockaddr_in m_addr;
	sockaddr_in m_laddr;
	sockaddr_in m_raddr;
	bool m_isBasy;
	DWORD m_received;
	std::shared_ptr<ACCEPTOVERLAPPED> m_acceptoverlapped;
	std::shared_ptr<RECVOVERLAPPED> m_recv;
	std::shared_ptr<SENDOVERLAPPED> m_send;
	CSendQueue<std::vector<char>> m_vecSend;//发送数据队列
};


template<MyOperator>
class AcceptOverlapped :public CMyOverlapped,ThreadFuncBase
{
public:
	AcceptOverlapped();
	int AcceptWorker();
};



template<MyOperator>
class RecvOverlapped :public CMyOverlapped, ThreadFuncBase
{
public:
	RecvOverlapped();
	int RecvWorker() {
		int ret = m_client->Recv();
		return ret;
	}
};

template<MyOperator>
class SendOverlapped :public CMyOverlapped, ThreadFuncBase
{
public:
	SendOverlapped();
	int SendWorker() {
		/*
		* 
		*/
		return -1;
	}
};

template<MyOperator>
class ErrorOverlapped :public CMyOverlapped, ThreadFuncBase
{
public:
	ErrorOverlapped() :m_operator(eAccept), m_worker(this, &ErrorOverlapped::ErrorWorker) {
		memset(&m_overlapped, 0, sizeof(OVERLAPPED));
		m_buffer.resize(1024);
	}
	int ErrorWorker() {
		return -1;
	}
};
typedef ErrorOverlapped<eError> ERROROVERLAPPED;


typedef std::shared_ptr<CMyClient> PCLIENT;

class CMyServer :public ThreadFuncBase{
public:
	CMyServer(const std::string ip="0.0.0.0",short port=9527) :m_pool(10) {
		m_hIOCP = INVALID_HANDLE_VALUE;
		m_sock = INVALID_SOCKET;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(port);
		m_addr.sin_addr.s_addr = inet_addr(ip.c_str());
	}
	~CMyServer();
	bool StartService();
	bool NewAccept();
	void BindNewSocket(SOCKET s);
private:
	void CreateSocket();
	int threadIOCP();
private:
	MyThreadPoor m_pool;
	HANDLE m_hIOCP;
	SOCKET m_sock;
	std::map<SOCKET, std::shared_ptr<CMyClient>> m_client;  //智能指针使用映射表
	sockaddr_in m_addr;
};

