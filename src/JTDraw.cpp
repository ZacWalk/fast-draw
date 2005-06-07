// JTDraw.cpp : Defines the class behaviors for the application.
//

#include "stdafx.h"
#include "JTDraw.h"

#include "MainFrm.h"
#include "ChildFrm.h"
#include "IpFrame.h"
#include "JTDrawDoc.h"
#include "JTDrawView.h"
#include "hyperlink.h"


#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

#define IDS_MAILADDR "mailto:jianz2@yahoo.com"
#define IDS_HOMEPAGEADDR  "http://www.andrew.cmu.edu/~jianz" 
/////////////////////////////////////////////////////////////////////////////
// CJTDrawApp

BEGIN_MESSAGE_MAP(CJTDrawApp, CWinApp)
	//{{AFX_MSG_MAP(CJTDrawApp)
	ON_COMMAND(ID_APP_ABOUT, OnAppAbout)
		// NOTE - the ClassWizard will add and remove mapping macros here.
		//    DO NOT EDIT what you see in these blocks of generated code!
	//}}AFX_MSG_MAP
	// Standard file based document commands
	ON_COMMAND(ID_FILE_NEW, CWinApp::OnFileNew)
	ON_COMMAND(ID_FILE_OPEN, CWinApp::OnFileOpen)
	// Standard print setup command
	ON_COMMAND(ID_FILE_PRINT_SETUP, CWinApp::OnFilePrintSetup)
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CJTDrawApp construction

CJTDrawApp::CJTDrawApp()
{
	// TODO: add construction code here,
	// Place all significant initialization in InitInstance
}

/////////////////////////////////////////////////////////////////////////////
// The one and only CJTDrawApp object

CJTDrawApp theApp;

/////////////////////////////////////////////////////////////////////////////
// Customization load/save methods

void CJTDrawApp::PreLoadState ()
{
	GetMouseManager()->AddView (IDR_JTDRAW, _T("My view"), IDR_JTDRAW);
	// TODO: add another views and windows were mouse double click
	// customization is required

	GetContextMenuManager()->AddMenu (_T("My menu"), ID_POPUP_MENU);
	// TODO: add another context menus here
}

// This identifier was generated to be statistically unique for your app.
// You may change it if you prefer to choose a specific identifier.

// {64222BB2-A5DC-11D3-B4DC-00A0C995EA3D}
static const CLSID clsid =
{ 0x64222bb2, 0xa5dc, 0x11d3, { 0xb4, 0xdc, 0x0, 0xa0, 0xc9, 0x95, 0xea, 0x3d } };

/////////////////////////////////////////////////////////////////////////////
// CJTDrawApp initialization

