// GUI.h: the window system's base classes, timers, hotkeys and the skin
// properties (c:\mud\tibiaplayer\GUI.h; GUI.cpp is .\GUI.cpp).
//
// Names from the binary: SkinProperties and the member paths in its
// assertions (PlayerStates.NumberOfIcons, BuddylistWindow.Logo,
// Buttons.Texture, CheckBox.Item, StatusWindow.HitpointsBar/ManaBar),
// DrawingList (and DrawingListLength, skin, srcx, srcy from an error text),
// Parent, FirstChild, Capture, GetWidth, GetHeight, OffsetX/OffsetY (from
// the parameters _OffsetX/_OffsetY), Width, Height and the parameter names.
// The class names and the other member names are ours; see
// notes/decisions.md "GUI.cpp".

#pragma once

#include "common.h"
#include "platform/KeyInput.h"
// The graphics types TRect, TRGBColor and TFont are graphics.h's; the window
// classes and SkinProperties need them complete.
#include "graphics.h"
// TGUI (below) owns Talk.cpp's, Help.cpp's and Status.cpp's objects (issue #107).
#include "Help.h"
// SkinProperties (and its types) and the skin table, which the windows read: Skins.h's since issue #116.
#include "Skins.h" // IWYU pragma: export
#include "Status.h"
#include "Talk.h"
#include "platform/Window.h"
#include "types/Callback.h" // IWYU pragma: export (TCallback, GUI.h's until issue #110)
#include "types/FixedString.h"
#include <array>
#include <flat_map>
#include <memory>
#include <vector>

void DrawSkin(int Surface, int x, int y, int width, int height, int skin, int srcx, int srcy);
void DrawSkinClipped(int Surface, int x, int y, int width, int height, int skin, int ClipX, int ClipY,
	int ClipWidth, int ClipHeight, int skinoffsetx, int skinoffsety);

// --- Window classes -------------------------------------------------------

class TComponent;

// TCallback: types/Callback.h (issue #110), exported above.

// One entry of a component's drawing list: a skin drawn at an
// offset in the component, tiled to width x height when they are not 0. The
// constructors are inlined: new TDrawing[8] zeroes the entries
// (MapWindow.cpp), and the subclasses assign whole entries.
struct TDrawing {
	TDrawing() : skin(0), x(0), y(0), width(0), height(0), srcx(0), srcy(0) {}
	TDrawing(int _skin, int _x, int _y, int _width, int _height, int _srcx, int _srcy)
		: skin(_skin), x(_x), y(_y), width(_width), height(_height), srcx(_srcx), srcy(_srcy) {}
	int skin;
	int x;
	int y;
	int width;
	int height;
	int srcx;
	int srcy;
};

// The abstract base class of every window element. Slots with _purecall in
// the binary are pure; the child functions throw "not possible" (only
// TWindow has children).
class TComponent {
public:
	TComponent(const TComponent &) = delete;
	TComponent &operator=(const TComponent &) = delete;
	// Inlined into the constructors of TWindow and TElement.
	TComponent() : DrawingList(nullptr), DrawingListLength(0), Parent(nullptr), Next(nullptr), OffsetX(0),
				   OffsetY(0), Width(0), Height(0) {}
	virtual ~TComponent(); // noexcept (issue #108)
	virtual void InsertChild(TComponent *Child);
	virtual void AppendChild(TComponent *Child);
	virtual void RemoveChild(TComponent *Child);
	virtual void DeleteChild(TComponent *Child);
	virtual void InsertChildAfter(TComponent *Previous, TComponent *Child);
	virtual void SwapChildren(TComponent *Child1, TComponent *Child2);
	virtual bool IsChild(TComponent *Child);
	virtual TComponent *GetFirstChild() { TERROR("not possible"); }
	virtual TComponent *GetLastChild() { TERROR("not possible"); }
	virtual TComponent *GetNext() { return Next; }
	virtual TComponent *GetPrevious();
	virtual TComponent *GetParent() { return Parent; }
	virtual void SetSize(int _Width, int _Height);
	virtual void SetPosition(int _OffsetX, int _OffsetY);
	virtual int GetMinimumWidth() { return 0; }
	virtual int GetMinimumHeight() { return 0; }
	virtual void Draw(int Surface, int x, int y);
	virtual void Arrange() {} // rebuilds the drawing list in subclasses
	virtual bool MouseMove(int x, int y) = 0;
	virtual bool LeftButtonDown(int x, int y, int Flags) = 0;
	virtual bool LeftButtonUp(int x, int y, int Flags) = 0; // Flags 0x200: both buttons
	virtual bool LeftButtonDoubleClick(int x, int y, int Flags) = 0;
	virtual bool RightButtonUp(int x, int y, int Flags) = 0;
	virtual bool MouseWheel(int x, int y, int Direction) = 0; // 1 up, 2 down, 6 left button held
	virtual void MouseHover(int x, int y) = 0;
	virtual bool HelpMouseMove(int x, int y) = 0; // action state 9
	virtual void KeyPress([[maybe_unused]] int Key) {}
	virtual void HandleEvent([[maybe_unused]] int Event) {} // see TCallback
	virtual void PositionChanged() {}                       // the parent moved
	virtual void SetCapture(TComponent *Component) = 0;
	virtual void ReleaseCapture(TComponent *Component) = 0;
	virtual void StopDragging() = 0;

