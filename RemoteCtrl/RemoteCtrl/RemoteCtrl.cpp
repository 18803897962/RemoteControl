// RemoteCtrl.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include "pch.h"
#include "framework.h"
#include "RemoteCtrl.h"
#include "ServerSocket.h"
#include <direct.h>
#include <atlimage.h>
#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// 唯一的应用程序对象
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
    result += ',';
    CPacket pack(1, (BYTE*)result.c_str(), result.size());   //利用重载构造函数进行打包
    Dump((BYTE*)pack.Data(),pack.Size());
    CServerSocket::getInstance()->Send(pack);
    return 0;
}

#include <stdio.h>
#include <io.h>
#include<list>

int MakeDirectoryInfo() {
    std::string strPath;
    //std::list<FILEINFO> lstFileInfos;
    if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
        OutputDebugString(_T("当前的命令不是获取文件列表，命令解析错误！"));
        return -1; //此时为错误信息
    }
    if (_chdir(strPath.c_str()) != 0) {
        FILEINFO finfo;
        finfo.HasNext = FALSE; //访问不了，没有后续文件
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
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
        return -3;
    }
    int count = 0;
    do {
        FILEINFO finfo;
        finfo.IsDirectory = (fdata.attrib & _A_SUBDIR)!=0;  //判断是否是文件夹
        memcpy(finfo.szFileName, fdata.name, sizeof(fdata.name));
        TRACE("<name>%s\r\n", finfo.szFileName);
        //lstFileInfos.push_back(finfo);
		CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
		CServerSocket::getInstance()->Send(pack);
        count++;
    } while (!_findnext(hfind, &fdata));
	TRACE("%s(%d)%s count=%d\r\n", __FILE__, __LINE__, __FUNCTION__, count);
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
        CPacket head(4, (BYTE*)&data, 8);
        CServerSocket::getInstance()->Send(head);
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
int SendScreen() {
    CImage screen;
    HDC hScreen=::GetDC(NULL);
    int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);  //正常情况下返回 rgb 24 or argb 32
    int nWidth = GetDeviceCaps(hScreen, HORZRES);  //获得屏幕的宽  一般是1920 
    int nHeight = GetDeviceCaps(hScreen, VERTRES); //获得屏幕的高  一般是1080
    screen.Create(nWidth, nHeight, nBitPerPixel);
    BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);  //把图像复制到screen内，相当于完成截屏
    ReleaseDC(NULL, hScreen);
    HGLOBAL hMem=GlobalAlloc(GMEM_MOVEABLE, 0);  //申请一个大小可变的内存（堆上），并得到一个句柄hMem
    if (hMem == NULL)  return -1; 
    IStream* pStream = NULL;
    HRESULT ret = CreateStreamOnHGlobal(hMem, true, &pStream);  //创建一个输入流
    if (ret == S_OK) {
        screen.Save(pStream, Gdiplus::ImageFormatPNG);  //将图片保存到内存流中
        LARGE_INTEGER bg = {};  //位置结构体
        pStream->Seek(bg, STREAM_SEEK_SET, NULL);   //将文件指针重新置零（因为在写入时，文件指针会被修改）
        PBYTE pData=(PBYTE)GlobalLock(hMem);     //GMEM_MOVEABLE属性申请的内存操作系统是可以移动的，必须要进行固定才能得到地址
        SIZE_T nSize = GlobalSize(hMem);
		CPacket pack(6, pData, nSize);
        CServerSocket::getInstance()->Send(pack);  //发送数据
        GlobalUnlock(hMem);   //解锁内存
    }
    /*
	DWORD tick=GetTickCount64();
    TRACE("png:  %d\r\n", GetTickCount64() - tick);
    tick = GetTickCount64();
	screen.Save(_T("test2020.jpg"), Gdiplus::ImageFormatJPEG);
	TRACE("jpg   %d\r\n", GetTickCount64() - tick);
    */
    pStream->Release();
	GlobalFree(hMem);
    screen.ReleaseDC();
    return 0;
}

