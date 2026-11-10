// ContentWindows.cpp: the windows of the side bar and of the dialog column
// (c:\mud\tibiaplayer\ContentWindows.cpp); see ContentWindows.h.
//
// Order: the functions follow the source's line numbers. The binary moves the
// creators and most constructors behind the rest (each calls functions defined
// further down, the pattern of Creatures.cpp): the creators come last, after
// the constructors of the bodies and of TAutomapWindow, TInventoryView's
// mouse handlers, TStatusWindow's and the container body's constructors.
// Functions without line evidence go where the binary has them.

#include "ContentWindows.h"
#include <cstdint>
#include <algorithm>
#include <array>
#include "Communication.h"
#include "ContextMenus.h"
#include "Creatures.h"
#include "Inventory.h"
#include "Objects.h"
#include "Map.h"
#include "Preferences.h"
#include "Skins.h"
#include "Status.h"
#include "Surface.h"
#include "Utils.h"
#include "graphics.h"
#include "DialogElements.h"
#include "DialogWindow.h"
#include "GUI.h"
#include "Help.h"
#include "Talk.h"
#include "common.h"
#include "compat/print.h"
#include "platform/Platform.h"
#include "types/FixedString.h"
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <vector>

namespace {

// The texts of the user interface that this file shows. The numbers are their places in the original's table of
// texts, a header every file included.
const char *TextPoisoned = "You are poisoned";                                  // 0
const char *TextOnFire = "You are on fire";                                     // 1
const char *TextMagicShield = "You are protected by a magic shield";            // 2
const char *TextParalysed = "You are paralysed";                                // 3
const char *TextHasted = "You are hasted";                                      // 4
const char *TextNoLogoutInFight = "You may not logout during a fight";          // 5
const char *TextDrunk = "You are drunk";                                        // 6
const char *TextStruckByEnergy = "You are struck by magical energy";            // 7
const char *TextMayBeAttacked = "You may be attacked by other players";         // 8
const char *TextPartyMember = "You are a member of a party";                    // 9
const char *TextAttackedUnmarked = "You have attacked an unmarked player";      // 10
const char *TextKilledTooMany = "You have killed too many unmarked players";    // 11
const char *TextMaximiseInventory = "Maximise inventory";                       // 20
const char *TextMinimiseInventory = "Minimise inventory";                       // 21
const char *TextCloseWindow = "Close this window";                              // 22
const char *TextMaximiseOrMinimise = "Maximise or minimise window";             // 23
const char *TextShowHigherContainer = "Show higher container";                  // 24
const char *TextOpenSkillsWindow = "Open skills window (Ctrl+S)";               // 25
const char *TextCloseSkillsWindow = "Close skills window (Ctrl+S)";             // 26
const char *TextOpenBattleWindow = "Open battle window (Ctrl+B)";               // 27
const char *TextCloseBattleWindow = "Close battle window (Ctrl+B)";             // 28
const char *TextOpenVIPWindow = "Open VIP window (Ctrl+P)";                     // 29
const char *TextCloseVIPWindow = "Close VIP window (Ctrl+P)";                   // 30
const char *TextLeaveGame = "Leave the game (Ctrl+Q or Ctrl+L)";                // 31
const char *TextCentreAroundPlayer = "Centre around player";                    // 32
const char *TextShowHigherFloor = "Show higher floor (Alt+PageUp)";             // 33
const char *TextShowLowerFloor = "Show lower floor (Alt+PageDown)";             // 34
const char *TextZoomIn = "Zoom in (Alt+End)";                                   // 35
const char *TextZoomOut = "Zoom out (Alt+Home)";                                // 36
const char *TextScrollAutomap = "Scroll automap (Alt+Cursor)";                  // 37
const char *TextOffensiveFighting = "Offensive fighting";                       // 38
const char *TextBalancedFighting = "Balanced fighting";                         // 39
const char *TextDefensiveFighting = "Defensive fighting";                       // 40
const char *TextStandWhileFighting = "Stand while fighting";                    // 41
const char *TextChaseOpponent = "Chase opponent";                               // 42
const char *TextCannotAttackUnmarked = "You cannot attack unmarked characters"; // 43
const char *TextCanAttackAny = "You can attack any character";                  // 44
const char *TextStopCurrentAction = "Stop current action (ESC)";                // 45
const char *TextOpenOptions = "Open options";                                   // 46
const char *TextOpenHelpMenu = "Open help menu";                                // 47
const char *TextCancelOffer = "Cancel offer";                                   // 48
const char *TextRejectOffer = "Reject offer";                                   // 49
const char *TextAcceptOffer = "Accept offer";                                   // 50
const char *TextPercentToGo = "You have %d percent to go";                      // 52
const char *TextWaitForCounterOffer = "Please wait for a\ncounteroffer";        // 56
const char *TextWaitForPartner = "Please wait for \nyour partner to accept.";   // 57

} // namespace

// Issue #111 (clang-tidy, the first look at this file in patella_core): cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg off for the file:
// the original's raw owners (notes/ownership.md): the window tree owns its children, the message queue its blocks,
// the lists their entries; the original's varargs (TMultiStateButton's constructor, _snprintf)
// NOLINTBEGIN(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg)


namespace {

// NOLINTNEXTLINE(bugprone-throwing-static-initialization): the binary's global; TCallback's constructor (GUI.cpp) stores two values
TCallback NoCallback(nullptr, 0);

} // namespace

// The texts TFixedStrings (issue #60).
static_assert(sizeof(patella::bridge::GUI().AutomapMouseOverText) == 100 && sizeof(patella::bridge::GUI().SkillMouseOverText) == 256, "the mouse-over texts");

// --- The creators -----------------------------------------------------------------

// The inventory: a plain content window with the body slots and the large
// combat controls.
TContentWindow *CreateInventoryWindow(TWindow *Receiver)
{
	TContentWindow *Window = new TContentWindow(INVENTORY_WINDOW, true);
	Window->SetSize(SkinProperties().SideBarWidth, SkinProperties().InventoryWindowHeight);
	TInventoryView *View = new TInventoryView(Receiver);
	Window->InsertChild(View);
	Window->SetChildPosition(View, SkinProperties().InventoryControlsAndView.width, SkinProperties().InventoryControlsAndView.height);
	TCombatControls *Controls = new TCombatControls(Receiver, true);
	Window->InsertChild(Controls);
	Window->SetChildPosition(Controls, SkinProperties().InventoryControlsAndView.x, SkinProperties().InventoryControlsAndView.y);
	Window->Arrange();
	return Window;
}

// The small inventory: the small combat controls, the "maximise" button
// (event 24 of the main window), capacity, soul and states.
TContentWindow *CreateSmallInventoryWindow(TWindow *Receiver)
{
	TSmallInventoryWindow *Window = new TSmallInventoryWindow;
	Window->SetSize(SkinProperties().SideBarWidth, SkinProperties().SmallInventoryWindowHeight);
	TCombatControls *Controls = new TCombatControls(Receiver, false);
	Window->InsertChild(Controls);
	Window->SetChildPosition(Controls, SkinProperties().SmallInventoryControlsAndMaximise.x, SkinProperties().SmallInventoryControlsAndMaximise.y);
	TSkinButton *Maximise = new TSkinButton(0, 0, SKIN_MAXIMISE_BUTTON_UP, SKIN_MAXIMISE_BUTTON_DOWN, TCallback(Receiver, EVENT_MAXIMISE_INVENTORY), TextMaximiseInventory, false);
	Window->InsertChild(Maximise);
	Window->SetChildPosition(Maximise, SkinProperties().SmallInventoryControlsAndMaximise.width, SkinProperties().SmallInventoryControlsAndMaximise.height);
	TCapacityLabel *Capacity = new TCapacityLabel;
	Window->InsertChild(Capacity);
	Window->SetChildArea(Capacity, SkinProperties().SmallInventoryCapacity);
	TSoulLabel *Soul = new TSoulLabel;
	Window->InsertChild(Soul);
	Window->SetChildArea(Soul, SkinProperties().SmallInventorySoul);
	TPlayerStates *States = new TPlayerStates(false);
	Window->InsertChild(States);
	Window->SetChildArea(States, SkinProperties().SmallInventoryStates);
	return Window;
}

// A container's window: close (event 10) and minimize (event 9) go to the
// window, "up" (event 3) to the body; the icon is the container object,
// the title its name. The window gets the body's initial height before
// MainWindow.cpp adds it to the column.
TSubWindow *CreateContainerWindow(int Container)
{
	TContainerWindowBody *Body = new TContainerWindowBody(Container);
	TSubWindow *Window = new TSubWindow(Container + CONTAINER_WINDOW, Body, true, false);
	TSkinButton *CloseButton = new TSkinButton(0, 0, SKIN_CLOSE_BUTTON_UP, SKIN_CLOSE_BUTTON_DOWN, TCallback(Window, EVENT_WINDOW_CLOSE), TextCloseWindow, false);
	TSkinButton *MinimizeButton = new TSkinButton(0, 0, SKIN_MINIMISE_BUTTON_UP, SKIN_MINIMISE_BUTTON_DOWN, TCallback(Window, EVENT_WINDOW_MINIMISE), TextMaximiseOrMinimise, false);
	TSkinButton *UpButton = new TSkinButton(0, 0, SKIN_HIGHER_CONTAINER_BUTTON_UP, SKIN_HIGHER_CONTAINER_BUTTON_DOWN, TCallback(Body, EVENT_CONTAINER_UP), TextShowHigherContainer, false);
	TObjectIcon *Icon = new TObjectIcon(GetContainerObject(Container), &SkinProperties().SubWindow.IconHighlight, false);
	TLabel *Title = new TLabel(SkinProperties().SubWindow.TitleFont, GetContainerName(Container));
	Window->SetSize(SkinProperties().SideBarWidth, TSubWindow::GetFrameHeight() + Body->GetInitialHeight());
	Window->SetCloseButton(CloseButton);
	Window->SetMinimizeButton(MinimizeButton);
	Window->SetUpButton(UpButton);
	Window->SetIcon(Icon);
	Window->SetTitle(Title);
	Window->Arrange();
	return Window;
}

TSubWindow *CreateSkillsWindow()
{
	TSkillsWindowBody *Body = new TSkillsWindowBody;
	TSubWindow *Window = new TSubWindow(SKILLS_WINDOW, Body, true, false);
	TSkinButton *CloseButton = new TSkinButton(0, 0, SKIN_CLOSE_BUTTON_UP, SKIN_CLOSE_BUTTON_DOWN, TCallback(Window, EVENT_WINDOW_CLOSE), TextCloseWindow, false);
	TSkinButton *MinimizeButton = new TSkinButton(0, 0, SKIN_MINIMISE_BUTTON_UP, SKIN_MINIMISE_BUTTON_DOWN, TCallback(Window, EVENT_WINDOW_MINIMISE), TextMaximiseOrMinimise, false);
	TLabel *Title = new TLabel(SkinProperties().SubWindow.TitleFont, "Skills");
	TSkinIcon *Icon = new TSkinIcon(SKIN_SKILLS_ICON);
	Window->SetSize(SkinProperties().SideBarWidth, TSubWindow::GetFrameHeight() + Body->GetInitialHeight());
	Window->SetCloseButton(CloseButton);
	Window->SetMinimizeButton(MinimizeButton);
	Window->SetIcon(Icon);
	Window->SetTitle(Title);
	Window->Arrange();
	return Window;
}

TSubWindow *CreateBattleWindow()
{
	TBattleWindowBody *Body = new TBattleWindowBody;
	TSubWindow *Window = new TSubWindow(BATTLE_WINDOW, Body, true, false);
	TSkinButton *CloseButton = new TSkinButton(0, 0, SKIN_CLOSE_BUTTON_UP, SKIN_CLOSE_BUTTON_DOWN, TCallback(Window, EVENT_WINDOW_CLOSE), TextCloseWindow, false);
	TSkinButton *MinimizeButton = new TSkinButton(0, 0, SKIN_MINIMISE_BUTTON_UP, SKIN_MINIMISE_BUTTON_DOWN, TCallback(Window, EVENT_WINDOW_MINIMISE), TextMaximiseOrMinimise, false);
	TLabel *Title = new TLabel(SkinProperties().SubWindow.TitleFont, "Battle");
	TSkinIcon *Icon = new TSkinIcon(SKIN_BATTLE_ICON);
	Window->SetSize(SkinProperties().SideBarWidth, TSubWindow::GetFrameHeight() + Body->GetInitialHeight());
	Window->SetCloseButton(CloseButton);
	Window->SetMinimizeButton(MinimizeButton);
	Window->SetIcon(Icon);
	Window->SetTitle(Title);
	Window->Arrange();
	return Window;
}

// The cancel button closes the window (event 10).
TSubWindow *CreateTradeWindow()
{
	TTradeWindowBody *Body = new TTradeWindowBody;
	TSubWindow *Window = new TSubWindow(TRADE_WINDOW, Body, true, false);
	TSkinButton *CloseButton = new TSkinButton(0, 0, SKIN_CLOSE_BUTTON_UP, SKIN_CLOSE_BUTTON_DOWN, TCallback(Window, EVENT_WINDOW_CLOSE), TextCloseWindow, false);
	TSkinButton *MinimizeButton = new TSkinButton(0, 0, SKIN_MINIMISE_BUTTON_UP, SKIN_MINIMISE_BUTTON_DOWN, TCallback(Window, EVENT_WINDOW_MINIMISE), TextMaximiseOrMinimise, false);
	TLabel *Title = new TLabel(SkinProperties().SubWindow.TitleFont, "Trade");
	TSkinIcon *Icon = new TSkinIcon(SKIN_TRADE_ICON);
	Window->SetSize(SkinProperties().SideBarWidth, TSubWindow::GetFrameHeight() + Body->GetInitialHeight());
	Window->SetCloseButton(CloseButton);
	Window->SetMinimizeButton(MinimizeButton);
	Window->SetIcon(Icon);
	Window->SetTitle(Title);
	Body->Buttons->Callback = TCallback(Window, EVENT_WINDOW_CLOSE);
	Body->Buttons->SetCancelButton(true);
	Window->Arrange();
	return Window;
}

TSubWindow *CreateBuddylistWindow()
{
	TBuddylistWindowBody *Body = new TBuddylistWindowBody;
	TSubWindow *Window = new TSubWindow(BUDDYLIST_WINDOW, Body, true, false);
	TSkinButton *CloseButton = new TSkinButton(0, 0, SKIN_CLOSE_BUTTON_UP, SKIN_CLOSE_BUTTON_DOWN, TCallback(Window, EVENT_WINDOW_CLOSE), TextCloseWindow, false);
	TSkinButton *MinimizeButton = new TSkinButton(0, 0, SKIN_MINIMISE_BUTTON_UP, SKIN_MINIMISE_BUTTON_DOWN, TCallback(Window, EVENT_WINDOW_MINIMISE), TextMaximiseOrMinimise, false);
	TLabel *Title = new TLabel(SkinProperties().SubWindow.TitleFont, "VIP");
	TSkinIcon *Icon = new TSkinIcon(SKIN_VIP_ICON);
	Window->SetSize(SkinProperties().SideBarWidth, TSubWindow::GetFrameHeight() + Body->GetInitialHeight());
	Window->SetCloseButton(CloseButton);
	Window->SetMinimizeButton(MinimizeButton);
	Window->SetIcon(Icon);
	Window->SetTitle(Title);
	Window->Arrange();
	return Window;
}

// --- TButtonsWindow ---------------------------------------------------------------

TButtonsWindow::TButtonsWindow(TDialogWindow *_CheckObject, TWindow *Receiver)
	: TContentWindow(BUTTONS_WINDOW, true)
{
	PATELLA_ASSERT(_CheckObject!=nullptr);
	CheckObject = _CheckObject;
	SetSize(SkinProperties().SideBarWidth, SkinProperties().ButtonsWindowHeight);
	SkillsButton = new TTextToggleButton(SkinProperties().ButtonsWindowSkillsAndBattle.x, SkinProperties().ButtonsWindowSkillsAndBattle.y, "Skills", SKIN_SMALL_BUTTON_UP, SKIN_SMALL_BUTTON_DOWN,
		TCallback(Receiver, EVENT_TOGGLE_SKILLS), TextOpenSkillsWindow, TextCloseSkillsWindow, false);
	InsertChild(SkillsButton);
	BattleButton = new TTextToggleButton(SkinProperties().ButtonsWindowSkillsAndBattle.width, SkinProperties().ButtonsWindowSkillsAndBattle.height, "Battle", SKIN_SMALL_BUTTON_UP,
		SKIN_SMALL_BUTTON_DOWN, TCallback(Receiver, EVENT_TOGGLE_BATTLE), TextOpenBattleWindow, TextCloseBattleWindow, false);
	InsertChild(BattleButton);
	BuddyButton = new TTextToggleButton(SkinProperties().ButtonsWindowBuddyAndLogout.x, SkinProperties().ButtonsWindowBuddyAndLogout.y, "VIP", SKIN_SMALL_BUTTON_UP, SKIN_SMALL_BUTTON_DOWN,
		TCallback(Receiver, EVENT_TOGGLE_VIP), TextOpenVIPWindow, TextCloseVIPWindow, false);
	InsertChild(BuddyButton);
	LogoutButton = new TTextButton(SkinProperties().ButtonsWindowBuddyAndLogout.width, SkinProperties().ButtonsWindowBuddyAndLogout.height, "Logout",
		TCallback(Receiver, EVENT_LEAVE_QUESTION), TextLeaveGame);
	InsertChild(LogoutButton);
}

