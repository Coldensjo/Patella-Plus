// DialogElements.h: the controls of the dialogs and windows.
//
// Separators, resizers, frames (TBorder), texts (TLabel, TFormattedText),
// links, icons, buttons of every kind, check boxes, edit lines (single line,
// password, multi-line) with their focus groups, scroll bars of both
// directions, list boxes and the progress bar.

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

// cppcoreguidelines-owning-memory is off for the file, for the original's raw owners: the window tree owns its
// children, the message queue its blocks, the lists their entries.
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

	bool Horizontal;
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

	TComponent *ToResize;
	bool Dragging;
	int StartX; // where the drag started
	int StartY;
	bool ResizeWidth;
	bool ResizeHeight;
	bool Invisible; // no skin
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

	TComponent *Window; // a window or an element (the outfit dialog frames its icon)
	int TopLeft;        // skins
	int Top;
	int TopRight;
	int Left;
	int Right;
	int BottomLeft;
	int Bottom;
	int BottomRight;
	int Background;   // 0 for none
	TRect WindowArea; // Window's area, set by Arrange
	int Border;       // space between the frame and the window
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

	std::unique_ptr<char[]> Text;     // the lines, each terminated
	std::unique_ptr<char[]> FullText; // the text as given (the tooltip of a shortened text)
	TFont Font;
	int Lines;
	std::unique_ptr<char *[]> Line; // the start of each line in Text (owns the table, not the lines)
	bool Shortened;                 // a line was cut to the width
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

	std::unique_ptr<char[]> Text;
	std::unique_ptr<char[]> Line; // the piece being laid out, a buffer as large as Text
	bool AutoWidth;
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

	void (*OpenURL)(const char *URL);
	std::unique_ptr<char[]> Destination;
	TTextButton *Button; // event 1
	TLabel *Label;
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

	int Outfit;
	std::array<int, 4> Colors; // head, body, legs, feet
	int Direction;             // kept, not drawn
	TRGBColor Highlight;
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

	int Skin;
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

	bool Framed;
	TRGBColor Highlight;
	TObject Obj;
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

	int ButtonUp;
	int ButtonDown;
	bool Pressed; // the mouse went down on the button
	bool MouseInside;
	bool Repeat;
	TCallback Callback;
	TRadioButtonGroup *RadioButtonGroup;
	const char *Tooltip;
	bool Enabled;
};

// The buttons of a radio group (the type name is ours). Selected
// is left uninitialised.
struct TRadioButtonGroup {
	TRadioButtonGroup() : Buttons(0, 0, 10, nullptr) { Count = 0; }
	TSkinButton *Selected;
	TIndexedVector<TSkinButton *> Buttons;
	int Count;
};

// A button on a coloured background.
class TColorButton final : public TSkinButton {
public:
	TColorButton(int x, int y, TRGBColor _Color, int _ButtonUp, int _ButtonDown, TCallback _Callback,
		const char *_Tooltip);
	void Draw(int Surface, int x, int y) override;

	TRGBColor Color;
};

// A button with an icon skin on it.
class TIconButton final : public TSkinButton {
public:
	TIconButton(int x, int y, int _Icon, int _ButtonUp, int _ButtonDown, TCallback _Callback,
		const char *_Tooltip);
	void Draw(int Surface, int x, int y) override;

	int Icon;
};

// A button that alternates between two pairs of skins: MapWindow.cpp's hint
// button.
class TBlinkingButton : public TSkinButton {
public:
	TBlinkingButton(int x, int y, int _ButtonUp1, int _ButtonDown1, int _ButtonUp2, int _ButtonDown2,
		TCallback _Callback, const char *_Tooltip);
	void Draw(int Surface, int x, int y) override;

	int NextChange; // GetMilliseconds() time
	int Phase;      // 1 or 2
	int ButtonUp1;
	int ButtonUp2;
	int ButtonDown1;
	int ButtonDown2;
};

