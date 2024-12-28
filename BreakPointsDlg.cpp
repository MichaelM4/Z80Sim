// CBreakPoinmtsDlg.cpp : implementation file
//

#include "pch.h"
#include "z80sim.h"
#include "afxdialogex.h"
#include "BreakPointsDlg.h"

// CBreakPoinmtsDlg dialog

IMPLEMENT_DYNAMIC(CBreakPointsDlg, CDialogEx)

CBreakPointsDlg::CBreakPointsDlg(CWnd* pParent /*=nullptr*/)
	: CDialogEx(IDD_BREAK_POINTS, pParent)
{

}

CBreakPointsDlg::~CBreakPointsDlg()
{
}

void CBreakPointsDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_BREAK_POINTS, m_cBreakPointList);
}

BEGIN_MESSAGE_MAP(CBreakPointsDlg, CDialogEx)
END_MESSAGE_MAP()

// CBreakPoinmtsDlg message handlers

BOOL CBreakPointsDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// TODO:  Add extra initialization here
	m_cBreakPointList.InsertColumn(0, _T("Address"), LVCFMT_LEFT, 80);

	return TRUE;  // return TRUE unless you set the focus to a control
								// EXCEPTION: OCX Property Pages should return FALSE
}
