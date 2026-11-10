// MainWindow.h: the main game window, the window that fills the client
// area and holds every other window except the modal one.
//
// TMainWindow has two modes. Out of the game it holds the start window
// (TStartWindow): the background picture and a frame with the buttons "Enter
// Game", "Access Account", "Options", "Info" and "Exit Game". In the game it
// holds the map frame (MapWindow.h) on the left above the console, a
// splitter between the two (TSplitter) that moves the console's top edge,
// and on the right the side bar (a frame around the panel of the side-bar
// windows) above the dialog window that holds the containers and the skill,
// battle, VIP and trade windows. The keys and hotkeys of the game window are
// handled here (walking, turning and the Ctrl shortcuts).
//
// Names from the binary: MainWindow and ModalWindow (assertions in
// Control.cpp), DialogWindow ("DialogWindow!=NULL"), GetBody, OldWindow,
// TRADE_WINDOW, ACTIONSTATE_NONE and the parameter names in the catch blocks
// (Width, Height, Surface, Key, Event, ConsoleHeight, dx, dy, Container,
// Identifier, x, y). The class names and the other names are ours
// (notes/decisions.md, "MainWindow.cpp").

#pragma once

#include "GUI.h"

class TMapFrame;      // MapWindow.h
class TBorder;        // DialogElements.h
class TDialogWindow;  // DialogWindow.h
class TConsoleWindow; // ConsoleWindow.h
class TStartWindow;
class TSplitter;

// TRADE_WINDOW is DialogWindow.h's since issue #118 (ContentWindows.cpp no longer includes this header).

class TMainWindow : public TWindow {
public:
	TMainWindow(int Width, int Height);
	~TMainWindow() override;
	void Draw(int Surface, int x, int y) override;
	void SetSize(int Width, int Height) override;
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;
	void ChangeMode(bool InGame);
	void ResizeConsole(int ConsoleHeight) const;
	[[nodiscard]] int GetConsoleWindowHeight() const;
	void Walk(int dx, int dy, bool Diagonal);
	void OpenContainer(int Container) const;
	void CloseContainer(int Container) const;
	void OpenTrade() const;
	void UpdateTrade() const;
	void CloseTrade() const;
	void OpenContentWindow(int Identifier) const;
	void CloseContentWindow(int Identifier) const;
	void SetConsoleActive(bool Active) const;

	// The children; all NULL in the mode that does not use them.
	TMapFrame *MapFrame;
	TBorder *SideBar;            // the frame around SidePanel
	TDialogWindow *DialogWindow; // containers, skills, battle, VIP and trade
	TDialogWindow *SidePanel;    // inventory and the other side-bar windows
	TConsoleWindow *Console;
	TSplitter *Splitter;
	TStartWindow *StartWindow;
	bool InGame;

protected:
	void OnClose() override; // the destructor's preferences (issue #108)
};

// The window behind the start dialogs.
class TStartWindow final : public TWindow {
public:
	TStartWindow();
	void Draw(int Surface, int x, int y) override;
	void SetSize(int Width, int Height) override;
	void HandleEvent(int Event) override;

	TBorder *Frame; // around the buttons
};

// The bar between the map and the console: dragging it changes the
// console's height.
class TSplitter final : public TElement {
public:
	TSplitter();
	void Arrange() override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	void ReleaseCapture(TComponent *Component) override
	{
		Dragging = false;
		TElement::ReleaseCapture(Component);
	}

	bool Dragging;
	int DragX; // where the button was pressed
	int DragY;
};

// ModalWindow and MainWindow are TGUI's members (GUI.h, issue #107):
// patella::bridge::GUI().ModalWindow and .MainWindow.

void InitMainWindow(int Width, int Height, void (*ExitProgram)(), void (*SetDisplayMode)(int Mode),
	void (*OpenURL)(const char *URL));
void ExitMainWindow();
