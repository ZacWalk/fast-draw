#include "style_panel.h"

#include <algorithm>
#include <array>
#include <string>

namespace
{

	constexpr int kPadding = 10;
	constexpr int kHeader = 20;
	constexpr int kRow = 24;
	constexpr int kGap = 6;
	constexpr int kSwatchWidth = 34;

	const wchar_t* const kDirectionGlyphs[] = {L"\u2193", L"\u2192"};
	static_assert(std::size(kDirectionGlyphs) == kGradientDirectionCount);

	void Chip(render::draw_context& dc, const double dpi, const Rect& bounds, const std::wstring_view label,
	          const bool active, const bool hot)
	{
		const RectF box = RectF::FromRect(bounds);
		const float radius = ui::ScaleF(dpi, 5.0f);

		ui::FillRoundRect(dc, box, radius, active ? ui::kAccent : (hot ? ui::kHover : ui::kPanel));
		ui::FrameRoundRect(dc, box.Inflated(-0.5f), radius, active ? ui::kAccent : (hot ? ui::kAccent : ui::kBorder),
		                   1.0f);

		if (!label.empty())
			ui::Text(dc, bounds, label, active ? Rgb(255, 255, 255) : ui::kText, ui::TextAlign::center,
			         ui::Font(dpi, false));
	}

	bool Contains(const Rect& rect, const Point point)
	{
		return point.x >= rect.left && point.x < rect.right && point.y >= rect.top && point.y < rect.bottom;
	}

} // namespace

void StylePanel::SetExtent(const Size extent, const double dpiScale)
{
	extent_ = extent;
	dpiScale_ = dpiScale;
	Layout();
}

void StylePanel::Invalidate()
{
	if (onInvalidate) onInvalidate();
}

void StylePanel::SetStyle(const Style& style, const bool hasSelection)
{
	if (style == style_ && hasSelection == hasSelection_) return;
	style_ = style;
	hasSelection_ = hasSelection;
	Invalidate();
}

int StylePanel::PreferredHeight(const double dpiScale) const
{
	// Three headed sections of seven rows, and a line under them for the hint.
	return ui::Scale(dpiScale, kPadding * 2 + kHeader * 4 + kRow * 7 + kGap * 6);
}

void StylePanel::Layout()
{
	const int padding = ui::Scale(dpiScale_, kPadding);
	const int header = ui::Scale(dpiScale_, kHeader);
	const int row = ui::Scale(dpiScale_, kRow);
	const int gap = ui::Scale(dpiScale_, kGap);
	const int left = padding;
	const int right = (std::max)(static_cast<int>(extent_.cx), left + ui::Scale(dpiScale_, 120)) - padding;
	const int swatch = ui::Scale(dpiScale_, kSwatchWidth);

	int y = padding;
	auto placeHeader = [&](const int index)
	{
		headers_[static_cast<std::size_t>(index)] = Rect{left, y, right, y + header};
		y += header;
	};
	auto share = [&](const int index, const int count)
	{
		const int width = (right - left - gap * (count - 1)) / count;
		return Rect{left + index * (width + gap), y, left + index * (width + gap) + width, y + row};
	};

	placeHeader(0);
	Bounds(Item::FillColour) = Rect{left, y, left + swatch, y + row};
	Bounds(Item::FillToggle) = Rect{left + swatch + gap, y, right, y + row};
	y += row + gap;

	Bounds(Item::GradientToggle) = Rect{left, y, right - swatch - gap, y + row};
	Bounds(Item::GradientColour) = Rect{right - swatch, y, right, y + row};
	y += row + gap;

	for (int i = 0; i < kGradientDirectionCount; ++i)
		Bounds(static_cast<Item>(static_cast<int>(Item::Direction0) + i)) = share(i, kGradientDirectionCount);
	y += row + gap;

	placeHeader(1);
	Bounds(Item::LineColour) = Rect{left, y, left + swatch, y + row};
	Bounds(Item::LineToggle) = Rect{left + swatch + gap, y, right, y + row};
	y += row + gap;

	const int stepper = ui::Scale(dpiScale_, 26);
	Bounds(Item::WidthDown) = Rect{right - stepper * 2 - gap, y, right - stepper - gap, y + row};
	Bounds(Item::WidthUp) = Rect{right - stepper, y, right, y + row};
	widthValue_ = Rect{left, y, right - stepper * 2 - gap * 2, y + row};
	y += row + gap;

	Bounds(Item::ArrowStart) = share(0, 2);
	Bounds(Item::ArrowEnd) = share(1, 2);
	y += row + gap;

	placeHeader(2);
	Bounds(Item::TextColour) = Rect{left, y, left + swatch, y + row};
	Bounds(Item::SizeDown) = Rect{left + swatch + gap, y, left + swatch + gap + stepper, y + row};
	sizeValue_ = Rect{left + swatch + gap * 2 + stepper, y, left + swatch + gap * 2 + stepper * 2, y + row};
	Bounds(Item::SizeUp) = Rect{left + swatch + gap * 3 + stepper * 2, y, left + swatch + gap * 3 + stepper * 3,
	                            y + row};
	y += row;

	contentHeight_ = y + padding;
}

