// drawobj.cpp - implementation for drawing objects
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


#include "stdafx.h"
#include "jtdraw.h"

#include "JTDrawDoc.h"
#include "JTDrawview.h"
#include "drawobj.h"


#include "DrawProperty.h"
#include "LineProperty.h"
#include "TextProperty.h"
#include <math.h>

#include "line.h"
#include "cntritem.h"

#define PADDING 2

IMPLEMENT_SERIAL(CDrawObj, CObject, 0)

CDrawObj::CDrawObj()
{
}

CDrawObj::~CDrawObj()
{
}

CDrawObj::CDrawObj(const CRect& position)
{
	m_position = position;
	m_pDocument = NULL;

	m_bPen = TRUE;
	m_round = false;
	m_logpen.lopnStyle = PS_SOLID;
	m_logpen.lopnWidth.x = 1;
	m_logpen.lopnWidth.y = 1;
	m_logpen.lopnColor = RGB(0, 0, 0);

	m_bBrush = TRUE;
	m_logbrush.lbStyle = BS_SOLID ;
	m_logbrush.lbColor = RGB(0xC0, 255, 255); //sky blue
	m_logbrush.lbHatch = HS_HORIZONTAL;
    m_dstyle = CDashLine::DL_SOLID;

}

void CDrawObj::Serialize(CArchive& ar)
{
	CObject::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << m_position;
		ar << (WORD)m_bPen;
		ar.Write(&m_logpen, sizeof(LOGPEN));
		ar << (WORD)m_bBrush;
		ar.Write(&m_logbrush, sizeof(LOGBRUSH));
		ar<<(WORD)m_round;
		ar<<(WORD)m_dstyle;
	}
	else
	{
		// get the document back pointer from the archive
		m_pDocument = (CJTDrawDoc*)ar.m_pDocument;
		ASSERT_VALID(m_pDocument);
		ASSERT_KINDOF(CJTDrawDoc, m_pDocument);

		WORD wTemp;
		ar >> m_position;
		ar >> wTemp; m_bPen = (BOOL)wTemp;
		ar.Read(&m_logpen,sizeof(LOGPEN));
		ar >> wTemp; m_bBrush = (BOOL)wTemp;
		ar.Read(&m_logbrush, sizeof(LOGBRUSH));
		ar>>wTemp; m_round=(bool)wTemp;
		ar>>wTemp; m_dstyle=(CDashLine::DashStyle)wTemp;
	}
}

void CDrawObj::Remove()
{
	delete this;
}

void CDrawObj::Draw(CDC*)
{
}

void CDrawObj::DrawTracker(CDC* pDC, TrackerState state)
{
	ASSERT_VALID(this);

	switch (state)
	{
	case normal:
		break;

	case selected:
	case active:
		{
			int nHandleCount = GetHandleCount();
			for (int nHandle = 1; nHandle <= nHandleCount; nHandle += 1)
			{
				CPoint handle = GetHandle(nHandle);
				pDC->PatBlt(handle.x - 3, handle.y - 3, 7, 7, DSTINVERT);
			}
		}
		break;
	}
}

// position is in logical
void CDrawObj::MoveTo(const CRect& position, CJTDrawView* pView)
{
	ASSERT_VALID(this);

	if (position == m_position)
		return;

	if (pView == NULL)
	{
		Invalidate();
		m_position = position;
		Invalidate();
	}
	else
	{
		pView->InvalObj(this);
		m_position = position;
		pView->InvalObj(this);
	}
	m_pDocument->SetModifiedFlag();
}

// Note: if bSelected, hit-codes start at one for the top-left
// and increment clockwise, 0 means no hit.
// If !bSelected, 0 = no hit, 1 = hit (anywhere)

// point is in logical coordinates
int CDrawObj::HitTest(CPoint point, CJTDrawView* pView, BOOL bSelected)
{
	ASSERT_VALID(this);
	ASSERT(pView != NULL);

	if (bSelected)
	{
		int nHandleCount = GetHandleCount();
		for (int nHandle = 1; nHandle <= nHandleCount; nHandle += 1)
		{
			// GetHandleRect returns in logical coords
			CRect rc = GetHandleRect(nHandle,pView);
			if (point.x >= rc.left && point.x < rc.right &&
				point.y <= rc.top && point.y > rc.bottom)
				return nHandle;
		}
	}
	else
	{
		if (point.x >= m_position.left && point.x < m_position.right &&
			point.y <= m_position.top && point.y > m_position.bottom)
			return 1;
	}
	return 0;
}

// rect must be in logical coordinates
BOOL CDrawObj::Intersects(const CRect& rect)
{
	ASSERT_VALID(this);

	CRect fixed = m_position;
	fixed.NormalizeRect();
	CRect rectT = rect;
	rectT.NormalizeRect();
	return !(rectT & fixed).IsRectEmpty();
}

BOOL CDrawObj::InRect(const CRect& rect)
{
	ASSERT_VALID(this);

	CRect fixed = m_position;
	fixed.NormalizeRect();
	CRect rectT = rect;
	rectT.NormalizeRect();
    CRect unionRect;
	unionRect.UnionRect(fixed,rectT);
	return (unionRect==rectT);
}

int CDrawObj::GetHandleCount()
{
	ASSERT_VALID(this);
	return 8;
}

// returns logical coords of center of handle
CPoint CDrawObj::GetHandle(int nHandle)
{
	ASSERT_VALID(this);
	int x, y, xCenter, yCenter;

	// this gets the center regardless of left/right and top/bottom ordering
	xCenter = m_position.left + m_position.Width() / 2;
	yCenter = m_position.top + m_position.Height() / 2;

	switch (nHandle)
	{
	default:
		ASSERT(FALSE);

	case 1:
		x = m_position.left;
		y = m_position.top;
		break;

	case 2:
		x = xCenter;
		y = m_position.top;
		break;

	case 3:
		x = m_position.right;
		y = m_position.top;
		break;

	case 4:
		x = m_position.right;
		y = yCenter;
		break;

	case 5:
		x = m_position.right;
		y = m_position.bottom;
		break;

	case 6:
		x = xCenter;
		y = m_position.bottom;
		break;

	case 7:
		x = m_position.left;
		y = m_position.bottom;
		break;

	case 8:
		x = m_position.left;
		y = yCenter;
		break;
	}

	return CPoint(x, y);
}

// return rectange of handle in logical coords
CRect CDrawObj::GetHandleRect(int nHandleID, CJTDrawView* pView)
{
	ASSERT_VALID(this);
	ASSERT(pView != NULL);

	CRect rect;
	// get the center of the handle in logical coords
	CPoint point = GetHandle(nHandleID);
	// convert to client/device coords
	pView->DocToClient(point);
	// return CRect of handle in device coords
	rect.SetRect(point.x-3, point.y-3, point.x+3, point.y+3);
	pView->ClientToDoc(rect);

	return rect;
}

