// drawobj.h - interface for CDrawObj and derivatives
// Written by Tao Zhu (tzzt@yahoo.com) and Jian Zhu (jianz2@yahoo.com)
// Part of Codes is based on MFC' sample -Drawcli.
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

#ifndef __DRAWOBJ_H__
#define __DRAWOBJ_H__

#include "stdafx.h"
#include "line.h"

class CJTDrawView;
class CJTDrawDoc;

/////////////////////////////////////////////////////////////////////////////
// CDrawObj - base class for all 'drawable objects'
/////////////////////////////////////////////////////////////////////////////
class CDrawObj : public CObject
{
protected:
	DECLARE_SERIAL(CDrawObj);
	CDrawObj();

// Constructors
public:
	CDrawObj(const CRect& position);

// Attributes
	CRect m_position;
	CJTDrawDoc* m_pDocument;

	virtual int GetHandleCount();
	virtual CPoint GetHandle(int nHandle);
	CRect GetHandleRect(int nHandleID, CJTDrawView* pView);
	virtual HCURSOR GetHandleCursor(int nHandle);
	virtual void SetLineColor(COLORREF color);
	virtual void SetFillColor(COLORREF color);
	virtual void SetTextColor(COLORREF color);

// Operations
	virtual void Draw(CDC* pDC);
	enum TrackerState { normal, selected, active };
	virtual void DrawTracker(CDC* pDC, TrackerState state);
	virtual void MoveTo(const CRect& positon, CJTDrawView* pView = NULL);
	virtual int HitTest(CPoint point, CJTDrawView* pView, BOOL bSelected);
	virtual BOOL Intersects(const CRect& rect);
	virtual BOOL InRect(const CRect& rect);
	virtual void MoveHandleTo(int nHandle, CPoint point, CJTDrawView* pView = NULL);
	virtual void OnOpen(CJTDrawView* pView);
	virtual void OnEditProperties();
	virtual CDrawObj* Clone(CJTDrawDoc* pDoc = NULL);
	virtual void Remove();
	void Invalidate();

// Implementation
public:
	virtual ~CDrawObj();
	virtual void Serialize(CArchive& ar);
#ifdef _DEBUG
	void AssertValid();
#endif

	// implementation data
protected:
	BOOL m_bPen;
	LOGPEN m_logpen;
	BOOL m_bBrush;
	LOGBRUSH m_logbrush;
	CDashLine::DashStyle m_dstyle;
	bool m_round;  //round of dashline

	friend class CJTDrawView;
};

// special 'list' class for this application (requires afxtempl.h)
typedef CTypedPtrList<CObList, CDrawObj*> CDrawObjList;

////////////////////////////////////////////////////////////////////////
// specialized draw objects
////////////////////////////////////////////////////////////////////////

class CDrawRect : public CDrawObj
{
protected:
	DECLARE_SERIAL(CDrawRect);
	CDrawRect();

public:
	CDrawRect(const CRect& position);

// Implementation
public:
	virtual void Serialize(CArchive& ar);
	virtual void Draw(CDC* pDC);
	virtual int GetHandleCount();
	virtual CPoint GetHandle(int nHandle);
	virtual HCURSOR GetHandleCursor(int nHandle);
	virtual void MoveHandleTo(int nHandle, CPoint point, CJTDrawView* pView = NULL);
	virtual BOOL Intersects(const CRect& rect);
	virtual CDrawObj* Clone(CJTDrawDoc* pDoc);
	virtual void OnEditProperties();
public:
	enum ArrowType { none, pureArrow, openArrow,stealthArrow, diamondArrow, ovalArrow};
	enum Shape { rectangle, roundRectangle, ellipse, line,arrow };

protected:
	Shape m_nShape;

	ArrowType m_lArrow;  //for arrow only
	ArrowType m_rArrow; //for arrow only
    int m_lArrowSize; //for arrow only
    int m_rArrowSize; //for arrow only
	
