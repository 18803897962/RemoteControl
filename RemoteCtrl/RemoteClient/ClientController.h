#pragma once
#include "ClientSocket.h"
#include "WatchDialog.h"
#include "StatusDlg.h"
#include "RemoteClientDlg.h"
#include "resource.h"
#include "ClientSocket.h"
#include "Tools.h"
#include <map>
//#define WM_SEND_PACK (WM_USER+1) //发送包数据
//#define WM_SEND_DATA (WM_USER+2)  //发送数据
#define WM_SHOW_STATUS (WM_USER+3)//展示状态
#define WM_SHOW_WATCH (WM_USER+4)  //远程监控
#define WM_SEND_MESSAGE (WM_USER+0x1000)  //自定义消息处理
class CClientController
{
public:
	int Invoke(CWnd*& pMainWnd);  //启动
	int InitController(); //初始化
	static CClientController* getInstance();  //获取单例对象
	LRESULT SendMessage(MSG msg);
	//更新网络地址
	void UpdateAddress(DWORD nIP, int nPort) {
		CClientSocket::getInstance()->UpdateAddress(nIP, nPort);
	}
	int DealCommand() {
		return CClientSocket::getInstance()->DealCommand();
	}
	void CloseSocket() {
		CClientSocket::getInstance()->CloseSocket();
	}
	//1 查看磁盘分区 2 查看指定目录下的文件 3 打开文件 4 下载文件 9删除文件 5鼠标操作 6 发送屏幕内容 7 锁机 8 解锁 1981 测试连接
	//返回值是命令号，如果小于0，则表示错误
	int SendCommandPacket(int nCmd,
		bool bAutoClose = true,
		BYTE* pData = NULL,
		size_t nLength = 0,
		std::list<CPacket>* plstPack = NULL
	);  //应答结果包
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
		m_watchDlg(&m_remoteDlg)//设置父窗口
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
	HANDLE m_hThreadDownload;  //下载线程的句柄，用于强制关闭下载
	HANDLE m_hThreadwatch;
	bool m_isClosed;//监视是否关闭
	CString m_strRemote;  //下载文件的远程路径
	CString m_strLocal;  //下载文件的本地保存路径
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
	static CHelper m_helper;   //声明一个静态类成员，用于调用其函数
};

