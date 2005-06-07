// MainFrm.h : interface of the CMainFrame class
//
/////////////////////////////////////////////////////////////////////////////
#ifndef MELAB_MAINFRAME_H
#define MELAB_MAINFRAME_H
#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#define CMDIFrameWnd CBCGMDIFrameWnd

#include "formatba.h"
class CMainFrame : public CMDIFrameWnd
{
	DECLARE_DYNAMIC(CMainFrame)
public:
	CMainFrame();

// Attributes
public:

// Operations
public:
//	void DockControlBarLeftOf(CToolBar* Bar,CToolBar* LeftOf);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CMainFrame)
	public:
	virtual BOOL PreCreateWindow(CREATESTRUCT& cs);
	//}}AFX_VIRTUAL
	virtual BOOL OnShowPopupMenu (CBCGPopupMenu* pMenuPopup);

// Implementation
public:
	virtual ~CMainFrame();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:  // control bar embedded members
	CBCGMenuBar		m_wndMenuBar;

	CBCGToolBar		m_wndToolBar;
	CBCGToolBar		m_netToolBar;
	CFormatBar		m_formatToolBar;
	CBCGToolBar		m_SimulateToolBar;

	CStatusBar		m_wndStatusBar;

	CBCGToolBarImages	m_UserImages;

// Generated message map functions
protected:
	//{{AFX_MSG(CMainFrame)
	afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct);
	afx_msg void OnViewBuildToolbar();
	afx_msg void OnUpdateViewBuildToolbar(CCmdUI* pCmdUI);
	afx_msg void OnClose();
	afx_msg void OnViewCustomize();
	afx_msg void OnUpdateViewSimulation(CCmdUI* pCmdUI);
	afx_msg void OnViewSimulation();
	afx_msg void OnViewFormatToolbar();
	afx_msg void OnUpdateViewFormatToolbar(CCmdUI* pCmdUI);
	//}}AFX_MSG
	afx_msg LRESULT OnToolbarReset(WPARAM,LPARAM);
	void OnToolsViewUserToolbar (UINT id);
	void OnUpdateToolsViewUserToolbar (CCmdUI* pCmdUI);
	afx_msg LRESULT OnToolbarContextMenu(WPARAM,LPARAM);
	afx_msg LRESULT OnHelpCustomizeToolbars(WPARAM wp, LPARAM lp);
	afx_msg LRESULT OnChangeColor(WPARAM, LPARAM);
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined MELAB_MAINFRAME_H
