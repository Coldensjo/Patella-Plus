// DialogElements.cpp: the controls of the dialogs and windows
// (c:\mud\tibiaplayer\DialogElements.cpp); see DialogElements.h.
//
// Order: the functions follow the source's line numbers. The binary moves
// most constructors and a few members that call functions defined further
// down (the labels' constructors, TFormattedText's constructor and Arrange,
// the buttons' constructors, TCheckBox's constructor and SetEnabled, the edit
// lines' first constructor, LeftButtonDown, KeyPress and SetFocusGroup,
// TEditText's Arrange, KeyPress and SetText, the scroll bars' and the list
// boxes' constructors and several list box members, TLink's constructor) to
// the end of the object, Creatures.cpp's pattern. Functions without line
// evidence go where the binary has them.

#include "DialogElements.h"
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <cstdarg>
#include "Status.h"
#include "Surface.h"
#include "Utils.h"
#include "sysinc.h"
#include "GUI.h"
#include "Skins.h"
#include "common.h"
#include "compat/span.h"
#include "types/FixedString.h"
#include "graphics.h"
#include <cstddef>
#include <memory>

// Issue #111 (clang-tidy, the first look at this file in patella_core): cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg off for the file:
// the original's raw owners (notes/ownership.md): the window tree owns its children, the message queue its blocks,
// the lists their entries; the original's varargs (TMultiStateButton's constructor, _snprintf)
// NOLINTBEGIN(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg)


// --- TSeparator -------------------------------------------------------------------

void TSeparator::SetSize(int Width, int Height)
{
	TComponent::SetSize(Width, Height);
	Arrange();
}

void TSeparator::Arrange()
{
	if (!Horizontal) {
		DrawingList[0] = TDrawing(SKIN_FRAME_EDGE_LEFT, 0, 0, Skins()[SKIN_FRAME_EDGE_LEFT].width, Height, 0, 0);
		DrawingList[1] = TDrawing(SKIN_FRAME_EDGE_RIGHT, Skins()[SKIN_FRAME_EDGE_LEFT].width, 0, Skins()[SKIN_FRAME_EDGE_RIGHT].width, Height, 0, 0);
	} else {
		DrawingList[0] = TDrawing(SKIN_FRAME_EDGE_TOP, 0, 0, Width, Skins()[SKIN_FRAME_EDGE_TOP].height, 0, 0);
		DrawingList[1] = TDrawing(SKIN_FRAME_EDGE_BOTTOM, 0, Skins()[SKIN_FRAME_EDGE_TOP].height, Width, Skins()[SKIN_FRAME_EDGE_BOTTOM].height, 0, 0);
	}
}

// --- TResizer ---------------------------------------------------------------------

TResizer::TResizer(TComponent *_ToResize, bool _ResizeWidth, bool _ResizeHeight, bool _Invisible)
{
	PATELLA_ASSERT(_ToResize!=nullptr);
	ToResize = _ToResize;
	Invisible = _Invisible;
	DrawingListLength = 1;
	DrawingList.reset(new TDrawing[1]);
	Dragging = false;
	StartY = 0;
	StartX = 0;
	ResizeWidth = _ResizeWidth;
	ResizeHeight = _ResizeHeight;
}

void TResizer::SetSize(int Width, int Height)
{
	TComponent::SetSize(Width, Height);
	Arrange();
}

void TResizer::Arrange()
{
	if (!Invisible)
		DrawingList[0] = TDrawing(SKIN_RESIZER, 0, 0, Width, Height, 0, 0);
	else
		DrawingList[0] = TDrawing(SKIN_NONE, 0, 0, 0, 0, 0, 0);
}

// The cursor is set before the assertion.
bool TResizer::MouseMove(int x, int y)
{
	SetMouseCursor(CURSOR_SIZENS);
	PATELLA_ASSERT(ToResize!=nullptr);
	if (Dragging) {
		int NewWidth = ToResize->Width;
		if (ResizeWidth)
			NewWidth = ToResize->Width - StartX + x;
		int NewHeight = ToResize->Height;
		if (ResizeHeight)
			NewHeight = ToResize->Height - StartY + y;
		ToResize->SetSize(NewWidth, NewHeight);
	}
	return true;
}

bool TResizer::LeftButtonDown(int x, int y, [[maybe_unused]] int Flags)
{
	PATELLA_ASSERT(ToResize!=nullptr);
	Dragging = true;
	SetCapture(nullptr);
	StartX = x;
	StartY = y;
	return true;
}

bool TResizer::LeftButtonUp(int x, int y, [[maybe_unused]] int Flags)
{
	PATELLA_ASSERT(ToResize!=nullptr);
	if (Dragging) {
		int NewWidth = ToResize->Width;
		if (ResizeWidth)
			NewWidth = ToResize->Width - StartX + x;
		int NewHeight = ToResize->Height;
		if (ResizeHeight)
			NewHeight = ToResize->Height - StartY + y;
		ToResize->SetSize(NewWidth, NewHeight);
		ReleaseCapture(nullptr);
	}
	return true;
}

// --- TBorder ----------------------------------------------------------------------

// WindowArea is cleared by its constructor and again in the body.
TBorder::TBorder(TComponent *_Window, int _Border)
{
	PATELLA_ASSERT(_Window!=nullptr);
	WindowArea = TRect(0, 0, 0, 0);
	DrawingListLength = 9;
	DrawingList.reset(new TDrawing[9]);
	Border = _Border;
	Window = _Window;
	TWindow::InsertChild(_Window);
	Background = SKIN_NONE;
	TopLeft = SKIN_BORDER_CORNER_TOPLEFT;
	Top = SKIN_BORDER_EDGE_TOP;
	TopRight = SKIN_BORDER_CORNER_TOPRIGHT;
	Left = SKIN_BORDER_EDGE_LEFT;
	Right = SKIN_BORDER_EDGE_RIGHT;
	BottomLeft = SKIN_BORDER_CORNER_BOTTOMLEFT;
	Bottom = SKIN_BORDER_EDGE_BOTTOM;
	BottomRight = SKIN_BORDER_CORNER_BOTTOMRIGHT;
}

TBorder::TBorder(TComponent *_Window, int _TopLeft, int _Top, int _TopRight, int _Left, int _Right, int _BottomLeft,
	int _Bottom, int _BottomRight, int _Border)
{
	PATELLA_ASSERT(_Window!=nullptr);
	WindowArea = TRect(0, 0, 0, 0);
	DrawingListLength = 9;
	DrawingList.reset(new TDrawing[9]);
	Border = _Border;
	Window = _Window;
	TWindow::InsertChild(_Window);
	Top = _Top;
	TopLeft = _TopLeft;
	Right = _Right;
	TopRight = _TopRight;
	Left = _Left;
	BottomRight = _BottomRight;
	BottomLeft = _BottomLeft;
	Background = SKIN_NONE;
	Bottom = _Bottom;
}

TBorder::TBorder(TComponent *_Window, int _TopLeft, int _Top, int _TopRight, int _Left, int _Right, int _BottomLeft,
	int _Bottom, int _BottomRight, int _Background, int _Border)
{
	PATELLA_ASSERT(_Window!=nullptr);
	WindowArea = TRect(0, 0, 0, 0);
	DrawingListLength = 9;
	DrawingList.reset(new TDrawing[9]);
	Border = _Border;
	Window = _Window;
	TWindow::InsertChild(_Window);
	Top = _Top;
	TopRight = _TopRight;
	TopLeft = _TopLeft;
	Right = _Right;
	BottomLeft = _BottomLeft;
	Left = _Left;
	BottomRight = _BottomRight;
	Background = _Background;
	Bottom = _Bottom;
}

void TBorder::SetSize(int Width, int Height)
{
	PATELLA_ASSERT_VALUES(Width>=GetMinimumWidth(), Width);
	PATELLA_ASSERT_VALUES(Height>=GetMinimumHeight(), Height);
	TComponent::SetSize(Width, Height);
	Arrange();
	Window->SetPosition(WindowArea.x, WindowArea.y);
	Window->SetSize(WindowArea.width, WindowArea.height);
	Window->Arrange();
}

// The frame's size for Window's size.
void TBorder::FitToWindow()
{
	SetSize(GetMinimumWidth() + Window->Width, GetMinimumHeight() + Window->Height);
}

void TBorder::Arrange()
{
	WindowArea.x = Skins()[Left].width + Border;
	WindowArea.width = Width - 2 * Border - Skins()[Right].width - Skins()[Left].width;
	WindowArea.y = Skins()[Top].height + Border;
	WindowArea.height = Height - 2 * Border - Skins()[Bottom].height - Skins()[Top].height;
	DrawingList[0] = TDrawing(TopLeft, 0, 0, 0, 0, 0, 0);
	DrawingList[1] = TDrawing(Top, Skins()[TopLeft].width, 0, Width - Skins()[TopRight].width - Skins()[TopLeft].width,
		Skins()[Top].height, 0, 0);
	DrawingList[2] = TDrawing(TopRight, Width - Skins()[TopRight].width, 0, 0, 0, 0, 0);
	DrawingList[3] = TDrawing(Left, 0, Skins()[TopLeft].height, Skins()[Left].width,
		Height - Skins()[BottomLeft].height - Skins()[TopLeft].height, 0, 0);
	DrawingList[4] = TDrawing(Right, Width - Skins()[Right].width, Skins()[TopRight].height, Skins()[Right].width,
		Height - Skins()[BottomRight].height - Skins()[TopRight].height, 0, 0);
	DrawingList[5] = TDrawing(BottomLeft, 0, Height - Skins()[BottomLeft].height, 0, 0, 0, 0);
	DrawingList[6] = TDrawing(Bottom, Skins()[BottomLeft].width, Height - Skins()[Bottom].height,
		Width - Skins()[BottomRight].width - Skins()[BottomLeft].width, Skins()[Bottom].height, 0, 0);
	DrawingList[7] = TDrawing(BottomRight, Width - Skins()[BottomRight].width, Height - Skins()[BottomRight].height, 0, 0,
		0, 0);
	if (Background != SKIN_NONE)
		DrawingList[8] = TDrawing(Background, Skins()[Left].width, Skins()[Top].height,
			Width - Skins()[Right].width - Skins()[Left].width, Height - Skins()[Bottom].height - Skins()[Top].height, 0, 0);
	else
		DrawingList[8] = TDrawing(SKIN_NONE, 0, 0, 0, 0, 0, 0);
}

// --- TLabel -----------------------------------------------------------------------

// The font's constructor runs before the assertion.
TLabel::TLabel(int x, int y, int Width, int Height, TFont _Font, const char *_Text, bool Shorten)
{
	PATELLA_ASSERT(_Text!=nullptr);
	Font = _Font;
	Text = nullptr;
	FullText = nullptr;
	Line = nullptr;
	Shortened = false;
	TComponent::SetPosition(x, y);
	TComponent::SetSize(Width, Height);
	SetText(_Text, Shorten);
}

// Sized to the widest line.
TLabel::TLabel(int x, int y, TFont _Font, const char *_Text)
{
	PATELLA_ASSERT(_Text!=nullptr);
	Font = _Font;
	Text = nullptr;
	FullText = nullptr;
	Line = nullptr;
	Shortened = false;
	SetText(_Text, false);
	int MaxWidth = 0;
	for (int i = 0; i < Lines; i++) {
		const int LineWidth = GetTextWidth(Font.FontNumber, Line[i]);
		if (LineWidth >= MaxWidth)
			MaxWidth = LineWidth;
	}
	TComponent::SetPosition(x, y);
	TComponent::SetSize(MaxWidth, Lines * GetFontLineHeight(Font.FontNumber));
}

TLabel::TLabel(TFont _Font, const char *_Text)
{
	PATELLA_ASSERT(_Text!=nullptr);
	Font = _Font;
	Text = nullptr;
	FullText = nullptr;
	Line = nullptr;
	Shortened = false;
	SetText(_Text, false);
	int MaxWidth = 0;
	for (int i = 0; i < Lines; i++) {
		const int LineWidth = GetTextWidth(Font.FontNumber, Line[i]);
		if (LineWidth >= MaxWidth)
			MaxWidth = LineWidth;
	}
	TComponent::SetSize(MaxWidth, Lines * GetFontLineHeight(Font.FontNumber));
}

// A text of several lines starts at the top.
void TLabel::Draw(int Surface, int x, int y)
{
	int TextX, TextY;
	GetTextPosition(&TextX, &TextY, x, y, Width, Height, &Font);
	if (Lines > 1)
		TextY = y;
	for (int i = 0; i < Lines; i++)
		PrintTextClipped(Surface, TextX, TextY + GetFontLineHeight(Font.FontNumber) * i, Font.FontNumber, Font.Color,
			Line[i], Font.Alignment, TRect(x, y, Width, Height));
}

