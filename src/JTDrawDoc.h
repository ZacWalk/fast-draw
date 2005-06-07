// JTDrawDoc.h : interface of the CJTDrawDoc class
//
/////////////////////////////////////////////////////////////////////////////

#if !defined(AFX_METLABDOC_H__64222BBD_A5DC_11D3_B4DC_00A0C995EA3D__INCLUDED_)
#define AFX_METLABDOC_H__64222BBD_A5DC_11D3_B4DC_00A0C995EA3D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

#include "drawobj.h"
#include "summinfo.h"

class CJTDrawSrvrItem;

class CJTDrawView;

class CJTDrawDoc : public COleDocument
{
protected: // create from serialization only
	CJTDrawDoc();
	DECLARE_DYNCREATE(CJTDrawDoc)

// Attributes
public:
	CDrawObjList* GetObjects() { return &m_objects; }
	const CSize& GetSize() const { return m_size; }
	void ComputePageSize();
	int GetMapMode() const { return m_nMapMode; }
	COLORREF GetPaperColor() const { return m_paperColor; }
	CSummInfo *m_pSummInfo;

// Operations
public:
	CDrawObj* ObjectAt(const CPoint& point);
	void Draw(CDC* pDC, CJTDrawView* pView);
	void Add(CDrawObj* pObj);
	void Remove(CDrawObj* pObj);

// Overrides
	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJTDrawDoc)
	public:
	virtual BOOL OnNewDocument();
	virtual void Serialize(CArchive& ar);
	//}}AFX_VIRTUAL

// Implementation
public:
	virtual ~CJTDrawDoc();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif

protected:
	virtual BOOL OnOpenDocument(LPCTSTR lpszPathName);
	virtual BOOL OnSaveDocument(LPCTSTR lpszPathName);

	CDrawObjList m_objects;
	CSize m_size;
	int m_nMapMode;
	COLORREF m_paperColor;

// Generated message map functions
protected:
	//{{AFX_MSG(CJTDrawDoc)
	afx_msg void OnFileSummaryinfo();
	afx_msg void OnViewPapercolor();
	//}}AFX_MSG
	DECLARE_MESSAGE_MAP()
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_METLABDOC_H__64222BBD_A5DC_11D3_B4DC_00A0C995EA3D__INCLUDED_)
