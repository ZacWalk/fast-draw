// formatba.cpp : implementation file
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.

#include "stdafx.h"

#include "jtdraw.h"
#include "jtdrawdoc.h"
#include "jtdrawview.h"
#include "formatba.h"
//#include "colorlis.h"
//#include "FontCombo.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static CSize GetBaseUnits(CFont* pFont)
{
/*	ASSERT(pFont != NULL);
	ASSERT(pFont->GetSafeHandle() != NULL);
	pFont = theApp.m_dcScreen.SelectObject(pFont);
	TEXTMETRIC tm;
	VERIFY(theApp.m_dcScreen.GetTextMetrics(&tm));

	theApp.m_dcScreen.SelectObject(pFont);
	return CSize(tm.tmAveCharWidth, tm.tmHeight);*/
	return CSize(8,8);
}

IMPLEMENT_SERIAL(CFormatBar, CBCGToolBar, 1)

CFormatBar::CFormatBar()
{
/*	m_textColorButton=new CColorMenuButton(ID_CHAR_COLOR);
	m_lineColorButton=new CColorMenuButton(ID_LINE_COLOR);
	m_fillColorButton=new CColorMenuButton(ID_FILL_COLOR);*/
/*	CFont fnt;
	fnt.Attach(GetStockObject(theApp.m_nDefFont));
	m_szBaseUnits = GetBaseUnits(&fnt);
	CLocalComboBox::m_nFontHeight = m_szBaseUnits.cy;
*/
}
CFormatBar::~CFormatBar(){
/*	delete m_textColorButton;
	delete m_lineColorButton;
	delete m_fillColorButton;*/
}



BEGIN_MESSAGE_MAP(CFormatBar, CBCGToolBar)
	//{{AFX_MSG_MAP(CFormatBar)
	//}}AFX_MSG_MAP
	// Global help commands
END_MESSAGE_MAP()

void CFormatBar::OnReset ()
{
	int iIndex;

	// Insert font name combo:
/*	CFontNameComboBoxButton comboButtonFontName (IDC_FONTNAME, 
		CImageHash::GetImageOfCommand (IDC_FONTNAME, FALSE),
		WS_TABSTOP | WS_VISIBLE | WS_TABSTOP | WS_VSCROLL | CBS_DROPDOWN |
		CBS_SORT | CBS_AUTOHSCROLL | CBS_HASSTRINGS | CBS_OWNERDRAWFIXED,
		(3*LF_FACESIZE*m_szBaseUnits.cx)/2);

	iIndex = CommandToIndex (IDC_FONTNAME);
	RemoveButton (iIndex);

	if (iIndex > GetCount ())
	{
		iIndex = GetCount ();
	}

	InsertButton (comboButtonFontName, iIndex);

	// Insert font size combo:
	CFontSizeComboBoxButton comboButtonFontSize (IDC_FONTSIZE, 
		CImageHash::GetImageOfCommand (IDC_FONTSIZE, FALSE),
		WS_TABSTOP|WS_VISIBLE|WS_TABSTOP|WS_VSCROLL|CBS_DROPDOWN,
		10*m_szBaseUnits.cx + 10);

	iIndex = CommandToIndex (IDC_FONTSIZE);
	RemoveButton (iIndex);

	if (iIndex > GetCount ())
	{
		iIndex = GetCount ();
	}

	InsertButton (comboButtonFontSize, iIndex);

*/	// Insert the color menu button:
	CColorMenuButton colorButton(ID_CHAR_COLOR);

	iIndex = CommandToIndex (ID_CHAR_COLOR);
	RemoveButton (iIndex);

	if (iIndex > GetCount ())
	{
		iIndex = GetCount ();
	}

	InsertButton (colorButton, iIndex);
	
	CColorMenuButton colorButton1(ID_LINE_COLOR);
	iIndex = CommandToIndex (ID_LINE_COLOR);
	RemoveButton (iIndex);

	if (iIndex > GetCount ())
	{
		iIndex = GetCount ();
	}

	InsertButton (colorButton1, iIndex);
	CColorMenuButton colorButton2(ID_FILL_COLOR);
	iIndex = CommandToIndex (ID_FILL_COLOR);
	RemoveButton (iIndex);

	if (iIndex > GetCount ())
	{
		iIndex = GetCount ();
	}

	InsertButton (colorButton2, iIndex); 
}
//***************************************************************************************
void CFormatBar::OnUpdateCmdUI(CFrameWnd* pTarget, BOOL bDisableIfNoHndler)
{
	CBCGToolBar::OnUpdateCmdUI(pTarget, bDisableIfNoHndler);

	// don't update combo boxes if either one has the focus
/*	if (!CLocalComboBox::m_bHasFocus)
	{
		CLocalComboBox::SyncToView(this);
	}
	*/
}

