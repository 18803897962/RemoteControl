// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Tools.h"
#include "Command.h"
#include <conio.h>
#include "Queue.h"
#include <MSWsock.h>
#include "MyServer.h"
#ifdef _DEBUG
#define new DEBUG_NEW
#endif
// 唯一的应用程序对象
//#define INVOKEPATH (_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe")) //注册表
#define INVOKEPATH _T("C:\\Users\\edoyun\\AppData\\Roaming\\Microsoft\\Windows\\Start Menu\\Programs\\Startup\\RemoteCtrl.exe")
CWinApp theApp;
using namespace std;
/*
* 改bug的思路
* 0 观察现象
* 1 确定范围
* 2 分析出错误原因的可能性
* 3 调试或打日志排查错误
* 4 处理错误
* 5 验证、长时间验证、多次验证、多条件验证
*/
bool ChooseAutoInvoke(const CString& strPath) {//自动启动
    if (PathFileExists(strPath)) {  //如果文件已经存在，就返回
        return true;
    }
    CString strInfo = _T("该程序只允许用于合法用途！\n");
    strInfo += _T("继续运行该程序将使得该机器处于被监控状态！\n");
    strInfo += _T("如果你不希望，请按取消按钮退出程序。\n");
    strInfo += _T("按下“是”按钮，该程序将被复制到你的机器，随系统自动运行。\n");
    strInfo += _T("按下“否”按钮，该程序值运行依次，将不会在系统中残留。\n");
    int ret = MessageBox(NULL, strInfo, _T("警告"), MB_YESNOCANCEL | MB_ICONWARNING | MB_TOPMOST);
    if (ret == IDYES) {
		if (CTools::WriteRegisterTable(strPath)==false) {//注册表方式开机自启
			::exit(0);
		}
		if (CTools::WriteStartup(strPath) == FALSE) {//添加到开机自启文件夹做法
			MessageBox(NULL, _T("复制文件失败，是否权限不足？"), _T("发生错误"), MB_ICONERROR | MB_TOPMOST);
			return false;
		}
    }
    else if (ret == IDCANCEL) {
        return false;
    }
    return true;
}
void iocp();
void udpServer();
void udpClient(bool isHost = true);
void initSock();
void clearSock();
//int wmain(int argc, TCHAR* argv[]) {}
int main(int argc,char* argv[])
{/*
	if (CTools::IsAdmin()) {  //管理员
		if (!CTools::init()) return 1;
		//OutputDebugString(L"current is run as administrator!\r\n");
		if (ChooseAutoInvoke(INVOKEPATH)) {
			CCommand cmd;
			int ret = CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
			switch (ret)
			{
			case -1:
				MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
				break;
			case -2:
				MessageBox(NULL, _T("多次无法接入用户，结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
				break;
			}
		}
	}
	else {
		//OutputDebugString(L"current is run as normal user!\r\n");//获取管理员权限，然后使用管理员权限创建进程
        if (CTools::RunAsAdmin()==false) {
            CTools::ShowError();
			return 1;
        }
	}*/

	if (!CTools::init()) return 1;//完成端口映射
	initSock();
	if (argc == 1) {//主程序启动
		CHAR wstrDir[MAX_PATH];
		GetCurrentDirectoryA(MAX_PATH,wstrDir);
		STARTUPINFOA si;
		memset(&si, 0, sizeof(si));
		PROCESS_INFORMATION pi;
		memset(&pi, 0, sizeof(pi));
		string strCmd = argv[0];
		strCmd += " 1";
		BOOL bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
		if (bRet == TRUE) {
			CloseHandle(pi.hThread);
			CloseHandle(pi.hProcess);
			TRACE("process id=%d\r\n", pi.dwProcessId);
			TRACE("thread id=%d\r\n", pi.dwThreadId);
			strCmd += " 2";
			bRet = CreateProcessA(NULL, (LPSTR)strCmd.c_str(), NULL, NULL, FALSE, 0, NULL, wstrDir, &si, &pi);
			if (bRet == TRUE) {
				CloseHandle(pi.hThread);
				CloseHandle(pi.hProcess);
				TRACE("process id=%d\r\n", pi.dwProcessId);
				TRACE("thread id=%d\r\n", pi.dwThreadId);
				udpServer();//服务器代码
			}
		}
		
	}
	else if (argc == 2) {//主客户端
		udpClient(true);  
	}
	else {
		udpClient(false); //从客户端
	}
	clearSock();
	/*
	HANDLE hIOCP = INVALID_HANDLE_VALUE;//IOCP IO Completion PORT
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);//1表示能够同时访问的线程数 设置为1，只有一个线程来处理队列 可以由多个线程需要该线程处理消息队列
	if (hIOCP == INVALID_HANDLE_VALUE||hIOCP==NULL) {
		printf("create iocp failed %d\n", GetLastError());
	}
	HANDLE hThread=(HANDLE) _beginthread(threadQueEntry, 0, hIOCP);
	DWORD tick = GetTickCount64();
	DWORD tick0 = GetTickCount64();
	while (_kbhit()==0) {
		if (GetTickCount64() - tick0 > 130) {
			PostQueuedCompletionStatus(hIOCP,sizeof(IOCP_Param), (ULONG_PTR)new IOCP_Param(iocp_list_pop, "hello world",func), NULL);
			tick0 = GetTickCount64();
		}
		if (GetTickCount64() - tick > 200) {
			PostQueuedCompletionStatus(hIOCP,sizeof(IOCP_Param), (ULONG_PTR)new IOCP_Param(iocp_list_push,"hello world"), NULL);
			tick = GetTickCount64();
		}
		Sleep(1);
	}
	if (hIOCP != INVALID_HANDLE_VALUE) {
		//TODO:唤醒完成端口 线程需要结束
		PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_Param), (ULONG_PTR)new IOCP_Param(iocp_list_empty, "hello world"), NULL);//传递一个状态
		PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);
		WaitForSingleObject(hThread,INFINITE);//等待线程结束
	}
	printf("exit\n");
	CloseHandle(hIOCP);
	::exit(0);*/
	//iocp();
    return 0;
}

