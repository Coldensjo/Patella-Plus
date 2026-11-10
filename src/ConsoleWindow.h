// ConsoleWindow.h: the console below the map (c:\mud\tibiaplayer\ConsoleWindow.h).
//
// TConsoleWindow is MainWindow.cpp's Console: a tab bar (TTabBar) with one tab
// (TTab) per open channel, the view of the current channel (TConsole: a
// TTextWindow and a scroll bar), the edit line and the buttons. A TTextWindow
// shows the lines of a Talk.cpp text buffer, broken into TLineFormats (0x28
// bytes) that fit its width, with a selection. Tabs that do not fit into the bar
// are moved into two hidden windows of the bar (LeftTabs, RightTabs), whose arrow
// buttons scroll them back in.
//
// Names from the binary: TLineFormat's LineText and its constructor's
// parameters (_TextLine, _LineText, _LineLength, _OriginalOffset, _Indent),
// GetFirstLineNumber, GetLastLineNumber, _CurrentLineNumber, InvisibleLines
// (an error message), GetTextBuffer, IsDefault, GetCaption, CurrentConsole,
// CurrentTab, DefaultTab, NewTab, Input, TextBuffer, _Caption, _TextBuffer,
// _Active, Tab, Container ("failed to get composite"), Search, Question,
// Addressee, ChannelID, TalkMode, MessageSeparator and the catch-block
// parameters (Surface, x, y, Width, Height, Code, Event, Key, LineNumber,
// Active, RecvMode, SendMode). The class names and the other names are ours.

#pragma once

#include "common.h"
#include "types/IndexedVector.h"
#include "GUI.h"
#include "Talk.h"
#include "graphics.h"
#include <memory>

class TEditLine;         // DialogElements.h
class TScrollBar;        // DialogElements.h
class TSkinButton;       // DialogElements.h
class TToggleButton;     // DialogElements.h
class TMultiStateButton; // DialogElements.h

// One line of a text window: the part of a talk line that fits into the
// window's width. Indent is 0 for the first line of a talk line and 10 for
// the following ones (0 for the modes 18-22); LineEnd is set on the last
// part and where the text has a line break (the selection copies a "\n"
// there).
struct TLineFormat {
	// The colour is cleared by its own constructor first.
	TLineFormat()
		: LineText(nullptr), LineLength(0), Indent(0), FontNumber(FONT_SMOOTH), TextLine(nullptr), LineEnd(true)
	{
		Color = DefaultFontColor;
	}
	TLineFormat(TTalkLine *_TextLine, const char *_LineText, int _LineLength, int _OriginalOffset, int _Indent,
		int _FontNumber, TRGBColor _Color, bool _LineEnd)
	{
		PATELLA_ASSERT(_TextLine!=nullptr);
		PATELLA_ASSERT(_LineText!=nullptr);
		PATELLA_ASSERT_VALUES(_LineLength>0, _LineLength);
		PATELLA_ASSERT_VALUES(_Indent>=0, _Indent);
		PATELLA_ASSERT_VALUES(_OriginalOffset>=0, _OriginalOffset);
		TextLine = _TextLine;
		Color = _Color;
		LineText = _LineText;
		Indent = _Indent;
		LineLength = _LineLength;
		LineEnd = _LineEnd;
		FontNumber = _FontNumber;
	}

	const char *LineText; // points into the talk line's text
	int LineLength;
	int Indent;
	TRGBColor Color;
	int FontNumber;
	TTalkLine *TextLine;
	bool LineEnd;
};

