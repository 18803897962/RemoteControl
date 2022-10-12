#include "pch.h"
#include "ClientController.h"

std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
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
		struct 
		{
			UINT nMsg;
			MSGFUNC func;
		}MsgFunc[] = { 
			{WM_SEND_PACK,&CClientController::OnSendPack},
			{WM_SEND_DATA,&CClientController::OnSendData},
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

LRESULT CClientController::SendMessage(MSG msg)
{
// 	UUID uuid;
// 	UuidCreate(&uuid);   //每次创建出一个不一样的uuid
// 	MSGINFO info(msg);
	//std::pair<std::map<UUID, MSG>::iterator, bool> pr=m_mapMessage.insert(std::pair<UUID, MSG>(uuid, msg));
	//auto pr = m_mapMessage.insert(std::pair<UUID, MSGINFO*>(uuid, &info));
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	if (hEvent == NULL) return -2;
	MSGINFO info(msg);
	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,     
		(WPARAM)&info, (LPARAM)hEvent);   //发送消息
	WaitForSingleObject(hEvent, -1);
	return info.result;
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

LRESULT CClientController::OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return LRESULT();
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
