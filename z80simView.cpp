
// z80simView.cpp : implementation of the Cz80simView class
//

#include "pch.h"
#include "framework.h"
// SHARED_HANDLERS can be defined in an ATL project implementing preview, thumbnail
// and search filter handlers and allows sharing of document code with that project.
#ifndef SHARED_HANDLERS
#include "z80sim.h"
#endif

#include "z80simDoc.h"
#include "z80simView.h"
#include "Z80Emu.h"
#include "Z80Dis.h"
#include "system.h"
#include "keyboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern byte g_byMemory[];
extern byte g_byVideoMemory[VIDEO_BUFFER_SIZE];
extern byte g_byMemoryMapSel;
extern byte g_byVideoNumCols;
extern byte g_byVideoNumRows;
extern int  g_nVideoRow;
extern int  g_nVideoCol;
extern int  g_nModel;
extern byte g_byModel4_ModReg;
extern byte g_byModel4_Page;

extern CpuType cpu;

// Cz80simView

IMPLEMENT_DYNCREATE(Cz80simView, CView)

BEGIN_MESSAGE_MAP(Cz80simView, CView)
	// Standard printing commands
	ON_COMMAND(ID_FILE_PRINT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_DIRECT, &CView::OnFilePrint)
	ON_COMMAND(ID_FILE_PRINT_PREVIEW, &Cz80simView::OnFilePrintPreview)
	ON_WM_CONTEXTMENU()
	ON_WM_RBUTTONUP()
	ON_WM_KEYDOWN()
	ON_WM_KEYDOWN()
	ON_WM_KEYUP()
	ON_WM_CHAR()
	ON_WM_ERASEBKGND()
END_MESSAGE_MAP()

// provides an override of the base class RedrawWindow() function which does not
// erase the window by default.  See the class definition for parameter defaults.
BOOL Cz80simView::RedrawWindow(LPCRECT lpRectUpdate, CRgn* prgnUpdate, UINT flags)
{
	return CView::RedrawWindow(lpRectUpdate, prgnUpdate, flags);
}

// Cz80simView construction/destruction

Cz80simView::Cz80simView() noexcept
{
	// TODO: add construction code here
	m_nFontSize     = 14;
	m_nSelectedFont = eNormal;
}

Cz80simView::~Cz80simView()
{
}

BOOL Cz80simView::PreCreateWindow(CREATESTRUCT& cs)
{
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return CView::PreCreateWindow(cs);
}

TCHAR GetNormalVideoChar(int addr)
{
	TCHAR by = g_byVideoMemory[addr];

  if (((by & 0xA0) == 0) && ((g_nModel == eModel1) || (g_nModel == eModel3)))
  {
    by |= 0x40;
  }

	switch (by)
	{
		case 128:
		case 130:
		case 160:
			by = ' ';
			break;

		case 138:
			by = 0x2590;
			break;

		case 133:
		case 149:
			by = 0x258C;
			break;

		case 131:
		case 135:
		case 143:
		case 159:
		case 175:
			by = 0x2580;
			break;

		case 136:
		case 140:
		case 180:
			by = 0x2584;
			break;

		case 189:
		case 190:
		case 191:
			by = 0x2588;
			break;

		case 188:
			by = 0x2584;
			break;

		default:
			if (by > 128)
			{
				by = 0x25A0;
			}

			break;
	}

	return by;
}

TCHAR GetAltVideoChar(int addr)
{
	TCHAR by = g_byVideoMemory[addr];

  if (((by & 0xA0) == 0) && (g_nModel != eModel4))
  {
    by |= 0x40;
  }

	return by;
}

