
// z80sim.cpp : Defines the class behaviors for the application.
//

#include "pch.h"
#include "framework.h"
#include "afxwinappex.h"
#include "afxdialogex.h"
#include "z80sim.h"
#include "MainFrm.h"
#include "BreakPointsDlg.h"

#include "z80simDoc.h"
#include "z80simView.h"
#include "z80Emu.h"
#include "Defines.h"
#include "System.h"
#include "fdc.h"
#include "keyboard.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

extern CpuType cpu;
extern int  nCharBufferHead;
extern int  nCharBufferTail;
extern int  nCharBufferSize;
extern UINT nCharBuffer[];
extern int  g_nVideoModified;
extern int  g_nModel;
extern int  g_nKeyboardReadCount;

void LoadIniFile(char* pszFileName);

// CZ80SimApp

BEGIN_MESSAGE_MAP(CZ80SimApp, CWinAppEx)
	ON_COMMAND(ID_APP_ABOUT, &CZ80SimApp::OnAppAbout)
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, &CWinAppEx::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, &CWinAppEx::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, &CWinAppEx::OnFilePrintSetup)
	ON_COMMAND(ID_SIMULATE_RESTART, &CZ80SimApp::OnSimulateRestart)
	ON_COMMAND(ID_SIMULATE_GO, &CZ80SimApp::OnSimulateGo)
	ON_COMMAND(ID_SIMULATE_STOP, &CZ80SimApp::OnSimulateStop)
	ON_COMMAND(ID_SIMULATE_BREAKSPOINTS, &CZ80SimApp::OnSimulateBreakspoints)
	ON_COMMAND(ID_FILE_ENABLELOG, &CZ80SimApp::OnFileEnablelog)
	ON_UPDATE_COMMAND_UI(ID_FILE_ENABLELOG, &CZ80SimApp::OnUpdateFileEnablelog)
	ON_COMMAND(ID_FILE_ENABLEFDCLOG, &CZ80SimApp::OnFileEnableFdcLog)
	ON_UPDATE_COMMAND_UI(ID_FILE_ENABLEFDCLOG, &CZ80SimApp::OnUpdateFileEnablefdclog)
END_MESSAGE_MAP()

// CZ80SimApp construction

CZ80SimApp::CZ80SimApp() noexcept
{
	m_bHiColorIcons = TRUE;

	m_nAppLook = 0;
	// support Restart Manager
	m_dwRestartManagerSupportFlags = AFX_RESTART_MANAGER_SUPPORT_ALL_ASPECTS;
#ifdef _MANAGED
	// If the application is built using Common Language Runtime support (/clr):
	//     1) This additional setting is needed for Restart Manager support to work properly.
	//     2) In your project, you must add a reference to System.Windows.Forms in order to build.
	System::Windows::Forms::Application::SetUnhandledExceptionMode(System::Windows::Forms::UnhandledExceptionMode::ThrowException);
#endif

	// TODO: replace application ID string below with unique ID string; recommended
	// format for string is CompanyName.ProductName.SubProduct.VersionInformation
	SetAppID(_T("z80sim.AppID.NoVersion"));

	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
	m_bRun = false;

	m_nLastUpdateTickCount = 0;
	m_pEmuThread = NULL;
}

CZ80SimApp::~CZ80SimApp()
{
	CloseCpuLogFile();
}

// The one and only CZ80SimApp object

CZ80SimApp theApp;

// CZ80SimApp initialization

