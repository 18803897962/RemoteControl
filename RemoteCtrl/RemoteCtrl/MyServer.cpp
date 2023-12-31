#include "pch.h"
#include "MyServer.h"
#include "Tools.h"
#pragma warning(disable:4407)
template<MyOperator op>
AcceptOverlapped<op>::AcceptOverlapped(){
	m_worker = ThreadWorker(this, (FUNCTYPE)&AcceptOverlapped<op>::AcceptWorker);
	m_operator = eAccept;
	memset(&m_overlapped, 0, sizeof(OVERLAPPED));
	m_buffer.resize(1024);
	m_server = NULL;
}
template<MyOperator op>
int AcceptOverlapped<op>::AcceptWorker() {
	INT lLength = 0, rLength = 0;
	if (m_client->GetBufferSize()> 0) {//收到buffer值
		sockaddr* pLocal = NULL, * pRemote = NULL;
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)&pLocal, &lLength, //本地地址
			(sockaddr**)&pRemote, &rLength);//远程地址
		memcpy(m_client->GetLocalAddr(), pLocal, sizeof(sockaddr_in));
		memcpy(m_client->GetRemoteAddr(), pRemote, sizeof(sockaddr_in));
		m_server->BindNewSocket(*m_client);
		//RECVOVERLAPPED 
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), m_client->RecvOverlapped(), NULL);
		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			//TODO:报错
			TRACE("ret= %d error num=%d", ret, WSAGetLastError());
		}
		if (!m_server->NewAccept()) {
			return -2;
		}
	}
	return -1;
}
CMyClient::CMyClient() :m_isBasy(false), m_flags(0),
			m_acceptoverlapped(new ACCEPTOVERLAPPED()), 
			m_recv(new RECVOVERLAPPED()),
			m_send(new SENDOVERLAPPED()), 
			m_vecSend(this,(SendCallBack)&CMyClient::Senddata){
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(sockaddr_in));
	memset(&m_raddr, 0, sizeof(sockaddr_in));
	//m_acceptoverlapped->m_client.reset(this);
}

void CMyClient::SetOverlapped(PCLIENT& ptr) {  //typedef std::shared_ptr<CMyClient> PCLIENT;
	m_acceptoverlapped->m_client = ptr.get();
	m_recv->m_client = ptr.get();
	m_send->m_client = ptr.get();

}

CMyClient::operator LPOVERLAPPED() {
	return &m_acceptoverlapped->m_overlapped;
}

LPWSABUF CMyClient::RecvWSABuffer() {
	return &m_recv->m_wsabuffer;
}

int CMyClient::Recv() {
	int ret = recv(m_sock, m_buffer.data() + m_used, m_buffer.size() - m_used, 0);
	if (ret <= 0) return -1;
	m_used += (size_t)ret;
	CTools::Dump((BYTE*)m_buffer.data(), ret);
	//TODO:解析数据
	return 0;
}

LPWSAOVERLAPPED CMyClient::RecvOverlapped() {
	return &m_send->m_overlapped;
}

int CMyClient::Send(void* buffer, size_t nSize){
	std::vector<char> data(nSize);
	memcpy(data.data(), buffer, nSize);
	if (m_vecSend.PushBack(data) == true) {
		return 0;
	}
	return - 1;
}

LPWSAOVERLAPPED CMyClient::SendOverlapped() {
	return &m_send->m_overlapped;
}

int CMyClient::Senddata(std::vector<char>& data)
{
	if (m_vecSend.Size() > 0) {//还有数据需要发送
		int ret=WSASend(m_sock,SendWSABuffer(),1,&m_received,m_flags,&m_send->m_overlapped,NULL);
		if (ret != 0&&WSAGetLastError()!=WSA_IO_PENDING) {
			CTools::ShowError();
			return -1; //有问题
		}
	}
	return 0;
}

LPWSABUF CMyClient::SendWSABuffer() {
	return &m_send->m_wsabuffer;
}

CMyServer::~CMyServer() {
	closesocket(m_sock);
	std::map<SOCKET, PCLIENT>::iterator it = m_client.begin();
	for (; it != m_client.end(); it++) {
		it->second.reset();  //智能指针，直接reset
	}
	m_client.clear();
	CloseHandle(m_hIOCP);
	m_pool.Stop();
	WSACleanup();
}

