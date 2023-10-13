
// z80simView.h : interface of the Cz80simView class
//

#pragma once

/////////////////////////////////////////////////////////////////////////////
// COLORREF colors

#define LIGHTGRAY RGB(192, 192, 192)
#define GRAY      RGB(128, 128, 128)
#define BLACK     RGB(0, 0, 0)
#define WHITE     RGB(255, 255, 255)
#define NAVY      RGB(0, 0, 128)
#define RED       RGB(255, 0, 0)
#define GREEN     RGB(0, 255, 0)
#define BLUE      RGB(0, 0, 255)
#define YELLOW    RGB(255, 255, 0)
#define AQUA      RGB(0, 255, 255)
#define PURPLE    RGB(255, 0, 255)
#define DARKBLUE  RGB(0, 0, 128)

class Cz80simView : public CView
{
protected: // create from serialization only
	Cz80simView() noexcept;
	DECLARE_DYNCREATE(Cz80simView)

enum {
	eNormal,
	eBold,
};

private:
	CFont m_FontNormal;
	CFont m_FontBold;
	int   m_nFontSize;
	int   m_nSelectedFont;

// Attributes
public:
	Cz80simDoc* GetDocument() const;
	BOOL  RedrawWindow(LPCRECT lpRectUpdate = NULL, CRgn* prgnUpdate = NULL, UINT flags = RDW_NOERASE); //RDW_INVALIDATE | RDW_UPDATENOW);

// Operations
public:

private:
	void OutputVideoLine(CDC* pDC, int nLine, int nYPos);

// Overrides
public:
	virtual void OnDraw(CDC* pDC);  // overridden to draw this view
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);

protected:
	virtual BOOL OnPreparePrinting(CPrintInfo* pInfo);
	virtual void OnBeginPrinting(CDC* pDC, CPrintInfo* pInfo);
	virtual void OnEndPrinting(CDC* pDC, CPrintInfo* pInfo);

// Implementation
public:
	virtual ~Cz80simView();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:

// Generated message map functions
protected:
	afx_msg void OnFilePrintPreview();
	afx_msg void OnRButtonUp(UINT nFlags, CPoint point);
	afx_msg void OnContextMenu(CWnd* pWnd, CPoint point);
	DECLARE_MESSAGE_MAP()

public:
	virtual void OnInitialUpdate();
	afx_msg void OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg void OnChar(UINT nChar, UINT nRepCnt, UINT nFlags);
	afx_msg BOOL OnEraseBkgnd(CDC* pDC);
};

#ifndef _DEBUG  // debug version in z80simView.cpp
inline Cz80simDoc* Cz80simView::GetDocument() const
   { return reinterpret_cast<Cz80simDoc*>(m_pDocument); }
#endif