	CPoint m_roundness; // for roundRect corners
private:
	void DrawArrow(ArrowType arrowType, POINT p1, POINT p2, int arrowSize, CDC *,int, int , int, CBrush *);

	friend class CRectTool;
	friend class CJTDrawView;
	friend class CLineProperty;
	friend class CDrawObj;
};

/////////////////////////////////////////////////////////////////////////////
// CDrawPoly 
/////////////////////////////////////////////////////////////////////////////
class CDrawPoly;

class CDrawPoly : public CDrawObj
{
protected:
	DECLARE_SERIAL(CDrawPoly);
	CDrawPoly();

public:
	CDrawPoly(const CRect& position);

// Operations
	void AddPoint(const CPoint& point, CJTDrawView* pView = NULL);
	BOOL RecalcBounds(CJTDrawView* pView = NULL);

// Implementation
public:
	virtual ~CDrawPoly();
	virtual void Serialize(CArchive& ar);
	virtual void Draw(CDC* pDC);
	virtual void MoveTo(const CRect& position, CJTDrawView* pView = NULL);
	virtual int GetHandleCount();
	virtual CPoint GetHandle(int nHandle);
	virtual HCURSOR GetHandleCursor(int nHandle);
	virtual void MoveHandleTo(int nHandle, CPoint point, CJTDrawView* pView = NULL);
	virtual BOOL Intersects(const CRect& rect);
	virtual CDrawObj* Clone(CJTDrawDoc* pDoc);
	virtual void OnEditProperties();

protected:
	int m_nPoints;
	int m_nAllocPoints;
	CPoint* m_points;
	CDrawPoly* m_pDrawObj;

	friend class CPolyTool;
};

/////////////////////////////////////////////////////////////////////////////
// CDrawPoly 
/////////////////////////////////////////////////////////////////////////////
class CJTDrawCntrItem;    // COleClientItem derived class
class CDrawOleObj : public CDrawObj
{
protected:
	DECLARE_SERIAL(CDrawOleObj);
	CDrawOleObj();

public:
	CDrawOleObj(const CRect& position);

// Implementation
public:
	virtual void Serialize(CArchive& ar);
	virtual void Draw(CDC* pDC);
	virtual CDrawObj* Clone(CJTDrawDoc* pDoc);
	virtual void OnOpen(CJTDrawView* pView);
	virtual void MoveTo(const CRect& positon, CJTDrawView* pView = NULL);
	virtual void OnEditProperties();
	virtual void Remove();
	virtual ~CDrawOleObj();

	static BOOL c_bShowItems;

	CJTDrawCntrItem* m_pClientItem;
	CSize m_extent; // current extent is tracked separate from scaled position
};

/////////////////////////////////////////////////////////////////////////////
// CDrawText 
/////////////////////////////////////////////////////////////////////////////
class CDrawText : public CDrawRect
{
protected:
	DECLARE_SERIAL(CDrawText);
	CDrawText();

public:
	CDrawText(const CRect& position);

// Implementation
public:
	virtual void Resize(CJTDrawView* pView);
	virtual void Serialize(CArchive& ar);
	virtual void Draw(CDC* pDC);
	virtual int GetHandleCount();
	virtual CPoint GetHandle(int nHandle);
	virtual HCURSOR GetHandleCursor(int nHandle);
	virtual void MoveHandleTo(int nHandle, CPoint point, CJTDrawView* pView = NULL);
	virtual BOOL Intersects(const CRect& rect);
	virtual CDrawObj* Clone(CJTDrawDoc* pDoc);
	virtual void OnEditProperties();
	virtual void SetTextColor(COLORREF color);
	CString m_text;
    int m_allignment;
protected:
	int MaxHeight(CDC* pDC);
	int TextLen(CDC* pDC);
	void Resize(int x, int y);
    int m_bBorder; //0 rect, 1 oval, 2 roundrect, 3, none
	LOGFONT m_logfont;
    COLORREF m_color; //font color
	friend class CTextTool;
	friend class CReactantTool;
};


#endif // __DRAWOBJ_H__
