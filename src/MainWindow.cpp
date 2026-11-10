// MainWindow.cpp: the main game window (TMainWindow), the start window shown
// before the game (TStartWindow) and the splitter above the console
// (TSplitter); see MainWindow.h.
//
// In the game the window is laid out in two columns: the map frame, the
// splitter and the console on the left, the side bar (SidePanel in a frame)
// and the dialog window on the right, SkinProperties.SideBarWidth wide. The
// heights of the map and the console follow the console height of the
// preferences and the minimum sizes; the splitter changes the console's
// height. Leaving a mode (ChangeMode, the close) stores the open
// sub-windows of both columns in the preferences; entering the game opens
// them again.

#include "MainWindow.h"
#include "ContentWindows.h"
#include "Communication.h"
#include "ConsoleWindow.h"
#include "Creatures.h"
#include "DialogElements.h"
#include "Inventory.h"
#include "MapWindow.h"
#include "Network.h"
#include "Preferences.h"
#include "Skins.h"
#include "Status.h"
#include "Surface.h"
#include "Talk.h"
#include "graphics.h"
#include "DialogWindow.h"
#include "GUI.h"
#include "BuildConfig.h"
#include "common.h"
#include "vulkan/DeviceChoice.h"

#include <cstdint>
#include <cstdlib>
#include <memory>
#include <utility>

namespace {

// The texts of the user interface that this file shows. The numbers are their places in the original's table of
// texts, a header every file included.
const char *TextNoBugReportRight = "You are not authorised to send bug reports."; // 124
const char *TextAccountURL = patella::buildconfig::LinkAccount;                   // 143

} // namespace

// cppcoreguidelines-no-malloc and cppcoreguidelines-owning-memory are off for the file, for the original's malloc'd
// entries and texts, which their lists free, and its raw owners: the window tree owns its children, the message queue
// its blocks, the lists their entries.
// NOLINTBEGIN(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)

// OpenURLCallback, SetDisplayModeCallback, ExitProgramCallback, ModalWindow and MainWindow are TGUI's (GUI.h).

// --- TMainWindow -----------------------------------------------------------

// The size is set before the children exist (TWindow's SetSize, not the
// own one); ChangeMode creates them. Ctrl+F and Alt+Enter switch between
// windowed and fullscreen mode.
TMainWindow::TMainWindow(int Width, int Height)
{
	TWindow::SetSize(Width, Height);
	ChangeMode(false);
	GUI().AddHotkey(KEY_CONTROL | 'F', TCallback(this, EVENT_TOGGLE_FULLSCREEN));
	GUI().AddHotkey(KEY_ALT | KEY_RETURN, TCallback(this, EVENT_TOGGLE_FULLSCREEN));
}

// Its work is the close's (OnClose).
TMainWindow::~TMainWindow()
{
	CheckClosed("TMainWindow");
}

// The open sub-windows go into the preferences (ChangeMode starts with the
// same code); then the children.
void TMainWindow::OnClose()
{
	try {
		if (FirstChild != nullptr && InGame) {
			Preferences().ClearOpenDialogs();
			if (SidePanel != nullptr)
				SidePanel->StoreOpenDialogs();
			if (DialogWindow != nullptr)
				DialogWindow->StoreOpenDialogs();
		}
	} catch (...) {
		CloseWhileUnwinding([this] { TWindow::OnClose(); });
		throw;
	}
	TWindow::OnClose();
}

// TGUI's constructor and destructor (GUI.h): here, where TMainWindow is complete for its owner.
patella::TGUI::TGUI(TNetwork &Network, TCommunication &Communication) : Network(Network), Communication(Communication)
{
}

patella::TGUI::~TGUI() = default;

void patella::TGUI::AbandonWindows() noexcept
{
	(void)MainWindow.release(); // NOLINT(bugprone-unused-return-value): left allocated on purpose (GUI.h)
}

// Called by Control.cpp's ExitControl. Each window is closed and then freed;
// a close that throws leaves its window allocated (the main window in its
// owner).
void patella::TGUI::ExitMainWindow()
{
	if (MainWindow != nullptr)
		MainWindow->Close();
	MainWindow.reset();
	if (ModalWindow != nullptr) {
		DeleteComponent(ModalWindow);
		Interaction.SetActionState(ACTIONSTATE_NONE);
	}
	ModalWindow = nullptr;
}

