#if !defined(AFX_LINEPROPERTY_H__04B0EAD2_ACD3_11D3_B929_0050DA078B89__INCLUDED_)
#define AFX_LINEPROPERTY_H__04B0EAD2_ACD3_11D3_B929_0050DA078B89__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000
// LineProperty.h : header file
//
// Written by Tao Zhu (tzzt@yahoo.com) and Jian Zhu (jianz2@yahoo.com)
// Copyright (c) 1999.

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
#include "ArrowPicker.h"
#include "line.h"
#include "drawobj.h"
#include "linePicker.h"
/////////////////////////////////////////////////////////////////////////////
// CLineProperty dialog

class CLineProperty : public CPropertyPage
{
	DECLARE_DYNCREATE(CLineProperty)

// Construction
public:
	void EnableArrowSetting( bool set);
	CLineProperty();
	~CLineProperty();

// Dialog Data
	//{{AFX_DATA(CLineProperty)
	enum { IDD = IDD_LINE };
	CLinePicker	m_linePicker;
	CArrowPicker	m_arrowEndPicker;
	CArrowPicker	m_arrowStartPicker;
	CColourPicker	m_FillPicker;
	CColourPicker	m_LinePicker;
	BOOL	m_bNoFill;
	UINT	m_penSize;
	int	m_nArrowStart;
	int	m_nArrowEnd;
	BOOL	m_round;
	//}}AFX_DATA
	CDrawRect::ArrowType m_arrowStart;
	CDrawRect::ArrowType m_arrowEnd;
	CDashLine::DashStyle m_lineStyle;
	COLORREF m_LineColor;
	COLORREF m_FillColor;
	bool m_isArrow;
// Overrides
	// ClassWizard generate virtual function overrides
	//{{AFX_VIRTUAL(CLineProperty)
	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV support
	//}}AFX_VIRTUAL

// Implementation
protected:
	// Generated message map functions
	//{{AFX_MSG(CLineProperty)
	virtual BOOL OnInitDialog();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()

};

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_LINEPROPERTY_H__04B0EAD2_ACD3_11D3_B929_0050DA078B89__INCLUDED_)
