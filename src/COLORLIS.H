// colorlis.h : header file
//

#include "BCGToolbarMenuButton.h"
#include "ColourPopup.h"

class CColorMenuButton : public CBCGToolbarMenuButton  
{
	DECLARE_SERIAL(CColorMenuButton)

public:
	//CColorMenuButton();
	CColorMenuButton(UINT id=ID_FILL_COLOR);

	virtual ~CColorMenuButton();

	virtual void OnDraw (CDC* pDC, const CRect& rect, CBCGToolBarImages* pImages,
						BOOL bHorz = TRUE, BOOL bCustomizeMode = FALSE,
						BOOL bHighlight = FALSE,
						BOOL bDrawBorder = TRUE,
						BOOL bGrayDisabledButtons = TRUE);
	virtual CBCGPopupMenu* CreatePopupMenu ()
	{
		return new CColourPopup (m_Color, COLOR_WINDOWTEXT, m_nID);
		//can I create a CWnd object instead of CBCGPopupMenu Object here? No

	}

	virtual BOOL IsEmptyMenuAllowed () const
	{
		return TRUE;
	}

	virtual void OnChangeParentWnd (CWnd* pWndParent);
	void SetColor (COLORREF clr, BOOL bNotify = TRUE);

protected:
	COLORREF	m_Color;
};


/////////////////////////////////////////////////////////////////////////////