HCURSOR CDrawObj::GetHandleCursor(int nHandle)
{
	ASSERT_VALID(this);

	LPCTSTR id;
	switch (nHandle)
	{
	default:
		ASSERT(FALSE);

	case 1:
	case 5:
		id = IDC_SIZENWSE;
		break;

	case 2:
	case 6:
		id = IDC_SIZENS;
		break;

	case 3:
	case 7:
		id = IDC_SIZENESW;
		break;

	case 4:
	case 8:
		id = IDC_SIZEWE;
		break;
	}

	return AfxGetApp()->LoadStandardCursor(id);
}

// point must be in logical
void CDrawObj::MoveHandleTo(int nHandle, CPoint point, CJTDrawView* pView)
{
	ASSERT_VALID(this);

	CRect position = m_position;
	switch (nHandle)
	{
	default:
		ASSERT(FALSE);

	case 1:
		position.left = point.x;
		position.top = point.y;
		break;

	case 2:
		position.top = point.y;
		break;

	case 3:
		position.right = point.x;
		position.top = point.y;
		break;

	case 4:
		position.right = point.x;
		break;

	case 5:
		position.right = point.x;
		position.bottom = point.y;
		break;

	case 6:
		position.bottom = point.y;
		break;

	case 7:
		position.left = point.x;
		position.bottom = point.y;
		break;

	case 8:
		position.left = point.x;
		break;
	}

	MoveTo(position, pView);
}

void CDrawObj::Invalidate()
{
	ASSERT_VALID(this);
	m_pDocument->UpdateAllViews(NULL, HINT_UPDATE_DRAWOBJ, this);
}

CDrawObj* CDrawObj::Clone(CJTDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawObj* pClone = new CDrawObj(m_position);
	pClone->m_bPen = m_bPen;
	pClone->m_logpen = m_logpen;
	pClone->m_bBrush = m_bBrush;
	pClone->m_logbrush = m_logbrush;
	ASSERT_VALID(pClone);

	if (pDoc != NULL)
		pDoc->Add(pClone);

	return pClone;
}

void CDrawObj::OnEditProperties()
{
    
	
	ASSERT_VALID(this);

    CDrawProperty sheet ( _T("Properties") );
	CLineProperty linePage;
	linePage.m_bNoFill = !m_bBrush;
	linePage.m_penSize = m_bPen ? m_logpen.lopnWidth.x : 0;
	linePage.m_FillColor=m_logbrush.lbColor;
	linePage.m_LineColor=m_logpen.lopnColor;
    linePage.m_lineStyle=m_dstyle;
	//if it is a arrow;
	CDrawRect* pRect;
	if(this->IsKindOf( RUNTIME_CLASS( CDrawRect ) ) ){
		//case this to CDrawRect
		pRect= dynamic_cast<CDrawRect* >(this) ;
		if(pRect->m_nShape==CDrawRect::arrow)
			//initialize the parameter of arrow here;
			linePage.m_nArrowStart=pRect->m_lArrowSize;
			linePage.m_nArrowEnd=pRect->m_rArrowSize;
			linePage.m_arrowEnd=pRect->m_rArrow;  
			linePage.m_arrowStart=pRect->m_lArrow;
	}    
		
	CTextProperty textPage;
	CDrawText* pText;
	if(this->IsKindOf( RUNTIME_CLASS( CDrawText ) ) ){
		//case this to CDrawRect
		pText= dynamic_cast<CDrawText* >(this) ;
			textPage.m_text=pText->m_text;
	}    


	
	sheet.AddPage( &linePage );
	sheet.AddPage( &textPage );

	if (sheet.DoModal() != IDOK)
		return;

	m_bBrush = !linePage.m_bNoFill;
	m_bPen = linePage.m_penSize > 0;
	if (m_bPen)
	{
	m_logpen.lopnWidth.x = linePage.m_penSize;
		m_logpen.lopnWidth.y = linePage.m_penSize;
	}
	m_logbrush.lbColor=linePage.m_FillColor;
	m_logpen.lopnColor=linePage.m_LineColor;
    m_dstyle=linePage.m_lineStyle;
	//if it is a arrow;
	if(this->IsKindOf( RUNTIME_CLASS( CDrawRect ) ) ){
		//case this is CDrawRect
		if(pRect->m_nShape==CDrawRect::arrow)
			//initialize the parameter of arrow here;
			pRect->m_lArrowSize=linePage.m_nArrowStart;
			pRect->m_rArrowSize=linePage.m_nArrowEnd;
			pRect->m_rArrow=linePage.m_arrowEnd;  
			pRect->m_lArrow=linePage.m_arrowStart;
	}  
	if(this->IsKindOf( RUNTIME_CLASS( CDrawText ) ) ){
		//case this to CDrawRect
		pText= dynamic_cast<CDrawText* >(this) ;
			pText->m_text=textPage.m_text;
	}    

	Invalidate();
	m_pDocument->SetModifiedFlag();

}

void CDrawObj::OnOpen(CJTDrawView* /*pView*/ )
{
	OnEditProperties();
}

void CDrawObj::SetLineColor(COLORREF color)
{
	ASSERT_VALID(this);

	m_logpen.lopnColor = color;
	Invalidate();
	m_pDocument->SetModifiedFlag();
}

void CDrawObj::SetTextColor(COLORREF color)
{
}

void CDrawObj::SetFillColor(COLORREF color)
{
	ASSERT_VALID(this);

	m_logbrush.lbColor = color;
	Invalidate();
	m_pDocument->SetModifiedFlag();
}

#ifdef _DEBUG
void CDrawObj::AssertValid()
{
	ASSERT(m_position.left <= m_position.right);
	ASSERT(m_position.bottom <= m_position.top);
}
#endif

////////////////////////////////////////////////////////////////////////////
// CDrawRect

IMPLEMENT_SERIAL(CDrawRect, CDrawObj, 0)

CDrawRect::CDrawRect()
{
}

CDrawRect::CDrawRect(const CRect& position)
	: CDrawObj(position)
{
	ASSERT_VALID(this);

	m_nShape = rectangle;
	m_roundness.x = 16;
	m_roundness.y = 16;
	m_lArrowSize=m_rArrowSize=8; //default
	m_lArrow=none;
	m_rArrow=pureArrow;
}

void CDrawRect::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CDrawObj::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << (WORD) m_nShape;
		ar << m_roundness;
		ar <<m_lArrowSize;
		ar <<m_rArrowSize;
		ar << (WORD) m_lArrow;
		ar << (WORD) m_rArrow;
	}
	else
	{
		WORD wTemp;
		ar >> wTemp; m_nShape = (Shape)wTemp;
		ar >> m_roundness;
		ar >> m_lArrowSize;
		ar >> m_rArrowSize;
		ar >> wTemp; m_lArrow = (ArrowType)wTemp;
		ar >> wTemp; m_rArrow = (ArrowType)wTemp;
	}
}