void Cz80simView::OutputVideoLine(CDC* pDC, int nLine, int nYPos)
{
	TCHAR szBuf[128], ch;
	int i, j, nXPos;
	int nOffset = (nLine * g_byVideoNumCols);

	i     = 0;
	nXPos = 0;

	if ((g_nModel == eModel4) && (g_byModel4_Page & 0x08)) // model 4 inverse video
	{
		for (j = 0; j < g_byVideoNumCols; ++j)
		{
			ch = GetAltVideoChar(nOffset+j);

			if (m_nSelectedFont == eNormal)
			{
				if (ch >= 0xA0)
				{
					szBuf[i] = 0;
					pDC->TextOut(nXPos, nYPos, szBuf);
					nXPos += pDC->GetTextExtent(szBuf).cx;

					i = 0;
					szBuf[i++] = ch - 0xA0 + ' ';

					m_nSelectedFont = eBold;
					pDC->SelectObject(&m_FontBold);
				}
				else
				{
					szBuf[i] = ch;
					++i;
				}
			}
			else if (m_nSelectedFont == eBold)
			{
				if (ch < 0xA0)
				{
					szBuf[i] = 0;
					pDC->TextOut(nXPos, nYPos, szBuf);
					nXPos += pDC->GetTextExtent(szBuf).cx;

					i = 0;
					szBuf[i++] = ch;

					m_nSelectedFont = eNormal;
					pDC->SelectObject(&m_FontNormal);
				}
				else
				{
					szBuf[i] = ch - 0xA0 + ' ';
					++i;
				}
			}
		}

		if (i > 0)
		{
			szBuf[i] = 0;
			CString str = szBuf;

			pDC->TextOut(nXPos, nYPos, str);
		}

		return;
	}

	for (j = 0; j < g_byVideoNumCols; ++j)
	{
		ch = GetNormalVideoChar(nOffset+j);
		szBuf[i] = ch;
		++i;
	}

	if ((nLine == g_nVideoRow) && (g_byKeyboardMode == eKeyPortMapped))
	{
		szBuf[g_nVideoCol] = '_';
	}

	szBuf[i] = 0;
	CString str = szBuf;
	pDC->TextOut(nXPos, nYPos, str);
}

void Cz80simView::OnDraw(CDC* pDC)
{
	Cz80simDoc* pDoc = GetDocument();
	CString str;
	CSize cs;
	int   i, nXPos, nYPos;

	if (g_byMemoryMapSel == 3)
	{
		return;
	}

	ASSERT_VALID(pDoc);

	if (!pDoc)
	{
		return;
	}

	CRect   rc;
	CDC     memDC;
	CBitmap memBM;
	CBrush  hbrBkGnd;
	 
	// Get the size of the edit box rectangle
	//
	GetClientRect(&rc);

	// Create a compatible DC in memory
	memDC.CreateCompatibleDC(pDC);

	// Create a compatible bitmap big enough for the edit box rectangle
	memBM.CreateCompatibleBitmap(pDC, rc.right - rc.left, rc.bottom - rc.top);

	// Select the bitmap into the memory DC
	//
	CBitmap *pBM = memDC.SelectObject(&memBM);

	// Erase the background by filling in the memory
	// DC with the appropriate background color
	hbrBkGnd.CreateSolidBrush(GetSysColor(COLOR_WINDOW));
	memDC.FillRect(&rc, &hbrBkGnd);

	// Select the font that was created in the constructor
	memDC.SelectObject(&m_FontNormal);
	m_nSelectedFont = eNormal;

	//////////////////////////////////////////////////////////////////////////////

	// determine chacter size
	cs.cx = memDC.GetTextExtent(_T("H"), 1).cx;   // character width
	cs.cy = memDC.GetTextExtent(_T("Hg"), 2).cy;  // line height

	nXPos = cs.cx;
	nYPos = 0;

	memDC.SetTextColor(BLACK);
	memDC.SetBkColor(WHITE);

	for (i = 0; i < g_byVideoNumRows; ++i)
	{
		OutputVideoLine(&memDC, i, nYPos);
		nYPos += cs.cy;
	}

	//////////////////////////////////////////////////////////////////////////////

	// Blit the changes from the memory DC to the screen DC
	//
	pDC->BitBlt(rc.left, rc.top, rc.right - rc.left, rc.bottom - rc.top, &memDC, 0, 0, SRCCOPY);

	// Cleanup
	//
	memDC.SelectObject(pBM);
	memBM.DeleteObject();
}

// Cz80simView printing

void Cz80simView::OnFilePrintPreview()
{
#ifndef SHARED_HANDLERS
	AFXPrintPreview(this);
#endif
}

