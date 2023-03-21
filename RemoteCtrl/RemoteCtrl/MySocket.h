#pragma once
#include <WinSock2.h>
#include <Memory>

class MySockaddr_in
{
public:
	MySockaddr_in() {
		memset(&m_addr, 0, sizeof(m_addr));
		m_Port = -1;
	}
	MySockaddr_in(UINT nIP, short nPort) {
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = htonl(nIP);
		m_Port = nPort;
		m_IP = inet_ntoa(m_addr.sin_addr);
	}
	MySockaddr_in(const std::string& strIP, short nPort) {
		m_IP = strIP;
		m_Port = nPort;
		m_addr.sin_family = AF_INET;
		m_addr.sin_port = htons(nPort);
		m_addr.sin_addr.s_addr = inet_addr(strIP.c_str());
	}
	MySockaddr_in(const sockaddr_in& addr) {
		memcpy(&m_addr, &addr, sizeof(addr));
		m_IP = inet_ntoa(m_addr.sin_addr);
		m_Port = m_addr.sin_port;
	}
	MySockaddr_in(const MySockaddr_in& addr) {
		memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
		m_IP = addr.m_IP;
		m_Port = addr.m_Port;
	}
	MySockaddr_in& operator=(const MySockaddr_in& addr) {
		if (this != &addr) {
			memcpy(&m_addr, &addr.m_addr, sizeof(m_addr));
			m_IP = addr.m_IP;
			m_Port = addr.m_Port;
		}
		return *this;
	}
	operator sockaddr* () {
		return (sockaddr*)&m_addr;
	}
	operator const sockaddr* () const {
		return (sockaddr*)&m_addr;
	}
	operator void* () {
		return (void*)&m_addr;
	}
	void Update() {
		m_IP = inet_ntoa(m_addr.sin_addr);
		m_Port = m_addr.sin_port;
	}
	std::string GetIP() {
		return m_IP;
	}
	short GetPort() {
		return m_Port;
	}
	inline size_t Size() const {
		return sizeof(sockaddr_in);
	}
private:
	sockaddr_in m_addr;
	std::string m_IP;
	short m_Port;
};

class MyBuffer:public std::string {
public:
	MyBuffer(size_t size = 0) :std::string() {//调用string的构造函数
		if (size > 0) {
			resize(size);
			memset(this, 0, size);
		}
	}
	MyBuffer(void* buffer, size_t size):std::string() {
		memcpy((void*)c_str(), buffer, size);
	}
	MyBuffer(const char* str) {
		resize(strlen(str));
		memcpy((void*)c_str(), str, strlen(str));
	}
	~MyBuffer() {
		std::string::~basic_string();
	}
	operator char* () const {
		return (char*)c_str();
	}
	operator const char* () const {
		return (char*)c_str();
	}
	operator BYTE* () const {
		return (BYTE*)c_str();
	}
	operator void* () const {
		return (void*)c_str();
	}
	void Update(void* buffer,size_t size) {
		resize(size);
		memcpy((void*)c_str(), buffer, size);
	}
};


enum class MyType{//支持tcp udp
	MyTypeTCP=1,
	MyTypeUDP//2
};
class CMySocket
{
public:
	CMySocket(MyType nType = MyType::MyTypeTCP, int nProtocol = 0) {
		m_socket = socket(PF_INET, (int)nType, nProtocol);
		m_type = nType;
		m_procotol = nProtocol;
	}
	CMySocket(const CMySocket& sock) {
		m_socket = socket(PF_INET, (int)sock.m_type, sock.m_procotol);
		m_type = sock.m_type;
		m_procotol = sock.m_procotol;
	}
	~CMySocket() {
		Close();
	}
	CMySocket& operator=(const CMySocket& sock) {
		if (*this != sock) {
			m_socket = socket(PF_INET, (int)sock.m_type, sock.m_procotol);
			m_type = sock.m_type;
			m_procotol = sock.m_procotol;
			m_addr = sock.m_addr;
		}
		return *this;
	}
	operator SOCKET()const{
		return m_socket;
	}
	operator SOCKET(){
		return m_socket;
	}
	bool operator==(SOCKET sock) const {
		return m_socket == sock;
	}
	int listen(int backlog = 5) {
		if (m_type != MyType::MyTypeTCP) {//TCP需要监听，ucp不需要监听
			return -1;
		}
		return ::listen(m_socket, backlog);
	}
	int bind(const std::string& IP,short nPort) {
		m_addr = MySockaddr_in(IP, nPort);
		return ::bind(m_socket, m_addr, m_addr.Size());
	}
	int accept() {}
	int connect() {}
	int send(const MyBuffer& buffer){
		return ::send(m_socket,buffer,buffer.size(),0);
	}
	int recv(MyBuffer& buffer) {
		return ::recv(m_socket,buffer,buffer.size(),0);
	}
	int sendto(const MyBuffer& buffer,const MySockaddr_in& to) {
		return ::sendto(m_socket, buffer, buffer.size(), 0, to, to.Size());
	};
	int recvfrom(MyBuffer& buffer,MySockaddr_in& from) {
		int len = from.Size();
		int ret=::recvfrom(m_socket,buffer,buffer.size(),0,from,&len);
		if (ret > 0) {
			from.Update();//将收到的客户端信息更新
		}
		return ret;
	}
	void Close() {
		if (m_socket != INVALID_SOCKET) {
			closesocket(m_socket);
			m_socket = INVALID_SOCKET;
		}
	}
private:
	MySockaddr_in m_addr;
	SOCKET m_socket;
	MyType m_type;
	int m_procotol;
};

typedef std::shared_ptr<CMySocket> MYSOCKET;