// Out of the game an error while drawing the start window becomes
// PROGRAM_ERROR 12 (the code is shared with the surface errors of
// Control.cpp, Player.cpp and Surface.cpp). In the game the children are
// drawn in a fixed order, the map frame after the right column.
void TMainWindow::Draw(int Surface, int x, int y)
{
	if (!InGame) {
		try {
			TWindow::Draw(Surface, x, y);
		} catch (...) {
			throw TProgramError{PROGRAM_ERROR::SURFACE};
		}
		return;
	}
	Console->Draw(Surface, Console->OffsetX + x, Console->OffsetY + y);
	SideBar->Draw(Surface, SideBar->OffsetX + x, SideBar->OffsetY + y);
	DialogWindow->Draw(Surface, DialogWindow->OffsetX + x, DialogWindow->OffsetY + y);
	MapFrame->Draw(Surface, MapFrame->OffsetX + x, MapFrame->OffsetY + y);
	Splitter->Draw(Surface, Splitter->OffsetX + x, Splitter->OffsetY + y);
}

// The map gets the height the console leaves, at least its minimum height
// and at most what the console's minimum height leaves.
void TMainWindow::SetSize(int Width, int Height)
{
	TWindow::SetSize(Width, Height);
	Interaction().SetWindowSize(Width, Height);
	if (InGame) {
		const int SplitterHeight = Splitter->Height;
		int MaxMapHeight = MapFrame->GetMinimumHeight();
		const int Rest = Height - Console->GetMinimumHeight() - SplitterHeight;
		if (Rest > MaxMapHeight)
			MaxMapHeight = Rest;
		const int MinMapHeight = MapFrame->GetMinimumHeight();
		int MapHeight = Height - Console->Height - Splitter->Height;
		if (MapHeight < MinMapHeight)
			MapHeight = MinMapHeight;
		if (MapHeight > MaxMapHeight)
			MapHeight = MaxMapHeight;
		const int MapWidth = Width - SkinProperties().SideBarWidth;
		MapFrame->SetPosition(0, 0);
		MapFrame->SetSize(MapWidth, MapHeight);
		MapHeight = MapFrame->Height;
		Splitter->SetSize(MapWidth, SkinProperties().SplitterHeight);
		Splitter->SetPosition(0, MapHeight);
		Splitter->Arrange();
		Console->SetSize(MapWidth, Height - SkinProperties().SplitterHeight - MapHeight);
		Console->SetPosition(0, Splitter->Height + MapHeight);
		Console->Arrange();
		SideBar->SetPosition(MapWidth, 0);
		DialogWindow->SetSize(SkinProperties().SideBarWidth, Height - SideBar->Height);
		DialogWindow->SetPosition(MapWidth, SideBar->Height);
	} else {
		StartWindow->SetSize(Width, Height);
		StartWindow->SetPosition(0, 0);
	}
	Arrange();
}

// Only in connection state 8 (in the game): the arrow keys and the keypad
// walk (the diagonals with Diagonal set), Ctrl with an arrow key turns,
// every other key goes to the console.
void TMainWindow::KeyPress(int Key)
{
	if (GetConnectionState() != CONNECTION_IN_GAME)
		return;
	switch (Key) {
	case KEY_UP: Walk(0, -1, false); break;
	case KEY_LEFT: Walk(-1, 0, false); break;
	case KEY_RIGHT: Walk(1, 0, false); break;
	case KEY_DOWN: Walk(0, 1, false); break;
	case KEY_PAD_END: Walk(-1, 1, true); break;
	case KEY_PAD_PAGE_DOWN: Walk(1, 1, true); break;
	case KEY_PAD_HOME: Walk(-1, -1, true); break;
	case KEY_PAD_PAGE_UP: Walk(1, -1, true); break;
	case KEY_CONTROL | KEY_UP: Communication().SendTurnNorth(); break;
	case KEY_CONTROL | KEY_LEFT: Communication().SendTurnWest(); break;
	case KEY_CONTROL | KEY_RIGHT: Communication().SendTurnEast(); break;
	case KEY_CONTROL | KEY_DOWN: Communication().SendTurnSouth(); break;
	default:
		Console->KeyPress(Key);
		break;
	}
}