// A button shows as pressed while its window is open.
void TButtonsWindow::Draw(int Surface, int x, int y)
{
	SkillsButton->State = CheckObject->hasContentWindow(SKILLS_WINDOW);
	BattleButton->State = CheckObject->hasContentWindow(BATTLE_WINDOW);
	BuddyButton->State = CheckObject->hasContentWindow(BUDDYLIST_WINDOW);
	TContentWindow::Draw(Surface, x, y);
}

bool TButtonsWindow::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	SetHelpHandle(HELP_CONTROL_BUTTONS);
	SetHelpSize(Width, Height);
	return true;
}

// --- TAutomapWindow ---------------------------------------------------------------

// The zoom comes from the preferences (2 unless it is 1, 2, 4 or 8);
// the hotkeys Alt+Home/End zoom, Alt+PageUp/PageDown change the
// floor, Alt+cursor scrolls.
TAutomapWindow::TAutomapWindow()
	: TContentWindow(AUTOMAP_WINDOW, true)
{
	DrawingListLength = 4;
	DrawingList.reset(new TDrawing[4]);
	SetSize(SkinProperties().SideBarWidth, SkinProperties().AutomapWindowHeight);
	Changed = true;
	Centred = true;
	Zoom = GetAutomapZoom();
	if (Zoom != 1 && Zoom != 2 && Zoom != 4 && Zoom != 8)
		Zoom = 2;
	View = new TAutomapView;
	CentreButton = new TTextButton(SkinProperties().AutomapCentreButtonAndCompassRose.x, SkinProperties().AutomapCentreButtonAndCompassRose.y, "Centre", TCallback(this, EVENT_AUTOMAP_CENTRE),
		TextCentreAroundPlayer);
	FloorUpButton = new TSkinButton(SkinProperties().AutomapFloorButtons.x, SkinProperties().AutomapFloorButtons.y, SKIN_HIGHER_FLOOR_BUTTON_UP, SKIN_HIGHER_FLOOR_BUTTON_DOWN, TCallback(this, EVENT_AUTOMAP_FLOOR_UP),
		TextShowHigherFloor, false);
	FloorDownButton = new TSkinButton(SkinProperties().AutomapFloorButtons.width, SkinProperties().AutomapFloorButtons.height, SKIN_LOWER_FLOOR_BUTTON_UP, SKIN_LOWER_FLOOR_BUTTON_DOWN,
		TCallback(this, EVENT_AUTOMAP_FLOOR_DOWN), TextShowLowerFloor, false);
	ZoomInButton = new TSkinButton(SkinProperties().AutomapZoomButtons.x, SkinProperties().AutomapZoomButtons.y, SKIN_ZOOM_IN_BUTTON_UP, SKIN_ZOOM_IN_BUTTON_DOWN, TCallback(this, EVENT_AUTOMAP_ZOOM_IN),
		TextZoomIn, false);
	ZoomOutButton = new TSkinButton(SkinProperties().AutomapZoomButtons.width, SkinProperties().AutomapZoomButtons.height, SKIN_ZOOM_OUT_BUTTON_UP, SKIN_ZOOM_OUT_BUTTON_DOWN,
		TCallback(this, EVENT_AUTOMAP_ZOOM_OUT), TextZoomOut, false);
	CompassRose = new TCompassRose(SKIN_COMPASS_ROSE);
	InsertChild(View);
	InsertChild(CentreButton);
	InsertChild(FloorUpButton);
	InsertChild(FloorDownButton);
	InsertChild(ZoomInButton);
	InsertChild(ZoomOutButton);
	InsertChild(CompassRose);
	SetChildArea(View, SkinProperties().AutomapView);
	SetChildPosition(CompassRose, SkinProperties().AutomapCentreButtonAndCompassRose.width, SkinProperties().AutomapCentreButtonAndCompassRose.height);
	CompassRose->SetSize(Skins()[SKIN_COMPASS_ROSE].width, Skins()[SKIN_COMPASS_ROSE].height);
	Arrange();
	AddHotkey(KEY_ALT | KEY_HOME, TCallback(this, EVENT_AUTOMAP_ZOOM_OUT));
	AddHotkey(KEY_ALT | KEY_END, TCallback(this, EVENT_AUTOMAP_ZOOM_IN));
	AddHotkey(KEY_ALT | KEY_PAGE_UP, TCallback(this, EVENT_AUTOMAP_FLOOR_UP));
	AddHotkey(KEY_ALT | KEY_PAGE_DOWN, TCallback(this, EVENT_AUTOMAP_FLOOR_DOWN));
	AddHotkey(KEY_ALT | KEY_UP, TCallback(this, EVENT_AUTOMAP_NORTH));
	AddHotkey(KEY_ALT | KEY_LEFT, TCallback(this, EVENT_AUTOMAP_WEST));
	AddHotkey(KEY_ALT | KEY_RIGHT, TCallback(this, EVENT_AUTOMAP_EAST));
	AddHotkey(KEY_ALT | KEY_DOWN, TCallback(this, EVENT_AUTOMAP_SOUTH));
}

// Its work is the close since issue #108.
TAutomapWindow::~TAutomapWindow()
{
	CheckClosed("TAutomapWindow");
}

// The destructor's work until issue #108: the zoom goes back into the
// preferences, the hotkeys are removed; then the children.
void TAutomapWindow::OnClose()
{
	try {
		SetAutomapZoom(Zoom);
		RemoveHotkey(KEY_ALT | KEY_HOME);
		RemoveHotkey(KEY_ALT | KEY_END);
		RemoveHotkey(KEY_ALT | KEY_PAGE_UP);
		RemoveHotkey(KEY_ALT | KEY_PAGE_DOWN);
		RemoveHotkey(KEY_ALT | KEY_UP);
		RemoveHotkey(KEY_ALT | KEY_LEFT);
		RemoveHotkey(KEY_ALT | KEY_RIGHT);
		RemoveHotkey(KEY_ALT | KEY_DOWN);
	} catch (...) {
		CloseWhileUnwinding([this] { TContentWindow::OnClose(); });
		throw;
	}
	TContentWindow::OnClose();
}

bool TAutomapWindow::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	SetHelpHandle(HELP_AUTOMAP);
	SetHelpSize(Width, Height);
	return true;
}

// The text of the mark under the mouse. Odd, kept: a mark is taken only when
// both of its coordinates are not 0, and the buffer is printed to stdout every
// time (a debugging leftover; printf("%s\n"), since issue #64 compat/print.h's
// PrintLine, the same bytes).
void TAutomapWindow::MouseHover(int x, int y)
{
	patella::TGUI &GUI = patella::bridge::GUI();
	TMark Mark = View->GetMark(x - View->OffsetX, y - View->OffsetY);
	if (Mark.x != 0 && Mark.y != 0) {
		const int Length = (int)strlen(Mark.Text);
		if (Length != 0) {
			CopyString(GUI.AutomapMouseOverText.Chars, Mark.Text);
			SetMouseOverText(GUI.AutomapMouseOverText);
		}
	}
	patella::compat::PrintLine("{}", static_cast<const char *>(GUI.AutomapMouseOverText));
}

// The frame around the view: skins 9 (top), 11 (left), 10 (right) and 8
// (bottom) along the edges of AutomapFrame.
void TAutomapWindow::Arrange()
{
	TContentWindow::Arrange();
	Changed = true;
	DrawingList[0] = TDrawing(SKIN_FRAME_EDGE_TOP, SkinProperties().AutomapFrame.x, SkinProperties().AutomapFrame.y, SkinProperties().AutomapFrame.width,
		Skins()[SKIN_FRAME_EDGE_TOP].height, 0, 0);
	DrawingList[1] = TDrawing(SKIN_FRAME_EDGE_LEFT, SkinProperties().AutomapFrame.x, SkinProperties().AutomapFrame.y, Skins()[SKIN_FRAME_EDGE_LEFT].width,
		SkinProperties().AutomapFrame.height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_FRAME_EDGE_RIGHT, SkinProperties().AutomapFrame.x - 1 + SkinProperties().AutomapFrame.width,
		SkinProperties().AutomapFrame.y, Skins()[SKIN_FRAME_EDGE_RIGHT].width, SkinProperties().AutomapFrame.height, 0, 0);
	DrawingList[3] = TDrawing(SKIN_FRAME_EDGE_BOTTOM, SkinProperties().AutomapFrame.x, SkinProperties().AutomapFrame.height - 1 + SkinProperties().AutomapFrame.y,
		SkinProperties().AutomapFrame.width, Skins()[SKIN_FRAME_EDGE_BOTTOM].height, 0, 0);
}

// A scrolled view leaves the player first (the view's centre is fixed where
// it is).
void TAutomapWindow::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_AUTOMAP_CENTRE:
		Centred = true;
		Changed = true;
		break;
	case EVENT_AUTOMAP_FLOOR_UP:
		if (Centred) {
			GetMapPosition(&ViewX, &ViewY, &ViewZ);
			GetMapPosition(&PlayerX, &PlayerY, &PlayerZ);
			Centred = false;
		}
		if (ViewZ > 0)
			ViewZ--;
		Changed = true;
		break;
	case EVENT_AUTOMAP_FLOOR_DOWN:
		if (Centred) {
			GetMapPosition(&ViewX, &ViewY, &ViewZ);
			GetMapPosition(&PlayerX, &PlayerY, &PlayerZ);
			Centred = false;
		}
		if (ViewZ < 15)
			ViewZ++;
		Changed = true;
		break;
	case EVENT_AUTOMAP_ZOOM_IN:
		if (Zoom < 8) {
			Zoom *= 2;
			Changed = true;
		}
		break;
	case EVENT_AUTOMAP_ZOOM_OUT:
		if (Zoom > 1) {
			Zoom /= 2;
			Changed = true;
		}
		break;
	case EVENT_AUTOMAP_NORTH:
		Scroll(1);
		break;
	case EVENT_AUTOMAP_SOUTH:
		Scroll(2);
		break;
	case EVENT_AUTOMAP_WEST:
		Scroll(3);
		break;
	case EVENT_AUTOMAP_EAST:
		Scroll(4);
		break;
	default:
		break;
	}
}

// 0 centres the view; 1-4 move it by 24 pixels (north,
// south, west, east).
void TAutomapWindow::Scroll(int Direction)
{
	if (Direction == 0) {
		Centred = true;
		Changed = true;
		return;
	}
	if (Centred) {
		GetMapPosition(&ViewX, &ViewY, &ViewZ);
		GetMapPosition(&PlayerX, &PlayerY, &PlayerZ);
		Centred = false;
	}
	switch (Direction) {
	case 1:
		ViewY += -24 / Zoom;
		break;
	case 2:
		ViewY += 24 / Zoom;
		break;
	case 3:
		ViewX += -24 / Zoom;
		break;
	case 4:
		ViewX += 24 / Zoom;
		break;
	default:
		break;
	}
	Changed = true;
}

// --- TAutomapView -----------------------------------------------------------------

// The map is rendered into the cache surface 5 only when it changed (or the surface was lost): 106x106 pixels
// from the automap colours, a white cross at the player's position and the marks of the four 256x256 blocks
// around the view (skins 74 and up, 11x11); then the surface is copied to the screen. Odd, kept: at zoom 1 the
// centre is rounded down to even coordinates once per pixel, inside the loop. MSVC's analysis (C6262, issue
// #111): the pixels' buffer on the stack, the original's.
PATELLA_MSVC_WARNING(suppress : 6262)
void TAutomapView::Draw(int Surface, int x, int y)
{
	TAutomapWindow *Window = static_cast<TAutomapWindow *>(GetParent());
	int PosX, PosY, PosZ;
	GetMapPosition(&PosX, &PosY, &PosZ);
	if (!Window->Centred) {
		if (PosX == Window->PlayerX && PosY == Window->PlayerY && PosZ == Window->PlayerZ) {
			PosX = Window->ViewX;
			PosY = Window->ViewY;
			PosZ = Window->ViewZ;
		} else {
			Window->Centred = true;
			Window->Changed = true;
		}
	}
	if (TakeAutomapChanged() || Window->Changed || GetSurface()->SurfaceRecreated(0)) {
		std::array<uint8, 106 * 106 * 3> Pixels;
		for (int i = 0; i < 106; i++) {
			for (int j = 0; j < 106; j++) {
				if (Window->Zoom == 1) {
					PosX = PosX / 2 * 2;
					PosY = PosY / 2 * 2;
				}
				const int Zoom = Window->Zoom;
				const TRGBColor Color = GetAutomapColor((Zoom * PosX + Zoom / 2 - 106 + i * 2) / Zoom,
					(Zoom * PosY + Zoom / 2 - 106 + j * 2) / Zoom, PosZ);
				Pixels[(j * 106 + i) * 3] = (uint8)Color.R;
				Pixels[(j * 106 + i) * 3 + 1] = (uint8)Color.G;
				Pixels[(j * 106 + i) * 3 + 2] = (uint8)Color.B;
			}
		}
		GetSurface()->WriteImage(5, 0, 0, 106, 106, Pixels.data());
		int CrossX = 53;
		int CrossY = 53;
		bool SameLevel = true;
		bool Visible = true;
		if (!Window->Centred) {
			GetMapPosition(&PosX, &PosY, &PosZ);
			CrossX = (PosX - Window->ViewX) * Window->Zoom / 2 + 53;
			CrossY = (PosY - Window->ViewY) * Window->Zoom / 2 + 53;
			if (PosZ != Window->ViewZ)
				SameLevel = false;
			Visible = CrossX >= 0 && CrossX <= 105 && CrossY >= 0 && CrossY <= 105 && SameLevel;
		}
		if (Visible) {
			if (Window->Zoom <= 2) {
				GetSurface()->SetPixel(5, CrossX, CrossY, TRGBColor(255, 255, 255));
				GetSurface()->SetPixel(5, CrossX + 1, CrossY, TRGBColor(255, 255, 255));
				GetSurface()->SetPixel(5, CrossX - 1, CrossY, TRGBColor(255, 255, 255));
				GetSurface()->SetPixel(5, CrossX, CrossY + 1, TRGBColor(255, 255, 255));
				GetSurface()->SetPixel(5, CrossX, CrossY - 1, TRGBColor(255, 255, 255));
			} else {
				for (int i = -3; i < 3; i++) {
					GetSurface()->SetPixel(5, CrossX + i, CrossY, TRGBColor(255, 255, 255));
					GetSurface()->SetPixel(5, CrossX + i, CrossY - 1, TRGBColor(255, 255, 255));
					GetSurface()->SetPixel(5, CrossX, CrossY + i, TRGBColor(255, 255, 255));
					GetSurface()->SetPixel(5, CrossX - 1, CrossY + i, TRGBColor(255, 255, 255));
				}
			}
		}
		int CentreX = PosX;
		int CentreY = PosY;
		if (!Window->Centred) {
			PosZ = Window->ViewZ;
			CentreX = Window->ViewX;
			CentreY = Window->ViewY;
		}
		const int Left = CentreX - 106 / Window->Zoom;
		const int Top = CentreY - 106 / Window->Zoom;
		for (int BlockX = Left / 256; BlockX < Left / 256 + 2; BlockX++) {
			for (int BlockY = Top / 256; BlockY < Top / 256 + 2; BlockY++) {
				std::vector<TMark> Marks; // the original's vector<TMark>(0, 0, 10) (issue #100)
				int Count;
				GetAutomapMarks(BlockX, BlockY, PosZ, &Marks, &Count);
				for (int i = 0; i < Count; i++) {
					const TMark Mark = Marks[static_cast<std::size_t>(i)];
					if (Mark.x != 0 || Mark.y != 0) {
						const int MarkX = (Mark.x - Left) * Window->Zoom / 2;
						const int MarkY = (Mark.y - Top) * Window->Zoom / 2;
						if (MarkX >= 0 && MarkY >= 0 && MarkX < 106 && MarkY < 106)
							DrawSkinClipped(5, MarkX - 5, MarkY - 5, 11, 11, Mark.Type + SKIN_MARK_0, 0, 0, 106, 106, 0, 0);
					}
				}
			}
		}
		Window->Changed = false;
	}
	GetSurface()->BlitRect(Surface, x, y, 5, TRect(0, 0, 106, 106));
}

// A click walks to the mark under the mouse, else to the point clicked.
bool TAutomapView::LeftButtonUp(int x, int y, [[maybe_unused]] int Flags)
{
	TAutomapWindow *Window = static_cast<TAutomapWindow *>(GetParent());
	TMark Mark = GetMark(x, y);
	if (Window->Centred)
		GetMapPosition(&Window->ViewX, &Window->ViewY, &Window->ViewZ);
	if (Mark.x == 0 || Mark.y == 0) {
		Mark.x = (x - Width / 2) * 2 / Window->Zoom + Window->ViewX;
		Mark.y = (y - Height / 2) * 2 / Window->Zoom + Window->ViewY;
	}
	const int Result = SetPlayerDestination(Mark.x, Mark.y, Window->ViewZ, false, false);
	if (Result == 1)
		SendWalk();
	else if (Result == 2)
		SendStop();
	return true;
}