void TLabel::MouseHover([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	if (Shortened)
		SetMouseOverText(FullText.get());
}

// Line breaks end the lines; Shorten cuts each line to the width.
void TLabel::SetText(const char *_Text, bool Shorten)
{
	PATELLA_ASSERT(_Text!=nullptr);
	if (Text != nullptr) {
		Text.reset();
		Text = nullptr;
	}
	if (FullText != nullptr) {
		FullText.reset();
		FullText = nullptr;
	}
	if (Line != nullptr) {
		Line.reset();
		Line = nullptr;
	}
	Shortened = false;
	const int Length = (int)strlen(_Text);
	Text.reset(new char[Length + 2]);
	FullText.reset(new char[Length + 2]);
	TStringBuffer(FullText.get(), static_cast<std::size_t>(Length) + 2).Assign(_Text);
	TStringBuffer(Text.get(), static_cast<std::size_t>(Length) + 2).Assign(_Text);
	Lines = 1;
	for (int i = 0; i < Length; i++) {
		if (Text[i] == '\n')
			Lines++;
	}
	Line.reset(new char *[Lines]);
	Line[0] = Text.get();
	int Number = 1;
	for (int i = 0; i < Length; i++) {
		if (Text[i] == '\n') {
			Line[Number] = Text.get() + i + 1;
			Number++;
			Text[i] = 0;
		}
	}
	Text[Length + 1] = 0;
	if (Shorten) {
		for (int i = 0; i < Lines; i++) {
			if (ShortenString(Line[i], GetFittingTextLength(Font.FontNumber, Line[i], Width)))
				Shortened = true;
		}
	}
}

// --- TFormattedText ---------------------------------------------------------------

TFormattedText::TFormattedText(const char *_Text)
{
	AutoWidth = true;
	Text = nullptr;
	Line = nullptr;
	SetText(_Text, false);
}

void TFormattedText::SetSize(int Width, int Height)
{
	AutoWidth = true;
	if (Width < 1)
		Width = 0;
	else
		AutoWidth = false;
	if (Height < 1)
		Height = 0;
	TComponent::SetSize(Width, Height);
	Arrange();
}

// The length of the token at the start of Text (0 when there is none):
// <b>, <n> and <h> (types 1-3) or <name=number> (types 10-18). A failed
// <name=...> keeps the type it matched.
int TFormattedText::GetToken(const char *Text, int &TokenType, int &TokenData)
{
	int State = 0;
	TokenType = 0;
	int Position = 0;
	TokenData = 0;
	for (;;) {
		const char c = Text[Position];
		switch (State) {
		case 0:
			if (c != '<')
				return 0;
			Position++;
			State = 1;
			break;
		case 1:
			if (c == '>') {
				Position++;
				State = 5;
			} else if (c == '=') {
				Position++;
				State = 6;
			} else {
				if (!IsAlpha(c))
					return 0;
				Position++;
			}
			break;
		case 5: {
			const int Length = Position - 2;
			Text++;
			if (CompareStrings(Text, "b", Length) == 0) {
				TokenType = 1;
			} else if (CompareStrings(Text, "n", Length) == 0) {
				TokenType = 2;
			} else {
				if (CompareStrings(Text, "h", Length) != 0)
					return 0;
				TokenType = 3;
			}
			return Position;
		}
		case 6: {
			const int Length = Position - 2;
			const char *Name = Text + 1;
			if (CompareStrings(Name, "font", Length) == 0) {
				TokenType = 10;
				State = 7;
			} else if (CompareStrings(Name, "hspace", Length) == 0) {
				TokenType = 11;
				State = 7;
			} else if (CompareStrings(Name, "indent", Length) == 0) {
				TokenType = 12;
				State = 7;
			} else if (CompareStrings(Name, "br", Length) == 0) {
				TokenType = 13;
				State = 7;
			} else if (CompareStrings(Name, "color", Length) == 0) {
				TokenType = 14;
				State = 7;
			} else if (CompareStrings(Name, "skin", Length) == 0) {
				TokenType = 15;
				State = 7;
			} else if (CompareStrings(Name, "red", Length) == 0) {
				TokenType = 16;
				State = 7;
			} else if (CompareStrings(Name, "green", Length) == 0) {
				TokenType = 17;
				State = 7;
			} else {
				if (CompareStrings(Name, "blue", Length) != 0)
					return 0;
				TokenType = 18;
				State = 7;
			}
			break;
		}
		case 7:
			if (!IsDigit(c)) {
				if (c != '>')
					return 0;
				return Position + 1;
			}
			TokenData = c - '0' + TokenData * 10;
			Position++;
			break;
		default:
			break;
		}
	}
}

// Lays the text out in pieces from left to right: a TLabel per run of text
// in the current font and colour, a TSkinIcon per <skin=n>, the other
// tokens change the font, colour, position and indent. A line break or a
// word that passes the width starts a new line at the indent. A '<' that
// starts no token advances the position twice, and IsSpace sees the
// character after it.
void TFormattedText::Arrange()
{
	while (FirstChild != nullptr)
		DeleteChild(FirstChild);
	TRGBColor Color(224, 224, 224);
	int FontNumber = FONT_BORDERED;
	int Indent = 0;
	int X = 0;
	int Y = 0;
	const int MaxWidth = AutoWidth ? -1 : Width;
	const char *Piece = Text.get();
	bool Finished = false;
	for (;;) {
		bool LineBreak = false;
		int Position = 0;
		int Skip = 0;
		int LastSpace = -1;
		int TokenType = 0;
		int TokenData = 0;
		for (;;) {
			if (Finished) {
				ClipChildren(false);
				return;
			}
			const char c = Piece[Position];
			if (c == 0) {
				Finished = true;
				break;
			}
			if (c == '<') {
				Skip = GetToken(Piece + Position, TokenType, TokenData);
				if (Skip > 0) {
					if (Position == 0)
						break;
					if (Position > 0) {
						TokenType = 0;
						Skip = 0;
						break;
					}
				}
				Position++;
			}
			if (IsSpace(c))
				LastSpace = Position;
			if (c == '\n') {
				LineBreak = true;
				Skip = 1;
				break;
			}
			if (MaxWidth > 0 && GetTextPartWidth(FontNumber, Piece, Position + 1) + X > MaxWidth) {
				LineBreak = true;
				if (LastSpace != -1)
					Skip = 1;
				break;
			}
			Position++;
		}
		if (LineBreak && LastSpace > 0)
			Position = LastSpace;
		TComponent *Element = nullptr;
		switch (TokenType) {
		case 0: {
			CopyString(patella::TSpan<char>(Line.get(), static_cast<std::size_t>(Position + 1)), Piece);
			TFont Font;
			Font.FontNumber = FontNumber;
			Font.Color = Color;
			Font.Alignment = LEFT_ALIGN;
			Element = new TLabel(Font, Line.get());
			break;
		}
		case 1:
			Color = TRGBColor(255, 128, 128);
			break;
		case 2:
			Color = TRGBColor(224, 224, 224);
			break;
		case 3:
			Color = TRGBColor(224, 224, 0);
			break;
		case 10:
			FontNumber = TokenData;
			break;
		case 11:
			if (TokenData >= 1 && TokenData <= 200)
				X += TokenData;
			break;
		case 12:
			if (TokenData >= 0 && TokenData <= 200)
				Indent = TokenData;
			break;
		case 13:
			if (TokenData >= 1 && TokenData <= 50)
				Y += TokenData;
			break;
		case 14:
			Color = TRGBColor(TokenData / 36 * 51, TokenData / 6 % 6 * 51, TokenData % 6 * 51);
			break;
		case 15:
			if (TokenData >= 0 && TokenData <= LASTSKIN)
				Element = new TSkinIcon(TokenData);
			break;
		case 16:
			Color = TRGBColor(TokenData, Color.G, Color.B);
			break;
		case 17:
			Color = TRGBColor(Color.R, TokenData, Color.B);
			break;
		case 18:
			Color = TRGBColor(Color.R, Color.G, TokenData);
			break;
		default:
			TERROR1("unknown token type", TokenType);
		}
		if (Element != nullptr) {
			InsertChild(Element);
			Element->SetPosition(X, Y);
		}
		if (LineBreak) {
			Y += GetFontLineHeight(FontNumber);
			X = Indent;
		} else {
			X += Element != nullptr ? Element->Width : 0;
			if (X < Indent)
				X = Indent;
		}
		Piece += Position + Skip;
	}
}

void TFormattedText::SetText(const char *_Text, bool Rearrange)
{
	PATELLA_ASSERT(_Text!=nullptr);
	if (Text != nullptr)
		Text.reset();
	if (Line != nullptr)
		Line.reset();
	Text = nullptr;
	Line = nullptr;
	const std::size_t Size = strlen(_Text) + 1;
	Text.reset(new char[Size]);
	Line.reset(new char[Size]);
	TStringBuffer(Text.get(), Size).Assign(_Text);
	if (Rearrange)
		Arrange();
}

// --- TLink ------------------------------------------------------------------------

TLink::TLink(const char *_Text, const char *ButtonText, const char *_Destination, void (*_OpenURL)(const char *URL),
	TFont Font)
{
	PATELLA_ASSERT(_Destination!=nullptr);
	PATELLA_ASSERT(_Text!=nullptr);
	PATELLA_ASSERT(ButtonText!=nullptr);
	const std::size_t Size = strlen(_Destination) + 1;
	Destination.reset(new char[Size]);
	TStringBuffer(Destination.get(), Size).Assign(_Destination);
	OpenURL = _OpenURL;
	Label = new TLabel(Font, _Text);
	TWindow::InsertChild(Label);
	Button = new TTextButton(0, 0, ButtonText, SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, 1), Destination.get());
	TWindow::InsertChild(Button);
}

// The label fills the width left of the button; the smaller of both is
// centred vertically on the larger.
void TLink::Arrange()
{
	const int Width = this->Width - Button->Width;
	PATELLA_ASSERT_VALUES(Width>0, Width);
	int LabelY = 0;
	int ButtonY = 0;
	if (Button->Height < Label->Height)
		ButtonY = (Label->Height - Button->Height) / 2;
	else
		LabelY = (Button->Height - Label->Height) / 2;
	SetChildArea(Label, TRect(0, LabelY, Width, Label->Height));
	SetChildPosition(Button, -Button->Width, ButtonY);
}

void TLink::SetSize(int Width, int Height)
{
	TComponent::SetSize(Width, Height);
	Arrange();
}

void TLink::HandleEvent(int Event)
{
	if (Event == 1) {
		OpenURL(Destination.get());
	}
}

// --- TOutfitIcon ------------------------------------------------------------------

TOutfitIcon::TOutfitIcon(int _Outfit, int _Direction, int *ColorData, const TRGBColor *_Highlight)
{
	Direction = _Direction;
	Outfit = _Outfit;
	SetColors(ColorData);
	Highlight = *_Highlight;
}

void TOutfitIcon::Draw(int Surface, int x, int y)
{
	const int Size = Width;
	PATELLA_ASSERT_VALUES(GetWidth()==GetHeight(), GetWidth()-GetHeight());
	DrawOutfit(Surface, x, y, Size, Outfit, Colors, Highlight, TRect(x, y, Size, Height), SkinProperties().ObjectCountFont);
}

void TOutfitIcon::SetColor(int Mask, int Color)
{
	PATELLA_ASSERT_VALUES(In(Mask,1,4), Mask);
	Colors[Mask - 1] = Color;
}

void TOutfitIcon::SetColors(const int *ColorData)
{
	PATELLA_ASSERT(ColorData!=nullptr);
	Colors[0] = ColorData[0];
	Colors[1] = ColorData[1];
	Colors[2] = ColorData[2];
	Colors[3] = ColorData[3];
}

// --- TSkinIcon --------------------------------------------------------------------

TSkinIcon::TSkinIcon(int _Skin) : Skin(_Skin)
{
	TComponent::SetSize(Skins()[Skin].width, Skins()[Skin].height);
}

void TSkinIcon::Draw(int Surface, int x, int y)
{
	PATELLA_ASSERT_VALUES(x>=0, x);
	PATELLA_ASSERT_VALUES(y>=0, y);
	DrawSkin(Surface, x, y, Width, Height, Skin, 0, 0);
}

// --- TObjectIcon ------------------------------------------------------------------

// The frame only narrows the clipping area: the object is drawn at the
// full size.
void TObjectIcon::Draw(int Surface, int x, int y)
{
	int ClipX = x;
	int ClipY = y;
	int ClipWidth = Width;
	int ClipHeight = Height;
	if (Framed) {
		PATELLA_ASSERT_VALUES(x>=0, x);
		PATELLA_ASSERT_VALUES(y>=0, y);
		DrawSkin(Surface, x, y, ClipWidth, ClipHeight, SKIN_SLOT_FRAME, 0, 0);
		ClipX = x + SkinProperties().ObjectIconFrameWidth;
		ClipY = y + SkinProperties().ObjectIconFrameWidth;
		ClipWidth -= 2 * SkinProperties().ObjectIconFrameWidth;
		ClipHeight -= 2 * SkinProperties().ObjectIconFrameWidth;
	}
	PATELLA_ASSERT_VALUES(GetWidth()==GetHeight(), GetWidth()-GetHeight());
	DrawObject(Surface, x, y, Width, Obj, Highlight, TRect(ClipX, ClipY, ClipWidth, ClipHeight),
		SkinProperties().ObjectCountFont, false);
}

// --- TSkinButton ------------------------------------------------------------------

TSkinButton::TSkinButton(int x, int y, int _ButtonUp, int _ButtonDown, TCallback _Callback, const char *_Tooltip,
	bool _Repeat) : Callback(nullptr, 0)
{
	TComponent::SetPosition(x, y);
	TComponent::SetSize(Skins()[_ButtonUp].width, Skins()[_ButtonUp].height);
	SetSkins(_ButtonUp, _ButtonDown);
	Callback = _Callback;
	Tooltip = _Tooltip;
	Repeat = _Repeat;
	Pressed = false;
	MouseInside = false;
	RadioButtonGroup = nullptr;
	Enabled = true;
}

void TSkinButton::Draw(int Surface, int x, int y)
{
	PATELLA_ASSERT_VALUES(x>=0, x);
	PATELLA_ASSERT_VALUES(y>=0, y);
	DrawSkin(Surface, x, y, Width, Height, IsPressed() ? ButtonDown : ButtonUp, 0, 0);
}

bool TSkinButton::MouseMove(int x, int y)
{
	MouseInside = x >= 0 && x <= Width - 1 && y >= 0 && y <= Height - 1;
	return true;
}

bool TSkinButton::LeftButtonDown(int x, int y, [[maybe_unused]] int Flags)
{
	if (Enabled) {
		Pressed = true;
		MouseInside = x >= 0 && x <= Width - 1 && y >= 0 && y <= Height - 1;
		SetCapture(nullptr);
	}
	return true;
}