	// The work of the original's destructors that can fail or acts outside the component (issue #108): the Send
	// functions, the preferences, the hotkeys, the assertions, DeleteTextBuffer. Close does it, once, in the order the
	// original's destructors did (a class's own part first, then its bases', a window's children in the order of its
	// list, an owned part that is not a child where the original's destructor deleted it); the destructors free the
	// memory only and are noexcept. Each delete of a component closes it first (DeleteComponent, below; TWindow's
	// DeleteChild). An exception of a close passes to the caller as the original's destructor's did, and the component
	// is left allocated and marked closed (the original had freed it): see notes/ownership.md, section 7.
	void Close();

	[[nodiscard]] int GetWidth() const { return Width; }   // "Child->GetWidth()"
	[[nodiscard]] int GetHeight() const { return Height; } // "Child->GetHeight()"

	std::unique_ptr<TDrawing[]> DrawingList; // allocated by the subclasses (issue #58: the list's only owner)
	int DrawingListLength;

private:
	// Close has run (issue #108; not in the original's layout). Here, in the padding after DrawingListLength on x64,
	// so that no class's size or member offsets change (at the end the Itanium ABI would put derived members into its
	// tail padding, which the golden tests' fake elements do not mirror).
	bool Closed = false;

public:
	TComponent *Parent;
	TComponent *Next; // the next child of Parent
	int OffsetX;      // relative to Parent
	int OffsetY;
	int Width;
	int Height;

protected:
	// A class's own part of the close (issue #108), overridden by the classes whose destructors did such work; each
	// override ends with its base's, as a destructor ends with its base's. Not one of the original's virtual functions.
	virtual void OnClose() {}
	// The rest of a close after an exception of a class's own part: the original's destructor went on with its bases'
	// destructors while the exception passed, and a second exception there ended the program (std::terminate), which
	// noexcept keeps.
	template <class F> static void CloseWhileUnwinding(F &&Rest) noexcept { Rest(); }
	// A component of a class with a close of its own that is destroyed without it (a delete without Close: a missed
	// close, its work not done) writes a line to the developer log (error level).
	void CheckClosed(const char *Class) const noexcept;
};

// Closes the component and deletes it (issue #108): the original's `delete Component`. Nothing for NULL. When the
// close throws, the component is not deleted and the exception passes on.
void DeleteComponent(TComponent *Component);

