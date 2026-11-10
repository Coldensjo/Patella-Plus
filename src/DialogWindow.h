// DialogWindow.h: the columns of windows on the right of the game window.
//
// TDialogWindow holds windows one above the other: MainWindow.cpp makes two,
// SidePanel for the inventory and the other side-bar windows and DialogWindow
// for the containers and the skill, battle, VIP and trade windows. A child
// can be dragged to another place in the column; Rearrange keeps the children
// in the column's height with one of six strategies. The free space below the
// last child is filled with skins.
//
// TContentWindow is the base of every window in a column: an identifier, an
// optional tiled background (skin 7) and the dragging. ContentWindows.cpp's
// side-bar windows derive from it directly. TSubWindow is a window with a
// body (the content, made by ContentWindows.cpp), a frame of eight skins, a
// title, an icon, the buttons "up", "minimize" and "close" and optionally
// two resizers (DialogElements.cpp).
//
// Names from the binary: TDialogWindow (MainWindow.cpp's
// "DialogWindow!=NULL" and the file name), HasContentWindow (which gives the
// class name TContentWindow and GetContentWindow), GetBody, Body, _Body,
// _Identifier, IsMinimized, Search, Work, DeltaHeight, Strategy and the
// parameter names in the catch blocks (Surface, Identifier, x, y, Width,
// Height, Event). The other names are ours (notes/decisions.md,
// "DialogWindow.cpp").

#pragma once

#include "GUI.h"
#include "graphics.h"
#include <climits>

class TResizer;    // DialogElements.h
class TSkinButton; // DialogElements.h

// The dialog window's identifier of the trade window ("HasContentWindow(
// TRADE_WINDOW)", MainWindow.cpp). The skill, battle and VIP windows are 6, 7 and 9, a
// container's window is 0x40 plus the container number. MainWindow.h's until issue #118: here, where both of its
// users (MainWindow.cpp and ContentWindows.cpp's CreateTradeWindow) find it without ContentWindows.cpp including
// MainWindow.h.
inline constexpr int TRADE_WINDOW = 8;
// A container's window: this plus the container number. An enumerator for the reason in GUI.h.
enum : int { CONTAINER_WINDOW = 0x40 }; // NOLINT(cppcoreguidelines-use-enum-class): an int constant
// The other identifiers, named as TRADE_WINDOW is, after the windows' classes and creators (ContentWindows.h). The
// first five are the side panel's fixed windows, the others the dialog window's, which the buttons open and close.
// They are also the numbers of Tibia.cfg's OpenDialogs and, for the three with a stored height, ContentWindow.
// Enumerators for the reason in GUI.h.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	INVENTORY_WINDOW = 1,
	SMALL_INVENTORY_WINDOW = 2, // the minimised inventory, in the place of INVENTORY_WINDOW
	AUTOMAP_WINDOW = 3,
	STATUS_WINDOW = 4, // the hit point and mana bars
	BUTTONS_WINDOW = 5,
	SKILLS_WINDOW = 6,
	BATTLE_WINDOW = 7,
	BUDDYLIST_WINDOW = 9, // the VIP list
};

// A window in a TDialogWindow. Identifier: 1 and 2 the two inventory
// windows, 3 the automap, 4 the status bars, 5 the buttons, 6 skills, 7 battle, TRADE_WINDOW
// (8, above), 9 VIP, 0x40 plus the number of a container.
class TContentWindow : public TWindow {
public:
	TContentWindow(int _Identifier, bool _Background);
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	void ReleaseCapture(TComponent *Component) override;

	int Identifier;  // +0x2c
	int BackgroundY; // +0x30, the background skin's row at the top edge
	bool Background; // +0x34, draw the tiled skin 7 behind the children
	bool Dragging;   // +0x35
	int DragX;       // +0x38, where the drag started (window coordinates)
	int DragY;       // +0x3c
};

