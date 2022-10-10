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
				TRACE("ִ������ʧ��,cmd=%d ret=%d\r\n", status, ret);
			}
		}
		else {
				MessageBox(NULL, _T("�����û�ʧ�ܣ��Զ�����"), _T("�����û�ʧ�ܣ�"), MB_OK | MB_ICONERROR);
		}
	}
protected:
	typedef int(CCommand::* CMDFUNC)(std::list<CPacket>&, CPacket& inPacket);  //��Ա����ָ��
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
	int MakeDriverInfo(std::list<CPacket>& lstCPacket, CPacket& inPacket){ //��ȡ���̷�����Ϣ
		std::string result;
		//_chdrive(i)��i����ֵ��1����A�̣�2����B�̣�3����C��...26����Z�̣�������̴��ڣ��򷵻�0
		for (int i = 1; i <= 26; i++) {
			if (_chdrive(i) == 0) {
				if (result.size() > 0) result += ',';
				result += ('A' + i - 1);   //�õ��̷�
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
			finfo.HasNext = FALSE; //���ʲ��ˣ�û�к����ļ�
			lstCPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
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
			lstCPacket.push_back(CPacket(2, (BYTE*)&finfo, sizeof(finfo)));
			return -3;
		}
		int count = 0;
		do {
			FILEINFO finfo;
			finfo.IsDirectory = (fdata.attrib & _A_SUBDIR) != 0;  //�ж��Ƿ����ļ���
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
		ShellExecuteA(NULL, NULL, strPath.c_str(), NULL, NULL, SW_SHOWNORMAL); //����Ӧ���ļ�
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
			} while (rlen >= 1024);  //˵���˴λ������Ѿ������ˣ����Խ����´ζ�ȡ
			fclose(pFile);
		}
		//����֮��õ�һ���յ����ݰ���˵���Ѿ����
		lstCPacket.push_back(CPacket(4, NULL, 0));
		return 0;
	}
	int MouseEvent(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		MOUSEEV mouse;
		memcpy(&mouse, inPacket.strData.c_str(), sizeof(MOUSEEV));
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
			lstCPacket.push_back(CPacket(5, NULL, 0));
		return 0;
	}
	int SendScreen(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
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
			lstCPacket.push_back(CPacket(6, pData, nSize));
			GlobalUnlock(hMem);   //�����ڴ�
		}
		pStream->Release();
		GlobalFree(hMem);
		screen.ReleaseDC();
		return 0;
	}
	int LockMachine(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		//�����û��˶�ε����������������߳�
		if ((dlg.m_hWnd == NULL) || (dlg.m_hWnd == INVALID_HANDLE_VALUE)) {   //dlg Ϊ�գ���ʾ�Ի���û�д��� or �Ѿ�������
			//_beginthread(threadLockDlg, 0, NULL);   //����һ���߳�ȥ����  ��ϸƷϸƷ
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
		//mbstowcs(sPath, strPath.c_str(), strPath.size());  //���ֽ��ַ���ת���ɿ��ֽ��ַ���
		MultiByteToWideChar(CP_ACP, 0, strPath.c_str(), strPath.size(), sPath,
			sizeof(sPath) / sizeof(TCHAR));   //���ֽ��ַ���ת���ɿ��ֽ��ַ���
		DeleteFile(sPath);
		lstCPacket.push_back(CPacket(9, NULL, 0));
		return 0;
	}
	int TestConnect(std::list<CPacket>& lstCPacket, CPacket& inPacket) {
		lstCPacket.push_back(CPacket(1981, NULL, 0));
		return 0;
	}
};