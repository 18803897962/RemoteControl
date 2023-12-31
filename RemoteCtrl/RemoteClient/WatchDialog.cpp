﻿// WatchDialog.cpp: 实现文件
//

#include "pch.h"
#include "RemoteClient.h"
#include "WatchDialog.h"
#include "afxdialogex.h"
#include "ClientController.h"
#include "Tools.h"
// CWatchDialog 对话框

IMPLEMENT_DYNAMIC(CWatchDialog, CDialog)

CWatchDialog::CWatchDialog(CWnd* pParent /*=nullptr*/)
	: CDialog(IDD_DLG_WATCH, pParent)
{
	m_isFull = false;
	m_nObjHeight = -1;
	m_nObjWidth = -1;
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
	ON_BN_CLICKED(IDC_BTN_LOCK, &CWatchDialog::OnBnClickedBtnLock)
	ON_BN_CLICKED(IDC_BTN_UNLOCK, &CWatchDialog::OnBnClickedBtnUnlock)
	ON_MESSAGE(WM_SEND_PACK_ACK,&CWatchDialog::OnSendPackAck)  //添加自定义消息
END_MESSAGE_MAP()


// CWatchDialog 消息处理程序


CPoint CWatchDialog::UserPointtoRemoteScreenPoint(CPoint& point,bool isScreen)   //客户端坐标到屏幕坐标
{   // 客户端 800*450
	CRect ClientRect;

	if(!isScreen) 
		ClientToScreen(&point);   //客户端坐标转为屏幕坐标,相对于屏幕左上角的绝对坐标
		//监视对话框内的坐标转换为相对应客户端电脑左上角的坐标
		
		m_picture.ScreenToClient(&point);  //转换为客户区域坐标（相对于picture控件左上角的坐标）
		//客户端电脑左上角的坐标转换为picture控件坐标
	//本地坐标到远程坐标
	m_picture.GetWindowRect(ClientRect);   //得到客户端坐标
// 	int width0 = ClientRect.Width();
// 	int height0 = ClientRect.Height();
// 	int width = 1920, height = 1080;
// 	int x = point.x * width / width0;
// 	int y = point.y * height / height0;

	return CPoint(point.x * m_nObjWidth / ClientRect.Width(), point.y * m_nObjHeight / ClientRect.Height());
}

BOOL CWatchDialog::OnInitDialog()
{
	CDialog::OnInitDialog();
	// TODO:  在此添加额外的初始化
	m_isFull = false;
	//SetTimer(0,45,NULL);  //设置定时器，50ms一次，不需要回调函数
	return TRUE;  // return TRUE unless you set the focus to a control
				  // 异常: OCX 属性页应返回 FALSE
}


void CWatchDialog::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//if (nIDEvent == 0) {
	//	CClientController* pControl = CClientController::getInstance();
	//	if (m_isFull) {  //缓存区满才显示
	//		CRect rect;
	//		m_picture.GetWindowRect(rect);
	//		m_nObjWidth = m_image.GetWidth();
	//		m_nObjHeight = m_image.GetHeight();
	//		m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
	//		m_picture.InvalidateRect(NULL);
	//		TRACE("图片：%08X\r\n", (HBITMAP)m_image);
	//		m_image.Destroy();
	//		m_isFull = false;
	//		TRACE("更新图片完成  %d   %d  \r\n", m_nObjHeight, m_nObjWidth);
	//	}
	//}
	//CDialog::OnTimer(nIDEvent);
}


LRESULT CWatchDialog::OnSendPackAck(WPARAM wParam, LPARAM lParam)
{
	if ((lParam == -1)||(lParam==-2)) {//出错

	}
	else if (lParam == 1) {
		//接收完毕,对方关闭了套接字或者网络设备异常
	}
	else{//正常接收
		CPacket* pPacket = (CPacket*)wParam;
		if (pPacket != NULL) {
			CPacket head = *(CPacket*)wParam;
			switch (head.sCmd)
			{
			case 6:
			{
				CTools::BytestoImage(m_image, head.strData);
				CRect rect;
				m_picture.GetWindowRect(rect);
				m_nObjWidth = m_image.GetWidth();
				m_nObjHeight = m_image.GetHeight();
				m_image.StretchBlt(m_picture.GetDC()->GetSafeHdc(), 0, 0, rect.Width(), rect.Height(), SRCCOPY);
				m_picture.InvalidateRect(NULL);
				TRACE("图片：%08X\r\n", (HBITMAP)m_image);
				m_image.Destroy();
				m_isFull = false;
				TRACE("更新图片完成  %d   %d  \r\n", m_nObjHeight, m_nObjWidth);
				break;
			}
			case 5:
				TRACE("远程端应答鼠标操作\r\n");
				break;
			case 7:
				TRACE("锁机命令应答\r\n");
				break;
			case 8:
			default:
				break;
			}
		}
	}
	return 0;
}

void CWatchDialog::OnLButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPointtoRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 1;  //双击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(),5, true, (BYTE*)&event, sizeof(event));
	}
		CDialog::OnLButtonDblClk(nFlags, point);
}


void CWatchDialog::OnLButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPointtoRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 2;  //按下
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
		CDialog::OnLButtonDown(nFlags, point);
}


void CWatchDialog::OnLButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPointtoRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 3;  //弹起
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
		CDialog::OnLButtonUp(nFlags, point);
}


void CWatchDialog::OnRButtonDblClk(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPointtoRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  //右键
		event.nAction = 1;  //双击
		
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));
	}
		CDialog::OnRButtonDblClk(nFlags, point);
}


void CWatchDialog::OnRButtonDown(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPointtoRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  //右键
		event.nAction = 2;  //按下  //TODO:服务端对应修改
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));

	}
		CDialog::OnRButtonDown(nFlags, point);
}


void CWatchDialog::OnRButtonUp(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPointtoRemoteScreenPoint(point);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 1;  //右键
		event.nAction = 3;  //弹起
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));

	}
		CDialog::OnRButtonUp(nFlags, point);
}


void CWatchDialog::OnMouseMove(UINT nFlags, CPoint point)
{
	// TODO: 在此添加消息处理程序代码和/或调用默认值
	//坐标转换
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint remote = UserPointtoRemoteScreenPoint(point);
		//封装
		MOUSEEV event;   //客户端坐标
		event.ptXY = remote;
		event.nButton = 8;  //没有按键  error 4
		event.nAction = 0;  //移动   error
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));

	}
		CDialog::OnMouseMove(nFlags, point);
}


void CWatchDialog::OnStnClickedWatch()   //鼠标点击事件
{
	if ((m_nObjWidth != -1) && (m_nObjHeight != -1)) {
		CPoint point;
		GetCursorPos(&point);  //屏幕坐标
		CPoint remote = UserPointtoRemoteScreenPoint(point, true);
		//封装
		MOUSEEV event;
		event.ptXY = remote;
		event.nButton = 0;  //左键
		event.nAction = 0;  //单击
		CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 5, true, (BYTE*)&event, sizeof(event));

	}
}


void CWatchDialog::OnOK()
{
	// TODO: 在此添加专用代码和/或调用基类

	//CDialog::OnOK();
}


void CWatchDialog::OnBnClickedBtnLock()
{
	// TODO: 在此添加控件通知处理程序代码

	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 7);
}


void CWatchDialog::OnBnClickedBtnUnlock()
{
	// TODO: 在此添加控件通知处理程序代码
	CClientController::getInstance()->SendCommandPacket(GetSafeHwnd(), 8);

}
