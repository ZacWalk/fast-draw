#include "print_backend.h"

#include "view.h"

#include <windows.h>
#include <algorithm>
#include <commdlg.h>

bool PrintDocument(View& view) {
    PRINTDLGW dialog{};
    dialog.lStructSize = sizeof(dialog);
    dialog.Flags = PD_RETURNDC | PD_NOSELECTION | PD_NOPAGENUMS;
    dialog.nCopies = 1;

    if (!::PrintDlgW(&dialog) || !dialog.hDC) {
        if (dialog.hDevMode) ::GlobalFree(dialog.hDevMode);
        if (dialog.hDevNames) ::GlobalFree(dialog.hDevNames);
        return false;
    }

    const std::wstring title = view.Doc().Title();
    DOCINFOW docInfo{};
    docInfo.cbSize = sizeof(docInfo);
    docInfo.lpszDocName = title.c_str();

    Canvas canvas;
    if (!RenderDocumentToCanvas(view.Doc(), canvas, 200)) {
        ::DeleteDC(dialog.hDC);
        if (dialog.hDevMode) ::GlobalFree(dialog.hDevMode);
        if (dialog.hDevNames) ::GlobalFree(dialog.hDevNames);
        return false;
    }

    const int printerWidth = ::GetDeviceCaps(dialog.hDC, HORZRES);
    const int printerHeight = ::GetDeviceCaps(dialog.hDC, VERTRES);
    const double scale = (std::min)(static_cast<double>(printerWidth) / canvas.Width(),
                                    static_cast<double>(printerHeight) / canvas.Height());
    const int width = static_cast<int>(canvas.Width() * scale);
    const int height = static_cast<int>(canvas.Height() * scale);

    BITMAPINFO bitmapInfo{};
    bitmapInfo.bmiHeader.biSize = sizeof(bitmapInfo.bmiHeader);
    bitmapInfo.bmiHeader.biWidth = canvas.Width();
    bitmapInfo.bmiHeader.biHeight = -canvas.Height();
    bitmapInfo.bmiHeader.biPlanes = 1;
    bitmapInfo.bmiHeader.biBitCount = 32;
    bitmapInfo.bmiHeader.biCompression = BI_RGB;

    bool ok = false;
    if (::StartDocW(dialog.hDC, &docInfo) > 0) {
        if (::StartPage(dialog.hDC) > 0) {
            ::SetStretchBltMode(dialog.hDC, HALFTONE);
            ::StretchDIBits(dialog.hDC, (printerWidth - width) / 2,
                            (printerHeight - height) / 2, width, height, 0, 0,
                            canvas.Width(), canvas.Height(), canvas.Pixels(), &bitmapInfo,
                            DIB_RGB_COLORS, SRCCOPY);
            ok = ::EndPage(dialog.hDC) > 0;
        }
        ok = ::EndDoc(dialog.hDC) > 0 && ok;
    }

    ::DeleteDC(dialog.hDC);
    if (dialog.hDevMode) ::GlobalFree(dialog.hDevMode);
    if (dialog.hDevNames) ::GlobalFree(dialog.hDevNames);
    return ok;
}
