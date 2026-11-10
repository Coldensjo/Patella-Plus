// DialogElements.h: the controls of the dialogs and windows
// (c:\mud\tibiaplayer\DialogElements.h; DialogElements.cpp is .\DialogElements.cpp).
//
// Separators, resizers, frames (TBorder), texts (TLabel, TFormattedText),
// links, icons, buttons of every kind, check boxes, edit lines (single line,
// password, multi-line) with their focus groups, scroll bars of both
// directions, list boxes and the progress bar.
//
// Names from the binary: SkinProperties.Buttons.Texture and CheckBox.Item,
// NumberOfSkins, GetMinimumWidth/GetMinimumHeight, GetWidth/GetHeight, Text,
// Cursor, MaxLength, FocusGroup, MemberList, Member, Active, Next, FieldWidth,
// Lines, RadioButtonGroup, ToResize, TokenType/TokenData, maximum/current, and
// the parameters (_ToResize, _Window, _Text, ButtonText, _Destination,
// _Outfit, _Direction, Mask, ColorData, _ButtonUp, _ButtonDown, _MaxLength,
// _FocusGroup, Position, insertlength, start, length, _Min, _Max, _Value,
// _ButtonRange, Number, Key, Code, Event, Surface, x, y, Width, Height).
// TCheckBox is named after SkinProperties.CheckBox. The other class, member
// and function names are ours; see notes/decisions.md, "DialogElements.cpp".

#pragma once

#include "common.h"
#include "types/FixedString.h"
#include "types/IndexedVector.h"
#include "GUI.h"
#include "graphics.h"
#include "Skins.h"   // Skins
#include "Status.h"  // SetMouseOverText
#include "Objects.h" // TObject
#include <array>
#include <cstring>
#include <memory>

// Issue #111 (clang-tidy, the first look at this file in patella_core): cppcoreguidelines-owning-memory off for the file:
// the original's raw owners (notes/ownership.md): the window tree owns its children, the message queue its blocks,
// the lists their entries
// NOLINTBEGIN(cppcoreguidelines-owning-memory)

// --- Separator, resizer and frame -----------------------------------------------

// A line between groups: skins 9 and 8 one above the other across the width,
// or 11 and 10 side by side down the height.
class TSeparator : public TElement {
public:
	TSeparator(bool _Horizontal)
	{
		Horizontal = _Horizontal;
		DrawingListLength = 2;
		DrawingList.reset(new TDrawing[2]);
	}
	void SetSize(int Width, int Height) override;
	void Arrange() override;

	bool Horizontal; // +0x24
};

// The resize handle of a window: dragging it changes ToResize's width
// and/or height by the mouse movement.
class TResizer : public TElement {
public:
	TResizer(TComponent *_ToResize, bool _ResizeWidth, bool _ResizeHeight, bool _Invisible);
	void SetSize(int Width, int Height) override;
	void Arrange() override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	void ReleaseCapture(TComponent *Component) override
	{
		Dragging = false;
		TElement::ReleaseCapture(Component);
	}

	TComponent *ToResize; // +0x24
	bool Dragging;        // +0x28
	int StartX;           // +0x2c, where the drag started
	int StartY;           // +0x30
	bool ResizeWidth;     // +0x34
	bool ResizeHeight;    // +0x35
	bool Invisible;       // +0x36, no skin
};

// A frame of nine skins (corners, edges and an optional background) around one
// window.
class TBorder : public TWindow {
public:
	TBorder(TComponent *_Window, int _Border); // the skins 26-33
	TBorder(TComponent *_Window, int _TopLeft, int _Top, int _TopRight, int _Left, int _Right, int _BottomLeft,
		int _Bottom, int _BottomRight, int _Border);
	TBorder(TComponent *_Window, int _TopLeft, int _Top, int _TopRight, int _Left, int _Right, int _BottomLeft,
		int _Bottom, int _BottomRight, int _Background, int _Border);
	void SetSize(int Width, int Height) override;
	int GetMinimumWidth() override { return Skins()[Right].width + Skins()[Left].width + 2 * Border; }
	int GetMinimumHeight() override { return Skins()[Bottom].height + Skins()[Top].height + 2 * Border; }
	void Arrange() override;
	void FitToWindow();

