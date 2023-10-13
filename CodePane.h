
#pragma once

class CFileViewToolBar : public CMFCToolBar
{
	virtual void OnUpdateCmdUI(CFrameWnd* /*pTarget*/, BOOL bDisableIfNoHndler)
	{
		CMFCToolBar::OnUpdateCmdUI((CFrameWnd*) GetOwner(), bDisableIfNoHndler);
	}

	virtual BOOL AllowShowOnList() const { return FALSE; }
};

class CCodePane : public CDockablePane
{
// Construction
public:
	CCodePane() noexcept;

	void AdjustLayout();
	void OnChangeVisualStyle();

// Attributes
protected:
	CMFCListCtrl     m_wndCodeView;
	CFileViewToolBar m_wndToolBar;

	int m_nSelectedItem;

// Implementation
public:
	virtual ~CCodePane();

	void UpdateCodeList();
	void RedrawCodeList();

protected:
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnSize(UINT nType, int cx, int cy);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	afx_msg void OnPaint();
	afx_msg void OnSetFocus(CWnd* pOldWnd);

	DECLARE_MESSAGE_MAP()
};

