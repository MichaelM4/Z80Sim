#pragma once
#include "afxdialogex.h"

// CBreakPoinmtsDlg dialog

class CBreakPointsDlg : public CDialogEx
{
	DECLARE_DYNAMIC(CBreakPointsDlg)

public:
	CBreakPointsDlg(CWnd* pParent = nullptr);   // standard constructor
	virtual ~CBreakPointsDlg();

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_BREAK_POINTS };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

	DECLARE_MESSAGE_MAP()
public:
	CListCtrl m_cBreakPointList;
	virtual BOOL OnInitDialog();
};