	TComponent *Window; // +0x2c, a window or an element (the outfit dialog frames its icon)
	int TopLeft;        // +0x30, skins
	int Top;            // +0x34
	int TopRight;       // +0x38
	int Left;           // +0x3c
	int Right;          // +0x40
	int BottomLeft;     // +0x44
	int Bottom;         // +0x48
	int BottomRight;    // +0x4c
	int Background;     // +0x50, 0 for none
	TRect WindowArea;   // +0x54, Window's area, set by Arrange
	int Border;         // +0x64, space between the frame and the window
};

// --- Texts ------------------------------------------------------------------------

// A text of one or more lines. The constructors without an area size the
// label to the text. Mouse events go through it.
class TLabel : public TElement {
public:
	TLabel(const TLabel &) = delete;
	TLabel &operator=(const TLabel &) = delete;
	TLabel(int x, int y, int Width, int Height, TFont _Font, const char *_Text, bool Shorten);
	TLabel(int x, int y, TFont _Font, const char *_Text);
	TLabel(TFont _Font, const char *_Text);
	~TLabel() override
	{
		if (Text != nullptr)
			Text.reset();
		if (FullText != nullptr)
			FullText.reset();
		if (Line != nullptr)
			Line.reset();
	}
	void Draw(int Surface, int x, int y) override;
	bool MouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) override { return false; }
	bool LeftButtonDown([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }
	bool LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }
	bool RightButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }
	void MouseHover(int x, int y) override;
	virtual void SetText(const char *_Text, bool Shorten);

	std::unique_ptr<char[]> Text;     // +0x24, the lines, each terminated (issue #58)
	std::unique_ptr<char[]> FullText; // +0x28, the text as given (the tooltip of a shortened text; issue #58)
	TFont Font;                       // +0x2c
	int Lines;                        // +0x40
	std::unique_ptr<char *[]> Line;   // +0x44, the start of each line in Text (owns the table, not the lines; #58)
	bool Shortened;                   // +0x48, a line was cut to the width
};

// A text with formatting tokens (<br>, <p>, <hr>, <vspace=n>, <hspace=n>,
// <indent=n>, colours ...), laid out as one TLabel per piece; the help
// bubble's text. A width or height below 1 means "as large as the text".
class TFormattedText : public TWindow {
public:
	TFormattedText(const TFormattedText &) = delete;
	TFormattedText &operator=(const TFormattedText &) = delete;
	TFormattedText(const char *_Text);
	~TFormattedText() override
	{
		if (Text != nullptr)
			Text.reset();
		if (Line != nullptr)
			Line.reset();
	}
	void SetSize(int Width, int Height) override;
	void Arrange() override;
	virtual void SetText(const char *_Text, bool Rearrange);
	int GetToken(const char *Text, int &TokenType, int &TokenData);

	std::unique_ptr<char[]> Text; // +0x2c (issue #58)
	std::unique_ptr<char[]> Line; // +0x30, the piece being laid out, a buffer as large as Text (issue #58)
	bool AutoWidth;               // +0x34
};

class TTextButton;

// A text with a button that opens Destination with OpenURL: the dialogs'
// links to the web site.
class TLink final : public TWindow {
public:
	TLink(const TLink &) = delete;
	TLink &operator=(const TLink &) = delete;
	TLink(const char *_Text, const char *ButtonText, const char *_Destination, void (*_OpenURL)(const char *URL),
		TFont Font);
	~TLink() override { Destination.reset(); }
	void SetSize(int Width, int Height) override;
	int GetMinimumWidth() override;
	int GetMinimumHeight() override;
	void Arrange() override;
	void HandleEvent(int Event) override;

	void (*OpenURL)(const char *URL);    // +0x2c
	std::unique_ptr<char[]> Destination; // +0x30 (issue #58)
	TTextButton *Button;                 // +0x34, event 1
	TLabel *Label;                       // +0x38
};