// The text of a channel (ContextMenus.cpp's _TextWindow). Lines are numbered
// from GetFirstLineNumber() (Lines.min, always 0) to GetLastLineNumber();
// CurrentLineNumber is the bottom line shown (the scroll bar's value). A
// selection runs from (SelectionStartLine, SelectionStartOffset) to
// (SelectionEndLine, SelectionEndOffset) in either order; -1 is none.
class TTextWindow final : public TElement {
public:
	TTextWindow(const TTextWindow &) = delete;
	TTextWindow &operator=(const TTextWindow &) = delete;
	TTextWindow();
	// Lines frees its entries.
	~TTextWindow() override
	{
		if (SelectedText != nullptr)
			SelectedText.reset();
	}
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool LeftButtonDoubleClick(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	void HandleEvent(int Event) override;
	// The end of a selection drag.
	void ReleaseCapture(TComponent *Component) override
	{
		Selecting = false;
		TElement::ReleaseCapture(Component);
	}
	virtual void FormatLines();
	virtual bool GetPosition(int x, int y, int *Line, int *Offset, int *Length);

	void SetTextBuffer(TTalkBuffer *TextBuffer);
	TTalkLine *GetTalkLine(int LineNumber);
	TLineFormat *GetLineFormat(int LineNumber);
	void SetCurrentLineNumber(int _CurrentLineNumber);
	bool GetSelectionStart(int *Line, int *Offset);
	bool GetSelectionEnd(int *Line, int *Offset);
	bool SelectAll();
	const char *GetSelectedText();
	[[nodiscard]] int GetFirstLineNumber() const { return Lines.min; }
	[[nodiscard]] int GetLastLineNumber() const { return LastLineNumber; }
	// A selection of at least one character exists.
	[[nodiscard]] bool HasSelection() const
	{
		if (TextBuffer == nullptr)
			return false;
		if (!TextBuffer->Complete) {
			TextBuffer->LinesChanged = false;
			return false;
		}
		return SelectionStartLine != -1 && SelectionStartOffset != -1 && SelectionEndLine != -1
			&& SelectionEndOffset != -1
			&& (SelectionStartLine != SelectionEndLine || SelectionStartOffset != SelectionEndOffset);
	}

	TTalkBuffer *TextBuffer;
	TIndexedVector<TLineFormat> Lines;
	int CurrentLineNumber;
	int InvisibleLines; // lines above the window when the last one is at the bottom
	int LastLineNumber;
	int SelectionStartLine;
	int SelectionStartOffset;
	int SelectionEndLine;
	int SelectionEndOffset;
	bool Selecting;                       // the left button is down
	std::unique_ptr<char[]> SelectedText; // GetSelectedText's result
	bool AtEnd;                           // the last line is shown
};

// The view of the current channel: the text window with a scroll bar on its
// right (ScrollToEnd moves it to the last line at the next Arrange).
class TConsole final : public TWindow {
public:
	TConsole();
	int GetMinimumHeight() override;
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool MouseWheel(int x, int y, int Code) override;
	[[nodiscard]] TTalkBuffer *GetTextBuffer() const { return TextWindow->TextBuffer; }

	TTextWindow *TextWindow;
	TScrollBar *ScrollBar;
	bool ScrollToEnd;
};

// The tab of a channel. Caption is the trimmed and, where it is too wide,
// shortened title; FullCaption the trimmed one. A tab that is not active
// lights up when its buffer gets new lines (Unread), blinking for 10 actions.
class TTab final : public TElement {
public:
	TTab(const TTab &) = delete;
	TTab &operator=(const TTab &) = delete;
	TTab(const char *_Caption, TTalkBuffer *_TextBuffer, TCallback Callback);
	~TTab() override;
	// The tab height.
	int GetMinimumHeight() override { return SkinProperties().Tab.Height; }
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	void MouseHover(int x, int y) override;
	// The end of a drag.
	void ReleaseCapture(TComponent *Component) override
	{
		Dragging = false;
		TElement::ReleaseCapture(Component);
	}
	void SetActive(bool _Active);
	[[nodiscard]] const char *GetCaption() const { return Shortened ? FullCaption.get() : Caption.get(); }

	TTalkBuffer *TextBuffer; // with the console's filter list: not one owner
	int LineCount;           // the buffer's lines when the tab last looked
	std::unique_ptr<char[]> Caption;
	std::unique_ptr<char[]> FullCaption;
	int Dragging;
	int DragX;
	bool Active;
	bool Unread;
	int BlinkEnd;       // action number; 0 = not blinking
	TCallback Callback; // sent when the tab is pressed
	bool Shortened;

protected:
	void OnClose() override; // the sends and DeleteTextBuffer
};

// The tab bar: its own tabs and the two hidden windows with the tabs that do
// not fit. The child functions work on whichever of the three holds the tab
// (GetContainer); Moved is the tab being dragged, drawn last.
class TTabBar final : public TWindow {
public:
	TTabBar(const TTabBar &) = delete;
	TTabBar &operator=(const TTabBar &) = delete;
	TTabBar();
	// The original's two deletes, in its order (OnClose closes the two
	// windows' tabs first).
	~TTabBar() override
	{
		LeftTabs.reset();
		RightTabs.reset();
	}
	void InsertChild(TComponent *Child) override;
	void AppendChild(TComponent *Child) override;
	void RemoveChild(TComponent *Child) override;
	void DeleteChild(TComponent *Child) override;
	void InsertChildAfter(TComponent *Previous, TComponent *Child) override;
	void SwapChildren(TComponent *Child1, TComponent *Child2) override;
	bool IsChild(TComponent *Child) override;
	void SetSize(int Width, int Height) override;
	int GetMinimumHeight() override { return SkinProperties().Tab.Height; }
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool HelpMouseMove(int x, int y) override;
	void HandleEvent(int Event) override;
	virtual void Rearrange(TTab *_Moved);

