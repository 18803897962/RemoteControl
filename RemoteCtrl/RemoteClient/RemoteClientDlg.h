﻿
// RemoteClientDlg.h: 头文件
//

#pragma once

#ifndef WM_SEND_PACK_ACK  //发送数据包应答
#define WM_SEND_PACK_ACK (WM_USER+2) //发送数据包应答
#endif
#include "ClientSocket.h"
#include "StatusDlg.h"

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
	CString GetPath(HTREEITEM hTree);
	void DeleteTreeChildrenItem(HTREEITEM hTree);
	void LoadFileCurrent();
	void String2Tree(const std::string& drivers,CTreeCtrl& tree);
	void UpDateFileInfo(const FILEINFO& Info,HTREEITEM hlParam);
	void UpdateDownloadFile(const std::string& strData, FILE* pFile);
	void InitUIdata();
	void DealCommand(WORD nCmd,const std::string& strData, LPARAM lParam);
private:
// 实现
public:
	void LoadFileInfo();
	
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
	afx_msg void OnBnClickedBtnStartWatch();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnIpnFieldchangedIpaddressServ(NMHDR* pNMHDR, LRESULT* pResult);
	afx_msg void OnEnChangeEditPort();
	afx_msg LRESULT OnSendPackAck(WPARAM wParam, LPARAM lParam);  //添加自定义消息
};
