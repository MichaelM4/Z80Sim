
#include "pch.h"
#include "framework.h"

#include "RegistersWnd.h"
#include "Resource.h"
#include "MainFrm.h"
#include "z80sim.h"
#include "Z80Emu.h"

#ifdef _DEBUG
#undef THIS_FILE
static char THIS_FILE[]=__FILE__;
#define new DEBUG_NEW
#endif

extern CpuType cpu;

/////////////////////////////////////////////////////////////////////////////

CRegistersWnd::CRegistersWnd() noexcept
{
	m_nComboHeight = 0;
}

CRegistersWnd::~CRegistersWnd()
{
}

BEGIN_MESSAGE_MAP(CRegistersWnd, CDockablePane)
	ON_WM_CREATE()
	ON_WM_SIZE()
	ON_COMMAND(ID_EXPAND_ALL, OnExpandAllProperties)
	ON_UPDATE_COMMAND_UI(ID_EXPAND_ALL, OnUpdateExpandAllProperties)
	ON_COMMAND(ID_SORTPROPERTIES, OnSortProperties)
	ON_UPDATE_COMMAND_UI(ID_SORTPROPERTIES, OnUpdateSortProperties)
	ON_COMMAND(ID_PROPERTIES1, OnProperties1)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES1, OnUpdateProperties1)
	ON_COMMAND(ID_PROPERTIES2, OnProperties2)
	ON_UPDATE_COMMAND_UI(ID_PROPERTIES2, OnUpdateProperties2)
	ON_WM_SETFOCUS()
	ON_WM_SETTINGCHANGE()
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CResourceViewBar message handlers

void CRegistersWnd::AdjustLayout()
{
	if (GetSafeHwnd () == nullptr || (AfxGetMainWnd() != nullptr && AfxGetMainWnd()->IsIconic()))
	{
		return;
	}

	CRect rectClient;
	GetClientRect(rectClient);

	m_wndRegisterList.SetWindowPos(nullptr, rectClient.left, rectClient.top + m_nComboHeight, rectClient.Width(), rectClient.Height() - m_nComboHeight, SWP_NOACTIVATE | SWP_NOZORDER);
}

int CRegistersWnd::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CDockablePane::OnCreate(lpCreateStruct) == -1)
		return -1;

	CRect rectDummy;
	rectDummy.SetRectEmpty();

	m_nComboHeight = 0;

	if (!m_wndRegisterList.Create(WS_VISIBLE | WS_CHILD, rectDummy, this, 2))
	{
		TRACE0("Failed to create Properties Grid \n");
		return -1;      // fail to create
	}

	InitPropList();
	AdjustLayout();
	return 0;
}

void CRegistersWnd::OnSize(UINT nType, int cx, int cy)
{
	CDockablePane::OnSize(nType, cx, cy);
	AdjustLayout();
}

void CRegistersWnd::OnExpandAllProperties()
{
	m_wndRegisterList.ExpandAll();
}

void CRegistersWnd::OnUpdateExpandAllProperties(CCmdUI* /* pCmdUI */)
{
}

void CRegistersWnd::OnSortProperties()
{
	m_wndRegisterList.SetAlphabeticMode(!m_wndRegisterList.IsAlphabeticMode());
}

void CRegistersWnd::OnUpdateSortProperties(CCmdUI* pCmdUI)
{
	pCmdUI->SetCheck(m_wndRegisterList.IsAlphabeticMode());
}

void CRegistersWnd::OnProperties1()
{
	// TODO: Add your command handler code here
}

void CRegistersWnd::OnUpdateProperties1(CCmdUI* /*pCmdUI*/)
{
	// TODO: Add your command update UI handler code here
}

void CRegistersWnd::OnProperties2()
{
	// TODO: Add your command handler code here
}

void CRegistersWnd::OnUpdateProperties2(CCmdUI* /*pCmdUI*/)
{
	// TODO: Add your command update UI handler code here
}

