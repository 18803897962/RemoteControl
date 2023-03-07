// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include "Tools.h"
#include "Command.h"
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

int main()
{
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
	}
    return 0;
}
