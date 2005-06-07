// MainFrm.cpp : implementation of the CMainFrame class
//

#include "stdafx.h"
#include "JTDraw.h"
#include "jtdrawview.h"
#include "MainFrm.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CMainFrame

IMPLEMENT_DYNAMIC(CMainFrame, CMDIFrameWnd)

const int  iMaxUserToolbars		= 10;
const UINT uiFirstUserToolBarId	= AFX_IDW_CONTROLBAR_FIRST + 40;
const UINT uiLastUserToolBarId	= uiFirstUserToolBarId + iMaxUserToolbars - 1;

BEGIN_MESSAGE_MAP(CMainFrame, CMDIFrameWnd)
	//{{AFX_MSG_MAP(CMainFrame)
	ON_WM_CREATE()
	ON_COMMAND(ID_VIEW_BUILD_TOOLBAR, OnViewBuildToolbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_BUILD_TOOLBAR, OnUpdateViewBuildToolbar)
	ON_WM_CLOSE()
	ON_COMMAND(ID_VIEW_CUSTOMIZE, OnViewCustomize)
	ON_UPDATE_COMMAND_UI(ID_VIEW_SIMULATION, OnUpdateViewSimulation)
	ON_COMMAND(ID_VIEW_SIMULATION, OnViewSimulation)
	ON_COMMAND(ID_VIEW_FORMAT_TOOLBAR, OnViewFormatToolbar)
	ON_UPDATE_COMMAND_UI(ID_VIEW_FORMAT_TOOLBAR, OnUpdateViewFormatToolbar)
	//}}AFX_MSG_MAP
	// Global help commands
	ON_REGISTERED_MESSAGE(BCGM_RESETTOOLBAR, OnToolbarReset)
	ON_REGISTERED_MESSAGE(BCGM_TOOLBARMENU, OnToolbarContextMenu)
	ON_COMMAND_EX_RANGE(ID_VIEW_USER_TOOLBAR1, ID_VIEW_USER_TOOLBAR10, OnToolsViewUserToolbar)
	ON_UPDATE_COMMAND_UI_RANGE(ID_VIEW_USER_TOOLBAR1, ID_VIEW_USER_TOOLBAR10, OnUpdateToolsViewUserToolbar)
	ON_REGISTERED_MESSAGE(BCGM_CUSTOMIZEHELP, OnHelpCustomizeToolbars)
	ON_COMMAND(ID_HELP_FINDER, CMDIFrameWnd::OnHelpFinder)
	ON_COMMAND(ID_HELP, CMDIFrameWnd::OnHelp)
	ON_COMMAND(ID_CONTEXT_HELP, CMDIFrameWnd::OnContextHelp)
	ON_COMMAND(ID_DEFAULT_HELP, CMDIFrameWnd::OnHelpFinder)
	ON_REGISTERED_MESSAGE(CPN_SELCHANGE, OnChangeColor)
END_MESSAGE_MAP()

static UINT indicators[] =
{
	ID_SEPARATOR,           // status line indicator
	ID_INDICATOR_CAPS,
	ID_INDICATOR_NUM,
	ID_INDICATOR_SCRL,
};

/////////////////////////////////////////////////////////////////////////////
// CMainFrame construction/destruction

CMainFrame::CMainFrame()
{
	// TODO: add member initialization code here
	
}

CMainFrame::~CMainFrame()
{
}

