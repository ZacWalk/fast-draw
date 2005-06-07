// drawobj.cpp - implementation for drawing objects
//
// This is a part of the Microsoft Foundation Classes C++ library.
// Copyright (C) 1992-1998 Microsoft Corporation
// All rights reserved.
//
// This source code is only intended as a supplement to the
// Microsoft Foundation Classes Reference and related
// electronic documentation provided with the library.
// See these sources for detailed information regarding the
// Microsoft Foundation Classes product.


#include "stdafx.h"
#include "drawcli.h"

#include "drawdoc.h"
#include "drawvw.h"
#include "drawtext.h"
#include "cntritem.h"
#include "rectdlg.h"

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

}
void CDrawText::Serialize(CArchive& ar)
{
	ASSERT_VALID(this);

	CDrawObj::Serialize(ar);
	if (ar.IsStoring())
	{
		ar << m_bBorder;
		ar << m_text;
	}
	else
	{
		ar >> m_bBorder;
		ar >> m_text;
	}
}

void CDrawText::Draw(CDC* pDC)
{
	ASSERT_VALID(this);

    if (m_bBorder){
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

		CRect rect = m_position;
		switch (m_nShape)
		{
		case rectangle:
			pDC->Rectangle(rect);
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

			pDC->MoveTo(rect.TopLeft());
			pDC->LineTo(rect.BottomRight());
			break;
		}

		pDC->SelectObject(pOldBrush);
		pDC->SelectObject(pOldPen);
	}
    //drawthe text here;


}


int CDrawText::GetHandleCount()
{
	ASSERT_VALID(this);

	return CDrawRect::GetHandleCount();
}

// returns center of handle in logical coordinates
CPoint CDrawText::GetHandle(int nHandle)
{
	ASSERT_VALID(this);
	return CDrawRect::GetHandle(nHandle);
}

HCURSOR CDrawText::GetHandleCursor(int nHandle)
{
	ASSERT_VALID(this);
	return CDrawRect::GetHandleCursor(nHandle);
}

// point is in logical coordinates
void CDrawText::MoveHandleTo(int nHandle, CPoint point, CDrawView* pView)
{
	ASSERT_VALID(this);
	CDrawRect::MoveHandleTo(nHandle, point, pView);
}

// rect must be in logical coordinates
BOOL CDrawText::Intersects(const CRect& rect)
{
	ASSERT_VALID(this);
	return CDrawRect.Intersects(rect);
}

CDrawObj* CDrawText::Clone(CDrawDoc* pDoc)
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