void CDrawRect::Draw(CDC* pDC)
{
	ASSERT_VALID(this);

	CBrush brush;
	if (!brush.CreateBrushIndirect(&m_logbrush))
		return;

	
	LOGBRUSH lbrush;
	lbrush.lbStyle = BS_SOLID;
	lbrush.lbColor = m_logpen.lopnColor;
	CPen pen(PS_GEOMETRIC | PS_SOLID | 
				(m_round ? 
				(PS_JOIN_ROUND | PS_ENDCAP_ROUND):
				(PS_JOIN_MITER | PS_ENDCAP_FLAT)	)		, m_logpen.lopnWidth.x, &lbrush);
	
				CBrush* pOldBrush;
	CPen* pOldPen;

	if (m_bBrush)
		pOldBrush = pDC->SelectObject(&brush);
	else
		pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);

	if (m_bPen)
		pOldPen = pDC->SelectObject(&pen);
	else
		pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);

	CRect rect = m_position;

	unsigned Type1[8];
	int width=3+m_logpen.lopnWidth.x;
	unsigned c1 = CDashLine::GetPattern(Type1, m_round, width, m_dstyle);
	CDashLine dashline(*pDC, Type1, c1); //construct the dashline object
    CRgn rgn; 
	int x;
	switch (m_nShape)
	{
	case rectangle:
		//pDC->Rectangle(rect);
        //rgn.CreateRectRgn(rect.left,rect.top,rect.right,rect.bottom);
		pDC->FillRect(rect,&brush);
		x= (m_round)? 0:m_logpen.lopnWidth.x/2;
		dashline.Rectangle(rect, x);
		
		break;

	case roundRectangle:
		pDC->RoundRect(rect, m_roundness);
		break;

	case ellipse:
		pDC->Ellipse(rect);
		break;

	case line:
		if (rect.top > rect.bottom)
		{
			rect.top -= m_logpen.lopnWidth.y / 2;
			rect.bottom += (m_logpen.lopnWidth.y + 1) / 2;
		}
		else
		{
			rect.top += (m_logpen.lopnWidth.y + 1) / 2;
			rect.bottom -= m_logpen.lopnWidth.y / 2;
		}

		if (rect.left > rect.right)
		{
			rect.left -= m_logpen.lopnWidth.x / 2;
			rect.right += (m_logpen.lopnWidth.x + 1) / 2;
		}
		else
		{
			rect.left += (m_logpen.lopnWidth.x + 1) / 2;
			rect.right -= m_logpen.lopnWidth.x / 2;
		}

// draw dashline under windows NT
/*		m_logpen.lopnStyle=PS_DASHDOT;

		pDC->MoveTo(rect.TopLeft());
		pDC->LineTo(rect.BottomRight());
*/
//using dashline to draw it. for win95/98
	    dashline.MoveTo(rect.TopLeft());
		dashline.LineTo(rect.BottomRight());
		break;
	case arrow:
//draw the line first;
        CRect saveRect=rect;
		int arrowlength=2*m_lArrowSize;  //arrowlength is the start arrow length
		int arrowlength1=2*m_rArrowSize; //arrowlength1 is the end arrow length
		int dx=0,dy=0,dx1=0,dy1=0;
		long r2=(rect.top-rect.bottom)*(rect.top-rect.bottom)+(rect.left-rect.right)*(rect.left-rect.right);
		int l=(int)sqrt((double)r2);
        if (l)
		{
			dy=(arrowlength*(rect.top-rect.bottom))/l;
			dx=(arrowlength*(rect.left-rect.right))/l;
			dy1=(arrowlength1*(rect.top-rect.bottom))/l;
			dx1=(arrowlength1*(rect.left-rect.right))/l;
		}
		if (rect.top > rect.bottom)
		{
			rect.top -= m_logpen.lopnWidth.y / 2;
			if (m_rArrow!=none )rect.top -= abs(dy1);
			rect.bottom += (m_logpen.lopnWidth.y + 1) / 2;
			if (m_lArrow!=none)rect.bottom += abs(dy);
		}
		else
		{
			rect.top += (m_logpen.lopnWidth.y + 1) / 2;
			if (m_rArrow!=none )rect.top += abs(dy1);
			rect.bottom -= m_logpen.lopnWidth.y / 2;
			if (m_lArrow!=none)rect.bottom -=abs(dy);
		}

		if (rect.left > rect.right)
		{
			rect.left -= m_logpen.lopnWidth.x / 2;
			if (m_rArrow!=none ) rect.left -=abs(dx1);
			rect.right += (m_logpen.lopnWidth.x + 1) / 2;
			if (m_lArrow!=none)rect.right +=abs(dx);
		}
		else
		{
			rect.left += (m_logpen.lopnWidth.x + 1) / 2;
			if (m_rArrow!=none) rect.left +=abs(dx1);
			rect.right -= m_logpen.lopnWidth.x / 2;
			if (m_lArrow!=none) rect.right -=abs(dx);
		}
	    dashline.MoveTo(rect.TopLeft());
		dashline.LineTo(rect.BottomRight());
		//draw left arrow
		DrawArrow(m_lArrow,saveRect.BottomRight(), rect.BottomRight(), 
			m_lArrowSize, pDC,dx,dy,arrowlength, &brush);
		//draw right arrow
        DrawArrow(m_rArrow,saveRect.TopLeft(), rect.TopLeft(), 
			m_rArrowSize, pDC,dx1,dy1,arrowlength1,&brush);
		break;
	}

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}


int CDrawRect::GetHandleCount()
{
	ASSERT_VALID(this);

	return (m_nShape == line ||m_nShape==arrow)? 2 :
		CDrawObj::GetHandleCount() + (m_nShape == roundRectangle);
}

// returns center of handle in logical coordinates
CPoint CDrawRect::GetHandle(int nHandle)
{
	ASSERT_VALID(this);

	if ((m_nShape == line || m_nShape==arrow) && nHandle == 2)
		nHandle = 5;
	else if (m_nShape == roundRectangle && nHandle == 9)
	{
		CRect rect = m_position;
		rect.NormalizeRect();
		CPoint point = rect.BottomRight();
		point.x -= m_roundness.x / 2;
		point.y -= m_roundness.y / 2;
		return point;
	}

	return CDrawObj::GetHandle(nHandle);
}

HCURSOR CDrawRect::GetHandleCursor(int nHandle)
{
	ASSERT_VALID(this);

	if ((m_nShape == line||m_nShape==arrow) && nHandle == 2)
		nHandle = 5;
	else if (m_nShape == roundRectangle && nHandle == 9)
		return AfxGetApp()->LoadStandardCursor(IDC_SIZEALL);
	return CDrawObj::GetHandleCursor(nHandle);
}