BOOL CJTDrawApp::InitInstance()
{
	SetRegistryKey(_T("FaSb"));
	// Initialize OLE libraries
	if (!AfxOleInit())
	{
		AfxMessageBox(IDP_OLE_INIT_FAILED);
		return FALSE;
	}

	AfxEnableControlContainer();

	// Standard initialization
	// If you are not using these features and wish to reduce the size
	//  of your final executable, you should remove from the following
	//  the specific initialization routines you do not need.

#ifdef _AFXDLL
	Enable3dControls();			// Call this when using MFC in a shared DLL
#else
	Enable3dControlsStatic();	// Call this when linking to MFC statically
#endif

	// Change the registry key under which our settings are stored.
	// TODO: You should modify this string to be something appropriate
	// such as the name of your company or organization.
	//SetRegistryKey(_T("Local AppWizard-Generated Applications"));

	LoadStdProfileSettings();  // Load standard INI file options (including MRU)
	SetRegistryBase (_T("Settings"));

	// Register the application's document templates.  Document templates
	//  serve as the connection between documents, frame windows and views.
	// Initialize all Managers for usage. They are automatically constructed
	// if not yet present
	InitMouseManager();
	InitContextMenuManager();
	InitKeyboardManager();

	CMultiDocTemplate* pDocTemplate;
	pDocTemplate = new CMultiDocTemplate(
		IDR_JTDRAW,
		RUNTIME_CLASS(CJTDrawDoc),
		RUNTIME_CLASS(CChildFrame), // custom MDI child frame
		RUNTIME_CLASS(CJTDrawView));
	pDocTemplate->SetContainerInfo(IDR_JTDRAWTYPE_CNTR_IP);
	pDocTemplate->SetServerInfo(
		IDR_JTDRAWTYPE_SRVR_EMB, IDR_JTDRAWTYPE_SRVR_IP,
		RUNTIME_CLASS(CInPlaceFrame));
	AddDocTemplate(pDocTemplate);

	// Connect the COleTemplateServer to the document template.
	//  The COleTemplateServer creates new documents on behalf
	//  of requesting OLE containers by using information
	//  specified in the document template.
	m_server.ConnectTemplate(clsid, pDocTemplate, FALSE);

	// Register all OLE server factories as running.  This enables the
	//  OLE libraries to create objects from other applications.
	COleTemplateServer::RegisterAll();
		// Note: MDI applications register all server objects without regard
		//  to the /Embedding or /Automation on the command line.

	// create main MDI Frame window
	CMainFrame* pMainFrame = new CMainFrame;
	if (!pMainFrame->LoadFrame(IDR_MAINFRAME))
		return FALSE;
	m_pMainWnd = pMainFrame;

	// Parse command line for standard shell commands, DDE, file open
	CCommandLineInfo cmdInfo;
	ParseCommandLine(cmdInfo);

	// Check to see if launched as OLE server
	if (cmdInfo.m_bRunEmbedded || cmdInfo.m_bRunAutomated)
	{
		// Application was run with /Embedding or /Automation.  Don't show the
		//  main window in this case.
		return TRUE;
	}

	// When a server application is launched stand-alone, it is a good idea
	//  to update the system registry in case it has been damaged.
	m_server.UpdateRegistry(OAT_INPLACE_SERVER);

	// Dispatch commands specified on the command line
	//if (!ProcessShellCommand(cmdInfo))
	//	return FALSE;

	// Enable drag/drop open
	m_pMainWnd->DragAcceptFiles();

	// The main window has been initialized, so show and update it.
	pMainFrame->ShowWindow(m_nCmdShow);
	pMainFrame->UpdateWindow();

	return TRUE;
}


/////////////////////////////////////////////////////////////////////////////
// CAboutDlg dialog used for App About

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// Dialog Data
	//{{AFX_DATA(CAboutDlg)
	enum { IDD = IDD_ABOUTBOX };
	CHyperLink	m_email;
	CHyperLink	m_www;
	//}}AFX_DATA

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CAboutDlg)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	//{{AFX_MSG(CAboutDlg)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
	//{{AFX_DATA_INIT(CAboutDlg)
	//}}AFX_DATA_INIT
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CAboutDlg)
	DDX_Control(pDX, IDC_EMAIL, m_email);
	DDX_Control(pDX, IDC_WWW, m_www);
	//}}AFX_DATA_MAP
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
	//{{AFX_MSG_MAP(CAboutDlg)
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

// App command to run the dialog
void CJTDrawApp::OnAppAbout()
{
	CAboutDlg aboutDlg;
	aboutDlg.DoModal();
}

/////////////////////////////////////////////////////////////////////////////
// CJTDrawApp message handlers


BOOL CAboutDlg::OnInitDialog() 
{
	CDialog::OnInitDialog();
	
	// Set HyperLink for E-Mail
	m_email.SetURL(_T(IDS_MAILADDR));
	m_email.SetUnderline(TRUE);
	m_email.SetLinkCursor(AfxGetApp()->LoadCursor(IDC_HAND));

	// Set HyperLink for Home Page
	m_www.SetURL(_T(IDS_HOMEPAGEADDR));
	m_www.SetUnderline(TRUE);
	m_www.SetLinkCursor(AfxGetApp()->LoadCursor(IDC_HAND));
		
	//SetLogoFont("Verdana", -25);

	// Write class version
	CString sVersion;
	sVersion.Format("JTDrawa v%s","1.0");
	GetDlgItem(IDC_VERSION)->SetWindowText(sVersion);

	// Fill friends listbox
	CListBox* lbxFriends;
	lbxFriends = (CListBox*)GetDlgItem(IDC_FRIENDS);
	lbxFriends->AddString("Llew S. Goodstadt <lg@ndirect.co.uk> for Dash Line");
	lbxFriends->AddString("Stas Levin<stas@iet.co.il> for  BCGControlBar library");
	lbxFriends->AddString("Chris Maunder  <chrismaunder@codeguru.com>");
	lbxFriends->AddString("     for colorpicker and HyperLink");
	
	return TRUE;  // return TRUE unless you set the focus to a control
	              // EXCEPTION: OCX Property Pages should return FALSE
}