void StylePanel::Render(render::draw_context& dc)
{
	Layout();

	dc.draw_rect({0, 0, extent_.cx, extent_.cy}, ui::ToCoreColor(ui::kChrome));

	const wchar_t* const titles[] = {L"Fill", L"Line", L"Text"};
	for (std::size_t i = 0; i < headers_.size(); ++i)
	{
		ui::Text(dc, headers_[i], titles[i], ui::kTextDim, ui::TextAlign::left, ui::Font(dpiScale_, true));
	}

	const auto hot = [this](const Item item)
	{
		return hot_ == static_cast<int>(item);
	};
	const float radius = ui::ScaleF(dpiScale_, 4.0f);

	ui::Swatch(dc, RectF::FromRect(Bounds(Item::FillColour)), style_.fill, !style_.hasFill, hot(Item::FillColour),
	           radius);
	Chip(dc, dpiScale_, Bounds(Item::FillToggle), style_.hasFill ? L"Filled" : L"No fill", style_.hasFill,
	     hot(Item::FillToggle));

	Chip(dc, dpiScale_, Bounds(Item::GradientToggle), L"Gradient", style_.gradient, hot(Item::GradientToggle));
	ui::Swatch(dc, RectF::FromRect(Bounds(Item::GradientColour)), style_.fillEnd, false, hot(Item::GradientColour),
	           radius);

	for (int i = 0; i < kGradientDirectionCount; ++i)
	{
		const auto item = static_cast<Item>(static_cast<int>(Item::Direction0) + i);
		const bool active = style_.gradient && static_cast<int>(style_.gradientDirection) == i;
		Chip(dc, dpiScale_, Bounds(item), kDirectionGlyphs[i], active, hot(item));
	}

	ui::Swatch(dc, RectF::FromRect(Bounds(Item::LineColour)), style_.line, !style_.hasLine, hot(Item::LineColour),
	           radius);
	Chip(dc, dpiScale_, Bounds(Item::LineToggle), style_.hasLine ? L"Stroked" : L"No line", style_.hasLine,
	     hot(Item::LineToggle));

	ui::Text(dc, widthValue_, L"Width " + std::to_wstring(style_.lineWidth), ui::kText, ui::TextAlign::left,
	         ui::Font(dpiScale_));
	Chip(dc, dpiScale_, Bounds(Item::WidthDown), L"\u2212", false, hot(Item::WidthDown));
	Chip(dc, dpiScale_, Bounds(Item::WidthUp), L"+", false, hot(Item::WidthUp));

	Chip(dc, dpiScale_, Bounds(Item::ArrowStart), L"\u25c0\u2014", style_.startArrow, hot(Item::ArrowStart));
	Chip(dc, dpiScale_, Bounds(Item::ArrowEnd), L"\u2014\u25b6", style_.endArrow, hot(Item::ArrowEnd));

	ui::Swatch(dc, RectF::FromRect(Bounds(Item::TextColour)), style_.textColour, false, hot(Item::TextColour), radius);
	Chip(dc, dpiScale_, Bounds(Item::SizeDown), L"\u2212", false, hot(Item::SizeDown));
	ui::Text(dc, sizeValue_, std::to_wstring(style_.fontSize), ui::kText, ui::TextAlign::center, ui::Font(dpiScale_));
	Chip(dc, dpiScale_, Bounds(Item::SizeUp), L"+", false, hot(Item::SizeUp));

	if (!hasSelection_)
	{
		Rect hint{ui::Scale(dpiScale_, kPadding), contentHeight_ - ui::Scale(dpiScale_, kPadding),
		          extent_.cx - ui::Scale(dpiScale_, kPadding), contentHeight_ + ui::Scale(dpiScale_, kRow)};
		ui::Text(dc, hint, L"Defaults for new shapes", ui::kTextDim, ui::TextAlign::left, ui::Font(dpiScale_));
	}

	ui::Line(dc, {0.0f, static_cast<float>(extent_.cy) - 0.5f},
	         {static_cast<float>(extent_.cx), static_cast<float>(extent_.cy) - 0.5f}, ui::kBorder, 1.0f);
	ui::Line(dc, {static_cast<float>(extent_.cx) - 0.5f, 0.0f},
	         {static_cast<float>(extent_.cx) - 0.5f, static_cast<float>(extent_.cy)}, ui::kBorder, 1.0f);
}