// The events of the hotkeys, the buttons of the side-bar windows and the menus. 0x17 and 0x18 swap the two inventory
// windows; 0xe, 0xf and 0x16 open or close the skill, battle and VIP windows; 0x1d gives up a connection; 0x1f8
// switches between windowed and fullscreen mode (a fullscreen mode below 640x480 is raised to it first). GCC's
// analyzer: the message queue owns the block it gets as an integer (AddMessage; the message's handler deletes it).
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic push")
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic ignored \"-Wanalyzer-malloc-leak\"")
void TMainWindow::HandleEvent(int Event)
{
	const patella::TGUI &GUI = ::GUI();
	switch (Event) {
	case EVENT_COPY_LINK:
		Interaction().CopyTaintedClipboardText();
		[[fallthrough]];
	case EVENT_CANCEL:
		Interaction().SetActionState(ACTIONSTATE_NONE);
		break;
	case EVENT_EXIT_PROGRAM:
		GUI.ExitProgramCallback();
		break;
	case EVENT_OPTIONS:
		AddMessage(MESSAGE_OPTIONS_DIALOG, 0, 0);
		break;
	case EVENT_REARRANGE:
		SetSize(Width, Height);
		break;
	case EVENT_TOGGLE_SKILLS:
		if (DialogWindow->GetContentWindow(SKILLS_WINDOW) != nullptr)
			CloseContentWindow(SKILLS_WINDOW);
		else
			OpenContentWindow(SKILLS_WINDOW);
		break;
	case EVENT_TOGGLE_BATTLE:
		if (DialogWindow->GetContentWindow(BATTLE_WINDOW) != nullptr)
			CloseContentWindow(BATTLE_WINDOW);
		else
			OpenContentWindow(BATTLE_WINDOW);
		break;
	case EVENT_TOGGLE_VIP:
		if (DialogWindow->GetContentWindow(BUDDYLIST_WINDOW) != nullptr)
			CloseContentWindow(BUDDYLIST_WINDOW);
		else
			OpenContentWindow(BUDDYLIST_WINDOW);
		break;
	case EVENT_MINIMISE_INVENTORY: {
		TContentWindow *OldWindow = SidePanel->GetContentWindow(INVENTORY_WINDOW);
		PATELLA_ASSERT(OldWindow!=nullptr);
		SidePanel->InsertChildAfter(OldWindow, CreateSmallInventoryWindow(this));
		SidePanel->DeleteChild(OldWindow);
		SidePanel->Rearrange(32, nullptr);
		SideBar->SetSize(SkinProperties().SideBarWidth, SidePanel->GetMinimumHeight() + SideBar->GetMinimumHeight());
		SetSize(Width, Height);
		break;
	}
	case EVENT_MAXIMISE_INVENTORY: {
		TContentWindow *OldWindow = SidePanel->GetContentWindow(SMALL_INVENTORY_WINDOW);
		PATELLA_ASSERT(OldWindow!=nullptr);
		SidePanel->InsertChildAfter(OldWindow, CreateInventoryWindow(this));
		SidePanel->DeleteChild(OldWindow);
		SidePanel->Rearrange(32, nullptr);
		SideBar->SetSize(SkinProperties().SideBarWidth, SidePanel->GetMinimumHeight() + SideBar->GetMinimumHeight());
		SetSize(Width, Height);
		break;
	}
	case EVENT_ABORT:
		Network().Disconnect(false);
		if (GetConnectionState() == CONNECTION_CONNECTING_GAME_SERVER || GetConnectionState() == CONNECTION_GAME_LOGIN_SENT) {
			AddMessage(MESSAGE_WAIT_DIALOG, 20, 0);
			AddMessage(MESSAGE_CHARACTER_LIST_DIALOG, 300, 0);
		}
		SetConnectionState(CONNECTION_OFFLINE);
		Interaction().SetActionState(ACTIONSTATE_NONE);
		break;
	case EVENT_STOP:
		PlayerState().ClearPlayerSteps();
		Creatures().SetAttackID(0);
		Communication().SendStopAttackAndWalk();
		break;
	case EVENT_TOGGLE_FULLSCREEN:
		if (Preferences().GetFullscreenMode()->Width != 0) {
			TFullscreenMode Mode = *Preferences().GetFullscreenMode();
			if (Mode.Width < 640 || Mode.Height < 480) {
				Mode.Width = 640;
				Mode.Height = 480;
				Preferences().SetFullscreenMode(&Mode);
				SetSize(640, 480);
			}
			GUI.SetDisplayModeCallback(Preferences().GetViewMode() == 1 ? 2 : 1);
		}
		break;
	case EVENT_BUG_REPORT:
		if (!Session().CheckRight(40, -1)) {
			SetStatusMessage(TextNoBugReportRight, false);
		} else if (GetConnectionState() == CONNECTION_IN_GAME && GUI.ModalWindow == nullptr) {
			AddMessage(MESSAGE_BUG_REPORT_DIALOG, 0, 0);
		}
		break;
	case EVENT_LOGOUT:
		Interaction().SetActionState(ACTIONSTATE_NONE);
		Communication().SendLogout();
		break;
	case EVENT_LEAVE_QUESTION: {
		// A question with an answer event; message 6 deletes the block.
		std::intptr_t *Data = new std::intptr_t[4];
		Data[0] = reinterpret_cast<std::intptr_t>("Warning");
		Data[1] = reinterpret_cast<std::intptr_t>("Are you sure you want to leave Tibia?");
		Data[2] = reinterpret_cast<std::intptr_t>(this);
		Data[3] = EVENT_LOGOUT;
		AddMessage(MESSAGE_QUESTION_DIALOG, reinterpret_cast<std::intptr_t>(Data), 0);
		break;
	}
	case EVENT_CHARACTER_LIST:
		AddMessage(MESSAGE_CHARACTER_LIST_DIALOG, 300, 0);
		break;
	case EVENT_HELP:
		AddMessage(MESSAGE_HELP_DIALOG, 0, 0);
		break;
	case EVENT_HELP_MODE:
		Interaction().SetActionState(ACTIONSTATE_NONE);
		AddMessage(MESSAGE_HELP_MODE, 0, 0);
		break;
	case EVENT_TUTOR_HELP:
		Communication().SendJoinChannel(7);
		break;
	case EVENT_REPORT:
		AddMessage(MESSAGE_REPORT_DIALOG, 0, 0);
		break;
	case EVENT_RULE_VIOLATION:
		if (Session().CheckAnyRight())
			AddMessage(MESSAGE_RULE_VIOLATION_DIALOG, 0, 0);
		break;
	case EVENT_DEVICE_AUTOMATIC:
		patella::TGUI::ChangeDevice(patella::vulkan::TDeviceButton::Automatic); // Control.cpp's
		break;
	case EVENT_DEVICE_GPU:
		patella::TGUI::ChangeDevice(patella::vulkan::TDeviceButton::Gpu); // Control.cpp's
		break;
	case EVENT_DEVICE_CPU:
		patella::TGUI::ChangeDevice(patella::vulkan::TDeviceButton::Cpu); // Control.cpp's
		break;
	default:
		break;
	}
}
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic pop")