bool TSkinButton::LeftButtonUp(int x, int y, [[maybe_unused]] int Flags)
{
	if (Pressed) {
		ReleaseCapture(nullptr);
		Pressed = false;
		MouseInside = x >= 0 && x <= Width - 1 && y >= 0 && y <= Height - 1;
		if (MouseInside) {
			if (RadioButtonGroup != nullptr)
				RadioButtonGroup->Selected = this;
			Callback.Send();
		}
	}
	return true;
}

// Code 6 comes while the left button is held.
bool TSkinButton::MouseWheel([[maybe_unused]] int x, [[maybe_unused]] int y, int Code)
{
	if (Code != 6)
		return false;
	if (Repeat && Pressed && MouseInside)
		Callback.Send();
	return true;
}

bool TSkinButton::IsPressed()
{
	if (RadioButtonGroup != nullptr && RadioButtonGroup->Selected == this)
		return true;
	return Pressed && MouseInside;
}

// The assertions print the skin only (the binary's messages,
// issue #34).
void TSkinButton::SetSkins(int _ButtonUp, int _ButtonDown)
{
	PATELLA_ASSERT_VALUES(_ButtonUp<patella::bridge::Skins().NumberOfSkins, _ButtonUp);
	PATELLA_ASSERT_VALUES(_ButtonDown<patella::bridge::Skins().NumberOfSkins, _ButtonDown);
	ButtonDown = _ButtonDown;
	ButtonUp = _ButtonUp;
}

void TSkinButton::SetRadioButtonGroup(TRadioButtonGroup *Group)
{
	PATELLA_ASSERT(RadioButtonGroup==nullptr);
	RadioButtonGroup = Group;
	Group->Buttons(Group->Count) = this;
	Group->Count++;
}

// --- The other buttons ------------------------------------------------------------

TColorButton::TColorButton(int x, int y, TRGBColor _Color, int _ButtonUp, int _ButtonDown, TCallback _Callback,
	const char *_Tooltip) : TSkinButton(x, y, _ButtonUp, _ButtonDown, _Callback, _Tooltip, false)
{
	Color = _Color;
}

void TColorButton::Draw(int Surface, int x, int y)
{
	GetSurface()->FillRectangle(Surface, TRect(x, y, Width, Height), Color);
	TSkinButton::Draw(Surface, x, y);
}

TIconButton::TIconButton(int x, int y, int _Icon, int _ButtonUp, int _ButtonDown, TCallback _Callback,
	const char *_Tooltip) : TSkinButton(x, y, _ButtonUp, _ButtonDown, _Callback, _Tooltip, false)
{
	Icon = _Icon;
}

void TIconButton::Draw(int Surface, int x, int y)
{
	TSkinButton::Draw(Surface, x, y);
	PATELLA_ASSERT_VALUES(x+SkinProperties().Buttons.Texture.x>=0, x);
	PATELLA_ASSERT_VALUES(y+SkinProperties().Buttons.Texture.y>=0, y);
	DrawSkin(Surface, x + SkinProperties().Buttons.Texture.x, y + SkinProperties().Buttons.Texture.y, Skins()[Icon].width,
		Skins()[Icon].height, Icon, 0, 0);
}

TBlinkingButton::TBlinkingButton(int x, int y, int _ButtonUp1, int _ButtonDown1, int _ButtonUp2, int _ButtonDown2,
	TCallback _Callback, const char *_Tooltip)
	: TSkinButton(x, y, _ButtonUp1, _ButtonDown1, _Callback, _Tooltip, false)
{
	ButtonUp1 = _ButtonUp1;
	ButtonDown1 = _ButtonDown1;
	ButtonUp2 = _ButtonUp2;
	ButtonDown2 = _ButtonDown2;
	NextChange = GetMilliseconds() + SkinProperties().BlinkingButtonInterval;
	Phase = 2;
}

void TBlinkingButton::Draw(int Surface, int x, int y)
{
	if (GetMilliseconds() > NextChange) {
		NextChange = GetMilliseconds() + SkinProperties().BlinkingButtonInterval;
		if (Phase == 1) {
			ButtonUp = ButtonUp1;
			ButtonDown = ButtonDown1;
			Phase = 2;
		} else {
			ButtonUp = ButtonUp2;
			ButtonDown = ButtonDown2;
			Phase = 1;
		}
	}
	TSkinButton::Draw(Surface, x, y);
}

TTextButton::TTextButton(int x, int y, const char *_Text, int _ButtonUp, int _ButtonDown, TCallback _Callback,
	const char *_Tooltip) : TSkinButton(x, y, _ButtonUp, _ButtonDown, _Callback, _Tooltip, false)
{
	PATELLA_ASSERT(_Text!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(_Text)<50, strlen(_Text));
	Text.Assign(_Text);
}

TTextButton::TTextButton(int x, int y, const char *_Text, TCallback _Callback, const char *_Tooltip)
	: TSkinButton(x, y, SKIN_BUTTON_UP, SKIN_BUTTON_DOWN, _Callback, _Tooltip, false)
{
	PATELLA_ASSERT(_Text!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(_Text)<50, strlen(_Text));
	Text.Assign(_Text);
}

// The text moves by Buttons.PressedOffset while pressed.
void TTextButton::Draw(int Surface, int x, int y)
{
	TSkinButton::Draw(Surface, x, y);
	const int Offset = IsPressed() ? SkinProperties().Buttons.PressedOffset : 0;
	int TextX, TextY;
	GetTextPosition(&TextX, &TextY, x, y, Width, Height, &SkinProperties().Buttons.Font);
	PrintFormattedText(Surface, TextX + Offset, TextY + Offset,
		Enabled ? SkinProperties().Buttons.Font : SkinProperties().Buttons.DisabledFont, Text);
}

// The base gets the button's own Tooltip, which is not set yet: the original's read of the memory's value, which
// TSkinButton stores back (the golden DialogElements test compares it), and which nothing reads (MouseHover shows
// Tooltip1 or Tooltip2). GCC's and Clang's -Wuninitialized name the read: allowed here (issue #111).
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wuninitialized"
#endif
TToggleButton::TToggleButton(int x, int y, int _ButtonUp, int _ButtonDown, TCallback _Callback,
	const char *_Tooltip1, const char *_Tooltip2, bool _State)
	: TSkinButton(x, y, _ButtonUp, _ButtonDown, _Callback, Tooltip, false)
{
	Tooltip2 = _Tooltip2;
	State = _State;
	Tooltip1 = _Tooltip1;
}

// The release does not check the position again.
bool TToggleButton::LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	if (Pressed) {
		ReleaseCapture(nullptr);
		Pressed = false;
		if (MouseInside) {
			if (RadioButtonGroup != nullptr)
				RadioButtonGroup->Selected = this;
			State = !State;
			Callback.Send();
		}
	}
	return true;
}

bool TToggleButton::IsPressed()
{
	if (RadioButtonGroup != nullptr && RadioButtonGroup->Selected == this)
		return true;
	if (State)
		return true;
	return Pressed && MouseInside;
}

// The base gets the button's own Tooltip, not set yet, as TToggleButton's (the warning allowed for both,
// above).
TTextToggleButton::TTextToggleButton(int x, int y, const char *_Text, int _ButtonUp, int _ButtonDown,
	TCallback _Callback, const char *_Tooltip1, const char *_Tooltip2, bool _State)
	: TTextButton(x, y, _Text, _ButtonUp, _ButtonDown, _Callback, Tooltip)
{
	Tooltip1 = _Tooltip1;
	State = _State;
	Tooltip2 = _Tooltip2;
}
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

bool TTextToggleButton::LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	if (Pressed) {
		ReleaseCapture(nullptr);
		Pressed = false;
		if (MouseInside) {
			if (RadioButtonGroup != nullptr)
				RadioButtonGroup->Selected = this;
			State = !State;
			Callback.Send();
		}
	}
	return true;
}

bool TTextToggleButton::IsPressed()
{
	if (RadioButtonGroup != nullptr && RadioButtonGroup->Selected == this)
		return true;
	if (State)
		return true;
	return Pressed && MouseInside;
}

// A varargs constructor (__cdecl): after the first state, each further
// state passes ButtonUp, ButtonDown and a TCallback.
TMultiStateButton::TMultiStateButton(int x, int y, const char *_Tooltip, int _States, int _ButtonUp,
	int _ButtonDown, TCallback _Callback, ...)
	: TSkinButton(x, y, _ButtonUp, _ButtonDown, _Callback, _Tooltip, false)
{
	State = 0;
	States = _States;
	ButtonUps.reset(new int[_States]);
	ButtonDowns.reset(new int[States]);
	Callbacks.reset(new TCallback[States]);
	ButtonUps[0] = _ButtonUp;
	ButtonDowns[0] = _ButtonDown;
	Callbacks[0] = _Callback;
	va_list Arguments;
	va_start(Arguments, _Callback);
	for (int i = 1; i < States; i++) {
		ButtonUps[i] = va_arg(Arguments, int);
		ButtonDowns[i] = va_arg(Arguments, int);
		// TCallback is trivially copyable (passed through ... as the caller's copy), but not a POD (its constructors),
		// which Clang's va_arg refuses by default (-Wnon-pod-varargs, issue #111).
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wnon-pod-varargs"
#endif
		Callbacks[i] = va_arg(Arguments, TCallback);
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
	}
	va_end(Arguments);
}

// Sends the callback of the state it leaves.
bool TMultiStateButton::LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	if (Pressed) {
		ReleaseCapture(nullptr);
		Pressed = false;
		if (MouseInside) {
			if (RadioButtonGroup != nullptr)
				RadioButtonGroup->Selected = this;
			Callbacks[State].Send();
			State++;
			if (State >= States)
				State = 0;
			ButtonUp = ButtonUps[State];
			ButtonDown = ButtonDowns[State];
		}
	}
	return true;
}

// --- TCheckBox --------------------------------------------------------------------

TCheckBox::TCheckBox(const char *_Text)
{
	PATELLA_ASSERT(_Text!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(_Text)<50, strlen(_Text));
	DrawingListLength = 4;
	DrawingList.reset(new TDrawing[4]);
	Checked = false;
	Enabled = true;
	Label = new TLabel(SkinProperties().CheckBox.Font, _Text);
	TWindow::InsertChild(Label);
	TWindow::FitToChild(Label, SkinProperties().CheckBox.Label, true);
	TWindow::SetChildArea(Label, SkinProperties().CheckBox.Label);
}

// Skin 150 checked, 151 unchecked or disabled.
void TCheckBox::Draw(int Surface, int x, int y)
{
	TWindow::Draw(Surface, x, y);
	int Skin = SKIN_CHECKBOX_UNCHECKED - (IsChecked() ? 1 : 0);
	if (!Enabled)
		Skin = SKIN_CHECKBOX_UNCHECKED;
	PATELLA_ASSERT_VALUES(x+SkinProperties().CheckBox.Item.x>=0, x);
	PATELLA_ASSERT_VALUES(y+SkinProperties().CheckBox.Item.y>=0, y);
	DrawSkin(Surface, x + SkinProperties().CheckBox.Item.x, y + SkinProperties().CheckBox.Item.y, Skins()[Skin].width,
		Skins()[Skin].height, Skin, 0, 0);
}

void TCheckBox::Arrange()
{
	DrawingList[0] = TDrawing(SKIN_CHECKBOX_EDGE_TOP, 0, 0, Width, Skins()[SKIN_CHECKBOX_EDGE_TOP].height, 0, 0);
	DrawingList[1] = TDrawing(SKIN_CHECKBOX_EDGE_LEFT, 0, Skins()[SKIN_CHECKBOX_EDGE_TOP].height, Skins()[SKIN_CHECKBOX_EDGE_LEFT].width, Height - Skins()[SKIN_CHECKBOX_EDGE_TOP].height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_CHECKBOX_EDGE_RIGHT, Width - Skins()[SKIN_CHECKBOX_EDGE_RIGHT].width, Skins()[SKIN_CHECKBOX_EDGE_TOP].height, Skins()[SKIN_CHECKBOX_EDGE_RIGHT].width,
		Height - Skins()[SKIN_CHECKBOX_EDGE_TOP].height, 0, 0);
	DrawingList[3] = TDrawing(SKIN_CHECKBOX_EDGE_BOTTOM, Skins()[SKIN_CHECKBOX_EDGE_LEFT].width, Height - Skins()[SKIN_CHECKBOX_EDGE_BOTTOM].height,
		Width - Skins()[SKIN_CHECKBOX_EDGE_RIGHT].width - Skins()[SKIN_CHECKBOX_EDGE_LEFT].width, Skins()[SKIN_CHECKBOX_EDGE_BOTTOM].height, 0, 0);
}

// Replaces the label by one in the enabled or disabled font (with only the
// first line of its text).
void TCheckBox::SetEnabled(bool _Enabled)
{
	Enabled = _Enabled;
	const TFont Font = _Enabled ? SkinProperties().CheckBox.Font : SkinProperties().CheckBox.DisabledFont;
	TLabel *NewLabel = new TLabel(Label->OffsetX, Label->OffsetY, Label->Width, Label->Height, Font, Label->Text.get(),
		false);
	DeleteChild(Label);
	Label = NewLabel;
	InsertChild(NewLabel);
}

// --- TEditLine --------------------------------------------------------------------

