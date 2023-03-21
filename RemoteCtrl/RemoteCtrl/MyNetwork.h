#pragma once
#include "MySocket.h"
#include "MyThread.h"
/*
* 1 Ū������Ĺ�����ʲô
* 2 ҵ���߼���ʲô
*/
class CMyNetwork
{

};

typedef int (*acceptFunc) (void* arg,MYSOCKET& client);
typedef int (*recvFunc) (void* arg, const MyBuffer& buffer);
typedef int (*sendFunc) (void* arg,const MYSOCKET& client,int ret);
typedef int (*recvfromFunc) (void* arg, const MyBuffer& buffer, MySockaddr_in& addr);
typedef int (*sendtoFunc) (void* arg,const MySockaddr_in& addr,int ret);


class MyServerParam {
public:
	MyServerParam(const std::string& nIP, short nPort, MyType type);
	MyServerParam(
		const std::string& nIP = "0.0.0.0",
		short nPort = 9527,
		MyType type = MyType::MyTypeTCP,
		acceptFunc acceptfunc = NULL,
		recvFunc recvfunc = NULL,
		sendFunc sendfunc = NULL,
		recvfromFunc recvfromfunc = NULL,
		sendtoFunc sendtofunc = NULL
	);
	//����
	MyServerParam& operator<<(acceptFunc func);
	MyServerParam& operator<<(recvFunc func);
	MyServerParam& operator<<(sendFunc func);
	MyServerParam& operator<<(recvfromFunc func);
	MyServerParam& operator<<(sendtoFunc func);
	MyServerParam& operator<<(std::string nIP);
	MyServerParam& operator<<(short nPort);
	MyServerParam& operator<<(MyType type);
	//���
	MyServerParam& operator>>(acceptFunc& func);
	MyServerParam& operator>>(recvFunc& func);
	MyServerParam& operator>>(sendFunc& func);
	MyServerParam& operator>>(recvfromFunc& func);
	MyServerParam& operator>>(sendtoFunc& func);
	MyServerParam& operator>>(std::string& nIP);
	MyServerParam& operator>>(short& nPort);
	MyServerParam& operator>>(MyType& type);
	//���ƹ���͵Ⱥ����أ�����ͬ���͸�ֵ
	MyServerParam(const	MyServerParam& param);
	MyServerParam& operator=(const MyServerParam& param);
	std::string m_IP;
	short m_port;
	MyType m_type;
	acceptFunc m_accept;
	recvFunc m_recv;
	sendFunc m_send;
	recvfromFunc m_recvfrom;
	sendtoFunc m_sendto;
};

class CServer :public ThreadFuncBase {
public:
	CServer(MyServerParam& param);//��ʱ���ùؼ�������ʵ������֮��Ҫ���е���
	~CServer();
	int Invoke(void* arg);
	int Send(MYSOCKET& client,const MyBuffer& buffer);
	int Sendto(MySockaddr_in& addr,const MyBuffer buffer);
	int Stop();
private:
	int threadFunc();
	int threadUDPFunc();
	int threadTCPFunc();
private:
	MyServerParam m_param;
	CMyThread m_thread;
	MYSOCKET m_sock;
	std::atomic<bool> m_stop;
	void* m_arg;
};