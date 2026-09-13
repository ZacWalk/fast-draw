// A modal colour picker rendered by hand: a curated palette grid plus the
// current colour. Returns nothing when the user dismisses it.

#pragma once

#include <windows.h>

#include <optional>

// `anchor` is a screen rectangle the popup is placed under, usually the swatch
// that was clicked.
std::optional<COLORREF> PickColour(HWND owner, const RECT& anchor, COLORREF initial);