// The mark menu (Control.cpp's message 59) gets the point clicked and
// the mark under the mouse.
bool TAutomapView::RightButtonUp(int x, int y, [[maybe_unused]] int Flags)
{
	TMarkMenuData *Data = new TMarkMenuData;
	TAutomapWindow *Window = static_cast<TAutomapWindow *>(GetParent());
	Data->Window = Window;
	if (Window->Centred)
		GetMapPosition(&Window->ViewX, &Window->ViewY, &Window->ViewZ);
	Data->Coordinate.x = (x - Width / 2) * 2 / Window->Zoom + Window->ViewX;
	Data->Coordinate.y = (y - Height / 2) * 2 / Window->Zoom + Window->ViewY;
	Data->Coordinate.z = Window->ViewZ;
	Data->Mark = GetMark(x, y);
	AddMessage(MESSAGE_MARK_MENU, reinterpret_cast<std::intptr_t>(Data), 0);
	return true;
}

// The last mark of the four blocks that lies within 5 pixels of x, y; an
// empty mark if there is none.
TMark TAutomapView::GetMark(int x, int y)
{
	TAutomapWindow *Window = static_cast<TAutomapWindow *>(GetParent());
	if (Window->Centred)
		GetMapPosition(&Window->ViewX, &Window->ViewY, &Window->ViewZ);
	const int Left = Window->ViewX - 106 / Window->Zoom;
	const int Top = Window->ViewY - 106 / Window->Zoom;
	for (int BlockX = Left / 256; BlockX < Left / 256 + 2; BlockX++) {
		for (int BlockY = Top / 256; BlockY < Top / 256 + 2; BlockY++) {
			std::vector<TMark> Marks; // the original's vector<TMark>(0, 0, 10) (issue #100)
			int Count;
			GetAutomapMarks(BlockX, BlockY, Window->ViewZ, &Marks, &Count);
			for (int i = Count - 1; i >= 0; i--) {
				TMark Mark = Marks[static_cast<std::size_t>(i)];
				if (Mark.x != 0 || Mark.y != 0) {
					const int MarkX = (Mark.x - Left) * Window->Zoom / 2;
					const int MarkY = (Mark.y - Top) * Window->Zoom / 2;
					if (MarkX >= 0 && MarkY >= 0 && MarkX < 106 && MarkY < 106 && abs(x - MarkX) < 6
						&& abs(y - MarkY) < 6)
						return Mark;
				}
			}
		}
	}
	return TMark();
}

// --- TCompassRose -----------------------------------------------------------------

TCompassRose::TCompassRose(int _RoseSkin)
{
	PATELLA_ASSERT_VALUES(_RoseSkin<patella::bridge::Skins().NumberOfSkins, _RoseSkin, patella::bridge::Skins().NumberOfSkins);
	RoseSkin = _RoseSkin;
}

void TCompassRose::Draw(int Surface, int x, int y)
{
	DrawPicturePart(Surface, x, y, 4, Skins()[RoseSkin].x, Skins()[RoseSkin].y, Skins()[RoseSkin].width,
		Skins()[RoseSkin].height);
}

// The middle (10x10 pixels) centres the view, the four arms scroll it
// (Scroll's directions); the later tests win.
bool TCompassRose::LeftButtonUp(int x, int y, [[maybe_unused]] int Flags)
{
	int Direction = -1;
	if (Width / 2 - 5 <= x && x <= Width / 2 + 5 && Height / 2 - 5 <= y && y <= Height / 2 + 5)
		Direction = 0;
	if (Width / 2 - 5 <= x && x <= Width / 2 + 5 && 0 <= y && y <= Height / 2 - 5)
		Direction = 1;
	if (Width / 2 - 5 <= x && x <= Width / 2 + 5 && Height / 2 + 5 <= y && y <= Height)
		Direction = 2;
	if (0 <= x && x <= Width / 2 - 5 && Height / 2 - 5 <= y && y <= Height / 2 + 5)
		Direction = 3;
	if (Width / 2 + 5 <= x && x <= Width && Height / 2 - 5 <= y && y <= Height / 2 + 5)
		Direction = 4;
	if (Direction != -1)
		(static_cast<TAutomapWindow *>(GetParent()))->Scroll(Direction);
	return true;
}

void TCompassRose::MouseHover([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	SetMouseOverText(TextScrollAutomap);
}

// --- TCapacityLabel, TSoulLabel ---------------------------------------------------

void TCapacityLabel::Draw(int Surface, int x, int y)
{
	TFixedString<20> Text;
	PrintFormattedText(Surface, SkinProperties().CapacityLabel.Caption.x + x, SkinProperties().CapacityLabel.Caption.y + y,
		SkinProperties().CapacityLabel.CaptionFont, "Cap:");
	Text.Format("{}", GetPlayerCapacity());
	PrintFormattedText(Surface, SkinProperties().CapacityLabel.Value.x + x, SkinProperties().CapacityLabel.Value.y + y,
		SkinProperties().CapacityLabel.ValueFont, Text);
}

void TSoulLabel::Draw(int Surface, int x, int y)
{
	TFixedString<20> Text;
	PrintFormattedText(Surface, SkinProperties().SoulLabel.Caption.x + x, SkinProperties().SoulLabel.Caption.y + y,
		SkinProperties().SoulLabel.CaptionFont, "Soul:");
	Text.Format("{}", GetPlayerSoul());
	PrintFormattedText(Surface, SkinProperties().SoulLabel.Value.x + x, SkinProperties().SoulLabel.Value.y + y,
		SkinProperties().SoulLabel.ValueFont, Text);
}

// --- TPlayerStates ----------------------------------------------------------------

// The small row has a background (skin 100) with a right edge
// (101).
TPlayerStates::TPlayerStates(bool islarge)
{
	IsLarge = islarge;
	if (!islarge) {
		MaxIcons = 9;
		DrawingListLength = 2;
		DrawingList.reset(new TDrawing[2]);
		DrawingList[0] = TDrawing(SKIN_PLAYER_STATES_BACKGROUND, 0, 0, SkinProperties().SmallInventoryStates.width, SkinProperties().SmallInventoryStates.height, 0, 0);
		DrawingList[1] = TDrawing(SKIN_PLAYER_STATES_EDGE_RIGHT, SkinProperties().SmallInventoryStates.width - 1, 0, Skins()[SKIN_PLAYER_STATES_EDGE_RIGHT].width, Skins()[SKIN_PLAYER_STATES_EDGE_RIGHT].height, 0,
			0);
	} else {
		MaxIcons = 10;
		DrawingListLength = 1;
		DrawingList.reset(new TDrawing[1]);
		DrawingList[0] = TDrawing(SKIN_PLAYER_STATES_BACKGROUND, 0, 0, SkinProperties().InventoryView.States.width,
			SkinProperties().InventoryView.States.height, 0, 0);
	}
}

// The icons side by side, PlayerStates.IconDistance pixels apart.
void TPlayerStates::Draw(int Surface, int x, int y)
{
	TComponent::Draw(Surface, x, y);
	const int Icons = GetNumberOfIcons();
	for (int i = 0; i < Icons; i++) {
		int destx = SkinProperties().PlayerStates.IconDistance * i;
		int desty;
		if (IsLarge) {
			destx += SkinProperties().PlayerStates.LargeRow.x;
			desty = SkinProperties().PlayerStates.LargeRow.y;
		} else {
			destx += SkinProperties().PlayerStates.SmallRow.x;
			desty = SkinProperties().PlayerStates.SmallRow.y;
		}
		const int Skin = GetIcon(i);
		PATELLA_ASSERT_VALUES(x+destx>=0, x+destx);
		PATELLA_ASSERT_VALUES(y+desty>=0, y+desty);
		if (Skin != SKIN_NONE)
			DrawSkin(Surface, x + destx, y + desty, Skins()[Skin].width, Skins()[Skin].height, Skin, 0, 0);
	}
}

int TPlayerStates::GetNumberOfIcons() const
{
	int Count = IsPlayerPoisoned() ? 1 : 0;
	if (IsPlayerBurning())
		Count++;
	if (IsPlayerManaShielded())
		Count++;
	if (IsPlayerParalyzed())
		Count++;
	if (HasPlayerSkull())
		Count++;
	if (IsPlayerInFight())
		Count++;
	if (IsPlayerDrunk())
		Count++;
	if (IsPlayerElectrified())
		Count++;
	if (IsPlayerHasted())
		Count++;
	if (Count > MaxIcons)
		Count = MaxIcons;
	return Count;
}

void TPlayerStates::MouseHover(int x, int y)
{
	const int Position = GetIconAt(x, y);
	if (Position != -1) {
		const int skin = GetIcon(Position);
		if (skin != SKIN_NONE) {
			const char *Text;
			switch (skin) {
			case SKIN_STATE_POISONED:
				Text = TextPoisoned;
				break;
			case SKIN_STATE_BURNING:
				Text = TextOnFire;
				break;
			case SKIN_STATE_ELECTRIFIED:
				Text = TextStruckByEnergy;
				break;
			case SKIN_STATE_DRUNK:
				Text = TextDrunk;
				break;
			case SKIN_STATE_MANA_SHIELD:
				Text = TextMagicShield;
				break;
			case SKIN_STATE_PARALYZED:
				Text = TextParalysed;
				break;
			case SKIN_STATE_HASTED:
				Text = TextHasted;
				break;
			case SKIN_STATE_IN_FIGHT:
				Text = TextNoLogoutInFight;
				break;
			case SKIN_STATE_SKULL_1:
				Text = TextMayBeAttacked;
				break;
			case SKIN_STATE_SKULL_2:
				Text = TextPartyMember;
				break;
			case SKIN_STATE_SKULL_3:
				Text = TextAttackedUnmarked;
				break;
			case SKIN_STATE_SKULL_4:
				Text = TextKilledTooMany;
				break;
			default:
				TERROR1("unknown skin", skin);
			}
			SetMouseOverText(Text);
		}
	}
}

// The icon under x, y; -1 for none. Each icon area is one pixel larger
// than the distance.
int TPlayerStates::GetIconAt(int x, int y) const
{
	const int Icons = GetNumberOfIcons();
	for (int i = 0; i < Icons; i++) {
		int IconX = SkinProperties().PlayerStates.SmallRow.x;
		int IconY = SkinProperties().PlayerStates.SmallRow.y;
		if (IsLarge) {
			IconX = SkinProperties().PlayerStates.LargeRow.x;
			IconY = SkinProperties().PlayerStates.LargeRow.y;
		}
		IconX = SkinProperties().PlayerStates.IconDistance * i + IconX;
		if (IconX <= x && x <= IconX + SkinProperties().PlayerStates.IconDistance && IconY <= y
			&& y <= IconY + SkinProperties().PlayerStates.IconDistance)
			return i;
	}
	return -1;
}

// The skin of the Position'th state that is on; 0 when fewer states are on.
// State 9 has no icon. The original's default, never reached (i is 0-9), was
// TERROR1(1063, "index out of range", i); it does nothing here (issue #79):
// MSVC x86 removed its code but kept the function's exception frame, without
// the frame's handler in the SafeSEH table, so an exception of a callee ended
// the program (notes/decisions.md, issue #79).
int TPlayerStates::GetIcon(int Position)
{
	PATELLA_ASSERT_VALUES(In(Position,0,SkinProperties().PlayerStates.NumberOfIcons-1), Position,
		SkinProperties().PlayerStates.NumberOfIcons);
	int Skin = SKIN_NONE;
	int Found = 0;
	for (int i = 0; i <= 9 && Found <= Position; i++) {
		Skin = SKIN_NONE;
		switch (i) {
		case 0:
			if (IsPlayerPoisoned()) {
				Found++;
				Skin = SKIN_STATE_POISONED;
			}
			break;
		case 1:
			if (IsPlayerBurning()) {
				Found++;
				Skin = SKIN_STATE_BURNING;
			}
			break;
		case 2:
			if (IsPlayerManaShielded()) {
				Found++;
				Skin = SKIN_STATE_MANA_SHIELD;
			}
			break;
		case 3:
			if (IsPlayerParalyzed()) {
				Found++;
				Skin = SKIN_STATE_PARALYZED;
			}
			break;
		case 4:
			switch (GetCreature(GetPlayerID())->PKFlag) {
			case PK_YELLOW:
				Found++;
				Skin = SKIN_STATE_SKULL_1;
				break;
			case PK_GREEN:
				Found++;
				Skin = SKIN_STATE_SKULL_2;
				break;
			case PK_WHITE:
				Found++;
				Skin = SKIN_STATE_SKULL_3;
				break;
			case PK_RED:
				Found++;
				Skin = SKIN_STATE_SKULL_4;
				break;
			default:
				break;
			}
			break;
		case 5:
			if (IsPlayerInFight()) {
				Found++;
				Skin = SKIN_STATE_IN_FIGHT;
			}
			break;
		case 6:
			if (IsPlayerDrunk()) {
				Found++;
				Skin = SKIN_STATE_DRUNK;
			}
			break;
		case 7:
			if (IsPlayerElectrified()) {
				Found++;
				Skin = SKIN_STATE_ELECTRIFIED;
			}
			break;
		case 8:
			if (IsPlayerHasted()) {
				Found++;
				Skin = SKIN_STATE_HASTED;
			}
			break;
		case 9:
			Skin = SKIN_NONE;
			break;
		default: // never reached (i is 0-9): the original's TERROR1, see above
			break;
		}
	}
	return Skin;
}

// --- TSmallInventoryWindow --------------------------------------------------------

TSmallInventoryWindow::TSmallInventoryWindow()
	: TContentWindow(SMALL_INVENTORY_WINDOW, true)
{
	DrawingListLength = 1;
	DrawingList.reset(new TDrawing[1]);
	DrawingList[0] = TDrawing(SKIN_SMALL_INVENTORY_DROP_AREA, SkinProperties().SmallInventoryDropArea.x, SkinProperties().SmallInventoryDropArea.y, SkinProperties().SmallInventoryDropArea.width,
		SkinProperties().SmallInventoryDropArea.height, 0, 0);
}

// A white frame around the drop area while an object is dragged over it.
void TSmallInventoryWindow::Draw(int Surface, int x, int y)
{
	TContentWindow::Draw(Surface, x, y);
	if (In(GetMouseX() - SkinProperties().SmallInventoryDropArea.x - x, 0, SkinProperties().SmallInventoryDropArea.width - 1)
		&& In(GetMouseY() - SkinProperties().SmallInventoryDropArea.y - y, 0, SkinProperties().SmallInventoryDropArea.height - 1)
		&& GetActionState() == ACTIONSTATE_DRAGGING)
		GetSurface()->DrawFrame(Surface,
			TRect(SkinProperties().SmallInventoryDropArea.x + x, SkinProperties().SmallInventoryDropArea.y + y,
				SkinProperties().SmallInventoryDropArea.width, SkinProperties().SmallInventoryDropArea.height),
			1, TRGBColor(255, 255, 255));
}

namespace {

// Patella-Plus, the option Drag Full Stacks (General Options). Whether a pile
// dropped on a slot asks for the count (the Move Objects dialog) instead of
// moving whole: without the option unless Ctrl (0x400) is held, as the
// original; with it only when Ctrl is held. TMapWindow::LeftButtonUp follows
// the same rule for the floor.
bool AsksForCount(int Flags)
{
	const bool Ctrl = (Flags & KEY_CONTROL) != 0;
	return GetDragFullStacks() ? Ctrl : !Ctrl;
}

} // namespace

// An object dropped on the area goes into the inventory (0xffff, 0, 0: the
// first free slot); a pile asks for the count or moves whole
// (AsksForCount), and Shift (0x200) moves one.
bool TSmallInventoryWindow::LeftButtonUp(int x, int y, int Flags)
{
	const bool Result = TContentWindow::LeftButtonUp(x, y, Flags);
	if (In(x - SkinProperties().SmallInventoryDropArea.x, 0, SkinProperties().SmallInventoryDropArea.width - 1)
		&& In(y - SkinProperties().SmallInventoryDropArea.y, 0, SkinProperties().SmallInventoryDropArea.height - 1) && GetActionState() == ACTIONSTATE_DRAGGING) {
		int Index, SrcX, SrcY, SrcZ;
		const TObject *Obj = GetDragObject(&Index, &SrcX, &SrcY, &SrcZ);
		if (!GetObjectFlag(Obj->Type, CUMULATIVE) || Obj->Data < 2 || (Flags & SHIFT_OR_BOTH_BUTTONS) != 0) {
			SendMoveObject(SrcX, SrcY, SrcZ, Obj->Type, Index, POSITION_NOT_ON_MAP, 0, 0, 1);
		} else if (AsksForCount(Flags)) {
			SetDragTarget(POSITION_NOT_ON_MAP, 0, 0);
			AddMessage(MESSAGE_MOVE_OBJECTS_DIALOG, 0, 0);
		} else {
			SendMoveObject(SrcX, SrcY, SrcZ, Obj->Type, Index, POSITION_NOT_ON_MAP, 0, 0, Obj->Data);
		}
	}
	return Result;
}

bool TSmallInventoryWindow::HelpMouseMove(int x, int y)
{
	if (!TWindow::HelpMouseMove(x, y)) {
		SetHelpHandle(HELP_MINIMISED_INVENTORY);
		SetHelpSize(Width, Height);
	}
	return true;
}

// --- TInventoryView ---------------------------------------------------------------

// The "minimise" button is event 23 of the main window.
TInventoryView::TInventoryView(TWindow *Receiver)
{
	SetSize(SkinProperties().InventoryView.Width, SkinProperties().InventoryView.Height);
	TSkinButton *Minimise = new TSkinButton(0, 0, SKIN_MINIMISE_BUTTON_UP, SKIN_MINIMISE_BUTTON_DOWN, TCallback(Receiver, EVENT_MINIMISE_INVENTORY), TextMinimiseInventory, false);
	InsertChild(Minimise);
	SetChildPosition(Minimise, SkinProperties().InventoryView.MinimiseButton.x, SkinProperties().InventoryView.MinimiseButton.y);
	TCapacityLabel *Capacity = new TCapacityLabel;
	InsertChild(Capacity);
	SetChildArea(Capacity, SkinProperties().InventoryView.Capacity);
	TSoulLabel *Soul = new TSoulLabel;
	InsertChild(Soul);
	SetChildArea(Soul, SkinProperties().InventoryView.Soul);
	TPlayerStates *States = new TPlayerStates(true);
	InsertChild(States);
	SetChildArea(States, SkinProperties().InventoryView.States);
	Arrange();
}

// The backgrounds of capacity and soul (skin 98), the children, then the ten slots: a
// frame (skin 102), the object or the slot's empty picture (skins
// InventoryView.EmptySlotSkins), and a white frame while an object is dragged over a
// slot.
void TInventoryView::Draw(int Surface, int x, int y)
{
	int destx = SkinProperties().InventoryView.Capacity.x + x;
	int desty = SkinProperties().InventoryView.Capacity.y + y;
	PATELLA_ASSERT_VALUES(destx>=0, destx);
	PATELLA_ASSERT_VALUES(desty>=0, desty);
	DrawSkin(Surface, destx, desty, SkinProperties().InventoryView.Capacity.width, SkinProperties().InventoryView.Capacity.height, SKIN_SOUL_CAPACITY_BACKGROUND,
		0, 0);
	destx = SkinProperties().InventoryView.Soul.x + x;
	desty = y + SkinProperties().InventoryView.Soul.y;
	PATELLA_ASSERT_VALUES(destx>=0, destx);
	PATELLA_ASSERT_VALUES(desty>=0, desty);
	DrawSkin(Surface, destx, desty, SkinProperties().InventoryView.Soul.width, SkinProperties().InventoryView.Soul.height, SKIN_SOUL_CAPACITY_BACKGROUND,
		0, 0);
	TWindow::Draw(Surface, x, y);
	for (int i = 0; i < 10; i++) {
		const TObject Obj = *GetBodyObject(i + 1);
		DrawPicturePart(Surface, SkinProperties().InventoryView.Slots[i].x + x, SkinProperties().InventoryView.Slots[i].y + y, 4,
			Skins()[SKIN_SLOT_FRAME].x, Skins()[SKIN_SLOT_FRAME].y, Skins()[SKIN_SLOT_FRAME].width, Skins()[SKIN_SLOT_FRAME].height);
		const int ObjX = SkinProperties().InventoryView.Objects[i].x + x;
		const int ObjY = SkinProperties().InventoryView.Objects[i].y + y;
		if (Obj.Type == 0) {
			const int Empty = SkinProperties().InventoryView.EmptySlotSkins[i];
			DrawPicturePart(Surface, ObjX, ObjY, 4, Skins()[Empty].x, Skins()[Empty].y, Skins()[Empty].width,
				Skins()[Empty].height);
		} else {
			DrawObject(Surface, ObjX, ObjY, 32, Obj, TRGBColor(0, 0, 0), TRect(ObjX, ObjY, 32, 32),
				SkinProperties().ObjectCountFont, false);
		}
		if (GetActionState() == ACTIONSTATE_DRAGGING && In(GetMouseX(), ObjX, ObjX + 31) && In(GetMouseY(), ObjY, ObjY + 31))
			GetSurface()->DrawFrame(Surface, TRect(ObjX - 1, ObjY - 1, 34, 34), 1, TRGBColor(255, 255, 255));
	}
}

// A press on a filled slot starts dragging its object.
bool TInventoryView::LeftButtonDown(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonDown(x, y, Flags)) {
		const int Slot = GetSlotAt(x, y);
		if (Slot == 0)
			return false;
		const TObject *Obj = GetBodyObject(Slot);
		if (Obj->Type != 0)
			SetDragObject(*Obj, 0, POSITION_NOT_ON_MAP, Slot, 0);
	}
	return true;
}