// A read-only line uses the second font.
TEditLine::TEditLine(TRect Area, int _MaxLength, bool _ReadOnly)
{
	PATELLA_ASSERT_VALUES(_MaxLength>=0, _MaxLength);
	Text.reset(new char[_MaxLength + 1]);
	Text[0] = 0;
	ReadOnly = _ReadOnly;
	Cursor = 0;
	FirstVisible = 0;
	SelectionStart = -1;
	SelectionLength = 0;
	Selecting = false;
	Active = false;
	SingleLine = true;
	Disabled = false;
	FocusGroup = nullptr;
	Font = _ReadOnly ? SkinProperties().EditLine.ReadOnlyFont : SkinProperties().EditLine.Font;
	MaxLength = _MaxLength;
	DrawingListLength = 4;
	DrawingList.reset(new TDrawing[4]);
	TWindow::SetPosition(Area.x, Area.y);
	TEditLine::SetSize(Area.width, Area.height);
	TEditLine::Arrange();
}

// Without an area (the dialogs size it later).
TEditLine::TEditLine(int _MaxLength, bool _ReadOnly)
{
	PATELLA_ASSERT_VALUES(_MaxLength>=0, _MaxLength);
	Text.reset(new char[_MaxLength + 1]);
	Text[0] = 0;
	ReadOnly = _ReadOnly;
	Cursor = 0;
	FirstVisible = 0;
	SelectionStart = -1;
	SelectionLength = 0;
	Selecting = false;
	Active = false;
	SingleLine = true;
	Disabled = false;
	FocusGroup = nullptr;
	Font = _ReadOnly ? SkinProperties().EditLine.ReadOnlyFont : SkinProperties().EditLine.Font;
	MaxLength = _MaxLength;
	DrawingListLength = 4;
	DrawingList.reset(new TDrawing[4]);
}

// Scrolls the text so that the cursor is visible; the cursor blinks every
// five action numbers.
void TEditLine::Draw(int Surface, int x, int y)
{
	GetSurface()->FillRectangle(Surface, TRect(x, y, Width, Height),
		IsDisabled() ? SkinProperties().EditLine.DisabledBackground : SkinProperties().EditLine.Background);
	TWindow::Draw(Surface, x, y);
	if (!IsDisabled()) {
		if (Cursor < FirstVisible)
			FirstVisible = Cursor;
		while (Cursor > FirstVisible
				+ GetFittingTextLength(Font.FontNumber, Text.get() + FirstVisible, Width - 2 * SkinProperties().EditLineTextMargin))
			FirstVisible++;
		const int TextX = x + SkinProperties().EditLineTextMargin;
		const int TextY = y + SkinProperties().EditLineTextMargin;
		const int Visible = GetFittingTextLength(Font.FontNumber, Text.get() + FirstVisible, Width - 2 * SkinProperties().EditLineTextMargin);
		if (SelectionStart != -1 && SelectionLength != 0) {
			const int Low = SelectionLength < 0 ? SelectionStart + SelectionLength : SelectionStart;
			const int High = SelectionLength < 0 ? SelectionStart : SelectionStart + SelectionLength;
			const int Begin = Low < FirstVisible ? FirstVisible : Low;
			const int End = High < FirstVisible + Visible ? High : FirstVisible + Visible;
			const int Offset = GetTextPartWidth(Font.FontNumber, Text.get() + FirstVisible, Begin - FirstVisible);
			const int SelectionWidth = GetTextPartWidth(Font.FontNumber, Text.get() + Begin, End - Begin);
			GetSurface()->FillRectangle(Surface,
				TRect(TextX + Offset, TextY, SelectionWidth,
					GetFontHeight(Font.FontNumber)),
				SkinProperties().EditLine.SelectionColor);
		}
		PrintFormattedTextPart(Surface, TextX, TextY, Font, Text.get() + FirstVisible, Visible);
		if (Active && GetActionNumber() / 5 % 2 == 0)
			GetSurface()->DrawVerticalLine(Surface,
				TextX + GetTextPartWidth(Font.FontNumber, Text.get() + FirstVisible, Cursor - FirstVisible), TextY, GetFontHeight(Font.FontNumber), SkinProperties().EditLine.CursorColor);
	}
}

void TEditLine::SetSize(int Width, int Height)
{
	TComponent::SetSize(Width, Height);
	Arrange();
}

// The frame of skins 8-11.
void TEditLine::Arrange()
{
	DrawingList[0] = TDrawing(SKIN_FRAME_EDGE_TOP, 0, 0, Width, Skins()[SKIN_FRAME_EDGE_TOP].height, 0, 0);
	DrawingList[1] = TDrawing(SKIN_FRAME_EDGE_LEFT, 0, Skins()[SKIN_FRAME_EDGE_TOP].height, Skins()[SKIN_FRAME_EDGE_LEFT].width, Height - Skins()[SKIN_FRAME_EDGE_TOP].height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_FRAME_EDGE_RIGHT, Width - Skins()[SKIN_FRAME_EDGE_RIGHT].width, Skins()[SKIN_FRAME_EDGE_TOP].height, Skins()[SKIN_FRAME_EDGE_RIGHT].width,
		Height - Skins()[SKIN_FRAME_EDGE_TOP].height, 0, 0);
	DrawingList[3] = TDrawing(SKIN_FRAME_EDGE_BOTTOM, Skins()[SKIN_FRAME_EDGE_LEFT].width, Height - Skins()[SKIN_FRAME_EDGE_BOTTOM].height,
		Width - Skins()[SKIN_FRAME_EDGE_RIGHT].width - Skins()[SKIN_FRAME_EDGE_LEFT].width, Skins()[SKIN_FRAME_EDGE_BOTTOM].height, 0, 0);
}

// A drag extends the selection.
bool TEditLine::MouseMove(int x, int y)
{
	TWindow::MouseMove(x, y);
	if (Selecting) {
		const int Position = GetPosition(x, y);
		int End = Position;
		Limit(&End, 0, (int)strlen(Text.get()));
		SelectionLength = End - SelectionStart;
		SetCursor(Position);
	}
	return true;
}

bool TEditLine::LeftButtonDown(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonDown(x, y, Flags) && !IsDisabled()) {
		if (FocusGroup == nullptr)
			SetActive(true);
		else
			FocusGroup->SetActive(this);
		const int Position = GetPosition(x, y);
		SetSelection(Position);
		SetCursor(Position);
		Selecting = true;
		SetCapture(nullptr);
	}
	return true;
}

bool TEditLine::LeftButtonUp(int x, int y, int Flags)
{
	TWindow::LeftButtonUp(x, y, Flags);
	if (Selecting) {
		ReleaseCapture(nullptr);
		Selecting = false;
	}
	return true;
}

// Selects the word under the mouse.
bool TEditLine::LeftButtonDoubleClick(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonDoubleClick(x, y, Flags) && !IsDisabled()) {
		int Start;
		for (Start = GetPosition(x, y); Start > 0; Start--) {
			if (IsSpace(Text[Start - 1]))
				break;
		}
		int Length = 0;
		while (!IsSpace(Text[Start + Length]) && Start + Length < (int)strlen(Text.get()))
			Length++;
		SetSelection(Start);
		int End = Start + Length;
		Limit(&End, 0, (int)strlen(Text.get()));
		SelectionLength = End - SelectionStart;
		SetCursor(Start + Length);
	}
	return true;
}

// Opens the edit line's context menu (ContextMenus.cpp) unless a dialog or
// another menu is open. The base class gets the event as a left button
// release (the binary calls TWindow::LeftButtonUp).
bool TEditLine::RightButtonUp(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonUp(x, y, Flags) && !IsDisabled() && GetActionState() != ACTIONSTATE_DIALOG
		&& GetActionState() != ACTIONSTATE_CONTEXT_MENU)
		AddMessage(MESSAGE_EDIT_MENU, reinterpret_cast<std::intptr_t>(this), 0);
	return true;
}

// Tab and Shift+Tab move through the focus group; the cursor keys, Home and
// End move the cursor; Ctrl/Shift+Insert, Ctrl+C/V/X, Shift+Delete and
// Ctrl+A are the clipboard events; Backspace, Delete and printable
// characters edit.
void TEditLine::KeyPress(int Key)
{
	if (FocusGroup != nullptr) {
		if (Key == KEY_TAB)
			FocusGroup->Next();
		if (Key == (KEY_SHIFT | KEY_TAB))
			FocusGroup->Previous();
	}
	if (!IsDisabled()) {
		const int Code = Key & KEY_CODE_MASK;
		if (Code == KEY_LEFT && Cursor > 0) {
			SetCursor(Cursor - 1);
			SelectionLength = 0;
		}
		if (Code == KEY_RIGHT && Cursor < (int)strlen(Text.get())) {
			SetCursor(Cursor + 1);
			SelectionLength = 0;
		}
		if (Code == KEY_HOME) {
			SetCursor(0);
			SelectionLength = 0;
		}
		if (Code == KEY_END) {
			SetCursor((int)strlen(Text.get()));
			SelectionLength = 0;
		}
		if (!ReadOnly) {
			if (Key == (KEY_SHIFT | KEY_INSERT) || Key == (KEY_CONTROL | 'v'))
				HandleEvent(EVENT_PASTE);
			if (Key == (KEY_CONTROL | KEY_INSERT) || Key == (KEY_CONTROL | 'c'))
				HandleEvent(EVENT_COPY);
			if (Key == (KEY_SHIFT | KEY_DELETE) || Key == (KEY_CONTROL | 'x'))
				HandleEvent(EVENT_CUT);
			if (Key == (KEY_CONTROL | KEY_DELETE))
				HandleEvent(EVENT_DELETE);
			if (Key == (KEY_CONTROL | 'a'))
				HandleEvent(EVENT_SELECT_ALL);
			if (Code == KEY_BACKSPACE) {
				if (SelectionStart == -1 || SelectionLength == 0) {
					if (Cursor > 0) {
						SetCursor(Cursor - 1);
						DeleteChar(Cursor);
					}
				} else {
					SetCursor(Cursor - 1);
					HandleEvent(EVENT_DELETE);
				}
			}
			if (Key == KEY_DELETE && Cursor <= (int)strlen(Text.get())) {
				if (SelectionStart == -1 || SelectionLength == 0)
					DeleteChar(Cursor);
				else
					HandleEvent(EVENT_DELETE);
			}
			// Past the modifier test the binary keeps the code in Key's stack slot:
			// the original's catch wrapper then prints the code.
			if ((Key & KEY_CONTROL) == 0 || (Key & KEY_ALT) != 0) {
				Key = Code;
				if (Key > 0x1f && Key < KEY_F1 && (int)strlen(Text.get()) < MaxLength) {
					if (HasSelection())
						HandleEvent(EVENT_DELETE);
					InsertChar(Cursor, (char)Key);
					SetCursor(Cursor + 1);
				}
			}
		}
	}
}

// 16 copy, 17 cut, 18 paste (a single line stops at the first line break),
// 19 delete the selection, 20 select all.
void TEditLine::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_COPY:
		if (SelectionStart != -1 && SelectionLength != 0) {
			const int End = SelectionLength < 0 ? SelectionStart : SelectionStart + SelectionLength;
			const int Begin = SelectionLength < 0 ? SelectionStart + SelectionLength : SelectionStart;
			CheckClipboardText(patella::TSpan<const char>(Text.get() + Begin, static_cast<std::size_t>(End - Begin)));
		}
		return;
	case EVENT_CUT: {
		if (ReadOnly || !HasSelection())
			return;
		int Begin = SelectionLength < 0 ? SelectionStart + SelectionLength : SelectionStart;
		int End = SelectionLength < 0 ? SelectionStart : SelectionStart + SelectionLength;
		CheckClipboardText(patella::TSpan<const char>(Text.get() + Begin, static_cast<std::size_t>(End - Begin)));
		SetCursor(SelectionLength < 0 ? SelectionStart + SelectionLength : SelectionStart);
		Begin = SelectionLength < 0 ? SelectionStart + SelectionLength : SelectionStart;
		End = SelectionLength < 0 ? SelectionStart : SelectionStart + SelectionLength;
		DeleteText(Begin, End - Begin);
		SelectionLength = 0;
		return;
	}
	case EVENT_PASTE: {
		if (ReadOnly || IsClipboardEmpty())
			return;
		const char *Clipboard = GetClipboardText();
		if (Clipboard == nullptr)
			return;
		if (HasSelection()) {
			SetCursor(SelectionLength < 0 ? SelectionStart + SelectionLength : SelectionStart);
			const int Begin = SelectionLength < 0 ? SelectionStart + SelectionLength : SelectionStart;
			const int End = SelectionLength < 0 ? SelectionStart : SelectionStart + SelectionLength;
			DeleteText(Begin, End - Begin);
			SelectionLength = 0;
		}
		if (!SingleLine)
			SetCursor(Cursor + InsertText(Cursor, Clipboard, (int)strlen(Clipboard)));
		else
			SetCursor(Cursor + InsertText(Cursor, Clipboard, (int)strcspn(Clipboard, "\n")));
		return;
	}
	case EVENT_DELETE: {
		if (ReadOnly || !HasSelection())
			return;
		SetCursor(SelectionLength < 0 ? SelectionStart + SelectionLength : SelectionStart);
		const int Begin = SelectionLength < 0 ? SelectionStart + SelectionLength : SelectionStart;
		const int End = SelectionLength < 0 ? SelectionStart : SelectionStart + SelectionLength;
		DeleteText(Begin, End - Begin);
		SelectionLength = 0;
		return;
	}
	case EVENT_SELECT_ALL:
		// The cursor goes to max(length, 0) (the binary keeps the test).
		SelectionStart = 0;
		SelectionLength = (int)strlen(Text.get());
		SetCursor(SelectionLength < 0 ? 0 : SelectionLength);
		return;
	default:
		TERROR1("unknown event", Event);
	}
}

// Losing the input clears the selection.
void TEditLine::SetActive(bool _Active)
{
	Active = _Active;
	if (!_Active)
		SelectionLength = 0;
}

void TEditLine::SetFocusGroup(TFocusGroup *_FocusGroup)
{
	PATELLA_ASSERT(_FocusGroup!=nullptr);
	FocusGroup = _FocusGroup;
	_FocusGroup->AddMember(this);
}

