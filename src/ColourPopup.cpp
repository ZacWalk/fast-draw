#include "stdafx.h"
#include <math.h>
#include "ColourPopup.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CColourPopup

IMPLEMENT_SERIAL(CColourPopup, CBCGPopupMenu, 1)

CColourPopup::CColourPopup(COLORREF color, BOOL bAutomatic, UINT id) :
	m_wndColorBar (color, bAutomatic,id)
{
//do nothing
}

CColourPopup::~CColourPopup()
{
}

BEGIN_MESSAGE_MAP(CColourPopup, CBCGPopupMenu)
    //{{AFX_MSG_MAP(CColourPopup)
	ON_WM_CREATE()
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CColourPopup message handlers

int CColourPopup::OnCreate(LPCREATESTRUCT lpCreateStruct) 
{
	if (CBCGToolBar::IsCustomizeMode ())
	{
		return -1;
	}

	if (CMiniFrameWnd::OnCreate(lpCreateStruct) == -1)
		return -1;
	
	if (!m_wndColorBar.Create (this, dwDefaultToolbarStyle | CBRS_TOOLTIPS | CBRS_FLYBY))
	{
		TRACE(_T("Can't create popup menu bar\n"));
		return -1;
	}

	m_wndColorBar.SetOwner (GetParent ());
	m_wndColorBar.SetBarStyle(m_wndColorBar.GetBarStyle() | CBRS_TOOLTIPS);

	ActivatePopupMenu ((CFrameWnd*) AfxGetMainWnd (), this);
	RecalcLayout ();
	return 0;
}