// A component without children. It takes every mouse event; a double
// click is a second button press. The inline slots are kept in earlier
// objects (ConsoleWindow.obj and the Communication/ConsoleWindow gap).
class TElement : public TComponent {
public:
	TElement();
	bool MouseMove([[maybe_unused]] int x, [[maybe_unused]] int y) override { return true; }
	bool LeftButtonDown([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return true; }
	bool LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return true; }
	bool LeftButtonDoubleClick(int x, int y, int Flags) override { return LeftButtonDown(x, y, Flags); }
	bool RightButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags) override { return true; }
	bool MouseWheel([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Direction) override { return true; }
	void MouseHover([[maybe_unused]] int x, [[maybe_unused]] int y) override {}
	bool HelpMouseMove(int x, int y) override;
	void SetCapture(TComponent *Component) override;
	void ReleaseCapture(TComponent *Component) override;
	void StopDragging() override {}
};

// A component with children; the base of the main window, the dialogs and
// the context menus. Children are a singly linked list through Next;
// Capture is the child that gets every mouse event while it is set (a
// drag).
class TWindow : public TComponent {
public:
	TWindow(const TWindow &) = delete;
	TWindow &operator=(const TWindow &) = delete;
	TWindow();
	~TWindow() override;
	void InsertChild(TComponent *Child) override;
	void AppendChild(TComponent *Child) override;
	void RemoveChild(TComponent *Child) override;
	void DeleteChild(TComponent *Child) override;
	void InsertChildAfter(TComponent *Previous, TComponent *Child) override;
	void SwapChildren(TComponent *Child1, TComponent *Child2) override;
	bool IsChild(TComponent *Child) override;
	TComponent *GetFirstChild() override { return FirstChild; }
	TComponent *GetLastChild() override;
	void SetPosition(int _OffsetX, int _OffsetY) override;
	void Draw(int Surface, int x, int y) override;
	void Arrange() override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool LeftButtonDoubleClick(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	bool MouseWheel(int x, int y, int Direction) override;
	void MouseHover(int x, int y) override;
	bool HelpMouseMove(int x, int y) override;
	void SetCapture(TComponent *Component) override;
	void ReleaseCapture(TComponent *Component) override;
	void StopDragging() override;
	virtual TComponent *GetCapture() { return Capture; }
	virtual void SetChildPosition(TComponent *Child, int x, int y);
	// The area is a 16-byte block passed by value (every caller builds it
	// with sub esp,0x10 and four stores, issue #27).
	virtual void SetChildArea(TComponent *Child, TRect Area);
	virtual void Fit(bool Notify) { FitToChildren(TRect(0, 0, 0, 0), Notify); }
	// The margins are one 16-byte block passed by value (the callers, Fit and
	// ContextMenus.cpp, build it with sub esp,0x10): a TRect whose x, y,
	// Width and Height are the left, top, right and bottom margins.
	virtual void FitToChildren(TRect Margin, bool Notify);
	// The area is a 16-byte block passed by value as well.
	virtual void FitToChild(TComponent *Child, TRect Area, bool Notify);
	virtual void ClipChildren(bool Strict);

	TComponent *FirstChild;
	TComponent *Capture;

protected:
	void OnClose() override; // the children, first to last (~TWindow's order; issue #108)
};

// --- Modifier flags -------------------------------------------------------

// The flags of a Key (KeyPress, the hotkeys) and of the mouse functions' Flags: the original's names (MainFrm.h), the
// values of platform/KeyInput.h. Enumerators on purpose: naming an inline constexpr int in a source changed the code
// generated elsewhere in its object file (measured in Release; these constants must leave the executable as it was).
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants, as Objects.h's
	KEY_SHIFT = patella::platform::KeyFlagShift,
	KEY_CONTROL = patella::platform::KeyFlagControl,
	KEY_ALT = patella::platform::KeyFlagAlt,
	// Not a key: the Flags of the left button release that TGUI sends when the second of both mouse buttons goes up
	// (Control.cpp's LeftButtonUp and RightButtonUp). The bit is KEY_SHIFT's, shared by the two meanings.
	BOTH_BUTTONS_RELEASED = KEY_SHIFT,
	// The shared bit where a test cannot tell the two meanings apart: a window's LeftButtonUp, and the RightButtonUp such
	// a LeftButtonUp passes a release with flags on to, act alike for Shift held and for both buttons released.
	SHIFT_OR_BOTH_BUTTONS = KEY_SHIFT | BOTH_BUTTONS_RELEASED,