// --- Icons ------------------------------------------------------------------------

// An outfit with its four colours.
class TOutfitIcon final : public TElement {
public:
	TOutfitIcon(int _Outfit, int _Direction, int *ColorData, const TRGBColor *_Highlight);
	void Draw(int Surface, int x, int y) override;
	bool MouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) override { return false; }
	bool LeftButtonDown([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }
	bool LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }
	bool RightButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }
	void SetOutfit(int _Outfit) { Outfit = _Outfit; }
	void SetColor(int Mask, int Color);
	void SetColors(const int *ColorData);

	int Outfit;                // +0x24
	std::array<int, 4> Colors; // +0x28, head, body, legs, feet
	int Direction;             // +0x38, kept, not drawn
	TRGBColor Highlight;       // +0x3c
};

// One skin, sized to it.
class TSkinIcon : public TElement {
public:
	TSkinIcon(int _Skin);
	void Draw(int Surface, int x, int y) override;
	bool MouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) override { return false; }
	bool LeftButtonDown([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }
	bool LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }
	bool RightButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }
	bool MouseWheel([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Direction) override { return false; }

	int Skin; // +0x24
};

// An object, optionally in a frame (skin 102).
class TObjectIcon final : public TElement {
public:
	TObjectIcon(TObject _Obj, const TRGBColor *_Highlight, bool _Framed);
	void Draw(int Surface, int x, int y) override;
	bool MouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) override { return false; }
	bool LeftButtonDown([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }
	bool LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }
	bool RightButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return false; }

	bool Framed;         // +0x34
	TRGBColor Highlight; // +0x38
	TObject Obj;         // +0x44
};

// --- Buttons ----------------------------------------------------------------------

struct TRadioButtonGroup;

// A button drawn with two skins (released, pressed) that sends Callback when
// it is released over itself. Repeat sends Callback again on every
// held-button tick (MouseWheel code 6). A button of a radio group shows as
// pressed while it is the group's selection.
class TSkinButton : public TElement {
public:
	TSkinButton(int x, int y, int _ButtonUp, int _ButtonDown, TCallback _Callback, const char *_Tooltip,
		bool _Repeat);
	void Draw(int Surface, int x, int y) override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool MouseWheel(int x, int y, int Code) override;
	void MouseHover([[maybe_unused]] int x, [[maybe_unused]] int y) override { SetMouseOverText(Tooltip); }
	void ReleaseCapture(TComponent *Component) override
	{
		Pressed = false;
		TElement::ReleaseCapture(Component);
	}
	virtual bool IsPressed();
	void SetSkins(int _ButtonUp, int _ButtonDown);
	void SetRadioButtonGroup(TRadioButtonGroup *Group);
	void SetEnabled(bool _Enabled) { Enabled = _Enabled; }

	int ButtonUp;                        // +0x24
	int ButtonDown;                      // +0x28
	bool Pressed;                        // +0x2c, the mouse went down on the button
	bool MouseInside;                    // +0x2d
	bool Repeat;                         // +0x2e
	TCallback Callback;                  // +0x30
	TRadioButtonGroup *RadioButtonGroup; // +0x38
	const char *Tooltip;                 // +0x3c
	bool Enabled;                        // +0x40
};

// The buttons of a radio group (0x28 bytes; the type name is ours). Selected
// is left uninitialised.
struct TRadioButtonGroup {
	TRadioButtonGroup() : Buttons(0, 0, 10, nullptr) { Count = 0; }
	TSkinButton *Selected;                 // +0x00
	TIndexedVector<TSkinButton *> Buttons; // +0x04
	int Count;                             // +0x24
};

// A button on a coloured background.
class TColorButton final : public TSkinButton {
public:
	TColorButton(int x, int y, TRGBColor _Color, int _ButtonUp, int _ButtonDown, TCallback _Callback,
		const char *_Tooltip);
	void Draw(int Surface, int x, int y) override;

