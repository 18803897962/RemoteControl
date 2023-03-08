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
int main()
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