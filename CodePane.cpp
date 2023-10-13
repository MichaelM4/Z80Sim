
#include "pch.h"
#include "framework.h"
#include "mainfrm.h"
#include "CodePane.h"
#include "Resource.h"
#include "z80sim.h"
#include "Z80Emu.h"
#include "Z80Dis.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

extern CpuType cpu;

/////////////////////////////////////////////////////////////////////////////
// CCodePane

CCodePane::CCodePane() noexcept
{
	m_nSelectedItem = -1;
}

CCodePane::~CCodePane()
{
}

BEGIN_MESSAGE_MAP(CCodePane, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_WM_CONTEXTMENU()
	ON_WM_PAINT()
	ON_WM_SETFOCUS()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CWorkspaceBar message handlers

int CCodePane::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	// Create view:
	const DWORD dwViewStyle = LBS_NOINTEGRALHEIGHT | WS_CHILD | WS_VISIBLE | WS_HSCROLL | WS_VSCROLL | LVS_REPORT;

	if (!m_wndCodeView.Create(dwViewStyle, rectDummy, this, 4))
	{
		TRACE0("Failed to create file view\n");
		return -1;      // fail to create
	}

	m_wndCodeView.ModifyStyle(0, LVS_SINGLESEL | LVS_SHOWSELALWAYS, 0);
	m_wndCodeView.SendMessage(LVM_SETEXTENDEDLISTVIEWSTYLE, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES, LVS_EX_FULLROWSELECT | LVS_EX_GRIDLINES);

	m_wndCodeView.InsertColumn(0, _T(" "), LVCFMT_LEFT, 50);
	m_wndCodeView.InsertColumn(1, _T(" "), LVCFMT_LEFT, 150);

	m_wndToolBar.Create(this, AFX_DEFAULT_TOOLBAR_STYLE, IDR_EXPLORER);
	m_wndToolBar.LoadToolBar(IDR_EXPLORER, 0, 0, TRUE /* Is locked */);

	OnChangeVisualStyle();

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() | CBRS_TOOLTIPS | CBRS_FLYBY);

	m_wndToolBar.SetPaneStyle(m_wndToolBar.GetPaneStyle() & ~(CBRS_GRIPPER | CBRS_SIZE_DYNAMIC | CBRS_BORDER_TOP | CBRS_BORDER_BOTTOM | CBRS_BORDER_LEFT | CBRS_BORDER_RIGHT));

	m_wndToolBar.SetOwner(this);

	// All commands will be routed via this control , not via the parent frame:
	m_wndToolBar.SetRouteCommandsViaFrame(FALSE);

	// Fill in some static tree view data (dummy code, nothing magic here)
	UpdateCodeList();
	AdjustLayout();

	return 0;
}

void CCodePane::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CCodePane::UpdateCodeList()
{
	CString str;
  char    szBuf[128];
	int     i, n, pc;

	m_wndCodeView.DeleteAllItems();

	pc = cpu.pc;

	for (i = 0; i < 30; ++i)
	{
		str.Format(_T("%04X"), pc);
		n = m_wndCodeView.InsertItem(i, str);
		m_wndCodeView.SetItemData(n, pc);

		pc += GetInstructionName(pc, szBuf, sizeof(szBuf)-2);
		str = szBuf;
		m_wndCodeView.SetItemText(i, 1, str);
	}

	m_wndCodeView.SetItemState(0, LVIS_SELECTED, LVIS_SELECTED);
	m_wndCodeView.EnsureVisible(0, FALSE);
	m_nSelectedItem = 0;
}

void CCodePane::RedrawCodeList()
{
	int nSize = m_wndCodeView.GetItemCount();
	int i;

	if (m_nSelectedItem >= 0)
	{
			m_wndCodeView.SetItemState(m_nSelectedItem, 0, LVIS_SELECTED);
			m_nSelectedItem = -1;
	}

	for (i = 0; i < nSize; ++i)
	{
		if (m_wndCodeView.GetItemData(i) == cpu.pc)
		{
			m_wndCodeView.SetItemState(i, LVIS_SELECTED, LVIS_SELECTED);
			m_nSelectedItem = i;
			return;
		}
	}

	UpdateCodeList();
}

void CCodePane::OnContextMenu(CWnd* pWnd, CPoint point)
{
}

void CCodePane::AdjustLayout()
{
	if (GetSafeHwnd() == nullptr)
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	int cyTlb = m_wndToolBar.CalcFixedLayout(FALSE, TRUE).cy;

	m_wndToolBar.SetWindowPos(nullptr, rectClient.left, rectClient.top, rectClient.Width(), cyTlb, SWP_NOACTIVATE | SWP_NOZORDER);
	m_wndCodeView.SetWindowPos(nullptr, rectClient.left + 1, rectClient.top + cyTlb + 1, rectClient.Width() - 2, rectClient.Height() - cyTlb - 2, SWP_NOACTIVATE | SWP_NOZORDER);
}

void CCodePane::OnPaint()
{
	CPaintDC dc(this); // device context for painting

	CRect rectTree;
	m_wndCodeView.GetWindowRect(rectTree);
	ScreenToClient(rectTree);

	rectTree.InflateRect(1, 1);
	dc.Draw3dRect(rectTree, ::GetSysColor(COLOR_3DSHADOW), ::GetSysColor(COLOR_3DSHADOW));
}

void CCodePane::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);

	m_wndCodeView.SetFocus();
}

void CCodePane::OnChangeVisualStyle()
{
	m_wndToolBar.CleanUpLockedImages();
	m_wndToolBar.LoadBitmap(theApp.m_bHiColorIcons ? IDB_EXPLORER_24 : IDR_EXPLORER, 0, 0, TRUE /* Locked */);

	UINT uiBmpId = theApp.m_bHiColorIcons ? IDB_FILE_VIEW_24 : IDB_FILE_VIEW;

	CBitmap bmp;
	if (!bmp.LoadBitmap(uiBmpId))
	{
		TRACE(_T("Can't load bitmap: %x\n"), uiBmpId);
		ASSERT(FALSE);
		return;
	}

	BITMAP bmpObj;
	bmp.GetBitmap(&bmpObj);
}