// Action state 7 uses the use object with the slot's object, 8 asks for a
// player, 6 drops the dragged object into the slot (a pile asks for the
// count or moves whole, AsksForCount, and Shift moves one); a release
// with flags is a right click.
bool TInventoryView::LeftButtonUp(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonUp(x, y, Flags)) {
		const int Slot = GetSlotAt(x, y);
		if (Slot == 0)
			return false;
		if (GetActionState() == ACTIONSTATE_USE_WITH) {
			const TObject *Obj = GetBodyObject(Slot);
			if (Obj->Type != 0) {
				int SrcIndex, SrcX, SrcY, SrcZ;
				const TObject &SrcObj = *GetUseObject(&SrcIndex, &SrcX, &SrcY, &SrcZ);
				PATELLA_ASSERT(SrcObj!=0);
				PATELLA_ASSERT(SrcObj.Type!=CREATURE);
				SendUseObjectWith(SrcX, SrcY, SrcZ, SrcObj.Type, SrcIndex, POSITION_NOT_ON_MAP, Slot, 0, Obj->Type, 0);
				return true;
			}
		} else if (GetActionState() == ACTIONSTATE_TRADE_WITH) {
			SetStatusMessage("Select a player to trade with.", false);
			return true;
		} else if (GetActionState() == ACTIONSTATE_DRAGGING) {
			int DragIndex, DragX, DragY, DragZ;
			const TObject *DragObject = GetDragObject(&DragIndex, &DragX, &DragY, &DragZ);
			if (DragX != POSITION_NOT_ON_MAP || DragY != Slot) {
				if (!GetObjectFlag(DragObject->Type, CUMULATIVE) || DragObject->Data < 2 || (Flags & SHIFT_OR_BOTH_BUTTONS) != 0) {
					SendMoveObject(DragX, DragY, DragZ, DragObject->Type, DragIndex, POSITION_NOT_ON_MAP, Slot, 0, 1);
				} else if (AsksForCount(Flags)) {
					SetDragTarget(POSITION_NOT_ON_MAP, Slot, 0);
					AddMessage(MESSAGE_MOVE_OBJECTS_DIALOG, 0, 0);
					return true;
				} else {
					SendMoveObject(DragX, DragY, DragZ, DragObject->Type, DragIndex, POSITION_NOT_ON_MAP, Slot, 0,
						DragObject->Data);
				}
			}
		} else if (Flags != 0) {
			return RightButtonUp(x, y, Flags);
		}
	}
	return true;
}

// The trade and use objects become the slot's object. Shift looks at it;
// with classic controls Ctrl opens the menu (message 52) and a plain click
// uses it, otherwise the other way round. An object used with something
// (MULTIUSE) starts action state 7 (message 44).
bool TInventoryView::RightButtonUp(int x, int y, int Flags)
{
	if (!TWindow::RightButtonUp(x, y, Flags)) {
		const int Slot = GetSlotAt(x, y);
		if (Slot == 0)
			return false;
		SetTradeObject(TObject(), 0, 0, 0, 0);
		SetUseObject(TObject(), 0, 0, 0, 0);
		SetTradeInfo(0);
		const TObject *Obj = GetBodyObject(Slot);
		SetTradeObject(*Obj, 0, POSITION_NOT_ON_MAP, Slot, 0);
		SetUseObject(*Obj, 0, POSITION_NOT_ON_MAP, Slot, 0);
		if (Obj->Type != 0) {
			if (GetClassicControl()) {
				if ((Flags & SHIFT_OR_BOTH_BUTTONS) != 0)
					SendLook(POSITION_NOT_ON_MAP, Slot, 0, Obj->Type, 0);
				else if ((Flags & KEY_CONTROL) != 0)
					AddMessage(MESSAGE_OBJECT_MENU, 0, 0);
				else if (GetObjectFlag(Obj->Type, MULTIUSE))
					AddMessage(MESSAGE_USE_WITH, 0, 0);
				else
					SendUseObject(POSITION_NOT_ON_MAP, Slot, 0, Obj->Type, 0, GetFreeContainer());
			} else {
				if ((Flags & SHIFT_OR_BOTH_BUTTONS) != 0)
					SendLook(POSITION_NOT_ON_MAP, Slot, 0, Obj->Type, 0);
				else if ((Flags & KEY_CONTROL) == 0)
					AddMessage(MESSAGE_OBJECT_MENU, 0, 0);
				else if (GetObjectFlag(Obj->Type, MULTIUSE))
					AddMessage(MESSAGE_USE_WITH, 0, 0);
				else
					SendUseObject(POSITION_NOT_ON_MAP, Slot, 0, Obj->Type, 0, GetFreeContainer());
			}
		}
	}
	return true;
}

bool TInventoryView::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	SetHelpHandle(HELP_INVENTORY);
	const TComponent *Parent = GetParent();
	PATELLA_ASSERT(Parent!=nullptr);
	SetHelpSize(Parent->Width, Parent->Height);
	MoveHelpPosition(-OffsetX, -OffsetY);
	return true;
}

// The body slot (1-10) whose area (InventoryView.Slots) holds x, y; 0 for
// none.
int TInventoryView::GetSlotAt(int x, int y)
{
	for (int i = 0; i < 10; i++) {
		const TRect &Area = SkinProperties().InventoryView.Slots[i];
		if (x - Area.x >= 0 && x - Area.x <= Area.width && y - Area.y >= 0 && y - Area.y <= Area.height)
			return i + 1;
	}
	return 0;
}

// --- TCombatControls --------------------------------------------------------------

// The layout (LargeCombatControls for the large controls, SmallCombatControls for the small
// ones) is copied first. The radio groups have no selection until the modes are read at the
// end (none for an unknown mode).
TCombatControls::TCombatControls(TWindow *Receiver, bool Large)
{
	IsLarge = Large;
	const TCombatControlsProperties Layout = Large ? SkinProperties().LargeCombatControls : SkinProperties().SmallCombatControls;
	AttackModes.reset(new TRadioButtonGroup);
	ChaseModes.reset(new TRadioButtonGroup);
	SetSize(Layout.Width, Layout.Height);
	TSkinButton *Offensive = new TSkinButton(Layout.Buttons[0].x, Layout.Buttons[0].y, SKIN_OFFENSIVE_BUTTON_UP, SKIN_OFFENSIVE_BUTTON_DOWN, TCallback(this, EVENT_ATTACK_OFFENSIVE),
		TextOffensiveFighting, false);
	Offensive->SetRadioButtonGroup(AttackModes.get());
	InsertChild(Offensive);
	TSkinButton *Balanced = new TSkinButton(Layout.Buttons[1].x, Layout.Buttons[1].y, SKIN_BALANCED_BUTTON_UP, SKIN_BALANCED_BUTTON_DOWN, TCallback(this, EVENT_ATTACK_BALANCED),
		TextBalancedFighting, false);
	Balanced->SetRadioButtonGroup(AttackModes.get());
	InsertChild(Balanced);
	TSkinButton *Defensive = new TSkinButton(Layout.Buttons[2].x, Layout.Buttons[2].y, SKIN_DEFENSIVE_BUTTON_UP, SKIN_DEFENSIVE_BUTTON_DOWN, TCallback(this, EVENT_ATTACK_DEFENSIVE),
		TextDefensiveFighting, false);
	Defensive->SetRadioButtonGroup(AttackModes.get());
	InsertChild(Defensive);
	StandButton = new TSkinButton(Layout.Buttons[3].x, Layout.Buttons[3].y, SKIN_STAND_BUTTON_UP, SKIN_STAND_BUTTON_DOWN, TCallback(this, EVENT_CHASE_STAND),
		TextStandWhileFighting, false);
	StandButton->SetRadioButtonGroup(ChaseModes.get());
	InsertChild(StandButton);
	ChaseButton = new TSkinButton(Layout.Buttons[4].x, Layout.Buttons[4].y, SKIN_CHASE_BUTTON_UP, SKIN_CHASE_BUTTON_DOWN, TCallback(this, EVENT_CHASE_ON),
		TextChaseOpponent, false);
	ChaseButton->SetRadioButtonGroup(ChaseModes.get());
	InsertChild(ChaseButton);
	SecureButton = new TToggleButton(Layout.Buttons[5].x, Layout.Buttons[5].y, SKIN_SECURE_BUTTON_UP, SKIN_SECURE_BUTTON_DOWN, TCallback(this, EVENT_SECURE_MODE),
		TextCannotAttackUnmarked, TextCanAttackAny, false);
	InsertChild(SecureButton);
	InsertChild(new TTextButton(Layout.Buttons[6].x, Layout.Buttons[6].y, "Stop", TCallback(Receiver, EVENT_STOP),
		TextStopCurrentAction));
	if (!IsLarge) {
		InsertChild(new TSkinButton(Layout.Buttons[7].x, Layout.Buttons[7].y, SKIN_OPTIONS_BUTTON_UP, SKIN_OPTIONS_BUTTON_DOWN, TCallback(Receiver, EVENT_OPTIONS),
			TextOpenOptions, false));
		InsertChild(new TSkinButton(Layout.Buttons[8].x, Layout.Buttons[8].y, SKIN_HELP_BUTTON_UP, SKIN_HELP_BUTTON_DOWN, TCallback(Receiver, EVENT_HELP),
			TextOpenHelpMenu, false));
	} else {
		InsertChild(new TTextButton(Layout.Buttons[7].x, Layout.Buttons[7].y, "Options", TCallback(Receiver, EVENT_OPTIONS),
			TextOpenOptions));
		InsertChild(new TTextButton(Layout.Buttons[8].x, Layout.Buttons[8].y, "Help", TCallback(Receiver, EVENT_HELP),
			TextOpenHelpMenu));
	}
	switch (GetAttackMode()) {
	case ATTACK_OFFENSIVE:
		AttackModes->Selected = Offensive;
		break;
	case ATTACK_BALANCED:
		AttackModes->Selected = Balanced;
		break;
	case ATTACK_DEFENSIVE:
		AttackModes->Selected = Defensive;
		break;
	default:
		break;
	}
	switch (GetChaseMode()) {
	case CHASE_STAND:
		ChaseModes->Selected = StandButton;
		break;
	case CHASE_ON:
		ChaseModes->Selected = ChaseButton;
		break;
	default:
		break;
	}
	SecureButton->State = !GetSecureMode();
	Arrange();
}

// The chase mode can change elsewhere (Escape): the selection
// follows it.
void TCombatControls::Draw(int Surface, int x, int y)
{
	switch (GetChaseMode()) {
	case CHASE_STAND:
		if (ChaseModes->Selected != StandButton)
			ChaseModes->Selected = StandButton;
		break;
	case CHASE_ON:
		if (ChaseModes->Selected != ChaseButton)
			ChaseModes->Selected = ChaseButton;
		break;
	default:
		break;
	}
	TWindow::Draw(Surface, x, y);
}

bool TCombatControls::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	if (!IsLarge)
		SetHelpHandle(HELP_COMBAT_CONTROLS_SMALL);
	else
		SetHelpHandle(HELP_COMBAT_CONTROLS_LARGE);
	const TComponent *Parent = GetParent();
	PATELLA_ASSERT(Parent!=nullptr);
	SetHelpSize(Parent->Width, Parent->Height);
	MoveHelpPosition(-OffsetX, -OffsetY);
	return true;
}

// Every change is sent to the server.
void TCombatControls::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_ATTACK_DEFENSIVE:
		SetAttackMode(ATTACK_DEFENSIVE);
		break;
	case EVENT_ATTACK_BALANCED:
		SetAttackMode(ATTACK_BALANCED);
		break;
	case EVENT_ATTACK_OFFENSIVE:
		SetAttackMode(ATTACK_OFFENSIVE);
		break;
	case EVENT_CHASE_STAND:
		SetChaseMode(CHASE_STAND);
		break;
	case EVENT_CHASE_ON:
		SetChaseMode(CHASE_ON);
		break;
	case EVENT_SECURE_MODE:
		SetSecureMode(!SecureButton->State);
		break;
	default:
		return;
	}
	SendFightModes();
}

// --- TStatusWindow ----------------------------------------------------------------

TStatusWindow::TStatusWindow()
	: TContentWindow(STATUS_WINDOW, true)
{
	DrawingListLength = 4;
	DrawingList.reset(new TDrawing[4]);
	Data44 = 100;
	Data48 = 100;
	SetSize(SkinProperties().SideBarWidth, SkinProperties().StatusWindow.Height);
	Arrange();
}