	TRGBColor Color; // +0x44
};

// A button with an icon skin on it.
class TIconButton final : public TSkinButton {
public:
	TIconButton(int x, int y, int _Icon, int _ButtonUp, int _ButtonDown, TCallback _Callback,
		const char *_Tooltip);
	void Draw(int Surface, int x, int y) override;

	int Icon; // +0x44
};

// A button that alternates between two pairs of skins: MapWindow.cpp's hint
// button.
class TBlinkingButton : public TSkinButton {
public:
	TBlinkingButton(int x, int y, int _ButtonUp1, int _ButtonDown1, int _ButtonUp2, int _ButtonDown2,
		TCallback _Callback, const char *_Tooltip);
	void Draw(int Surface, int x, int y) override;

	int NextChange;  // +0x44, GetMilliseconds() time
	int Phase;       // +0x48, 1 or 2
	int ButtonUp1;   // +0x4c
	int ButtonUp2;   // +0x50
	int ButtonDown1; // +0x54
	int ButtonDown2; // +0x58
};

// A button with a text.
class TTextButton : public TSkinButton {
public:
	TTextButton(int x, int y, const char *_Text, int _ButtonUp, int _ButtonDown, TCallback _Callback,
		const char *_Tooltip);
	TTextButton(int x, int y, const char *_Text, TCallback _Callback, const char *_Tooltip);
	void Draw(int Surface, int x, int y) override;
	virtual void SetText(const char *_Text) { Text.Assign(_Text); }

	TFixedString<50> Text; // +0x44
};
inline TObjectIcon::TObjectIcon(TObject _Obj, const TRGBColor *_Highlight, bool _Framed)
{
	Obj = _Obj;
	Framed = _Framed;
	Highlight = *_Highlight;
}

inline int TLink::GetMinimumWidth()
{
	return Button->Width;
}

inline int TLink::GetMinimumHeight()
{
	return Button->Height;
}

// A button that stays pressed while State is set; a click toggles it.
// The tooltips belong to the two states.
class TToggleButton : public TSkinButton {
public:
	TToggleButton(int x, int y, int _ButtonUp, int _ButtonDown, TCallback _Callback, const char *_Tooltip1,
		const char *_Tooltip2, bool _State);
	bool LeftButtonUp(int x, int y, int Flags) override;
	void MouseHover([[maybe_unused]] int x, [[maybe_unused]] int y) override { SetMouseOverText(State ? Tooltip2 : Tooltip1); }
	bool IsPressed() override;

	bool State;           // +0x44
	const char *Tooltip1; // +0x48
	const char *Tooltip2; // +0x4c
};

// A text button with a state: ContentWindows.cpp's TButtonsWindow sets State
// while it draws.
class TTextToggleButton : public TTextButton {
public:
	TTextToggleButton(int x, int y, const char *_Text, int _ButtonUp, int _ButtonDown, TCallback _Callback,
		const char *_Tooltip1, const char *_Tooltip2, bool _State);
	bool LeftButtonUp(int x, int y, int Flags) override;
	void MouseHover([[maybe_unused]] int x, [[maybe_unused]] int y) override { SetMouseOverText(State ? Tooltip2 : Tooltip1); }
	bool IsPressed() override;

	bool State;           // +0x78
	const char *Tooltip1; // +0x7c
	const char *Tooltip2; // +0x80
};

// A button that steps through States states, each with its skins and
// callback: the console's volume. The constructor takes the first state's
// skins and callback, then those of the other states.
class TMultiStateButton : public TSkinButton {
public:
	TMultiStateButton(const TMultiStateButton &) = delete;
	TMultiStateButton &operator=(const TMultiStateButton &) = delete;
	TMultiStateButton(int x, int y, const char *_Tooltip, int _States, int _ButtonUp, int _ButtonDown,
		TCallback _Callback, ...);
	~TMultiStateButton() override
	{
		ButtonUps.reset();
		ButtonDowns.reset();
		Callbacks.reset();
	}
	bool LeftButtonUp(int x, int y, int Flags) override;
	// State is set past the last state, which wraps to state 0.
	void Reset()
	{
		State = States + 1;
		if (State >= States)
			State = 0;
		ButtonUp = ButtonUps[State];
		ButtonDown = ButtonDowns[State];
	}