bool CMyServer::StartService() {
	CreateSocket();
	if (bind(m_sock, (sockaddr*)&m_addr, sizeof(sockaddr_in)) == -1) {//绑定失败
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	if (listen(m_sock, 3) == -1) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		return false;
	}
	m_hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 4);
	if (m_hIOCP == NULL) {
		closesocket(m_sock);
		m_sock = INVALID_SOCKET;
		m_hIOCP = INVALID_HANDLE_VALUE;
		return false;
	}
	CreateIoCompletionPort((HANDLE)m_sock, m_hIOCP, (ULONG_PTR)this, 0);
	m_pool.Invoke();
	m_pool.dispatchWorker(ThreadWorker(this, (FUNCTYPE)&CMyServer::threadIOCP));
	if (NewAccept() == false) {
		return false;
	}
	return true;
}

bool CMyServer::NewAccept() {
	PCLIENT pClient(new CMyClient());  //CMyClient*转换为智能指针
	pClient->SetOverlapped(pClient);
	m_client.insert(std::pair<SOCKET, PCLIENT>(*pClient, pClient));
	if (AcceptEx(m_sock, *pClient, *pClient, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, *pClient, *pClient) == FALSE) {
		TRACE("%d\r\n", WSAGetLastError());
		if (WSAGetLastError() != WSA_IO_PENDING) {  //错误码997 重叠IO进行中，此情况不能认为是错误的
			closesocket(m_sock);
			m_sock = INVALID_SOCKET;
			m_hIOCP = INVALID_HANDLE_VALUE;
			return false;
		}
	}
	return true;
}

void CMyServer::BindNewSocket(SOCKET s)
{
	CreateIoCompletionPort((HANDLE)s, m_hIOCP, (ULONG_PTR)this, 0);
}

void CMyServer::CreateSocket() {
	WSADATA WSAData;
	WSAStartup(MAKEWORD(2, 2), &WSAData);
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	int opt = 1;
	setsockopt(m_sock, SOL_SOCKET, SO_REUSEADDR, (const char*)&opt, sizeof(opt));
}

int CMyServer::threadIOCP() {
	DWORD transferred = 0;
	ULONG_PTR completionkey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &completionkey, &lpOverlapped, INFINITE)) {
		if ( completionkey != 0) {
			CMyOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, CMyOverlapped, m_overlapped);
			pOverlapped->m_server = this;
			TRACE("pOverlapped->m_operator=%d\r\n", pOverlapped->m_operator);
			switch (pOverlapped->m_operator) {
			case eAccept:
			{
				ACCEPTOVERLAPPED* pAccept = (ACCEPTOVERLAPPED*)pOverlapped;
				m_pool.dispatchWorker(pAccept->m_worker);
			}
			break;
			case eRecv:
			{
				RECVOVERLAPPED* pRecv = (RECVOVERLAPPED*)pOverlapped;
				m_pool.dispatchWorker(pRecv->m_worker);
			}
			break;
			case eSend:
			{
				SENDOVERLAPPED* pSend = (SENDOVERLAPPED*)pOverlapped;
				m_pool.dispatchWorker(pSend->m_worker);
			}
			break;
			case eError:
			{
				ERROROVERLAPPED* pError = (ERROROVERLAPPED*)pOverlapped;
				m_pool.dispatchWorker(pError->m_worker);
			}
			break;
			}
		}
		else {
			return -1;
		}

	}
	return 0;
}

template<MyOperator op>
inline RecvOverlapped<op>::RecvOverlapped() {
	m_operator = op;
	m_worker = ThreadWorker(this, (FUNCTYPE)&RecvOverlapped<op>::RecvWorker);
	memset(&m_overlapped, 0, sizeof(OVERLAPPED));
	m_buffer.resize(1024 * 256);
}

template<MyOperator op>
inline SendOverlapped<op>::SendOverlapped() {
	m_operator = eSend;
	m_worker = ThreadWorker(this, (FUNCTYPE)&SendOverlapped<op>::SendWorker);
	memset(&m_overlapped, 0, sizeof(OVERLAPPED));
	m_buffer.resize(1024 * 256);
}