// The values, then the bars cut to the share of the maximum (full when the
// value is not below it).
void TStatusWindow::Draw(int Surface, int x, int y)
{
	TContentWindow::Draw(Surface, x, y);
	const int Health = GetPlayerHealth();
	const int MaxHealth = GetPlayerMaxHealth();
	const int Mana = GetPlayerMana();
	const int MaxMana = GetPlayerMaxMana();
	TFixedString<16> Text;
	Text.Format("{}", Health);
	PrintFormattedText(Surface, SkinProperties().StatusWindow.HitpointsValue.x + x, SkinProperties().StatusWindow.HitpointsValue.y + y,
		SkinProperties().StatusWindow.ValueFont, Text);
	Text.Format("{}", Mana);
	PrintFormattedText(Surface, SkinProperties().StatusWindow.ManaValue.x + x, SkinProperties().StatusWindow.ManaValue.y + y,
		SkinProperties().StatusWindow.ValueFont, Text);
	int BarWidth = SkinProperties().StatusWindow.HitpointsBar.width;
	if (Health < MaxHealth)
		BarWidth = SkinProperties().StatusWindow.HitpointsBar.width * Health / MaxHealth;
	PATELLA_ASSERT_VALUES(x+SkinProperties().StatusWindow.HitpointsBar.x>=0, x);
	PATELLA_ASSERT_VALUES(y+SkinProperties().StatusWindow.HitpointsBar.y>=0, y);
	DrawSkin(Surface, SkinProperties().StatusWindow.HitpointsBar.x + x, y + SkinProperties().StatusWindow.HitpointsBar.y,
		BarWidth, Skins()[SKIN_HITPOINTS_BAR].height, SKIN_HITPOINTS_BAR, 0, 0);
	BarWidth = SkinProperties().StatusWindow.ManaBar.width;
	if (Mana < MaxMana)
		BarWidth = SkinProperties().StatusWindow.ManaBar.width * Mana / MaxMana;
	PATELLA_ASSERT_VALUES(x+SkinProperties().StatusWindow.ManaBar.x>=0, x);
	PATELLA_ASSERT_VALUES(y+SkinProperties().StatusWindow.ManaBar.y>=0, y);
	DrawSkin(Surface, SkinProperties().StatusWindow.ManaBar.x + x, SkinProperties().StatusWindow.ManaBar.y + y, BarWidth,
		Skins()[SKIN_MANA_BAR].height, SKIN_MANA_BAR, 0, 0);
}

bool TStatusWindow::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	SetHelpHandle(HELP_STATUS_BARS);
	SetHelpSize(Width, Height);
	return true;
}

// Two pictures (skins 119, 122) and the two bars' backgrounds (120, 123).
void TStatusWindow::Arrange()
{
	TContentWindow::Arrange();
	DrawingList[0] = TDrawing(SKIN_HITPOINTS_ICON, SkinProperties().StatusWindow.Pictures.x, SkinProperties().StatusWindow.Pictures.y,
		Skins()[SKIN_HITPOINTS_ICON].width, Skins()[SKIN_HITPOINTS_ICON].height, 0, 0);
	DrawingList[1] = TDrawing(SKIN_MANA_ICON, SkinProperties().StatusWindow.Pictures.width, SkinProperties().StatusWindow.Pictures.height,
		Skins()[SKIN_MANA_ICON].width, Skins()[SKIN_MANA_ICON].height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_HITPOINTS_BAR_BACKGROUND, SkinProperties().StatusWindow.HitpointsBar.x, SkinProperties().StatusWindow.HitpointsBar.y,
		SkinProperties().StatusWindow.HitpointsBar.width, SkinProperties().StatusWindow.HitpointsBar.height, 0, 0);
	DrawingList[3] = TDrawing(SKIN_MANA_BAR_BACKGROUND, SkinProperties().StatusWindow.ManaBar.x, SkinProperties().StatusWindow.ManaBar.y,
		SkinProperties().StatusWindow.ManaBar.width, SkinProperties().StatusWindow.ManaBar.height, 0, 0);
}

// --- TContainerWindowBody ---------------------------------------------------------

TContainerWindowBody::TContainerWindowBody(int _Container)
{
	Container = _Container;
	ScrollBar = new TScrollBar(TRect(0, 0, SkinProperties().ScrollBarMinimumWidth, SkinProperties().ScrollBarMinimumHeight), 0, 0, 0, 0, 10);
	InsertChild(ScrollBar);
	HasParent = false;
	Arrange();
}

// Its work is the close since issue #108.
TContainerWindowBody::~TContainerWindowBody()
{
	CheckClosed("TContainerWindowBody");
}

// The destructor's work until issue #108: closing the window closes the
// container; then the children.
void TContainerWindowBody::OnClose()
{
	try {
		SendCloseContainer(Container);
	} catch (...) {
		CloseWhileUnwinding([this] { TSubWindowBody::OnClose(); });
		throw;
	}
	TSubWindowBody::OnClose();
}

// The background (skin 7, scrolled with the view), then the slots row by
// row, cut at the top and the bottom edge: the slot picture (skin 125),
// the object and, while an object is dragged over the slot, a white frame.
// Odd, kept: a slot cut at the top shows the top of its picture, not the
// part below the edge.
void TContainerWindowBody::Draw(int Surface, int x, int y)
{
	if (ObjectCount != GetContainerObjectCount(Container))
		Arrange();
	if (!IsMinimized()) {
		if (!Minimized)
			TWindow::Draw(Surface, x, y);
		const int ScrollY = ScrollBar->Range->Value;
		const int Bottom = Height - 1;
		PATELLA_ASSERT_VALUES(x>=0, x);
		PATELLA_ASSERT_VALUES(y>=0, y);
		DrawSkin(Surface, x, y, Width - ScrollBar->Width, Height, SKIN_BACKGROUND, 0, ScrollY % Skins()[SKIN_BACKGROUND].height);
		if (IsContainerOpen(Container)) {
			for (int i = 0; i < Capacity; i++) {
				const int SlotX = i % SkinProperties().ContainerSlotsPerRow * SkinProperties().ContainerColumnWidth;
				const int SlotY = i / SkinProperties().ContainerSlotsPerRow * SkinProperties().ContainerRowHeight - ScrollY;
				int Top = SlotY + SkinProperties().ContainerFirstSlot.y;
				int ClipHeight = SkinProperties().ContainerSlotSizeAndObject.y;
				if (Top + ClipHeight > 0 && Top <= Bottom) {
					if (Top < 0) {
						ClipHeight = Top + ClipHeight;
						Top = 0;
					}
					if (ClipHeight + Top > Bottom)
						ClipHeight = Bottom - Top + 1;
					const int ScreenX = SkinProperties().ContainerFirstSlot.x + SlotX + x;
					const int ScreenY = Top + y;
					const int SlotWidth = SkinProperties().ContainerSlotSizeAndObject.x;
					DrawPicturePart(Surface, ScreenX, ScreenY, 4, Skins()[SKIN_CONTAINER_SLOT].x, Skins()[SKIN_CONTAINER_SLOT].y, SlotWidth, ClipHeight);
					if (i < ObjectCount)
						DrawObject(Surface, SkinProperties().ContainerSlotSizeAndObject.width + SlotX + x,
							SkinProperties().ContainerSlotSizeAndObject.height + SlotY + y, 32, GetContainerContent(Container, i),
							TRGBColor(0, 0, 0), TRect(ScreenX, ScreenY, SlotWidth, ClipHeight), SkinProperties().ObjectCountFont,
							false);
					if (GetActionState() == ACTIONSTATE_DRAGGING && In(GetMouseX(), ScreenX, ScreenX + SlotWidth - 1)
						&& In(GetMouseY(), ScreenY, ScreenY + ClipHeight - 1))
						GetSurface()->DrawFrameClipped(Surface,
							TRect(SkinProperties().ContainerFirstSlot.x + SlotX + x,
								SlotY + SkinProperties().ContainerFirstSlot.y + y, 34, 34),
							1, TRGBColor(255, 255, 255),
							TRect(ScreenX, ScreenY, SlotWidth, ClipHeight));
				}
			}
		}
	}
}

// A window taller than its content keeps its height but does not grow;
// the scroll bar stays at the right edge.
void TContainerWindowBody::SetSize(int Width, int Height)
{
	const int ScrollbarX = Width - ScrollBar->Width;
	PATELLA_ASSERT_VALUES(ScrollbarX>0, ScrollbarX);
	const int Maximum = GetMaximumHeight();
	if (Height > Maximum && this->Height < Height)
		Height = (std::max)(this->Height, Maximum);
	TWindow::SetSize(Width, Height);
	ScrollBar->SetPosition(ScrollbarX, 0);
	ScrollBar->SetSize(ScrollBar->Width, Height);
	Arrange();
}

int TContainerWindowBody::getMinimumHeight()
{
	return (std::max)(ScrollBar->getMinimumHeight(), SkinProperties().ContainerMinimumHeight);
}

int TContainerWindowBody::GetMaximumHeight()
{
	return GetContentHeight();
}

// The counts of the container, the scroll range and, in a window, the
// icon and title again.
void TContainerWindowBody::Arrange()
{
	ObjectCount = GetContainerObjectCount(Container);
	Capacity = GetContainerCapacity(Container);
	int ContentHeight = GetContentHeight();
	if (ContentHeight <= Height)
		ContentHeight = Height;
	ScrollBar->Range->SetRange(0, ContentHeight - Height, ScrollBar->Range->Value, Height);
	TSubWindow *Window = static_cast<TSubWindow *>(GetParent());
	if (Window != nullptr) {
		TObjectIcon *Icon = new TObjectIcon(GetContainerObject(Container), &SkinProperties().SubWindow.IconHighlight, false);
		TLabel *Title = new TLabel(SkinProperties().SubWindow.TitleFont, GetContainerName(Container));
		Window->SetIcon(Icon);
		Window->SetTitle(Title);
	}
}

// A press on a filled slot starts dragging its object (from 0xffff, 0x40
// plus the container, the slot).
bool TContainerWindowBody::LeftButtonDown(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonDown(x, y, Flags)) {
		const int Position = GetSlotAt(x, y, true);
		if (Position != -1) {
			const TObject Obj = GetContainerContent(Container, Position);
			if (Obj.Type != 0)
				SetDragObject(Obj, Position, POSITION_NOT_ON_MAP, Container + POSITION_CONTAINER_BASE, Position);
		}
	}
	return true;
}

// As TInventoryView::LeftButtonUp for a container slot; a drop may
// go into an empty slot.
bool TContainerWindowBody::LeftButtonUp(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonUp(x, y, Flags)) {
		if (GetActionState() == ACTIONSTATE_USE_WITH) {
			const int Position = GetSlotAt(x, y, true);
			if (Position != -1) {
				const TObject Obj = GetContainerContent(Container, Position);
				if (Obj.Type != 0) {
					int SrcIndex, SrcX, SrcY, SrcZ;
					const TObject &SrcObj = *GetUseObject(&SrcIndex, &SrcX, &SrcY, &SrcZ);
					PATELLA_ASSERT(SrcObj!=0);
					PATELLA_ASSERT(SrcObj.Type!=CREATURE);
					SendUseObjectWith(SrcX, SrcY, SrcZ, SrcObj.Type, SrcIndex, POSITION_NOT_ON_MAP, Container + POSITION_CONTAINER_BASE, Position,
						Obj.Type, Position);
				}
			}
		} else if (GetActionState() == ACTIONSTATE_TRADE_WITH) {
			SetStatusMessage("Select a player to trade with.", false);
		} else if (GetActionState() == ACTIONSTATE_DRAGGING) {
			const int Position = GetSlotAt(x, y, false);
			int DragIndex, DragX, DragY, DragZ;
			const TObject *DragObject = GetDragObject(&DragIndex, &DragX, &DragY, &DragZ);
			if (DragX != POSITION_NOT_ON_MAP || DragY != Container + POSITION_CONTAINER_BASE || DragZ != Position) {
				if (!GetObjectFlag(DragObject->Type, CUMULATIVE) || DragObject->Data < 2 || (Flags & SHIFT_OR_BOTH_BUTTONS) != 0) {
					SendMoveObject(DragX, DragY, DragZ, DragObject->Type, DragIndex, POSITION_NOT_ON_MAP, Container + POSITION_CONTAINER_BASE, Position,
						1);
				} else if (AsksForCount(Flags)) {
					SetDragTarget(POSITION_NOT_ON_MAP, Container + POSITION_CONTAINER_BASE, Position);
					AddMessage(MESSAGE_MOVE_OBJECTS_DIALOG, 0, 0);
					return true;
				} else {
					SendMoveObject(DragX, DragY, DragZ, DragObject->Type, DragIndex, POSITION_NOT_ON_MAP, Container + POSITION_CONTAINER_BASE, Position,
						DragObject->Data);
				}
			}
		} else if (Flags != 0) {
			return RightButtonUp(x, y, Flags);
		}
	}
	return true;
}

// As TInventoryView::RightButtonUp; a container's object is used into
// the container itself.
bool TContainerWindowBody::RightButtonUp(int x, int y, int Flags)
{
	if (!TWindow::RightButtonUp(x, y, Flags)) {
		const int Position = GetSlotAt(x, y, true);
		if (Position == -1)
			return true;
		SetTradeObject(TObject(), 0, 0, 0, 0);
		SetUseObject(TObject(), 0, 0, 0, 0);
		SetTradeInfo(0);
		const TObject Obj = GetContainerContent(Container, Position);
		SetTradeObject(Obj, Position, POSITION_NOT_ON_MAP, Container + POSITION_CONTAINER_BASE, Position);
		SetUseObject(Obj, Position, POSITION_NOT_ON_MAP, Container + POSITION_CONTAINER_BASE, Position);
		if (Obj.Type == 0)
			return true;
		if (GetClassicControl()) {
			if ((Flags & SHIFT_OR_BOTH_BUTTONS) != 0)
				SendLook(POSITION_NOT_ON_MAP, Container + POSITION_CONTAINER_BASE, Position, Obj.Type, Position);
			else if ((Flags & KEY_CONTROL) != 0)
				AddMessage(MESSAGE_OBJECT_MENU, 0, 0);
			else if (GetObjectFlag(Obj.Type, MULTIUSE))
				AddMessage(MESSAGE_USE_WITH, 0, 0);
			else
				SendUseObject(POSITION_NOT_ON_MAP, Container + POSITION_CONTAINER_BASE, Position, Obj.Type, Position, Container);
		} else {
			if ((Flags & SHIFT_OR_BOTH_BUTTONS) != 0)
				SendLook(POSITION_NOT_ON_MAP, Container + POSITION_CONTAINER_BASE, Position, Obj.Type, Position);
			else if ((Flags & KEY_CONTROL) == 0)
				AddMessage(MESSAGE_OBJECT_MENU, 0, 0);
			else if (GetObjectFlag(Obj.Type, MULTIUSE))
				AddMessage(MESSAGE_USE_WITH, 0, 0);
			else
				SendUseObject(POSITION_NOT_ON_MAP, Container + POSITION_CONTAINER_BASE, Position, Obj.Type, Position, Container);
		}
	}
	return true;
}

// The wheel scrolls the view (1 up, 2 down).
bool TContainerWindowBody::MouseWheel(int x, int y, int Code)
{
	if (!TWindow::MouseWheel(x, y, Code) && (Code == 1 || Code == 2))
		return ScrollBar->MouseWheel(x, y, Code);
	return true;
}

bool TContainerWindowBody::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	SetHelpHandle(HELP_OPEN_CONTAINERS);
	const TComponent *Parent = GetParent();
	PATELLA_ASSERT(Parent!=nullptr);
	SetHelpSize(Parent->Width, Parent->Height);
	MoveHelpPosition(-OffsetX, -OffsetY);
	return true;
}

// The "up" button (event 3) opens the container the container is in,
// once.
void TContainerWindowBody::HandleEvent(int Event)
{
	if (Event == EVENT_CONTAINER_UP && HasParent) {
		HasParent = false;
		SendContainerUp(Container);
	}
}

// MainWindow.cpp's OpenContainer on an open window: the view goes back
// to the top.
void TContainerWindowBody::Update()
{
	ScrollBar->Range->SetValue(0);
	Arrange();
	UpdateUpButton();
}

// The slot under x, y (counted over the whole capacity); -1 for
// none, or, with Occupied, for an empty one.
int TContainerWindowBody::GetSlotAt(int x, int y, bool Occupied) const
{
	// x and y become positions in the slot grid in their own variables: the
	// original's catch wrapper printed them (issue #34).
	x -= SkinProperties().ContainerFirstSlot.x;
	y += ScrollBar->Range->Value - SkinProperties().ContainerFirstSlot.y;
	const int Column = x / SkinProperties().ContainerColumnWidth;
	if (Column < SkinProperties().ContainerSlotsPerRow && x % SkinProperties().ContainerColumnWidth <= SkinProperties().ContainerSlotSizeAndObject.x
		&& y % SkinProperties().ContainerRowHeight <= SkinProperties().ContainerSlotSizeAndObject.y) {
		const int Position = SkinProperties().ContainerSlotsPerRow * (y / SkinProperties().ContainerRowHeight) + Column;
		if (Occupied && GetContainerObjectCount(Container) <= Position)
			return -1;
		return Position;
	}
	return -1;
}

