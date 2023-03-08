#include "pch.h"
#include "MyServer.h"
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
	if (*(LPDWORD)*m_client.get() > 0) {//收到buffer值
		GetAcceptExSockaddrs(*m_client, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16,
			(sockaddr**)m_client->GetLocalAddr(), &lLength, //本地地址
			(sockaddr**)m_client->GetRemoteAddr(), &rLength);//远程地址
		//RECVOVERLAPPED 
		int ret = WSARecv((SOCKET)*m_client, m_client->RecvWSABuffer(), 1, *m_client, &m_client->flags(), *m_client, NULL);
		if (ret == SOCKET_ERROR && WSAGetLastError() != WSA_IO_PENDING) {
			//TODO:报错

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
			m_send(new SENDOVERLAPPED()){
	m_sock = WSASocket(PF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	m_buffer.resize(1024);
	memset(&m_laddr, 0, sizeof(sockaddr_in));
	memset(&m_raddr, 0, sizeof(sockaddr_in));
	m_acceptoverlapped->m_client.reset(this);
}

void CMyClient::SetOverlapped(PCLIENT& ptr) {
	m_acceptoverlapped->m_client = ptr;
	m_recv->m_client = ptr;
	m_send->m_client = ptr;

}

CMyClient::operator LPOVERLAPPED() {
	return &m_acceptoverlapped->m_overlapped;
}

LPWSABUF CMyClient::RecvWSABuffer() {
	return &m_recv->m_wsabuffer;
}

LPWSABUF CMyClient::SendWSABuffer() {
	return &m_send->m_wsabuffer;
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

int CMyServer::threadIOCP() {
	DWORD transferred = 0;
	ULONG_PTR completionkey = 0;
	OVERLAPPED* lpOverlapped = NULL;
	if (GetQueuedCompletionStatus(m_hIOCP, &transferred, &completionkey, &lpOverlapped, INFINITE)) {
		if (transferred > 0 && completionkey != 0) {
			CMyOverlapped* pOverlapped = CONTAINING_RECORD(lpOverlapped, CMyOverlapped, m_overlapped);
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