	// The key codes. Below the modifier flags a Key holds a character of the client's code page (written as the
	// character: Control with I is KEY_CONTROL | 'I') or one of these codes. The values are platform/KeyInput.h's, whose
	// TranslateKey and KeyCharacter make them; the original's names of them are not known, these are the keys' names. A
	// hotkey's letter is upper case (CheckHotkey makes it so before it looks), a letter compared with KeyPress's Key is
	// the case the key gave: with Control and without Alt lower case (CMainFrame::OnChar). In the flags' enumeration,
	// because C++26 does not allow a bitwise operation between the enumerators of two (GCC refuses KEY_SHIFT | KEY_TAB).
	// The control characters of their keys:
	KEY_BACKSPACE = patella::platform::KeyCodeBackspace,
	KEY_TAB = patella::platform::KeyCodeTab,
	KEY_RETURN = patella::platform::KeyCodeReturn, // also the keypad's Enter
	KEY_ESCAPE = patella::platform::KeyCodeEscape,
	// The keys without a character. Every code is KEY_F1 or above, every character below it.
	KEY_F1 = patella::platform::KeyCodeF1,
	KEY_F2 = patella::platform::KeyCodeF2,
	KEY_F3 = patella::platform::KeyCodeF3,
	KEY_F4 = patella::platform::KeyCodeF4,
	KEY_F5 = patella::platform::KeyCodeF5,
	KEY_F6 = patella::platform::KeyCodeF6,
	KEY_F7 = patella::platform::KeyCodeF7,
	KEY_F8 = patella::platform::KeyCodeF8,
	KEY_F9 = patella::platform::KeyCodeF9,
	KEY_F10 = patella::platform::KeyCodeF10,
	KEY_F11 = patella::platform::KeyCodeF11,
	KEY_F12 = patella::platform::KeyCodeF12,
	// The navigation keys, the keypad's too where it has no code of its own.
	KEY_UP = patella::platform::KeyCodeUp,
	KEY_LEFT = patella::platform::KeyCodeLeft,
	KEY_RIGHT = patella::platform::KeyCodeRight,
	KEY_DOWN = patella::platform::KeyCodeDown,
	KEY_INSERT = patella::platform::KeyCodeInsert,
	KEY_DELETE = patella::platform::KeyCodeDelete,
	KEY_HOME = patella::platform::KeyCodeHome,
	KEY_END = patella::platform::KeyCodeEnd,
	KEY_PAGE_UP = patella::platform::KeyCodePageUp,
	KEY_PAGE_DOWN = patella::platform::KeyCodePageDown,
	// The keypad's diagonal keys, which TMainWindow::KeyPress walks diagonally with.
	KEY_PAD_END = patella::platform::KeyCodePadEnd,            // keypad 1
	KEY_PAD_PAGE_DOWN = patella::platform::KeyCodePadPageDown, // keypad 3
	KEY_PAD_HOME = patella::platform::KeyCodePadHome,          // keypad 7
	KEY_PAD_PAGE_UP = patella::platform::KeyCodePadPageUp,     // keypad 9
	// The mask of a Key's character or code, without the modifier flags.
	KEY_CODE_MASK = 0x1ff,
};

// --- Timers ---------------------------------------------------------------

// A countdown registered in the timer list while it runs. UpdateTimers
// counts it down and sends its callback on every update, the last time when
// it reaches 0.
class TTimer {
public:
	TTimer(int Time, TCallback Callback); // Time in tenths of a second
	virtual ~TTimer();
	[[nodiscard]] int GetTime() const; // tenths of a second left
	[[nodiscard]] bool IsExpired() const;
	void Update(int elapsed);
	int Remaining; // milliseconds
	bool Active;
	TCallback Callback;
};

void AddTimer(TTimer *Timer);
void RemoveTimer(TTimer *Timer);
void UpdateTimers();

// --- Hotkeys --------------------------------------------------------------

void InitHotkeys();
void ExitHotkeys();
void AddHotkey(int Key, TCallback Callback);
void RemoveHotkey(int Key);
bool CheckHotkey(int Key);