// Also called by TSubWindow::Arrange for the windows of containers.
void TContainerWindowBody::UpdateUpButton()
{
	TSubWindow *Window = static_cast<TSubWindow *>(GetParent());
	if (HasContainerParent(Container)) {
		HasParent = true;
		if (Window != nullptr)
			Window->ShowUpButton();
	} else {
		HasParent = false;
		if (Window != nullptr)
			Window->HideUpButton();
	}
}

// --- TSkillsWindowBody ------------------------------------------------------------

// The scroll bar moves by half a line.
TSkillsWindowBody::TSkillsWindowBody()
{
	ContextMenu = nullptr;
	ScrollBar = new TScrollBar(TRect(0, 0, SkinProperties().ScrollBarMinimumWidth, SkinProperties().ScrollBarMinimumHeight), 0, 0, 0, 0,
		GetFontLineHeight(SkinProperties().SkillsFont.FontNumber) / 2);
	InsertChild(ScrollBar);
	Lines = 15;
	Arrange();
}

// An open skill menu forgets the window.
TSkillsWindowBody::~TSkillsWindowBody()
{
	if (ContextMenu != nullptr)
		ContextMenu->SkillsWindow = nullptr;
}

// Not in the original (issue #118; ContextMenus.h): TSkillContextMenu's stores into the window and its call of the
// window's Arrange, here where the class is complete.
void SetSkillsWindowMenu(TSkillsWindowBody *Window, TSkillContextMenu *Menu) noexcept
{
	Window->ContextMenu = Menu;
}

void ArrangeSkillsWindow(TSkillsWindowBody *Window)
{
	Window->Arrange();
}

// Not in the original (issue #118; DialogWindow.h): TSubWindow::Arrange's call for a container's window.
void UpdateContainerUpButton(TSubWindowBody *Body)
{
	(static_cast<TContainerWindowBody *>(Body))->UpdateUpButton();
}

// One line per skill: the name on the left, the value on the right; a bar of the progress below
// it when the skill's bar is on (SkillsFirstBarsColor for the first seven lines, then
// SkillsOtherBarsColor); line 8 is a separator. Odd, kept: a line with a bar is 10 pixels taller
// here, but 9 in MouseHover and GetContentHeight.
void TSkillsWindowBody::Draw(int Surface, int x, int y)
{
	if (!IsMinimized()) {
		const int ScrollY = ScrollBar->Range->Value;
		const int Bottom = Height - 1;
		PATELLA_ASSERT_VALUES(x>=0, x);
		PATELLA_ASSERT_VALUES(y>=0, y);
		DrawSkin(Surface, x, y, Width - ScrollBar->Width, Height, SKIN_BACKGROUND, 0, ScrollY % Skins()[SKIN_BACKGROUND].height);
		if (!Minimized)
			TWindow::Draw(Surface, x, y);
		const int LineHeight = GetFontLineHeight(SkinProperties().SkillsFont.FontNumber);
		int LineY = -ScrollY;
		const int TextWidth = Width - SkinProperties().SkillsHorizontalMargin * 2 - ScrollBar->Width;
		int Progress = -1;
		const int Margin = SkinProperties().SkillsHorizontalMargin;
		for (int i = 0; i < Lines; i++) {
			TFixedString<100> Text;
			int Value;
			const bool HasValue = GetLine(i + 1, Text, Value, Progress);
			int ItemHeight = LineHeight;
			if (Progress != -1)
				ItemHeight = LineHeight + 10;
			int Top = LineY + SkinProperties().SkillsVerticalMargin;
			int ClipHeight = ItemHeight;
			if (Top + ItemHeight > 0) {
				if (Top > Bottom)
					return;
				if (Top < 0) {
					ClipHeight = Top + ItemHeight;
					Top = 0;
				}
				if (ClipHeight + Top > Bottom)
					ClipHeight = Bottom - Top + 1;
				const int ClipX = SkinProperties().SkillsHorizontalMargin + x;
				const int ClipY = Top + y;
				const int LineMiddle = GetFontHeight(SkinProperties().SkillsFont.FontNumber) / 2 + SkinProperties().SkillsVerticalMargin - 1 + LineY;
				if (!HasValue) {
					if (LineMiddle >= 0 && LineMiddle <= Bottom)
						GetSurface()->DrawHorizontalLine(Surface, ClipX, LineMiddle + y, TextWidth,
							SkinProperties().SkillsFont.Color);
				} else {
					TFixedString<20> Number;
					FormatNumber(Value, Number.Chars);
					PrintTextClipped(Surface, SkinProperties().SkillsHorizontalMargin + x, SkinProperties().SkillsVerticalMargin + LineY + y,
						SkinProperties().SkillsFont.FontNumber, SkinProperties().SkillsFont.Color, Text, 0,
						TRect(ClipX, ClipY, TextWidth, ClipHeight));
					PrintTextClipped(Surface, SkinProperties().SkillsHorizontalMargin + TextWidth - 1 + x, SkinProperties().SkillsVerticalMargin + LineY + y,
						SkinProperties().SkillsFont.FontNumber, SkinProperties().SkillsFont.Color, Number, 2,
						TRect(ClipX, ClipY, TextWidth, ClipHeight));
					if (Progress != -1) {
						const int BarHeight = 5;
						GetSurface()->DrawFrameClipped(Surface,
							TRect(Margin + x, LineHeight + 8 + LineY + y, TextWidth,
								BarHeight),
							1, TRGBColor(0), TRect(ClipX, ClipY, TextWidth, ClipHeight));
						GetSurface()->FillRectangleClipped(Surface,
							TRect(Margin + x + 1, LineHeight + 8 + LineY + y + 1,
								(TextWidth - 2) * Progress / 100, BarHeight - 2),
							i < 7 ? SkinProperties().SkillsFirstBarsColor
								  : SkinProperties().SkillsOtherBarsColor,
							TRect(ClipX, ClipY, TextWidth, ClipHeight));
					}
				}
			}
			LineY += ItemHeight;
		}
	}
}

bool TSkillsWindowBody::MouseWheel(int x, int y, int Code)
{
	if (!TWindow::MouseWheel(x, y, Code) && (Code == 1 || Code == 2))
		return ScrollBar->MouseWheel(x, y, Code);
	return true;
}

bool TSkillsWindowBody::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	SetHelpHandle(HELP_SKILLS);
	const TComponent *Parent = GetParent();
	PATELLA_ASSERT(Parent!=nullptr);
	SetHelpSize(Parent->Width, Parent->Height);
	MoveHelpPosition(-OffsetX, -OffsetY);
	return true;
}

// Over a line with a bar: the percent still to go.
void TSkillsWindowBody::MouseHover(int x, int y)
{
	patella::TGUI &GUI = patella::bridge::GUI();
	if (!IsMinimized()) {
		const int ScrollY = ScrollBar->Range->Value;
		const int Bottom = Height - 1;
		const int LineHeight = GetFontLineHeight(SkinProperties().SkillsFont.FontNumber);
		const int TextWidth = Width - SkinProperties().SkillsHorizontalMargin * 2 - ScrollBar->Width;
		int LineY = -ScrollY;
		int Progress = -1;
		for (int i = 0; i < Lines;) {
			i++;
			TFixedString<100> Text;
			int Value;
			GetLine(i, Text, Value, Progress);
			int ItemHeight = LineHeight;
			if (Progress != -1)
				ItemHeight = LineHeight + 9;
			int Top = SkinProperties().SkillsVerticalMargin + LineY;
			if (Top + ItemHeight > 0) {
				if (Top > Bottom)
					return;
				int ClipHeight = ItemHeight;
				if (Top < 0) {
					Top = 0;
					ClipHeight = SkinProperties().SkillsVerticalMargin + LineY + ItemHeight;
				}
				if (ClipHeight + Top > Bottom)
					ClipHeight = Bottom - Top + 1;
				if (Top <= y && y <= ClipHeight + Top && SkinProperties().SkillsHorizontalMargin <= x
					&& x <= SkinProperties().SkillsHorizontalMargin + TextWidth && Progress != -1) {
					MsvcSnprintf(GUI.SkillMouseOverText, 256, TextPercentToGo, 100 - Progress);
					SetMouseOverText(GUI.SkillMouseOverText);
				}
			}
			LineY += ItemHeight;
		}
	}
}

// Opens the skill menu (Control.cpp's message 60).
bool TSkillsWindowBody::RightButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	AddMessage(MESSAGE_SKILL_MENU, reinterpret_cast<std::intptr_t>(this), 0);
	return true;
}

// Not higher than the content.
void TSkillsWindowBody::SetSize(int Width, int Height)
{
	const int ScrollbarX = Width - ScrollBar->Width;
	PATELLA_ASSERT_VALUES(ScrollbarX>0, ScrollbarX);
	const int Maximum = GetMaximumHeight();
	if (Height >= Maximum)
		Height = Maximum;
	TWindow::SetSize(Width, Height);
	ScrollBar->SetPosition(ScrollbarX, 0);
	ScrollBar->SetSize(ScrollBar->Width, Height);
	Arrange();
}

// The visible part leaves the margin (SkillsVerticalMargin) at the top and the bottom.
void TSkillsWindowBody::Arrange()
{
	int ContentHeight = GetContentHeight();
	const int Visible = Height - SkinProperties().SkillsVerticalMargin * 2;
	if (ContentHeight <= Visible)
		ContentHeight = Visible;
	ScrollBar->Range->SetRange(0, ContentHeight - Visible, ScrollBar->Range->Value, Visible);
}

// At least one line.
int TSkillsWindowBody::getMinimumHeight()
{
	// The original's order: a call's arguments right to left (MSVC; GCC and Clang may differ, issue #91).
	const int LineHeight = GetFontLineHeight(SkinProperties().SkillsFont.FontNumber);
	return (std::max)(ScrollBar->getMinimumHeight(), LineHeight);
}

// The stored height ("ContentWindow" 6 in Tibia.cfg) or the full height,
// at least the minimum.
int TSkillsWindowBody::GetInitialHeight()
{
	int Height = GetContentWindowSize(SKILLS_WINDOW);
	if (Height == 0)
		Height = GetMaximumHeight();
	if (Height < getMinimumHeight())
		return getMinimumHeight();
	return Height;
}

int TSkillsWindowBody::GetMaximumHeight()
{
	return GetContentHeight() + SkinProperties().SkillsVerticalMargin * 2;
}

int TSkillsWindowBody::GetContentHeight()
{
	const int LineHeight = GetFontLineHeight(SkinProperties().SkillsFont.FontNumber);
	int Result = 0;
	for (int i = 0; i < Lines;) {
		i++;
		TFixedString<100> Text;
		int Value, Progress;
		GetLine(i, Text, Value, Progress);
		if (Progress != -1)
			Result += 9;
		Result += LineHeight;
	}
	return Result;
}

// The lines are 1-15: experience, level, hit points, mana, soul points,
// capacity, magic level, a separator (false, without the checks below) and
// the seven skills. Progress is -1 without a bar.
bool TSkillsWindowBody::GetLine(int NumberOfLine, char *Text, int &Value, int &Progress) const
{
	PATELLA_ASSERT_VALUES(In(NumberOfLine,1,Lines), NumberOfLine, Lines);
	Value = 0;
	Progress = -1;
	TStringBuffer Name(Text, 100); // every caller passes char[100]
	switch (NumberOfLine) {
	case 1:
		Name.Assign("Experience");
		Value = GetPlayerExperience();
		break;
	case 2:
		Name.Assign("Level");
		Value = GetPlayerLevel();
		if (GetLevelBar())
			Progress = GetPlayerLevelProgress();
		else
			Progress = -1;
		break;
	case 3:
		Name.Assign("Hit Points");
		Value = GetPlayerHealth();
		break;
	case 4:
		Name.Assign("Mana");
		Value = GetPlayerMana();
		break;
	case 5:
		Name.Assign("Soul Points");
		Value = GetPlayerSoul();
		break;
	case 6:
		Name.Assign("Capacity");
		Value = GetPlayerCapacity();
		break;
	case 7:
		Name.Assign("Magic Level");
		Value = GetPlayerMagicLevel();
		if (GetMagLevelBar())
			Progress = GetPlayerMagicLevelProgress();
		else
			Progress = -1;
		break;
	case 8:
		return false;
	case 9:
	case 10:
	case 11:
	case 12:
	case 13:
	case 14:
	case 15: {
		const int SkillNumber = NumberOfLine - 9;
		Name.Assign(GetSkillName(SkillNumber));
		Value = GetPlayerSkillValue(SkillNumber);
		if (GetSkillBar(SkillNumber))
			Progress = GetPlayerSkillProgress(SkillNumber);
		else
			Progress = -1;
		break;
	}
	default:
		break;
	}
	PATELLA_ASSERT_VALUES(Value>=0, Value, NumberOfLine);
	PATELLA_ASSERT_VALUES(In(Progress,-1,100), Progress, NumberOfLine);
	return true;
}

// --- TBattleWindowBody ------------------------------------------------------------

// The scroll bar moves by a third of a row.
TBattleWindowBody::TBattleWindowBody()
{
	ScrollBar = new TScrollBar(TRect(0, 0, SkinProperties().ScrollBarMinimumWidth, SkinProperties().ScrollBarMinimumHeight), 0, 0, 0, 0,
		SkinProperties().BattleWindow.RowHeight / 3);
	InsertChild(ScrollBar);
	MouseOverText[0] = 0;
	NumberOfOpponents = -1;
	Arrange();
}

// The inline destructor's work until issue #108: no creature is
// highlighted any more; then the children.
void TBattleWindowBody::OnClose()
{
	try {
		SetHighlightID(0);
	} catch (...) {
		CloseWhileUnwinding([this] { TSubWindowBody::OnClose(); });
		throw;
	}
	TSubWindowBody::OnClose();
}

