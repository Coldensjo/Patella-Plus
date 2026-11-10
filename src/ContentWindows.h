// ContentWindows.h: the windows of the side bar and of the dialog column
// (c:\mud\tibiaplayer\ContentWindows.h).
//
// MainWindow.cpp fills two TDialogWindow columns (DialogWindow.h) with these
// windows. The side panel holds the side-bar windows, TContentWindows with
// fixed contents: the inventory (identifier 1, a plain TContentWindow with a
// TInventoryView and a TCombatControls, made by CreateInventoryWindow), the
// small inventory (2, TSmallInventoryWindow), the automap (3,
// TAutomapWindow: a TAutomapView, a TCompassRose and buttons), the status
// bars (4, TStatusWindow) and the buttons that open the skill, battle and
// VIP windows (5, TButtonsWindow). The dialog column holds TSubWindows with a
// body (TSubWindowBody): the containers (0x40 plus the container number), the
// skills (6), the battle list (7), the trade (8, TRADE_WINDOW) and the VIP
// list (9). The Create* functions build those windows with their buttons,
// icon and title and size them before MainWindow.cpp hands them to
// Rearrange(16).
//
// Names from the binary: _CheckObject, _RoseSkin, NumberOfSkins, islarge,
// _Container, ScrollbarX, EventComponent, NumberOfLine, Lines, Value,
// Progress, Position, SkinProperties.StatusWindow and PlayerStates (the class
// names TStatusWindow and TPlayerStates), BuddylistWindow, RadioButtonGroup
// (DialogElements.cpp's assertion) and the catch-block parameters (Surface, x,
// y, Width, Height, Event, Direction, Code). The class, member and function
// names are ours; see notes/decisions.md, "ContentWindows.cpp".

#pragma once

#include "common.h" // IWYU pragma: keep (first: it configures the CRT and MFC for the headers after it)
#include "GUI.h"
#include "DialogWindow.h"
#include "Map.h" // TMark
#include "DialogElements.h"
#include "types/FixedString.h"
#include <memory>

class TSkillContextMenu; // ContextMenus.h

// --- The side-bar windows -------------------------------------------------------

// Identifier 5: the buttons that open and close the skill, battle and VIP
// windows of the dialog column (_CheckObject, whose windows they show as
// pressed) and "Logout". Their events go to the main window (14 skills, 15
// battle, 22 VIP, 507 logout).
class TButtonsWindow : public TContentWindow {
public:
	TButtonsWindow(TDialogWindow *_CheckObject, TWindow *Receiver);
	void Draw(int Surface, int x, int y) override;
	bool HelpMouseMove(int x, int y) override;

	TDialogWindow *CheckObject;      // +0x44
	TTextToggleButton *SkillsButton; // +0x48
	TTextToggleButton *BattleButton; // +0x4c
	TTextToggleButton *BuddyButton;  // +0x50
	TTextButton *LogoutButton;       // +0x54
};

// The map of the automap window: 106x106 pixels around ViewX/ViewY/ViewZ of
// its parent, one automap point per Zoom/2 pixels.
class TAutomapView final : public TElement {
public:
	void Draw(int Surface, int x, int y) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	virtual TMark GetMark(int x, int y);
};

// The compass rose of the automap window: a click on one of its five areas
// (the middle and the four directions) centres or scrolls the map.
class TCompassRose final : public TElement {
public:
	TCompassRose(int _RoseSkin);
	void Draw(int Surface, int x, int y) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	void MouseHover(int x, int y) override;

	int RoseSkin; // +0x24
};

// Identifier 3: the automap with its view, the compass rose and the buttons
// for the floor and the zoom. The shown area follows the player (Centred)
// until it is scrolled; Changed asks the view to redraw.
class TAutomapWindow : public TContentWindow {
public:
	TAutomapWindow();
	~TAutomapWindow() override;
	bool HelpMouseMove(int x, int y) override;
	void MouseHover(int x, int y) override;
	void Arrange() override;
	void HandleEvent(int Event) override;
	void Scroll(int Direction);