BOOL Cz80simView::OnPreparePrinting(CPrintInfo* pInfo)
{
	// default preparation
	return DoPreparePrinting(pInfo);
}

void Cz80simView::OnBeginPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add extra initialization before printing
}

void Cz80simView::OnEndPrinting(CDC* /*pDC*/, CPrintInfo* /*pInfo*/)
{
	// TODO: add cleanup after printing
}

void Cz80simView::OnRButtonUp(UINT /* nFlags */, CPoint point)
{
	ClientToScreen(&point);
	OnContextMenu(this, point);
}

void Cz80simView::OnContextMenu(CWnd* /* pWnd */, CPoint point)
{
#ifndef SHARED_HANDLERS
	theApp.GetContextMenuManager()->ShowPopupMenu(IDR_POPUP_EDIT, point.x, point.y, this, TRUE);
#endif
}

// Cz80simView diagnostics

#ifdef _DEBUG
void Cz80simView::AssertValid() const
{
	CView::AssertValid();
}

void Cz80simView::Dump(CDumpContext& dc) const
{
	CView::Dump(dc);
}

Cz80simDoc* Cz80simView::GetDocument() const // non-debug version is inline
{
	ASSERT(m_pDocument->IsKindOf(RUNTIME_CLASS(Cz80simDoc)));
	return (Cz80simDoc*)m_pDocument;
}
#endif //_DEBUG

// Cz80simView message handlers

void Cz80simView::OnInitialUpdate()
{
	CView::OnInitialUpdate();

	CClientDC dc(this);
	int nHeight = -((dc.GetDeviceCaps (LOGPIXELSY) * m_nFontSize) / 72);

	m_FontNormal.CreateFont(nHeight, 0, 0, 0, FW_NORMAL, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH | FF_DONTCARE, _T("Courier New"));
	m_FontBold.CreateFont(nHeight, 0, 0, 0, FW_BOLD, 0, 0, 0, DEFAULT_CHARSET, 0, 0, 0, FIXED_PITCH | FF_DONTCARE, _T("Courier New"));
}

////////////////////////////////////////////////////////////////////////////////////////////////////

#define CHAR_BUFFSER_SIZE 32

int nCharBufferHead = 0;
int nCharBufferTail = 0;
int nCharBufferSize = CHAR_BUFFSER_SIZE;
UINT nCharBuffer[CHAR_BUFFSER_SIZE];

void BufferChar(UINT nChar)
{
	nCharBuffer[nCharBufferHead] = nChar;
	++nCharBufferHead;

	if (nCharBufferHead >= nCharBufferSize)
	{
		nCharBufferHead = 0;
	}
}

void Cz80simView::OnChar(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	BufferChar(nChar);

	CView::OnChar(nChar, nRepCnt, nFlags);
}

BOOL Cz80simView::OnEraseBkgnd(CDC* pDC)
{
	// TODO: Add your message handler code here and/or call default

	return 1; //CView::OnEraseBkgnd(pDC);
}

void Cz80simView::OnKeyDown(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (g_byKeyboardMode == eKeyMemoryMapped)
	{
		switch (nChar)
		{
			case VK_UP:
			case VK_DOWN:
			case VK_LEFT:
			case VK_RIGHT:
			case VK_ESCAPE:
			case VK_HOME:
				SimulateKeyDown(nChar | 0x1000);
				break;
		}
	}

	CView::OnKeyDown(nChar, nRepCnt, nFlags);
}

void Cz80simView::OnKeyUp(UINT nChar, UINT nRepCnt, UINT nFlags)
{
	if (g_byKeyboardMode == eKeyMemoryMapped)
	{
		switch (nChar)
		{
			case VK_UP:
			case VK_DOWN:
			case VK_LEFT:
			case VK_RIGHT:
			case VK_ESCAPE:
			case VK_HOME:
				SimulateKeyUp(nChar | 0x1000);
				break;
		}
	}
	else if (GetKeyState(VK_CONTROL) & 0x8000)
	{
		BufferChar(toupper(nChar)-'A'+1);
	}

	CView::OnKeyUp(nChar, nRepCnt, nFlags);
}
