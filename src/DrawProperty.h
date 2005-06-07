#if !defined(AFX_DRAWPROPERTY_H__04B0EAD1_ACD3_11D3_B929_0050DA078B89__INCLUDED_)
#define AFX_DRAWPROPERTY_H__04B0EAD1_ACD3_11D3_B929_0050DA078B89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// DrawProperty.h : header file
//

/////////////////////////////////////////////////////////////////////////////
// CDrawProperty

class CDrawProperty : public CPropertySheet
{
	DECLARE_DYNAMIC(CDrawProperty)

// Construction
public:
	CDrawProperty(UINT nIDCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);
	CDrawProperty(LPCTSTR pszCaption, CWnd* pParentWnd = NULL, UINT iSelectPage = 0);

// Attributes
public:

// Operations
public:

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CDrawProperty)
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CDrawProperty();

	// Generated message map functions
protected:
	//{{AFX_MSG(CDrawProperty)
		// NOTE - the ClassWizard will add and remove member functions here.
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_DRAWPROPERTY_H__04B0EAD1_ACD3_11D3_B929_0050DA078B89__INCLUDED_)