// Cut to MaxLength.
void TEditLine::SetText(const char *_Text)
{
	PATELLA_ASSERT(_Text!=nullptr);
	strncpy(Text.get(), _Text, MaxLength);
	Text[MaxLength] = 0;
	SetCursor((int)strlen(Text.get()));
	FirstVisible = 0;
	SelectionLength = 0;
	Arrange();
}

// Inserts at the cursor (Position only enters the assertion and the length
// of the rest); text that does not fit is cut, first the rest after the
// cursor. Returns the inserted length.
int TEditLine::InsertText(int Position, const char *String, int insertlength)
{
	PATELLA_ASSERT_VALUES(Position <= (int)strlen(Text.get()), Position, strlen(Text.get()));
	int appendlength = (int)strlen(Text.get()) - Position;
	if ((int)strlen(Text.get()) + insertlength <= MaxLength) {
		PATELLA_ASSERT_VALUES(Cursor+insertlength+appendlength<=MaxLength, Cursor+insertlength+appendlength);
		memmove(Text.get() + Cursor + insertlength, Text.get() + Cursor, appendlength + 1);
		memcpy(Text.get() + Cursor, String, insertlength);
		LimitCursor();
		SelectionLength = 0;
		Arrange();
		return insertlength;
	}
	if (Cursor + insertlength > MaxLength)
		insertlength = MaxLength - Cursor;
	if (Cursor + insertlength + appendlength > MaxLength)
		appendlength = MaxLength - Cursor - insertlength;
	PATELLA_ASSERT_VALUES(Cursor+insertlength+appendlength<=MaxLength, Cursor+insertlength+appendlength);
	memmove(Text.get() + Cursor + insertlength, Text.get() + Cursor, appendlength);
	memcpy(Text.get() + Cursor, String, insertlength);
	Text[Cursor + insertlength + appendlength] = 0;
	LimitCursor();
	SelectionLength = 0;
	Arrange();
	return insertlength;
}

void TEditLine::InsertChar(int Position, char c)
{
	PATELLA_ASSERT_VALUES(Position<=(int)strlen(Text.get()), Position, strlen(Text.get()));
	TFixedString<2> Help;
	Help.Format("{}", c);
	Help[1] = 0;
	InsertText(Position, Help, 1);
	Arrange();
}

void TEditLine::DeleteText(int start, int length)
{
	const int Length = (int)strlen(Text.get());
	if (start >= Length)
		start = Length;
	if (start + length > Length)
		length = Length - start;
	if (length != 0) {
		memmove(Text.get() + start, Text.get() + start + length, Length - start - length + 1);
		LimitCursor();
		SelectionLength = 0;
		Arrange();
	}
}

void TEditLine::Clear()
{
	Text[0] = 0;
	SetCursor(0);
	FirstVisible = 0;
	SelectionLength = 0;
	Arrange();
}

// --- TPasswordEditLine ------------------------------------------------------------

TPasswordEditLine::TPasswordEditLine(int _MaxLength) : TEditLine(_MaxLength, false)
{
	Stars.reset(new char[_MaxLength + 1]);
	Stars[0] = 0;
}

// Draws a star per character in place of the text.
void TPasswordEditLine::Draw(int Surface, int x, int y)
{
	int i;
	for (i = 0; i < (int)strlen(Text.get()); i++)
		Stars[i] = '*';
	Stars[strlen(Text.get())] = 0;
	// The original swapped the two pointers through a local (issue #81: the
	// owners swap their blocks).
	Text.swap(Stars);
	TEditLine::Draw(Surface, x, y);
	Text.swap(Stars);
}

// Only pasting; the catch blocks name Event with the constant 18.
void TPasswordEditLine::HandleEvent(int Event)
{
	if (Event == EVENT_PASTE) {
		TEditLine::HandleEvent(EVENT_PASTE);
	}
}

// --- TEditText --------------------------------------------------------------------

TEditText::TEditText(int MaxLength, bool _ReadOnly) : TEditLine(MaxLength, _ReadOnly), LineStart(0, 9, 10, 0)
{
	ScrollBar = new TScrollBar(TRect(0, 0, SkinProperties().ScrollBarWidth, SkinProperties().ScrollBarMinimumHeight), 0, 0, 0, 0, 1);
	TWindow::InsertChild(ScrollBar);
	Lines = 0;
	SingleLine = false;
}

// The visible lines from the scroll position on, with the selection; the
// cursor shows in its line (or after the end).
void TEditText::Draw(int Surface, int x, int y)
{
	GetSurface()->FillRectangle(Surface, TRect(x, y, Width, Height), SkinProperties().EditLine.Background);
	// NOLINTNEXTLINE(bugprone-parent-virtual-call): the original's call of the base's base (the image's)
	TWindow::Draw(Surface, x, y);
	if (!IsDisabled()) {
		const int LineHeight = GetFontLineHeight(Font.FontNumber);
		const int TextX = x + SkinProperties().EditLineTextMargin;
		const int TextY = y + SkinProperties().EditLineTextMargin;
		const int First = ScrollBar->Range->Value;
		for (int i = 0; i < VisibleLines; i++) {
			const int Line = i + First;
			if (Line >= Lines)
				break;
			const int Start = LineStart(Line);
			const int End = LineStart(Line + 1);
			if (SelectionStart != -1 && SelectionLength != 0) {
				const int Low = SelectionLength < 0 ? SelectionStart + SelectionLength : SelectionStart;
				const int High = SelectionLength < 0 ? SelectionStart : SelectionStart + SelectionLength;
				const int Begin = Start > Low ? Start : Low;
				const int Stop = High < End ? High : End;
				if (Begin < End && Start < Stop) {
					const int Offset = GetTextPartWidth(Font.FontNumber, Text.get() + Start, Begin - Start);
					const int SelectionWidth = GetTextPartWidth(Font.FontNumber, Text.get() + Begin, Stop - Begin);
					int SelectionHeight = GetFontLineHeight(Font.FontNumber);
					if (High < End)
						SelectionHeight = GetFontHeight(Font.FontNumber);
					GetSurface()->FillRectangle(Surface,
						TRect(TextX + Offset, i * LineHeight + TextY, SelectionWidth,
							SelectionHeight),
						SkinProperties().EditLine.SelectionColor);
				}
			}
			const int LineY = i * LineHeight + TextY;
			PrintFormattedTextPart(Surface, TextX, LineY, Font, Text.get() + Start, End - Start);
			if (Active && !ReadOnly && GetActionNumber() / 5 % 2 == 0) {
				if ((Cursor >= Start && Cursor <= End - 1)
					|| (Line == Lines - 1 && Cursor == (int)strlen(Text.get())))
					GetSurface()->DrawVerticalLine(Surface,
						TextX + GetTextPartWidth(Font.FontNumber, Text.get() + Start, Cursor - Start), LineY, GetFontHeight(Font.FontNumber), SkinProperties().EditLine.CursorColor);
			}
		}
	}
}

// The scroll bar's height subtracts skin widths (11 and 10, not the
// heights of 9 and 8).
void TEditText::Arrange()
{
	const int ScrollBarWidth = ScrollBar->Width;
	SetChildArea(ScrollBar,
		TRect(-(Skins()[SKIN_FRAME_EDGE_RIGHT].width + ScrollBarWidth), Skins()[SKIN_FRAME_EDGE_TOP].height, ScrollBarWidth,
			Height - Skins()[SKIN_FRAME_EDGE_LEFT].width - Skins()[SKIN_FRAME_EDGE_RIGHT].width));
	FieldWidth = Width - 2 * SkinProperties().EditLineTextMargin - ScrollBar->Width;
	FieldHeight = Height - 2 * SkinProperties().EditLineTextMargin;
	VisibleLines = FieldHeight / GetFontLineHeight(Font.FontNumber);
	FormatText();
	if (Lines < VisibleLines)
		ScrollBar->Range->SetRange(0, 0, 0, 0);
	else
		ScrollBar->Range->SetRange(0, Lines - VisibleLines, ScrollBar->Range->Value, VisibleLines);
	TEditLine::Arrange();
}

bool TEditText::MouseWheel(int x, int y, int Code)
{
	if (!TWindow::MouseWheel(x, y, Code) && (Code == 1 || Code == 2))
		return ScrollBar->MouseWheel(x, y, Code);
	return true;
}

// Read-only: the keys scroll (copy and select all still work). Otherwise
// Return inserts a line break and the cursor keys, Home, End, PageUp and
// PageDown move between lines (at the same pixel column); everything else
// is the edit line's. Then the view follows the cursor. PageUp moves one
// line more than a page and reads LineStart(-1) on the first line.
void TEditText::KeyPress(int Key)
{
	if (ReadOnly) {
		int Value = ScrollBar->Range->Value;
		if (Key == (KEY_CONTROL | KEY_INSERT) || Key == (KEY_CONTROL | 'c'))
			HandleEvent(EVENT_COPY);
		if (Key == (KEY_CONTROL | 'a'))
			HandleEvent(EVENT_SELECT_ALL);
		switch (Key & KEY_CODE_MASK) {
		case KEY_UP:
			Value--;
			break;
		case KEY_DOWN:
			Value++;
			break;
		case KEY_PAGE_UP:
			Value -= VisibleLines;
			break;
		case KEY_PAGE_DOWN:
			Value += VisibleLines;
			break;
		default:
			break;
		}
		if (Value < 0)
			Value = 0;
		if (Value > Lines - VisibleLines)
			Value = Lines - VisibleLines;
		ScrollBar->Range->SetValue(Value);
		return;
	}
	if (Key == KEY_RETURN && (int)strlen(Text.get()) < MaxLength) {
		if (SelectionStart != -1 && SelectionLength != 0)
			HandleEvent(EVENT_DELETE);
		InsertChar(Cursor, '\n');
		SetCursor(Cursor + 1);
	}
	const int Code = Key & KEY_CODE_MASK;
	if ((Code != KEY_HOME && Code != KEY_END && Code != KEY_UP && Code != KEY_DOWN && Code != KEY_PAGE_UP && Code != KEY_PAGE_DOWN)
		|| (Key & (KEY_SHIFT | KEY_CONTROL | KEY_ALT)) != 0) {
		TEditLine::KeyPress(Key);
	} else {
		int Line = 0;
		while (Line < Lines - 1 && LineStart(Line + 1) <= Cursor)
			Line++;
		SelectionLength = 0;
		int PixelX, Next, MaxPosition, Fitting, Start;
		switch (Code) {
		case KEY_UP:
			if (Line == 0)
				break;
			PixelX = GetTextPartWidth(Font.FontNumber, Text.get() + LineStart(Line), Cursor - LineStart(Line));
			Next = LineStart(Line);
			Line--;
			MaxPosition = LineStart(Line) <= Next - 1 ? Next - 1 : LineStart(Line);
			Fitting = GetFittingTextLength(Font.FontNumber, Text.get() + LineStart(Line), PixelX);
			Start = LineStart(Line);
			Cursor = MaxPosition <= Start + Fitting ? MaxPosition : Start + Fitting;
			break;
		case KEY_DOWN:
			if (Line == Lines - 1)
				break;
			PixelX = GetTextPartWidth(Font.FontNumber, Text.get() + LineStart(Line), Cursor - LineStart(Line));
			Next = LineStart(Line + 2);
			Line++;
			MaxPosition = LineStart(Line) <= Next - 1 ? Next - 1 : LineStart(Line);
			Fitting = GetFittingTextLength(Font.FontNumber, Text.get() + LineStart(Line), PixelX);
			Start = LineStart(Line);
			Cursor = MaxPosition <= Start + Fitting ? MaxPosition : Start + Fitting;
			break;
		case KEY_HOME:
			Cursor = LineStart(Line);
			break;
		case KEY_END:
			if (Line == Lines - 1) {
				Cursor = (int)strlen(Text.get());
			} else {
				Next = LineStart(Line + 1);
				Cursor = LineStart(Line) <= Next - 1 ? Next - 1 : LineStart(Line);
			}
			break;
		case KEY_PAGE_UP:
			Line -= VisibleLines;
			if (Line < 0) {
				Line = 0;
				Cursor = 0;
			}
			PixelX = GetTextPartWidth(Font.FontNumber, Text.get() + LineStart(Line), Cursor - LineStart(Line));
			Next = LineStart(Line);
			Line--;
			MaxPosition = LineStart(Line) <= Next - 1 ? Next - 1 : LineStart(Line);
			Fitting = GetFittingTextLength(Font.FontNumber, Text.get() + LineStart(Line), PixelX);
			Start = LineStart(Line);
			Cursor = MaxPosition <= Start + Fitting ? MaxPosition : Start + Fitting;
			break;
		case KEY_PAGE_DOWN:
			Line += VisibleLines;
			if (Line >= Lines) {
				Line = Lines - 1;
				Cursor = LineStart(Line);
			}
			PixelX = GetTextPartWidth(Font.FontNumber, Text.get() + LineStart(Line), Cursor - LineStart(Line));
			Next = LineStart(Line + 2);
			Line++;
			MaxPosition = LineStart(Line) <= Next - 1 ? Next - 1 : LineStart(Line);
			Fitting = GetFittingTextLength(Font.FontNumber, Text.get() + LineStart(Line), PixelX);
			Start = LineStart(Line);
			Cursor = MaxPosition <= Start + Fitting ? MaxPosition : Start + Fitting;
			break;
		default:
			break;
		}
	}
	Arrange();
	int Line = 0;
	while (Line < Lines - 1 && LineStart(Line + 1) <= Cursor)
		Line++;
	if (Line < ScrollBar->Range->Value)
		ScrollBar->Range->SetValue(Line);
	if (Line > ScrollBar->Range->Visible - 1 + ScrollBar->Range->Value)
		ScrollBar->Range->SetValue(Line - ScrollBar->Range->Visible + 1);
}

