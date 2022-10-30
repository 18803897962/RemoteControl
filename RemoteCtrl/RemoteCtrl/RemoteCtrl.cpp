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
CWinApp theApp;
using namespace std;

//开机启动的时候，权限是跟随用户权限的
//如果两者权限不一致，则会导致程序启动失败
//开机启动对环境变量有要求，如果依赖dll库动态库，则可能启动失败
//可以将dll库赋值到system32或sysWow64下
//system32下多是64位程序，而sysWow64下多是32位程序
void ChooseAutoInvoke() {//自动启动
	CString strPath = (CString)(_T("C:\\Windows\\SysWOW64\\RemoteCtrl.exe"));
    if (PathFileExists(strPath)) {  //如果文件已经存在，就返回
        return;
    }
    CString strSubKey=_T("SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
    CString strInfo = _T("该程序只允许用于合法用途！\n");
    strInfo += _T("继续运行该程序将使得该机器处于被监控状态！\n");
	strInfo += _T("如果你不希望，请按取消按钮退出程序。\n");
	strInfo += _T("按下“是”按钮，该程序将被复制到你的机器，随系统自动运行。\n");
	strInfo += _T("按下“否”按钮，该程序值运行依次，将不会在系统中残留。\n");
    int ret=MessageBox(NULL,strInfo,_T("警告"),MB_YESNOCANCEL|MB_ICONWARNING|MB_TOPMOST);
    if (ret == IDYES) {
        char sPath[MAX_PATH] = "";
        char sSys[MAX_PATH] = "";
        std::string strExe = "\\RemoteCtrl.exe ";
        GetCurrentDirectoryA(MAX_PATH, sPath);
        GetSystemDirectoryA(sSys, sizeof(sSys));
        std::string strCmd = "cmd /K mklink "+std::string(sSys)+strExe + std::string(sPath) + strExe;  //创建链接
        ret=system(strCmd.c_str());
        TRACE("ret=%d\r\n", ret);
        //注册表操作
        HKEY hKey=NULL;
        ret=RegOpenKeyEx(HKEY_LOCAL_MACHINE, strSubKey, 0, KEY_ALL_ACCESS|KEY_WOW64_64KEY, &hKey);    //Windows10下必须要加KEY_WOW64_64KEY
        if (ret != ERROR_SUCCESS) {
            RegCloseKey(hKey);
            MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败"),_T("错误"),MB_ICONERROR|MB_TOPMOST);
            exit(0);
        }
        ret=RegSetValueEx(hKey, _T("RemoteCtrl"), 0, REG_EXPAND_SZ,(BYTE*)(LPCTSTR)strPath,strPath.GetLength()*sizeof(TCHAR));
        if (ret != ERROR_SUCCESS) {
			RegCloseKey(hKey);
			MessageBox(NULL, _T("设置自动开机启动失败！是否权限不足？\r\n程序启动失败"), _T("错误"), MB_ICONERROR | MB_TOPMOST);
			exit(0);
        }
        RegCloseKey(hKey);
    }
    else if (ret == IDCANCEL) {
        exit(0);
    }
    return;
}
int main()
{
    int nRetCode = 0;

    HMODULE hModule = ::GetModuleHandle(nullptr);

    if (hModule != nullptr)
    {
        // 初始化 MFC 并在失败时显示错误
        if (!AfxWinInit(hModule, nullptr, ::GetCommandLine(), 0))
        {
            // TODO: 在此处为应用程序的行为编写代码。
            wprintf(L"错误: MFC 初始化失败\n");
            nRetCode = 1;
        }
        else
        {
            // TODO: 在此处为应用程序的行为编写代码。
            CCommand cmd;
            ChooseAutoInvoke();
            int ret= CServerSocket::getInstance()->Run(&CCommand::RunCommand, &cmd);
            switch (ret)
            {
            case -1:
                MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
                exit(0);
                break;
            case -2:
                MessageBox(NULL, _T("多次无法接入用户，结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                exit(0);
                break;
            default:
                break;
            }
        }
    }
    else
    {
        // TODO: 更改错误代码以符合需要
        wprintf(L"错误: GetModuleHandle 失败\n");
        nRetCode = 1;
    }

    return nRetCode;
}
