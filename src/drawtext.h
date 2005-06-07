#ifndef __DRAWTEXT_H__
#define __DRAWTEXT_H__

class CDrawView;
class CDrawDoc;
#include "drawobj.h"


class CDrawText : public CDrawRect
{
protected:
	DECLARE_SERIAL(CDrawText);
	CDrawText();

public:
	CDrawText(const CRect& position);

// Implementation
public:
	virtual void Serialize(CArchive& ar);
	virtual void Draw(CDC* pDC);
	virtual int GetHandleCount();
	virtual CPoint GetHandle(int nHandle);
	virtual HCURSOR GetHandleCursor(int nHandle);
	virtual void MoveHandleTo(int nHandle, CPoint point, CDrawView* pView = NULL);
	virtual BOOL Intersects(const CRect& rect);
	virtual CDrawObj* Clone(CDrawDoc* pDoc);

protected:
    int m_bBorder;
	CString m_text;

	friend class CTextTool;
};

#endif