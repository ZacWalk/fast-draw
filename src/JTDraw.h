// JTDraw.h : main header file for the METLAB application
//

#if !defined(AFX_METLAB_H__64222BB5_A5DC_11D3_B4DC_00A0C995EA3D__INCLUDED_)
#define AFX_METLAB_H__64222BB5_A5DC_11D3_B4DC_00A0C995EA3D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#ifndef __AFXWIN_H__
	#error include 'stdafx.h' before including this file for PCH
#endif

#include "resource.h"       // main symbols

/////////////////////////////////////////////////////////////////////////////
// CJTDrawApp:
// See JTDraw.cpp for the implementation of this class
//

class CJTDrawApp : public CWinApp, public CBCGWorkspace
{
public:
	CJTDrawApp();

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJTDrawApp)
	public:
	virtual BOOL InitInstance();
	//}}AFX_VIRTUAL

// Override from CBCGWorkspace
	virtual void PreLoadState();
// Implementation
	COleTemplateServer m_server;
		// Server object for document creation
	//{{AFX_MSG(CJTDrawApp)
	afx_msg void OnAppAbout();
		// NOTE - the ClassWizard will add and remove member functions here.
		//    DO NOT EDIT what you see in these blocks of generated code !
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};


/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_METLAB_H__64222BB5_A5DC_11D3_B4DC_00A0C995EA3D__INCLUDED_)