// --- Skin properties ------------------------------------------------------

// SkinProperties and its types are Skins.h's since issue #116 (TGraphics' TSkins); InitSkinProperties, which fills it,
// stays GUI.cpp's.

void InitSkinProperties();

class TMainWindow;

namespace patella {

class TNetwork;       // Network.h
class TCommunication; // Communication.h

// The GUI's state (issue #107, M7; notes/decisions.md, "#104: TClient and the subsystem objects" and "#107"), the
// sixth of TClient's subsystems: Talk.cpp's TTalk, Help.cpp's THelp and Status.cpp's TInteraction, and the data of
// GUI.cpp (the timers and hotkeys), ContentWindows.cpp (the two mouse-over texts), Control.cpp (the window, the
// callbacks, the mouse's last move, the frame counter's switch) and MainWindow.cpp (ModalWindow, MainWindow and the
// callbacks), in the original's address order with the same initial values. GUI.cpp's timer and hotkey functions,
// Control.cpp's and MainWindow.cpp's functions (issue #118) are its members of the same names; the free functions
// forward to them. SkinProperties is TGraphics' since issue #116 (with Skins.cpp's table, TSkins: Skins.h).
//
// Control.cpp, the controller above the windows, is TGUI's since issue #118: the windows' calls into it (the engine
// buttons of the main window's options, MainWindow.cpp, and the graphics options' fullscreen switch, Dialogs.cpp) are
// calls of TGUI's members, and TGUI takes the earlier subsystems Control.cpp reads (TNetwork's reconnect, TCommunication's
// packet ring and outfit request) by constructor reference.
//
// MainWindow owns the main window (issue #108; raw until then): InitMainWindow makes it, ExitMainWindow closes and
// frees it, at the original's new and delete; CPlayerApp::FatalError abandons it before exit(1) (AbandonWindows), so
// TClient's static destruction never destroys it. ModalWindow keeps the original's raw pointer (below): Control.cpp
// makes each dialog into it and closes and deletes the one it ends (DeleteComponent). notes/decisions.md, "#108".
class TGUI {
public:
	TGUI(TNetwork &Network, TCommunication &Communication); // MainWindow.cpp (TMainWindow is complete there)
	TGUI(const TGUI &) = delete;
	TGUI &operator=(const TGUI &) = delete;
	TGUI(TGUI &&) = delete;
	TGUI &operator=(TGUI &&) = delete;
	~TGUI();

	// GUI.cpp's timers and hotkeys.
	void AddTimer(TTimer *Timer);
	void RemoveTimer(TTimer *Timer);
	void UpdateTimers();
	void InitHotkeys();
	static void ExitHotkeys(); // empty (as TCommunication::ExitCommunication, issue #105)
	void AddHotkey(int Key, TCallback Callback);
	void RemoveHotkey(int Key);
	bool CheckHotkey(int Key);

	// Control.cpp's (issue #118; Control.h has the free functions of the same names, which forward here). Their calls
	// into the other files (GUI.cpp's and MainWindow.cpp's included) are those files' free functions, which the golden
	// Control and Communication tests fake (commworld.cpp's WorldFakes); their calls of each other are member calls.
	void Resize(int Width, int Height);
	void CheckSurface(bool Force);
	static void ChangeEngine(int engine);
	void Redraw(bool Force);
	void EnterGame() const;
	void LeaveGame();
	void MouseMove(int x, int y);
	void LeftButtonDown(int x, int y, int Flags);
	void LeftButtonUp(int x, int y, int Flags);
	void LeftButtonDoubleClick(int x, int y, int Flags);
	void RightButtonDown(int x, int y, int Flags);
	void RightButtonUp(int x, int y, int Flags);
	void MouseWheel(int x, int y, int Direction);
	void KeyPress(int Key);
	[[nodiscard]] bool CloseRequest() const;
	static void LinkCopyWarning();
	void ConnectionEstablished();
	void ReceivePackets();
	void ConnectionClosed();
	void TimerEvent() const;
	void ProcessEvents();
	void InitWindow(platform::TWindowHandle Window, void (*ExitProgram)(), void (*SetDisplayMode)(int Mode), void (*OpenURL)(const char *URL));
	static void InitControl(bool Gamemaster);
	static void ExitControl();
	[[nodiscard]] std::array<uint8, 10> GetLastPacketTypes() const;