	TAutomapView *View;           // +0x44
	TTextButton *CentreButton;    // +0x48, event 106
	TSkinButton *FloorUpButton;   // +0x4c, event 3
	TSkinButton *FloorDownButton; // +0x50, event 4
	TSkinButton *ZoomInButton;    // +0x54, event 107
	TSkinButton *ZoomOutButton;   // +0x58, event 108
	TCompassRose *CompassRose;    // +0x5c
	bool Changed;                 // +0x60, the view must be redrawn
	bool Centred;                 // +0x61, the view follows the player
	int ViewX;                    // +0x64, the centre of the view
	int ViewY;                    // +0x68
	int ViewZ;                    // +0x6c
	int PlayerX;                  // +0x70, the player's position when the view was scrolled
	int PlayerY;                  // +0x74
	int PlayerZ;                  // +0x78
	int Zoom;                     // +0x7c, 1, 2, 4 or 8 (Preferences.cpp's AutomapZoom)

protected:
	void OnClose() override; // the destructor's zoom and hotkeys (issue #108)
};

// The capacity and the soul points of the inventory (CapacityLabel, SoulLabel).
class TCapacityLabel final : public TElement {
public:
	// The constructor and the destructor are the implicit ones.
	void Draw(int Surface, int x, int y) override;
};
class TSoulLabel final : public TElement {
public:
	// The constructor and the destructor are the implicit ones.
	void Draw(int Surface, int x, int y) override;
};

// The icons of the player's states (poisoned, burning, ...) in a row: in the
// inventory (IsLarge, ten icons) or in the small inventory (nine).
class TPlayerStates final : public TElement {
public:
	TPlayerStates(bool islarge);
	void Draw(int Surface, int x, int y) override;
	void MouseHover(int x, int y) override;
	[[nodiscard]] int GetNumberOfIcons() const;
	[[nodiscard]] int GetIconAt(int x, int y) const;
	int GetIcon(int Position);

	bool IsLarge; // +0x24
	int MaxIcons; // +0x28, 10 or 9
};

// Identifier 2: the small inventory, a drop area for the body slots (skin
// 99) with the combat controls, the "maximise" button, capacity, soul and
// the states.
class TSmallInventoryWindow final : public TContentWindow {
public:
	TSmallInventoryWindow();
	void Draw(int Surface, int x, int y) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool HelpMouseMove(int x, int y) override;
};

// The inventory's ten body slots with the "minimise" button, capacity, soul
// and the states.
class TInventoryView final : public TWindow {
public:
	TInventoryView(TWindow *Receiver);
	void Draw(int Surface, int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	bool HelpMouseMove(int x, int y) override;
	int GetSlotAt(int x, int y); // 1-10 or 0
};

// The fight modes (radio buttons), the secure mode, "Stop" and the options
// and help buttons; skin buttons in the small inventory, text buttons in the
// large one (IsLarge).
class TCombatControls final : public TWindow {
public:
	TCombatControls(const TCombatControls &) = delete;
	TCombatControls &operator=(const TCombatControls &) = delete;
	TCombatControls(TWindow *Receiver, bool Large);
	~TCombatControls() override
	{
		AttackModes.reset();
		ChaseModes.reset();
	}
	void Draw(int Surface, int x, int y) override;
	bool HelpMouseMove(int x, int y) override;
	void HandleEvent(int Event) override;

	bool IsLarge;                                   // +0x2c
	std::unique_ptr<TRadioButtonGroup> AttackModes; // +0x30, offensive, balanced, defensive (issue #58)
	std::unique_ptr<TRadioButtonGroup> ChaseModes;  // +0x34, stand, chase
	TSkinButton *StandButton;                       // +0x38
	TSkinButton *ChaseButton;                       // +0x3c
	TToggleButton *SecureButton;                    // +0x40, State: attacks on unmarked players allowed
};

// Identifier 4: the hit point and mana bars with their values.
class TStatusWindow : public TContentWindow {
public:
	TStatusWindow();
	void Draw(int Surface, int x, int y) override;
	bool HelpMouseMove(int x, int y) override;
	void Arrange() override;