// point is in logical coordinates
void CDrawRect::MoveHandleTo(int nHandle, CPoint point, CJTDrawView* pView)
{
	ASSERT_VALID(this);

	if ((m_nShape == line || m_nShape==arrow) && nHandle == 2)
		nHandle = 5;
	else if (m_nShape == roundRectangle && nHandle == 9)
	{
		CRect rect = m_position;
		rect.NormalizeRect();
		if (point.x > rect.right - 1)
			point.x = rect.right - 1;
		else if (point.x < rect.left + rect.Width() / 2)
			point.x = rect.left + rect.Width() / 2;
		if (point.y > rect.bottom - 1)
			point.y = rect.bottom - 1;
		else if (point.y < rect.top + rect.Height() / 2)
			point.y = rect.top + rect.Height() / 2;
		m_roundness.x = 2 * (rect.right - point.x);
		m_roundness.y = 2 * (rect.bottom - point.y);
		m_pDocument->SetModifiedFlag();
		if (pView == NULL)
			Invalidate();
		else
			pView->InvalObj(this);
		return;
	}

	CDrawObj::MoveHandleTo(nHandle, point, pView);
}

// rect must be in logical coordinates
BOOL CDrawRect::Intersects(const CRect& rect)
{
	ASSERT_VALID(this);
    CRect rectT = rect;
	rectT.NormalizeRect();

	CRect fixed = m_position;
	fixed.NormalizeRect();

	//sometimes vertical line 
	if ((rectT & fixed).IsRectEmpty())
		if(m_nShape!=line)
			return FALSE;

	CRgn rgn;
	switch (m_nShape)
	{
	case rectangle:
		return TRUE;

	case roundRectangle:
		rgn.CreateRoundRectRgn(fixed.left, fixed.top, fixed.right, fixed.bottom,
			m_roundness.x, m_roundness.y);
		break;

	case ellipse:
		rgn.CreateEllipticRgnIndirect(fixed);
		break;

	case line:
	case arrow:
		{
			int w = (m_logpen.lopnWidth.x + 10) / 2;
			if (m_nShape==arrow) w+=m_rArrowSize;
			//int y = (m_logpen.lopnWidth.y + 5) / 2;   document says lopenWidth.y is not used;
			double l=sqrt((double)(fixed.top-fixed.bottom)*(fixed.top-fixed.bottom)+(fixed.left-fixed.right)*(fixed.left-fixed.right));
			double dx=w*(fixed.top-fixed.bottom)/l;
			double dy=w*(fixed.left-fixed.right)/l;
			int x=-(int)dx;
			int y=-(int)dy;
			
			int change=1;
			if (fixed.bottom!=m_position.bottom) change*=-1;
			if (fixed.left!=m_position.left) change*=-1;
			
			POINT points[4];
			points[0].x = m_position.left;
			points[0].y = m_position.top;
			points[1].x = m_position.left;
			points[1].y = m_position.top;
			points[2].x = m_position.right;
			points[2].y = m_position.bottom;
			points[3].x = m_position.right;
			points[3].y = m_position.bottom;

			if ((fixed.left-fixed.right)<=0)
			{
				points[0].x -= x;
				points[1].x += x;
				points[2].x += x;
				points[3].x -= x;
			}
			else
			{
				points[0].x += x;
				points[1].x -= x;
				points[2].x -= x;
				points[3].x += x;
			}

			if ((fixed.top - fixed.bottom)*change>=0)
			{
				points[0].y -= y;
				points[1].y += y;
				points[2].y += y;
				points[3].y -= y;
			}
			else
			{
				points[0].y += y;
				points[1].y -= y;
				points[2].y -= y;
				points[3].y += y;
			}
			rgn.CreatePolygonRgn(points, 4, WINDING);
		}
		break;
	}
	//return rgn.RectInRegion(fixed); error!
	return rgn.RectInRegion(rectT);
}

CDrawObj* CDrawRect::Clone(CJTDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawRect* pClone = new CDrawRect(m_position);
	pClone->m_bPen = m_bPen;
	pClone->m_logpen = m_logpen;
	pClone->m_bBrush = m_bBrush;
	pClone->m_logbrush = m_logbrush;
	pClone->m_nShape = m_nShape;
	pClone->m_roundness = m_roundness;  //for roundrect
	pClone->m_lArrowSize = m_lArrowSize;  //for arrow
	pClone->m_rArrowSize = m_rArrowSize;  //for arrow
	pClone->m_rArrow = m_rArrow;		//for arrow
	pClone->m_lArrow = m_lArrow;		//for arrow
	ASSERT_VALID(pClone);
	if (pDoc != NULL)
		pDoc->Add(pClone);

	ASSERT_VALID(pClone);
	return pClone;
}

void CDrawRect::OnEditProperties()
{
	ASSERT_VALID(this);

    CDrawProperty sheet ( _T("Properties") );
	CLineProperty linePage;
	linePage.m_bNoFill = !m_bBrush;
	linePage.m_penSize = m_bPen ? m_logpen.lopnWidth.x : 0;
	linePage.m_FillColor=m_logbrush.lbColor;
	linePage.m_LineColor=m_logpen.lopnColor;
    linePage.m_lineStyle=m_dstyle;
	linePage.m_round=m_round;
	//if it is a arrow;
	linePage.m_isArrow=false;
    if(m_nShape==CDrawRect::arrow){
		//initialize the parameter of arrow here;
		linePage.m_isArrow=true;
		linePage.m_nArrowStart=m_lArrowSize;
		linePage.m_nArrowEnd=m_rArrowSize;
		linePage.m_arrowEnd=m_rArrow;  
		linePage.m_arrowStart=m_lArrow;
	}    
		
	sheet.AddPage( &linePage );

	if (sheet.DoModal() != IDOK)
		return;

	m_bBrush = !linePage.m_bNoFill;
	m_bPen = linePage.m_penSize > 0;
	if (m_bPen)
	{
	m_logpen.lopnWidth.x = linePage.m_penSize;
		m_logpen.lopnWidth.y = linePage.m_penSize;
	}
	m_logbrush.lbColor=linePage.m_FillColor;
	m_logpen.lopnColor=linePage.m_LineColor;
    m_dstyle=linePage.m_lineStyle;
	m_round=linePage.m_round;
	//if it is a arrow;
	if(m_nShape==CDrawRect::arrow){
	//initialize the parameter of arrow here;
		m_lArrowSize=linePage.m_nArrowStart;
		m_rArrowSize=linePage.m_nArrowEnd;
		m_rArrow=linePage.m_arrowEnd;  
		m_lArrow=linePage.m_arrowStart;
	}  
	
	Invalidate();
	m_pDocument->SetModifiedFlag();

}

