
// RemoteClientDlg.cpp: 实现文件
//

#include "pch.h"
#include "framework.h"
#include "RemoteClient.h"
#include "RemoteClientDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


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

int CRemoteClientDlg::SendCommandPacket(int nCmd, bool bAutoClose, BYTE* pData, size_t nLength)
{
	UpdateData();
	CClientSocket* pClient = CClientSocket::getInstance();
	bool ret = pClient->InitSocket(m_server_address, atoi((LPCTSTR)m_nPort));   //将ip地址和端口地址传入
	//TODO：返回值处理
	if (ret == false) {
		AfxMessageBox("网络初始化失败");
		return -1;
	}
	CPacket pack(nCmd, pData, nLength);
	ret = pClient->Send(pack);
	TRACE("Send ret:%d\r\n", ret);
	int cmd = pClient->DealCommand();
	TRACE("cmd:%d\r\n", cmd);
	TRACE("command:%d\r\n", pClient->GetPacket().sCmd);
	if(bAutoClose==true)
		pClient->CloseSocket();
	return cmd;
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
	m_server_address = 0x7F000001;
	m_nPort = _T("9527");
	UpdateData(FALSE);
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
	SendCommandPacket(1981);
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
	int ret=SendCommandPacket(1);  //获取磁盘分区信息
	if (ret == -1) {
		MessageBox(_T("命令处理失败!"));
		return;
	}
	CClientSocket* pClient = CClientSocket::getInstance();
	std::string drivers=pClient->GetPacket().strData;
	std::string dr;
	m_tree.DeleteAllItems();  //清空
	for (size_t i = 0; i < drivers.size(); i++) {   //文件数的插入
		if (drivers[i] == ',') {
			dr += ':';
			HTREEITEM hTemp = m_tree.InsertItem(dr.c_str(),TVI_ROOT,TVI_LAST);
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
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();   //获取当前结点信息
	CClientSocket* pClient = CClientSocket::getInstance();

	while (pInfo->HasNext) {
		TRACE("<name>%s   <isdir>%d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (pInfo->IsDirectory) {
			if ((CString(pInfo->szFileName) == ".") || (CString(pInfo->szFileName) == "..")) {
				int cmd = pClient->DealCommand();
				TRACE("ack:%d\r\n", cmd);
				if (cmd < 0) break;
				pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
				continue;
			}
			HTREEITEM hTemp = m_tree.InsertItem(pInfo->szFileName, hTreeSelected, TVI_LAST);
			m_tree.InsertItem("", hTemp, TVI_LAST);
		}
		else {
			m_List.InsertItem(0,pInfo->szFileName);
		}
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
}
void CRemoteClientDlg::LoadFileCurrent()
{
	HTREEITEM hTree = m_tree.GetSelectedItem();
	CString strPath = GetPath(hTree);
	m_List.DeleteAllItems();
	int nCmd = SendCommandPacket(2, false, (BYTE*)(LPCTSTR)strPath, strPath.GetLength());
	PFILEINFO pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();   //获取当前结点信息
	CClientSocket* pClient = CClientSocket::getInstance();

	while (pInfo->HasNext) {
		TRACE("<name>%s   <isdir>%d\r\n", pInfo->szFileName, pInfo->IsDirectory);
		if (!pInfo->IsDirectory) {
			m_List.InsertItem(0, pInfo->szFileName);
		}
		int cmd = pClient->DealCommand();
		TRACE("ack:%d\r\n", cmd);
		if (cmd < 0) break;
		pInfo = (PFILEINFO)CClientSocket::getInstance()->GetPacket().strData.c_str();
	}
	pClient->CloseSocket();
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
	CString strFile=m_List.GetItemText(nListSelected, 0);
	CFileDialog dlg(FALSE,NULL, strFile,OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT,NULL,this);
	if (dlg.DoModal() == IDOK) {
		FILE* pfile = fopen(dlg.GetPathName(), "wb+");  //在本地开一个文件用于写入
		if (pfile == NULL) {
			AfxMessageBox("文件创建失败或无权限下载文件!");
			return;
		}
		HTREEITEM hSelected = m_tree.GetSelectedItem();
		strFile = GetPath(hSelected) + strFile;
		TRACE("%s\r\n", LPCTSTR(strFile));
		CClientSocket* pClient = CClientSocket::getInstance();
		do 
		{
			int ret = SendCommandPacket(4, false, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
			if (ret < 0) {
				AfxMessageBox("执行下载命令失败");
				TRACE("执行下载失败，ret：%d\r\n", ret);
				break;
			}
			long long nlength = *(long long*)pClient->GetPacket().strData.c_str();
			if (nlength == 0) {
				AfxMessageBox("文件长度为0或者无法读取文件！");
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
	}
}


void CRemoteClientDlg::OnDeleteFile() //删除文件
{
	HTREEITEM hSelected = m_tree.GetSelectedItem();   //得到当前已选定的项
	CString strPath = GetPath(hSelected);   //得到其路径
	int nSelected = m_List.GetSelectionMark();  //得到选中的地方的索引号
	CString strFile = m_List.GetItemText(nSelected, 0);  //得到文件名
	strFile = strPath + strFile;
	int ret = SendCommandPacket(9, true, (BYTE*)(LPCTSTR)strFile, strFile.GetLength());
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
	int ret=SendCommandPacket(3,true,(BYTE*)(LPCTSTR)strFile,strFile.GetLength());
	if (ret < 0) {
		AfxMessageBox("文件打开失败");
	}

}