class COverlapped
{
public:
	OVERLAPPED m_overlapped;
	DWORD m_operator;
	char m_buffer[4096];
	COverlapped() {
		m_operator = 0;
		memset(&m_overlapped, 0, sizeof(OVERLAPPED));
		memset(m_buffer, 0, sizeof(m_buffer));
	}
	~COverlapped() {}

private:

};
void iocp() {
	/*
	SOCKET sock = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);//重叠结构套接字使用
	if (sock = INVALID_SOCKET) {
		CTools::ShowError();
		return;
	}
	HANDLE hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, sock, 4);  //iocp
	SOCKET client = WSASocket(AF_INET, SOCK_STREAM, 0, NULL, 0, WSA_FLAG_OVERLAPPED);
	CreateIoCompletionPort((HANDLE)sock, hIOCP, 0, 0);

	sockaddr_in addr;
	addr.sin_family = PF_INET;
	addr.sin_addr.s_addr = inet_addr("0.0.0.0");
	addr.sin_port = htons(9527);
	bind(sock,(sockaddr*)&addr,sizeof(addr));
	listen(sock,5);

	COverlapped Overlapped;
	Overlapped.m_operator = 1;//accept
	//memset(&Overlapped, 0, sizeof(OVERLAPPED));
	DWORD received = 0;
	if (AcceptEx(sock, client, Overlapped.m_buffer, 0, sizeof(sockaddr_in) + 16, sizeof(sockaddr_in) + 16, &received, &Overlapped.m_overlapped) == FALSE) {
		CTools::ShowError();
	}

	while (true) {//代表一个线程
		LPOVERLAPPED pOverlapped = NULL;
		DWORD transferred = 0;
		DWORD key = 0;
		if (GetQueuedCompletionStatus(hIOCP, &transferred, &key, &pOverlapped, INFINITY)) {
			COverlapped* po = CONTAINING_RECORD(pOverlapped,COverlapped,m_overlapped);
			switch (po->m_operator)
			{
			case 1://accept

				
			default:
				break;
			}
			
		}
	}*/
	CMyServer server;
	server.StartService();
	getchar();
}
void initSock() {
	WSADATA wsa;
	WSAStartup(MAKEWORD(2, 2), &wsa);

}
void clearSock() {
	WSACleanup();
}
void udpServer() {  //服务器
	SOCKET sock_server = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock_server == INVALID_SOCKET) {
		TRACE("socket create failse the error code=%d\r\n",WSAGetLastError());
		printf("server %s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		closesocket(sock_server);
		return;
	}
	std::list<sockaddr_in> client_list;
	sockaddr_in server_addr,client_addr;
	memset(&server_addr, 0, sizeof(sockaddr_in));
	memset(&client_addr, 0, sizeof(sockaddr_in));
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(20000);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	//ucp与tcp的区别：没有listen  直接bind和recvfrom、sendto  整个过程都是操作自己的套接字
	//tcp使用recv和send   ucp使用recvfrom和sendto  
	if (bind(sock_server, (sockaddr*)&server_addr, sizeof(sockaddr_in)) == -1) {  
		printf("socket create failse the error code=%d\r\n", WSAGetLastError());
		printf("server %s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		closesocket(sock_server);
		return;
	}
	std::string buffer;
	buffer.resize(1024 * 256);
	memset((char*)buffer.c_str(), 0, buffer.size());
	int len=sizeof(sockaddr);
	int ret = 0;
	while (!_kbhit()) {
		ret=recvfrom(sock_server, (char*)buffer.c_str(), buffer.size(), 0, (sockaddr*)&client_addr, &len);  //接收来自client的消息  在recvfrom的过程中会得到客户端地址
		if (ret > 0) {//收到内容
			if (client_list.size() == 0) {//第一个连接的客户端
				client_list.push_back(client_addr);//记录客户端的地址
				printf("server (%d) ip:%08X port=%d\r\n",  __LINE__,  client_addr.sin_addr.s_addr, client_addr.sin_port);
				ret = sendto(sock_server, (char*)buffer.c_str(), ret, 0, (sockaddr*)&client_addr, sizeof(sockaddr));
				printf("server (%d)  ip:%08X port=%d\r\n",  __LINE__, client_addr.sin_addr.s_addr, client_addr.sin_port);
			}
			else {//说明是第二个连接的客户端  把ip地址发给他
				buffer.clear();
				memcpy((void*)buffer.c_str(), (const void*)&client_list.front(), sizeof(client_list.front()));
				sendto(sock_server,(char*)buffer.c_str(),sizeof(client_list.front()),0,(sockaddr*)&client_addr,len);
				printf("the other ip has been send to this client\r\n");
			}
			//CTools::Dump((BYTE*)buffer.c_str(),ret);
		}
		else {
			printf("socket create failse the error code=%d  ret=%d\r\n", WSAGetLastError(),ret);
		}
		Sleep(1);
	}
	closesocket(sock_server);
	getchar();
}

void udpClient(bool isHost) {//两个客户端
	Sleep(2000);  //等待服务器启动
	sockaddr_in addr;
	sockaddr_in server_addr;
	int len = sizeof(sockaddr);
	server_addr.sin_family = AF_INET;
	server_addr.sin_port = htons(20000);
	server_addr.sin_addr.s_addr = inet_addr("127.0.0.1");
	SOCKET sock_client = socket(PF_INET, SOCK_DGRAM, 0);
	if (sock_client == INVALID_SOCKET) {
		TRACE("socket create failse\r\n");
		return;
	}
	if (isHost) {
		printf("host  (%d) \r\n", __LINE__ );
		string msg = "hello world\n";
		int ret = sendto(sock_client, msg.c_str(), msg.size(), 0, (sockaddr*)&server_addr, sizeof(sockaddr));
		printf("host (%d) ret=%d  error code=%d\r\n",  __LINE__, ret,WSAGetLastError());
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret=recvfrom(sock_client, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&server_addr, &len);
			printf("host (%d) ret=%d\r\n",  __LINE__, ret);
			if (ret > 0) {
				printf("server (%d) ip:%08X port=%d\r\n",__LINE__, server_addr.sin_addr.s_addr, ntohs(server_addr.sin_port));
				printf("host (%d) msg=%s\r\n",  __LINE__,  msg.c_str());
			}
			ret = recvfrom(sock_client, (char*)msg.c_str(), msg.size(), 0, (sockaddr*)&server_addr, &len);
			printf("host (%d) ret=%d error code=%d msg:%s\r\n", __LINE__, ret,WSAGetLastError(),msg.c_str());
			if (ret > 0) {
				printf("server (%d) ip:%08X port=%d\r\n", __LINE__, server_addr.sin_addr.s_addr, ntohs(server_addr.sin_port));
				printf("host (%d) msg=%s\r\n", __LINE__, msg.c_str());
			}
		}
	}
	else {
		printf("not host (%d)\r\n",  __LINE__);
		string msg = "hello world\n";
		int ret = sendto(sock_client, msg.c_str(), msg.length(), 0, (sockaddr*)&server_addr, sizeof(sockaddr));
		printf("not host (%d) ret=%d  error code=%d\r\n", __LINE__,ret, WSAGetLastError());
		if (ret > 0) {
			msg.resize(1024);
			memset((char*)msg.c_str(), 0, msg.size());
			ret = recvfrom(sock_client, (char*)msg.c_str(), sizeof(msg), 0, (sockaddr*)&server_addr, &len);
			printf("not host (%d)ret=%d  error code=%d msg:%s \r\n", __LINE__, ret,WSAGetLastError(),msg.c_str());
			if (ret > 0) {
				sockaddr_in curaddr;
				memcpy(&curaddr, (void*)msg.c_str(), sizeof(curaddr));
				sockaddr_in* paddr = (sockaddr_in*)&curaddr;
				printf("server (%d)  ip:%08X port=%d\r\n",  __LINE__, server_addr.sin_addr.s_addr, ntohs(server_addr.sin_port));
				printf("not host (%d) msg=%s\r\n", __LINE__, msg.c_str());
				printf(" (%d) this ip address: %08X\r\n", __LINE__ , curaddr.sin_addr.S_un.S_addr);
				msg = "hello i am client";
				sendto(sock_client,msg.c_str(),msg.size(),0,(sockaddr*)paddr,sizeof(sockaddr));
			}
		}
	}
	closesocket(sock_client);
}