// Rebuilds the window for the game (InGame) or for the start window. The
// open sub-windows are stored in the preferences first and read back when
// the game starts: the side-bar windows 1-5 (missing ones are added: 5, the
// inventory, 4 and 3; InsertChild puts each on top) and the skill, battle
// and VIP windows (not the trade window). The member pointers are not
// initialised before the first call: only FirstChild guards them. SideBar is
// not cleared with the others: out of the game it points to the deleted
// frame (never used there).
void TMainWindow::ChangeMode(bool InGame)
{
	if (FirstChild != nullptr && this->InGame) {
		Preferences().ClearOpenDialogs();
		if (SidePanel != nullptr)
			SidePanel->StoreOpenDialogs();
		if (DialogWindow != nullptr)
			DialogWindow->StoreOpenDialogs();
	}
	while (FirstChild != nullptr)
		DeleteChild(FirstChild);
	MapFrame = nullptr;
	Console = nullptr;
	DialogWindow = nullptr;
	SidePanel = nullptr;
	Splitter = nullptr;
	StartWindow = nullptr;
	this->InGame = InGame;
	if (InGame) {
		MapFrame = new TMapFrame;
		Splitter = new TSplitter;
		Console = new TConsoleWindow;
		SidePanel = new TDialogWindow;
		DialogWindow = new TDialogWindow;
		int *Dialogs;
		const int Count = Preferences().GetOpenDialogs(&Dialogs);
		bool HaveAutomap = false;
		bool HaveStatus = false;
		bool HaveInventory = false;
		bool HaveButtons = false;
		for (int i = Count - 1; i >= 0; i--) {
			switch (Dialogs[i]) {
			case INVENTORY_WINDOW:
				SidePanel->InsertChild(CreateInventoryWindow(this));
				HaveInventory = true;
				break;
			case SMALL_INVENTORY_WINDOW:
				SidePanel->InsertChild(CreateSmallInventoryWindow(this));
				HaveInventory = true;
				break;
			case AUTOMAP_WINDOW:
				SidePanel->InsertChild(new TAutomapWindow);
				HaveAutomap = true;
				break;
			case STATUS_WINDOW:
				SidePanel->InsertChild(new TStatusWindow);
				HaveStatus = true;
				break;
			case BUTTONS_WINDOW:
				SidePanel->InsertChild(new TButtonsWindow(DialogWindow, this));
				HaveButtons = true;
				break;
			default:
				break;
			}
		}
		if (!HaveButtons)
			SidePanel->InsertChild(new TButtonsWindow(DialogWindow, this));
		if (!HaveInventory)
			SidePanel->InsertChild(CreateInventoryWindow(this));
		if (!HaveStatus)
			SidePanel->InsertChild(new TStatusWindow);
		if (!HaveAutomap)
			SidePanel->InsertChild(new TAutomapWindow);
		SideBar = new TBorder(SidePanel, 0);
		InsertChild(MapFrame);
		InsertChild(Splitter);
		InsertChild(Console);
		InsertChild(SideBar);
		InsertChild(DialogWindow);
		// The console keeps its stored height unless the map would get
		// smaller than its minimum height; then it gets its own minimum.
		int ConsoleHeight = Console->GetMinimumHeight();
		const int StoredHeight = Preferences().GetConsoleHeight();
		if (StoredHeight > ConsoleHeight)
			ConsoleHeight = StoredHeight;
		int MapWidth = Width - SkinProperties().SideBarWidth;
		int MapHeight = Height - ConsoleHeight - SkinProperties().SplitterHeight;
		if (MapHeight < MapFrame->GetMinimumHeight()) {
			ConsoleHeight = Console->GetMinimumHeight();
			MapWidth = Width - SkinProperties().SideBarWidth;
			MapHeight = Height - ConsoleHeight - SkinProperties().SplitterHeight;
		}
		MapFrame->SetPosition(0, 0);
		MapFrame->SetSize(MapWidth, MapHeight);
		Splitter->SetSize(MapWidth, SkinProperties().SplitterHeight);
		Splitter->SetPosition(0, MapHeight);
		Splitter->Arrange();
		Console->SetSize(MapWidth, ConsoleHeight);
		Console->SetPosition(0, MapHeight + SkinProperties().SplitterHeight);
		Console->OpenDefaultChannel(Talk().DefaultTextBuffer, "Default");
		Console->Arrange();
		SideBar->SetPosition(MapWidth, 0);
		SideBar->SetSize(SkinProperties().SideBarWidth, SidePanel->GetMinimumHeight() + SideBar->GetMinimumHeight());
		DialogWindow->SetSize(SkinProperties().SideBarWidth, Height - SideBar->Height);
		DialogWindow->SetPosition(MapWidth, SideBar->Height);
		for (int i = 0; i < Count; i++) {
			const int Identifier = Dialogs[i];
			if (Identifier >= SKILLS_WINDOW && (Identifier <= BATTLE_WINDOW || Identifier == BUDDYLIST_WINDOW))
				OpenContentWindow(Identifier);
		}
		if (Count > 0) {
			free(Dialogs);
			Dialogs = nullptr;
		}
		// Ctrl+L and Ctrl+Q log out, Ctrl+G asks for another character,
		// Ctrl+S, Ctrl+B and Ctrl+P toggle the skill, battle and VIP windows,
		// Ctrl+Z reports a bug, Ctrl+R a rule violation, Ctrl+Y opens the
		// gamemaster dialog, Ctrl+H the hints, Ctrl+T the channel 7 and
		// Escape cancels the attack and the walk.
		GUI().AddHotkey(KEY_CONTROL | 'L', TCallback(this, EVENT_LOGOUT));
		GUI().AddHotkey(KEY_CONTROL | 'Q', TCallback(this, EVENT_LOGOUT));
		GUI().AddHotkey(KEY_CONTROL | 'G', TCallback(this, EVENT_CHARACTER_LIST));
		GUI().AddHotkey(KEY_CONTROL | 'S', TCallback(this, EVENT_TOGGLE_SKILLS));
		GUI().AddHotkey(KEY_CONTROL | 'B', TCallback(this, EVENT_TOGGLE_BATTLE));
		GUI().AddHotkey(KEY_CONTROL | 'P', TCallback(this, EVENT_TOGGLE_VIP));
		GUI().AddHotkey(KEY_CONTROL | 'Z', TCallback(this, EVENT_BUG_REPORT));
		GUI().AddHotkey(KEY_CONTROL | 'R', TCallback(this, EVENT_REPORT));
		GUI().AddHotkey(KEY_CONTROL | 'Y', TCallback(this, EVENT_RULE_VIOLATION));
		GUI().AddHotkey(KEY_CONTROL | 'H', TCallback(this, EVENT_HELP_MODE));
		GUI().AddHotkey(KEY_CONTROL | 'T', TCallback(this, EVENT_TUTOR_HELP));
		GUI().AddHotkey(KEY_ESCAPE, TCallback(this, EVENT_STOP));
	} else {
		StartWindow = new TStartWindow;
		InsertChild(StartWindow);
		GUI().RemoveHotkey(KEY_CONTROL | 'L');
		GUI().RemoveHotkey(KEY_CONTROL | 'Q');
		GUI().RemoveHotkey(KEY_CONTROL | 'G');
		GUI().RemoveHotkey(KEY_CONTROL | 'S');
		GUI().RemoveHotkey(KEY_CONTROL | 'B');
		GUI().RemoveHotkey(KEY_CONTROL | 'P');
		GUI().RemoveHotkey(KEY_CONTROL | 'Z');
		GUI().RemoveHotkey(KEY_CONTROL | 'R');
		GUI().RemoveHotkey(KEY_CONTROL | 'Y');
		GUI().RemoveHotkey(KEY_CONTROL | 'H');
		GUI().RemoveHotkey(KEY_CONTROL | 'T');
		GUI().RemoveHotkey(KEY_ESCAPE);
	}
	SetSize(Width, Height);
	SetConsoleActive(GUI().ModalWindow == nullptr);
}

