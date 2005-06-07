// DrawProperty.cpp : implementation file
//

#include "stdafx.h"
#include "jtdraw.h"
#include "DrawProperty.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#undef THIS_FILE
static char THIS_FILE[] = __FILE__;
#endif

/////////////////////////////////////////////////////////////////////////////
// CDrawProperty

IMPLEMENT_DYNAMIC(CDrawProperty, CPropertySheet)

CDrawProperty::CDrawProperty(UINT nIDCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(nIDCaption, pParentWnd, iSelectPage)
{
}

CDrawProperty::CDrawProperty(LPCTSTR pszCaption, CWnd* pParentWnd, UINT iSelectPage)
	:CPropertySheet(pszCaption, pParentWnd, iSelectPage)
{
}

CDrawProperty::~CDrawProperty()
{
}


BEGIN_MESSAGE_MAP(CDrawProperty, CPropertySheet)
	//{{AFX_MSG_MAP(CDrawProperty)
		// NOTE - the ClassWizard will add and remove mapping macros here.
	//}}AFX_MSG_MAP
END_MESSAGE_MAP()

/////////////////////////////////////////////////////////////////////////////
// CDrawProperty message handlers
