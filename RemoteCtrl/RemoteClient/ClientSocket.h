#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
#include <list>
#include <map>
#include <mutex>
#define WM_SEND_PACK (WM_USER+1) //发送包数据
#define WM_SEND_PACK_ACK (WM_USER+2) //发送数据包应答
#pragma pack(push)
#pragma pack(1)
class CPacket   //声明数据包的类
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
		size_t i = 0;   //数据段的偏移量
		for (; i < nSize; i++) {
			if (*(WORD*)(pData + i) == 0xFEFF) {
				sHead = *(WORD*)(pData + i);
				i += 2;
				break;
			}
		}
		if (i + 4 + 2 + 2 > nSize) {  //包数据可能不全，或包头未能全部接收，解析失败
			nSize = 0;
			return;
		}
		nLength = *(DWORD*)(pData + i); i += 4;
		if (nLength + i > nSize) {  //避免只解析一半的情况，包未完全接收到就返回，解析失败
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
		WORD sum = 0;  //进行校验
		for (size_t j = 0; j < strData.size(); j++) {
			sum += BYTE(strData[j]) & 0xFF;
		}
		if (sum == sSum) { //解析成功
			nSize = i;  //此时的i是包的实际长度
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
	int Size() {  //获得包数据的大小
		return nLength + 6;  //nLength是包长度
	}
	const char* Data(std::string& strOut) const{
		strOut.resize(nLength + 6);
		BYTE* pData = (BYTE*)strOut.c_str();  //定义一个指针，用于后续修改字符串内容
		*(WORD*)pData = sHead; pData += 2;
		*(DWORD*)pData = nLength; pData += 4;
		*(WORD*)pData = sCmd; pData += 2;
		memcpy(pData, strData.c_str(), strData.size()); pData += strData.size();
		*(WORD*)pData = sSum;
		return strOut.c_str();
	}
public:
	WORD sHead;  //包头，固定位FEFF
	DWORD nLength;  //包长度，控制命令开始到和校验结束
	WORD sCmd; //控制命令
	std::string strData;  //数据
	WORD sSum;  //和校验
};

#pragma pack(pop)

typedef struct MouseEvent {
	MouseEvent() {//默认构造函数
		nAction = 0;
		nButton = -1;
		ptXY.x = 0;
		ptXY.y = 0;
	}
	WORD nAction;//动作，包括点击、移动、双击
	WORD nButton;//左键、右键、滚轮
	POINT ptXY;//坐标
}MOUSEEV, * PMOUSEEV;
typedef struct file_info
{
	file_info() { //构造函数
		IsInvalid = FALSE;
		IsDirectory = -1;
		HasNext = TRUE;
		memset(szFileName, 0, sizeof(szFileName));
	}
	BOOL IsInvalid; //是否有效
	BOOL IsDirectory;//是否为目录 0否 1是
	BOOL HasNext; //是否含有后续文件 0没有 1有  用于实现找到一个发送一个
	char szFileName[260];
}FILEINFO, * PFILEINFO;

enum {
	CSM_AUTOCLOSE = 1  //CSM===Client Socket Mode,为1表示自动关闭
};

typedef struct PacketData
{
	std::string strData;//包数据
	UINT nMod;//模式
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
		if (m_instance == NULL) {  //静态函数没有this指针,无法直接访问私有成员变量
			m_instance = new CClientSocket();
			TRACE("CClientSocket size =%d\r\n", sizeof(CClientSocket));
		}
		return m_instance;
	}  //设置静态函数，用于后续调用类的私有成员函数
	bool InitSocket();
#define BUFFER_SIZE 4096000
	int DealCommand() {
		if (m_sock == -1) return -1;
		char* buffer = m_buffer.data();  
		//多线程发送命令，可能在发送图片时，图片由tcp分为多个包，导致图片接收不完全，被鼠标消息数据中断
		static size_t index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer+index, BUFFER_SIZE - index, 0);
			if (((int)len <= 0)&&((int)index<=0)) return -1;
			//TODO:处理命令
			index += len;  //这个len是收到数据包的长度
			len = index;
			m_packet = CPacket((BYTE*)buffer, len);
			if (len > 0) {
				memmove(buffer, buffer + len, index - len);  //这个len是实际上有效的数据包长度
				index -= len;
				return m_packet.sCmd;
			}
		}
		return -1;
	}
	
	//bool SendPacket(const CPacket& pack, std::list<CPacket>& lstPack, bool isAutoclose = true);
	bool SendPacket(HWND hWnd/*数据包收到后需要应答的窗口*/, const CPacket& pack, bool isAutoclose = true, WPARAM wParam=0);
	bool GetFilePath(std::string& strPath) {
		if ((m_packet.sCmd >= 2) && (m_packet.sCmd <= 4)) {  //当前命令为获取文件列表时，此时数据段strData为所需路径
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
	HANDLE m_EventInvoke;//启动事件
	UINT m_nThreadID;
	typedef void(CClientSocket::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	std::map<UINT, MSGFUNC> m_mapFunc;
	HANDLE m_hThread;
	std::mutex m_lock;
	std::list<CPacket> m_lstSend;  //要发送的数据
	std::map<HANDLE, std::list<CPacket>&> m_mapAck; //命令，应答的包 选用list的原因是，list的效率受包的大小影响较小，而vector的效率受影响较大
	DWORD m_nIP;  //记录IP
	int m_nPort; //记录端口
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
	void SendPack(UINT nMsg, WPARAM wParam/*缓冲区的值*/, LPARAM lParam/*缓冲区的长度*/);
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
	static CClientSocket* m_instance;  //静态函数没有this指针,无法直接访问私有成员变量，因此将m_instance也设置为静态变量
	static void releaseInstance()
	{
		if (m_instance != NULL) {
			TRACE("CClientSocket delete called\r\n");
			CClientSocket* temp = m_instance;
			m_instance = NULL;
			delete temp;   //temp 指向new出来的对象，delete时会调用其析构函数
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
	static CHelper m_helper;   //声明一个静态类成员，用于调用其函数
};
extern CClientSocket server;

