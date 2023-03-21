#include "pch.h"
#include "MyNetwork.h"
CServer::CServer(MyServerParam& param):m_stop(false),m_arg(NULL)
{
	m_param = param;
	m_thread.UpdateWorker(ThreadWorker((void*)this,(FUNCTYPE)&CServer::threadFunc));
}

CServer::~CServer()
{
	Stop();
}

int CServer::Invoke(void* arg){
	m_sock.reset(new CMySocket(m_param.m_type));
	//SOCKET sock_server = socket(PF_INET, SOCK_DGRAM, 0);
	//MYSOCKET sock_server(new CMySocket(MyType::MyTypeUDP));
	if (*m_sock == INVALID_SOCKET) {
		TRACE("socket create failse the error code=%d\r\n", WSAGetLastError());
		printf("server %s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		closesocket(*m_sock);
		return -1;
	}
	if (m_param.m_type == MyType::MyTypeTCP) {//TCP需要监听
		if (m_sock->listen() == -1) {
			return -2;
		}
	}
	MySockaddr_in client_addr;
	//ucp与tcp的区别：没有listen  直接bind和recvfrom、sendto  整个过程都是操作自己的套接字
	//tcp使用recv和send   ucp使用recvfrom和sendto  
	if (m_sock->bind(m_param.m_IP, m_param.m_port) == -1) {
		printf("socket create failse the error code=%d\r\n", WSAGetLastError());
		printf("server %s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		return -3;
	}
	if (m_thread.Start() == -1) {
		return -4;
	}
	m_arg = arg;
	return 0;
}

int CServer::Send(MYSOCKET& client, const MyBuffer& buffer)
{
	int ret = m_sock->send(buffer);  //TODO:待优化  不一定每次都发送全部的buffer
	if (m_param.m_send) {
		m_param.m_send(m_arg, client, ret);
	}
	return ret;
}

int CServer::Sendto(MySockaddr_in& addr, const MyBuffer buffer)
{
	int ret = m_sock->sendto(buffer, addr);
	if (m_param.m_sendto) {
		m_param.m_sendto(m_arg, addr, ret);
	}
	return ret;
}

int CServer::Stop()
{
	if (m_stop == false){
		m_sock->Close();
		m_stop = true;
		m_thread.Stop();
	}
	return 0;
}

int CServer::threadFunc()
{
	if (m_param.m_type == MyType::MyTypeTCP) {
		return threadTCPFunc();
	}
	else {
		return threadUDPFunc();
	}
	
	return 0;
}

int CServer::threadUDPFunc()
{
	MyBuffer buffer(1024 * 256);
	MySockaddr_in client;
	int ret = 0;
	while (!m_stop) {
		ret = m_sock->recvfrom(buffer, client);
		if (ret > 0) {
			client.Update();
			if (m_param.m_recvfrom != NULL) {
				m_param.m_recvfrom(m_arg,buffer,client);
			}
		}
		else {
			printf("recvfrom failed %d  the error code=%d\r\n", __LINE__, WSAGetLastError());
			break;
		}
	}
	if (m_stop == false) {//结束
		m_stop = true;
	}
	m_sock->Close();
	return 0;
}

int CServer::threadTCPFunc()
{
	return 0;
}

MyServerParam::MyServerParam(const std::string& nIP, short nPort, MyType type, acceptFunc acceptfunc, recvFunc recvfunc, sendFunc sendfunc, recvfromFunc recvfromfunc, sendtoFunc sendtofunc)
{
	m_IP = nIP;
	m_port = nPort;
	m_type = type;
	m_accept = acceptfunc;
	m_recv = recvfunc;
	m_send = sendfunc;
	m_recvfrom = recvfromfunc;
	m_sendto = sendtofunc;
}

MyServerParam& MyServerParam::operator<<(acceptFunc func)
{
	m_accept = func;
	return *this;
}

MyServerParam& MyServerParam::operator<<(recvFunc func)
{
	m_recv = func;
	return *this;
}

MyServerParam& MyServerParam::operator<<(sendFunc func)
{
	m_send = func;
	return *this;

}

MyServerParam& MyServerParam::operator<<(recvfromFunc func)
{
	m_recvfrom = func;
	return *this;
}

MyServerParam& MyServerParam::operator<<(sendtoFunc func)
{
	m_sendto = func;
	return *this;
}

MyServerParam& MyServerParam::operator<<(std::string nIP)
{
	m_IP = nIP;
	return *this;
}

MyServerParam& MyServerParam::operator<<(short nPort)
{
	m_port = nPort;
	return *this;
}

MyServerParam& MyServerParam::operator<<(MyType type)
{
	m_type = type;
	return *this;
}

MyServerParam& MyServerParam::operator>>(acceptFunc& func)
{
	func = m_accept;
	return *this;

}

MyServerParam& MyServerParam::operator>>(recvFunc& func)
{
	// TODO: 在此处插入 return 语句
	func = m_recv;
	return *this;

}

MyServerParam& MyServerParam::operator>>(sendFunc& func)
{
	// TODO: 在此处插入 return 语句
	func = m_send;
	return *this;

}

MyServerParam& MyServerParam::operator>>(recvfromFunc& func)
{
	// TODO: 在此处插入 return 语句
	func = m_recvfrom;
	return *this;

}

MyServerParam& MyServerParam::operator>>(sendtoFunc& func)
{
	// TODO: 在此处插入 return 语句
	func = m_sendto;
	return *this;

}

MyServerParam& MyServerParam::operator>>(std::string& nIP)
{
	// TODO: 在此处插入 return 语句
	nIP = m_IP;
	return *this;

}

MyServerParam& MyServerParam::operator>>(short& nPort)
{
	// TODO: 在此处插入 return 语句
	nPort = m_port;
	return *this;

}

MyServerParam& MyServerParam::operator>>(MyType& type)
{
	// TODO: 在此处插入 return 语句
	type = m_type;
	return *this;

}

MyServerParam::MyServerParam(const MyServerParam& param)
{
	this->m_IP = param.m_IP;
	this->m_port = param.m_port;
	this->m_accept = param.m_accept;
	this->m_recv = param.m_recv;
	this->m_recvfrom = param.m_recvfrom;
	this->m_send = param.m_send;
	this->m_sendto = param.m_sendto;
	this->m_type = param.m_type;
}

MyServerParam& MyServerParam::operator=(const MyServerParam& param)
{
	// TODO: 在此处插入 return 语句
	this->m_IP = param.m_IP;
	this->m_port = param.m_port;
	this->m_accept = param.m_accept;
	this->m_recv = param.m_recv;
	this->m_recvfrom = param.m_recvfrom;
	this->m_send = param.m_send;
	this->m_sendto = param.m_sendto;
	this->m_type = param.m_type;
	return *this;
}
