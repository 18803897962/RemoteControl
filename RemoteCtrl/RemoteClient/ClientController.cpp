#include "pch.h"
#include "ClientController.h"
#include "ClientSocket.h"
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


int CClientController::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	if (pClient->InitSocket() == false) return false;
	HANDLE hEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
	CPacket pack(nCmd, pData, nLength,hEvent);
	CClientSocket::getInstance()->Send(pack);
	int cmd = DealCommand();
	TRACE("cmd:%d\r\n", cmd);
	if (bAutoClose == true)
		CloseSocket();
	return cmd;
}

int CClientController::DownFile(CString strPath)
{
	CFileDialog dlg(FALSE, NULL, strPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, NULL, &m_remoteDlg);
	if (dlg.DoModal() == IDOK) {
		m_strRemote = strPath;
		m_strLocal = dlg.GetPathName();
		m_hThreadDownload = (HANDLE)_beginthread(&CClientController::threadDownloadEntry, 0, this);
		if (WaitForSingleObject(m_hThreadDownload, 0) != WAIT_TIMEOUT) {
			//等待线程超时，说明线程启动成功，不超时，则表示启动失败
			TRACE("%s(%d):%s：线程启动失败\r\n", __FILE__, __LINE__, __FUNCTION__);
			return -1;
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
	while (!m_isClosed) {
		if (m_watchDlg.isFull() == false) {
			int ret = SendCommandPacket(6);
			if (ret == 6) {
				if (GetImage(m_remoteDlg.GetImage()) == 0) {
					m_watchDlg.SetImageStatus(true);
				}
				else
				{
					TRACE("获取图片失败\r\n");
				}
			}
			else {
				Sleep(1);    //发包不成功，避免得到实例之后发包之前出现网络故障，此时ret返回-1，for循环一直执行，导致CPU占用过度而卡死
			}
		}
		Sleep(1);

	}
}

void CClientController::threadDownloadFile()
{
	FILE* pfile = fopen(m_strLocal, "wb+");  //在本地开一个文件用于写入
	if (pfile == NULL) {
		AfxMessageBox("文件创建失败或无权限下载文件!");
		m_statusDlg.ShowWindow(SW_HIDE);
		m_remoteDlg.EndWaitCursor();   //鼠标光标结束
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	do 
	{
		int ret = SendCommandPacket(4, false,
			(BYTE*)(LPCTSTR)m_strRemote,
			m_strRemote.GetLength());
		if (ret < 0) {
			AfxMessageBox("执行下载命令失败");
			TRACE("执行下载失败，ret：%d\r\n", ret);
			m_statusDlg.ShowWindow(SW_HIDE);
			break;
		}
		long long nlength = *(long long*)pClient->GetPacket().strData.c_str();
		if (nlength == 0) {
			AfxMessageBox("文件长度为0或者无法读取文件！");
			m_statusDlg.ShowWindow(SW_HIDE);
			break;
		}
		long long nCount = 0;
		while (nCount < nlength) {
			ret = pClient->DealCommand();
			if (ret < 0) {
				AfxMessageBox("传输失败");
				TRACE("传输失败，ret:%d\r\n", ret);
				break;
			}
			fwrite(pClient->GetPacket().strData.c_str(), 1, pClient->GetPacket().strData.size(), pfile);  //将文件进行写入
			nCount += pClient->GetPacket().strData.size();
		}
	} while (false);
	fclose(pfile);
	pClient->CloseSocket();
	m_statusDlg.ShowWindow(SW_HIDE);
	m_remoteDlg.EndWaitCursor();//鼠标光标结束
	m_remoteDlg.MessageBox(_T("下载完成"), _T("完成"));
}

void CClientController::threadDownloadEntry(void* arg)
{
	CClientController* thiz = (CClientController*)arg;
	thiz->threadDownloadFile();
	_endthread();
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
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket* pPacket = (CPacket*)wParam;
	return pClient->Send(*pPacket);
}

LRESULT CClientController::OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	CClientSocket* pClient = CClientSocket::getInstance();
	char* pBuffer = (char*)wParam;
	return pClient->Send(pBuffer,int(lParam));
}

LRESULT CClientController::OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_statusDlg.ShowWindow(SW_SHOW);
}

LRESULT CClientController::OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam)
{
	return m_watchDlg.DoModal();
}