	int Data44; // +0x44, 100, never read
	int Data48; // +0x48, 100, never read
};

// --- The bodies of the sub-windows ------------------------------------------------

// TSubWindowBody, the content of a TSubWindow, is DialogWindow.h's since issue #118.

// The objects of a container in rows of SkinProperties.ContainerSlotsPerRow slots, with
// a scroll bar.
class TContainerWindowBody : public TSubWindowBody {
public:
	TContainerWindowBody(int _Container);
	~TContainerWindowBody() override;
	void Draw(int Surface, int x, int y) override;
	void SetSize(int Width, int Height) override;
	int getMinimumHeight() override;
	int GetMaximumHeight() override;
	void Arrange() override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	bool MouseWheel(int x, int y, int Code) override;
	bool HelpMouseMove(int x, int y) override;
	void HandleEvent(int Event) override;
	virtual void Update();
	[[nodiscard]] int GetSlotAt(int x, int y, bool Occupied) const;
	void UpdateUpButton();
	// Odd, kept: the rows start at ContainerFirstSlot.x (the left edge of the
	// first slot, the same as ContainerFirstSlot.y in the game's skin).
	int GetInitialHeight() override
	{
		return (ObjectCount / SkinProperties().ContainerSlotsPerRow + (ObjectCount % SkinProperties().ContainerSlotsPerRow > 0))
			* SkinProperties().ContainerRowHeight
			+ SkinProperties().ContainerFirstSlot.x;
	}
	int GetContentHeight() override
	{
		return (Capacity / SkinProperties().ContainerSlotsPerRow + (Capacity % SkinProperties().ContainerSlotsPerRow > 0))
			* SkinProperties().ContainerRowHeight
			+ SkinProperties().ContainerFirstSlot.x;
	}

	TScrollBar *ScrollBar; // +0x30
	int Container;         // +0x34
	int ObjectCount;       // +0x38, at the last Arrange
	int Capacity;          // +0x3c
	bool HasParent;        // +0x40, the "up" button is shown

protected:
	void OnClose() override; // the destructor's SendCloseContainer (issue #108)
};

// The player's skills, one per line (Lines, 15), with a progress bar for
// those whose bar is switched on.
class TSkillsWindowBody : public TSubWindowBody {
public:
	TSkillsWindowBody();
	~TSkillsWindowBody() override;
	void Draw(int Surface, int x, int y) override;
	bool MouseWheel(int x, int y, int Code) override;
	bool HelpMouseMove(int x, int y) override;
	void MouseHover(int x, int y) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	void SetSize(int Width, int Height) override;
	void Arrange() override;
	int getMinimumHeight() override;
	int GetInitialHeight() override;
	int GetMaximumHeight() override;
	int GetContentHeight() override;
	bool GetLine(int NumberOfLine, char *Text, int &Value, int &Progress) const;

	TSkillContextMenu *ContextMenu; // +0x30, while the skill menu (ContextMenus.cpp) is open
	TScrollBar *ScrollBar;          // +0x34
	int Lines;                      // +0x38, 15
};

// The battle list: the opponents (Creatures.cpp) with outfit, name, health
// bar and marks.
class TBattleWindowBody final : public TSubWindowBody {
public:
	TBattleWindowBody();
	// Its SetHighlightID(0) is the close since issue #108.
	~TBattleWindowBody() override { CheckClosed("TBattleWindowBody"); }
	void Draw(int Surface, int x, int y) override;
	void SetSize(int Width, int Height) override;
	void Arrange() override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	void MouseHover(int x, int y) override;
	bool MouseWheel(int x, int y, int Code) override;
	bool HelpMouseMove(int x, int y) override;
	int getMinimumHeight() override;
	int GetInitialHeight() override;
	int GetContentHeight() override;
	int GetOpponentAt(int x, int y);

	TScrollBar *ScrollBar;          // +0x30
	int NumberOfOpponents;          // +0x34, at the last Draw; -1 at first
	TFixedString<32> MouseOverText; // +0x38, a shortened name in full

protected:
	void OnClose() override; // the destructor's SetHighlightID(0) (issue #108)
};
// The parts of the trade body: the two offers (TTradeOffers), the names
// above them (TTradeNames) and the buttons below (TTradeButtons).
class TTradeOffers final : public TWindow {
public:
	TTradeOffers();
	void Draw(int Surface, int x, int y) override;
	void SetSize(int Width, int Height) override;
	int getMinimumHeight() override;
	int GetContentHeight();
	void Arrange() override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	bool MouseWheel(int x, int y, int Code) override;
	void GetOfferAt(int x, int y, int *Side, int *Position) const;

