#include "pch.h"
#include "ClientSocket.h"

//CClientSocket server;
CClientSocket* CClientSocket::m_instance = NULL;  //��m_instance�ÿ�
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstance();   //ȫ�ֱ�����ȫ�ֱ�����main����ִ��֮ǰ���Ѿ�����

std::string GetErrorInfo(int wsaErrorCode) {  //����geterrornum�Ĵ���ԭ�򷵻غ���
	std::string ret;
	LPVOID lpMsgBuf = NULL;
	FormatMessage(
		FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_ALLOCATE_BUFFER,
		NULL,
		wsaErrorCode,
		MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
		(LPTSTR)&lpMsgBuf,
		0,
		NULL
	);
	ret = (char*)lpMsgBuf;
	LocalFree(lpMsgBuf);
	return ret;
}
void CClientSocket::threadEntry(void* arg) {
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc();
}
void CClientSocket::threadFunc()
{
	std::string strBuffer;
	strBuffer.resize(BUFFER_SIZE);
	char* pBuffer = (char*)strBuffer.c_str();
	int index = 0;
	InitSocket();
	while (m_sock != INVALID_SOCKET) {
		if (m_lstSend.size() > 0) { //֤��������Ҫ����
			TRACE("m_lstSend size=%d\r\n", m_lstSend.size());
			m_lock.lock();
			CPacket& head = m_lstSend.front();
			m_lock.unlock();
			if (Send(head) == false) {
				TRACE("����ʧ��\r\n");
				//ԭ�������ڷ���˽���֮���CloseSocket���ٴη���ʱ�ͻ�ʧ�ܣ������������һ��
				continue;
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end()) {
				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClose.find(head.hEvent);
				do
				{
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					TRACE("recv.....=%d\r\n", BUFFER_SIZE - index);
					TRACE("length=%d\r\n", length);
					if ((length > 0) || (index > 0)) {
						index += length;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);
						if (size > 0) {
							//TODO:֪ͨ��Ӧ�¼�
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);  //ȥ���ù������ݶ�
							index -= size;
							TRACE("index=%d\r\n", index);
							if (it0->second == true) {
								SetEvent(head.hEvent);
								break;
							}
						}
					}
					else if (length <= 0 && index <= 0) {
						CloseSocket();
						SetEvent(head.hEvent);
						m_mapAutoClose.erase(it0);
						break;
						//�ȴ��������ر������֪ͨ�¼����
					}
				} while (it0->second == false);
			}
			m_lock.lock();
			m_lstSend.pop_front();
			m_lock.unlock();
			if (InitSocket() == false) {
				InitSocket();
			}
		}
		else {
			Sleep(1);
		}
		
	}
	CloseSocket();
}

bool CClientSocket::InitSocket()
{
	if (m_sock != INVALID_SOCKET)
		closesocket(m_sock);
	m_sock = socket(PF_INET, SOCK_STREAM, 0);  //ʹ��tcp
	sockaddr_in serv_adr;
	memset(&serv_adr, 0, sizeof(serv_adr));
	serv_adr.sin_family = AF_INET;
	serv_adr.sin_addr.s_addr = htonl(m_nIP);    //�������ֽ���ipת���������ֽ�����Ϊһ�������������ϴ�����ֽ��ǲ�һ���ģ��Ƿִ�С�˵�
	serv_adr.sin_port = htons(m_nPort);
	if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
		AfxMessageBox("ָ����ip��ַ������");
		return false;
	}
	if (connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
		AfxMessageBox("����ʧ��");
		TRACE("����ʧ�ܣ�%d %s\r\n", WSAGetLastError(), GetErrorInfo(WSAGetLastError()).c_str());   //����ʧ�ܵı�����Ϣ
		return false;
	}
	return true;
}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("m_sock:%d\r\n", m_sock);
	std::string strOut;
	pack.Data(strOut);
	TRACE("############cmd=%d\r\n", pack.sCmd);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0 ? true : false;
}
bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPack, bool isAutoclose) {
	if (m_sock == INVALID_SOCKET&&m_hThread==INVALID_HANDLE_VALUE) {
		//if (InitSocket() == false) return false;
		m_hThread=(HANDLE)_beginthread(CClientSocket::threadEntry, 0, this);
		TRACE("start thread\r\n");
	}
	m_lock.lock();
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>
		(pack.hEvent, lstPack));
	m_mapAutoClose.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoclose));
	m_lstSend.push_back(pack);
	m_lock.unlock();//��ֹ�����߳�ͬʱ����m_lstSend()
	TRACE("cmd=%d command=%08X threada id=%d\r\n", pack.sCmd, pack.hEvent,GetCurrentThreadId());
	WaitForSingleObject(pack.hEvent, INFINITE); //INFINITE���޵ȴ���ֱ���ȴ������
	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent);
	if (it != m_mapAck.end()) {
		m_lock.lock();
		m_mapAck.erase(it);
		m_lock.unlock();
		return true;
	}
	return false;
}