// Unless KeepPosition is set, the cursor and the view go to the end.
void TEditText::SetText(const char *_Text, bool KeepPosition)
{
	PATELLA_ASSERT(_Text!=nullptr);
	TEditLine::SetText(_Text);
	if (!KeepPosition)
		SetCursor((int)strlen(Text.get()));
	SelectionLength = 0;
	Arrange();
	if (!KeepPosition)
		ScrollBar->Range->SetValue(ScrollBar->Range->Max);
}

// A read-only field never takes the input.
void TEditText::SetActive(bool _Active)
{
	if (ReadOnly)
		_Active = false;
	TEditLine::SetActive(_Active);
}

// Left of or above the text: the first visible line's start; right of or
// below it: the end of the last visible line.
int TEditText::GetPosition(int x, int y)
{
	x -= SkinProperties().EditLineTextMargin;
	y -= SkinProperties().EditLineTextMargin;
	int Line;
	if (x < 0 || y < 0) {
		Line = ScrollBar->Range->Value;
	} else if (x <= Width - 2 * SkinProperties().EditLineTextMargin && y <= Height - 2 * SkinProperties().EditLineTextMargin) {
		Line = ScrollBar->Range->Value + y / GetFontLineHeight(Font.FontNumber);
		if (Line > Lines)
			Line = Lines;
		const int Start = LineStart(Line);
		int Length;
		if (Line == Lines)
			Length = (int)strlen(Text.get()) - Start;
		else
			Length = LineStart(Line + 1) - Start;
		const int Fitting = GetFittingTextLength(Font.FontNumber, Text.get() + Start, x);
		if (Fitting <= Length)
			Length = Fitting;
		int Position = Length + Start;
		if (Position > (int)strlen(Text.get()))
			Position = (int)strlen(Text.get());
		Limit(&Position, 0, (int)strlen(Text.get()));
		return Position;
	} else {
		Line = VisibleLines + ScrollBar->Range->Value;
		if (Line == Lines)
			return (int)strlen(Text.get());
	}
	if (Line > Lines)
		Line = Lines - 1;
	return LineStart(Line);
}

// Breaks the text into lines of FieldWidth pixels, after the last character
// that is neither a letter nor a digit when there is one; LineStart(Lines) is
// the text's end.
void TEditText::FormatText()
{
	PATELLA_ASSERT_VALUES(FieldWidth>0, FieldWidth);
	int Position = 0;
	Lines = 0;
	const int Length = (int)strlen(Text.get());
	while (Position <= Length) {
		LineStart(Lines) = Position;
		Lines++;
		const int Fitting = GetFittingTextLength(Font.FontNumber, Text.get() + Position, FieldWidth);
		int Break = -1;
		int i;
		bool Wrapped = false;
		for (i = 0; Text[Position + i] != 0 && Text[Position + i] != '\n'; i++) {
			if (i >= Fitting) {
				if (Break == -1)
					Position += Fitting;
				else
					Position += Break + 1;
				Wrapped = true;
				break;
			}
			if (!IsAlpha(Text[Position + i]) && !IsDigit(Text[Position + i]))
				Break = i;
		}
		if (!Wrapped)
			Position += i + 1;
	}
	LineStart(Lines) = Length;
}

// --- TFocusGroup ------------------------------------------------------------------

// No EH frame.
TFocusGroup::TFocusGroup() : MemberList(0, 2, 2, nullptr)
{
	Members = 0;
	Active = nullptr;
}

void TFocusGroup::SetActive(TEditLine *Member)
{
	if (Active != nullptr)
		Active->SetActive(false);
	Active = Member;
	Active->SetActive(true);
}

// The duplicate test skips the last member.
void TFocusGroup::AddMember(TEditLine *Member)
{
	for (int i = 0; i < Members - 1; i++)
		PATELLA_ASSERT_VALUES(MemberList(i) != Member, i);
	PATELLA_ASSERT(Member!=nullptr);
	MemberList(Members) = Member;
	Members++;
}

void TFocusGroup::Next()
{
	PATELLA_ASSERT(Active!=nullptr);
	int Next = -1;
	for (int i = 0; i < Members; i++) {
		if (MemberList(i) == Active) {
			Next = i + 1;
			break;
		}
	}
	if (Next == Members)
		Next = 0;
	PATELLA_ASSERT(Next != -1);
	SetActive(MemberList(Next));
}

void TFocusGroup::Previous()
{
	PATELLA_ASSERT(Active!=nullptr);
	int Next = -1;
	for (int i = 0; i < Members; i++) {
		if (MemberList(i) == Active) {
			Next = i - 1;
			if (Next == -1)
				Next = Members - 1;
			break;
		}
	}
	PATELLA_ASSERT(Next != -1);
	SetActive(MemberList(Next));
}

// --- THorizontalScrollBarRange ----------------------------------------------------

THorizontalScrollBarRange::THorizontalScrollBarRange(int _Min, int _Max, int _Value, int _Visible,
	TCallback _Callback) : Callback(nullptr, 0)
{
	PATELLA_ASSERT_VALUES(_Min<=_Max, _Min, _Max);
	PATELLA_ASSERT_VALUES(In(_Value,_Min,_Max), _Value, _Max);
	DrawingListLength = 4;
	DrawingList.reset(new TDrawing[4]);
	Min = _Min;
	Value = _Value;
	Enabled = _Min != _Max;
	Max = _Max;
	Callback = _Callback;
	Visible = _Visible;
	Dragging = false;
	PageScroll = false;
	Pressed = false;
	DragOffset = 0;
	SliderPosition = 0;
	SliderWidth = 0;
	ButtonRange = 1;
}

// Not while the slider is dragged.
void THorizontalScrollBarRange::SetValue(int _Value)
{
	if (!Dragging)
		Value = _Value;
	Limit(&Value, Min, Max);
	Callback.Send();
	Arrange();
}

void THorizontalScrollBarRange::SetSize(int Width, int Height)
{
	TComponent::SetSize(Width, Height);
	Arrange();
}

// The slider's share is computed on the x87 stack (double precision here).
void THorizontalScrollBarRange::Arrange()
{
	DrawingList[0] = TDrawing(SKIN_HSCROLLBAR_TRACK, 0, 0, Width, Height, 0, 0);
	if (!Enabled) {
		DrawingList[1] = TDrawing(SKIN_NONE, 0, 0, 0, 0, 0, 0);
		DrawingList[2] = TDrawing(SKIN_NONE, 0, 0, 0, 0, 0, 0);
		DrawingList[3] = TDrawing(SKIN_NONE, 0, 0, 0, 0, 0, 0);
	} else {
		const int MinimumWidth = Skins()[SKIN_HSCROLLBAR_SLIDER_RIGHT].width + Skins()[SKIN_HSCROLLBAR_SLIDER_LEFT].width;
		const double Share = (double)Visible / (double)(Max - Min + Visible);
		SliderWidth = Width;
		if (Share < 1.0)
			SliderWidth = (int)(Width * Share);
		if (SliderWidth < MinimumWidth)
			SliderWidth = MinimumWidth;
		SliderPosition = (Value - Min) * (Width - SliderWidth) / (Max - Min);
		if (SliderPosition + SliderWidth > Width)
			SliderPosition = Width - SliderWidth;
		DrawingList[1] = TDrawing(SKIN_HSCROLLBAR_SLIDER_LEFT, SliderPosition, 0, 0, 0, 0, 0);
		DrawingList[2] = TDrawing(SKIN_HSCROLLBAR_SLIDER_MIDDLE, SliderPosition + Skins()[SKIN_HSCROLLBAR_SLIDER_LEFT].width, 0,
			SliderWidth - Skins()[SKIN_HSCROLLBAR_SLIDER_RIGHT].width - Skins()[SKIN_HSCROLLBAR_SLIDER_LEFT].width, Height, 0, 0);
		DrawingList[3] = TDrawing(SKIN_HSCROLLBAR_SLIDER_RIGHT, SliderPosition - Skins()[SKIN_HSCROLLBAR_SLIDER_RIGHT].width + SliderWidth, 0, 0, 0, 0, 0);
	}
}

bool THorizontalScrollBarRange::MouseMove(int x, [[maybe_unused]] int y)
{
	if (Enabled && Dragging) {
		Value = (x - DragOffset) * (Max - Min + 1) / (Width - SliderWidth + 1) + Min;
		Limit(&Value, Min, Max);
		Callback.Send();
		Arrange();
	}
	return true;
}

bool THorizontalScrollBarRange::LeftButtonDown(int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	if (!Enabled)
		return true;
	Pressed = true;
	SetCapture(nullptr);
	if (SliderPosition <= x && x <= SliderWidth + SliderPosition) {
		DragOffset = x - SliderPosition;
		Dragging = true;
	}
	return true;
}

// A click beside the slider moves by one, or by a page (Visible-1) with
// PageScroll.
bool THorizontalScrollBarRange::LeftButtonUp(int x, int y, [[maybe_unused]] int Flags)
{
	if (Pressed)
		ReleaseCapture(nullptr);
	Pressed = false;
	if (!Enabled)
		return true;
	if (Dragging)
		return true;
	if (x < 0 || x > Width || y < 0 || y > Height)
		return true;
	if (!PageScroll) {
		if (x < SliderPosition)
			Value = Value - 1;
		else if (x > SliderWidth + SliderPosition)
			Value = Value + 1;
	} else {
		if (x < SliderPosition)
			Value = Value + (1 - Visible);
		else if (x > SliderWidth + SliderPosition)
			Value = Value + Visible - 1;
	}
	Limit(&Value, Min, Max);
	Callback.Send();
	Arrange();
	return true;
}

// Codes 1 and 2 are the wheel, 6 the held button (a page step beside the
// slider, then the common tail runs again).
bool THorizontalScrollBarRange::MouseWheel(int x, int y, int Code)
{
	if (!Enabled)
		return true;
	if (Code == 1) {
		Value -= ButtonRange;
	} else if (Code == 2) {
		Value += ButtonRange;
	} else if (Code == 6 && Pressed && !Dragging && In(x, 0, Width) && In(y, 0, Height)) {
		if (!PageScroll) {
			if (x < SliderPosition)
				Value = Value - 1;
			else if (x > SliderWidth + SliderPosition)
				Value = Value + 1;
		} else {
			if (x < SliderPosition)
				Value = Value + (1 - Visible);
			else if (x > SliderWidth + SliderPosition)
				Value = Value + Visible - 1;
		}
		Limit(&Value, Min, Max);
		Callback.Send();
		Arrange();
	}
	Limit(&Value, Min, Max);
	Callback.Send();
	Arrange();
	return true;
}

// The callback goes out before the value is limited.
void THorizontalScrollBarRange::KeyPress(int Key)
{
	if (Enabled) {
		if (Key == KEY_LEFT)
			Value -= ButtonRange;
		else if (Key == KEY_RIGHT)
			Value += ButtonRange;
		Callback.Send();
		Limit(&Value, Min, Max);
		Arrange();
	}
}

// --- THorizontalScrollBar ---------------------------------------------------------

// The catch block prints the minimum and maximum under the names Min and
// Max (the binary's message; the range's assertion says _Min and _Max).
THorizontalScrollBar::THorizontalScrollBar(int Min, int Max, int _Value, int _Visible, int _ButtonRange,
	TCallback _Callback)
{
	PATELLA_ASSERT_VALUES(_ButtonRange>0, _ButtonRange);
	LeftButton = new TSkinButton(0, 0, SKIN_SCROLL_LEFT_BUTTON_UP, SKIN_SCROLL_LEFT_BUTTON_DOWN, TCallback(this, EVENT_SCROLL_LEFT), nullptr, true);
	RightButton = new TSkinButton(0, 0, SKIN_SCROLL_RIGHT_BUTTON_UP, SKIN_SCROLL_RIGHT_BUTTON_DOWN, TCallback(this, EVENT_SCROLL_RIGHT), nullptr, true);
	TWindow::InsertChild(LeftButton);
	TWindow::InsertChild(RightButton);
	Range = new THorizontalScrollBarRange(Min, Max, _Value, _Visible, _Callback);
	Range->PageScroll = true;
	Range->ButtonRange = _ButtonRange;
	TWindow::InsertChild(Range);
}

// The parameters are changed in place (the catch blocks report the
// changed values).
void THorizontalScrollBar::SetSize(int Width, int Height)
{
	Height = SkinProperties().HorizontalScrollBarHeight;
	Width -= RightButton->Width + LeftButton->Width;
	if (Width < Range->GetMinimumWidth())
		Width = Range->GetMinimumWidth();
	Range->SetSize(Width, Height);
	Range->Arrange();
	TComponent::SetSize(Width + RightButton->Width + LeftButton->Width, Height);
	LeftButton->SetPosition(0, 0);
	Range->SetPosition(LeftButton->Width, 0);
	RightButton->SetPosition(Range->Width + LeftButton->Width, 0);
}

bool THorizontalScrollBar::MouseWheel(int x, int y, int Code)
{
	if (TWindow::MouseWheel(x, y, Code))
		return true;
	return Range->MouseWheel(x, y, Code);
}

// The buttons' events.
void THorizontalScrollBar::HandleEvent(int Event)
{
	if (Event == EVENT_SCROLL_LEFT)
		Range->SetValue(Range->Value - Range->ButtonRange);
	else if (Event == EVENT_SCROLL_RIGHT)
		Range->SetValue(Range->Value + Range->ButtonRange);
}

// --- TScrollBarRange --------------------------------------------------------------

