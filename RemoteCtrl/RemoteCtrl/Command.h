#pragma once
#include "resource.h"
#include <map>
#include <atlimage.h>
#include <direct.h>
#include <stdio.h>
#include <io.h>
#include<list>
#include "Packet.h"
#include "RemoteCtrl.h"
#include "Tools.h"
#include "LockDialog.h"
class CCommand
{
public:
	CCommand();
	~CCommand() {}
	int ExcuteCommand(int nCmd, std::list<CPacket>& lstCPacket,CPacket& inPacket);
	static void RunCommand(void* arg, int status, std::list<CPacket>& listCPacket, CPacket& inPacket) {
		CCommand* thiz = (CCommand*)arg;
		if (status > 0) {
			int ret = thiz->ExcuteCommand(status,listCPacket,inPacket);
			if (ret != 0) {
				TRACE("执行命令失败,cmd=%d ret=%d\r\n", status, ret);
			}
		}
		else {
				MessageBox(NULL, _T("接入用户失败，自动重试"), _T("接入用户失败！"), MB_OK | MB_ICONERROR);
		}
	}
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket);  //成员函数指针
	std::map<int, CMDFUNC> m_mapFunction;  //从命令号到功能的映射
	CLockDialog dlg;
	unsigned threadid;
protected:
	static unsigned __stdcall threadLockDlg(void* arg) {
		CCommand* thiz = (CCommand*)arg;
		thiz->threadLockDlgMain();
		_endthreadex(0);
		return 0;
	}
	void threadLockDlgMain() {
		TRACE("%s(%d):%s\r\n", __FILE__, __LINE__, __FUNCTION__);
		dlg.Create(IDD_DIALOG_INFO, NULL);
		dlg.ShowWindow(SW_SHOW);
		dlg.SetWindowPos(&dlg.wndTopMost, 0, 0, 0, 0, SWP_NOSIZE | SWP_NOMOVE);
		CRect rect;
		rect.left = 0;
		rect.top = 0;
		rect.right = GetSystemMetrics(SM_CXFULLSCREEN);
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);   //整个对话框大小为  (rect.right-rect.left) * (rect.bottom-rect.top)  rect.left=rect.top=0
		rect.bottom = (LONG)(rect.bottom * 1.15);
		dlg.MoveWindow(rect);  //设置对话框窗口大小，遮蔽其他进程
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
		if (pText) {
			CRect rtText;
			pText->GetWindowRect(rtText);
			int nWidth = rtText.Width() / 2;      //文本框大小为  rtText.Width() *  rtText.Height()
			int x = (rect.right - nWidth) / 2;
			int nHeight = rtText.Height();
			int y = (rect.bottom - nHeight) / 2;
			pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
		}
		TRACE("%d\r\n", rect.bottom);
		//1、限制鼠标功能：将鼠标位置限制在对话框范围内，且不显示鼠标光标
		ShowCursor(false);   //不显示鼠标光标
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);  //隐藏任务栏
		dlg.GetWindowRect(rect);  // 方法1 得到鼠标的范围
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
		ClipCursor(NULL);
		//恢复任务栏
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
		//恢复鼠标
		ShowCursor(true);   //不显示鼠标光标
		dlg.DestroyWindow();
	}
	int MakeDriverInfo(std::list<CPacket>& lstCPacket, CPacket& inPacket){ //获取磁盘分区信息
		std::string result;
		//_chdrive(i)的i的数值：1代表A盘，2代表B盘，3代表C盘...26代表Z盘，如果磁盘存在，则返回0
		for (int i = 1; i <= 26; i++) {
			if (_chdrive(i) == 0) {
				if (result.size() > 0) result += ',';
				result += ('A' + i - 1);   //得到盘符
			}
		}
		result += ',';
		lstCPacket.push_back(CPacket(1, (BYTE*)result.c_str(), result.size()));
		return 0;
	}
	int MakeDirectoryInfo(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		std::string strPath=inPacket.strData;
		if (_chdir(strPath.c_str()) != 0) {
			FILEINFO finfo;
			finfo.HasNext = FALSE; //访问不了，没有后续文件
			lstCPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			OutputDebugString(_T("没有权限访问目录！"));
			return -2;
		}
		_finddata_t fdata;
		int hfind = 0;
		if ((hfind = _findfirst("*", &fdata)) == -1)//第一个参数表示读取的类型"*.txt、*.exe"等，"*"表示读取所有类型的文件
		{//查找失败
			OutputDebugString(_T("没有找到该文件！"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			lstCPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			return -3;
		}
		int count = 0;
		do {
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;  //判断是否是文件夹
			memcpy(finfo.szFileName, fdata.name, sizeof(fdata.name));
			TRACE("<name>%s\r\n", finfo.szFileName);
			lstCPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			count++;
		} while (!_findnext(hfind, &fdata));
		TRACE("%s(%d)%s count=%d\r\n", __FILE__, __LINE__, __FUNCTION__, count);
		FILEINFO finfo;
		finfo.HasNext = FALSE;
		lstCPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
		return 0;
	}
	int RunFile(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		std::string strPath=inPacket.strData;
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL); //打开相应的文件
		lstCPacket.push_back(CPacket(3, NULL, 0));
		return 0;
	}
	int DownloadFile(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		std::string strPath = inPacket.strData;
		long long data = 0;
		FILE* pFile = NULL;
		errno_t err = fopen_s(&pFile, strPath.c_str(), "rb");
		if ((err != 0)) {
			lstCPacket.push_back(CPacket(4, (BYTE*)data, 8));
			return -1;
		}
		if (pFile != NULL) {
			fseek(pFile, 0, SEEK_END);
			data = _ftelli64(pFile);
			lstCPacket.push_back(CPacket(4, (BYTE*)&data, 8));
			fseek(pFile, 0, SEEK_SET);
			char buffer[1024] = "";
			size_t rlen = 0;
			do
			{
				rlen = fread(buffer, 1, 1024, pFile);
				lstCPacket.push_back(CPacket(4, (BYTE*)buffer, rlen));
				//memset(buffer, 0, 1024);
			} while (rlen >= 1024);  //说明此次缓冲区已经读满了，可以接着下次读取
			fclose(pFile);
		}
		//读完之后得到一个空的数据包，说明已经完成
		lstCPacket.push_back(CPacket(4, NULL, 0));
		return 0;
	}
	int MouseEvent(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
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
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
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
			lstCPacket.push_back(CPacket(5, NULL, 0));
		return 0;
	}
	int SendScreen(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		CImage screen;
		HDC hScreen = ::GetDC(NULL);
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);  //正常情况下返回 rgb 24 or argb 32
		int nWidth = GetDeviceCaps(hScreen, HORZRES);  //获得屏幕的宽  一般是1920 
		int nHeight = GetDeviceCaps(hScreen, VERTRES); //获得屏幕的高  一般是1080
		screen.Create(nWidth, nHeight, nBitPerPixel);
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);  //把图像复制到screen内，相当于完成截屏
		ReleaseDC(NULL, hScreen);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  //申请一个大小可变的内存（堆上），并得到一个句柄hMem
		if (hMem == NULL)  return -1;
		IStream* pStream = NULL;
		HRESULT ret = CreateStreamOnHGlobal(hMem, true, &pStream);  //创建一个输入流
		if (ret == S_OK) {
			screen.Save(pStream, Gdiplus::ImageFormatPNG);  //将图片保存到内存流中
			LARGE_INTEGER bg = {};  //位置结构体
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);   //将文件指针重新置零（因为在写入时，文件指针会被修改）
			PBYTE pData = (PBYTE)GlobalLock(hMem);     //GMEM_MOVEABLE属性申请的内存操作系统是可以移动的，必须要进行固定才能得到地址
			SIZE_T nSize = GlobalSize(hMem);

			lstCPacket.push_back(CPacket(6, pData, nSize));
			GlobalUnlock(hMem);   //解锁内存
		}


		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();
		return 0;
	}
	int LockMachine(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		//避免用户端多次点击锁机，开启多个线程
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {   //dlg 为空，表示对话框还没有创建 or 已经被销毁
			//_beginthread(threadLockDlg, 0, NULL);   //建立一个线程去锁机  ，细品细品
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
			TRACE("threadid=%d", threadid);
		}
		lstCPacket.push_back(CPacket(7, NULL, 0));
		return 0;
	}
	int UnLockMachine(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		//dlg.SendMessage(WM_KEYDOWN, 0x0000001B, 0x00010001);
		//::SendMessage(dlg.m_hWnd, WM_KEYDOWN, 0x0000001B, 0x00010001);
		PostThreadMessage(threadid, WM_KEYDOWN, 0x41, 0);
		lstCPacket.push_back(CPacket(8, NULL, 0));
		return 0;
	}
	int DeleteLocalFile(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		//TODO:
		std::string strPath = inPacket.strData;
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath, strPath.c_str(), strPath.size());  //多字节字符集转换成宽字节字符集
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
			sizeof(sPath) / sizeof(TCHAR));   //多字节字符集转换成宽字节字符集
		DeleteFile(sPath);
		lstCPacket.push_back(CPacket(9, NULL, 0));
		return 0;
	}
	int TestConnect(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		lstCPacket.push_back(CPacket(1981, NULL, 0));
		return 0;
	}
};