// A button with a text.
class TTextButton : public TSkinButton {
public:
	TTextButton(int x, int y, const char *_Text, int _ButtonUp, int _ButtonDown, TCallback _Callback,
		const char *_Tooltip);
	TTextButton(int x, int y, const char *_Text, TCallback _Callback, const char *_Tooltip);
	void Draw(int Surface, int x, int y) override;
	virtual void SetText(const char *_Text) { Text.Assign(_Text); }

	TFixedString<50> Text;
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

	bool State;
	const char *Tooltip1;
	const char *Tooltip2;
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

	bool State;
	const char *Tooltip1;
	const char *Tooltip2;
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

	int State;
	int States;
	std::unique_ptr<int[]> ButtonUps;
	std::unique_ptr<int[]> ButtonDowns;
	std::unique_ptr<TCallback[]> Callbacks;
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

	TLabel *Label;
	bool Checked;
	bool Enabled;
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

	std::unique_ptr<char[]> Text;
	int MaxLength;
	int Cursor;
	bool SingleLine; // a paste stops at the first line break
	TFocusGroup *FocusGroup;
	TFont Font;
	int FirstVisible;    // the first character shown
	bool Active;         // shows the cursor
	int SelectionStart;  // -1 without a selection
	int SelectionLength; // negative when selected to the left
	bool Selecting;      // a mouse drag
	bool Disabled;
	bool ReadOnly;
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

	std::unique_ptr<char[]> Stars;
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
	using TEditLine::SetText; // TEditLine's SetText(Text) too (GCC's -Woverloaded-virtual)
	void FormatText();

	TIndexedVector<int> LineStart;
	TScrollBar *ScrollBar;
	int Lines;
	int FieldWidth;
	int FieldHeight;
	int VisibleLines;
};

// The edit lines of a dialog between which Tab and Shift+Tab move the input.
class TFocusGroup {
public:
	TFocusGroup();
	void SetActive(TEditLine *Member);
	void AddMember(TEditLine *Member);
	void Next();
	void Previous();

	TEditLine *Active;
	TIndexedVector<TEditLine *> MemberList;
	int Members;
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

	int ButtonRange; // the step of a button or wheel
	int Min;
	int Max;
	int Value;
	int Visible;   // the part shown at once (the slider's share)
	bool Dragging; // the slider
	bool Pressed;
	int DragOffset;
	int SliderWidth;
	int SliderPosition;
	bool PageScroll; // a click beside the slider moves by Visible-1
	bool Enabled;    // Min!=Max
	TCallback Callback;
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

	THorizontalScrollBarRange *Range;
	TSkinButton *LeftButton;  // event 5
	TSkinButton *RightButton; // event 6
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

	int ButtonRange;
	int Min;
	int Max;
	int Value;
	int Visible;
	bool Pressed;
	bool Dragging;
	int DragOffset;
	int SliderHeight;
	int SliderPosition;
	bool PageScroll;
	bool Enabled;
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

	TScrollBarRange *Range;
	TSkinButton *UpButton;   // event 3
	TSkinButton *DownButton; // event 4
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
		if (Selected < 0 || Selected >= Lines)
			return -1;
		return Selected;
	}
	void SetEditable(bool _Editable) { Editable = _Editable; }
	[[nodiscard]] int GetNumberOfLines() const { return Lines; }

	int Lines;
	int TextWidth;
	TIndexedVector<char *> LineText;
	int Selected; // -1 for none
	TCallback SelectCallback;
	TCallback ActionCallback;
	TScrollBar *ScrollBar;
	int VisibleLines;
	bool Editable;
	bool Editing;
	TEditLine *EditLine;
	int EditScroll; // the scroll position when the edit started
};

// A line of a TFontListBox (the type name is ours). The
// constructor is implicit (TFont's defaults; Text is left as it is).
struct TListBoxLine {
	char *Text;
	TFont Font;
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

	TIndexedVector<TListBoxLine> FontLines;
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

	int Maximum;
	int Current;
};
// NOLINTEND(cppcoreguidelines-owning-memory)