// One row per opponent: a frame in the colour of the mouse (the opponent
// under it becomes the highlighted creature), the attack or the follow
// target, the outfit, the party shield (248-251; 248 only while the player
// is in no party) and the skull (240-243, blinking for four seconds after
// SkullTime), the name (shortened to the width) and the health bar. The
// children are drawn before the background.
void TBattleWindowBody::Draw(int Surface, int x, int y)
{
	SetHighlightID(0);
	const int Opponents = GetNumberOfOpponents();
	if (NumberOfOpponents != Opponents) {
		NumberOfOpponents = Opponents;
		Arrange();
	}
	if (!IsMinimized()) {
		if (!Minimized)
			TWindow::Draw(Surface, x, y);
		const int ScrollY = ScrollBar->Range->Value;
		const int Bottom = Height - 1;
		PATELLA_ASSERT_VALUES(x>=0, x);
		PATELLA_ASSERT_VALUES(y>=0, y);
		DrawSkin(Surface, x, y, Width - ScrollBar->Width, Height, SKIN_BACKGROUND, 0, ScrollY % Skins()[SKIN_BACKGROUND].height);
		const int ClipWidth = Width - ScrollBar->Width;
		for (int i = 0; i < Opponents; i++) {
			TCreature *Creature = GetOpponent(i);
			const int RowY = SkinProperties().BattleWindow.RowHeight * i - ScrollY;
			int Offset = 0;
			if (SkinProperties().BattleWindow.RowHeight + RowY <= 0)
				continue;
			if (RowY > Bottom)
				return;
			int ClipY = RowY;
			int ClipHeight = SkinProperties().BattleWindow.RowHeight;
			if (RowY < 0) {
				ClipY = 0;
				ClipHeight = SkinProperties().BattleWindow.RowHeight + RowY;
			}
			if (ClipHeight + ClipY > Bottom)
				ClipHeight = Bottom - ClipY + 1;
			ClipY += y;
			bool Framed = false;
			TRGBColor Color = SkinProperties().BattleWindow.NameFont.Color;
			// GetOpponentAt's arguments in the original's order, right to left (MSVC; GCC and Clang may differ, issue
			// #91).
			const auto OpponentAtMouse = [&] {
				const int MouseY = GetMouseY();
				return GetOpponentAt(GetMouseX() - x, MouseY - y);
			};
			if (In(GetMouseY(), ClipY, ClipY + ClipHeight) && In(GetMouseX(), x, ClipWidth + x) && i == OpponentAtMouse()) {
				SetHighlightID(Creature->CreatureID);
				Color = SkinProperties().HighlightColor;
				Framed = true;
			}
			if (Creature->CreatureID == GetAttackID()) {
				if (Framed)
					Color = SkinProperties().HighlightedAttackColor;
				else
					Color = SkinProperties().AttackColor;
				Framed = true;
			} else if (Creature->CreatureID == GetFollowID()) {
				if (Framed)
					Color = SkinProperties().HighlightedFollowColor;
				else
					Color = SkinProperties().FollowColor;
				Framed = true;
			}
			if (Framed)
				GetSurface()->DrawFrameClipped(Surface,
					TRect(SkinProperties().BattleWindow.Frame.x + x,
						SkinProperties().BattleWindow.Frame.y + RowY + y, SkinProperties().BattleWindow.Frame.width,
						SkinProperties().BattleWindow.Frame.height),
					1, Color, TRect(x, ClipY, ClipWidth, ClipHeight));
			GetSurface()->Flag26 = true;
			DrawOutfit(Surface, SkinProperties().BattleWindow.Outfit.x + x, SkinProperties().BattleWindow.Outfit.y + RowY + y,
				SkinProperties().BattleWindow.Outfit.width, Creature->OutfitType, Creature->Colors,
				SkinProperties().BattleWindow.OutfitHighlight, TRect(x, ClipY, ClipWidth, ClipHeight), SkinProperties().ObjectCountFont);
			GetSurface()->Flag26 = false;
			if (Creature->PARTYFlag > PARTY_NONE) {
				int Skin = SKIN_NONE; // none: no shield in the party (flag 1) or an unknown flag
				switch (Creature->PARTYFlag) {
				case PARTY_INVITER:
					if (!IsPlayerInParty())
						Skin = SKIN_BATTLE_SHIELD_1;
					break;
				case PARTY_INVITEE:
					Skin = SKIN_BATTLE_SHIELD_2;
					break;
				case PARTY_MEMBER:
					Skin = SKIN_BATTLE_SHIELD_3;
					break;
				case PARTY_LEADER:
					Skin = SKIN_BATTLE_SHIELD_4;
					break;
				default:
					break;
				}
				if (Skin != SKIN_NONE) {
					DrawSkinClipped(Surface, SkinProperties().BattleWindow.Name.x + x, SkinProperties().BattleWindow.Name.y + RowY + y,
						Skins()[Skin].width, Skins()[Skin].height, Skin, x, ClipY, ClipWidth, ClipHeight, 0, 0);
					Offset = SkinProperties().BattleWindow.FlagOffset;
				}
			}
			if (Creature->PKFlag > PK_NONE) {
				int Skin = SKIN_NONE; // none: an unknown flag
				switch (Creature->PKFlag) {
				case PK_YELLOW:
					Skin = SKIN_BATTLE_SKULL_1;
					break;
				case PK_GREEN:
					Skin = SKIN_BATTLE_SKULL_2;
					break;
				case PK_WHITE:
					Skin = SKIN_BATTLE_SKULL_3;
					break;
				case PK_RED:
					Skin = SKIN_BATTLE_SKULL_4;
					break;
				default:
					break;
				}
				if (Skin != SKIN_NONE) {
					if (Creature->SkullTime == 0 || !Creature->SkullKept(patella::platform::Platform().Clock.WallSeconds()) || GetBlinkState())
						DrawSkinClipped(Surface, SkinProperties().BattleWindow.Name.x + Offset + x,
							SkinProperties().BattleWindow.Name.y + RowY + y, Skins()[Skin].width, Skins()[Skin].height, Skin, x,
							ClipY, ClipWidth, ClipHeight, 0, 0);
					Offset += SkinProperties().BattleWindow.FlagOffset;
				}
			}
			const char *Name = Creature->Name;
			const int MaxLength = GetFittingTextLength(SkinProperties().BattleWindow.NameFont.FontNumber, Name,
				SkinProperties().BattleWindow.Name.width);
			TFixedString<32> Shortened;
			if (MaxLength < (int)strlen(Name)) {
				Shortened.Assign(Name);
				ShortenString(Shortened, MaxLength);
				Name = Shortened;
			}
			if (Framed)
				PrintTextClipped(Surface, SkinProperties().BattleWindow.Name.x + Offset + x,
					SkinProperties().BattleWindow.Name.y + RowY + y, SkinProperties().BattleWindow.NameFont.FontNumber, Color, Name,
					SkinProperties().BattleWindow.NameFont.Alignment, TRect(x, ClipY, ClipWidth, ClipHeight));
			else
				PrintFormattedTextClipped(Surface, SkinProperties().BattleWindow.Name.x + Offset + x,
					SkinProperties().BattleWindow.Name.y + RowY + y, SkinProperties().BattleWindow.NameFont, Name,
					TRect(x, ClipY, ClipWidth, ClipHeight));
			GetSurface()->DrawFrameClipped(Surface,
				TRect(SkinProperties().BattleWindow.HealthBar.x + x,
					SkinProperties().BattleWindow.HealthBar.y + RowY + y, SkinProperties().BattleWindow.HealthBar.width,
					SkinProperties().BattleWindow.HealthBar.height),
				1, SkinProperties().BattleWindow.HealthBarFrameColor,
				TRect(x, ClipY, ClipWidth, ClipHeight));
			GetSurface()->FillRectangleClipped(Surface,
				TRect(SkinProperties().BattleWindow.HealthBar.x + x + 1,
					SkinProperties().BattleWindow.HealthBar.y + RowY + y + 1,
					Creature->Health * (SkinProperties().BattleWindow.HealthBar.width - 2) / 100,
					SkinProperties().BattleWindow.HealthBar.height - 2),
				Creature->GetHealthColor(),
				TRect(x, ClipY, ClipWidth, ClipHeight));
		}
	}
}

void TBattleWindowBody::SetSize(int Width, int Height)
{
	const int ScrollbarX = Width - ScrollBar->Width;
	PATELLA_ASSERT_VALUES(ScrollbarX>0, ScrollbarX);
	TWindow::SetSize(Width, Height);
	ScrollBar->SetPosition(ScrollbarX, 0);
	ScrollBar->SetSize(ScrollBar->Width, Height);
	Arrange();
}

void TBattleWindowBody::Arrange()
{
	int ContentHeight = GetContentHeight();
	if (ContentHeight <= Height)
		ContentHeight = Height;
	ScrollBar->Range->SetRange(0, ContentHeight - Height, ScrollBar->Range->Value, Height);
}

// A press is taken even where no child takes it.
bool TBattleWindowBody::LeftButtonDown(int x, int y, int Flags)
{
	TWindow::LeftButtonDown(x, y, Flags);
	return true;
}

// Action state 7 uses the use object on a monster (not on players), 8
// offers a trade to a player; a plain click is a right click with 0x800
// (attack, RightButtonUp).
bool TBattleWindowBody::LeftButtonUp(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonUp(x, y, Flags)) {
		const int Position = GetOpponentAt(x, y);
		if (Position != -1) {
			const TCreature *Creature = GetOpponent(Position);
			if (Creature != nullptr) {
				if (GetActionState() == ACTIONSTATE_USE_WITH) {
					int SrcIndex, SrcX, SrcY, SrcZ;
					const TObject &SrcObj = *GetUseObject(&SrcIndex, &SrcX, &SrcY, &SrcZ);
					PATELLA_ASSERT(SrcObj!=0);
					PATELLA_ASSERT(SrcObj.Type!=CREATURE);
					if (!IsPlayer(Creature->CreatureID)) {
						SendUseObjectOnCreature(SrcX, SrcY, SrcZ, SrcObj.Type, SrcIndex, Creature->CreatureID);
						return true;
					}
					SetStatusMessage("You are not allowed to shoot directly on players.", false);
				} else if (GetActionState() == ACTIONSTATE_TRADE_WITH) {
					int TradeIndex, TradeX, TradeY, TradeZ;
					const TObject &Obj = *GetTradeObject(&TradeIndex, &TradeX, &TradeY, &TradeZ);
					PATELLA_ASSERT(Obj!=0);
					if (IsPlayer(Creature->CreatureID)) {
						SendRequestTrade(TradeX, TradeY, TradeZ, Obj.Type, TradeIndex, Creature->CreatureID);
						return true;
					}
					SetStatusMessage("Select a player to trade with.", false);
				} else if (GetActionState() != ACTIONSTATE_DRAGGING) {
					if (Flags != 0)
						RightButtonUp(x, y, Flags);
					else
						RightButtonUp(x, y, KEY_ALT);
				}
			}
		}
	}
	return true;
}

// 0x800 (a left click) attacks the opponent or stops attacking it, Shift
// looks at it, otherwise its menu opens (message 53).
bool TBattleWindowBody::RightButtonUp(int x, int y, int Flags)
{
	if (!TWindow::RightButtonUp(x, y, Flags)) {
		const int Position = GetOpponentAt(x, y);
		if (Position != -1) {
			const TCreature *Creature = GetOpponent(Position);
			if (Creature != nullptr) {
				if ((Flags & KEY_ALT) != 0) {
					const uint32 AttackID = GetAttackID();
					SetAttackID(Creature->CreatureID == AttackID ? 0 : Creature->CreatureID);
					SendAttack(GetAttackID());
				} else if ((Flags & SHIFT_OR_BOTH_BUTTONS) != 0) {
					SendLook(Creature->RealX, Creature->RealY, Creature->RealZ, CREATURE, 0);
				} else {
					AddMessage(MESSAGE_CREATURE_MENU, Creature->CreatureID, 0);
				}
			}
		}
	}
	return true;
}

// A shortened name in full.
void TBattleWindowBody::MouseHover(int x, int y)
{
	const int Position = GetOpponentAt(x, y);
	if (Position != -1) {
		TCreature *Creature = GetOpponent(Position);
		if (Creature != nullptr) {
			const int MaxLength = GetFittingTextLength(SkinProperties().BattleWindow.NameFont.FontNumber, Creature->Name,
				SkinProperties().BattleWindow.Name.width);
			if (MaxLength < (int)strlen(Creature->Name)) {
				MouseOverText.Assign(Creature->Name);
				SetMouseOverText(MouseOverText);
			}
		}
	}
}

bool TBattleWindowBody::MouseWheel(int x, int y, int Code)
{
	if (!TWindow::MouseWheel(x, y, Code) && (Code == 1 || Code == 2))
		return ScrollBar->MouseWheel(x, y, Code);
	return true;
}

bool TBattleWindowBody::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	SetHelpHandle(HELP_BATTLE);
	const TComponent *Parent = GetParent();
	PATELLA_ASSERT(Parent!=nullptr);
	SetHelpSize(Parent->Width, Parent->Height);
	MoveHelpPosition(-OffsetX, -OffsetY);
	return true;
}

// At least one row.
int TBattleWindowBody::getMinimumHeight()
{
	return (std::max)(ScrollBar->getMinimumHeight(), SkinProperties().BattleWindow.RowHeight);
}

// The stored height ("ContentWindow" 7) or the content.
int TBattleWindowBody::GetInitialHeight()
{
	int Height = GetContentWindowSize(BATTLE_WINDOW);
	if (Height == 0)
		Height = GetContentHeight();
	if (Height < getMinimumHeight())
		Height = getMinimumHeight();
	return Height;
}

int TBattleWindowBody::GetContentHeight()
{
	return GetNumberOfOpponents() * SkinProperties().BattleWindow.RowHeight + 1;
}

// The row under x, y; -1 for none.
int TBattleWindowBody::GetOpponentAt(int x, int y)
{
	y += ScrollBar->Range->Value;
	const int Position = y / SkinProperties().BattleWindow.RowHeight;
	if (y >= 0 && x >= 0 && x <= Width - ScrollBar->Width && Position < GetNumberOfOpponents())
		return Position;
	return -1;
}

// --- TTradeOffers -----------------------------------------------------------------

TTradeOffers::TTradeOffers()
{
	ScrollBar = new TScrollBar(TRect(0, 0, SkinProperties().ScrollBarMinimumWidth, SkinProperties().ScrollBarMinimumHeight), 0, 0, 0, 0, 10);
	InsertChild(ScrollBar);
	Separator = new TSeparator(false);
	InsertChild(Separator);
}

// Both offers side by side (TradeWindow.OwnOffer the own, CounterOffer the
// partner's) in rows of SlotsPerRow slots (skin 125), scrolled together.
void TTradeOffers::Draw(int Surface, int x, int y)
{
	const int ScrollY = ScrollBar->Range->Value;
	const int Bottom = Height - 1;
	PATELLA_ASSERT_VALUES(x>=0, x);
	PATELLA_ASSERT_VALUES(y>=0, y);
	DrawSkin(Surface, x, y, Width - ScrollBar->Width, Height, SKIN_BACKGROUND, 0, ScrollY % Skins()[SKIN_BACKGROUND].height);
	TWindow::Draw(Surface, x, y);
	const int ClipHeight = Height;
	const int ClipWidth = Width;
	for (int Side = 0; Side < 2; Side++) {
		const TRect &Area = Side == 0 ? SkinProperties().TradeWindow.OwnOffer : SkinProperties().TradeWindow.CounterOffer;
		const int AreaX = Area.x;
		const int AreaY = Area.y;
		const int AreaWidth = Area.width;
		const int AreaHeight = Area.height;
		// ObjectOffsets: the object's offset in a slot, x/y for the own offer,
		// Width/Height for the partner's.
		const int *ObjectOffset = Side == 0 ? &SkinProperties().TradeWindow.ObjectOffsets.x : &SkinProperties().TradeWindow.ObjectOffsets.width;
		const int OffsetX = ObjectOffset[0];
		const int OffsetY = ObjectOffset[1];
		const int Count = Side == 0 ? GetOwnOfferCount() : GetCounterOfferCount();
		for (int i = 0; i < Count; i++) {
			const TObject Obj = Side == 0 ? GetOwnOfferObject(i) : GetCounterOfferObject(i);
			const int SlotX = i % SkinProperties().TradeWindow.SlotsPerRow * SkinProperties().TradeWindow.ColumnWidth;
			const int RowY = i / SkinProperties().TradeWindow.SlotsPerRow * SkinProperties().TradeWindow.RowHeight - ScrollY;
			if (AreaHeight + AreaY + RowY > 0 && AreaY + RowY <= Bottom) {
				const int DestX = SlotX + AreaX;
				int DestY = AreaY + RowY;
				int SlotHeight = AreaHeight;
				if (DestY < 0) {
					SlotHeight = DestY + AreaHeight;
					DestY = 0;
				}
				if (SlotHeight + DestY > Bottom)
					SlotHeight = Bottom - DestY + 1;
				PATELLA_ASSERT_VALUES(x+DestX>=0, x+DestX);
				PATELLA_ASSERT_VALUES(y+DestY>=0, y+DestY);
				DrawSkin(Surface, x + DestX, y + DestY, AreaWidth, SlotHeight, SKIN_CONTAINER_SLOT, 0, 0);
				DrawObject(Surface, OffsetX + SlotX + x, OffsetY + RowY + y, SkinProperties().TradeWindow.ObjectSize, Obj,
					TRGBColor(0, 0, 0), TRect(x, y, ClipWidth, ClipHeight), SkinProperties().ObjectCountFont, false);
			}
		}
	}
}

void TTradeOffers::SetSize(int Width, int Height)
{
	const int ScrollBarWidth = ScrollBar->Width;
	TWindow::SetSize(Width, Height);
	ScrollBar->SetPosition(Width - ScrollBarWidth, 0);
	ScrollBar->SetSize(ScrollBar->Width, Height);
	SetChildArea(Separator, SkinProperties().TradeWindow.OffersSeparator);
	Arrange();
}

int TTradeOffers::getMinimumHeight()
{
	return ScrollBar->getMinimumHeight();
}

// The rows of the longer offer. Odd, kept: the top margin added is
// OwnOffer.x, the left edge of the own offer.
int TTradeOffers::GetContentHeight()
{
	const int CounterCount = GetCounterOfferCount();
	const int OwnCount = GetOwnOfferCount();
	const int Count = (std::max)(OwnCount, CounterCount);
	const int Result = SkinProperties().TradeWindow.RowHeight
			* (Count / SkinProperties().TradeWindow.SlotsPerRow + (Count % SkinProperties().TradeWindow.SlotsPerRow > 0))
		+ 1 + SkinProperties().TradeWindow.OwnOffer.x;
	return (std::max)(getMinimumHeight(), Result);
}

void TTradeOffers::Arrange()
{
	int ContentHeight = GetContentHeight();
	if (ContentHeight <= Height)
		ContentHeight = Height;
	ScrollBar->Range->SetRange(0, ContentHeight - Height, ScrollBar->Range->Value, Height);
}

// A click on an object of either offer inspects it.
bool TTradeOffers::LeftButtonUp(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonUp(x, y, Flags) && GetActionState() != ACTIONSTATE_DRAGGING && GetActionState() != ACTIONSTATE_USE_WITH
		&& GetActionState() != ACTIONSTATE_TRADE_WITH) {
		int Side, Position;
		GetOfferAt(x, y, &Side, &Position);
		if (Side != -1)
			SendInspectTrade(Side, Position);
	}
	return true;
}

bool TTradeOffers::LeftButtonDown(int x, int y, int Flags)
{
	TWindow::LeftButtonDown(x, y, Flags);
	return true;
}

bool TTradeOffers::RightButtonUp(int x, int y, int Flags)
{
	if (!TWindow::RightButtonUp(x, y, Flags) && GetActionState() != ACTIONSTATE_DRAGGING && GetActionState() != ACTIONSTATE_USE_WITH
		&& GetActionState() != ACTIONSTATE_TRADE_WITH) {
		int Side, Position;
		GetOfferAt(x, y, &Side, &Position);
		if (Side != -1)
			SendInspectTrade(Side, Position);
	}
	return true;
}

bool TTradeOffers::MouseWheel(int x, int y, int Code)
{
	if (!TWindow::MouseWheel(x, y, Code) && (Code == 1 || Code == 2))
		return ScrollBar->MouseWheel(x, y, Code);
	return true;
}