// The content of a TSubWindow (ContentWindows.h's bodies derive from it; here since issue #118, so that
// DialogWindow.cpp, which calls its virtual functions, does not include ContentWindows.h). Its constructor and its virtual functions
// are inline; no vtable of its own is left (the constructors of the
// subclasses store theirs right after Minimized).
class TSubWindowBody : public TWindow {
public:
	TSubWindowBody() { Minimized = false; }
	// A double click on the body is taken (it does not minimize the window);
	// the VIP body overrides it.
	bool LeftButtonDoubleClick([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return true; }
	virtual void Minimize() { Minimized = true; }  // TSubWindow::Minimize
	virtual void Maximize() { Minimized = false; } // TSubWindow::Maximize
	virtual bool IsMinimized() { return Minimized; }
	virtual int GetInitialHeight() = 0; // the creators size the window with it
	virtual int GetMaximumHeight() { return INT_MAX; }
	virtual int GetContentHeight() = 0;

	bool Minimized; // +0x2c
};

// Not in the original (issue #118): TSubWindow::Arrange's call of a container body's UpdateUpButton
// (TContainerWindowBody, ContentWindows.h), defined in ContentWindows.cpp where that class is complete.
void UpdateContainerUpButton(TSubWindowBody *Body);

// A content window with a body and a frame (MainWindow.cpp's containers and
// the skill, battle, VIP and trade windows).
class TSubWindow final : public TContentWindow {
public:
	TSubWindow(const TSubWindow &) = delete;
	TSubWindow &operator=(const TSubWindow &) = delete;
	TSubWindow(int _Identifier, TSubWindowBody *_Body, bool _Resizable, bool _Background);
	~TSubWindow() override;
	void SetSize(int Width, int Height) override;
	int GetMinimumHeight() override;
	void Arrange() override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonDoubleClick(int x, int y, int Flags) override;
	void HandleEvent(int Event) override;
	virtual void Minimize();
	virtual void Maximize();
	void ShowUpButton();
	void HideUpButton();
	void SetUpButton(TComponent *Button);
	void SetCloseButton(TComponent *Button);
	void SetMinimizeButton(TSkinButton *Button);
	void SetIcon(TComponent *_Icon);
	void SetTitle(TComponent *_Title);
	static int GetFrameHeight(); // the title bar and the bottom edge
	// Inline ("Window->GetBody()!=NULL", MainWindow.cpp).
	[[nodiscard]] TSubWindowBody *GetBody() const { return Body; }

	// The parts are owned by the child list while they are children and by the member while they are not (a minimized
	// window's, a hidden "up" button), so they stay raw (notes/ownership.md, section 7; issue #108).
	TSubWindowBody *Body;        // +0x44
	TResizer *Resizer1;          // +0x48, area SubWindow.Resizer1
	TResizer *Resizer2;          // +0x4c, area SubWindow.Resizer2
	TComponent *UpButton;        // +0x50, a container's "up" button, shown while it has a parent
	TComponent *CloseButton;     // +0x54, event 10
	TSkinButton *MinimizeButton; // +0x58, event 9
	TComponent *Icon;            // +0x5c
	TComponent *Title;           // +0x60
	bool IsMinimized;            // +0x64
	int RestoreHeight;           // +0x6c, the height before Minimize

protected:
	void OnClose() override; // the destructor's preferences and the parts that are not children (issue #108)
};

// A column of content windows.
class TDialogWindow final : public TWindow {
public:
	TDialogWindow();
	void InsertChild(TComponent *Child) override;
	void AppendChild(TComponent *Child) override;
	void DeleteChild(TComponent *Child) override;
	void InsertChildAfter(TComponent *Previous, TComponent *Child) override;
	void SetSize(int Width, int Height) override;
	int GetMinimumWidth() override;
	int GetMinimumHeight() override;
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	// Strategy: 1 a child is dragged (Work, NULL when the drag ends), 2 a
	// child grew, 4 a child (Work) shrank, 8 the column's height changed, 16
	// a new child (Work) is appended, 32 a child was removed.
	virtual void Rearrange(int Strategy, TComponent *Work);
	TContentWindow *GetContentWindow(int Identifier);
	void StoreOpenDialogs();
	// Inline ("!DialogWindow->HasContentWindow(TRADE_WINDOW)", MainWindow.cpp).
	bool HasContentWindow(int Identifier) { return GetContentWindow(Identifier) != nullptr; }
	void PositionChildren(bool SkipDragged);
	void FitChildren();
	bool Shrink();
	void DragChild(TComponent *Work);
	void MakeRoom(TComponent *Work);
	void Grow(TComponent *Work);
	int GetFreeHeight();

	TRect Area;            // +0x2c, where the children go (x, y, width, height)
	TComponent *Dragged;   // +0x3c
	bool Busy;             // +0x40, Rearrange is running
	int FreeHeight;        // +0x44, GetFreeHeight's last result while UpdateFreeHeight
	bool UpdateFreeHeight; // +0x48
};
