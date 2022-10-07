#pragma once
#include "pch.h"
#include "framework.h"
#include <string>
#include <vector>
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
	CPacket(const BYTE* pData, size_t& nSize) {
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
	const char* Data() {
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
	std::string strOut; //整个包的数据保存在strOut中
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
std::string GetErrorInfo(int wsaErrorCode);
void Dump(BYTE* pData, size_t nSize);
class CClientSocket
{
public:
	static CClientSocket* getInstance() {
		if (m_instance == NULL) {  //静态函数没有this指针,无法直接访问私有成员变量
			m_instance = new CClientSocket();
		}
		return m_instance;
	}  //设置静态函数，用于后续调用类的私有成员函数
	bool InitSocket(int nIP,int nPort) {
		if (m_sock != INVALID_SOCKET)
			closesocket(m_sock);
		m_sock = socket(PF_INET, SOCK_STREAM, 0);  //使用tcp
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = htonl(nIP);    //将主机字节序ip转换成网络字节序，因为一般主机跟网络上传输的字节是不一样的，是分大小端的
		serv_adr.sin_port = htons(nPort);
		if (serv_adr.sin_addr.s_addr == INADDR_NONE) {
			AfxMessageBox("指定的ip地址不存在");
			return false;
		}
		if (connect(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) {
			AfxMessageBox("连接失败");
			TRACE("连接失败：%d %s\r\n", WSAGetLastError(),GetErrorInfo(WSAGetLastError()).c_str());   //连接失败的报错信息
			return false;
		}
		return true;
	}
#define BUFFER_SIZE 409600
	int DealCommand() {
		if (m_sock == -1) return -1;
		//char buffer[1024]="";
		char* buffer = m_buffer.data();
		static size_t index = 0;
		while (true) {
			size_t len = recv(m_sock, buffer+index, BUFFER_SIZE - index, 0);
			if ((len <= 0)&&(index<=0)) return -1;
			Dump((BYTE*)buffer, index);
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
	bool Send(const char* pData, size_t nSize) {
		return send(m_sock, pData, nSize, 0) > 0 ? true : false;
	}
	bool Send(CPacket& pack) {
		TRACE("m_sock:%d\r\n", m_sock);
		return send(m_sock, pack.Data(), pack.Size(), 0) > 0 ? true : false;
	}
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
private:
	std::vector<char> m_buffer;
	SOCKET m_sock;
	CPacket m_packet;
	CClientSocket& operator=(const CClientSocket& ss) {}
	CClientSocket(const CClientSocket& ss) {
		m_sock = ss.m_sock;
	}
	CClientSocket() {
		m_sock = INVALID_SOCKET;   //INVALID_SOCKET	=-1
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("无法初始化套接字环境，请检查网络设置"), _T("初始化错误!"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_buffer.resize(BUFFER_SIZE);
		memset(m_buffer.data(), 0, BUFFER_SIZE);

	}
	~CClientSocket() {
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
	static CClientSocket* m_instance;  //静态函数没有this指针,无法直接访问私有成员变量，因此将m_instance也设置为静态变量
	static void releaseInstance()
	{
		if (m_instance != NULL) {
			CClientSocket* temp = m_instance;
			m_instance = NULL;
			delete temp;   //temp 指向new出来的对象，delete时会调用其析构函数
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