void CDrawRect::DrawArrow(ArrowType arrowType, 
						  POINT  p1, POINT p2, int arrowSize ,CDC *pDC,
						  int dx, int dy, int arrowlength, CBrush* pbrush)
{
		POINT points[4];
	    CRgn rgn;
	    switch (arrowType){
		case none:
			break;
		case pureArrow:
            //actually a triagle
			points[0]=p1;
			points[1]=points[2]=p2;
			points[1].x+=arrowSize*dy/arrowlength;
			points[1].y-=arrowSize*dx/arrowlength;
			points[2].x-=arrowSize*dy/arrowlength;
			points[2].y+=arrowSize*dx/arrowlength;
			rgn.CreatePolygonRgn(points, 3, ALTERNATE);
			if(m_bBrush)
				pDC->FillRgn(&rgn,pbrush);
			pDC->Polygon(points,3);
			break;
		case openArrow:
			points[0]=p1;
			points[1]=points[2]=points[3]=p2;
			points[1].x+=arrowSize*dy/arrowlength;
			points[1].y-=arrowSize*dx/arrowlength;
			points[2].x-=arrowSize*dy/arrowlength;
			points[2].y+=arrowSize*dx/arrowlength;
            pDC->MoveTo(points[1]);
			pDC->LineTo(points[0]);
			pDC->LineTo(points[2]);
            pDC->MoveTo(points[0]);
			pDC->LineTo(points[3]);
			break;
		case stealthArrow:
			points[0]=p1;
			points[1]=points[2]=points[3]=p2;
			points[1].x-=(p1.x-p2.x);
			points[1].y-=(p1.y-p2.y);
			points[3]=points[1];
			points[1].x+=arrowSize*dy/arrowlength;
			points[1].y-=arrowSize*dx/arrowlength;
			points[3].x-=arrowSize*dy/arrowlength;
			points[3].y+=arrowSize*dx/arrowlength;
			rgn.CreatePolygonRgn(points, 4, ALTERNATE);
			if(m_bBrush)
	            pDC->FillRgn(&rgn,pbrush);
			pDC->Polygon(points,4);
			break;
		case diamondArrow:
			points[0]=p1;
			points[2]=p2;
			points[1].x=points[3].x=(p1.x+p2.x)/2;
			points[1].y=points[3].y=(p1.y+p2.y)/2;
			//points[2].x-=(p1.x-p2.x);
			//points[2].y-=(p1.y-p2.y);
			points[1].x+=(long)(.5*arrowSize*dy/arrowlength);
			points[1].y-=(long)(.5*arrowSize*dx/arrowlength);
			points[3].x-=(long)(.5*arrowSize*dy/arrowlength);
			points[3].y+=(long)(.5*arrowSize*dx/arrowlength);
			rgn.CreatePolygonRgn(points, 4, ALTERNATE);
			if(m_bBrush)
	            pDC->FillRgn(&rgn,pbrush);
			pDC->Polygon(points,4);
			break;
		case ovalArrow:
			POINT p;
			p.x=(p1.x+p2.x)/2;
			p.y=(p1.y+p2.y)/2;
            long r=(long)sqrt((p1.x-p2.x)*(p1.x-p2.x)+(p1.y-p2.y)*(p1.y-p2.y))/2+1;
			CRect circleRect;
			circleRect.left=p.x-r;
			circleRect.right=p.x+r;
			circleRect.bottom=p.y+r;
			circleRect.top=p.y-r;
			pDC->Ellipse(&circleRect);
			break;
		}
}
////////////////////////////////////////////////////////////////////////////
// CDrawPoly

IMPLEMENT_SERIAL(CDrawPoly, CDrawObj, 0)

CDrawPoly::CDrawPoly()
{
	m_points = NULL;
	m_nPoints = 0;
	m_nAllocPoints = 0;
}

CDrawPoly::CDrawPoly(const CRect& position)
	: CDrawObj(position)
{
	m_points = NULL;
	m_nPoints = 0;
	m_nAllocPoints = 0;
	m_bPen = TRUE;
	m_bBrush = FALSE;
}

CDrawPoly::~CDrawPoly()
{
	if (m_points != NULL)
		delete[] m_points;
}

void CDrawPoly::Serialize( CArchive& ar )
{
	int i;
	CDrawObj::Serialize( ar );
	if( ar.IsStoring() )
	{
		ar << (WORD) m_nPoints;
		ar << (WORD) m_nAllocPoints;
		for (i = 0;i< m_nPoints; i++)
			ar << m_points[i];
	}
	else
	{
		WORD wTemp;
		ar >> wTemp; m_nPoints = wTemp;
		ar >> wTemp; m_nAllocPoints = wTemp;
		m_points = new CPoint[m_nAllocPoints];
		for (i = 0;i < m_nPoints; i++)
			ar >> m_points[i];
	}
}

void CDrawPoly::Draw(CDC* pDC)
{
	ASSERT_VALID(this);

	CBrush brush;
	if (!brush.CreateBrushIndirect(&m_logbrush))
		return;
	CPen pen;
	if (!pen.CreatePenIndirect(&m_logpen))
		return;

	CBrush* pOldBrush;
	CPen* pOldPen;

	if (m_bBrush)
		pOldBrush = pDC->SelectObject(&brush);
	else
		pOldBrush = (CBrush*)pDC->SelectStockObject(NULL_BRUSH);

	if (m_bPen)
		pOldPen = pDC->SelectObject(&pen);
	else
		pOldPen = (CPen*)pDC->SelectStockObject(NULL_PEN);

	pDC->Polygon(m_points, m_nPoints);

	pDC->SelectObject(pOldBrush);
	pDC->SelectObject(pOldPen);
}

// position must be in logical coordinates
void CDrawPoly::MoveTo(const CRect& position, CJTDrawView* pView)
{
	ASSERT_VALID(this);
	if (position == m_position)
		return;

	if (pView == NULL)
		Invalidate();
	else
		pView->InvalObj(this);

	for (int i = 0; i < m_nPoints; i += 1)
	{
		m_points[i].x += position.left - m_position.left;
		m_points[i].y += position.top - m_position.top;
	}

	m_position = position;

	if (pView == NULL)
		Invalidate();
	else
		pView->InvalObj(this);
	m_pDocument->SetModifiedFlag();
}

int CDrawPoly::GetHandleCount()
{
	return m_nPoints;
}

CPoint CDrawPoly::GetHandle(int nHandle)
{
	ASSERT_VALID(this);

	ASSERT(nHandle >= 1 && nHandle <= m_nPoints);
	return m_points[nHandle - 1];
}

HCURSOR CDrawPoly::GetHandleCursor(int )
{
	return AfxGetApp()->LoadStandardCursor(IDC_ARROW);
}