#include "LockDialog.h"
CLockDialog dlg;
unsigned threadid = 0;
unsigned __stdcall threadLockDlg(void* arg) {
	TRACE("%s(%d):%s\r\n", __FILE__,__LINE__,__FUNCTION__);
	dlg.Create(IDD_DIALOG_INFO, NULL);
	dlg.ShowWindow(SW_SHOW);
	dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
	CRect rect;
	rect.left = 0;
	rect.top = 0;
	rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
	rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);
	rect.bottom = (LONG)(rect.bottom*1.1);
	dlg.MoveWindow(rect);  //设置对话框窗口大小，遮蔽其他进程
	TRACE("%d\r\n", rect.bottom);
	//1、限制鼠标功能：将鼠标位置限制在对话框范围内，且不显示鼠标光标
	ShowCursor(false);   //不显示鼠标光标
	::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);  //隐藏任务栏
	//dlg.GetWindowRect(rect);  // 方法1 得到鼠标的范围
	rect.left = 0;
	rect.top = 0;
	rect.right = 1;
	rect.bottom = 1;  //  方法2 限制鼠标范围只有一个像素点
	ClipCursor(rect);  //限制鼠标范围
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0))  //消息循环
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
		if (msg.message == WM_KEYDOWN) {
			TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);   //得到esc按键的信息
			if (msg.wParam == 0x41)  //当按下的按键是a时，就退出
				break;
		}
	}
    ::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
	ShowCursor(true);   //不显示鼠标光标
	dlg.DestroyWindow();
    _endthreadex(0);
    return 0;
}
int LockMachine() {
    //避免用户端多次点击锁机，开启多个线程
    if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)){   //dlg 为空，表示对话框还没有创建 or 已经被销毁
		//_beginthread(threadLockDlg, 0, NULL);   //建立一个线程去锁机  ，细品细品
        _beginthreadex(NULL, 0, threadLockDlg, NULL, 0, &threadid);
        TRACE("threadid=%d", threadid);
    }
	CPacket pack(7, NULL, 0);
	CServerSocket::getInstance()->Send(pack);   
    return 0;
}
int UnLockMachine() {
    //dlg.SendMessage(WM_KEYDOWN, 0x0000001B, 0x00010001);
    //::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x0000001B, 0x00010001);
    PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
	CPacket pack(8, NULL, 0);
	CServerSocket::getInstance()->Send(pack);
    return 0;
}
int TestConnect() {
    CPacket pack(1981, NULL, 0);
    bool ret=CServerSocket::getInstance()->Send(pack);
    TRACE("Send ret:%d\r\n", ret);
    return 0;
}
int DeleteLocalFile() {
    //TODO:
    std::string strPath;
    CServerSocket::getInstance()->GetFilePath(strPath);
    TCHAR sPath[MAX_PATH] = _T("");
    //mbstowcs(sPath, strPath.c_str(), strPath.size());  //多字节字符集转换成宽字节字符集
    MultiByteToWideChar(CP_ACP,0 , strPath.c_str(), strPath.size(), sPath,
        sizeof(sPath) / sizeof(TCHAR));   //多字节字符集转换成宽字节字符集
    DeleteFile(sPath);
    CPacket pack(9, NULL, 0);
    bool ret = CServerSocket::getInstance()->Send(pack);
    TRACE("ret=%d\r\n", ret);
    return 0;
}
int ExcuteCommand(int nCmd) {
	TRACE("ExcuteCommand:%d\r\n", nCmd);
    int ret = 0;
	switch (nCmd) {
	case 1:
		ret=MakeDriverInfo();  //查看磁盘分区
		break;
	case 2://查看指定目录下的文件
        ret = MakeDirectoryInfo();
		break;
	case 3:
        ret = RunFile(); //运行文件
		break;
	case 4:
        ret = DownloadFile();//下载文件
		break;
	case 5:
        ret = MouseEvent();//鼠标移动
		break;
	case 6: //发送屏幕的内容==》发送屏幕的截图
        ret = SendScreen();
		break;
	case 7:  //锁机
        ret = LockMachine();
		break;
	case 8:  //解锁
        ret = UnLockMachine();
		break;
    case 9:  //删除文件
        ret = DeleteLocalFile();
        break;
    case 1981:
        ret = TestConnect();
        break;
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
            CServerSocket* pserver = CServerSocket::getInstance();  //此时pserver指向new出来的CServerSocket对象
            int count = 0;
			if (pserver->InitSocket() == false) {
				MessageBox(NULL, _T("网络初始化异常，未能成功初始化，请检查网络状态"), _T("网络初始化失败！"), MB_OK | MB_ICONERROR);
				exit(0);
			}
            while (CServerSocket::getInstance() !=NULL) {  //accept 失败，允许三次自动重新连接
                if (pserver->AcceptClient() == false) {
                    if (count >= 3) {
						MessageBox(NULL, _T("多次无法接入用户，结束程序"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                        exit(0);   
                    }
					MessageBox(NULL, _T("接入用户失败，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
                    count++;
                }
                TRACE("AcceptClient return true\r\n");
                int ret = pserver->DealCommand();
                TRACE("DealCommand:%d\r\n", ret);
                if (ret > 0) {
                    ret=ExcuteCommand(ret);
                    if (ret != 0) {
                        TRACE("执行命令失败:%d ret=%d\r\n", pserver->GetPacket().sCmd,ret);
                    }
                    pserver->CloseClient();
                    TRACE("Command has done!\r\n");
                }
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
