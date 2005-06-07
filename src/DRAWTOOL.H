// drawtool.h - interface for CDrawTool and derivatives
//
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

#ifndef __DRAWTOOL_H__
#define __DRAWTOOL_H__

#include "drawobj.h"

class CJTDrawView;

enum DrawShape
{
	selection,
	line,
	rect,
	roundRect,
	ellipse,
	poly,
	arrow,
	text
};

class CDrawTool
{
// Constructors
public:
	CDrawTool(DrawShape nDrawShape);

// Overridables
	virtual void OnLButtonDown(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonDblClk(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonUp(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnMouseMove(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnEditProperties(CJTDrawView* pView);
	virtual void OnCancel();

// Attributes
	DrawShape m_drawShape;

	static CDrawTool* FindTool(DrawShape drawShape);
	static CPtrList c_tools;
	static CPoint c_down;
	static UINT c_nDownFlags;
	static CPoint c_last;
	static DrawShape c_drawShape;
};

class CSelectTool : public CDrawTool
{
// Constructors
public:
	CSelectTool();

// Implementation
	virtual void OnLButtonDown(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonDblClk(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonUp(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnMouseMove(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnEditProperties(CJTDrawView* pView);
};

class CRectTool : public CDrawTool
{
// Constructors
public:
	CRectTool(DrawShape drawShape);

// Implementation
	virtual void OnLButtonDown(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonDblClk(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonUp(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnMouseMove(CJTDrawView* pView, UINT nFlags, const CPoint& point);
};

class CTextTool : public CRectTool
{
// Constructors
public:
	CTextTool();

// Implementation
	virtual void OnLButtonDown(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonDblClk(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonUp(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnMouseMove(CJTDrawView* pView, UINT nFlags, const CPoint& point);
};

class CPolyTool : public CDrawTool
{
// Constructors
public:
	CPolyTool();

// Implementation
	virtual void OnLButtonDown(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonDblClk(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnLButtonUp(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnMouseMove(CJTDrawView* pView, UINT nFlags, const CPoint& point);
	virtual void OnCancel();

	CDrawPoly* m_pDrawObj;
};

////////////////////////////////////////////////////////////////////////////

#endif // __DRAWTOOL_H__