void CRegistersWnd::InitPropList()
{
	m_wndRegisterList.EnableHeaderCtrl(FALSE);
	m_wndRegisterList.EnableDescriptionArea();
	m_wndRegisterList.SetVSDotNetLook();
	m_wndRegisterList.MarkModifiedProperties();

	CMFCPropertyGridProperty* pGroup1 = new CMFCPropertyGridProperty(_T("Registers"));

	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("A"),   _T("00")));
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("B"),   _T("00")));
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("C"),   _T("00")));
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("D"),   _T("00")));
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("E"),   _T("00")));
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("H"),   _T("00")));
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("L"),   _T("00")));
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("F"),   _T("00")));
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("SP"),  _T("00")));
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("PC"),  _T("00")));
	pGroup1->AddSubItem(new CMFCPropertyGridProperty(_T("MHz"), _T("00")));

	m_wndRegisterList.AddProperty(pGroup1);

}

void CRegistersWnd::OnSetFocus(CWnd* pOldWnd)
{
	CDockablePane::OnSetFocus(pOldWnd);
	m_wndRegisterList.SetFocus();
}

void CRegistersWnd::OnSettingChange(UINT uFlags, LPCTSTR lpszSection)
{
	CDockablePane::OnSettingChange(uFlags, lpszSection);
}

static void GetFlagsStr(byte f, char szBuf[])
{
	if (f & S_FLAG_MASK)
	{
		szBuf[0] = 'S';
	}
	else
	{
		szBuf[0] = 's';
	}

	if (f & Z_FLAG_MASK)
	{
		szBuf[1] = 'Z';
	}
	else
	{
		szBuf[1] = 'z';
	}

	if (f & X_FLAG_MASK)
	{
		szBuf[2] = 'X';
	}
	else
	{
		szBuf[2] = 'x';
	}

	if (f & H_FLAG_MASK)
	{
		szBuf[3] = 'H';
	}
	else
	{
		szBuf[3] = 'h';
	}

	if (f & X_FLAG_MASK)
	{
		szBuf[4] = 'X';
	}
	else
	{
		szBuf[4] = 'x';
	}

	if (f & P_FLAG_MASK)
	{
		szBuf[5] = 'P';
	}
	else
	{
		szBuf[5] = 'p';
	}

	if (f & N_FLAG_MASK)
	{
		szBuf[6] = 'n';
	}
	else
	{
		szBuf[6] = 'N';
	}

	if (f & C_FLAG_MASK)
	{
		szBuf[7] = 'C';
	}
	else
	{
		szBuf[7] = 'c';
	}

	szBuf[8] = 0;
}

static void GetPropValue(int i, CString& str)
{
	char szBuf[64];

	switch (i)
	{
		case 0:
			str.Format(_T("%02X"), cpu.regs.byteregs.a);
			break;

		case 1:
			str.Format(_T("%02X"), cpu.regs.byteregs.b);
			break;

		case 2:
			str.Format(_T("%02X"), cpu.regs.byteregs.c);
			break;

		case 3:
			str.Format(_T("%02X"), cpu.regs.byteregs.d);
			break;

		case 4:
			str.Format(_T("%02X"), cpu.regs.byteregs.e);
			break;

		case 5:
			str.Format(_T("%02X"), cpu.regs.byteregs.h);
			break;

		case 6:
			str.Format(_T("%02X"), cpu.regs.byteregs.l);
			break;
	
		case 7:
			GetFlagsStr(cpu.regs.byteregs.f, szBuf);
			str = szBuf;
			break;

		case 8:
			str.Format(_T("%04X"), cpu.sp);
			break;

		case 9:
			str.Format(_T("%04X"), cpu.pc);
			break;

		case 10:
			if (cpu.nExecTickCount > 0)
			{
				str.Format(_T("%d MHz"), (int)((cpu.nTotalCycles / cpu.nExecTickCount) / 1000));
			}

			break;
	}
}

void CRegistersWnd::UpdateRegisters()
{
	CString str;
	int i;

	CMFCPropertyGridProperty* pProp = m_wndRegisterList.GetProperty(0);
	CMFCPropertyGridProperty* pSubProp;

	if (pProp == NULL)
	{
		return;
	}

	for (i = 0; i < 11; ++i)
	{
		GetPropValue(i, str);
		pSubProp = pProp->GetSubItem(i);

		if (pSubProp != NULL)
		{
			pSubProp->SetValue(str);
		}
	}
}
