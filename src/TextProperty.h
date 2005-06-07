#if !defined(AFX_TEXTPROPERTY_H__04B0EAD7_ACD3_11D3_B929_0050DA078B89__INCLUDED_)
#define AFX_TEXTPROPERTY_H__04B0EAD7_ACD3_11D3_B929_0050DA078B89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// TextProperty.h : header file
// Written by Tao Zhu (tzzt@yahoo.com) and Jian Zhu (jianz2@yahoo.com)
//
// This code may be used in compiled form in any way you desire. This
// file may be redistributed unmodified by any means PROVIDING it is 
// not sold for profit without the authors written consent, and 
// providing that this notice and the authors name is included. If 
// the source code in  this file is used in any commercial application 
// then a simple email would be nice.
//
// This file is provided "as is" with no expressed or implied warranty.
// The author accepts no liability if it causes any damage whatsoever.
#include "ColourPicker.h"

/////////////////////////////////////////////////////////////////////////////
// CTextProperty dialog


class CTextProperty : public CPropertyPage
{
	DECLARE_DYNCREATE(CTextProperty)

// Construction
public:
	CTextProperty();
	~CTextProperty();

// Dialog Data
	//{{AFX_DATA(CTextProperty)
	enum { IDD = IDD_TEXT };
	CString	m_text;
	int		m_borderType;
	int		m_allignment;
	//}}AFX_DATA
//	COLORREF m_textColor;
//    CFontComboBox fontCombo;
    bool m_bFont;
	static LOGFONT NEAR m_lfDefFont;
	static LOGFONT NEAR m_lfDefFontOld;
	CFont m_font;

	static LOGFONT NEAR m_lfDefPrintFont;
	static LOGFONT NEAR m_lfDefPrintFontOld;
	CFont m_fontPrint;
    COLORREF m_color;
	bool m_isBold;
	bool m_isItalic;
	bool m_isUnder;
	bool m_isStrike;
    int m_fontSize;
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CTextProperty)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CTextProperty)
	afx_msg void OnFont();
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_TEXTPROPERTY_H__04B0EAD7_ACD3_11D3_B929_0050DA078B89__INCLUDED_)