// point is in logical coordinates
void CDrawPoly::MoveHandleTo(int nHandle, CPoint point, CJTDrawView* pView)
{
	ASSERT_VALID(this);
	ASSERT(nHandle >= 1 && nHandle <= m_nPoints);
	if (m_points[nHandle - 1] == point)
		return;

	m_points[nHandle - 1] = point;
	RecalcBounds(pView);

	if (pView == NULL)
		Invalidate();
	else
		pView->InvalObj(this);
	m_pDocument->SetModifiedFlag();
}

// rect must be in logical coordinates
BOOL CDrawPoly::Intersects(const CRect& rect)
{
	ASSERT_VALID(this);
	CRgn rgn;
	rgn.CreatePolygonRgn(m_points, m_nPoints, ALTERNATE);
	return rgn.RectInRegion(rect);
}

CDrawObj* CDrawPoly::Clone(CJTDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawPoly* pClone = new CDrawPoly(m_position);
	pClone->m_bPen = m_bPen;
	pClone->m_logpen = m_logpen;
	pClone->m_bBrush = m_bBrush;
	pClone->m_logbrush = m_logbrush;
	pClone->m_points = new CPoint[m_nAllocPoints];
	memcpy(pClone->m_points, m_points, sizeof(CPoint) * m_nPoints);
	pClone->m_nAllocPoints = m_nAllocPoints;
	pClone->m_nPoints = m_nPoints;
	ASSERT_VALID(pClone);

	if (pDoc != NULL)
		pDoc->Add(pClone);

	ASSERT_VALID(pClone);
	return pClone;
}

// point is in logical coordinates
void CDrawPoly::AddPoint(const CPoint& point, CJTDrawView* pView)
{
	ASSERT_VALID(this);
	if (m_nPoints == m_nAllocPoints)
	{
		CPoint* newPoints = new CPoint[m_nAllocPoints + 10];
		if (m_points != NULL)
		{
			memcpy(newPoints, m_points, sizeof(CPoint) * m_nAllocPoints);
			delete[] m_points;
		}
		m_points = newPoints;
		m_nAllocPoints += 10;
	}

	if (m_nPoints == 0 || m_points[m_nPoints - 1] != point)
	{
		m_points[m_nPoints++] = point;
		if (!RecalcBounds(pView))
		{
			if (pView == NULL)
				Invalidate();
			else
				pView->InvalObj(this);
		}
		m_pDocument->SetModifiedFlag();
	}
}

BOOL CDrawPoly::RecalcBounds(CJTDrawView* pView)
{
	ASSERT_VALID(this);

	if (m_nPoints == 0)
		return FALSE;

	CRect bounds(m_points[0], CSize(0, 0));
	for (int i = 1; i < m_nPoints; ++i)
	{
		if (m_points[i].x < bounds.left)
			bounds.left = m_points[i].x;
		if (m_points[i].x > bounds.right)
			bounds.right = m_points[i].x;
		if (m_points[i].y < bounds.top)
			bounds.top = m_points[i].y;
		if (m_points[i].y > bounds.bottom)
			bounds.bottom = m_points[i].y;
	}

	if (bounds == m_position)
		return FALSE;

	if (pView == NULL)
		Invalidate();
	else
		pView->InvalObj(this);

	m_position = bounds;

	if (pView == NULL)
		Invalidate();
	else
		pView->InvalObj(this);

	return TRUE;
}

void CDrawPoly::OnEditProperties()
{
	ASSERT_VALID(this);

    CDrawProperty sheet ( _T("Properties") );
	CLineProperty linePage;
	linePage.m_bNoFill = !m_bBrush;
	linePage.m_penSize = m_bPen ? m_logpen.lopnWidth.x : 0;
	linePage.m_FillColor=m_logbrush.lbColor;
	linePage.m_LineColor=m_logpen.lopnColor;
    linePage.m_lineStyle=m_dstyle;
		
	sheet.AddPage( &linePage );

	if (sheet.DoModal() != IDOK)
		return;

	m_bBrush = !linePage.m_bNoFill;
	m_bPen = linePage.m_penSize > 0;
	if (m_bPen)
	{
	m_logpen.lopnWidth.x = linePage.m_penSize;
		m_logpen.lopnWidth.y = linePage.m_penSize;
	}
	m_logbrush.lbColor=linePage.m_FillColor;
	m_logpen.lopnColor=linePage.m_LineColor;
    m_dstyle=linePage.m_lineStyle;
	
	Invalidate();
	m_pDocument->SetModifiedFlag();

}
////////////////////////////////////////////////////////////////////////////

IMPLEMENT_SERIAL(CDrawOleObj, CDrawObj, 0)

BOOL CDrawOleObj::c_bShowItems = TRUE;

CDrawOleObj::CDrawOleObj() : m_extent(0,0)
{
	m_pClientItem = NULL;
}

CDrawOleObj::CDrawOleObj(const CRect& position)
	: CDrawObj(position), m_extent(0, 0)
{
	m_pClientItem = NULL;
}

CDrawOleObj::~CDrawOleObj()
{
	if (m_pClientItem != NULL)
	{
		m_pClientItem->Release();
		m_pClientItem = NULL;
	}
}

void CDrawOleObj::Remove()
{
	if (m_pClientItem != NULL)
	{
		m_pClientItem->Delete();
		m_pClientItem = NULL;
	}
	CDrawObj::Remove();
}

void CDrawOleObj::Serialize( CArchive& ar )
{
	ASSERT_VALID(this);

	CDrawObj::Serialize(ar);

	if (ar.IsStoring())
	{
		ar << m_extent;
		ar << m_pClientItem;
	}
	else
	{
		ar >> m_extent;
		ar >> m_pClientItem;
		m_pClientItem->m_pDrawObj = this;
	}
}

CDrawObj* CDrawOleObj::Clone(CJTDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	AfxGetApp()->BeginWaitCursor();

	CDrawOleObj* pClone = NULL;
	CJTDrawCntrItem* pItem = NULL;
	TRY
	{
		// perform a "deep copy" -- need to copy CDrawOleObj and the CJTDrawCntrItem
		//  that it points to.
		CDrawOleObj* pClone = new CDrawOleObj(m_position);
		CJTDrawCntrItem* pItem = new CJTDrawCntrItem(m_pDocument, pClone);
		if (!pItem->CreateCloneFrom(m_pClientItem))
			AfxThrowMemoryException();

		pClone->m_pClientItem = pItem;
		pClone->m_bPen = m_bPen;
		pClone->m_logpen = m_logpen;
		pClone->m_bBrush = m_bBrush;
		pClone->m_logbrush = m_logbrush;
		ASSERT_VALID(pClone);

		if (pDoc != NULL)
			pDoc->Add(pClone);
	}
	CATCH_ALL(e)
	{
		pItem->Delete();
		pClone->m_pClientItem = NULL;
		pClone->Remove();
		AfxGetApp()->EndWaitCursor();

		THROW_LAST();
	}
	END_CATCH_ALL

	AfxGetApp()->EndWaitCursor();
	return pClone;
}