	int State;                              // +0x44
	int States;                             // +0x48
	std::unique_ptr<int[]> ButtonUps;       // +0x4c (issue #58)
	std::unique_ptr<int[]> ButtonDowns;     // +0x50
	std::unique_ptr<TCallback[]> Callbacks; // +0x54
};

// A check box with a text.
class TCheckBox final : public TWindow {
public:
	TCheckBox(const char *_Text);
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override
	{
		if (Enabled)
			Checked = !Checked;
		return true;
	}
	virtual void SetChecked(bool _Checked) { Checked = _Checked; }
	virtual bool IsChecked() { return Checked; }
	void SetEnabled(bool _Enabled);

	TLabel *Label; // +0x2c
	bool Checked;  // +0x30
	bool Enabled;  // +0x31
};

// --- Edit lines -------------------------------------------------------------------

class TFocusGroup;

// A line of text input ("_Edit" in ContextMenus.cpp), the console's Input.
// HandleEvent takes 16 copy, 17 cut, 18 paste, 19 delete and 20 select all. A
// read-only line (ReadOnly) allows only moving, copying and selecting; a disabled
// one (Disabled) shows no text and takes no input.
class TEditLine : public TWindow {
public:
	TEditLine(const TEditLine &) = delete;
	TEditLine &operator=(const TEditLine &) = delete;
	TEditLine(TRect Area, int _MaxLength, bool _ReadOnly);
	TEditLine(int _MaxLength, bool _ReadOnly);
	~TEditLine() override { Text.reset(); }
	void SetSize(int Width, int Height) override;
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool LeftButtonDoubleClick(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;
	void ReleaseCapture(TComponent *Component) override
	{
		Selecting = false;
		TWindow::ReleaseCapture(Component);
	}
	virtual void SetText(const char *_Text);
	virtual void Clear();
	virtual void DeleteText(int start, int length);
	virtual void DeleteChar(int Position) { DeleteText(Position, 1); }
	virtual int InsertText(int Position, const char *String, int insertlength);
	virtual void InsertChar(int Position, char c);
	virtual void SetDisabled(bool _Disabled) { Disabled = _Disabled; }
	virtual bool IsDisabled() { return Disabled; }
	virtual void SetFont(TFont _Font) { Font = _Font; }
	virtual int GetPosition(int x, int y);
	void SetActive(bool _Active);
	void SetFocusGroup(TFocusGroup *_FocusGroup);
	[[nodiscard]] bool HasSelection() const { return SelectionStart != -1 && SelectionLength != 0; }
	void SetCursor(int Position)
	{
		Cursor = Position;
		Limit(&Cursor, 0, (int)strlen(Text.get()));
	}
	void LimitCursor() { Limit(&Cursor, 0, (int)strlen(Text.get())); }
	void SetSelection(int Position)
	{
		Limit(&Position, 0, (int)strlen(Text.get()));
		SelectionLength = 0;
		SelectionStart = Position;
	}

	std::unique_ptr<char[]> Text; // +0x2c (issue #81)
	int MaxLength;                // +0x30
	int Cursor;                   // +0x34
	bool SingleLine;              // +0x38, a paste stops at the first line break
	TFocusGroup *FocusGroup;      // +0x3c
	TFont Font;                   // +0x40
	int FirstVisible;             // +0x54, the first character shown
	bool Active;                  // +0x58, shows the cursor
	int SelectionStart;           // +0x5c, -1 without a selection
	int SelectionLength;          // +0x60, negative when selected to the left
	bool Selecting;               // +0x64, a mouse drag
	bool Disabled;                // +0x65
	bool ReadOnly;                // +0x66
};

// The character under (x, y): the first visible one left of the text, the
// end right of or below it.
inline int TEditLine::GetPosition(int x, int y)
{
	x -= SkinProperties().EditLineTextMargin;
	y -= SkinProperties().EditLineTextMargin;
	if (x < 0 || y < 0)
		return FirstVisible;
	if (x > Width - 2 * SkinProperties().EditLineTextMargin || y > Height - 2 * SkinProperties().EditLineTextMargin)
		return (int)strlen(Text.get());
	const int Position = GetFittingTextLength(Font.FontNumber, Text.get() + FirstVisible, x) + FirstVisible;
	if (Position > (int)strlen(Text.get()))
		return (int)strlen(Text.get());
	return Position;
}

// An edit line that shows stars. It only pastes.
class TPasswordEditLine final : public TEditLine {
public:
	TPasswordEditLine(const TPasswordEditLine &) = delete;
	TPasswordEditLine &operator=(const TPasswordEditLine &) = delete;
	TPasswordEditLine(int _MaxLength);
	~TPasswordEditLine() override { Stars.reset(); }
	void Draw(int Surface, int x, int y) override;
	// NOLINTNEXTLINE(bugprone-parent-virtual-call): the original's call of the base's base (the image's)
	bool MouseMove(int x, int y) override { return TWindow::MouseMove(x, y); } // no selecting
	// NOLINTNEXTLINE(bugprone-parent-virtual-call): the original's call of the base's base (the image's)
	bool RightButtonUp(int x, int y, int Flags) override { return TWindow::RightButtonUp(x, y, Flags); } // no menu
	void HandleEvent(int Event) override;