BOOL CZ80SimApp::InitInstance()
{
	// InitCommonControlsEx() is required on Windows XP if an application
	// manifest specifies use of ComCtl32.dll version 6 or later to enable
	// visual styles.  Otherwise, any window creation will fail.
	INITCOMMONCONTROLSEX InitCtrls;
	InitCtrls.dwSize = sizeof(InitCtrls);
	// Set this to include all the common control classes you want to use
	// in your application.
	InitCtrls.dwICC = ICC_WIN95_CLASSES;
	InitCommonControlsEx(&InitCtrls);

	CWinAppEx::InitInstance();

	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	EnableTaskbarInteraction(FALSE);

	// AfxInitRichEdit2() is required to use RichEdit control
	// AfxInitRichEdit2();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	// of your final executable, you should remove from the following
	// the specific initialization routines you do not need
	// Change the registry key under which our settings are stored
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization
	SetRegistryKey(_T("Local AppWizard-Generated Applications"));
	LoadStdProfileSettings(4);  // Load standard INI file options (including MRU)

	InitContextMenuManager();

	InitKeyboardManager();

	InitTooltipManager();
	CMFCToolTipInfo ttParams;
	ttParams.m_bVislManagerTheme = TRUE;
	theApp.GetTooltipManager()->SetTooltipParams(AFX_TOOLTIP_TYPE_ALL,
		RUNTIME_CLASS(CMFCToolTipCtrl), &ttParams);

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views
	CSingleDocTemplate* pDocTemplate;
	pDocTemplate = new CSingleDocTemplate(
		IDR_MAINFRAME,
		RUNTIME_CLASS(Cz80simDoc),
		RUNTIME_CLASS(CMainFrame),       // main SDI frame window
		RUNTIME_CLASS(Cz80simView));

	if (!pDocTemplate)
	{
		return FALSE;
	}

	AddDocTemplate(pDocTemplate);

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Dispatch commands specified on the command line.  Will return FALSE if
	// app was launched with /RegServer, /Register, /Unregserver or /Unregister.
	if (!ProcessShellCommand(cmdInfo))
	{
		return FALSE;
	}

	// The one and only window has been initialized, so show and update it
	m_pMainWnd->ShowWindow(SW_SHOW);
	m_pMainWnd->UpdateWindow();

	SetModel(eCpm);
	FileSystemInit();
	InitVars();
	InitSystem();

	CString str = AfxGetApp()->GetProfileString(_T("Files"), _T("DefaultIni"), _T(""));
	OpenDocumentFile(str.GetBuffer());

	return TRUE;
}

int CZ80SimApp::ExitInstance()
{
	//TODO: handle additional resources you may have added
	StopFdcThread();
	StopEmuThread();
	AfxOleTerm(FALSE);

	return CWinAppEx::ExitInstance();
}

// CZ80SimApp message handlers

// CAboutDlg dialog used for App About

class CAboutDlg : public CDialogEx
{
public:
	CAboutDlg() noexcept;

// Dialog Data
#ifdef AFX_DESIGN_TIME
	enum { IDD = IDD_ABOUTBOX };
#endif

protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support

// Implementation
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() noexcept : CDialogEx(IDD_ABOUTBOX)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialogEx)
END_MESSAGE_MAP()

// App command to run the dialog
void CZ80SimApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

// CZ80SimApp customization load/save methods

void CZ80SimApp::PreLoadState()
{
	BOOL bNameValid;
	CString strName;
	bNameValid = strName.LoadString(IDS_EDIT_MENU);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EDIT);
	bNameValid = strName.LoadString(IDS_EXPLORER);
	ASSERT(bNameValid);
	GetContextMenuManager()->AddMenu(strName, IDR_POPUP_EXPLORER);
}

void CZ80SimApp::LoadCustomState()
{
}

void CZ80SimApp::SaveCustomState()
{
}

// CZ80SimApp message handlers

void CZ80SimApp::RedrawAllWindows(void)
{
	CDocTemplate* pTemplate;
	CDocument*    pDoc;
	POSITION      pTemplateIndex = GetFirstDocTemplatePosition();
	POSITION      pDocIndex;

	while (pTemplateIndex != NULL)
	{
		pTemplate = GetNextDocTemplate(pTemplateIndex);
		pDocIndex = pTemplate->GetFirstDocPosition();

		while (pDocIndex != NULL)
		{
			pDoc = pTemplate->GetNextDoc(pDocIndex);

			if (pDoc != NULL)
			{
				pDoc->UpdateAllViews(NULL);
			}
		}
	}

	if (m_pMainWnd != NULL)
	{
		m_pMainWnd->RedrawWindow();
		CMainFrame* pMainFrame = (CMainFrame*)m_pMainWnd;
		pMainFrame->RedrawProperties();
		pMainFrame->RedrawCode();
	}
}

