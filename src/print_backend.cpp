#include "print_backend.h"

#include "view.h"

#include <windows.h>
#include <algorithm>
#include <commdlg.h>

bool PrintDocument(View& view)
{
	PRINTDLGW dialog{};
	dialog.lStructSize = sizeof(dialog);
	dialog.Flags = PD_RETURNDC | PD_NOSELECTION | PD_NOPAGENUMS;
	dialog.nCopies = 1;

	if (!::PrintDlgW(&dialog) || !dialog.hDC)
	{
		if (dialog.hDevMode) ::GlobalFree(dialog.hDevMode);
		if (dialog.hDevNames) ::GlobalFree(dialog.hDevNames);
		return false;
	}

	const std::wstring title = view.Doc().Title();
	DOCINFOW docInfo{};
	docInfo.cbSize = sizeof(docInfo);
	docInfo.lpszDocName = title.c_str();

	const core::surface_ptr page = RenderDocument(view.Doc(), 200);
	if (!page)
	{
		::DeleteDC(dialog.hDC);
		if (dialog.hDevMode) ::GlobalFree(dialog.hDevMode);
		if (dialog.hDevNames) ::GlobalFree(dialog.hDevNames);
		return false;
	}

	const int pageWidth = static_cast<int>(page->width());
	const int pageHeight = static_cast<int>(page->height());
	const int printerWidth = ::GetDeviceCaps(dialog.hDC, HORZRES);
	const int printerHeight = ::GetDeviceCaps(dialog.hDC, VERTRES);
	const double scale = (std::min)(static_cast<double>(printerWidth) / pageWidth,
	                                static_cast<double>(printerHeight) / pageHeight);
	const int width = static_cast<int>(pageWidth * scale);
	const int height = static_cast<int>(pageHeight * scale);

	// The surface's rows may be padded, so the bitmap is as wide as a row and
	// only the page's own columns are copied from it.
	BITMAPINFO bitmapInfo{};
	bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
	bitmapInfo.bmiHeader.biWidth = static_cast<LONG>(page->stride() / 4);
	bitmapInfo.bmiHeader.biHeight = -pageHeight;
	bitmapInfo.bmiHeader.biPlanes = 1;
	bitmapInfo.bmiHeader.biBitCount = 32;
	bitmapInfo.bmiHeader.biCompression = BI_RGB;

	bool ok = false;
	if (::StartDocW(dialog.hDC, &docInfo) > 0)
	{
		if (::StartPage(dialog.hDC) > 0)
		{
			::SetStretchBltMode(dialog.hDC, HALFTONE);
			::StretchDIBits(dialog.hDC, (printerWidth - width) / 2, (printerHeight - height) / 2, width, height, 0, 0,
			                pageWidth, pageHeight, page->pixels(), &bitmapInfo, DIB_RGB_COLORS, SRCCOPY);
			ok = ::EndPage(dialog.hDC) > 0;
		}
		ok = ::EndDoc(dialog.hDC) > 0 && ok;
	}

	::DeleteDC(dialog.hDC);
	if (dialog.hDevMode) ::GlobalFree(dialog.hDevMode);
	if (dialog.hDevNames) ::GlobalFree(dialog.hDevNames);
	return ok;
}
