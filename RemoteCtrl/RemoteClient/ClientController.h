#pragma once
#include "ClientSocket.h"
#include "WatchDialog.h"
#include "StatusDlg.h"
#include "RemoteClientDlg.h"
#include "resource.h"
#include "ClientSocket.h"
#include "Tools.h"
#include <map>
//#define WM_SEND_PACK (WM_USER+1) //���Ͱ�����
//#define WM_SEND_DATA (WM_USER+2)  //��������
#define WM_SHOW_STATUS (WM_USER+3)//չʾ״̬
#define WM_SHOW_WATCH (WM_USER+4)  //Զ�̼��
#define WM_SEND_MESSAGE (WM_USER+0x1000)  //�Զ�����Ϣ����
class CClientController
{
public:
	int Invoke(CWnd*& pMainWnd);  //����
	int InitController(); //��ʼ��
	static CClientController* getInstance();  //��ȡ��������
	LRESULT SendMessage(MSG msg);
	//���������ַ
	void UpdateAddress(DWORD nIP, int nPort) {
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}
	//1 �鿴���̷��� 2 �鿴ָ��Ŀ¼�µ��ļ� 3 ���ļ� 4 �����ļ� 9ɾ���ļ� 5������ 6 ������Ļ���� 7 ���� 8 ���� 1981 ��������
	//����ֵ������ţ����С��0�����ʾ����
	int SendCommandPacket(int nCmd,
		bool bAutoClose = true,
		BYTE* pData = NULL,
		size_t nLength = 0,
		std::list<CPacket>* plstPack = NULL
	);  //Ӧ������
	int GetImage(CImage& image) {
		CClientSocket* pClient = CClientSocket::getInstance();
		return CTools::BytestoImage(image, pClient->GetPacket().strData);
	}
	int DownFile(CString strPath);
	void StartWatchScreen();
protected:
	static void threadEntryForWatchScreen(void* arg);
	void threadWatchScreen();
	void threadDownloadFile();
	static void threadDownloadEntry(void* arg);
	static void releaseInstance() {
		if (m_instance != NULL) {
			TRACE("CClientController delete called\r\n");
			delete m_instance;
			TRACE("CClientController delete\r\n");
			m_instance = NULL;
		}
	}
	CClientController():
		m_statusDlg(&m_remoteDlg),
		m_watchDlg(&m_remoteDlg)//���ø�����
	{
		m_isClosed = true;
		m_hThreadwatch = INVALID_HANDLE_VALUE;
		m_hThreadDownload = INVALID_HANDLE_VALUE;
		m_hThread = INVALID_HANDLE_VALUE;
		m_nThreadID = -1;
	}
	~CClientController() {
		WaitForSingleObject(m_hThread,100);
	}
	void threadFunc();
	static unsigned __stdcall threadEntry(void* arg);
	//LRESULT OnSendPack(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnSendData(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowStatus(UINT nMsg, WPARAM wParam, LPARAM lParam);
	LRESULT OnShowWatcher(UINT nMsg, WPARAM wParam, LPARAM lParam);
private:
	typedef struct MsgInfo{
		MSG msg;
		LRESULT result;
		MsgInfo(MSG m) {
			result = 0;
			memcpy(&msg, &m, sizeof(MSG));
		}
		MsgInfo() {
			result = 0;
			memset(&msg, 0, sizeof(msg));
		}
		MsgInfo(const MsgInfo& m) {
			result = m.result;
			memcpy(&msg, &m.msg, sizeof(MSG));
		}
		MsgInfo& operator=(const MsgInfo& m) {
			if (this != &m) {
				result = m.result;
				memcpy(&msg, &m.msg, sizeof(MSG));
			}
			return *this;
		}
	}MSGINFO;
	typedef LRESULT(CClientController::* MSGFUNC)(UINT nMsg, WPARAM wParam, LPARAM lParam);
	static std::map<UINT, MSGFUNC> m_mapFunc;
	CWatchDialog m_watchDlg;
	CRemoteClientDlg m_remoteDlg;
	CStatusDlg m_statusDlg;
	HANDLE m_hThread;
	HANDLE m_hThreadDownload;  //�����̵߳ľ��������ǿ�ƹر�����
	HANDLE m_hThreadwatch;
	bool m_isClosed;//�����Ƿ�ر�
	CString m_strRemote;  //�����ļ���Զ��·��
	CString m_strLocal;  //�����ļ��ı��ر���·��
	unsigned m_nThreadID;
	static CClientController* m_instance;
	class CHelper {
	public:
		CHelper() {
			
		}
		~CHelper() {
			CClientController::releaseInstance();
		}
	};
	static CHelper m_helper;   //����һ����̬���Ա�����ڵ����亯��
};

