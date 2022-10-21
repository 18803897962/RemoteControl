
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"
#include "ClientController.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif
#include "WatchDialog.h"


// 用于应用程序“关于”菜单项的 CAboutDlg 对话框

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg();

// 对话框数据
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV 支持

// 实现
protected:
	DECLARE_MESSAGE_MAP()
public:
};

CAboutDlg::CAboutDlg() : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()


// CRemoteClientDlg 对话框



CRemoteClientDlg::CRemoteClientDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_REMOTECLIENT_DIALOG, pParent)
	, m_server_address(0)
	, m_nPort(_T(""))
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRemoteClientDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_IPAddress(pDX, IDC_IPADDRESS_SERV, m_server_address);
	DDX_Text(pDX, IDC_EDIT_PORT, m_nPort);
	DDX_Control(pDX, IDC_TREE_DIR, m_tree);
	DDX_Control(pDX, IDC_LIST_FILE, m_List);
}


BEGIN_MESSAGE_MAP(CRemoteClientDlg, CDialogEx)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BTN_TEST, &CRemoteClientDlg::OnBnClickedBtnTest)
	//ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS1, &CRemoteClientDlg::OnIpnFieldchangedIpaddress1)
	ON_BN_CLICKED(IDC_BTN_FILEINFO, &CRemoteClientDlg::OnBnClickedBtnFileinfo)
	ON_NOTIFY(NM_DBLCLK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMDblclkTreeDir)
	ON_NOTIFY(NM_CLICK, IDC_TREE_DIR, &CRemoteClientDlg::OnNMClickTreeDir)
	ON_NOTIFY(NM_RCLICK, IDC_LIST_FILE, &CRemoteClientDlg::OnNMRClickListFile)
	ON_COMMAND(ID_DOWNLOAD_FILE, &CRemoteClientDlg::OnDownloadFile)
	ON_COMMAND(ID_DELETE_FILE, &CRemoteClientDlg::OnDeleteFile)
	ON_COMMAND(ID_RUN_FILE, &CRemoteClientDlg::OnRunFile)
	ON_BN_CLICKED(IDC_BTN_START_WATCH, &CRemoteClientDlg::OnBnClickedBtnStartWatch)
	ON_WM_TIMER()
	ON_NOTIFY(IPN_FIELDCHANGED, IDC_IPADDRESS_SERV, &CRemoteClientDlg::OnIpnFieldchangedIpaddressServ)
	ON_EN_CHANGE(IDC_EDIT_PORT, &CRemoteClientDlg::OnEnChangeEditPort)
	ON_MESSAGE(WM_SEND_PACK_ACK, &CRemoteClientDlg::OnSendPackAck)  //添加自定义消息
END_MESSAGE_MAP()


// CRemoteClientDlg 消息处理程序

BOOL CRemoteClientDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// 将“关于...”菜单项添加到系统菜单中。

	// IDM_ABOUTBOX 必须在系统命令范围内。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != nullptr)
	{
		BOOL bNameValid;
		CString strAboutMenu;
		bNameValid = strAboutMenu.LoadString(IDS_ABOUTBOX);
		ASSERT(bNameValid);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// 设置此对话框的图标。  当应用程序主窗口不是对话框时，框架将自动
	//  执行此操作
	SetIcon(m_hIcon, TRUE);			// 设置大图标
	SetIcon(m_hIcon, FALSE);		// 设置小图标

	// TODO: 在此添加额外的初始化代码
	UpdateData(TRUE);
	m_server_address = 0xC0A8016A;  //192.168.1.104
	m_nPort = _T("9527");
	UpdateData(FALSE);
	UpdateData();
	CClientController::getInstance()->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
	m_dlgStatus.Create(IDD_DLG_STATUS, this);
	m_dlgStatus.ShowWindow(SW_HIDE);
	return TRUE;  // 除非将焦点设置到控件，否则返回 TRUE
}

void CRemoteClientDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialogEx::OnSysCommand(nID, lParam);
	}
}

// 如果向对话框添加最小化按钮，则需要下面的代码
//  来绘制该图标。  对于使用文档/视图模型的 MFC 应用程序，
//  这将由框架自动完成。

void CRemoteClientDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 用于绘制的设备上下文

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// 使图标在工作区矩形中居中
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// 绘制图标
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

//当用户拖动最小化窗口时系统调用此函数取得光标
//显示。
HCURSOR CRemoteClientDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRemoteClientDlg::OnBnClickedBtnTest()
{
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),1981);
}


void CRemoteClientDlg::OnIpnFieldchangedIpaddress1(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
}


void CRemoteClientDlg::OnBnClickedBtnFileinfo()  
{
	// TODO: 查看文件信息
	std::list<CPacket> lstPacks;
	int ret= CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 1,true,NULL,0);  //获取磁盘分区信息
	if (ret == 0) {
		MessageBox(_T("命令处理失败!"));
		return;
	}
}