// The offer (0 own, 1 the partner's) and the position under x, y; -1, -1
// for none.
void TTradeOffers::GetOfferAt(int x, int y, int *Side, int *Position) const
{
	*Position = -1;
	*Side = -1;
	for (int i = 0; i <= 1; i++) {
		const TRect &Area = i == 0 ? SkinProperties().TradeWindow.OwnOffer : SkinProperties().TradeWindow.CounterOffer;
		const int AreaWidth = Area.width;
		const int AreaX = Area.x;
		const int AreaY = Area.y;
		const int AreaHeight = Area.height;
		const int Count = i == 0 ? GetOwnOfferCount() : GetCounterOfferCount();
		const int RelativeY = ScrollBar->Range->Value - AreaY + y;
		const int Column = (x - AreaX) / SkinProperties().TradeWindow.ColumnWidth;
		if (Column < SkinProperties().TradeWindow.SlotsPerRow && (x - AreaX) % SkinProperties().TradeWindow.ColumnWidth <= AreaWidth
			&& RelativeY % SkinProperties().TradeWindow.RowHeight <= AreaHeight) {
			const int Found = RelativeY / SkinProperties().TradeWindow.RowHeight * SkinProperties().TradeWindow.SlotsPerRow + Column;
			if (Found < Count) {
				*Position = Found;
				*Side = i;
				return;
			}
		}
	}
}

// --- TTradeNames ------------------------------------------------------------------

TTradeNames::TTradeNames()
{
	Separator = new TSeparator(true);
	InsertChild(Separator);
	const char *Name = GetOwnOfferCount() < 1 ? "" : GetOwnOfferName();
	OwnName = new TLabel(SkinProperties().TradeWindow.OwnName.x, SkinProperties().TradeWindow.OwnName.y,
		SkinProperties().TradeWindow.OwnName.width, SkinProperties().TradeWindow.OwnName.height, SkinProperties().TradeWindow.NameFont, Name,
		true);
	InsertChild(OwnName);
	Name = GetCounterOfferCount() < 1 ? "" : GetCounterOfferName();
	CounterName = new TLabel(SkinProperties().TradeWindow.CounterName.x, SkinProperties().TradeWindow.CounterName.y,
		SkinProperties().TradeWindow.CounterName.width, SkinProperties().TradeWindow.CounterName.height, SkinProperties().TradeWindow.NameFont, Name,
		true);
	InsertChild(CounterName);
}

void TTradeNames::Draw(int Surface, int x, int y)
{
	PATELLA_ASSERT_VALUES(x>=0, x);
	PATELLA_ASSERT_VALUES(y>=0, y);
	DrawSkin(Surface, x, y, Width, Height, SKIN_BACKGROUND, 0, 0);
	TWindow::Draw(Surface, x, y);
}

void TTradeNames::SetSize(int Width, int Height)
{
	TWindow::SetSize(Width, Height);
	SetChildArea(OwnName, SkinProperties().TradeWindow.OwnName);
	SetChildArea(CounterName, SkinProperties().TradeWindow.CounterName);
	SetChildArea(Separator, SkinProperties().TradeWindow.NamesSeparator);
}

// The names once the offers came.
void TTradeNames::Arrange()
{
	if (GetOwnOfferCount() > 0)
		OwnName->SetText(GetOwnOfferName(), true);
	if (GetCounterOfferCount() > 0)
		CounterName->SetText(GetCounterOfferName(), true);
}

// --- TTradeButtons ----------------------------------------------------------------

// Until the partner offers, a text asks to wait.
TTradeButtons::TTradeButtons()
	: Callback(nullptr, 0)
{
	Callback = NoCallback;
	CancelButton = nullptr;
	WaitLabel = nullptr;
	AcceptButton = nullptr;
	WaitLabel = new TLabel(SkinProperties().TradeWindow.WaitFont, TextWaitForCounterOffer);
	InsertChild(WaitLabel);
	Separator = new TSeparator(true);
	InsertChild(Separator);
}

void TTradeButtons::Draw(int Surface, int x, int y)
{
	PATELLA_ASSERT_VALUES(x>=0, x);
	PATELLA_ASSERT_VALUES(y>=0, y);
	DrawSkin(Surface, x, y, Width, Height, SKIN_BACKGROUND, 0, 0);
	TWindow::Draw(Surface, x, y);
}

void TTradeButtons::SetSize(int Width, int Height)
{
	TWindow::SetSize(Width, Height);
	SetChildArea(Separator, SkinProperties().TradeWindow.ButtonsSeparator);
	if (CancelButton != nullptr)
		SetChildPosition(CancelButton, SkinProperties().TradeWindow.AcceptAndCancelButtons.width, SkinProperties().TradeWindow.AcceptAndCancelButtons.height);
	if (WaitLabel != nullptr)
		SetChildArea(WaitLabel, SkinProperties().TradeWindow.WaitLabel);
	if (AcceptButton != nullptr)
		SetChildPosition(AcceptButton, SkinProperties().TradeWindow.AcceptAndCancelButtons.x, SkinProperties().TradeWindow.AcceptAndCancelButtons.y);
}

// "Cancel" before the partner offered, "Reject" after; both send
// Callback.
void TTradeButtons::SetCancelButton(bool Cancel)
{
	if (CancelButton != nullptr)
		DeleteChild(CancelButton);
	if (!Cancel)
		CancelButton = new TTextButton(0, 0, "Reject", Callback, TextRejectOffer);
	else
		CancelButton = new TTextButton(0, 0, "Cancel", Callback, TextCancelOffer);
	InsertChild(CancelButton);
	SetSize(Width, Height);
}

// The counter offer came: "Reject" and "Accept" (event 1 of
// EventComponent). Odd, kept: the wait text is deleted without a NULL
// check.
void TTradeButtons::ShowAcceptButton(TComponent *EventComponent)
{
	PATELLA_ASSERT(EventComponent!=nullptr);
	SetCancelButton(false);
	DeleteChild(WaitLabel);
	WaitLabel = nullptr;
	AcceptButton = new TTextButton(0, 0, "Accept", TCallback(EventComponent, 1), TextAcceptOffer);
	InsertChild(AcceptButton);
	SetSize(Width, Height);
	Arrange();
}

// Accepted: wait for the partner (again without a NULL check).
void TTradeButtons::Wait()
{
	DeleteChild(AcceptButton);
	AcceptButton = nullptr;
	WaitLabel = new TLabel(SkinProperties().TradeWindow.WaitFont, TextWaitForPartner);
	InsertChild(WaitLabel);
	SetSize(Width, Height);
	Arrange();
}

// --- TTradeWindowBody -------------------------------------------------------------

TTradeWindowBody::TTradeWindowBody()
{
	Buttons = new TTradeButtons;
	Offers = new TTradeOffers;
	Names = new TTradeNames;
	InsertChild(Buttons);
	InsertChild(Offers);
	InsertChild(Names);
	SetSize(170, 100);
}

// Its work is the close since issue #108.
TTradeWindowBody::~TTradeWindowBody()
{
	CheckClosed("TTradeWindowBody");
}

// The destructor's work until issue #108: closing the window rejects an
// open offer; then the children.
void TTradeWindowBody::OnClose()
{
	try {
		if (GetOwnOfferCount() > 0)
			SendRejectTrade();
		ClearOffers();
	} catch (...) {
		CloseWhileUnwinding([this] { TSubWindowBody::OnClose(); });
		throw;
	}
	TSubWindowBody::OnClose();
}

int TTradeWindowBody::GetContentHeight()
{
	return getMinimumHeight();
}

// The height is limited to the range between the minimum and the maximum
// height (both read twice, in either order). The calls go in the binary's
// order (maximum, minimum, for the lower bound, then again for the upper
// one) in their own variables: in one expression our compiler called them
// in another order (issue #34).
void TTradeWindowBody::SetSize(int Width, int Height)
{
	int Maximum = GetMaximumHeight();
	int Minimum = getMinimumHeight();
	const int Lower = (std::min)(Maximum, Minimum);
	Maximum = GetMaximumHeight();
	Minimum = getMinimumHeight();
	Limit(&Height, Lower, (std::max)(Maximum, Minimum));
	TWindow::SetSize(Width, Height);
	SetChildArea(Buttons, SkinProperties().TradeWindow.Buttons);
	SetChildArea(Offers, SkinProperties().TradeWindow.Offers);
	SetChildArea(Names, SkinProperties().TradeWindow.Names);
	Arrange();
}

bool TTradeWindowBody::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	SetHelpHandle(HELP_TRADE);
	const TComponent *Parent = GetParent();
	PATELLA_ASSERT(Parent!=nullptr);
	SetHelpSize(Parent->Width, Parent->Height);
	MoveHelpPosition(-OffsetX, -OffsetY);
	return true;
}

int TTradeWindowBody::getMinimumHeight()
{
	return (std::max)(Offers->getMinimumHeight(), SkinProperties().TradeWindow.MinimumHeight);
}

int TTradeWindowBody::GetInitialHeight()
{
	return GetMaximumHeight();
}

int TTradeWindowBody::GetMaximumHeight()
{
	const int NamesHeight = Names->Height;
	const int ButtonsHeight = Buttons->Height;
	return Offers->GetContentHeight() + ButtonsHeight + NamesHeight;
}

// Event 21: the partner's offer came (MainWindow.cpp's UpdateTrade); event
// 1: "Accept".
void TTradeWindowBody::HandleEvent(int Event)
{
	if (Event == EVENT_TRADE_COUNTER_OFFER) {
		Buttons->ShowAcceptButton(this);
		Arrange();
	} else if (Event == 1) {
		SendAcceptTrade();
		Buttons->Wait();
		Arrange();
	}
}

// --- TBuddylistWindowBody ---------------------------------------------------------

// The scroll bar moves by half a row.
TBuddylistWindowBody::TBuddylistWindowBody()
{
	ScrollBar = new TScrollBar(TRect(0, 0, SkinProperties().ScrollBarMinimumWidth, SkinProperties().ScrollBarMinimumHeight), 0, 0, 0, 0,
		SkinProperties().BuddylistWindow.RowHeight / 2);
	InsertChild(ScrollBar);
	MouseOverText = nullptr;
	NumberOfBuddies = -1;
	Arrange();
}

// One row per buddy: the icon (skins 218 and up), the name (shortened to the
// width) in the colour of its state (online, offline, new). Clipped to the
// whole window.
void TBuddylistWindowBody::Draw(int Surface, int x, int y)
{
	if (NumberOfBuddies != GetNumberOfBuddies()) {
		NumberOfBuddies = GetNumberOfBuddies();
		Arrange();
	}
	if (!IsMinimized()) {
		const int ScrollY = ScrollBar->Range->Value;
		const int Bottom = Height - 1;
		PATELLA_ASSERT_VALUES(x>=0, x);
		PATELLA_ASSERT_VALUES(y>=0, y);
		DrawSkin(Surface, x, y, Width - ScrollBar->Width, Height, SKIN_BACKGROUND, 0, ScrollY % Skins()[SKIN_BACKGROUND].height);
		if (!Minimized)
			TWindow::Draw(Surface, x, y);
		const int ClipHeight = Height;
		const int ClipWidth = Width - ScrollBar->Width;
		for (int i = 0; i < GetNumberOfBuddies(); i++) {
			const int ItemX = 0;
			const int ItemY = SkinProperties().BuddylistWindow.RowHeight * i - ScrollY;
			if (SkinProperties().BuddylistWindow.RowHeight + ItemY <= 0)
				continue;
			if (ItemY > Bottom)
				return;
			if (GetBuddyListIcon(i) > 0) {
				const int Icon = GetBuddyListIcon(i);
				PATELLA_ASSERT_VALUES(x+ItemX+SkinProperties().BuddylistWindow.Logo.x>=0, x);
				PATELLA_ASSERT_VALUES(y+ItemY+SkinProperties().BuddylistWindow.Logo.y>=0, y);
				DrawSkinClipped(Surface, SkinProperties().BuddylistWindow.Logo.x + x,
					SkinProperties().BuddylistWindow.Logo.y + ItemY + y, SkinProperties().BuddylistWindow.Logo.width,
					SkinProperties().BuddylistWindow.Logo.height, Icon + SKIN_BUDDY_ICON_BASE, x, y, ClipWidth, ClipHeight, 0, 0);
			}
			TRGBColor Color = IsBuddyOnline(i) ? SkinProperties().BuddylistWindow.OnlineColor
											   : SkinProperties().BuddylistWindow.OfflineColor;
			if (IsBuddyNewlyOnline(i))
				Color = SkinProperties().BuddylistWindow.NewColor;
			const char *Name = GetBuddyListName(i);
			const int MaxLength = GetFittingTextLength(SkinProperties().BuddylistWindow.NameFont.FontNumber, Name,
				SkinProperties().BuddylistWindow.Name.width);
			TFixedString<40> Shortened;
			if (MaxLength < (int)strlen(Name)) {
				Shortened.Assign(Name);
				ShortenString(Shortened, MaxLength);
				Name = Shortened;
			}
			PrintTextClipped(Surface, SkinProperties().BuddylistWindow.Name.x + x,
				SkinProperties().BuddylistWindow.Name.y + ItemY + y, SkinProperties().BuddylistWindow.NameFont.FontNumber,
				Color, Name, SkinProperties().BuddylistWindow.NameFont.Alignment, TRect(x, y, ClipWidth, ClipHeight));
		}
	}
}

void TBuddylistWindowBody::SetSize(int Width, int Height)
{
	const int ScrollbarX = Width - ScrollBar->Width;
	PATELLA_ASSERT_VALUES(ScrollbarX>0, ScrollbarX);
	TWindow::SetSize(Width, Height);
	ScrollBar->SetPosition(ScrollbarX, 0);
	ScrollBar->SetSize(ScrollBar->Width, Height);
	Arrange();
}

// The VIP menu (message 54), also for no buddy (0).
bool TBuddylistWindowBody::RightButtonUp(int x, int y, int Flags)
{
	if (!TWindow::RightButtonUp(x, y, Flags)) {
		const int Position = GetBuddyAt(x, y);
		AddMessage(MESSAGE_BUDDY_MENU, Position == -1 ? 0 : GetBuddyListID(Position), 0);
	}
	return true;
}

// A double click opens a private channel.
bool TBuddylistWindowBody::LeftButtonDoubleClick(int x, int y, int Flags)
{
	// NOLINTNEXTLINE(bugprone-parent-virtual-call): the original's call of the base's base (the image's)
	if (!TWindow::LeftButtonDoubleClick(x, y, Flags)) {
		const int Position = GetBuddyAt(x, y);
		if (Position != -1)
			SendOpenPrivateConversation(GetBuddyListName(Position));
	}
	return true;
}

bool TBuddylistWindowBody::MouseWheel(int x, int y, int Code)
{
	if (!TWindow::MouseWheel(x, y, Code) && (Code == 1 || Code == 2))
		return ScrollBar->MouseWheel(x, y, Code);
	return true;
}

// A shortened name in full, in a new buffer each time.
void TBuddylistWindowBody::MouseHover(int x, int y)
{
	const int Position = GetBuddyAt(x, y);
	if (Position != -1) {
		const char *Name = GetBuddyListName(Position);
		const int MaxLength = GetFittingTextLength(SkinProperties().BuddylistWindow.NameFont.FontNumber, Name,
			SkinProperties().BuddylistWindow.Name.width);
		if (MaxLength < (int)strlen(Name)) {
			if (MouseOverText != nullptr)
				MouseOverText.reset();
			MouseOverText = nullptr;
			MouseOverText.reset(new char[36]);
			MouseOverText[0] = 0;
			TStringBuffer(MouseOverText.get(), 36).Append(Name);
			SetMouseOverText(MouseOverText.get());
		}
	}
}

bool TBuddylistWindowBody::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	SetHelpHandle(HELP_VIP);
	const TComponent *Parent = GetParent();
	PATELLA_ASSERT(Parent!=nullptr);
	SetHelpSize(Parent->Width, Parent->Height);
	MoveHelpPosition(-OffsetX, -OffsetY);
	return true;
}

void TBuddylistWindowBody::Arrange()
{
	int ContentHeight = GetContentHeight();
	if (ContentHeight <= Height)
		ContentHeight = Height;
	ScrollBar->Range->SetRange(0, ContentHeight - Height, ScrollBar->Range->Value, Height);
}

// At least one row.
int TBuddylistWindowBody::getMinimumHeight()
{
	return (std::max)(ScrollBar->getMinimumHeight(), SkinProperties().BuddylistWindow.RowHeight);
}

// The stored height ("ContentWindow" 9) or the content.
int TBuddylistWindowBody::GetInitialHeight()
{
	int Height = GetContentWindowSize(BUDDYLIST_WINDOW);
	if (Height == 0)
		Height = GetContentHeight();
	if (Height < getMinimumHeight())
		return getMinimumHeight();
	return Height;
}

int TBuddylistWindowBody::GetContentHeight()
{
	return GetNumberOfBuddies() * SkinProperties().BuddylistWindow.RowHeight + SkinProperties().Data868;
}

// The row under x, y; -1 for none. Odd, kept: y is not tested against the top. y
// gets the scroll position in place (the parameter's slot): the original's catch
// wrapper printed the moved y (issue #34).
int TBuddylistWindowBody::GetBuddyAt(int x, int y)
{
	if (!IsMinimized()) {
		y += ScrollBar->Range->Value;
		const int Position = y / SkinProperties().BuddylistWindow.RowHeight;
		if (x >= 0 && x <= Width - ScrollBar->Width && Position < GetNumberOfBuddies())
			return Position;
	}
	return -1;
}
// NOLINTEND(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg)