int StylePanel::HitTest(const Point point) const
{
	for (std::size_t i = 0; i < bounds_.size(); ++i)
		if (Contains(bounds_[i], point)) return static_cast<int>(i);
	return -1;
}

void StylePanel::OnMouseMove(const Point point)
{
	const int hot = HitTest(point);
	if (hot == hot_) return;
	hot_ = hot;
	Invalidate();
}

void StylePanel::OnMouseLeave()
{
	tracking_ = false;
	if (hot_ == -1) return;
	hot_ = -1;
	Invalidate();
}

void StylePanel::OnLeftDown(const Point point)
{
	pressed_ = HitTest(point);
	if (pressed_ >= 0 && onCapture) onCapture(true);
}

void StylePanel::OnLeftUp(const Point point)
{
	const int pressed = pressed_;
	pressed_ = -1;
	if (onCapture) onCapture(false);

	const int index = HitTest(point);
	if (index >= 0 && index == pressed) Activate(static_cast<Item>(index));
}

void StylePanel::Activate(const Item item)
{
	auto pick = [this](Color& colour)
	{
		if (!onPickColour) return false;
		if (auto chosen = onPickColour(colour))
		{
			colour = *chosen;
			return true;
		}
		return false;
	};

	switch (item)
	{
	case Item::FillColour:
		if (pick(style_.fill)) style_.hasFill = true;
		break;
	case Item::FillToggle: style_.hasFill = !style_.hasFill; break;
	case Item::GradientToggle: style_.gradient = !style_.gradient; break;
	case Item::GradientColour:
		if (pick(style_.fillEnd)) style_.gradient = true;
		break;
	case Item::Direction0:
	case Item::Direction1:
		style_.gradientDirection = static_cast<GradientDirection>(static_cast<int>(item) -
		                                                          static_cast<int>(Item::Direction0));
		style_.gradient = true;
		break;
	case Item::LineColour:
		if (pick(style_.line)) style_.hasLine = true;
		break;
	case Item::LineToggle: style_.hasLine = !style_.hasLine; break;
	case Item::WidthDown: style_.lineWidth = (std::max)(1, style_.lineWidth - 1); break;
	case Item::WidthUp: style_.lineWidth = (std::min)(40, style_.lineWidth + 1); break;
	case Item::ArrowStart: style_.startArrow = !style_.startArrow; break;
	case Item::ArrowEnd: style_.endArrow = !style_.endArrow; break;
	case Item::TextColour: pick(style_.textColour); break;
	case Item::SizeDown: style_.fontSize = (std::max)(6, style_.fontSize - 1); break;
	case Item::SizeUp: style_.fontSize = (std::min)(96, style_.fontSize + 1); break;
	case Item::Count: return;
	}

	Publish();
}

void StylePanel::Publish()
{
	Invalidate();
	if (onChange) onChange(style_);
}
