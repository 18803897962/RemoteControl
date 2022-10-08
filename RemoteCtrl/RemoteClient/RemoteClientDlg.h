
// RemoteClientDlg.h: 头文件
//

#pragma once
#include "ClientSocket.h"
#include "StatusDlg.h"

#define WM_SEND_PACKET (WM_USER + 1)  //发送数据包的消息
// CRemoteClientDlg 对话框
class CRemoteClientDlg : public CDialogEx
{
// 构造
public:
	CRemoteClientDlg(CWnd* pParent = nullptr);	// 标准构造函数

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_REMOTECLIENT_DIALOG };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV 支持

private:
	//1 查看磁盘分区 2 查看指定目录下的文件 3 打开文件 4 下载文件 9删除文件 5鼠标操作 6 发送屏幕内容 7 锁机 8 解锁 1981 测试连接
	int SendCommandPacket(int nCmd,bool bAutoClose=true,BYTE* pData=NULL,size_t nLength=0);
	//返回值是命令号，如果小于0，则表示错误
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void LoadFileInfo();
	void LoadFileCurrent();
	static void threadEntryForDownFile(void* arg);
	void threadDownFile();
	static void threadEntryForWatchData(void* arg);  //静态函数不能访问this指针
	void threadWatchData();  //借此成员函数使用this指针
private:
	CImage m_image;   //图片缓存
	bool m_isFull;   //标记缓存是否已经被写入  true 表示有缓存 false 表示无缓存
// 实现
public:
	bool isFull() const{    //后缀加const表示该函数不会修改任何变量的值
		return m_isFull;
	}
	CImage& GetImage() {
		return m_image;
	}
	void SetImageStatus(bool isFull = false) {
		m_isFull = isFull;
	}
protected:
	HICON m_hIcon;
	CStatusDlg m_dlgStatus;
	bool m_isClosed;
	// 生成的消息映射函数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedBtnTest();
	afx_msg void OnIpnFieldchangedIpaddress1(NMHDR* pNMHDR, LRESULT* pResult);
	DWORD m_server_address;
	CString m_nPort;
	afx_msg void OnBnClickedBtnFileinfo();
	CTreeCtrl m_tree;
	afx_msg void OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult);
	// 显示文件
	CListCtrl m_List;
	afx_msg void OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnDownloadFile();
	afx_msg void OnDeleteFile();
	afx_msg void OnRunFile();
	afx_msg LRESULT OnSendPacket(WPARAM wParam, LPARAM lParam);   //消息函数
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
};