	void SetCurrentTab(TTab *Tab);
	TWindow *GetContainer(TComponent *Child);
	TTab *FindTab(TTalkBuffer *TextBuffer);
	void ShowTab(TTab *Tab);
	void StoreOpenChannels();

	TTab *Moved;
	TTab *CurrentTab;
	std::unique_ptr<TWindow> LeftTabs; // the bar is their only owner, they are not its children
	std::unique_ptr<TWindow> RightTabs;
	bool Busy;    // Rearrange is running
	bool Changed; // the console updates its buttons when it is set

protected:
	void OnClose() override; // the two windows' tabs, then the bar's own
};

// The console. TalkMode is the volume of the default channel (1 say,
// 2 whisper, 3 yell), back to 1 after every message; History keeps the sent
// lines (HistoryPosition -1 is the edit line itself).
class TConsoleWindow : public TWindow {
public:
	TConsoleWindow(const TConsoleWindow &) = delete;
	TConsoleWindow &operator=(const TConsoleWindow &) = delete;
	TConsoleWindow();
	~TConsoleWindow() override;
	void SetSize(int Width, int Height) override;
	int GetMinimumHeight() override { return SkinProperties().ConsoleWindow.MinimumHeight; }
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool HelpMouseMove(int x, int y) override;
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	void SetActive(bool Active) const;
	void UpdateButtons();
	void OpenDefaultChannel(TTalkBuffer *TextBuffer, const char *Caption);
	TTalkBuffer *OpenChannel(int RecvMode, int SendMode, const char *Name, int Channel,
		const char *Caption);
	void AddChannel(TTalkBuffer *TextBuffer, const char *Caption);
	void CloseChannel();
	void ReportRuleViolation(const char *Question);
	[[nodiscard]] bool GetServerMessages() const;
	void SetServerMessages(bool Show);
	[[nodiscard]] TTalkBuffer *GetTextBuffer() const;
	void SendInput();
	void AddToHistory();
	void ParseInput(char **Text, char **Addressee, int *TalkMode, char *MessageSeparator) const;
	[[nodiscard]] bool SaveChannel() const;

	TTabBar *TabBar;
	TConsole *CurrentConsole;
	TTab *DefaultTab;
	TEditLine *Input;
	int SwitchTime; // new channels become current only from this action number on
	int HistoryPosition;
	// The input history, a plain TTalkBuffer. TTalkBuffer's destructor is not virtual (the original's; Talk.cpp's
	// DeleteTextBuffer destroys the derived buffers through it), so the deleter calls it as a delete would, by its
	// qualified name, which Clang's -Wdelete-non-abstract-non-virtual-dtor does not take for a virtual call.
	struct THistoryDelete {
		void operator()(TTalkBuffer *Buffer) const noexcept
		{
			Buffer->TTalkBuffer::~TTalkBuffer();
			::operator delete(Buffer);
		}
	};
	std::unique_ptr<TTalkBuffer, THistoryDelete> History;
	// The buttons shown by need (UpdateButtons) are owned by the child list while they are children and by the member
	// while they are not, so they stay raw.
	TMultiStateButton *VolumeButton;     // events 206-208
	TSkinButton *CloseButton;            // event 202
	TToggleButton *ServerMessagesButton; // event 203
	TSkinButton *IgnoreButton;           // event 204
	TSkinButton *NewChannelButton;       // event 205
	TSkinButton *MoveRightButton;        // the tab bar's event 5
	TSkinButton *MoveLeftButton;         // the tab bar's event 6
	int TalkMode;

protected:
	void OnClose() override; // the preferences and hotkeys, the buttons that are not children
};
