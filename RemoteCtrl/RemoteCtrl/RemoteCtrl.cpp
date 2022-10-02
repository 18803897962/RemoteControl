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

#include <stdio.h>
#include <io.h>
#include<list>
typedef struct file_info
{
    file_info() { //构造函数
        IsInvalid = FALSE;
        IsDirectory = -1;
        HasNext = TRUE;
        memset(szFileNmae, 0, sizeof(szFileNmae));
    }
    BOOL IsInvalid; //是否有效
	BOOL IsDirectory;//是否为目录 0否 1是
    BOOL HasNext; //是否含有后续文件 0没有 1有  用于实现找到一个发送一个
    char szFileNmae[260];
}FILEINFO,*PFILEINFO;
int MakeDirectoryInfo() {
    std::string strPath;
    //std::list<FILEINFO> lstFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("当前的命令不是获取文件列表，命令解析错误！"));
        return -1; //此时为错误信息
    }
    if (_chdir(strPath.c_str()) != 0) {
        FILEINFO finfo;
        finfo.IsInvalid = TRUE;  //该文件无效
        finfo.IsDirectory = TRUE; //文件访问不了，则不是目录
        finfo.HasNext = FALSE; //访问不了，没有后续文件
        memcpy(finfo.szFileNmae, strPath.c_str(), strPath.size());
        //lstFileInfos.push_back(finfo);
        CPacket pack(2, (BYTE*)&finfo,sizeof(finfo));
        CServerSocket::getInstance()->Send(pack);
        OutputDebugString(_T("没有权限访问目录！"));
        return -2;
    }
    _finddata_t fdata;
    int hfind = 0;
    if ((hfind=_findfirst("*", &fdata)) == -1)//第一个参数表示读取的类型"*.txt、*.exe"等，"*"表示读取所有类型的文件
    {//查找失败
        OutputDebugString(_T("没有找到该文件！"));
        return -3;
    }
    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR)!=0;  //判断是否是文件夹
        memcpy(finfo.szFileNmae, fdata.name, sizeof(fdata.name));
        //lstFileInfos.push_back(finfo);
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
    } while (!_findnext(hfind, &fdata));
    FILEINFO finfo;
    finfo.HasNext = FALSE;
	CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
	CServerSocket::getInstance()->Send(pack);
    return 0;
}
int RunFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    ShellExecuteA(NULL, NULL, strPath.c_str(),NULL,NULL,SW_SHOWNORMAL); //打开相应的文件
	CPacket pack(3, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
    return 0;
}
int DownloadFile() {
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    long long data = 0;
    FILE* pFile = NULL;
    errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
    if ((err != 0)) {
        CPacket pack(4, (BYTE*)data, 8);
        CServerSocket::getInstance()->Send(pack);
        return -1;
    }
    if (pFile != NULL) {
        fseek(pFile, 0, SEEK_END);
        data = _ftelli64(pFile);
        CPacket head(4, (BYTE*)data, 8);
        fseek(pFile, 0, SEEK_SET);
        char buffer[1024] = "";
        size_t rlen = 0;
        do
        {
            rlen = fread(buffer, 1, 1024, pFile);
            CPacket pack(4, (BYTE*)buffer, rlen);
            CServerSocket::getInstance()->Send(pack);
            //memset(buffer, 0, 1024);
        } while (rlen >= 1024);  //说明此次缓冲区已经读满了，可以接着下次读取
        fclose(pFile);
    }
	CPacket pack(4, NULL, 0);
	CServerSocket::getInstance()->Send(pack);  //读完之后得到一个空的数据包，说明已经完成
    return 0;
}
int MouseEvent() {
    MOUSEEV mouse;
    if(CServerSocket::getInstance()->GetMouseEvent(mouse)){
        DWORD nFlags = 0;
        switch (mouse.nButton)
        {
        case 0://左键
            nFlags = 1;
            break;
        case 1://右键
            nFlags = 2;
            break;
        case 2://中间键or滚轮
            nFlags = 4;
            break;
        case 4://没有按键
            nFlags = 8;
            break;
        }
        if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
        switch (mouse.nAction)
        {
        case 0://单击
            nFlags |= 0x10;
            break;
		case 1://双击
			nFlags |= 0x20;
			break;
		case 2://按下
			nFlags |= 0x40;
			break;
		case 3://松开
			nFlags |= 0x80;
			break;
        default:
            break;
        }
        switch (nFlags)
        {
        case 0x21://左键双击
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());  //按下，GetMessageExtraInfo可获取线程中的一些信息，包括键盘、鼠标
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo()); //弹起
        case 0x11://左键单击
            mouse_event(MOUSEEVENTF_LEFTDOWN,0,0,0,GetMessageExtraInfo()); 
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo()); 
            break;
        case 0x22://右键双击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo()); 
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo()); 
        case 0x12://右键单击
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());  
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo()); 
			break;
        case 0x24://滚轮双击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());  
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
		case 0x14://滚轮单击
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());  
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo()); 
			break;
        case 0x41://左键按下
			mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());  
            break;
		case 0x42://右键按下
			mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());  
            break;
		case 0x44://滚轮按下
			mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo()); 
            break;
		case 0x81://左键松开
			mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo()); 
            break;
		case 0x82://右键松开
			mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo()); 
            break;
		case 0x84://滚轮松开
			mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo()); 
            break;
        case 0x08://单纯的鼠标移动
            mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
            break;
        }
        CPacket pack(4, NULL, 0);
        CServerSocket::getInstance()->Send(pack);
    }
    else {
        OutputDebugString(_T("获取鼠标操作参数失败!"));
        return -1;
    }
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
            case 2://查看指定目录下的文件
                MakeDirectoryInfo();
                break;
            case 3:
                RunFile(); //运行文件
                break;
            case 4:
                DownloadFile();
                break;
            case 5:
                MouseEvent();
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