CString CRemoteClientDlg::GetPath(HTREEITEM hTree) {
	CString strRet,strTmp;
	do {
		strTmp = m_tree.GetItemText(hTree);
		strRet = strTmp+'\\'+strRet;     //保存获得的文件路径
		hTree = m_tree.GetParentItem(hTree);
	} while (hTree!=NULL);
	return strRet;
}

void CRemoteClientDlg::DeleteTreeChildrenItem(HTREEITEM hTree) {  //函数实现删除当前页文件结点，避免多次按下按钮导致文件树一直增长
	HTREEITEM hSub = NULL;
	do 
	{
		hSub = m_tree.GetChildItem(hTree);
		if (hSub!= NULL) m_tree.DeleteItem(hSub);
	} while (hSub!=NULL);
}

void CRemoteClientDlg::LoadFileInfo() {
	CPoint ptMouse;
	GetCursorPos(&ptMouse);
	m_tree.ScreenToClient(&ptMouse);
	HTREEITEM hTreeSelected = m_tree.HitTest(ptMouse, 0);  //判断鼠标是否点击
	if (hTreeSelected == NULL)
		return;
	if (m_tree.GetChildItem(hTreeSelected) == NULL) return;
	DeleteTreeChildrenItem(hTreeSelected);
	m_List.DeleteAllItems();
	CString strPath = GetPath(hTreeSelected);   //鼠标点中某个结点
	std::list<CPacket> lstPack;
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength(),(WPARAM)hTreeSelected);
	if(lstPack.size()>0){
		TRACE("lstPack size=%d\r\n", lstPack.size());
		std::list<CPacket>::iterator it=lstPack.begin();
		for (; it != lstPack.end(); it++) {
			PFILEINFO pInfo = (PFILEINFO)(*it).strData.c_str();
			if(pInfo->HasNext==false) continue;
			TRACE("<name>%s   <isdir>%d\r\n", pInfo->szFileName, pInfo->IsDirectory);
			if (pInfo->IsDirectory) {
				if ((CString(pInfo->szFileName) == ".") || (CString(pInfo->szFileName) == "..")) {
					continue;
				}
				HTREEITEM hTemp = m_tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
				m_tree.InsertItem("", hTemp, TVI_LAST);
			}
			else {
				m_List.InsertItem(0, pInfo->szFileName);
			}
		}
	}
}
void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	int nCmd = CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 2, false, (BYTE*)(LPCTSTR)strPath);
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();   //获取当前结点信息

	while (pInfo->HasNext) {
		TRACE("<name>%s   <isdir>%d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = CClientController::getInstance()->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	CClientController::getInstance()->CloseSocket();
}
void CRemoteClientDlg::OnNMDblclkTreeDir(NMHDR* pNMHDR, LRESULT* pResult)  //双击鼠标左键的事件
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMClickTreeDir(NMHDR* pNMHDR, LRESULT* pResult)
{
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	LoadFileInfo();
}


void CRemoteClientDlg::OnNMRClickListFile(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMITEMACTIVATE pNMItemActivate = reinterpret_cast<LPNMITEMACTIVATE>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	*pResult = 0;
	CPoint ptMouse,ptList;
	GetCursorPos(&ptMouse);  //得到鼠标坐标
	ptList = ptMouse;
	m_List.ScreenToClient(&ptList);     //坐标转换，屏幕坐标转换成客户端坐标
	int ListSelected=m_List.HitTest(ptList);
	if (ListSelected < 0) return;
	CMenu menu;
	menu.LoadMenu(IDR_MENU_RCLICK);    //加载一个文件菜单资源
	CMenu* pPopup=menu.GetSubMenu(0);  //取到其第一个子菜单
	if (pPopup != NULL) {
		pPopup->TrackPopupMenu(TPM_LEFTALIGN|TPM_RIGHTBUTTON,ptMouse.x,ptMouse.y,this);
	}
}


void CRemoteClientDlg::OnDownloadFile()  //下载文件
{
	int nListSelected = m_List.GetSelectionMark();
	CString strFile = m_List.GetItemText(nListSelected, 0);
	HTREEITEM hSelected = m_tree.GetSelectedItem();
	strFile = GetPath(hSelected) + strFile;
	int ret=CClientController::getInstance()->DownFile(strFile);
	if (ret != 0) {
		MessageBox("下载失败！\n");
		TRACE("下载失败 ret=%d\r\n", ret);
	}
}


void CRemoteClientDlg::OnDeleteFile() //删除文件
{
	HTREEITEM hSelected = m_tree.GetSelectedItem();   //得到当前已选定的项
	CString strPath = GetPath(hSelected);   //得到其路径
	int nSelected = m_List.GetSelectionMark();  //得到选中的地方的索引号
	CString strFile = m_List.GetItemText(nSelected, 0);  //得到文件名
	strFile = strPath + strFile;
	int ret = CClientController::getInstance()->SendCommandPacket
	(GetSafeHwnd(), 9, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("文件删除失败");
	}
	LoadFileCurrent();  
}


void CRemoteClientDlg::OnRunFile()   //打开文件
{
	HTREEITEM hSelected = m_tree.GetSelectedItem();   //得到当前已选定的项
	CString strPath = GetPath(hSelected);   //得到其路径
	int nSelected = m_List.GetSelectionMark();  //得到选中的地方的索引号
	CString strFile = m_List.GetItemText(nSelected, 0);  //得到文件名
	strFile = strPath + strFile;
	int ret= CClientController::getInstance()->SendCommandPacket
	(GetSafeHwnd(), 3,true,(BYTE*)(LPCTSTR)strFile,strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("文件打开失败");
	}

}


void CRemoteClientDlg::OnBnClickedBtnStartWatch()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::getInstance()->StartWatchScreen();
}