// The console's new height, at least its minimum; the caller lays out the
// window again.
void TMainWindow::ResizeConsole(int ConsoleHeight) const
{
	// One call of GetMinimumHeight.
	const int MinimumHeight = Console->GetMinimumHeight();
	if (ConsoleHeight < MinimumHeight)
		ConsoleHeight = MinimumHeight;
	Console->SetSize(Console->Width, ConsoleHeight);
}

int TMainWindow::GetConsoleWindowHeight() const
{
	return Console->Height;
}

// SetPlayerDestination returns 1 when the step can be walked and 2 when
// the player has to stop.
void TMainWindow::Walk(int dx, int dy, bool Diagonal)
{
	int x, y, z;
	PlayerState().GetPlayerPosition(&x, &y, &z);
	switch (PlayerState().SetPlayerDestination(x + dx, y + dy, z, true, Diagonal)) {
	case 1:
		Communication().SendWalk();
		break;
	case 2:
		Communication().SendStopWalk();
		break;
	default:
		break;
	}
}

// An open container window is filled again: the body is updated (scrolled to the top, its counts, icon, title and "up"
// button; ContentWindows.cpp) and, while it is minimized, the window gets the name and the object of the container and
// the "up" state of a container that has a parent here. GCC's analyzer (Debug) takes SkinProperties()' styles, set by
// InitSkinProperties, as uninitialized, and the title and icon SetTitle and SetIcon take as leaks (the window owns
// them).
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic push")
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic ignored \"-Wanalyzer-use-of-uninitialized-value\"")
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic ignored \"-Wanalyzer-malloc-leak\"")
void TMainWindow::OpenContainer(int Container) const
{
	PATELLA_ASSERT(DialogWindow!=nullptr);
	TSubWindow *Window = static_cast<TSubWindow *>(DialogWindow->GetContentWindow(Container + CONTAINER_WINDOW));
	if (Window == nullptr) {
		DialogWindow->Rearrange(16, CreateContainerWindow(Container));
		return;
	}
	Window->Arrange();
	TContainerWindowBody *Body = static_cast<TContainerWindowBody *>(Window->Body);
	Body->Update();
	if (Body->IsMinimized()) {
		Window->SetTitle(new TLabel(SkinProperties().SubWindow.TitleFont, Inventory().GetContainerName(Container)));
		Window->SetIcon(new TObjectIcon(Inventory().GetContainerObject(Container), &SkinProperties().SubWindow.IconHighlight, false));
		if (Inventory().HasContainerParent(Container))
			Window->ShowUpButton();
		else
			Window->HideUpButton();
	}
}
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic pop")