TScrollBarRange::TScrollBarRange(int x, int y, int Width, int Height, int _Min, int _Max, int _Value, int _Visible)
{
	PATELLA_ASSERT_VALUES(_Min<=_Max, _Min, _Max);
	PATELLA_ASSERT_VALUES(In(_Value,_Min,_Max), _Value, _Max);
	DrawingListLength = 4;
	DrawingList.reset(new TDrawing[4]);
	Min = _Min;
	Enabled = _Min != _Max;
	Value = _Value;
	Visible = _Visible;
	Max = _Max;
	Dragging = false;
	PageScroll = false;
	Pressed = false;
	DragOffset = 0;
	SliderPosition = 0;
	SliderHeight = 0;
	ButtonRange = 1;
	TComponent::SetPosition(x, y);
	TScrollBarRange::SetSize(Width, Height);
	TScrollBarRange::Arrange();
}

// The value stays while the slider is dragged.
void TScrollBarRange::SetRange(int _Min, int _Max, int _Value, int _Visible)
{
	PATELLA_ASSERT_VALUES(_Min<=_Max, _Min, _Max);
	Enabled = _Min != _Max;
	Min = _Min;
	Max = _Max;
	if (!Dragging)
		Value = _Value;
	if (Value < _Min)
		Value = _Min;
	if (Value > _Max)
		Value = _Max;
	Visible = _Visible;
	Arrange();
}

void TScrollBarRange::SetValue(int _Value)
{
	if (!Dragging)
		Value = _Value;
	Limit(&Value, Min, Max);
	Arrange();
}

void TScrollBarRange::SetSize(int Width, int Height)
{
	TComponent::SetSize(Width, Height);
	Arrange();
}

// The vertical form of THorizontalScrollBarRange::Arrange (skins
// 153-156).
void TScrollBarRange::Arrange()
{
	DrawingList[0] = TDrawing(SKIN_SCROLLBAR_TRACK, 0, 0, Width, Height, 0, 0);
	if (!Enabled) {
		DrawingList[1] = TDrawing(SKIN_NONE, 0, 0, 0, 0, 0, 0);
		DrawingList[2] = TDrawing(SKIN_NONE, 0, 0, 0, 0, 0, 0);
		DrawingList[3] = TDrawing(SKIN_NONE, 0, 0, 0, 0, 0, 0);
	} else {
		const int MinimumHeight = Skins()[SKIN_SCROLLBAR_SLIDER_BOTTOM].height + Skins()[SKIN_SCROLLBAR_SLIDER_TOP].height;
		const double Share = (double)Visible / (double)(Max - Min + Visible);
		SliderHeight = Height;
		if (Share < 1.0)
			SliderHeight = (int)(Height * Share);
		if (SliderHeight < MinimumHeight)
			SliderHeight = MinimumHeight;
		SliderPosition = (Value - Min) * (Height - SliderHeight) / (Max - Min);
		if (SliderPosition + SliderHeight > Height)
			SliderPosition = Height - SliderHeight;
		DrawingList[1] = TDrawing(SKIN_SCROLLBAR_SLIDER_TOP, 0, SliderPosition, 0, 0, 0, 0);
		DrawingList[2] = TDrawing(SKIN_SCROLLBAR_SLIDER_MIDDLE, 0, SliderPosition + Skins()[SKIN_SCROLLBAR_SLIDER_TOP].height, Width,
			SliderHeight - Skins()[SKIN_SCROLLBAR_SLIDER_BOTTOM].height - Skins()[SKIN_SCROLLBAR_SLIDER_TOP].height, 0, 0);
		DrawingList[3] = TDrawing(SKIN_SCROLLBAR_SLIDER_BOTTOM, 0, SliderPosition - Skins()[SKIN_SCROLLBAR_SLIDER_BOTTOM].height + SliderHeight, 0, 0, 0, 0);
	}
}

bool TScrollBarRange::MouseMove([[maybe_unused]] int x, int y)
{
	if (Enabled && Dragging) {
		Value = (y - DragOffset) * (Max - Min + 1) / (Height - SliderHeight + 1) + Min;
		Limit(&Value, Min, Max);
		Arrange();
	}
	return true;
}

bool TScrollBarRange::LeftButtonDown([[maybe_unused]] int x, int y, [[maybe_unused]] int Flags)
{
	if (!Enabled)
		return true;
	Pressed = true;
	SetCapture(nullptr);
	if (SliderPosition <= y && y <= SliderHeight + SliderPosition) {
		DragOffset = y - SliderPosition;
		Dragging = true;
	}
	return true;
}

bool TScrollBarRange::LeftButtonUp(int x, int y, [[maybe_unused]] int Flags)
{
	if (Pressed)
		ReleaseCapture(nullptr);
	Pressed = false;
	if (!Enabled)
		return true;
	if (Dragging)
		return true;
	if (x < 0 || x > Width || y < 0 || y > Height)
		return true;
	if (!PageScroll) {
		if (y < SliderPosition)
			Value = Value - 1;
		else if (y > SliderHeight + SliderPosition)
			Value = Value + 1;
	} else {
		if (y < SliderPosition)
			Value = Value + (1 - Visible);
		else if (y > SliderHeight + SliderPosition)
			Value = Value + Visible - 1;
	}
	Limit(&Value, Min, Max);
	Arrange();
	return true;
}

bool TScrollBarRange::MouseWheel(int x, int y, int Code)
{
	if (!Enabled)
		return true;
	if (Code == 1) {
		Value -= ButtonRange;
	} else if (Code == 2) {
		Value += ButtonRange;
	} else if (Code == 6 && Pressed && !Dragging && In(x, 0, Width) && In(y, 0, Height)) {
		if (!PageScroll) {
			if (y < SliderPosition)
				Value = Value - 1;
			else if (y > SliderHeight + SliderPosition)
				Value = Value + 1;
		} else {
			if (y < SliderPosition)
				Value = Value + (1 - Visible);
			else if (y > SliderHeight + SliderPosition)
				Value = Value + Visible - 1;
		}
		Limit(&Value, Min, Max);
		Arrange();
	}
	Limit(&Value, Min, Max);
	Arrange();
	return true;
}

void TScrollBarRange::KeyPress(int Key)
{
	if (Enabled) {
		if (Key == KEY_UP)
			Value -= ButtonRange;
		else if (Key == KEY_DOWN)
			Value += ButtonRange;
		Limit(&Value, Min, Max);
		Arrange();
	}
}

// --- TScrollBar -------------------------------------------------------------------

// The catch block prints Min and Max, as THorizontalScrollBar's.
TScrollBar::TScrollBar(TRect Area, int Min, int Max, int _Value, int _Visible, int _ButtonRange)
{
	PATELLA_ASSERT_VALUES(_ButtonRange>0, _ButtonRange);
	UpButton = new TSkinButton(0, 0, SKIN_SCROLL_UP_BUTTON_UP, SKIN_SCROLL_UP_BUTTON_DOWN, TCallback(this, EVENT_SCROLL_UP), nullptr, true);
	DownButton = new TSkinButton(0, 0, SKIN_SCROLL_DOWN_BUTTON_UP, SKIN_SCROLL_DOWN_BUTTON_DOWN, TCallback(this, EVENT_SCROLL_DOWN), nullptr, true);
	TWindow::InsertChild(UpButton);
	TWindow::InsertChild(DownButton);
	Range = new TScrollBarRange(0, 0, Area.width, Area.height - DownButton->Height - UpButton->Height, Min, Max,
		_Value, _Visible);
	Range->PageScroll = true;
	Range->ButtonRange = _ButtonRange;
	TWindow::InsertChild(Range);
	TWindow::SetPosition(Area.x, Area.y);
	TScrollBar::SetSize(Area.width, Area.height);
}

// The parameters are changed in place.
void TScrollBar::SetSize(int Width, int Height)
{
	Width = SkinProperties().ScrollBarWidth;
	Height -= DownButton->Height + UpButton->Height;
	if (Height < Range->GetMinimumHeight())
		Height = Range->GetMinimumHeight();
	Range->SetSize(Width, Height);
	Range->Arrange();
	TComponent::SetSize(Width, Height + DownButton->Height + UpButton->Height);
	UpButton->SetPosition(0, 0);
	Range->SetPosition(0, UpButton->Height);
	DownButton->SetPosition(0, Range->Height + UpButton->Height);
}

bool TScrollBar::MouseWheel(int x, int y, int Code)
{
	if (TWindow::MouseWheel(x, y, Code))
		return true;
	return Range->MouseWheel(x, y, Code);
}

// The buttons' events.
void TScrollBar::HandleEvent(int Event)
{
	if (Event == EVENT_SCROLL_UP)
		Range->SetValue(Range->Value - Range->ButtonRange);
	else if (Event == EVENT_SCROLL_DOWN)
		Range->SetValue(Range->Value + Range->ButtonRange);
}

// --- TListBox ---------------------------------------------------------------------

// The edit line is a child only while a line is edited.
TListBox::TListBox(TCallback _SelectCallback, TCallback _ActionCallback)
	: LineText(0, 9, 10, nullptr), SelectCallback(nullptr, 0), ActionCallback(nullptr, 0)
{
	ScrollBar = new TScrollBar(TRect(0, 0, SkinProperties().ScrollBarWidth, SkinProperties().ScrollBarMinimumHeight), 0, 0, 0, 0, 1);
	TWindow::InsertChild(ScrollBar);
	Lines = 0;
	TextWidth = 0;
	Selected = -1;
	DrawingListLength = 4;
	DrawingList.reset(new TDrawing[4]);
	SelectCallback = _SelectCallback;
	ActionCallback = _ActionCallback;
	Editable = false;
	Editing = false;
	EditScroll = 0;
	EditLine = new TEditLine(TRect(0, 0, 32, 16), 30, false);
}

// The loop stops at VisibleLines or Lines lines, not at the last line after
// the scroll position. The edited line is not drawn.
void TListBox::Draw(int Surface, int x, int y)
{
	const int First = ScrollBar->Range->Value;
	if (Editing && First != EditScroll)
		StopEdit();
	GetSurface()->FillRectangle(Surface, TRect(x, y, Width, Height), SkinProperties().ListBox.Background);
	TWindow::Draw(Surface, x, y);
	x += SkinProperties().ListBox.TextMargin;
	y += SkinProperties().ListBox.TextMargin;
	const int LineHeight = GetFontLineHeight(SkinProperties().ListBox.Font.FontNumber);
	for (int i = 0; i < VisibleLines && i < Lines; i++) {
		const int Line = i + First;
		const int Fitting = GetFittingTextLength(SkinProperties().ListBox.Font.FontNumber, LineText(Line), TextWidth);
		if (Line == Selected) {
			if (!Editing) {
				const int Margin = SkinProperties().ListBoxSelectionMargin;
				GetSurface()->FillRectangle(Surface,
					TRect(x - Margin, i * LineHeight - Margin + y, TextWidth + 2 * Margin,
						GetFontHeight(SkinProperties().ListBox.Font.FontNumber) + 2 * Margin),
					SkinProperties().ListBox.SelectedBackground);
				PrintFormattedTextPart(Surface, x, i * LineHeight + y, SkinProperties().ListBox.SelectedFont, LineText(Line),
					Fitting);
			}
		} else {
			PrintFormattedTextPart(Surface, x, i * LineHeight + y, SkinProperties().ListBox.Font, LineText(Line),
				Fitting);
		}
	}
}

void TListBox::SetSize(int Width, int Height)
{
	TComponent::SetSize(Width, Height);
	Arrange();
}

// The scroll bar's height subtracts skin widths, as in TEditText::Arrange.
void TListBox::Arrange()
{
	TextWidth = Width - 2 * SkinProperties().ListBox.TextMargin - ScrollBar->Width;
	SetChildArea(ScrollBar,
		TRect(-(Skins()[SKIN_FRAME_EDGE_RIGHT].width + ScrollBar->Width), Skins()[SKIN_FRAME_EDGE_TOP].height, ScrollBar->Width,
			Height - Skins()[SKIN_FRAME_EDGE_LEFT].width - Skins()[SKIN_FRAME_EDGE_RIGHT].width));
	VisibleLines = (Height - 2 * SkinProperties().ListBox.TextMargin)
		/ GetFontLineHeight(SkinProperties().ListBox.Font.FontNumber);
	if (VisibleLines < Lines)
		ScrollBar->Range->SetRange(0, Lines - VisibleLines, ScrollBar->Range->Value, VisibleLines);
	else
		ScrollBar->Range->SetRange(0, 0, 0, 0);
	DrawingList[0] = TDrawing(SKIN_FRAME_EDGE_TOP, 0, 0, Width, Skins()[SKIN_FRAME_EDGE_TOP].height, 0, 0);
	DrawingList[1] = TDrawing(SKIN_FRAME_EDGE_LEFT, 0, Skins()[SKIN_FRAME_EDGE_TOP].height, Skins()[SKIN_FRAME_EDGE_LEFT].width, Height - Skins()[SKIN_FRAME_EDGE_TOP].height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_FRAME_EDGE_RIGHT, Width - Skins()[SKIN_FRAME_EDGE_RIGHT].width, Skins()[SKIN_FRAME_EDGE_TOP].height, Skins()[SKIN_FRAME_EDGE_RIGHT].width,
		Height - Skins()[SKIN_FRAME_EDGE_TOP].height, 0, 0);
	DrawingList[3] = TDrawing(SKIN_FRAME_EDGE_BOTTOM, Skins()[SKIN_FRAME_EDGE_LEFT].width, Height - Skins()[SKIN_FRAME_EDGE_BOTTOM].height,
		Width - Skins()[SKIN_FRAME_EDGE_RIGHT].width - Skins()[SKIN_FRAME_EDGE_LEFT].width, Skins()[SKIN_FRAME_EDGE_BOTTOM].height, 0, 0);
}

bool TListBox::LeftButtonDown(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonDown(x, y, Flags)) {
		const int Line = GetLineAt(x, y);
		if (Editing && Line != Selected)
			StopEdit();
		if (Line != -1)
			Selected = Line;
		SelectCallback.Send();
	}
	return true;
}