int CMainFrame::OnCreate(LPCREATESTRUCT lpCreateStruct)
{
	if (CMDIFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	//--------------------------
	// Load toolbar user images:
	//--------------------------
	if (!m_UserImages.Load (_T(".\\UserImages.bmp")))
	{
		TRACE(_T("Failed to load user images\n"));
		ASSERT (FALSE);
	}
	else
	{
		CBCGToolBar::SetUserImages (&m_UserImages);
	}

	CBCGToolBar::EnableQuickCustomization ();


	// TODO: Define your own basic commands. Be sure, that each pulldown 
	// menu have at least one basic command.

	CList<UINT, UINT>	lstBasicCoomads;

	lstBasicCoomads.AddTail (ID_FILE_NEW);
	lstBasicCoomads.AddTail (ID_FILE_OPEN);
	lstBasicCoomads.AddTail (ID_FILE_SAVE);
	lstBasicCoomads.AddTail (ID_FILE_PRINT);
	lstBasicCoomads.AddTail (ID_APP_EXIT);
	lstBasicCoomads.AddTail (ID_EDIT_CUT);
	lstBasicCoomads.AddTail (ID_EDIT_PASTE);
	lstBasicCoomads.AddTail (ID_EDIT_UNDO);
	lstBasicCoomads.AddTail (ID_APP_ABOUT);
	lstBasicCoomads.AddTail (ID_VIEW_TOOLBAR);
	lstBasicCoomads.AddTail (ID_VIEW_STATUS_BAR);
	lstBasicCoomads.AddTail (ID_VIEW_CUSTOMIZE);
	lstBasicCoomads.AddTail (ID_VIEW_TOOLBARS);
	lstBasicCoomads.AddTail (ID_OBJECT_LINECOLOR);
	lstBasicCoomads.AddTail (ID_OBJECT_FILLCOLOR);
	lstBasicCoomads.AddTail (ID_OBJECT_MOVETOFRONT);
	lstBasicCoomads.AddTail (ID_DUMMY_COMPILE);
	lstBasicCoomads.AddTail (ID_DUMMY_BUILD);
	lstBasicCoomads.AddTail (ID_DUMMY_GO);
	lstBasicCoomads.AddTail (ID_DUMMY_EXECUTE);
	lstBasicCoomads.AddTail (ID_DUMMY_SELECT_ACTIVE_CONFIGURATION);

	lstBasicCoomads.AddTail (ID_WINDOW_TILE_HORZ);
	CBCGToolBar::SetBasicCommands (lstBasicCoomads);

	//-----------------
	// Create menu bar:
	//-----------------
	if (!m_wndMenuBar.Create (this))
	{
		TRACE0("Failed to create menubar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.SetBarStyle(m_wndMenuBar.GetBarStyle() | CBRS_SIZE_DYNAMIC);

	//---------------------
	// Create main toolbar:
	//---------------------
	if (!m_wndToolBar.CreateEx(this, TBSTYLE_FLAT, WS_CHILD | WS_VISIBLE | CBRS_TOP
		| CBRS_GRIPPER | CBRS_TOOLTIPS | CBRS_FLYBY | CBRS_SIZE_DYNAMIC) ||
		!m_wndToolBar.LoadToolBar(IDR_MAINFRAME))
	{
		TRACE0("Failed to create toolbar\n");
		return -1;      // fail to create
	}

	CString strMainToolbarTitle;
	strMainToolbarTitle.LoadString (IDS_MAIN_TOOLBAR);
	m_wndToolBar.SetWindowText (strMainToolbarTitle);

	//----------------------
	// Create JTDraw toolbar:
	//----------------------
	if (!m_netToolBar.Create(this,
		WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_HIDE_INPLACE|CBRS_SIZE_DYNAMIC|
		CBRS_GRIPPER | CBRS_BORDER_3D,
		ID_VIEW_BUILD_TOOLBAR) ||
		!m_netToolBar.LoadToolBar (IDR_JTDRAW))
	{
		TRACE0("Failed to create build toolbar\n");
		return FALSE;      // fail to create
	}

	CString strNetToolbarTitle;
	strNetToolbarTitle.LoadString (IDS_BUILD_TOOLBAR);
	m_netToolBar.SetWindowText (strNetToolbarTitle);

	//----------------------
	// Create format toolbar:
	//----------------------
	if (!m_formatToolBar.Create(this,
		WS_CHILD|WS_VISIBLE|CBRS_TOP|CBRS_TOOLTIPS|CBRS_FLYBY|CBRS_HIDE_INPLACE|CBRS_SIZE_DYNAMIC|
		CBRS_GRIPPER | CBRS_BORDER_3D,
		ID_VIEW_FORMAT_TOOLBAR) ||
		!m_formatToolBar.LoadToolBar (IDR_FORMAT_TOOLBAR))
	{
		TRACE0("Failed to create build toolbar\n");
		return FALSE;      // fail to create
	}

	CString strFormatToolbarTitle;
	strFormatToolbarTitle.LoadString (IDR_FORMAT_TOOLBAR);
	m_formatToolBar.SetWindowText (strFormatToolbarTitle);

	//-------------------
	// Create status bar:
	//-------------------
	if (!m_wndStatusBar.Create(this) ||
		!m_wndStatusBar.SetIndicators(indicators,
		  sizeof(indicators)/sizeof(UINT)))
	{
		TRACE0("Failed to create status bar\n");
		return -1;      // fail to create
	}

	m_wndMenuBar.EnableDocking(CBRS_ALIGN_ANY);
	m_wndToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_netToolBar.EnableDocking(CBRS_ALIGN_ANY);
	m_formatToolBar.EnableDocking(CBRS_ALIGN_ANY);
    
	EnableDocking(CBRS_ALIGN_ANY);

	DockControlBar(&m_wndMenuBar);
	DockControlBar(&m_wndToolBar);
	DockControlBarLeftOf (&m_netToolBar, &m_wndToolBar);
	DockControlBar(&m_formatToolBar);

	//----------------------------------------
	// Allow user-defined toolbars operations:
	//----------------------------------------
	InitUserToobars (NULL,
					uiFirstUserToolBarId,
					uiLastUserToolBarId);

	return 0;
}

BOOL CMainFrame::PreCreateWindow(CREATESTRUCT& cs)
{
	if( !CMDIFrameWnd::PreCreateWindow(cs) )
		return FALSE;
	// TODO: Modify the Window class or styles here by modifying
	//  the CREATESTRUCT cs

	return TRUE;
}

/////////////////////////////////////////////////////////////////////////////
// CMainFrame diagnostics

#ifdef _DEBUG
void CMainFrame::AssertValid() const
{
	CMDIFrameWnd::AssertValid();
}

void CMainFrame::Dump(CDumpContext& dc) const
{
	CMDIFrameWnd::Dump(dc);
}

#endif //_DEBUG


LRESULT CMainFrame::OnToolbarContextMenu(WPARAM,LPARAM lp)
{
	CPoint point (LOWORD (lp), HIWORD(lp));

	CMenu menu;
	VERIFY(menu.LoadMenu (IDR_POPUP_TOOLBAR));

	CMenu* pPopup = menu.GetSubMenu(0);
	ASSERT(pPopup != NULL);

	SetupToolbarMenu (*pPopup, ID_VIEW_USER_TOOLBAR1, ID_VIEW_USER_TOOLBAR10);

	CBCGPopupMenu* pPopupMenu = new CBCGPopupMenu;
	pPopupMenu->Create (this, point.x, point.y, pPopup->Detach ());

	return 0;
}

void CMainFrame::OnToolsViewUserToolbar (UINT uiId)
{
	CBCGToolBar* pUserToolBar = GetUserBarByIndex (uiId - ID_VIEW_USER_TOOLBAR1);
	if (pUserToolBar == NULL)
	{
		ASSERT (FALSE);
		return;
	}

	ShowControlBar (pUserToolBar, !(pUserToolBar->GetStyle () & WS_VISIBLE), FALSE);
	RecalcLayout ();
}

void CMainFrame::OnUpdateToolsViewUserToolbar (CCmdUI* pCmdUI)
{
	CBCGToolBar* pUserToolBar = GetUserBarByIndex (pCmdUI->m_nID - ID_VIEW_USER_TOOLBAR1);
	if (pUserToolBar == NULL)
	{
		pCmdUI->Enable (FALSE);
		return;
	}

	pCmdUI->Enable ();
	pCmdUI->SetCheck (pUserToolBar->GetStyle () & WS_VISIBLE);
}

afx_msg LRESULT CMainFrame::OnToolbarReset(WPARAM wp,LPARAM)
{
	UINT uiToolBarId = (UINT) wp;

	switch (uiToolBarId)
	{
	case IDR_MAINFRAME:
		{
			//--------------------------------------
			// Replace "Find..." button by combobox:
			//--------------------------------------
			m_wndToolBar.ReplaceButton (ID_EDIT_FIND, 
				CBCGToolbarComboBoxButton (ID_EDIT_FIND_COMBO, 
				CImageHash::GetImageOfCommand (ID_EDIT_FIND, FALSE),
				CBS_DROPDOWN));

			//-------------------------------------
			// Replace "Undo" button by menubutton:
			//-------------------------------------
			CMenu menu;
			menu.LoadMenu (IDR_UNDO);

			CMenu* pPopupMenu = menu.GetSubMenu (0);
			VERIFY (pPopupMenu != NULL);

			m_wndToolBar.ReplaceButton (ID_EDIT_UNDO, CBCGToolbarMenuButton 
				(ID_EDIT_UNDO, pPopupMenu->GetSafeHmenu (), 
				CImageHash::GetImageOfCommand (ID_EDIT_UNDO, FALSE)));

			m_wndToolBar.ReplaceButton (ID_EDIT_REDO, CBCGToolbarMenuButton 
				(ID_EDIT_REDO, pPopupMenu->GetSafeHmenu (), 
				CImageHash::GetImageOfCommand (ID_EDIT_REDO, FALSE)));
		}
		break;

	case IDR_JTDRAW: /*
		{
			//-----------------------------------------------
			// Replace "Confoguration..." button by combobox:
			//-----------------------------------------------
			CBCGToolbarComboBoxButton comboButton (ID_DUMMY_SELECT_ACTIVE_CONFIGURATION, 
				CImageHash::GetImageOfCommand (ID_DUMMY_SELECT_ACTIVE_CONFIGURATION, FALSE),
				CBS_DROPDOWNLIST);
			comboButton.AddItem (_T("Win32 Debug"));
			comboButton.AddItem (_T("Win32 Release"));
			comboButton.SelectItem (0);

			m_netToolBar.ReplaceButton (ID_DUMMY_SELECT_ACTIVE_CONFIGURATION,
											comboButton);
		} */
		break;
	}

	return 0;
}

LRESULT CMainFrame::OnHelpCustomizeToolbars(WPARAM wp, LPARAM lp)
{
	int iPageNum = (int) wp;

	CBCGToolbarCustomize* pDlg = (CBCGToolbarCustomize*) lp;
	ASSERT_VALID (pDlg);

	// TODO: show help about page number iPageNum


	return 0;
}

BOOL CMainFrame::OnShowPopupMenu (CBCGPopupMenu* pMenuPopup)
{
    CBCGMDIFrameWnd::OnShowPopupMenu (pMenuPopup);

    if (pMenuPopup == NULL)
	{
		return TRUE;
	}

    if (pMenuPopup->GetMenuBar ()->CommandToIndex (ID_VIEW_TOOLBARS) >= 0)
    {
		if (CBCGToolBar::IsCustomizeMode ())
		{
			return FALSE;
		}

		pMenuPopup->RemoveAllItems ();

		CMenu menu;
		VERIFY(menu.LoadMenu (IDR_POPUP_TOOLBAR));

		CMenu* pPopup = menu.GetSubMenu(0);
		ASSERT(pPopup != NULL);

		SetupToolbarMenu (*pPopup, ID_VIEW_USER_TOOLBAR1, ID_VIEW_USER_TOOLBAR10);
		pMenuPopup->GetMenuBar ()->ImportFromMenu (*pPopup, TRUE);
    }

    if (pMenuPopup->GetMenuBar ()->CommandToIndex (ID_EDIT_UNDO_DUMMY) >= 0)
    {
		if (CBCGToolBar::IsCustomizeMode ())
		{
			return FALSE;
		}

		pMenuPopup->RemoveAllItems ();
		pMenuPopup->InsertItem (CBCGToolbarMenuButton (
			IDS_DUMMY_UNDO_ACTION, NULL, -1, _T("Typing")));
    }

	return TRUE;
}


void CMainFrame::OnViewBuildToolbar() 
{
	ShowControlBar (&m_netToolBar,
					!(m_netToolBar.GetStyle () & WS_VISIBLE),
					FALSE);
}

void CMainFrame::OnUpdateViewBuildToolbar(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck (m_netToolBar.GetStyle () & WS_VISIBLE);
}

void CMainFrame::OnClose() 
{
	CMDIFrameWnd::OnClose();
}

void CMainFrame::OnViewCustomize()
{
	//------------------------------------
	// Create a customize toolbars dialog:
	//------------------------------------
	CBCGToolbarCustomize* pDlgCust = new CBCGToolbarCustomize (this,
		TRUE /* Automatic menus scaning */);

	pDlgCust->EnableUserDefinedToolbars ();

	//-----------------
	// Setup combboxes:
	//-----------------
	CBCGToolbarComboBoxButton comboButtonFind (ID_EDIT_FIND_COMBO, 
			CImageHash::GetImageOfCommand (ID_EDIT_FIND, FALSE),
			CBS_DROPDOWN);
	pDlgCust->ReplaceButton (ID_EDIT_FIND, comboButtonFind);

	CBCGToolbarComboBoxButton comboButtonConfig (ID_DUMMY_SELECT_ACTIVE_CONFIGURATION, 
			CImageHash::GetImageOfCommand (ID_DUMMY_SELECT_ACTIVE_CONFIGURATION, FALSE),
			CBS_DROPDOWNLIST);
	comboButtonConfig.AddItem (_T("Win32 Debug"));
	comboButtonConfig.AddItem (_T("Win32 Release"));
	comboButtonConfig.SelectItem (0);

	pDlgCust->ReplaceButton (ID_DUMMY_SELECT_ACTIVE_CONFIGURATION, comboButtonConfig);

	//-------------------------
	// Setup undo/redo buttons:
	//-------------------------
	CMenu menu;
	menu.LoadMenu (IDR_UNDO);

	CMenu* pPopupMenu = menu.GetSubMenu (0);
	VERIFY (pPopupMenu != NULL);

	pDlgCust->ReplaceButton (ID_EDIT_UNDO, CBCGToolbarMenuButton 
		(ID_EDIT_UNDO, pPopupMenu->GetSafeHmenu (), 
		CImageHash::GetImageOfCommand (ID_EDIT_UNDO, FALSE)));

	pDlgCust->ReplaceButton (ID_EDIT_REDO, CBCGToolbarMenuButton 
		(ID_EDIT_REDO, pPopupMenu->GetSafeHmenu (), 
		CImageHash::GetImageOfCommand (ID_EDIT_REDO, FALSE)));

	pDlgCust->Create ();
}

void CMainFrame::OnUpdateViewSimulation(CCmdUI* pCmdUI) 
{
	pCmdUI->SetCheck (m_SimulateToolBar.GetStyle () & WS_VISIBLE);
	
}

void CMainFrame::OnViewSimulation() 
{
	ShowControlBar (&m_SimulateToolBar,
					!(m_SimulateToolBar.GetStyle () & WS_VISIBLE),
					FALSE);
	
}

LRESULT CMainFrame::OnChangeColor(WPARAM wp, LPARAM lp)
{
//	CJTDrawView* pView = DYNAMIC_DOWNCAST (CJTDrawView, GetActiveView());
//  GetActivateView doesn't work for CMDIFrameWnd
//  Use following code, to get the pointer of active View
	//1, Get the active MDI child window.
	//CMDIFrameWnd *pFrame = (CMDIFrameWnd*)AfxGetApp()->m_pMainWnd;
	//2. Get the active view attached to the active MDI child window.
	CMDIChildWnd *pChild = (CMDIChildWnd *) GetActiveFrame();
    CJTDrawView* pView = DYNAMIC_DOWNCAST (CJTDrawView, pChild->GetActiveView());
	
    if (pView != NULL)
	{
		switch ((UINT)lp){
		case ID_CHAR_COLOR:
			pView->SetTextColor ((COLORREF) wp);
			pView->OnCharColor();
			break;
		case ID_LINE_COLOR:
			pView->SetLineColor ((COLORREF) wp);
			pView->OnLineColor();
			break;
		case ID_FILL_COLOR:
			pView->SetFillColor ((COLORREF) wp);
			pView->OnFillColor();
			break;
		}			
	}

	return 0l;
}

void CMainFrame::OnViewFormatToolbar() 
{
	// TODO: Add your command handler code here
	ShowControlBar (&m_formatToolBar,
					!(m_formatToolBar.GetStyle () & WS_VISIBLE),
					FALSE);
	
	
}

void CMainFrame::OnUpdateViewFormatToolbar(CCmdUI* pCmdUI) 
{
	// TODO: Add your command update UI handler code here
	pCmdUI->SetCheck (m_formatToolBar.GetStyle () & WS_VISIBLE);
	
}