void TMainWindow::CloseContainer(int Container) const
{
	if (DialogWindow != nullptr) {
		TContentWindow *Window = DialogWindow->GetContentWindow(Container + CONTAINER_WINDOW);
		if (Window != nullptr)
			DialogWindow->DeleteChild(Window);
	}
}

void TMainWindow::OpenTrade() const
{
	PATELLA_ASSERT(DialogWindow!=nullptr);
	PATELLA_ASSERT(!DialogWindow->HasContentWindow(TRADE_WINDOW));
	OpenContentWindow(TRADE_WINDOW);
}

// The trade window's body gets event 0x15 (the counter offer arrived).
void TMainWindow::UpdateTrade() const
{
	const TSubWindow *Window = static_cast<TSubWindow *>(DialogWindow->GetContentWindow(TRADE_WINDOW));
	if (Window != nullptr) {
		PATELLA_ASSERT(Window->GetBody()!=nullptr);
		Window->GetBody()->HandleEvent(EVENT_TRADE_COUNTER_OFFER);
	}
}

void TMainWindow::CloseTrade() const
{
	CloseContentWindow(TRADE_WINDOW);
}

void TMainWindow::OpenContentWindow(int Identifier) const
{
	if (DialogWindow->GetContentWindow(Identifier) == nullptr) {
		TWindow *Window;
		switch (Identifier) {
		case SKILLS_WINDOW:
			Window = CreateSkillsWindow();
			break;
		case BATTLE_WINDOW:
			Window = CreateBattleWindow();
			break;
		case TRADE_WINDOW:
			Window = CreateTradeWindow();
			break;
		case BUDDYLIST_WINDOW:
			Window = CreateBuddylistWindow();
			break;
		default:
			TERROR1("unknown identifier", Identifier);
		}
		DialogWindow->Rearrange(16, Window);
	}
}

