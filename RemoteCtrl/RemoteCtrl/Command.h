#pragma once
#include "resource.h"
#include <map>
#include <atlimage.h>
#include <direct.h>
#include <stdio.h>
#include <io.h>
#include<list>
#include "ServerSocket.h"
#include "RemoteCtrl.h"
#include "Tools.h"
#include "LockDialog.h"
class CCommand
{
public:
	CCommand();
	~CCommand() {}
	int ExcuteCommand(int nCmd);
protected:
	typedef int(CCommand::* CMDFUNC)();  //��Ա����ָ��
	std::map<int, CMDFUNC> m_mapFunction;  //������ŵ����ܵ�ӳ��
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
		rect.bottom = GetSystemMetrics(SM_CYFULLSCREEN);   //�����Ի����СΪ  (rect.right-rect.left) * (rect.bottom-rect.top)  rect.left=rect.top=0
		rect.bottom = (LONG)(rect.bottom * 1.15);
		dlg.MoveWindow(rect);  //���öԻ��򴰿ڴ�С���ڱ���������
		CWnd* pText = dlg.GetDlgItem(IDC_STATIC);
		if (pText) {
			CRect rtText;
			pText->GetWindowRect(rtText);
			int nWidth = rtText.Width() / 2;      //�ı����СΪ  rtText.Width() *  rtText.Height()
			int x = (rect.right - nWidth) / 2;
			int nHeight = rtText.Height();
			int y = (rect.bottom - nHeight) / 2;
			pText->MoveWindow(x, y, rtText.Width(), rtText.Height());
		}
		TRACE("%d\r\n", rect.bottom);
		//1��������깦�ܣ������λ�������ڶԻ���Χ�ڣ��Ҳ���ʾ�����
		ShowCursor(false);   //����ʾ�����
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_HIDE);  //����������
		dlg.GetWindowRect(rect);  // ����1 �õ����ķ�Χ
		rect.left = 0;
		rect.top = 0;
		rect.right = 1;
		rect.bottom = 1;  //  ����2 ������귶Χֻ��һ�����ص�
		ClipCursor(rect);  //������귶Χ
		MSG msg;
		while (GetMessage(&msg, NULL, 0, 0))  //��Ϣѭ��
		{
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN) {
				TRACE("msg:%08X wparam:%08X lparam:%08X\r\n", msg.message, msg.wParam, msg.lParam);   //�õ�esc��������Ϣ
				if (msg.wParam == 0x41)  //�����µİ�����aʱ�����˳�
					break;
			}
		}
		ClipCursor(NULL);
		//�ָ�������
		::ShowWindow(::FindWindow(_T("Shell_TrayWnd"), NULL), SW_SHOW);
		//�ָ����
		ShowCursor(true);   //����ʾ�����
		dlg.DestroyWindow();
	}
	int MakeDriverInfo(){ //��ȡ���̷�����Ϣ
		std::string result;
		//_chdrive(i)��i����ֵ��1����A�̣�2����B�̣�3����C��...26����Z�̣�������̴��ڣ��򷵻�0
		for (int i = 1; i <= 26; i++) {
			if (_chdrive(i) == 0) {
				if (result.size() > 0) result += ',';
				result += ('A' + i - 1);   //�õ��̷�
			}
		}
		result += ',';
		CPacket pack(1, (BYTE*)result.c_str(), result.size());   //�������ع��캯�����д��
		CTools::Dump((BYTE*)pack.Data(), pack.Size());
		CServerSocket::getInstance()->Send(pack);
		return 0;
	}
	int MakeDirectoryInfo() {
		std::string strPath;
		//std::list<FILEINFO> lstFileInfos;
		if (CServerSocket::getInstance()->GetFilePath(strPath) == false) {
			OutputDebugString(_T("��ǰ������ǻ�ȡ�ļ��б������������"));
			return -1; //��ʱΪ������Ϣ
		}
		if (_chdir(strPath.c_str()) != 0) {
			FILEINFO finfo;
			finfo.HasNext = FALSE; //���ʲ��ˣ�û�к����ļ�
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			OutputDebugString(_T("û��Ȩ�޷���Ŀ¼��"));
			return -2;
		}
		_finddata_t fdata;
		int hfind = 0;
		if ((hfind = _findfirst("*", &fdata)) == -1)//��һ��������ʾ��ȡ������"*.txt��*.exe"�ȣ�"*"��ʾ��ȡ�������͵��ļ�
		{//����ʧ��
			OutputDebugString(_T("û���ҵ����ļ���"));
			FILEINFO finfo;
			finfo.HasNext = FALSE;
			CPacket pack(2, (BYTE*)&finfo, sizeof(finfo));
			CServerSocket::getInstance()->Send(pack);
			return -3;
		}
		int count = 0;
		do {
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;  //�ж��Ƿ����ļ���
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
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL); //����Ӧ���ļ�
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
			} while (rlen >= 1024);  //˵���˴λ������Ѿ������ˣ����Խ����´ζ�ȡ
			fclose(pFile);
		}
		CPacket pack(4, NULL, 0);
		CServerSocket::getInstance()->Send(pack);  //����֮��õ�һ���յ����ݰ���˵���Ѿ����
		return 0;
	}
	int MouseEvent() {
		MOUSEEV mouse;
		if (CServerSocket::getInstance()->GetMouseEvent(mouse)) {
			DWORD nFlags = 0;
			switch (mouse.nButton)
			{
			case 0://���
				nFlags = 1;
				break;
			case 1://�Ҽ�
				nFlags = 2;
				break;
			case 2://�м��or����
				nFlags = 4;
				break;
			case 4://û�а���
				nFlags = 8;
				break;
			}
			if (nFlags != 8) SetCursorPos(mouse.ptXY.x, mouse.ptXY.y);
			switch (mouse.nAction)
			{
			case 0://����
				nFlags |= 0x10;
				break;
			case 1://˫��
				nFlags |= 0x20;
				break;
			case 2://����
				nFlags |= 0x40;
				break;
			case 3://�ɿ�
				nFlags |= 0x80;
				break;
			default:
				break;
			}
			switch (nFlags)
			{
			case 0x21://���˫��
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());  //���£�GetMessageExtraInfo�ɻ�ȡ�߳��е�һЩ��Ϣ���������̡����
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo()); //����
			case 0x11://�������
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x22://�Ҽ�˫��
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x12://�Ҽ�����
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x24://����˫��
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
			case 0x14://���ֵ���
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x41://�������
				mouse_event(MOUSEEVENTF_LEFTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x42://�Ҽ�����
				mouse_event(MOUSEEVENTF_RIGHTDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x44://���ְ���
				mouse_event(MOUSEEVENTF_MIDDLEDOWN, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x81://����ɿ�
				mouse_event(MOUSEEVENTF_LEFTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x82://�Ҽ��ɿ�
				mouse_event(MOUSEEVENTF_RIGHTUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x84://�����ɿ�
				mouse_event(MOUSEEVENTF_MIDDLEUP, 0, 0, 0, GetMessageExtraInfo());
				break;
			case 0x08://����������ƶ�
				mouse_event(MOUSEEVENTF_MOVE, mouse.ptXY.x, mouse.ptXY.y, 0, GetMessageExtraInfo());
				break;
			}
			CPacket pack(4, NULL, 0);
			CServerSocket::getInstance()->Send(pack);
		}
		else {
			OutputDebugString(_T("��ȡ����������ʧ��!"));
			return -1;
		}
		return 0;
	}
	int SendScreen() {
		CImage screen;
		HDC hScreen = ::GetDC(NULL);
		int nBitPerPixel = GetDeviceCaps(hScreen, BITSPIXEL);  //��������·��� rgb 24 or argb 32
		int nWidth = GetDeviceCaps(hScreen, HORZRES);  //�����Ļ�Ŀ�  һ����1920 
		int nHeight = GetDeviceCaps(hScreen, VERTRES); //�����Ļ�ĸ�  һ����1080
		screen.Create(nWidth, nHeight, nBitPerPixel);
		BitBlt(screen.GetDC(), 0, 0, nWidth, nHeight, hScreen, 0, 0, SRCCOPY);  //��ͼ���Ƶ�screen�ڣ��൱����ɽ���
		ReleaseDC(NULL, hScreen);
		HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, 0);  //����һ����С�ɱ���ڴ棨���ϣ������õ�һ�����hMem
		if (hMem == NULL)  return -1;
		IStream* pStream = NULL;
		HRESULT ret = CreateStreamOnHGlobal(hMem, true, &pStream);  //����һ��������
		if (ret == S_OK) {
			screen.Save(pStream, Gdiplus::ImageFormatPNG);  //��ͼƬ���浽�ڴ�����
			LARGE_INTEGER bg = {};  //λ�ýṹ��
			pStream->Seek(bg, STREAM_SEEK_SET, NULL);   //���ļ�ָ���������㣨��Ϊ��д��ʱ���ļ�ָ��ᱻ�޸ģ�
			PBYTE pData = (PBYTE)GlobalLock(hMem);     //GMEM_MOVEABLE����������ڴ����ϵͳ�ǿ����ƶ��ģ�����Ҫ���й̶����ܵõ���ַ
			SIZE_T nSize = GlobalSize(hMem);
			CPacket pack(6, pData, nSize);
			CServerSocket::getInstance()->Send(pack);  //��������
			GlobalUnlock(hMem);   //�����ڴ�
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
	int LockMachine() {
		//�����û��˶�ε����������������߳�
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {   //dlg Ϊ�գ���ʾ�Ի���û�д��� or �Ѿ�������
			//_beginthread(threadLockDlg, 0, NULL);   //����һ���߳�ȥ����  ��ϸƷϸƷ
			_beginthreadex(NULL, 0, &CCommand::threadLockDlg, this, 0, &threadid);
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
		bool ret = CServerSocket::getInstance()->Send(pack);
		TRACE("Send ret:%d\r\n", ret);
		return 0;
	}
	int DeleteLocalFile() {
		//TODO:
		std::string strPath;
		CServerSocket::getInstance()->GetFilePath(strPath);
		TCHAR sPath[MAX_PATH] = _T("");
		//mbstowcs(sPath, strPath.c_str(), strPath.size());  //���ֽ��ַ���ת���ɿ��ֽ��ַ���
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
			sizeof(sPath) / sizeof(TCHAR));   //���ֽ��ַ���ת���ɿ��ֽ��ַ���
		DeleteFile(sPath);
		CPacket pack(9, NULL, 0);
		bool ret = CServerSocket::getInstance()->Send(pack);
		TRACE("ret=%d\r\n", ret);
		return 0;
	}
};

