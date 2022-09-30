// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象

//分支001
CWinApp theApp;

using namespace std;
void Dump(BYTE* pData, size_t nSize) {
    std::string strout;
    for (size_t i = 0; i < nSize; i++) {
        char buf[8] = "";
        if (i > 0 && (i % 16 == 0)) strout += "\n";
        snprintf(buf, sizeof(buf), "%02X", pData[i] & 0xFF);
        strout += buf;
    }
    strout += "\n";
    OutputDebugStringA(strout.c_str());
}
int MakeDriverInfo() { //获取磁盘分区信息
    std::string result;
    //_chdrive(i)的i的数值：1代表A盘，2代表B盘，3代表C盘...26代表Z盘，如果磁盘存在，则返回0
    for (int i = 1; i <= 26; i++) {
        if (_chdrive(i) == 0) {
            if (result.size() > 0) result += ',';
            result += ('A' + i - 1);   //得到盘符
        }
    }
    CPacket pack(1, (const BYTE*)result.c_str(), result.size());   //利用重载构造函数进行打包
    Dump((BYTE*)pack.Data(),pack.Size());
    CServerSocket::getInstance()->Send(pack);
    return 0;
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
   //         CServerSocket* pserver = CServerSocket::getInstance();  //此时pserver指向new出来的CServerSocket对象
   //         int count = 0;
			//if (pserver->InitSocket() == false) {
			//	MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
			//	exit(0);
			//}
   //         while (CServerSocket::getInstance() !=NULL) {  //accept 失败，允许三次自动重新连接
   //             if (pserver->AcceptClient() == false) {
   //                 if (count >= 3) {
			//			MessageBox(NULL, _T("多次无法接入用户，结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
   //                     exit(0);   
   //                 }
			//		MessageBox(NULL, _T("接入用户失败，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
   //                 count++;
   //             }
   //             int ret = pserver->DealCommand();
                //TODO:处理命令
           //}   

            int nCmd=1;
            switch (nCmd) {
            case 1:
				MakeDriverInfo();  //查看磁盘分区
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