void CDrawOleObj::Draw(CDC* pDC)
{
	ASSERT_VALID(this);

	CJTDrawCntrItem* pItem = m_pClientItem;
	if (pItem != NULL)
	{
		// draw the OLE item itself
		pItem->Draw(pDC, m_position);

		// don't draw tracker in print preview or on printer
		if (!pDC->IsPrinting())
		{
			// use a CRectTracker to draw the standard effects
			CRectTracker tracker;
			tracker.m_rect = m_position;
			pDC->LPtoDP(tracker.m_rect);

			if (c_bShowItems)
			{
				// put correct border depending on item type
				if (pItem->GetType() == OT_LINK)
					tracker.m_nStyle |= CRectTracker::dottedLine;
				else
					tracker.m_nStyle |= CRectTracker::solidLine;
			}

			// put hatching over the item if it is currently open
			if (pItem->GetItemState() == COleClientItem::openState ||
				pItem->GetItemState() == COleClientItem::activeUIState)
			{
				tracker.m_nStyle |= CRectTracker::hatchInside;
			}
			tracker.Draw(pDC);
		}
	}
}

void CDrawOleObj::OnOpen(CJTDrawView* pView)
{
	AfxGetApp()->BeginWaitCursor();
	m_pClientItem->DoVerb(
		GetKeyState(VK_CONTROL) < 0 ? OLEIVERB_OPEN : OLEIVERB_PRIMARY,
		pView);
	AfxGetApp()->EndWaitCursor();
}

void CDrawOleObj::OnEditProperties()
{
	// using COlePropertiesDialog directly means no scaling
	COlePropertiesDialog dlg(m_pClientItem, 100, 100, NULL);

	dlg.DoModal();
}

// position is in logical
void CDrawOleObj::MoveTo(const CRect& position, CJTDrawView* pView)
{
	ASSERT_VALID(this);

	if (position == m_position)
		return;

	// call base class to update position
	CDrawObj::MoveTo(position, pView);

	// update position of in-place editing session on position change
	if (m_pClientItem->IsInPlaceActive())
		m_pClientItem->SetItemRects();
}

////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
// CDrawText

IMPLEMENT_SERIAL(CDrawText, CDrawObj, 0)

CDrawText::CDrawText()
{

}

CDrawText::CDrawText(const CRect& position)
	: CDrawRect(position)
{
	ASSERT_VALID(this);
	m_text="Text";
	m_bBorder=TRUE;
    m_nShape=rectangle;
	strcpy(m_logfont.lfFaceName, "Times");
	m_logfont.lfWeight=FW_REGULAR;
	m_logfont.lfWidth=0;
	m_logfont.lfHeight=0;
	m_logfont.lfItalic=FALSE;
	m_logfont.lfOrientation=0;
	m_logfont.lfEscapement=0;
	m_logfont.lfUnderline=FALSE;
	m_logfont.lfStrikeOut=FALSE;
	m_logfont.lfOutPrecision=OUT_OUTLINE_PRECIS;
	m_logfont.lfCharSet=ANSI_CHARSET;
	m_logfont.lfOutPrecision=OUT_DEFAULT_PRECIS; 
	m_logfont.lfPitchAndFamily=FF_MODERN|DEFAULT_PITCH;
	m_color=RGB(0,0,0);
}
void CDrawText::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CDrawRect::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << m_bBorder;
		ar << m_text;
		ar.Write(&m_logfont, sizeof(LOGFONT));
		ar <<m_color;
	}
	else
	{
		ar >> m_bBorder;
		ar >> m_text;
		ar.Read(&m_logfont, sizeof(LOGFONT));
		ar >>m_color;
	}
}

void CDrawText::Draw(CDC* pDC)
{
	ASSERT_VALID(this);

    if (m_bBorder>=0 && m_bBorder<3 ){ //if have border
		CDrawRect::Draw(pDC);
	}
    //drawthe text here;
	CFont font;
	if (!font.CreateFontIndirect(&m_logfont))
		return;
    CFont *pOldFont=(CFont *)pDC->SelectObject(&font);
	int x=(m_position.left < m_position.right)? m_position.left: m_position.right;
	int y=(m_position.top > m_position.bottom)? m_position.top:m_position.bottom;
    COLORREF oldColor2=pDC->SetTextColor(m_color);
	//calculate the x, y
	int dy=abs(m_position.top - m_position.bottom);
	int dx=abs(m_position.left -m_position.right);
	int lx= TextLen(pDC);//length of your string;
    int ly= MaxHeight(pDC);//hight of the string;

    switch(m_allignment){
	case 0: //middle
		x+=(dx-lx)/2;
		y-=(dy-ly)/2;
		break;
	case 1: //left
		x+=m_logpen.lopnWidth.x + PADDING;
		y-=(dy-ly)/2;
		//y-=m_logpen.lopnWidth.x + PADDING;
		break;
	case 2: //right
		x+=dx-lx-PADDING-m_logpen.lopnWidth.x;
		y-=(dy-ly)/2;
		//y-=(dy-ly+PADDING+m_logpen.lopnWidth.x);
		break;
	default: //middle
		x+=(dx-lx)/2;
		y-=(dy-ly)/2;
		break;
	}

	int oldMode;
	oldMode=pDC->SetBkMode(TRANSPARENT);
	pDC->ExtTextOut(x,y,ETO_OPAQUE,NULL,m_text,NULL);
//    pDC->SetBkColor(oldColor);
    pDC->SetTextColor(oldColor2);
    pDC->SelectObject(pOldFont);
	pDC->SetBkMode(oldMode);
}


int CDrawText::GetHandleCount()
{
	ASSERT_VALID(this);

	return CDrawObj::GetHandleCount() + (m_nShape == roundRectangle);
}

// returns center of handle in logical coordinates
CPoint CDrawText::GetHandle(int nHandle)
{
	ASSERT_VALID(this);
    if (m_nShape == roundRectangle && nHandle == 9)
		{
		CRect rect = m_position;
		rect.NormalizeRect();
		CPoint point = rect.BottomRight();
		point.x -= m_roundness.x / 2;
		point.y -= m_roundness.y / 2;
		return point;
		}
	return CDrawObj::GetHandle(nHandle);
}

HCURSOR CDrawText::GetHandleCursor(int nHandle)
{
	ASSERT_VALID(this);
	if (m_nShape == roundRectangle && nHandle == 9)
		return AfxGetApp()->LoadStandardCursor(IDC_SIZEALL);
	return CDrawObj::GetHandleCursor(nHandle);
}

