#include "pch.h"
#include "ClientSocket.h"

//CClientSocket server;
CClientSocket* CClientSocket::m_instance = NULL;  //��m_instance�ÿ�
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstance();   //ȫ�ֱ�����ȫ�ֱ�����main����ִ��֮ǰ���Ѿ�����
CClientSocket::CClientSocket() :m_nIP(INADDR_ANY), m_nPort(0), m_sock(INVALID_SOCKET), m_isAutoClose(true), m_hThread(INVALID_HANDLE_VALUE) {
	m_sock = INVALID_SOCKET;   //INVALID_SOCKET	=-1
	struct {
		UINT nMsg;
		MSGFUNC msgFunc;
	}funcs[] = {
		{WM_SEND_PACK,&CClientSocket::SendPack},
		{0,NULL}
	};
	for (int i = 0; funcs[i].msgFunc != NULL; i++) {
		if (m_mapFunc.insert(std::pair<UINT, MSGFUNC>(funcs[i].nMsg, funcs[i].msgFunc)).second == false) {
			TRACE("����ʧ�ܣ���Ϣֵ��%d ������%08X\r\n", funcs[i].nMsg, funcs[i].msgFunc);
		}
	}
	if (InitSockEnv() == FALSE) {
		MessageBox(NULL, _T("�޷���ʼ���׽��ֻ�����������������"), _T("��ʼ������!"), MB_OK | MB_ICONERROR);
		exit(0);
	}
	m_buffer.resize(BUFFER_SIZE);
	memset(m_buffer.data(), 0, BUFFER_SIZE);
}
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
unsigned CClientSocket::threadEntry(void* arg) {
	CClientSocket* thiz = (CClientSocket*)arg;
	thiz->threadFunc2();
	_endthreadex(0);
	return 0;
}
/*
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
						if(it0 != m_mapAutoClose.end()){
							TRACE("setEvent %d %d\r\n", head.sCmd, it0->second);
						}
						else
						{
							TRACE("�����쳣��û�ж�Ӧ��pair\r\n");
						}
						break;
						//�ȴ��������ر������֪ͨ�¼����
					}
				} while (it0->second == false);
			}
			m_lock.lock();
			m_lstSend.pop_front();
			m_mapAutoClose.erase(head.hEvent);
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
*/
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

void CClientSocket::SendPack(UINT nMsg, WPARAM wParam, LPARAM lParam) {
//TODO:����һ����Ϣ�����ݽṹ�����ݳ��ȡ�ģʽ�����ݣ�  ����һ���ص���Ϣ�����ݽṹ(���ڵľ��HAND)  �ص�ʲô��Ϣ
	PACKET_DATA data = *(PACKET_DATA*)wParam;  //������,pData���浽�ֲ�����ֹ����ʹ��wParamʱ�����ڴ�й©�����Ա���֮��ֱ��delete��
	delete (PACKET_DATA*)wParam;
	HWND hWnd = (HWND)lParam;
	if (InitSocket() == true) {
		TRACE("m_sock: % d\r\n", m_sock);
		int ret = send(m_sock, (char*)data.strData.c_str(), (int)data.strData.size(), 0); //�������ݰ�
		if (ret > 0) {
			size_t index = 0;
			std::string strBuffer;
			strBuffer.resize(BUFFER_SIZE);
			char* pBuffer = (char*)strBuffer.c_str();
			while (m_sock != INVALID_SOCKET) {
				int length = recv(m_sock, pBuffer+index, BUFFER_SIZE-index, 0);
				if (length > 0||index>0) {//length>0˵��recv�����ݳ��ȴ���0��index����0��ʾ�������л�������
					index += (size_t)length;
					size_t nLen = index;
					CPacket pack((BYTE*)pBuffer, nLen);
					if (nLen > 0) {//����ɹ�
						::SendMessage(hWnd, WM_SEND_PACK_ACK, (WPARAM)new CPacket(pack), data.wParam);
						if (data.nMod & CSM_AUTOCLOSE) {  //�Զ��رգ����������ʾ����һ���Խ�����ɣ�����ֱ�ӷ���Ӧ��
							CloseSocket();
							return;
						}
						
					}
					index -= length;
					memmove(pBuffer, pBuffer + index, nLen);

				}
				else {//�������,�Է��ر����׽��ֻ��������豸�쳣
					CloseSocket();
					::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, 1);
				}
				
			}
		}
		else {
			CloseSocket();
			::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -1);
			//������ֹ����
		}
	}
	else { //��ʼ��ʧ�ܣ���һ���հ�
		//TODO:
		::SendMessage(hWnd, WM_SEND_PACK_ACK, NULL, -2);
	}
}



void CClientSocket::threadFunc2()  //ͨ��������Ϣ������һϵ�еķ�Ӧ
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (m_mapFunc.find(msg.message) != m_mapFunc.end()) {
			(this->*m_mapFunc[msg.message])(msg.message,msg.wParam,msg.lParam);//����ָ�����
		}
	}
}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("m_sock:%d\r\n", m_sock);
	std::string strOut;
	pack.Data(strOut);
	TRACE("############cmd=%d\r\n", pack.sCmd);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0 ? true : false;
}
bool CClientSocket::SendPacket(HWND hWnd, const CPacket& pack, bool isAutoclose,WPARAM wParam) {
	if (m_hThread == INVALID_HANDLE_VALUE) {
		m_hThread = (HANDLE)_beginthreadex(NULL, 0, &CClientSocket::threadEntry, this, 0, &m_nThreadID);
	}
	UINT nMode = isAutoclose?CSM_AUTOCLOSE:0;
	std::string strOut;
	pack.Data(strOut);
	return PostThreadMessage(m_nThreadID, WM_SEND_PACK, (WPARAM)new PACKET_DATA(strOut.c_str(),strOut.size(), nMode,wParam), (LPARAM)hWnd);//ʧ�ܷ���0
}
/*
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
*/