BOOL CZ80SimApp::OnIdle(LONG lCount)
{
	static UINT64 nPrevTickCount = GetTickCount64();
	UINT64 nTicks;

	// give default handler a chance to perform idle time processing
	if (CWinAppEx::OnIdle(lCount))
	{
		return TRUE;
	}

	if ((g_nKeyboardReadCount == 0) && (g_byKeyboardMode == eKeyMemoryMapped) && (nCharBufferTail != nCharBufferHead))
	{
		SimulateKeyDown(nCharBuffer[nCharBufferTail]);
	}

	UINT64 nCurrentTickCount = GetTickCount64();
	nTicks = (nCurrentTickCount - nPrevTickCount);
	nPrevTickCount = nCurrentTickCount;

	if (m_bRun)
	{
		cpu.nExecTickCount += nTicks;
	}

	if ((g_byKeyboardMode == eKeyMemoryMapped) && (nCharBufferTail != nCharBufferHead))
	{
		SimulateKeyUp(nCharBuffer[nCharBufferTail]);

		++nCharBufferTail;

		if (nCharBufferTail >= nCharBufferSize)
		{
			nCharBufferTail = 0;
		}
	}

	if (m_nLastUpdateTickCount < nCurrentTickCount)
	{
		m_nLastUpdateTickCount = GetTickCount64() + 100;
		RedrawAllWindows();
	}

	return m_bRun;
}

void CZ80SimApp::OnSimulateRestart()
{
	InitSystem();
	RedrawAllWindows();

	if (m_pMainWnd != NULL)
	{
		CMainFrame* pMainFrame = (CMainFrame*)m_pMainWnd;
		pMainFrame->RedrawCode();
	}
}

void CZ80SimApp::StartThreads(void)
{
	int data = 0;
	m_pEmuThread = AfxBeginThread(EmuExecute, (LPVOID)&data); 
	m_pFdcThread = AfxBeginThread(FdcExecute, (LPVOID)&data); 
	m_bRun = true;
}

void CZ80SimApp::StopThreads(void)
{
	StopFdcThread();
	StopEmuThread();
	m_bRun = false;
}

void CZ80SimApp::OnSimulateGo()
{
	StartThreads();
}

void CZ80SimApp::OnSimulateStop()
{
	StopThreads();

	if (m_pMainWnd != NULL)
	{
		CMainFrame* pMainFrame = (CMainFrame*)m_pMainWnd;
		pMainFrame->RedrawCode();
	}
}

void CZ80SimApp::OnFileEnablelog()
{
	if (g_bCpuLogOpen)
	{
		CloseCpuLogFile();
	}
	else
	{
		OpenCpuLogFile();
	}
}

void CZ80SimApp::OnUpdateFileEnablelog(CCmdUI* pCmdUI)
{
	if (g_bCpuLogOpen)
	{
		pCmdUI->SetCheck(1);
	}
	else
	{
		pCmdUI->SetCheck(0);
	}
}

void CZ80SimApp::OnFileEnableFdcLog()
{
	if (g_bFdcLogOpen)
	{
		CloseFdcLogFile();
	}
	else
	{
		OpenFdcLogFile();
	}
}

void CZ80SimApp::OnUpdateFileEnablefdclog(CCmdUI* pCmdUI)
{
	if (g_bFdcLogOpen)
	{
		pCmdUI->SetCheck(1);
	}
	else
	{
		pCmdUI->SetCheck(0);
	}
}

CDocument* CZ80SimApp::OpenDocumentFile(LPCTSTR lpszFileName)
{
	char szFilePath[256];
	int  i = 0;

	StopThreads();

	AfxGetApp()->WriteProfileString(_T("Files"), _T("DefaultIni"), lpszFileName);

	while ((*lpszFileName != 0) && (i < sizeof(szFilePath)-2))
	{
		szFilePath[i] = (char)*lpszFileName;
		++lpszFileName;
		++i;
	}

	szFilePath[i] = 0;

	LoadIniFile(szFilePath);

	CString str;

	switch (sysdef.nModel)
	{
		case eModel1:
			str = _T("Emulating - TRS-80 Model 1");
			break;

		case eModel3:
			str = _T("Emulating - TRS-80 Model 3");
			break;

		case eModel4:
			str = _T("Emulating - TRS-80 Model 4");
			break;

		case eCpm:
			str = _T("Emulating - CPM 2.2");
			break;
	}

	str += _T(", ");
	str += szFilePath;
	str += _T(", ");
	str += sysdef.szDrivePath[0];
	str += _T(", ");
	str += sysdef.szDrivePath[1];

	m_pMainWnd->SetWindowText(str);

	if (m_pMainWnd != NULL)
	{
		CMainFrame* pMainFrame = (CMainFrame*)m_pMainWnd;
		pMainFrame->UpdateCodeList();
	}

	StartThreads();

	return NULL;
}

void CZ80SimApp::OnSimulateBreakspoints()
{
	CBreakPointsDlg dlg;

	dlg.DoModal();
}