// point is in logical coordinates
void CDrawText::MoveHandleTo(int nHandle, CPoint point, CJTDrawView* pView)
{
	ASSERT_VALID(this);
	if (m_nShape == roundRectangle && nHandle == 9)
	{
		CRect rect = m_position;
		rect.NormalizeRect();
		if (point.x > rect.right - 1)
			point.x = rect.right - 1;
		else if (point.x < rect.left + rect.Width() / 2)
			point.x = rect.left + rect.Width() / 2;
		if (point.y > rect.bottom - 1)
			point.y = rect.bottom - 1;
		else if (point.y < rect.top + rect.Height() / 2)
			point.y = rect.top + rect.Height() / 2;
		m_roundness.x = 2 * (rect.right - point.x);
		m_roundness.y = 2 * (rect.bottom - point.y);
		m_pDocument->SetModifiedFlag();
		if (pView == NULL)
			Invalidate();
		else{
			pView->InvalObj(this);
			Resize(pView);
		}
		return;
	}

    //nHandle dicide with point is moved
	CDrawObj::MoveHandleTo(nHandle, point, pView);    //make sure the movement is good
	Resize(pView);
    //must resize
}

// rect must be in logical coordinates
BOOL CDrawText::Intersects(const CRect& rect)
{
	ASSERT_VALID(this);
    CRect rectT = rect;
	rectT.NormalizeRect();

	CRect fixed = m_position;
	fixed.NormalizeRect();

	//sometimes vertical line 
	if ((rectT & fixed).IsRectEmpty())
			return FALSE;

	CRgn rgn;
	switch (m_nShape)
	{
	case rectangle:
		return TRUE;

	case roundRectangle:
		rgn.CreateRoundRectRgn(fixed.left, fixed.top, fixed.right, fixed.bottom,
			m_roundness.x, m_roundness.y);
		break;

	case ellipse:
		rgn.CreateEllipticRgnIndirect(fixed);
		break;

	case line:
	case arrow:
		return TRUE;
		break;
	}
	//return rgn.RectInRegion(fixed); error!
	return rgn.RectInRegion(rectT);
}

CDrawObj* CDrawText::Clone(CJTDrawDoc* pDoc)
{
	ASSERT_VALID(this);

	CDrawText* pClone = new CDrawText(m_position);
	pClone->m_bPen = m_bPen;
	pClone->m_logpen = m_logpen;
	pClone->m_bBrush = m_bBrush;
	pClone->m_logbrush = m_logbrush;
	pClone->m_nShape = m_nShape;
	pClone->m_roundness = m_roundness;
    pClone->m_text=m_text;
	pClone->m_bBorder=m_bBorder;
	ASSERT_VALID(pClone);

	if (pDoc != NULL)
		pDoc->Add(pClone);

	ASSERT_VALID(pClone);
	return pClone;
}

void CDrawText::OnEditProperties()
{
	ASSERT_VALID(this);

    CDrawProperty sheet ( _T("Properties") );
	CLineProperty linePage;
	linePage.m_bNoFill = !m_bBrush;
	linePage.m_penSize = m_bPen ? m_logpen.lopnWidth.x : 0;
	linePage.m_FillColor=m_logbrush.lbColor;
	linePage.m_LineColor=m_logpen.lopnColor;
    linePage.m_lineStyle=m_dstyle;
	linePage.m_isArrow=false;
	
	CTextProperty textPage;
	textPage.m_text=m_text;
    textPage.m_font.DeleteObject();
	textPage.m_font.CreateFontIndirect(&m_logfont);
	
	textPage.m_borderType=(int)m_nShape; //
	textPage.m_allignment=m_allignment;

	sheet.AddPage( &textPage );
	sheet.AddPage( &linePage );

	if (sheet.DoModal() != IDOK)
		return;

	m_bBrush = !linePage.m_bNoFill;
	m_bPen = linePage.m_penSize > 0;
	if (m_bPen)
	{
	m_logpen.lopnWidth.x = linePage.m_penSize;
		m_logpen.lopnWidth.y = linePage.m_penSize;
	}
	m_logbrush.lbColor=linePage.m_FillColor;
	m_logpen.lopnColor=linePage.m_LineColor;
    m_dstyle=linePage.m_lineStyle;
	m_text=textPage.m_text;
	if(textPage.m_bFont)
		textPage.m_font.GetLogFont(&m_logfont);
	m_nShape=(CDrawRect::Shape)textPage.m_borderType; //
	m_bBorder=textPage.m_borderType;
	m_allignment=textPage.m_allignment;

    //must resize
	POSITION ps=m_pDocument->GetFirstViewPosition();
	CJTDrawView* pView=(CJTDrawView*)m_pDocument->GetNextView(ps);
    Resize(pView);
	
	
	Invalidate();
	m_pDocument->SetModifiedFlag();

}

void CDrawText::Resize(int x, int y)
{
	if(abs(m_position.left-m_position.right)<x+2*m_logpen.lopnWidth.x){
		if (m_position.left<m_position.right) 
			m_position.right=m_position.left+x+2*m_logpen.lopnWidth.x;
        else
			m_position.left=m_position.right+x+2*m_logpen.lopnWidth.x;
	}
	if(abs(m_position.top-m_position.bottom)<y+2*m_logpen.lopnWidth.x){
		if (m_position.top<m_position.bottom) 
			m_position.bottom=m_position.top+y+2*m_logpen.lopnWidth.x;
        else
			m_position.top=m_position.bottom+y+2*m_logpen.lopnWidth.x;
	}
	return;
}

int CDrawText::MaxHeight(CDC *pDC)
{
	CFont font;
	TEXTMETRIC TM;
	if (!font.CreateFontIndirect(&m_logfont))
		return 0;
    CFont *pOldFont=(CFont *)pDC->SelectObject(&font);
	pDC->GetTextMetrics(&TM);
    int x=TM.tmHeight+TM.tmExternalLeading;	
    pDC->SelectObject(pOldFont);
    return x;
}

void CDrawText::SetTextColor(COLORREF color)
{
	ASSERT_VALID(this);
	m_color = color;
	Invalidate();
	m_pDocument->SetModifiedFlag();
}

int CDrawText::TextLen(CDC *pDC)
{
	CFont font;
	if (!font.CreateFontIndirect(&m_logfont))
		return 0;
    CFont *pOldFont=(CFont *)pDC->SelectObject(&font);
	CSize sz=pDC->GetTextExtent(m_text);
    pDC->SelectObject(pOldFont);
	return sz.cx;
}

void CDrawText::Resize(CJTDrawView* pView)
{
	CClientDC CDC(pView);
	CFont font;
	if (!font.CreateFontIndirect(&m_logfont))
		return;
    CFont *pOldFont=(CFont *)CDC.SelectObject(&font);
	int lx= TextLen(&CDC);//length of your string;
    int ly= MaxHeight(&CDC);//hight of the string;
    Resize(lx+2*m_logpen.lopnWidth.x+2*PADDING,
		ly+m_logpen.lopnWidth.x*2+2*PADDING); //resize the m_position, by the new lenght and height of string

    CDC.SelectObject(pOldFont);

}