	// MainWindow.cpp's (issue #118; MainWindow.h has the free functions).
	void InitMainWindow(int Width, int Height, void (*ExitProgram)(), void (*SetDisplayMode)(int Mode),
		void (*OpenURL)(const char *URL));
	void ExitMainWindow();

	// The earlier subsystems Control.cpp reads (issue #118).
	TNetwork &Network;
	TCommunication &Communication;

	// The files' objects.
	TTalk Talk;               // Talk.cpp
	THelp Help;               // Help.cpp
	TInteraction Interaction; // Status.cpp's user interaction

	// Data, in the original's address order; public where another file or a golden test reads it (named).
	TFixedString<100> AutomapMouseOverText{}; // ContentWindows.cpp: a mark's text (TAutomapWindow::MouseHover)
	TFixedString<256> SkillMouseOverText{};   // ContentWindows.cpp: "You have %d percent to go"
	// Control.cpp's; the golden Control test reads and sets them.
	int LastMouseMove{}; // timer tick of the last mouse move, 0 after a click
	void (*URLCallback)(const char *URL){};
	void (*DisplayModeCallback)(int Mode){};
	void (*ExitCallback)(){}; // stored but not used
	platform::TWindowHandle WindowHandle{};
	bool ShowFPS{}; // toggled by key 0x907

private:
	// GUI.cpp's (SkinProperties is TGraphics' since issue #116: Skins.h).
	// NumberOfHotkeys, the used entries of Hotkeys: Hotkeys.size() (issue #100).
	// The original's vector<TTimer *>(0, 0, 2, NULL): AddTimer takes the first
	// NULL entry from 0 or appends, RemoveTimer sets the entry to NULL, UpdateTimers goes through the entries in
	// order. Its min stays 0 and max + 1 is the size, so a std::vector (issue #100; notes/decisions.md, "#100").
	std::vector<TTimer *> Timers;
	// The original's vector<THotkey>(0, 50, 10) with NumberOfHotkeys: a lookup by
	// key, each key at most once (AddHotkey replaces its callback), the order never read (RemoveHotkey moves the
	// last entry into the gap). A std::flat_map from the key to the callback (issue #100).
	std::flat_map<int, TCallback> Hotkeys;
	int LastTime{};     // UpdateTimers' static
	bool LastTimeSet{}; // its guard

public:
	// MainWindow.cpp's: the callbacks InitMainWindow stores, the modal window and the main window.
	void (*OpenURLCallback)(const char *URL){};
	void (*SetDisplayModeCallback)(int Mode){};
	void (*ExitProgramCallback)(){};
	// The dialog on top. Raw (one owner, but not a std::unique_ptr; issue #108): MouseMove's help bubble leaves it
	// pointing to the bubble it deleted when the new bubble or its Place throws (issue #36), which the golden Control
	// test compares ("deleted ..."), and an owner cannot hold a freed window. Never destroyed at exit (a raw pointer).
	TWindow *ModalWindow{};
	std::unique_ptr<TMainWindow> MainWindow; // issue #108: its owner

	// Issue #108: leaves the main window allocated, neither closed nor freed, and MainWindow empty: what the
	// original's global was at exit(1). Called by CPlayerApp::FatalError before it, so that TClient's static
	// destruction does not run the window's destructors (nor, through ~TWindow, close its children: the leave-channel
	// and close-container packets and the preferences the original never sent or wrote on a fatal error).
	void AbandonWindows() noexcept;
};

// The transition's access path (issue #107; notes/decisions.md, "#104: TClient and the subsystem objects"): the one
// TClient's TGUI (Client.cpp). It goes when no free function needs it.
namespace bridge {
TGUI &GUI();
} // namespace bridge

} // namespace patella


inline bool TElement::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	SetHelpHandle(HELP_NONE);
	return false;
}
