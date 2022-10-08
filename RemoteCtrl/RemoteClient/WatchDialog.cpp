﻿// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "WatchDialog.h"
#include "afxdialogex.h"
#include "RemoteClientDlg.h"

// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{

}

CWatchDialog::~CWatchDialog()
{
}

void CWatchDialog::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_WATCH, m_picture);
}


BEGIN_MESSAGE_MAP(CWatchDialog, CDialog)
	ON_WM_TIMER()
	ON_WM_LBUTTONDBLCLK()
	ON_WM_LBUTTONDOWN()
	ON_WM_LBUTTONUP()
	ON_WM_RBUTTONDBLCLK()
	ON_WM_RBUTTONDOWN()
	ON_WM_RBUTTONUP()
	ON_WM_MOUSEMOVE()
	ON_STN_CLICKED(IDC_WATCH, &CWatchDialog::OnStnClickedWatch)
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPointtoRemoteScreenPoint(CPoint& point)   //客户端坐标到屏幕坐标
{   // 客户端 800*450
	CRect ClientRect;
	ScreenToClient(&point);  //全局坐标到客户端坐标
	//本地坐标到远程坐标
	m_picture.GetWindowRect(ClientRect);   //得到客户端坐标
// 	int width0 = ClientRect.Width();
// 	int height0 = ClientRect.Height();
// 	int width = 1920, height = 1080;
// 	int x = point.x * width / width0;
// 	int y = point.x * height / height0;

	return CPoint(point.x * 1920 / ClientRect.Width(), point.x * 1080 / ClientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();

	// TODO:  在此添加额外的初始化
	SetTimer(0,45,NULL);  //设置定时器，50ms一次，不需要回调函数
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	if (nIDEvent == 0) {
		CRemoteClientDlg* pParent = (CRemoteClientDlg*)GetParent();  //得到父窗口
		if (pParent->isFull()) {  //缓存区满才显示
			CRect rect;
			m_picture.GetWindowRect(rect);
			//pParent->GetImage().BitBlt(m_picture.GetDC()->GetSafeHdc(),0,0,SRCCOPY);
			pParent->GetImage().StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0,rect.Width(),rect.Height(), SRCCOPY);
			m_picture.InvalidateRect(NULL);
			pParent->GetImage().Destroy();
			pParent->SetImageStatus();
		}
	}
	CDialog::OnTimer(nIDEvent);
}


void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	CPoint remote = UserPointtoRemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;  //左键
	event.nAction = 2;  //双击
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	
	CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	//CPoint remote = UserPointtoRemoteScreenPoint(point);
	////封装
	//MOUSEEV event;
	//event.ptXY = remote;
	//event.nButton = 0;  //左键
	//event.nAction = 4;  //弹起
	//CClientSocket* pClient = CClientSocket::getInstance();
	//CPacket pack(5, (BYTE*)&event, sizeof(event));
	//pClient->Send(pack);
	CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	CPoint remote = UserPointtoRemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;  //左键
	event.nAction = 3;  //双击
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	CPoint remote = UserPointtoRemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;  //左键
	event.nAction = 3;  //按下  //TODO:服务端对应修改
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	CPoint remote = UserPointtoRemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;  //左键
	event.nAction = 4;  //弹起
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	CPoint remote = UserPointtoRemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;  //左键
	event.nAction = 1;  //移动
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
	CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()   //鼠标点击事件
{
	CPoint point;
	GetCursorPos(&point);
	CPoint remote = UserPointtoRemoteScreenPoint(point);
	//封装
	MOUSEEV event;
	event.ptXY = remote;
	event.nButton = 0;  //左键
	event.nAction = 3;  //按下
	CClientSocket* pClient = CClientSocket::getInstance();
	CPacket pack(5, (BYTE*)&event, sizeof(event));
	pClient->Send(pack);
}