	TScrollBar *ScrollBar; // +0x2c
	TSeparator *Separator; // +0x30
};

class TTradeNames final : public TWindow {
public:
	TTradeNames();
	void Draw(int Surface, int x, int y) override;
	void SetSize(int Width, int Height) override;
	void Arrange() override;

	TLabel *OwnName;       // +0x2c
	TLabel *CounterName;   // +0x30
	TSeparator *Separator; // +0x34
};

class TTradeButtons final : public TWindow {
public:
	TTradeButtons();
	void Draw(int Surface, int x, int y) override;
	void SetSize(int Width, int Height) override;
	void SetCancelButton(bool Cancel);
	void ShowAcceptButton(TComponent *EventComponent);
	void Wait();

	int Data2C;                // +0x2c, not used
	int Data30;                // +0x30, not used
	TTextButton *CancelButton; // +0x34, "Cancel" or "Reject" (Callback)
	TTextButton *AcceptButton; // +0x38, "Accept" (event 1 of the body)
	TLabel *WaitLabel;         // +0x3c
	TSeparator *Separator;     // +0x40
	TCallback Callback;        // +0x44, closes the window (CreateTradeWindow)
};

// The trade: both offers, the names and the buttons. Event 21 (MainWindow.cpp's
// UpdateTrade: the counter offer came) shows "Accept", event 1 accepts.
class TTradeWindowBody final : public TSubWindowBody {
public:
	TTradeWindowBody();
	~TTradeWindowBody() override;
	void Draw(int Surface, int x, int y) override
	{
		if (!Minimized)
			TWindow::Draw(Surface, x, y);
	}
	int GetContentHeight() override;
	void SetSize(int Width, int Height) override;
	bool HelpMouseMove(int x, int y) override;
	int getMinimumHeight() override;
	int GetInitialHeight() override;
	int GetMaximumHeight() override;
	void HandleEvent(int Event) override;

	TTradeButtons *Buttons; // +0x30
	TTradeOffers *Offers;   // +0x34
	TTradeNames *Names;     // +0x38

protected:
	void OnClose() override; // the destructor's SendRejectTrade and ClearOffers (issue #108)
};

// The VIP list (Creatures.cpp's buddies) with icon, name and online state.
class TBuddylistWindowBody final : public TSubWindowBody {
public:
	TBuddylistWindowBody(const TBuddylistWindowBody &) = delete;
	TBuddylistWindowBody &operator=(const TBuddylistWindowBody &) = delete;
	TBuddylistWindowBody();
	~TBuddylistWindowBody() override
	{
		if (MouseOverText != nullptr)
			MouseOverText.reset();
		MouseOverText = nullptr;
	}
	void Draw(int Surface, int x, int y) override;
	void SetSize(int Width, int Height) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	bool LeftButtonDoubleClick(int x, int y, int Flags) override;
	bool MouseWheel(int x, int y, int Code) override;
	void MouseHover(int x, int y) override;
	bool HelpMouseMove(int x, int y) override;
	void Arrange() override;
	int getMinimumHeight() override;
	int GetInitialHeight() override;
	int GetContentHeight() override;
	int GetBuddyAt(int x, int y);

	TScrollBar *ScrollBar;                 // +0x30
	int NumberOfBuddies;                   // +0x34, at the last Draw; -1 at first
	std::unique_ptr<char[]> MouseOverText; // +0x38, a shortened name in full (new char[36]; issue #58)
};

// --- The creators ------------------------------------------------------------------

// The inventory windows' Receiver (and TButtonsWindow's, TInventoryView's, TCombatControls'), the main window
// (MainWindow.cpp passes this): only their buttons' TCallback receiver, so a TWindow since issue #118 (the original's
// TMainWindow; ContentWindows.cpp then does not include MainWindow.h, which made ContentWindows and MainWindow a
// cycle).

TContentWindow *CreateInventoryWindow(TWindow *Receiver);
TContentWindow *CreateSmallInventoryWindow(TWindow *Receiver);
TSubWindow *CreateContainerWindow(int Container);
TSubWindow *CreateSkillsWindow();
TSubWindow *CreateBattleWindow();
TSubWindow *CreateTradeWindow();
TSubWindow *CreateBuddylistWindow();
