// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Tools.h"
#include "Command.h"
#include <conio.h>
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

#define IOCP_LIST_EMPTY 0
#define IOCP_LIST_PUSH 1
#define IOCP_LIST_POP 2
enum {
	iocp_list_empty,
	iocp_list_push,
	iocp_list_pop
};

//iocp完成端口映射
int messcount = 0;
int delecount = 0;
struct IOCP_Param {
	int	nOperator;//插入or删除
	std::string strData;
	_beginthread_proc_type cbFunc;//回调函数
	IOCP_Param(int op, const char* sData, _beginthread_proc_type cbfunc = NULL) {
		nOperator = op;
		strData = sData;
		cbFunc = cbfunc;
	}
	IOCP_Param() :nOperator(-1) {}
}IOCPPARAM;

void threadPlay(HANDLE hIOCP) {
	std::list<std::string> lstString;
	DWORD dwTransfered = 0;
	ULONG_PTR CompletionKey = 0;
	OVERLAPPED* pOverlaped = 0;
	while (GetQueuedCompletionStatus(hIOCP, &dwTransfered, &CompletionKey, &pOverlaped, INFINITE)) {
		if (dwTransfered == 0 || CompletionKey == NULL) {  //主动按下键盘，结束信号的发送时，就代表iocp已经退出
			printf("thread is prapare to exit!\n");
			break;
		}
		IOCP_Param* pParam = (IOCP_Param*)CompletionKey;
		if (pParam->nOperator == iocp_list_push) {
			lstString.push_back(pParam->strData);
			printf("push\n");
		}
		else if (pParam->nOperator == iocp_list_pop) {
			std::string* pStr = NULL;
			if (lstString.size() > 0) {
				pStr = new std::string(lstString.front());
				lstString.pop_front();
			}
			if (pParam->cbFunc) {
				pParam->cbFunc(pStr);
			}
		}
		else if (pParam->nOperator == iocp_list_empty) {
			lstString.clear();
		}
		delete pParam;
		delecount++;
	}
}

void threadQueEntry(HANDLE hIOCP) {
	threadPlay(hIOCP);  //这种做法，确保线程函数中的变量在函数结束时被释放，避免_endthread()调用时，变量未被释放而导致内存泄漏
	_endthread();
}

void func(void* arg) {
	std::string* pstr = (std::string*)arg;
	if (pstr != NULL) {
		printf("pop from list:%s\n", pstr->c_str());
	}
	else {
		printf("the list is empty\n");
	}
	delete pstr;
}



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
	HANDLE hIOCP = INVALID_HANDLE_VALUE;//IOCP IO Completion PORT
	hIOCP = CreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, NULL, 1);//1表示能够同时访问的线程数 设置为1，只有一个线程来处理队列 可以由多个线程需要该线程处理消息队列
	if (hIOCP == INVALID_HANDLE_VALUE||hIOCP==NULL) {
		printf("create iocp failed %d\n", GetLastError());
	}
	HANDLE hThread=(HANDLE) _beginthread(threadQueEntry, 0, hIOCP);
	DWORD tick = GetTickCount64();
	DWORD tick0 = GetTickCount64();
	while (_kbhit()==0) {
		if (GetTickCount64() - tick0 > 1300) {
			PostQueuedCompletionStatus(hIOCP,sizeof(IOCP_Param), (ULONG_PTR)new IOCP_Param(iocp_list_pop, "hello world",func), NULL);
			messcount++;
			tick0 = GetTickCount64();
		}
		if (GetTickCount64() - tick > 2000) {
			PostQueuedCompletionStatus(hIOCP,sizeof(IOCP_Param), (ULONG_PTR)new IOCP_Param(iocp_list_push,"hello world"), NULL);
			messcount++;
			tick = GetTickCount64();
		}
		Sleep(1);
	}
	if (hIOCP != INVALID_HANDLE_VALUE) {
		//TODO:唤醒完成端口 线程需要结束
		PostQueuedCompletionStatus(hIOCP, sizeof(IOCP_Param), (ULONG_PTR)new IOCP_Param(iocp_list_empty, "hello world"), NULL);//传递一个状态
		messcount++;
		PostQueuedCompletionStatus(hIOCP, 0, NULL, NULL);
		WaitForSingleObject(hThread,INFINITE);//等待线程结束
	}
	printf("exit\n");
	printf("messcount=%d  delecount=%d\n", messcount, delecount);
	CloseHandle(hIOCP);
	::exit(0);
    return 0;
}