	std::unique_ptr<char[]> Stars; // +0x68 (issue #81)
};

// A multi-line edit field with a scroll bar. LineStart holds the first
// character of every line (Lines entries, then the text's end).
class TScrollBar;
class TEditText final : public TEditLine {
public:
	TEditText(int MaxLength, bool _ReadOnly);
	int GetMinimumWidth() override;
	int GetMinimumHeight() override;
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool MouseWheel(int x, int y, int Code) override;
	void KeyPress(int Key) override;
	int GetPosition(int x, int y) override;
	// NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): the original's virtual over TEditLine's plain one
	virtual void SetActive(bool _Active);
	virtual void SetText(const char *_Text, bool KeepPosition);
	using TEditLine::SetText; // TEditLine's SetText(Text) too (GCC's -Woverloaded-virtual, issue #111)
	void FormatText();

	TIndexedVector<int> LineStart; // +0x68
	TScrollBar *ScrollBar;         // +0x88
	int Lines;                     // +0x8c
	int FieldWidth;                // +0x90
	int FieldHeight;               // +0x94
	int VisibleLines;              // +0x98
};

// The edit lines of a dialog between which Tab and Shift+Tab move the input
// (0x28 bytes).
class TFocusGroup {
public:
	TFocusGroup();
	void SetActive(TEditLine *Member);
	void AddMember(TEditLine *Member);
	void Next();
	void Previous();

	TEditLine *Active;                      // +0x00
	TIndexedVector<TEditLine *> MemberList; // +0x04
	int Members;                            // +0x24
};

// --- Scroll bars ------------------------------------------------------------------

// The track and slider of a horizontal scroll bar; Callback is sent on
// every change of Value.
class THorizontalScrollBarRange final : public TElement {
public:
	THorizontalScrollBarRange(int _Min, int _Max, int _Value, int _Visible, TCallback _Callback);
	void SetSize(int Width, int Height) override;
	int GetMinimumWidth() override { return SkinProperties().HorizontalScrollBarRangeMinimumWidth; }
	int GetMinimumHeight() override { return SkinProperties().HorizontalScrollBarHeight; }
	void Arrange() override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool MouseWheel(int x, int y, int Code) override;
	void KeyPress(int Key) override;
	void ReleaseCapture(TComponent *Component) override
	{
		Dragging = false;
		TElement::ReleaseCapture(Component);
	}
	void SetValue(int _Value);