void CRemoteClientDlg::OnTimer(UINT_PTR nIDEvent)
{
	CDialogEx::OnTimer(nIDEvent);
}


void CRemoteClientDlg::OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult)
{
	LPNMIPADDRESS pIPAddr = reinterpret_cast<LPNMIPADDRESS>(pNMHDR);
	// TODO: 在此添加控件通知处理程序代码
	UpdateData();
	CClientController::getInstance()->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
	*pResult = 0;
}


void CRemoteClientDlg::OnEnChangeEditPort()
{
	// TODO:  如果该控件是 RICHEDIT 控件，它将不
	// 发送此通知，除非重写 CDialogEx::OnInitDialog()
	// 函数并调用 CRichEditCtrl().SetEventMask()，
	// 同时将 ENM_CHANGE 标志“或”运算到掩码中。

	// TODO:  在此添加控件通知处理程序代码
	UpdateData();
	CClientController::getInstance()->UpdateAddress(m_server_address, atoi((LPCTSTR)m_nPort));
}

LRESULT CRemoteClientDlg::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if ((lParam == -1) || (lParam == -2)) {//出错

	}
	else if (lParam == 1) {
		//接收完毕,对方关闭了套接字或者网络设备异常
	}
	else if(lParam == 0) {//正常接收
		CPacket* pPacket = (CPacket*)wParam;
		CPacket& head = *pPacket;
		if (pPacket != NULL) {
			switch (pPacket->sCmd)
			{
			case 1://获取磁盘驱动信息
			{
				std::string drivers = head.strData;
				std::string dr;
				m_tree.DeleteAllItems();  //清空
				for (size_t i = 0; i < drivers.size(); i++) {   //文件数的插入
					if (drivers[i] == ',') {
						dr += ':';
						HTREEITEM hTemp = m_tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
						m_tree.InsertItem(NULL, hTemp, TVI_LAST);
						dr.clear();
						continue;
					}
					dr += drivers[i];
					//if ((i == drivers.size() - 1) && !dr.empty()) {
					//	dr += ':';
					//	HTREEITEM hTemp = m_tree.InsertItem(dr.c_str(), TVI_ROOT, TVI_LAST);
					//	m_tree.InsertItem(NULL, hTemp, TVI_LAST);
					//	dr.clear();
					//}
				}
			}
				break;
			case 2://获取文件信息
			{
				PFILEINFO pInfo = (PFILEINFO)head.strData.c_str();
				if (pInfo->HasNext == false) break;
				TRACE("<name>%s   <isdir>%d\r\n", pInfo->szFileName, pInfo->IsDirectory);
				if (pInfo->IsDirectory) {
					if ((CString(pInfo->szFileName) == ".") || (CString(pInfo->szFileName) == "..")) {
						break;
					}
					HTREEITEM hTemp = m_tree.InsertItem(pInfo->szFileName, (HTREEITEM)lParam, TVI_LAST);
					m_tree.InsertItem("", hTemp, TVI_LAST);
				}
				else {
					m_List.InsertItem(0, pInfo->szFileName);
				}
			}
				break;
			case 3:
				TRACE("run file successfully\r\n");
				break;
			case 4:
			{
				static LONGLONG length = 0,index=0;
				if (length == 0) {//说明是第一个包
					length = *(LONGLONG*)head.strData.c_str();
					AfxMessageBox("文件长度为0或者无法读取文件！");
					CClientController::getInstance()->DownloadEnd();
					break;
				}
				else if (length > 0 && (index >= length)) {//文件已经接收完成 index表示已经接收的大小 length表示每次接收的大小
					fclose((FILE*)lParam);
					length = 0;
					index = 0;
					CClientController::getInstance()->DownloadEnd();
				}
				else {
					FILE* pFile = (FILE*)lParam;
					fwrite(head.strData.c_str(), 1, head.strData.size(), pFile);
					index += head.strData.size();
				}
			}
				break;
			case 9:
				TRACE("delete file done\r\n");
				break;
			case 1981:
				TRACE("test connection successfully\r\n");
				break;
			default:
				TRACE("unknow data received! cmd=%d\r\n",head.sCmd);
				break;
			}
		}
	}
	return 0;
}
