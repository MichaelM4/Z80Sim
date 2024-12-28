
// z80sim.h : main header file for the z80sim application
//
#pragma once

#ifndef __AFXWIN_H__
	#error "include 'pch.h' before including this file for PCH"
#endif

#include "resource.h"       // main symbols

class CMainFrame;

// CZ80SimApp:
// See z80sim.cpp for the implementation of this class
//

class CZ80SimApp : public CWinAppEx
{
public:
	CZ80SimApp() noexcept;

private:
	bool   m_bRun;
	UINT64 m_nLastUpdateTickCount;

	CWinThread* m_pEmuThread;
	CWinThread* m_pFdcThread;

private:
	void StartThreads(void);
	void StopThreads(void);

// Overrides
public:
	void RedrawAllWindows(void);

	virtual ~CZ80SimApp();
	virtual BOOL InitInstance();
	virtual int ExitInstance();

// Implementation
	UINT  m_nAppLook;
	BOOL  m_bHiColorIcons;

	virtual void PreLoadState();
	virtual void LoadCustomState();
	virtual void SaveCustomState();

	afx_msg void OnAppAbout();
	DECLARE_MESSAGE_MAP()

	afx_msg void OnSimulateRestart();
	afx_msg void OnSimulateGo();
	virtual BOOL OnIdle(LONG lCount);
	afx_msg void OnSimulateStop();
	afx_msg void OnFileEnablelog();
	virtual CDocument* OpenDocumentFile(LPCTSTR lpszFileName);
	afx_msg void OnSimulateBreakspoints();
};

extern CZ80SimApp theApp;