	int ButtonRange;    // +0x24, the step of a button or wheel
	int Min;            // +0x28
	int Max;            // +0x2c
	int Value;          // +0x30
	int Visible;        // +0x34, the part shown at once (the slider's share)
	bool Dragging;      // +0x38, the slider
	bool Pressed;       // +0x39
	int DragOffset;     // +0x3c
	int SliderWidth;    // +0x40
	int SliderPosition; // +0x44
	bool PageScroll;    // +0x48, a click beside the slider moves by Visible-1
	bool Enabled;       // +0x49, Min!=Max
	TCallback Callback; // +0x4c
};

// A horizontal scroll bar: two buttons and the range. The height is always
// SkinProperties.HorizontalScrollBarHeight.
class THorizontalScrollBar final : public TWindow {
public:
	THorizontalScrollBar(int Min, int Max, int _Value, int _Visible, int _ButtonRange, TCallback _Callback);
	void SetSize(int Width, int Height) override;
	int GetMinimumWidth() override { return SkinProperties().HorizontalScrollBarMinimumWidth; }
	int GetMinimumHeight() override { return SkinProperties().HorizontalScrollBarHeight; }
	bool MouseWheel(int x, int y, int Code) override;
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	THorizontalScrollBarRange *Range; // +0x2c
	TSkinButton *LeftButton;          // +0x30, event 5
	TSkinButton *RightButton;         // +0x34, event 6
};

inline void THorizontalScrollBar::KeyPress(int Key)
{
	Range->KeyPress(Key);
}

// The track and slider of a vertical scroll bar.
class TScrollBarRange : public TElement {
public:
	TScrollBarRange(int x, int y, int Width, int Height, int _Min, int _Max, int _Value, int _Visible);
	void SetSize(int Width, int Height) override;
	int GetMinimumWidth() override { return SkinProperties().ScrollBarMinimumWidth; }
	int GetMinimumHeight() override { return SkinProperties().ScrollBarRangeMinimumHeight; }
	void Arrange() override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool MouseWheel(int x, int y, int Code) override;
	void KeyPress(int Key) override;
	void ReleaseCapture(TComponent *Component) override
	{
		Dragging = false;
		TElement::ReleaseCapture(Component);
	}
	void SetRange(int _Min, int _Max, int _Value, int _Visible);
	void SetValue(int _Value);

	int ButtonRange;    // +0x24
	int Min;            // +0x28
	int Max;            // +0x2c
	int Value;          // +0x30
	int Visible;        // +0x34
	bool Pressed;       // +0x38
	bool Dragging;      // +0x39
	int DragOffset;     // +0x3c
	int SliderHeight;   // +0x40
	int SliderPosition; // +0x44
	bool PageScroll;    // +0x48
	bool Enabled;       // +0x49
};

// A vertical scroll bar: two buttons and the range. The width is always
// SkinProperties.ScrollBarWidth.
class TScrollBar : public TWindow {
public:
	TScrollBar(TRect Area, int Min, int Max, int _Value, int _Visible, int _ButtonRange);
	void SetSize(int Width, int Height) override;
	int GetMinimumWidth() override { return SkinProperties().ScrollBarMinimumWidth; }
	int GetMinimumHeight() override { return SkinProperties().ScrollBarMinimumHeight; }
	bool MouseWheel(int x, int y, int Code) override;
	void KeyPress(int Key) override { Range->KeyPress(Key); }
	void HandleEvent(int Event) override;

