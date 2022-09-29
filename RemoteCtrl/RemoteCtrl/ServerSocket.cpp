#include "pch.h"
#include "ServerSocket.h"

//CServerSocket server;
CServerSocket* CServerSocket::m_instance = NULL;  //将m_instance置空

CServerSocket* pserver = CServerSocket::getInstance();   //全局变量，全局变量在main函数执行之前就已经产生

CServerSocket::CHelper CServerSocket::m_helper;
