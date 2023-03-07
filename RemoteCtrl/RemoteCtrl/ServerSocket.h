#pragma once
#include "pch.h"
#include "framework.h"
#include "Packet.h"
#include<list>
#include "Tools.h"
typedef void(*SOCKET_CALLBACK)(void*,int, std::list<CPacket>&,CPacket&);  //����ص�����ָ��
class CServerSocket
{
public:
	static CServerSocket* getInstance() {
		if (m_instance == NULL) {  //��̬����û��thisָ��,�޷�ֱ�ӷ���˽�г�Ա����
			m_instance = new CServerSocket();
		}
		return m_instance;
	}  //���þ�̬���������ں����������˽�г�Ա����
	int Run(SOCKET_CALLBACK callback, void* arg, short port = 9527) {
		bool ret = InitSocket(port);
		if (ret == false) return -1;
		std::list<CPacket> listCPacket;
		m_callback = callback;
		m_arg = arg;
		int count = 0;
		while (true)
		{
			if (AcceptClient() == false) {
				if (count >= 3) return -2;
				count++;
			}
			int ret = DealCommand();
			if (ret > 0) {
				m_callback(m_arg, ret, listCPacket, m_packet);
				while (listCPacket.size() > 0) {//������Ҫ��
					Send(listCPacket.front());  //����
					listCPacket.pop_front(); //�ѷ����İ�����
				}
			}
			CloseClient();
		}

		return 0;
	}
protected:
	bool InitSocket(short port) {
		sockaddr_in serv_adr;
		memset(&serv_adr, 0, sizeof(serv_adr));
		serv_adr.sin_family = AF_INET;
		serv_adr.sin_addr.s_addr = INADDR_ANY;
		serv_adr.sin_port = htons(port);
		if (bind(m_sock, (sockaddr*)&serv_adr, sizeof(serv_adr)) == -1) 	return false;
		if (listen(m_sock, 1) == -1) return false;
		
		return true;
	}
	
	bool AcceptClient() {
		TRACE("AcceptClient called\r\n");
		//char buffer[1024];
		sockaddr_in client_adr;
		int cli_sz = sizeof(client_adr);
		m_client =accept(m_sock, (sockaddr*)&client_adr, &cli_sz);
		TRACE("m_client=%d\r\n", m_client);
		if (m_client == -1) return false;
		return true;
	}
#define BUFFER_SIZE 4096
	int DealCommand() {
		if (m_client == -1) return -1;
		//char buffer[1024]="";
		char* buffer = new char[BUFFER_SIZE];
		if (buffer == NULL) {
			TRACE("�ڴ治��\r\n");
			return -2;
		}
		memset(buffer, 0, BUFFER_SIZE);
		size_t index = 0;
		while (true) {
			size_t len = recv(m_client, buffer, BUFFER_SIZE-index, 0);
			if (len <= 0) {
				delete[]buffer;
				return -1;
			}
			TRACE("recv:%d\r\n", len);
			//TODO:��������
			index +=len;  //���len���յ����ݰ��ĳ���
			len = index;
			m_packet=CPacket((BYTE*)buffer, index);
			if (len > 0) {
				memmove(buffer, buffer + len, BUFFER_SIZE-len);  //���len��ʵ������Ч�����ݰ�����
				index -= len;
				delete[]buffer;
				return m_packet.sCmd;
			}
		}
		delete[]buffer;
		return -1;
	}
	bool Send(const char* pData, size_t nSize) {
		return send(m_client, pData, nSize, 0)>0?true:false;
	}
	bool Send(CPacket& pack) {
		//CTools::Dump((BYTE*)pack.Data(), pack.Size());
		return send(m_client, pack.Data(), pack.Size(), 0) > 0 ? true : false;
	}
	void CloseClient() {
		if (m_client != INVALID_SOCKET) {
			closesocket(m_client);
			m_client = INVALID_SOCKET;
		}
	}
private:
	SOCKET_CALLBACK m_callback=NULL;
	void* m_arg=NULL;
	SOCKET m_sock;
	SOCKET m_client;
	CPacket m_packet;
	CServerSocket& operator=(const CServerSocket& ss) {}
	CServerSocket(const CServerSocket& ss) {
		m_client = ss.m_client;
		m_sock = ss.m_sock;
	}
	CServerSocket() {
		m_sock = INVALID_SOCKET;   //INVALID_SOCKET	=-1
		m_client = INVALID_SOCKET;
		if (InitSockEnv() == FALSE) {
			MessageBox(NULL, _T("�޷���ʼ���׽��ֻ�����������������"), _T("��ʼ������!"), MB_OK | MB_ICONERROR);
			exit(0);
		}
		m_sock = socket(PF_INET, SOCK_STREAM, 0);  //ʹ��tcp
	}
	~CServerSocket() {
		closesocket(m_sock);
		WSACleanup();
	}
	BOOL InitSockEnv() {
		WSADATA data;
		if (WSAStartup(MAKEWORD(2, 0), &data) != 0) {
			return FALSE;
		}
		return TRUE;
	}
	static CServerSocket* m_instance;  //��̬����û��thisָ��,�޷�ֱ�ӷ���˽�г�Ա��������˽�m_instanceҲ����Ϊ��̬����
	static void releaseInstance()
	{
		if (m_instance != NULL) {
			//Ϊʲô��ֱ��delete m_instance��ԭ�򣺷����Ա�̣��Ƚ�m_instance��ֵ��ֵ��temp��֮��������m_instance�ÿ�
			//��Ϊ��delete��ʱ�򣬻���������������������������ִ���ڼ�m_instance�ֱ����ã�������õ����ڱ�������m_instance
			//�������Դ����߳��������������е��ȶ���
			CServerSocket* temp = m_instance;
			m_instance = NULL;
			delete temp;   //temp ָ��new�����Ķ���deleteʱ���������������
		}
	}
	class CHelper {
	public:
		CHelper() {
			CServerSocket::getInstance();
		}
		~CHelper() {
			CServerSocket::releaseInstance();
		}
	};
	static CHelper m_helper;   //����һ����̬���Ա�����ڵ����亯��
};
extern CServerSocket server;