bool TListBox::LeftButtonDoubleClick(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonDoubleClick(x, y, Flags)) {
		const int Line = GetLineAt(x, y);
		if (Editing && Line != Selected)
			StopEdit();
		if (Line != -1 && Editable) {
			Selected = Line;
			StartEdit();
		}
		ActionCallback.Send();
	}
	return true;
}

// The full text of a cut line.
void TListBox::MouseHover(int x, int y)
{
	const int Line = GetLineAt(x, y);
	if (Line != -1) {
		const int Fitting = GetFittingTextLength(SkinProperties().ListBox.Font.FontNumber, LineText(Line), TextWidth);
		if (LineText(Line) != nullptr && Fitting < (int)strlen(LineText(Line)))
			SetMouseOverText(LineText(Line));
	}
}

bool TListBox::MouseWheel(int x, int y, int Code)
{
	if (!TWindow::MouseWheel(x, y, Code) && (Code == 1 || Code == 2))
		return ScrollBar->MouseWheel(x, y, Code);
	return true;
}

// Up and Down move the selection, a letter selects the first line from it
// on in alphabetical order; while editing, Return ends the edit and the
// other keys go to the edit line.
void TListBox::KeyPress(int _Key)
{
	const int Code = _Key & KEY_CODE_MASK;
	if (Code == KEY_UP) {
		if (Editing)
			StopEdit();
		Selected = Selected - 1;
	} else if (Code == KEY_DOWN) {
		if (Editing)
			StopEdit();
		Selected = Selected + 1;
	} else if (!Editing) {
		if ((Code >= 'a' && Code <= 'z') || (Code >= 'A' && Code <= 'Z')) {
			const char c = ToLower(Code);
			for (int i = LineText.min; i <= LineText.max; i++) {
				if (LineText(i) != nullptr && (char)ToLower(*LineText(i)) >= c) {
					Selected = i;
					break;
				}
			}
		}
	} else if (Code == KEY_RETURN) {
		StopEdit();
	} else {
		EditLine->KeyPress(_Key);
	}
	if (Lines < 0)
		Selected = -1;
	else
		Limit(&Selected, 0, Lines - 1);
	if (Selected < ScrollBar->Range->Value)
		ScrollBar->Range->SetValue(Selected);
	if (Selected > ScrollBar->Range->Visible - 1 + ScrollBar->Range->Value)
		ScrollBar->Range->SetValue(Selected - ScrollBar->Range->Visible + 1);
	SelectCallback.Send();
}

void TListBox::AddLine(const char *Text)
{
	PATELLA_ASSERT(Text!=nullptr);
	const std::size_t Size = strlen(Text) + 1;
	LineText(Lines) = new char[Size];
	TStringBuffer(LineText(Lines), Size).Assign(Text);
	Lines++;
	Arrange();
}

void TListBox::SetLine(int Number, const char *Text)
{
	PATELLA_ASSERT(Text!=nullptr);
	PATELLA_ASSERT_VALUES(In(Number,0,Lines-1), Number, Lines);
	if (LineText(Number) != nullptr)
		delete[] LineText(Number);
	const std::size_t Size = strlen(Text) + 1;
	LineText(Number) = new char[Size];
	TStringBuffer(LineText(Number), Size).Assign(Text);
	Arrange();
}

char *TListBox::GetLine(int Number)
{
	PATELLA_ASSERT_VALUES(In(Number,0,Lines-1), Number, Lines);
	return LineText(Number);
}

// Scrolls the selection into view.
void TListBox::SetSelection(int Number)
{
	if (Editing)
		StopEdit();
	if (Lines == 0 || Number == -1) {
		Selected = -1;
	} else {
		PATELLA_ASSERT_VALUES(In(Number,0,Lines-1), Number, Lines);
		Selected = Number;
		if (Number < ScrollBar->Range->Value)
			ScrollBar->Range->SetValue(Number);
		if (Selected > ScrollBar->Range->Visible - 1 + ScrollBar->Range->Value)
			ScrollBar->Range->SetValue(Selected - ScrollBar->Range->Visible + 1);
	}
}

void TListBox::Clear()
{
	StopEdit();
	for (int i = 0; i < Lines; i++) {
		if (LineText(i) != nullptr)
			delete[] LineText(i);
		LineText(i) = nullptr;
	}
	Lines = 0;
	Arrange();
}

// The visible line at y (-1 for none).
int TListBox::GetLineAt([[maybe_unused]] int x, int y) const
{
	const int LineHeight = GetFontLineHeight(SkinProperties().ListBox.Font.FontNumber);
	const int Offset = (LineHeight - GetFontHeight(SkinProperties().ListBox.Font.FontNumber)) / 2;
	if (y >= SkinProperties().ListBox.TextMargin - Offset) {
		const int First = ScrollBar->Range->Value;
		const int Line = (Offset - SkinProperties().ListBox.TextMargin + y) / LineHeight + First;
		if (Line < Lines && Line < VisibleLines + First)
			return Line;
	}
	return -1;
}

void TListBox::StartEdit()
{
	if (!Editing) {
		const int LineHeight = GetFontLineHeight(SkinProperties().ListBox.Font.FontNumber);
		const int ScrollBarWidth = ScrollBar->Width;
		const int ListWidth = Width;
		Editing = true;
		EditLine->SetText(GetLine(Selected));
		InsertChild(EditLine);
		SetChildArea(EditLine,
			TRect(0, (Selected - ScrollBar->Range->Value) * LineHeight,
				ListWidth - ScrollBarWidth, 16));
		EditScroll = ScrollBar->Range->Value;
	}
}

void TListBox::StopEdit()
{
	if (Editing) {
		RemoveChild(EditLine);
		Editing = false;
		SetLine(Selected, EditLine->Text.get());
	}
}

// --- TFontListBox -----------------------------------------------------------------

TFontListBox::TFontListBox(TCallback _SelectCallback, TCallback _ActionCallback)
	: TListBox(_SelectCallback, _ActionCallback), FontLines(0, 9, 10)
{
}

// TListBox::Draw with the lines' own fonts (also for the selection).
void TFontListBox::Draw(int Surface, int x, int y)
{
	const int First = ScrollBar->Range->Value;
	if (Editing && First != EditScroll)
		StopEdit();
	GetSurface()->FillRectangle(Surface, TRect(x, y, Width, Height), SkinProperties().ListBox.Background);
	// NOLINTNEXTLINE(bugprone-parent-virtual-call): the original's call of the base's base (the image's)
	TWindow::Draw(Surface, x, y);
	x += SkinProperties().ListBox.TextMargin;
	y += SkinProperties().ListBox.TextMargin;
	const int LineHeight = GetFontLineHeight(SkinProperties().ListBox.Font.FontNumber);
	for (int i = 0; i < VisibleLines && i < Lines; i++) {
		const int Line = i + First;
		const int Fitting = GetFittingTextLength(FontLines(Line).Font.FontNumber, FontLines(Line).Text, TextWidth);
		if (Line == Selected) {
			if (!Editing) {
				const int Margin = SkinProperties().ListBoxSelectionMargin;
				GetSurface()->FillRectangle(Surface,
					TRect(x - Margin, i * LineHeight - Margin + y, TextWidth + 2 * Margin,
						GetFontHeight(FontLines(Line).Font.FontNumber) + 2 * Margin),
					SkinProperties().ListBox.SelectedBackground);
				PrintFormattedTextPart(Surface, x, i * LineHeight + y, FontLines(Line).Font, FontLines(Line).Text,
					Fitting);
			}
		} else {
			PrintFormattedTextPart(Surface, x, i * LineHeight + y, FontLines(Line).Font, FontLines(Line).Text,
				Fitting);
		}
	}
}

void TFontListBox::AddLine(const char *Text, TFont Font)
{
	PATELLA_ASSERT(Text!=nullptr);
	const std::size_t Size = strlen(Text) + 1;
	FontLines(Lines).Text = new char[Size];
	TStringBuffer(FontLines(Lines).Text, Size).Assign(Text);
	FontLines(Lines).Font = Font;
	Lines++;
	Arrange();
}

// An empty line with room for MaxLength characters; its font stays
// the default.
void TFontListBox::AddLine(int MaxLength)
{
	PATELLA_ASSERT_VALUES(MaxLength>=0, MaxLength);
	FontLines(Lines).Text = new char[MaxLength + 1];
	FontLines(Lines).Text[0] = 0;
	Lines++;
	Arrange();
}

void TFontListBox::SetLine(int Number, const char *Text, TFont Font)
{
	PATELLA_ASSERT(Text!=nullptr);
	PATELLA_ASSERT_VALUES(In(Number,0,Lines-1), Number, Lines-1);
	if (FontLines(Number).Text != nullptr)
		delete[] FontLines(Number).Text;
	const std::size_t Size = strlen(Text) + 1;
	FontLines(Number).Text = new char[Size];
	TStringBuffer(FontLines(Number).Text, Size).Assign(Text);
	FontLines(Number).Font = Font;
	Arrange();
}

// The texts are freed, the pointers kept.
void TFontListBox::Clear()
{
	StopEdit();
	for (int i = 0; i < Lines; i++)
		delete[] FontLines(i).Text;
	Lines = 0;
	Arrange();
}

char *TFontListBox::GetLine(int Number, TFont *Font)
{
	PATELLA_ASSERT_VALUES(In(Number,0,Lines-1), Number, Lines);
	*Font = FontLines(Number).Font;
	return FontLines(Number).Text;
}

void TFontListBox::MouseHover(int x, int y)
{
	const int Line = GetLineAt(x, y);
	if (Line != -1) {
		const int Fitting = GetFittingTextLength(SkinProperties().ListBox.Font.FontNumber, FontLines(Line).Text, TextWidth);
		if (FontLines(Line).Text != nullptr && Fitting < (int)strlen(FontLines(Line).Text))
			SetMouseOverText(FontLines(Line).Text);
	}
}

// The edit starts with the whole text selected.
void TFontListBox::StartEdit()
{
	if (!Editing) {
		const int LineHeight = GetFontLineHeight(SkinProperties().ListBox.Font.FontNumber);
		const int EditWidth = Width - ScrollBar->Width;
		TFont Font;
		const char *Edited = GetLine(Selected, &Font);
		Editing = true;
		EditLine->SetText(Edited);
		InsertChild(EditLine);
		SetChildArea(EditLine, TRect(0, (Selected - ScrollBar->Range->Value) * LineHeight, EditWidth, 16));
		EditLine->SetActive(true);
		EditLine->SetSelection(0);
		int End = (int)strlen(Edited);
		Limit(&End, 0, (int)strlen(EditLine->Text.get()));
		EditLine->SelectionLength = End - EditLine->SelectionStart;
		EditScroll = ScrollBar->Range->Value;
	}
}

// Keeps the line's font.
void TFontListBox::StopEdit()
{
	if (Editing) {
		const char *Text = EditLine->Text.get();
		TFont Font;
		GetLine(Selected, &Font);
		SetLine(Selected, Text, Font);
		Editing = false;
		RemoveChild(EditLine);
	}
}

// --- TProgressBar -----------------------------------------------------------------

// Skins 256-258 are the frame, 259-261 the filled part (start, middle, end).
void TProgressBar::Arrange()
{
	DrawingList[0] = TDrawing(SKIN_PROGRESS_LEFT, 0, 0, Skins()[SKIN_PROGRESS_LEFT].width, Height, 0, 0);
	DrawingList[1] = TDrawing(SKIN_PROGRESS_MIDDLE, Skins()[SKIN_PROGRESS_LEFT].width, 0, Width - Skins()[SKIN_PROGRESS_RIGHT].width - Skins()[SKIN_PROGRESS_LEFT].width, Height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_PROGRESS_RIGHT, Width - Skins()[SKIN_PROGRESS_RIGHT].width, 0, Skins()[SKIN_PROGRESS_RIGHT].width, Height, 0, 0);
	int Filled = Current * Width / Maximum;
	if (Filled < 1)
		DrawingList[3] = TDrawing(SKIN_NONE, 0, 0, 0, 0, 0, 0);
	else
		DrawingList[3] = TDrawing(SKIN_PROGRESS_FILLED_LEFT, 0, 0, Filled, Height, 0, 0);
	if (Filled > Width - Skins()[SKIN_PROGRESS_FILLED_RIGHT].width) {
		DrawingList[5] = TDrawing(SKIN_PROGRESS_FILLED_RIGHT, Width - Skins()[SKIN_PROGRESS_FILLED_RIGHT].width, 0, Skins()[SKIN_PROGRESS_FILLED_RIGHT].width - Width + Filled, Height, 0, 0);
		Filled = Width - Skins()[SKIN_PROGRESS_FILLED_RIGHT].width;
	} else {
		DrawingList[5] = TDrawing(SKIN_NONE, 0, 0, 0, 0, 0, 0);
	}
	if (Filled > Skins()[SKIN_PROGRESS_FILLED_LEFT].width)
		DrawingList[4] = TDrawing(SKIN_PROGRESS_FILLED_MIDDLE, Skins()[SKIN_PROGRESS_FILLED_LEFT].width, 0, Filled - Skins()[SKIN_PROGRESS_FILLED_LEFT].width, Height, 0, 0);
	else
		DrawingList[4] = TDrawing(SKIN_NONE, 0, 0, 0, 0, 0, 0);
}

void TProgressBar::SetMaximum(int maximum)
{
	PATELLA_ASSERT_VALUES(maximum>0, maximum);
	Maximum = maximum;
	Arrange();
}

void TProgressBar::SetCurrent(int current)
{
	Current = current;
	Limit(&Current, 0, Maximum);
	Arrange();
}

// NOLINTEND(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg)
