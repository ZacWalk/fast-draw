// CntrItem.h : interface of the CJTDrawCntrItem class
//

#if !defined(AFX_CNTRITEM_H__64222BC6_A5DC_11D3_B4DC_00A0C995EA3D__INCLUDED_)
#define AFX_CNTRITEM_H__64222BC6_A5DC_11D3_B4DC_00A0C995EA3D__INCLUDED_

#if _MSC_VER > 1000
#pragma once
#endif // _MSC_VER > 1000

class CJTDrawDoc;
class CJTDrawView;

class CJTDrawCntrItem : public COleClientItem
{
	DECLARE_SERIAL(CJTDrawCntrItem)

// Constructors
public:
	CJTDrawCntrItem(CJTDrawDoc* pContainer=NULL ,CDrawOleObj* pDrawObj=NULL);//: COleClientItem(pContainer){}
		// Note: pContainer is allowed to be NULL to enable IMPLEMENT_SERIALIZE.
		//  IMPLEMENT_SERIALIZE requires the class have a constructor with
		//  zero arguments.  Normally, OLE items are constructed with a
		//  non-NULL document pointer.

// Attributes
public:
	CJTDrawDoc* GetDocument()
		{ return (CJTDrawDoc*)COleClientItem::GetDocument(); }
	CJTDrawView* GetActiveView()
		{ return (CJTDrawView*)COleClientItem::GetActiveView(); }

	// ClassWizard generated virtual function overrides
	//{{AFX_VIRTUAL(CJTDrawCntrItem)
	public:
	virtual void OnChange(OLE_NOTIFICATION wNotification, DWORD dwParam);
	virtual void OnActivate();
	protected:
	virtual void OnGetItemPosition(CRect& rPosition);
	virtual void OnDeactivateUI(BOOL bUndoable);
	virtual BOOL OnChangeItemPosition(const CRect& rectPos);
	//}}AFX_VIRTUAL
// Operations
public:
		BOOL UpdateExtent();

// Implementation
public:
	~CJTDrawCntrItem();
#ifdef _DEBUG
	virtual void AssertValid() const;
	virtual void Dump(CDumpContext& dc) const;
#endif
	virtual void Serialize(CArchive& ar);
public:	
	CDrawOleObj* m_pDrawObj;    // back pointer to OLE draw object
};

/////////////////////////////////////////////////////////////////////////////

//{{AFX_INSERT_LOCATION}}
// Microsoft Visual C++ will insert additional declarations immediately before the previous line.

#endif // !defined(AFX_CNTRITEM_H__64222BC6_A5DC_11D3_B4DC_00A0C995EA3D__INCLUDED_)