void TMainWindow::CloseContentWindow(int Identifier) const
{
	TContentWindow *Window = DialogWindow->GetContentWindow(Identifier);
	if (Window != nullptr)
		DialogWindow->DeleteChild(Window);
}

// The console takes the keyboard while no modal window is open
// (Control.cpp passes ModalWindow==NULL).
void TMainWindow::SetConsoleActive(bool Active) const
{
	if (Console != nullptr)
		Console->SetActive(Active);
}

// --- TStartWindow ----------------------------------------------------------

// The buttons sit on a plain window that the frame (skins 0x34-0x3c)
// surrounds.
TStartWindow::TStartWindow()
{
	TWindow *Panel = new TWindow;
	Panel->SetSize(SkinProperties().StartWindow.PanelWidth, SkinProperties().StartWindow.PanelHeight);
	TComponent *Button = new TTextButton(0, 0, "Enter Game", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_ENTER_GAME), nullptr);
	Panel->InsertChild(Button);
	Panel->SetChildPosition(Button, SkinProperties().StartWindow.EnterGameButton.x, SkinProperties().StartWindow.EnterGameButton.y);
	Button = new TTextButton(0, 0, "Options", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_START_OPTIONS), nullptr);
	Panel->InsertChild(Button);
	Panel->SetChildPosition(Button, SkinProperties().StartWindow.OptionsButton.x, SkinProperties().StartWindow.OptionsButton.y);
	Button = new TTextButton(0, 0, "Info", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_INFO), nullptr);
	Panel->InsertChild(Button);
	Panel->SetChildPosition(Button, SkinProperties().StartWindow.InfoButton.x, SkinProperties().StartWindow.InfoButton.y);
	Button = new TTextButton(0, 0, "Exit Game", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_EXIT_PROGRAM), nullptr);
	Panel->InsertChild(Button);
	Panel->SetChildPosition(Button, SkinProperties().StartWindow.ExitGameButton.x, SkinProperties().StartWindow.ExitGameButton.y);
	Button = new TTextButton(0, 0, "Access Account", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_ACCOUNT_URL), nullptr);
	Panel->InsertChild(Button);
	Panel->SetChildPosition(Button, SkinProperties().StartWindow.AccessAccountButton.x, SkinProperties().StartWindow.AccessAccountButton.y);
	Frame = new TBorder(Panel, SKIN_PANEL_CORNER_TOPLEFT, SKIN_PANEL_EDGE_TOP, SKIN_PANEL_CORNER_TOPRIGHT, SKIN_PANEL_EDGE_LEFT, SKIN_PANEL_EDGE_RIGHT, SKIN_PANEL_CORNER_BOTTOMLEFT, SKIN_PANEL_EDGE_BOTTOM, SKIN_PANEL_CORNER_BOTTOMRIGHT, SKIN_PANEL_BACKGROUND, SkinProperties().StartWindowBorder);
	InsertChild(Frame);
	Frame->FitToWindow();
}