	TScrollBarRange *Range;  // +0x2c
	TSkinButton *UpButton;   // +0x30, event 3
	TSkinButton *DownButton; // +0x34, event 4
};

inline int TEditText::GetMinimumWidth()
{
	return ScrollBar->GetMinimumWidth();
}

inline int TEditText::GetMinimumHeight()
{
	return ScrollBar->GetMinimumHeight();
}

// --- List boxes -------------------------------------------------------------------

// A list of text lines with a selection and a scroll bar. SelectCallback
// goes out when a line is clicked or selected by key, ActionCallback on a
// double click. An editable list edits the selected line in EditLine (a
// double click starts, Return or another line ends).
class TListBox : public TWindow {
public:
	TListBox(const TListBox &) = delete;
	TListBox &operator=(const TListBox &) = delete;
	TListBox(TCallback _SelectCallback, TCallback _ActionCallback);
	~TListBox() override
	{
		for (int i = 0; i < Lines; i++)
			delete[] LineText(i);
		if (!Editing && EditLine != nullptr)
			delete EditLine;
	}
	void SetSize(int Width, int Height) override;
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonDoubleClick(int x, int y, int Flags) override;
	bool MouseWheel(int x, int y, int Code) override;
	void MouseHover(int x, int y) override;
	void KeyPress(int _Key) override;
	virtual void StartEdit();
	virtual void StopEdit();
	void AddLine(const char *Text);
	void SetLine(int Number, const char *Text);
	char *GetLine(int Number);
	void SetSelection(int Number);
	void Clear();
	[[nodiscard]] int GetLineAt(int x, int y) const;
	[[nodiscard]] int GetSelection() const
	{
		if (Selected < 0 || Selected > Lines - 1)
			return -1;
		return Selected;
	}
	void SetEditable(bool _Editable) { Editable = _Editable; }
	[[nodiscard]] int GetNumberOfLines() const { return Lines; }

	int Lines;                       // +0x2c
	int TextWidth;                   // +0x30
	TIndexedVector<char *> LineText; // +0x34
	int Selected;                    // +0x54, -1 for none
	TCallback SelectCallback;        // +0x58
	TCallback ActionCallback;        // +0x60
	TScrollBar *ScrollBar;           // +0x68
	int VisibleLines;                // +0x6c
	bool Editable;                   // +0x70
	bool Editing;                    // +0x71
	TEditLine *EditLine;             // +0x74
	int EditScroll;                  // +0x78, the scroll position when the edit started
};

// A line of a TFontListBox (0x18 bytes; the type name is ours). The
// constructor is implicit (TFont's defaults; Text is left as it is).
struct TListBoxLine {
	char *Text; // +0x00
	TFont Font; // +0x04
};

// A list box whose lines have their own fonts. It keeps its lines in
// FontLines; the base's LineText stays empty.
class TFontListBox final : public TListBox {
public:
	TFontListBox(const TFontListBox &) = delete;
	TFontListBox &operator=(const TFontListBox &) = delete;
	TFontListBox(TCallback _SelectCallback, TCallback _ActionCallback);
	~TFontListBox() override
	{
		for (int i = 0; i < Lines; i++)
			delete[] FontLines(i).Text;
	}
	void Draw(int Surface, int x, int y) override;
	void MouseHover(int x, int y) override;
	void StartEdit() override;
	void StopEdit() override;
	void AddLine(const char *Text, TFont Font);
	void AddLine(int MaxLength);
	void SetLine(int Number, const char *Text, TFont Font);
	// NOLINTNEXTLINE(bugprone-derived-method-shadowing-base-method): the original's, called on a TFontListBox only
	void Clear();
	char *GetLine(int Number, TFont *Font);

	TIndexedVector<TListBoxLine> FontLines; // +0x7c
};

// --- Progress bar -----------------------------------------------------------------

// A bar filled to current/maximum.
class TProgressBar final : public TElement {
public:
	TProgressBar()
	{
		DrawingListLength = 6;
		DrawingList.reset(new TDrawing[6]);
		Current = 0;
		Maximum = 100;
	}
	int GetMinimumWidth() override { return Skins()[SKIN_PROGRESS_LEFT].width + Skins()[SKIN_PROGRESS_RIGHT].width + Skins()[SKIN_PROGRESS_MIDDLE].width; }
	int GetMinimumHeight() override { return Skins()[SKIN_PROGRESS_LEFT].height; }
	void Arrange() override;
	void SetMaximum(int maximum);
	void SetCurrent(int current);

	int Maximum; // +0x24
	int Current; // +0x28
};
// NOLINTEND(cppcoreguidelines-owning-memory)
