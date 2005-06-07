// colorlis.cpp : implementation file
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
#include "colorlis.h"
#include "ColourPopup.h"
#include "BCGMenuBar.h"
#include "MenuImages.h"
//#include "BCGPopupMenuBar.h"
#include "JTDrawdoc.h"
#include "JTDrawView.h"

#ifdef _DEBUG
#undef THIS_FILE
static char BASED_CODE THIS_FILE[] = __FILE__;
#endif

static const int SEPARATOR_SIZE = 2;

//////////////////////////////////////////////////////////////////////
// CColorMenuButton Class
//////////////////////////////////////////////////////////////////////

IMPLEMENT_SERIAL(CColorMenuButton, CBCGToolbarMenuButton, VERSIONABLE_SCHEMA | 1)

//////////////////////////////////////////////////////////////////////
// Construction/Destruction
//////////////////////////////////////////////////////////////////////

/*CColorMenuButton::CColorMenuButton() :
	CBCGToolbarMenuButton (ID_CHAR_COLOR, NULL,
		CImageHash::GetImageOfCommand (ID_CHAR_COLOR, FALSE))
{
	m_strText = _T("Text Color...");
	m_Color = (COLORREF) -1;	// Automatic

	CFrameWnd* pFrame = DYNAMIC_DOWNCAST (CFrameWnd, AfxGetApp()->m_pMainWnd);
	if (pFrame != NULL)
	{
		CJTDrawView* pView = DYNAMIC_DOWNCAST (CJTDrawView, pFrame->GetActiveView());
		if (pView != NULL)
		{
			m_Color = pView->m_clrDefault;
		}
	}
}

*/
CColorMenuButton::CColorMenuButton(UINT id) :
	CBCGToolbarMenuButton (id, NULL,
		CImageHash::GetImageOfCommand (id, FALSE))
{
	//m_strText = _T("Text Color...");
	m_Color = (COLORREF) -1;	// Automatic
	CFrameWnd* pFrame = DYNAMIC_DOWNCAST (CFrameWnd, AfxGetApp()->m_pMainWnd);
	if (pFrame != NULL)
	{
		CJTDrawView* pView = DYNAMIC_DOWNCAST (CJTDrawView, pFrame->GetActiveView());
		if (pView != NULL)
		{
			switch (id){
			case ID_CHAR_COLOR:
				m_Color = pView->m_textColor;
				break;
			case ID_LINE_COLOR:
  				m_Color = pView->m_lineColor;
				break;
			case ID_FILL_COLOR:
  				m_Color = pView->m_fillColor;
				break;
			}
		}
	}
}
//*****************************************************************************************
CColorMenuButton::~CColorMenuButton()
{
}
//*****************************************************************************************
void CColorMenuButton::OnDraw (CDC* pDC, const CRect& rect, CBCGToolBarImages* pImages,
			BOOL bHorz, BOOL bCustomizeMode, BOOL bHighlight,
			BOOL bDrawBorder, BOOL bGrayDisabledButtons)
{
	CBCGToolbarMenuButton::OnDraw (pDC, rect, pImages, bHorz, bCustomizeMode, 
		bHighlight, bDrawBorder, bGrayDisabledButtons);

	if (!IsDrawImage () || pImages == NULL)
	{
		return;
	}

	ASSERT (pImages != NULL);

	CRect rectColor = rect;
	const int iColorBoxSize = 5;

	if (m_bMenuMode)
	{
		rectColor.left += 2;
		rectColor.right = rectColor.left + pImages->GetImageSize ().cx + 2;
		rectColor.top = rectColor.bottom - iColorBoxSize;
	}
	else
	{
		if (m_bDrawDownArrow)
		{
			if (bHorz)
			{
				rectColor.right -= CMenuImages::Size ().cx + SEPARATOR_SIZE;
			}
			else
			{
				rectColor.bottom -= CMenuImages::Size ().cy + SEPARATOR_SIZE;
			}
		}

		int iTextLen = 0;

		if (IsDrawText ())
		{
			CString strWithoutAmp = m_strText;
			strWithoutAmp.Remove (_T('&'));

			iTextLen = pDC->GetTextExtent (strWithoutAmp).cx + 2 * TEXT_MARGIN;
		}

		int dx = bHorz ? rectColor.Width () - iTextLen : rect.Width ();
		int dy = bHorz ? rectColor.Height () : rect.Height () - iTextLen;

		rectColor.left += (dx - pImages->GetImageSize ().cx) / 2 + 1;
		rectColor.top += 2;

		if (bHorz)
		{
			rectColor.bottom -= 2;
			rectColor.top = rectColor.bottom - iColorBoxSize;
			rectColor.right = rectColor.left + pImages->GetImageSize ().cx;
		}
		else
		{
			rectColor.right -= 2;
			rectColor.left = rectColor.right - iColorBoxSize;
			rectColor.bottom = rectColor.top + pImages->GetImageSize ().cy;
		}
	}

	if (m_bMenuMode)
	{
		rectColor.DeflateRect (1, 0);
		rectColor.OffsetRect (1, -1);
	}
	else if (((m_nStyle & (TBBS_PRESSED | TBBS_CHECKED)) ||
		m_pPopupMenu != NULL) && !m_bClickedOnMenu)
	{
		rectColor.OffsetRect (1, 1);
	}

	pDC->Draw3dRect (rectColor, ::GetSysColor (COLOR_3DSHADOW),
								::GetSysColor (COLOR_3DLIGHT));
	
	rectColor.DeflateRect (1, 1);

	CBrush br (m_Color);
	pDC->FillRect (rectColor, &br);
}
//**********************************************************************************************
void CColorMenuButton::SetColor (COLORREF clr, BOOL bNotify)
{
	m_Color = clr;

	if (m_pWndParent->GetSafeHwnd () != NULL)
	{
		m_pWndParent->InvalidateRect (m_rect);
	}

	if (bNotify)
	{
		CObList listButtons;
		if (CBCGToolBar::GetCommandButtons (m_nID, listButtons) > 0)
		{
			for (POSITION pos = listButtons.GetHeadPosition (); pos != NULL;)
			{
				CColorMenuButton* pOther = 
					DYNAMIC_DOWNCAST (CColorMenuButton, listButtons.GetNext (pos));
				ASSERT (pOther != NULL);

				if (pOther != this)
				{
					pOther->SetColor (clr, FALSE);
				}
			}
		}
	}

}
//****************************************************************************************
void CColorMenuButton::OnChangeParentWnd (CWnd* pWndParent)
{
	if (pWndParent != NULL)
	{
		if (pWndParent->IsKindOf (RUNTIME_CLASS (CBCGMenuBar)))
		{
			m_bText = TRUE;
		}

		if (pWndParent->IsKindOf (RUNTIME_CLASS (CBCGPopupMenuBar)))
		{
			m_bMenuMode = TRUE;
			m_bText = TRUE;
		}
		else
		{
			m_bMenuMode = FALSE;
		}
	}

	m_bDrawDownArrow = TRUE;
	m_pWndParent = pWndParent;
}
