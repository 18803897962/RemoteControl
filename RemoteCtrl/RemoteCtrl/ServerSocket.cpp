#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server;
CServerSocket* CServerSocket::m_instance = NULL;  //��m_instance�ÿ�

CServerSocket* pserver = CServerSocket::getInstance();   //ȫ�ֱ�����ȫ�ֱ�����main����ִ��֮ǰ���Ѿ�����

CServerSocket::CHelper CServerSocket::m_helper;
