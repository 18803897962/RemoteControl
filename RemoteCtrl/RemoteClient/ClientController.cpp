#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"
#include "Tools.h"
std::map<UINT, CClientController::MSGFUNC> CClientController::m_mapFunc;
CClientController* CClientController::m_instance = NULL;
CClientController::CHelper CClientController::m_helper;
//ͷ�ļ��еľ�̬����ֻ������
int CClientController::Invoke(CWnd*& pMainWnd)
{
	pMainWnd = &m_remoteDlg;
	return m_remoteDlg.DoModal();
}

int CClientController::InitController()
{
	m_hThread = (HANDLE)_beginthreadex(NULL, 0, 
		&CClientController::threadEntry, 
		this, 0, &m_nThreadID);   //m_nThreadID�õ��߳�ID��m_hThread�õ��߳̾��
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
// // 	UuidCreate(&uuid);   //ÿ�δ�����һ����һ����uuid
// // 	MSGINFO info(msg);
// 	//std::pair<std::map<UUID, MSG>::iterator, bool> pr=m_mapMessage.insert(std::pair<UUID, MSG>(uuid, msg));
// 	//auto pr = m_mapMessage.insert(std::pair<UUID, MSGINFO*>(uuid, &info));
// 	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
// 	if (hEvent == NULL) return -2;
// 	MSGINFO info(msg);
// 	PostThreadMessage(m_nThreadID, WM_SEND_MESSAGE,     
// 		(WPARAM)&info, (LPARAM)hEvent);   //������Ϣ
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
	m_remoteDlg.EndWaitCursor();//��������
	m_remoteDlg.MessageBox(_T("�������"), _T("���"));
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, NULL, strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		FILE* pfile = fopen(m_strLocal, "wb+");  //�ڱ��ؿ�һ���ļ�����д��
		if (pfile == NULL) {
			AfxMessageBox("�ļ�����ʧ�ܻ���Ȩ�������ļ�!");
			return -1;
		}
		int ret = SendCommandPacket(m_remoteDlg, 4, false,(BYTE*)(LPCTSTR)m_strRemote,m_strRemote.GetLength(),(WPARAM)pfile);
		if (ret < 0) {
			AfxMessageBox("ִ����������ʧ��");
			TRACE("ִ������ʧ�ܣ�ret��%d\r\n", ret);
			m_statusDlg.ShowWindow(SW_HIDE);
			
		}
		m_remoteDlg.BeginWaitCursor();
		m_statusDlg.m_info.SetWindowText(_T("����ִ����"));
		m_statusDlg.ShowWindow(SW_SHOW);   //��ʾ������ʾ
		m_statusDlg.CenterWindow(&m_remoteDlg);    //���þ���
		m_statusDlg.SetActiveWindow();
	}
	return 0;
}

void CClientController::StartWatchScreen()
{
	m_isClosed = false;
	//m_watchDlg.SetParent(&m_remoteDlg);
	HANDLE hTread = (HANDLE)_beginthread(CClientController::threadEntryForWatchScreen, 0, this);
	m_watchDlg.DoModal();   //���ó�ģ̬�Ի��򣬷�ֹ�ظ����Զ�̼�ذ�ť
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
{//���ܴ����첽���⵼�³������
	Sleep(50);
	ULONGLONG nTick = GetTickCount64();
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == false) {
			if (GetTickCount64() - nTick < 200) {
				Sleep(200 - DWORD(GetTickCount64()-nTick));//������50ms����ֹ���͹���Ƶ�����¿���
				TRACE("%d\r\n", GetTickCount64());
			}
			nTick = GetTickCount64();
			std::list<CPacket> lstPacks;
			int ret = SendCommandPacket(m_watchDlg.GetSafeHwnd(), 6, true, NULL, 0);
			//TODO:�����Ϣ��Ӧ����WM_SEND_PACK_ACK
			//TODO:���Ʒ���Ƶ��
			if (ret == 1) {
				TRACE("�ɹ���������ͼƬ����\r\n");
			}
			else {
				TRACE("��ȡͼƬʧ�� ret=%d\r\n", ret);
			}
		}
		Sleep(1);
	}
	TRACE("thread is closed %d\r\n", m_isClosed);
}


void CClientController::threadFunc()
{
	MSG msg;
	while (::GetMessage(&msg, NULL, 0, 0)) {  //�յ���Ϣ
		TranslateMessage(&msg);   //ת����Ϣ
		DispatchMessage(&msg);   //�ַ���Ϣ
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
	thiz->threadFunc();  //�����������̺߳���
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
