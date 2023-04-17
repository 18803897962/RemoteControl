#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"
#include "Tools.h"
std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;
//头文件中的静态类型只是声明
int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, 
		&CClientController::threadEntry, 
		this, 0, &m_nThreadID);   //m_nThreadID得到线程ID，m_hThread得到线程句柄
	m_statusDlg.Create(IDD_DLG_STATUS, &m_remoteDlg);
	return 0;
}

CClientController* CClientController::getInstance()
{
	if (m_instance == NULL) {
		m_instance = new CClientController();
		TRACE("CClientController size =%d\r\n", sizeof(CClientController));
		struct 
		{
			UINT nMsg;
			MSGFUNC func;
		}MsgFunc[] = { 
			//{WM_SEND_PACK,&CClientController::OnSendPack},
			//{WM_SEND_DATA,&CClientController::OnSendData},
			{WM_SHOW_STATUS,&CClientController::OnShowStatus},
			{WM_SHOW_WATCH,&CClientController::OnShowWatcher},
			{(UINT)-1,NULL}
		};
		for (int i = 0; MsgFunc[i].nMsg != -1; i++) {
			m_mapFunc.insert(std::pair<UINT, MSGFUNC>
				(MsgFunc[i].nMsg, MsgFunc[i].func));
		}
	}
	return m_instance;
}

// LRESULT CClientController::SendMessage(MSG msg)
// {
// // 	UUID uuid;
// // 	UuidCreate(&uuid);   //每次创建出一个不一样的uuid
// // 	MSGINFO info(msg);
// 	//std::pair<std::map<UUID, MSG>::iterator, bool> pr=m_mapMessage.insert(std::pair<UUID, MSG>(uuid, msg));
// 	//auto pr = m_mapMessage.insert(std::pair<UUID, MSGINFO*>(uuid, &info));
// 	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
// 	if (hEvent == NULL) return -2;
// 	MSGINFO info(msg);
// 	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,     
// 		(WPARAM)&info, (LPARAM)hEvent);   //发送消息
// 	WaitForSingleObject(hEvent, INFINITE);
// 	CloseHandle(hEvent);
// 	return info.result;
// }


bool CClientController::SendCommandPacket(HWND hWnd,int nCmd, bool bAutoClose, BYTE* pData,
	size_t nLength, WPARAM wParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret= pClient->SendPacket(hWnd, CPacket(nCmd, pData, nLength), bAutoClose, wParam);
	return ret;
}

void CClientController::DownloadEnd()
{
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();//鼠标光标结束
	m_remoteDlg.MessageBox(_T("下载完成"), _T("完成"));
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, NULL, strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		FILE* pfile = fopen(m_strLocal, "wb+");  //在本地开一个文件用于写入
		if (pfile == NULL) {
			AfxMessageBox("文件创建失败或无权限下载文件!");
			return -1;
		}
		int ret = SendCommandPacket(m_remoteDlg, 4, false,(BYTE*)(LPCTSTR)m_strRemote,m_strRemote.GetLength(),(WPARAM)pfile);
		if (ret < 0) {
			AfxMessageBox("执行下载命令失败");
			TRACE("执行下载失败，ret：%d\r\n", ret);
			m_statusDlg.ShowWindow(SW_HIDE);
			
		}
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("命令执行中"));
		m_statusDlg.ShowWindow(SW_SHOW);   //提示窗口显示
		m_statusDlg.CenterWindow(&m_remoteDlg);    //设置居中
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	//m_watchDlg.SetParent(&m_remoteDlg);
	HANDLE hTread = (HANDLE)_beginthread(CClientController::threadEntryForWatchScreen, 0, this);
	m_watchDlg.DoModal();   //设置成模态对话框，防止重复点击远程监控按钮
	m_isClosed = true;
	WaitForSingleObject(hTread, 500);
}

void CClientController::threadEntryForWatchScreen(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadWatchScreen();
	_endthread();
}

void CClientController::threadWatchScreen()
{//可能存在异步问题导致程序崩溃
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == false) {
			if (GetTickCount64() - nTick < 200) {
				Sleep(200 - DWORD(GetTickCount64()-nTick));//休眠满50ms，防止发送过于频繁导致卡顿
				TRACE("%d\r\n", GetTickCount64());
			}
			nTick = GetTickCount64();
			std::list<CPacket> lstPacks;
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0);
			//TODO:添加消息响应函数WM_SEND_PACK_ACK
			//TODO:控制发送频率
			if (ret == 1) {
				TRACE("成功发送请求图片命令\r\n");
			}
			else {
				TRACE("获取图片失败 ret=%d\r\n", ret);
			}
		}
		Sleep(1);
	}
	TRACE("thread is closed %d\r\n", m_isClosed);
}


void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {  //收到消息
		TranslateMessage(&msg);   //转换消息
		DispatchMessage(&msg);   //分发消息
		if (msg.message == WM_SEND_MESSAGE) {
			MSGINFO* pMsg = (MSGINFO*)msg.wParam;
			HANDLE hEvent=(HANDLE)msg.lParam;
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				pMsg->result = (this->*it->second)(pMsg->msg.message, pMsg->msg.wParam, pMsg->msg.lParam);
			}
			else {
				pMsg->result = -1;
			}
			SetEvent(hEvent);
		}
		else {
			std::map<UINT, MSGFUNC>::iterator it = m_mapFunc.find(msg.message);
			if (it != m_mapFunc.end()) {
				(this->*it->second)(msg.message, msg.wParam, msg.lParam);
			}
		}
		
	}
}

unsigned __stdcall CClientController::threadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadFunc();  //启动真正的线程函数
	_endthreadex(0);
	return 0;
}

// LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
// {
// 	CClientSocket* pClient = CClientSocket::getInstance();
// 	CPacket* pPacket = (CPacket*)wParam;
// 	return pClient->Send(*pPacket);
// }

 //LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
 //{
 //	CClientSocket* pClient = CClientSocket::getInstance();
 //	char* pBuffer = (char*)wParam;
 //	return pClient->Send(pBuffer,int(lParam));
 //}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
