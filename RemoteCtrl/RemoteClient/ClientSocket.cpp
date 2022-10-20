#include "pch.h"
#include "ClientSocket.h"

//CClientSocket server;
CClientSocket* CClientSocket::m_instance = NULL;  //将m_instance置空
CClientSocket::CHelper CClientSocket::m_helper;

CClientSocket* pclient = CClientSocket::getInstance();   //全局变量，全局变量在main函数执行之前就已经产生

std::string GetErrorInfo(int wsaErrorCode) {  //设置geterrornum的错误原因返回函数
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
		if (m_lstSend.size() > 0) { //证明有数据要发送
			TRACE("m_lstSend size=%d\r\n", m_lstSend.size());
			CPacket& head = m_lstSend.front();
			if (Send(head) == false) {
				TRACE("发送失败\r\n");
				//原因是由于服务端接收之后会CloseSocket，再次发送时就会失败，因此重新连接一次
				continue;
			}
			std::map<HANDLE, std::list<CPacket>&>::iterator it;
			it = m_mapAck.find(head.hEvent);
			if (it != m_mapAck.end()) {
				std::map<HANDLE, bool>::iterator it0 = m_mapAutoClose.find(head.hEvent);
				do
				{
					int length = recv(m_sock, pBuffer + index, BUFFER_SIZE - index, 0);
					if (length > 0 || index > 0) {
						index += length;
						size_t size = (size_t)index;
						CPacket pack((BYTE*)pBuffer, size);
						if (size > 0) {
							//TODO:通知对应事件
							pack.hEvent = head.hEvent;
							it->second.push_back(pack);
							memmove(pBuffer, pBuffer + size, index - size);  //去掉用过的数据段
							index -= size;
							if (it0->second == true) {
								SetEvent(head.hEvent);
							}
						}
					}
					else if (length <= 0 && index <= 0) {
						CloseSocket();
						SetEvent(head.hEvent);
						m_mapAutoClose.erase(it0);
						break;
						//等待服务器关闭命令，再通知事件完成
					}
				} while (it0->second == false);
			}
			m_lstSend.pop_front();
			if (InitSocket() == false) {
				InitSocket();
			}
		}
		
	}
	CloseSocket();
}

bool CClientSocket::Send(const CPacket& pack)
{
	TRACE("m_sock:%d\r\n", m_sock);
	std::string strOut;
	pack.Data(strOut);
	return send(m_sock, strOut.c_str(), strOut.size(), 0) > 0 ? true : false;
}
bool CClientSocket::SendPacket(const CPacket& pack, std::list<CPacket>& lstPack, bool isAutoclose) {
	if (m_sock == INVALID_SOCKET) {
		//if (InitSocket() == false) return false;
		_beginthread(CClientSocket::threadEntry, 0, this);
	}
	auto pr = m_mapAck.insert(std::pair<HANDLE, std::list<CPacket>&>
		(pack.hEvent, lstPack));
	m_mapAutoClose.insert(std::pair<HANDLE, bool>(pack.hEvent, isAutoclose));
	m_lstSend.push_back(pack);
	TRACE("cmd=%d command=%08X threada id=%d\r\n", pack.sCmd, pack.hEvent,GetCurrentThreadId());
	WaitForSingleObject(pack.hEvent, INFINITE); //INFINITE无限等待，直到等待出结果
	std::map<HANDLE, std::list<CPacket>&>::iterator it;
	it = m_mapAck.find(pack.hEvent);
	if (it != m_mapAck.end()) {

		m_mapAck.erase(it);
		return true;
	}
	return false;
}