// Picture 1, the start screen, goes to the 640x480 surface 2 and is
// stretched over the window.
void TStartWindow::Draw(int Surface, int x, int y)
{
	Graphics().DrawPicture(2, 0, 0, 1);
	GraphicsEngine().GetSurface()->StretchBlitRect(Surface, TRect(x, y, Width, Height), 2, TRect(0, 0, 640, 480));
	TWindow::Draw(Surface, x, y);
}

void TStartWindow::SetSize(int Width, int Height)
{
	TWindow::SetSize(Width, Height);
	SetChildPosition(Frame, SkinProperties().StartWindow.FramePosition.x, SkinProperties().StartWindow.FramePosition.y);
}

// The buttons work only while there is no connection.
void TStartWindow::HandleEvent(int Event)
{
	const patella::TGUI &GUI = ::GUI();
	if (GetConnectionState() != CONNECTION_OFFLINE)
		return;
	switch (Event) {
	case EVENT_ENTER_GAME:
		AddMessage(MESSAGE_LOGIN_DIALOG, 0, 0);
		break;
	case EVENT_INFO:
		AddMessage(MESSAGE_INFO_DIALOG, 0, 0);
		break;
	case EVENT_START_OPTIONS:
		AddMessage(MESSAGE_OPTIONS_DIALOG, 0, 0);
		break;
	case EVENT_EXIT_PROGRAM:
		GUI.ExitProgramCallback();
		break;
	case EVENT_ACCOUNT_URL:
		GUI.OpenURLCallback(TextAccountURL);
		break;
	default:
		break;
	}
}

// --- TSplitter -------------------------------------------------------------

TSplitter::TSplitter()
{
	Dragging = false;
	DrawingListLength = 3;
	DrawingList.reset(new TDrawing[3]);
	for (int i = 0; i < DrawingListLength; i++)
		DrawingList[i].skin = SKIN_NONE;
}

// Skins 8 (top), 7 (middle, stretched) and 9 (bottom).
void TSplitter::Arrange()
{
	DrawingList[0] = TDrawing(SKIN_FRAME_EDGE_BOTTOM, 0, 0, Width, Skins()[SKIN_FRAME_EDGE_BOTTOM].height, 0, 0);
	DrawingList[1] = TDrawing(SKIN_BACKGROUND, 0, Skins()[SKIN_FRAME_EDGE_BOTTOM].height, Width, Height - Skins()[SKIN_FRAME_EDGE_TOP].height - Skins()[SKIN_FRAME_EDGE_BOTTOM].height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_FRAME_EDGE_TOP, 0, Height - Skins()[SKIN_FRAME_EDGE_TOP].height, Width, Skins()[SKIN_FRAME_EDGE_TOP].height, 0, 0);
}

// Mouse cursor 2 is the vertical size arrow. While dragging, the console
// follows the mouse; the global MainWindow is used, not the parent.
bool TSplitter::MouseMove([[maybe_unused]] int x, int y)
{
	const patella::TGUI &GUI = ::GUI();
	Interaction().SetMouseCursor(CURSOR_SIZENS);
	if (Dragging) {
		GUI.MainWindow->ResizeConsole(DragY - y + GUI.MainWindow->GetConsoleWindowHeight());
		GUI.MainWindow->SetSize(GUI.MainWindow->Width, GUI.MainWindow->Height);
	}
	return true;
}

bool TSplitter::LeftButtonDown(int x, int y, [[maybe_unused]] int Flags)
{
	SetCapture(nullptr);
	DragX = x;
	Dragging = true;
	DragY = y;
	return true;
}

// ReleaseCapture clears Dragging.
bool TSplitter::LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	if (Dragging)
		ReleaseCapture(nullptr);
	return true;
}

// --- The main window -------------------------------------------------------

// Called by SdlFrame.cpp's Create.
void patella::TGUI::InitMainWindow(int Width, int Height, void (*ExitProgram)(), void (*SetDisplayMode)(int Mode),
	void (*OpenURL)(const char *URL))
{
	std::unique_ptr<TMainWindow> Made(new TMainWindow(Width, Height));
	(void)MainWindow.release(); // NOLINT(bugprone-unused-return-value): the original stored over it (set only once)
	MainWindow = std::move(Made);
	PATELLA_ASSERT_VALUES(Interaction.GetActionState()==ACTIONSTATE_NONE, Interaction.GetActionState());
	ExitProgramCallback = ExitProgram;
	ModalWindow = nullptr;
	SetDisplayModeCallback = SetDisplayMode;
	OpenURLCallback = OpenURL;
}
// NOLINTEND(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
