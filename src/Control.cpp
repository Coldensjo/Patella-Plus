// Control.cpp: reconstructed from Tibia.exe 7.72.
//
// Functions are in original source order, which comes from the assertion and
// catch line numbers. The binary does not keep that order: CheckSurface,
// Redraw, ProcessEvents, ReceivePackets and InitControl come after the
// others. The functions here have no handlers apart from their own:
// CheckSurface's (an error of the surface becomes PROGRAM_ERROR 12),
// ChangeDevice's (each step's error with its message) and InitControl's (a
// PROGRAM_ERROR shuts the modules down again).
//
// The functions are TGUI's members (GUI.h), the data they read TGUI's and,
// through its constructor references, TNetwork's and TCommunication's; the
// free functions of the original's names, at the end of the file, forward to
// them. Their calls of each other are member calls; their calls into the
// other files (GUI.cpp's hotkeys and timers and MainWindow.cpp's
// ExitMainWindow included, with :: where a TGUI member of the same name would
// hide them) stay those files' free functions, which the golden Control and
// Communication tests fake (commworld.cpp's WorldFakes).
//
// The event queue (Status.cpp, AddMessage and FetchMessage) carries a type
// and two parameters; ProcessEvents turns each event into a dialog, a context
// menu or an action. Parameters that are strings or arrays belong to the
// event and are deleted here.

#include "Control.h"
#include "ConsoleWindow.h"
#include "ContextMenus.h"
#include "Dialogs.h"
#include "Help.h"
#include "Inventory.h"
#include "MainWindow.h"
#include "Network.h"
#include "Map.h"
#include "Objects.h"
#include "Communication.h"
#include "Creatures.h"
#include "graphics.h"
#include "Preferences.h"
#include "Skins.h"
#include "Status.h"
#include "Surface.h"
#include "Talk.h"
#include "sysinc.h"
#include "GUI.h"
#include "common.h"
#include "compat/span.h"
#include "platform/Platform.h"
#include "types/FixedString.h"
#include <cstdint>
#include <array>
#include <cstddef>
#include <cstdio>
#include <cstring>
#include "vulkan/DeviceSettings.h"
#include "vulkan/DeviceChoice.h"

namespace {

// The texts of the user interface that this file shows. The numbers are their places in the original's table of
// texts, a header every file included.
const char *TextCharacterListLoading = "Your character list is being loaded. Please wait."; // 109
const char *TextConnectingGameWorld = "Connecting to the game world. Please wait.";         // 110
const char *TextWaitForCharacterList = "Please wait for your character list.";              // 111
const char *TextLinkCopyWarning =                                                           // 114
	"Warning!\n"
	"You are about to copy a link. Keep in mind that unofficial\n"
	"websites may be used to hack your account! Continue?";
const char *TextNeverGiveAccount = // 115
	"Warning!\n"
	"Never give your account number or your password to anybody!\n"
	"Not even gamemasters or CipSoft members will need this data.\n"
	"People asking you for this information are trying to hack you!";
const char *TextRuleViolations = "Rule Violations"; // 145

} // namespace

// cppcoreguidelines-owning-memory and cppcoreguidelines-pro-type-vararg are off for the file, for the original's raw
// owners (the window tree owns its children, the message queue its blocks, the lists their entries) and its varargs
// (TMultiStateButton's constructor, _snprintf).
// NOLINTBEGIN(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg)

// The client area changed size (MainFrm.cpp); in fullscreen mode (view
// mode 2) the screen size counts. An open context menu (action state 11)
// is closed, a dialog (10) is centred again and a help bubble (9)
// follows the main window.
void patella::TGUI::Resize(int Width, int Height)
{
	SetMouseCursor(CURSOR_ARROW);
	if (GetViewMode() == 2) {
		const TFullscreenMode *Mode = GetFullscreenMode();
		Width = Mode->Width;
		Height = Mode->Height;
	}
	MainWindow->SetSize(Width, Height);
	if (ModalWindow != nullptr) {
		if (GetActionState() == ACTIONSTATE_CONTEXT_MENU) {
			DeleteComponent(ModalWindow);
			ModalWindow = nullptr;
			SetActionState(ACTIONSTATE_NONE);
			MainWindow->SetConsoleActive(true);
		}
		if (ModalWindow != nullptr && GetActionState() == ACTIONSTATE_DIALOG)
			ModalWindow->SetPosition(MainWindow->Width / 2 - ModalWindow->Width / 2,
				MainWindow->Height / 2 - ModalWindow->Height / 2);
		if (ModalWindow != nullptr && GetActionState() == ACTIONSTATE_HELP)
			(static_cast<THelpBubble *>(ModalWindow))->Place(MainWindow.get());
	}
}

// Lets the surface check the window and display mode; chooses the
// automatic device when that fails.
void patella::TGUI::CheckSurface(bool Force)
{
	TFullscreenMode *Resolution = GetFullscreenMode();
	bool Success;
	try {
		Success = GetSurface()->Init(&WindowHandle, GetViewMode(), Resolution, Force);
	} catch (...) {
		throw TProgramError{PROGRAM_ERROR::SURFACE};
	}
	if (!Success)
		ChangeDevice(patella::vulkan::TDeviceButton::Automatic);
	if (GetSurface()->SurfaceRecreated(1))
		ResetVideoCache();
}

// A click on one of the advanced graphics options' three device buttons (MainWindow.cpp); CheckSurface
// makes the automatic one's when the surface cannot be set up. A new choice is applied at once: the
// surface is released and the new device tested on a new one (the next CheckSurface creates its
// surfaces). When the test fails, the old choice stays; when it passes, Patella.ini gets the new choice
// as its Device. The button's number becomes the preferences' Engine (vulkan/DeviceSettings.h) when the
// click leaves the choice as it was and when the new device passed its test. In fullscreen mode the
// dialog disables the buttons.
void patella::TGUI::ChangeDevice(patella::vulkan::TDeviceButton Button)
{
	const patella::vulkan::TDeviceChoice OldChoice = GetDeviceChoice();
	const patella::vulkan::TDeviceChoice NewChoice = patella::vulkan::ChoiceAfterButton(OldChoice, Button,
		GetNumberOfGpus(), GetBestGpu());
	if (NewChoice == OldChoice) {
		SetEngine(EngineNumberOfButton(Button));
		return;
	}
	SetDeviceChoice(NewChoice);
	GetSurface()->ExitSurfaces();
	SurfaceExitInstance();
	const bool Success = GetSurface()->TestEngine();
	if (!Success) {
		SetDeviceChoice(OldChoice);
		SurfaceExitInstance();
	}
	GetSurface()->InitSurfaces();
	if (Success) {
		SetEngine(EngineNumberOfButton(Button));
		SaveDeviceChoice();
	}
}

// Draws the windows, the tooltip, the frame rate and the mouse cursor, then
// shows the frame. The tooltip is kept inside the screen; when it is wider or
// higher than the screen, the order of the two limits is swapped and it ends
// at 1.
void patella::TGUI::Redraw(bool Force)
{
	CheckSurface(Force);
	if (GetSurface()->IsUnavailable()) {
		patella::platform::Platform().Clock.FrameWait(10); // Sleep(10): the platform's frame wait
		return;
	}
	MainWindow->Draw(1, 0, 0);
	if (ModalWindow != nullptr)
		ModalWindow->Draw(1, ModalWindow->OffsetX, ModalWindow->OffsetY);
	UpdateBlinkState();
	const char *Tooltip = GetMouseOverText();
	if (Tooltip != nullptr) {
		int x, y;
		GetMouseOverTextPosition(&x, &y);
		const int Width = GetTextWidth(SkinProperties().TooltipFont.FontNumber, Tooltip) + SkinProperties().TooltipBorder * 2;
		const int Height = GetTextHeight(SkinProperties().TooltipFont.FontNumber, Tooltip) + SkinProperties().TooltipBorder * 2;
		y -= Height;
		const int MaxX = GetWindowWidth() - Width - 1;
		if (MaxX >= 1) {
			if (x < 1)
				x = 1;
			if (x > MaxX)
				x = MaxX;
		} else {
			if (x < MaxX)
				x = MaxX;
			if (x > 1)
				x = 1;
		}
		const int MaxY = GetWindowHeight() - Height - 1;
		if (MaxY >= 1) {
			if (y < 1)
				y = 1;
			if (y > MaxY)
				y = MaxY;
		} else {
			if (y < MaxY)
				y = MaxY;
			if (y > 1)
				y = 1;
		}
		const TRect Rect = {x, y, Width, Height};
		GetSurface()->FillRectangle(1, Rect, SkinProperties().TooltipBackground);
		GetSurface()->DrawFrame(1, Rect, 1, SkinProperties().TooltipFrame);
		PrintFormattedText(1, SkinProperties().TooltipBorder + x, SkinProperties().TooltipBorder + y,
			SkinProperties().TooltipFont, Tooltip);
	}
	if (ShowFPS) {
		const double FPS = GetSurface()->GetFPS();
		TFixedString<64> Text;
		// Stays _snprintf: printf's %f and std::format's differ for NaN ("-nan(ind)"), and a rate with more
		// than 10 digits fills the 20 characters (no NUL), which the type would cut.
		MsvcSnprintf(Text, 20, "DX9 %.02f fps", FPS);
		const TRGBColor Color = {200, 200, 200};
		PrintText(1, 4, 4, FONT_BORDERED, Color, Text, 0);
	}
	if (!GetWindowsMousepointer()) {
		const int Cursor = GetMouseCursor();
		if (GetMouseX() >= 0 && GetMouseY() >= 0) {
			int x = GetMouseX();
			int y = GetMouseY();
			switch (Cursor) {
			case CURSOR_ARROW:
				x -= SkinProperties().CursorHotspot1.x;
				y -= SkinProperties().CursorHotspot1.y;
				break;
			case CURSOR_SIZENS:
				x -= SkinProperties().CursorHotspot2.x;
				y -= SkinProperties().CursorHotspot2.y;
				break;
			case CURSOR_WAIT:
				x -= SkinProperties().CursorHotspot3.x;
				y -= SkinProperties().CursorHotspot3.y;
				break;
			case CURSOR_CROSSHAIR:
				x -= SkinProperties().CursorHotspot4.x;
				y -= SkinProperties().CursorHotspot4.y;
				break;
			case CURSOR_DRAG:
				x -= SkinProperties().CursorHotspot5.x;
				y -= SkinProperties().CursorHotspot5.y;
				break;
			case CURSOR_MAGNIFIER:
				x -= SkinProperties().CursorHotspot6.x;
				y -= SkinProperties().CursorHotspot6.y;
				break;
			default:
				break;
			}
			const TSkin *Picture = &Skins()[Cursor];
			DrawPicturePart(1, x, y, 4, Picture->x, Picture->y, Picture->width, Picture->height);
		}
	}
	GetSurface()->FinishFrame();
}

// Game packet 0x0a, once the connection state is 8.
void patella::TGUI::EnterGame() const
{
	MainWindow->ChangeMode(true);
	if (ModalWindow == nullptr) {
		SetActionState(ACTIONSTATE_NONE);
		MainWindow->SetConsoleActive(true);
	}
	ClearMap();
	InitInventory();
	ResetCreatures();
	ResetTalk();
	ShowPermanentIgnoreWarning();
	SetAmbientLight(-1, 0);
}

// The game connection was closed.
void patella::TGUI::LeaveGame()
{
	MainWindow->ChangeMode(false);
	if (ModalWindow != nullptr) {
		DeleteComponent(ModalWindow);
		ModalWindow = nullptr;
		MainWindow->SetConsoleActive(true);
	}
	SetActionState(ACTIONSTATE_NONE);
	SavePreferences();
	if (!Network.Reconnect)
		AddMessage(MESSAGE_CHARACTER_LIST_DIALOG, 300, 0);
}

// In help mode (action state 9) the help bubble is replaced when the help
// text has changed.
void patella::TGUI::MouseMove(int x, int y)
{
	SetMousePosition(x, y);
	SetMouseCursor(CURSOR_ARROW);
	switch (GetActionState()) {
	case ACTIONSTATE_LEFT_BUTTON_DOWN:
		CheckDragStart(x, y);
		[[fallthrough]];
	default:
		MainWindow->MouseMove(x, y);
		break;
	case ACTIONSTATE_DIALOG:
	case ACTIONSTATE_CONTEXT_MENU:
		PATELLA_ASSERT_VALUES(ModalWindow!=nullptr, GetActionState());
		ModalWindow->MouseMove(x - ModalWindow->OffsetX, y - ModalWindow->OffsetY);
		break;
	case ACTIONSTATE_HELP: {
		SetHelpPosition(0, 0);
		SetHelpSize(0, 0);
		MainWindow->HelpMouseMove(x, y);
		// The bubble is kept in a local: ModalWindow is read after
		// HelpMouseMove and written only after Place, so an exception before
		// that leaves it as it was.
		THelpBubble *Bubble = static_cast<THelpBubble *>(ModalWindow);
		if (TakeHelpHandleChanged()) {
			DeleteComponent(Bubble);
			Bubble = nullptr;
			if (GetHelpHandle() != HELP_NONE)
				Bubble = new THelpBubble(GetCurrentHelpText());
		}
		if (Bubble != nullptr)
			Bubble->Place(MainWindow.get());
		ModalWindow = Bubble;
		break;
	}
	}
	LastMouseMove = GetActionNumber();
	SetMouseOverText(nullptr);
}

void patella::TGUI::LeftButtonDown(int x, int y, int Flags)
{
	switch (GetActionState()) {
	case ACTIONSTATE_NONE:
		SetDragStart(x, y);
		MainWindow->LeftButtonDown(x, y, Flags);
		SetActionState(ACTIONSTATE_LEFT_BUTTON_DOWN);
		break;
	case ACTIONSTATE_RIGHT_BUTTON_DOWN:
		if (GetClassicControl())
			SetActionState(ACTIONSTATE_BOTH_BUTTONS_DOWN);
		break;
	case ACTIONSTATE_DIALOG:
	case ACTIONSTATE_CONTEXT_MENU:
		PATELLA_ASSERT(ModalWindow!=nullptr);
		ModalWindow->LeftButtonDown(x - ModalWindow->OffsetX, y - ModalWindow->OffsetY, Flags);
		break;
	default:
		break;
	}
	SetPendingAction(6, GetActionNumber());
	LastMouseMove = 0;
	SetMouseOverText(nullptr);
}

// Releasing the second of both buttons (state 4) sends a left button
// release with Flags 0x200.
void patella::TGUI::LeftButtonUp(int x, int y, int Flags)
{
	switch (GetActionState()) {
	case ACTIONSTATE_NONE:
		if (MainWindow->GetCapture() != nullptr) {
			MainWindow->LeftButtonUp(x, y, Flags);
			MainWindow->StopDragging();
		}
		break;
	case ACTIONSTATE_BOTH_BUTTONS_DOWN:
		SetActionState(ACTIONSTATE_LEFT_RELEASED);
		break;
	case ACTIONSTATE_RIGHT_RELEASED:
		MainWindow->LeftButtonUp(x, y, BOTH_BUTTONS_RELEASED);
		SetActionState(ACTIONSTATE_NONE);
		break;
	case ACTIONSTATE_LEFT_BUTTON_DOWN:
	case ACTIONSTATE_DRAGGING:
	case ACTIONSTATE_USE_WITH:
	case ACTIONSTATE_TRADE_WITH:
		MainWindow->LeftButtonUp(x, y, Flags);
		SetActionState(ACTIONSTATE_NONE);
		break;
	case ACTIONSTATE_DIALOG:
	case ACTIONSTATE_CONTEXT_MENU:
		PATELLA_ASSERT(ModalWindow!=nullptr);
		ModalWindow->LeftButtonUp(x - ModalWindow->OffsetX, y - ModalWindow->OffsetY, Flags);
		if (GetActionState() == ACTIONSTATE_NONE || GetActionState() == ACTIONSTATE_USE_WITH || GetActionState() == ACTIONSTATE_TRADE_WITH) {
			DeleteComponent(ModalWindow);
			ModalWindow = nullptr;
			MainWindow->SetConsoleActive(true);
		}
		break;
	case ACTIONSTATE_HELP:
		DeleteComponent(ModalWindow);
		ModalWindow = nullptr;
		SetActionState(ACTIONSTATE_NONE);
		break;
	default:
		break;
	}
	SetPendingAction(0, 0);
	LastMouseMove = 0;
	SetMouseOverText(nullptr);
}

void patella::TGUI::LeftButtonDoubleClick(int x, int y, int Flags)
{
	switch (GetActionState()) {
	case ACTIONSTATE_NONE:
		SetDragStart(x, y);
		MainWindow->LeftButtonDoubleClick(x, y, Flags);
		SetActionState(ACTIONSTATE_LEFT_BUTTON_DOWN);
		break;
	case ACTIONSTATE_RIGHT_BUTTON_DOWN:
		if (GetClassicControl())
			SetActionState(ACTIONSTATE_BOTH_BUTTONS_DOWN);
		break;
	case ACTIONSTATE_DIALOG:
	case ACTIONSTATE_CONTEXT_MENU:
		PATELLA_ASSERT(ModalWindow!=nullptr);
		ModalWindow->LeftButtonDoubleClick(x - ModalWindow->OffsetX, y - ModalWindow->OffsetY, Flags);
		if (GetActionState() == ACTIONSTATE_NONE || GetActionState() == ACTIONSTATE_USE_WITH || GetActionState() == ACTIONSTATE_TRADE_WITH) {
			DeleteComponent(ModalWindow);
			ModalWindow = nullptr;
			MainWindow->SetConsoleActive(true);
		}
		break;
	default:
		break;
	}
	SetPendingAction(6, GetActionNumber());
	LastMouseMove = 0;
	SetMouseOverText(nullptr);
}

// Only changes the action state; the windows are not told (the position and
// flags are not used).
void patella::TGUI::RightButtonDown([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	switch (GetActionState()) {
	case ACTIONSTATE_NONE:
		SetActionState(ACTIONSTATE_RIGHT_BUTTON_DOWN);
		break;
	case ACTIONSTATE_LEFT_BUTTON_DOWN:
		if (GetClassicControl())
			SetActionState(ACTIONSTATE_BOTH_BUTTONS_DOWN);
		break;
	default:
		break;
	}
	LastMouseMove = 0;
	SetMouseOverText(nullptr);
}

// Releasing the second of both buttons (state 5) sends a left button
// release with Flags 0x200.
void patella::TGUI::RightButtonUp(int x, int y, int Flags)
{
	switch (GetActionState()) {
	case ACTIONSTATE_NONE:
		if (MainWindow->GetCapture() != nullptr)
			MainWindow->StopDragging();
		break;
	case ACTIONSTATE_RIGHT_BUTTON_DOWN:
		MainWindow->RightButtonUp(x, y, Flags);
		SetActionState(ACTIONSTATE_NONE);
		break;
	case ACTIONSTATE_BOTH_BUTTONS_DOWN:
		SetActionState(ACTIONSTATE_RIGHT_RELEASED);
		break;
	case ACTIONSTATE_LEFT_RELEASED:
		MainWindow->LeftButtonUp(x, y, BOTH_BUTTONS_RELEASED);
		SetActionState(ACTIONSTATE_NONE);
		break;
	case ACTIONSTATE_DIALOG:
	case ACTIONSTATE_CONTEXT_MENU:
		PATELLA_ASSERT(ModalWindow!=nullptr);
		ModalWindow->RightButtonUp(x - ModalWindow->OffsetX, y - ModalWindow->OffsetY, Flags);
		if (GetActionState() == ACTIONSTATE_NONE) {
			DeleteComponent(ModalWindow);
			ModalWindow = nullptr;
			MainWindow->SetConsoleActive(true);
		}
		break;
	case ACTIONSTATE_HELP:
		DeleteComponent(ModalWindow);
		ModalWindow = nullptr;
		SetActionState(ACTIONSTATE_NONE);
		break;
	case ACTIONSTATE_USE_WITH:
	case ACTIONSTATE_TRADE_WITH:
		SetActionState(ACTIONSTATE_NONE);
		break;
	default:
		break;
	}
	LastMouseMove = 0;
	SetMouseOverText(nullptr);
}

// Direction 1 up, 2 down (MainFrm's OnMouseWheel).
void patella::TGUI::MouseWheel(int x, int y, int Direction)
{
	switch (GetActionState()) {
	case ACTIONSTATE_HELP:
		break;
	case ACTIONSTATE_DIALOG:
	case ACTIONSTATE_CONTEXT_MENU:
		PATELLA_ASSERT(ModalWindow!=nullptr);
		ModalWindow->MouseWheel(x - ModalWindow->OffsetX, y - ModalWindow->OffsetY, Direction);
		break;
	default:
		MainWindow->MouseWheel(x, y, Direction);
		break;
	}
	LastMouseMove = 0;
	SetMouseOverText(nullptr);
}

// Key 0x907 switches the frame rate display.
void patella::TGUI::KeyPress(int Key)
{
	if (Key == (KEY_ALT | KEY_F8)) {
		ShowFPS = !ShowFPS;
		return;
	}
	if (GetActionState() == ACTIONSTATE_HELP)
		SetActionState(ACTIONSTATE_NONE);
	if (ModalWindow != nullptr) {
		ModalWindow->KeyPress(Key);
		if (GetActionState() == ACTIONSTATE_NONE) {
			DeleteComponent(ModalWindow);
			ModalWindow = nullptr;
			MainWindow->SetConsoleActive(true);
		}
	} else if (!::CheckHotkey(Key)) {
		MainWindow->KeyPress(Key);
	}
	LastMouseMove = 0;
	SetMouseOverText(nullptr);
}

// False while a character is in the game; then the logout warning is posted (event 7) unless another window is open.
// The array is allocated with new[]; ProcessEvents deletes it. GCC's analyzer: the message queue owns the block it gets
// as an integer (AddMessage; the message's handler deletes it).
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic push")
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic ignored \"-Wanalyzer-malloc-leak\"")
bool patella::TGUI::CloseRequest() const
{
	if (GetConnectionState() == CONNECTION_IN_GAME) {
		if (ModalWindow == nullptr) {
			std::intptr_t *Data = new std::intptr_t[4];
			Data[0] = reinterpret_cast<std::intptr_t>("Warning");
			Data[1] = reinterpret_cast<std::intptr_t>("If you shut down the program, your character might stay in the game.\n"
													  "Click on \"Logout\" to ensure that your character leaves the game properly.\n"
													  "Click on \"Exit\" if you want to exit the program without logging out your character.");
			Data[2] = 0;
			Data[3] = 0;
			AddMessage(MESSAGE_LOGOUT_DIALOG, reinterpret_cast<std::intptr_t>(Data), 0);
		}
		return false;
	}
	return true;
}
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic pop")

// Callback for InitStatus.
void patella::TGUI::LinkCopyWarning()
{
	SetActionState(ACTIONSTATE_NONE);
	AddMessage(MESSAGE_LINK_COPY_WARNING, reinterpret_cast<std::intptr_t>("Link Copy Warning"), 0);
}

// The connect handler for InitNetwork: sends the login packet to the login
// server (state 2) or the game server (state 5).
void patella::TGUI::ConnectionEstablished()
{
	if (GetConnectionState() == CONNECTION_CONNECTING_LOGIN_SERVER) {
		SetConnectionState(CONNECTION_LOGIN_SENT);
		// The original's order: a call's arguments right to left (MSVC; GCC and Clang may differ).
		const char *Password = GetAccountPassword();
		SendLogin(GetAccountNumber(), Password);
		return;
	}
	if (GetConnectionState() == CONNECTION_CONNECTING_GAME_SERVER) {
		const int OldConnectionState = GetConnectionState();
		SetConnectionState(CONNECTION_GAME_LOGIN_SENT);
		Communication.OutfitRequested = false;
		// The original's order: In's arguments right to left (MSVC; GCC and Clang may differ).
		const int AccountCharacters = GetAccountCharacters();
		PATELLA_ASSERT_VALUES(In(GetActiveCharacter(),0,AccountCharacters-1), GetActiveCharacter(), OldConnectionState);
		// The original's order: a call's arguments right to left (MSVC; GCC and Clang may differ).
		const char *Password = GetAccountPassword();
		const char *Name = GetCharacterName(GetActiveCharacter());
		SendGameLogin(GetAccountNumber(), Name, Password);
		return;
	}
	SetConnectionState(CONNECTION_OFFLINE);
	Disconnect(true);
	SetActionState(ACTIONSTATE_NONE);
}

// The receive handler for InitNetwork. The connection state chooses the
// table: 4 the login server, otherwise the game server. The first packet
// moves state 3 to 4 and state 6 to 7.
void patella::TGUI::ReceivePackets()
{
	int Type = -1;
	while ((Type = GetNextCommand()) != -1) {
		Communication.LastPacketTypes[Communication.LastPacketIndex] = (uint8)Type;
		Communication.LastPacketIndex++;
		if (Communication.LastPacketIndex >= 10)
			Communication.LastPacketIndex -= 10;
		if (GetConnectionState() == CONNECTION_LOGIN_SENT)
			SetConnectionState(CONNECTION_LOGIN_ANSWERED);
		if (GetConnectionState() == CONNECTION_LOGIN_ANSWERED) {
			switch (Type) {
			case LOGIN_RECEIVE_ERROR: ReceiveLoginError(); break;
			case LOGIN_RECEIVE_MESSAGE: ReceiveLoginMessage(); break;
			case LOGIN_RECEIVE_MESSAGE_OF_THE_DAY:
				ReceiveMessageOfTheDay();
				break;
			// The patch packets (Tibia.exe: ReceivePatch, ReceiveWholeFile, ReceivePatchBlock) end the login.
			case LOGIN_RECEIVE_PATCH:
			case LOGIN_RECEIVE_WHOLE_FILE:
			case LOGIN_RECEIVE_PATCH_BLOCK: ReceiveUpdateRequest(); break;
			case LOGIN_RECEIVE_SERVER_OVERLOADED: ReceiveLoginServerOverloaded(); break;
			case LOGIN_RECEIVE_CHARACTER_LIST: ReceiveCharacterList(); break;
			default:
				TERROR1("unknown packet type during login", Type);
			}
		} else {
			if (GetConnectionState() == CONNECTION_GAME_LOGIN_SENT)
				SetConnectionState(CONNECTION_GAME_LOGIN_ANSWERED);
			switch (Type) {
			case GAME_RECEIVE_LOGIN:
				SetConnectionState(CONNECTION_IN_GAME);
				EnterGame();
				ReceiveGameLogin();
				AddEventHelp(HINT_FIRST_LOGIN);
				SendFightModes();
				break;
			case GAME_RECEIVE_GAMEMASTER_ACTIONS: ReceiveGamemasterActions(); break;
			case GAME_RECEIVE_ERROR: ReceiveGameError(); break;
			case GAME_RECEIVE_MESSAGE: ReceiveGameMessage(); break;
			case GAME_RECEIVE_LOGIN_WAIT: ReceiveLoginWait(); break;
			case GAME_RECEIVE_PING: ReceivePing(); break;
			case GAME_RECEIVE_MAP: ReceiveMap(); break;
			case GAME_RECEIVE_MAP_NORTH: ReceiveMapNorth(); break;
			case GAME_RECEIVE_MAP_EAST: ReceiveMapEast(); break;
			case GAME_RECEIVE_MAP_SOUTH: ReceiveMapSouth(); break;
			case GAME_RECEIVE_MAP_WEST: ReceiveMapWest(); break;
			case GAME_RECEIVE_FIELD: ReceiveField(); break;
			case GAME_RECEIVE_ADD_OBJECT: ReceiveAddObject(); break;
			case GAME_RECEIVE_CHANGE_OBJECT: ReceiveChangeObject(); break;
			case GAME_RECEIVE_DELETE_OBJECT: ReceiveDeleteObject(); break;
			case GAME_RECEIVE_MOVE_CREATURE: ReceiveMoveCreature(); break;
			case GAME_RECEIVE_OPEN_CONTAINER: {
				int Container;
				ReceiveOpenContainer(Container);
				MainWindow->OpenContainer(Container);
				break;
			}
			case GAME_RECEIVE_CLOSE_CONTAINER: {
				int Container;
				ReceiveCloseContainer(Container);
				MainWindow->CloseContainer(Container);
				break;
			}
			case GAME_RECEIVE_CONTAINER_ADD_OBJECT: ReceiveContainerAddObject(); break;
			case GAME_RECEIVE_CONTAINER_CHANGE_OBJECT: ReceiveContainerChangeObject(); break;
			case GAME_RECEIVE_CONTAINER_DELETE_OBJECT: ReceiveContainerDeleteObject(); break;
			case GAME_RECEIVE_INVENTORY_OBJECT: ReceiveInventoryObject(); break;
			case GAME_RECEIVE_DELETE_INVENTORY_OBJECT:
				ReceiveDeleteInventoryObject();
				break;
				// clang-format off
			case GAME_RECEIVE_OWN_TRADE: ReceiveOwnTrade(); MainWindow->OpenTrade(); break;
			case GAME_RECEIVE_COUNTER_TRADE: ReceiveCounterTrade(); MainWindow->UpdateTrade(); break;
			case GAME_RECEIVE_CLOSE_TRADE: ReceiveCloseTrade(); MainWindow->CloseTrade(); break;
			// clang-format on
			case GAME_RECEIVE_WORLD_LIGHT: ReceiveWorldLight(); break;
			case GAME_RECEIVE_MAGIC_EFFECT: ReceiveMagicEffect(); break;
			case GAME_RECEIVE_ANIMATED_TEXT: ReceiveAnimatedText(); break;
			case GAME_RECEIVE_DISTANCE_EFFECT: ReceiveDistanceEffect(); break;
			case GAME_RECEIVE_CREATURE_MARK: ReceiveCreatureMark(); break;
			case GAME_RECEIVE_CREATURE_HEALTH: ReceiveCreatureHealth(); break;
			case GAME_RECEIVE_CREATURE_LIGHT: ReceiveCreatureLight(); break;
			case GAME_RECEIVE_CREATURE_OUTFIT: ReceiveCreatureOutfit(); break;
			case GAME_RECEIVE_CREATURE_SPEED: ReceiveCreatureSpeed(); break;
			case GAME_RECEIVE_CREATURE_SKULL: ReceiveCreatureSkull(); break;
			case GAME_RECEIVE_CREATURE_SHIELD: ReceiveCreatureShield(); break;
			case GAME_RECEIVE_EDIT_TEXT: ReceiveEditText(); break;
			case GAME_RECEIVE_EDIT_HOUSE_TEXT: ReceiveEditHouseText(); break;
			case GAME_RECEIVE_PLAYER_STATS: ReceivePlayerStats(); break;
			case GAME_RECEIVE_PLAYER_SKILLS: ReceivePlayerSkills(); break;
			case GAME_RECEIVE_PLAYER_ICONS: ReceivePlayerIcons(); break;
			case GAME_RECEIVE_CANCEL_ATTACK: ReceiveCancelAttack(); break;
			case GAME_RECEIVE_TALK: ReceiveTalk(); break;
			case GAME_RECEIVE_CHANNEL_LIST: ReceiveChannelList(); break;
			case GAME_RECEIVE_OPEN_CHANNEL: ReceiveOpenChannel(); break;
			case GAME_RECEIVE_OPEN_PRIVATE_CONVERSATION: ReceiveOpenPrivateConversation(); break;
			case GAME_RECEIVE_RULE_VIOLATION_CHANNEL: ReceiveRuleViolationChannel(); break;
			case GAME_RECEIVE_REMOVE_RULE_VIOLATION: ReceiveRemoveRuleViolation(); break;
			case GAME_RECEIVE_CANCEL_RULE_VIOLATION: ReceiveCancelRuleViolation(); break;
			case GAME_RECEIVE_LOCK_RULE_VIOLATION: ReceiveLockRuleViolation(); break;
			case GAME_RECEIVE_OWN_CHANNEL: ReceiveOwnChannel(); break;
			case GAME_RECEIVE_CLOSE_CHANNEL: ReceiveCloseChannel(); break;
			case GAME_RECEIVE_TEXT_MESSAGE: ReceiveTextMessage(); break;
			case GAME_RECEIVE_CANCEL_WALK: ReceiveCancelWalk(); break;
			case GAME_RECEIVE_FLOOR_UP: ReceiveFloorUp(); break;
			case GAME_RECEIVE_FLOOR_DOWN: ReceiveFloorDown(); break;
			case GAME_RECEIVE_OUTFIT: ReceiveOutfit(); break;
			case GAME_RECEIVE_BUDDY: ReceiveBuddy(); break;
			case GAME_RECEIVE_BUDDY_LOGIN: ReceiveBuddyLogin(); break;
			case GAME_RECEIVE_BUDDY_LOGOUT: ReceiveBuddyLogout(); break;
			default:
				TERROR1("unknown packet type during game", Type);
			}
		}
	}
}

// The close handler for InitNetwork. A pending reconnect (event 0x3f) logs in again: to the game server if the
// character was in the game and the 5 seconds have not passed, else to the login server. GCC's analyzer: the message
// queue owns the block it gets as an integer (AddMessage; the message's handler deletes it).
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic push")
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic ignored \"-Wanalyzer-malloc-leak\"")
void patella::TGUI::ConnectionClosed()
{
	const bool InGame = GetConnectionState() == CONNECTION_IN_GAME;
	if (GetConnectionState() == CONNECTION_IN_GAME) {
		LeaveGame();
	} else if (GetConnectionState() == CONNECTION_GAME_LOGIN_SENT) {
		char *Message = new char[1024];
		GetErrorMessage(patella::TSpan<char, ERROR_MESSAGE_SIZE>(Message, ERROR_MESSAGE_SIZE), ERROR_DISCONNECTED, 2);
		AddMessage(1, reinterpret_cast<std::intptr_t>("Connection failed"), reinterpret_cast<std::intptr_t>(Message));
		AddMessage(MESSAGE_CHARACTER_LIST_DIALOG, 300, 0);
	} else if (GetConnectionState() == CONNECTION_LOGIN_SENT) {
		char *Message = new char[1024];
		GetErrorMessage(patella::TSpan<char, ERROR_MESSAGE_SIZE>(Message, ERROR_MESSAGE_SIZE), ERROR_DISCONNECTED, 1);
		AddMessage(1, reinterpret_cast<std::intptr_t>("Connection failed"), reinterpret_cast<std::intptr_t>(Message));
	}
	SetConnectionState(CONNECTION_OFFLINE);
	if (Network.Reconnect) {
		Network.Reconnect = false;
		if (InGame) {
			if (GetMilliseconds() < Network.ReconnectTimeout) {
				AddMessage(MESSAGE_CONNECT_GAME_SERVER, 0, 0);
				AddMessage(MESSAGE_CONNECTING_GAME_DIALOG, 0, 0);
			}
		} else {
			AddMessage(MESSAGE_CONNECT_LOGIN_SERVER, 0, 0);
			AddMessage(MESSAGE_CONNECTING_LOGIN_DIALOG, 0, 0);
		}
	}
}
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic pop")

// The mouse position for the context menus' constructors, the y first: the original's order of their arguments, right
// to left (MSVC; GCC and Clang may differ).
namespace {
struct TMousePosition {
	int x, y;
};
TMousePosition MousePosition()
{
	const int y = GetMouseY();
	return {GetMouseX(), y};
}
} // namespace

// Called by the main frame's timer. Ten ticks after the last mouse move the
// window under the mouse is told (hover). While the left button is held
// (button event 6), it is told again every tick from the seventh tick on;
// before that the function returns early and skips the connection timeout
// check and the two calls after it.
void patella::TGUI::TimerEvent() const
{
	IncrementActionNumber();
	if (LastMouseMove != 0 && LastMouseMove == GetActionNumber() - 10) {
		// The original's order: a call's arguments right to left (MSVC; GCC and Clang may differ).
		const int MouseY = GetMouseY();
		const int MouseX = GetMouseX();
		if (ModalWindow != nullptr)
			ModalWindow->MouseHover(MouseX - ModalWindow->OffsetX, MouseY - ModalWindow->OffsetY);
		else
			MainWindow->MouseHover(MouseX, MouseY);
		if (GetMouseOverText() != nullptr) {
			const int TextY = GetMouseY();
			SetMouseOverTextPosition(GetMouseX(), TextY);
		}
	}
	UpdateTalkboxes();
	UpdateStatusMessage();
	if (GetActionNumber() % 10 == 0)
		UpdateAmbientLight();
	ProcessEffects();
	if (GetPendingActionNumber() != 0 && GetPendingActionState() == 6) {
		if (GetActionNumber() - GetPendingActionNumber() < 7)
			return;
		if (GetPendingActionState() == 6) {
			// The original's order: a call's arguments right to left (MSVC; GCC and Clang may differ).
			const int MouseY = GetMouseY();
			const int MouseX = GetMouseX();
			if (ModalWindow != nullptr)
				ModalWindow->MouseWheel(MouseX - ModalWindow->OffsetX, MouseY - ModalWindow->OffsetY, 6);
			else
				MainWindow->MouseWheel(MouseX, MouseY, 6);
		}
	}
	CheckConnectionTimeout();
	::UpdateTimers();
	UpdateSkulls();
}

// Handles queued events until a window is open, the main window is dragging
// or the queue is empty. Events below 0x34 that open a window open a
// centred dialog (action state 10), 0x34-0x3c open a context menu at the
// mouse (state 11). Event 0x40 (logout) is posted again while connected and
// ends the loop; 0x48 ends the program. Odd cases kept from the binary:
// event 0x2f passes Param2 twice to OpenChannel, and event 0x32 deletes
// Param2 after using Param1.
void patella::TGUI::ProcessEvents()
{
	int Type = 0;
	if (ModalWindow != nullptr && GetActionState() == ACTIONSTATE_NONE) {
		DeleteComponent(ModalWindow);
		ModalWindow = nullptr;
		SetActionState(ACTIONSTATE_NONE);
		MainWindow->SetConsoleActive(true);
		Redraw(false);
	}
	while (MainWindow->GetCapture() == nullptr && ModalWindow == nullptr && PeekMessageType() != 0) {
		std::intptr_t Param1, Param2;
		FetchMessage(&Type, &Param1, &Param2);
		switch (Type) {
		case 0x01:
			ModalWindow = CreateMessageDialog(reinterpret_cast<const char *>(Param1), reinterpret_cast<const char *>(Param2));
			delete[] reinterpret_cast<char *>(Param2);
			break;
		case MESSAGE_FORMATTED_DIALOG:
			ModalWindow = CreateFormattedMessageDialog(reinterpret_cast<const char *>(Param1), reinterpret_cast<const char *>(Param2));
			delete[] reinterpret_cast<char *>(Param2);
			break;
		case MESSAGE_CONNECTING_LOGIN_DIALOG:
			ModalWindow = CreateConnectingDialog("Connecting", TextCharacterListLoading, MainWindow.get());
			break;
		case MESSAGE_CONNECTING_GAME_DIALOG:
			ModalWindow = CreateConnectingDialog("Connecting", TextConnectingGameWorld, MainWindow.get());
			break;
		case MESSAGE_RECONNECT_DIALOG:
			ModalWindow = CreateReconnectDialog("Please wait", reinterpret_cast<const char *>(Param1), (int)Param2);
			delete[] reinterpret_cast<char *>(Param1);
			break;
		case MESSAGE_QUESTION_DIALOG: {
			const std::intptr_t *Data = reinterpret_cast<std::intptr_t *>(Param1);
			ModalWindow = CreateQuestionDialog(reinterpret_cast<const char *>(Data[0]), reinterpret_cast<const char *>(Data[1]), reinterpret_cast<TWindow *>(Data[2]),
				(int)Data[3], EVENT_CANCEL);
			delete[] Data;
			break;
		}
		case MESSAGE_LOGOUT_DIALOG: {
			const std::intptr_t *Data = reinterpret_cast<std::intptr_t *>(Param1);
			ModalWindow = CreateLogoutDialog(reinterpret_cast<const char *>(Data[0]), reinterpret_cast<const char *>(Data[1]), MainWindow.get());
			delete[] Data;
			break;
		}
		case MESSAGE_LOGIN_DIALOG:
			ModalWindow = CreateLoginDialog(URLCallback);
			break;
		case MESSAGE_WAIT_DIALOG:
			ModalWindow = CreateWaitDialog("Please wait", TextWaitForCharacterList, (int)Param1);
			break;
		case MESSAGE_CHARACTER_LIST_DIALOG:
			ModalWindow = CreateCharacterListDialog((int)Param1);
			break;
		case MESSAGE_LINK_COPY_WARNING:
			ModalWindow = CreateQuestionDialog(reinterpret_cast<const char *>(Param1), TextLinkCopyWarning, MainWindow.get(), EVENT_COPY_LINK, EVENT_CANCEL);
			break;
		case MESSAGE_ACCOUNT_DATA_WARNING:
			ModalWindow = CreateMessageDialog(reinterpret_cast<const char *>(Param1), TextNeverGiveAccount);
			break;
		case MESSAGE_MOVE_OBJECTS_DIALOG:
			ModalWindow = CreateMoveObjectsDialog();
			break;
		case MESSAGE_TEXT_DIALOG: {
			const std::intptr_t *Texts = reinterpret_cast<std::intptr_t *>(Param2);
			ModalWindow = CreateTextDialog(reinterpret_cast<int *>(Param1), reinterpret_cast<const char *>(Texts[0]), reinterpret_cast<const char *>(Texts[1]));
			delete[] reinterpret_cast<char *>(Param1);
			delete[] reinterpret_cast<char *>(Texts[0]);
			delete[] reinterpret_cast<char *>(Texts[1]);
			delete[] Texts;
			break;
		}
		case MESSAGE_EDIT_LIST_DIALOG:
			ModalWindow = CreateEditListDialog(reinterpret_cast<int *>(Param1), reinterpret_cast<const char *>(Param2));
			delete[] reinterpret_cast<char *>(Param1);
			delete[] reinterpret_cast<char *>(Param2);
			break;
		case MESSAGE_OUTFIT_DIALOG:
			Communication.OutfitRequested = false;
			ModalWindow = CreateOutfitDialog(reinterpret_cast<int *>(Param1));
			delete[] reinterpret_cast<char *>(Param1);
			break;
		case MESSAGE_BUG_REPORT_DIALOG:
			ModalWindow = CreateBugReportDialog();
			break;
		case MESSAGE_NEW_CHANNEL_DIALOG: {
			// ReceiveChannelList's int[]: the count, then the channel IDs.
			int *Data = reinterpret_cast<int *>(Param1);
			ModalWindow = CreateNewChannelDialog(Data[0], &Data[1], reinterpret_cast<const char (*)[30]>(Param2)); // NOLINT(modernize-avoid-c-arrays): CreateNewChannelDialog's parameter (Dialogs.h)
			delete[] Data;
			if (Param2 != 0)
				delete[] reinterpret_cast<char *>(Param2);
			break;
		}
		case MESSAGE_INVITE_DIALOG:
			ModalWindow = CreateInviteDialog();
			break;
		case MESSAGE_EXCLUDE_DIALOG:
			ModalWindow = CreateExcludeDialog();
			break;
		case MESSAGE_IGNORE_LIST_DIALOG:
			ModalWindow = CreateIgnoreListDialog();
			break;
		case MESSAGE_REPORT_DIALOG:
			ModalWindow = CreateReportDialog();
			break;
		case MESSAGE_ADD_BUDDY_DIALOG:
			ModalWindow = CreateAddBuddyDialog();
			break;
		case MESSAGE_EDIT_BUDDY_DIALOG:
			ModalWindow = CreateEditBuddyDialog((int)Param1);
			break;
		case MESSAGE_TEMPORARY_IGNORE:
			AddTemporaryIgnore(reinterpret_cast<const char *>(Param1));
			delete[] reinterpret_cast<char *>(Param1);
			break;
		case MESSAGE_TEMPORARY_UNIGNORE:
			RemoveTemporaryIgnore(reinterpret_cast<const char *>(Param1));
			delete[] reinterpret_cast<char *>(Param1);
			break;
		case MESSAGE_INFO_DIALOG:
			ModalWindow = CreateInfoDialog(URLCallback);
			break;
		case MESSAGE_RULE_VIOLATION_DIALOG:
			ModalWindow = CreateRuleViolationDialog(reinterpret_cast<const char *>(Param1), reinterpret_cast<TTalkLine *>(Param2));
			if (Param1 != 0)
				delete[] reinterpret_cast<char *>(Param1);
			if (Param2 != 0)
				delete[] reinterpret_cast<char *>(Param2);
			break;
		case MESSAGE_SET_MARK_DIALOG:
			ModalWindow = CreateEditMarkDialog(reinterpret_cast<TCoordinate *>(Param1), nullptr);
			break;
		case MESSAGE_EDIT_MARK_DIALOG:
			ModalWindow = CreateEditMarkDialog(reinterpret_cast<TCoordinate *>(Param1), reinterpret_cast<TMark *>(Param2));
			break;
		case MESSAGE_HELP_DIALOG:
			ModalWindow = CreateHelpDialog(URLCallback);
			break;
		case MESSAGE_HINTS_DIALOG:
			ModalWindow = CreateHintsDialog();
			break;
		case MESSAGE_OPTIONS_DIALOG:
			ModalWindow = CreateOptionsDialog();
			break;
		case MESSAGE_NETWORK_OPTIONS_DIALOG:
			ModalWindow = CreateNetworkOptionsDialog(URLCallback);
			break;
		case MESSAGE_GRAPHICS_OPTIONS_DIALOG:
			ModalWindow = CreateGraphicsOptionsDialog(URLCallback, DisplayModeCallback, MainWindow.get());
			break;
		case MESSAGE_ADVANCED_GRAPHICS_DIALOG:
			ModalWindow = CreateAdvancedGraphicsDialog(URLCallback, reinterpret_cast<TComponent *>(Param1));
			break;
		case MESSAGE_CONSOLE_OPTIONS_DIALOG:
			ModalWindow = CreateConsoleOptionsDialog(URLCallback);
			break;
		case MESSAGE_HOTKEY_DIALOG:
			ModalWindow = CreateHotkeyDialog();
			break;
		case MESSAGE_GENERAL_OPTIONS_DIALOG:
			ModalWindow = CreateGeneralOptionsDialog(URLCallback);
			break;
		case MESSAGE_MESSAGE_OF_THE_DAY:
			ModalWindow = CreateMessageDialog("Message of the Day", GetMessageOfTheDay());
			break;
		case MESSAGE_USE_WITH:
			SetActionState(ACTIONSTATE_USE_WITH);
			break;
		case MESSAGE_TRADE_WITH:
			SetActionState(ACTIONSTATE_TRADE_WITH);
			break;
		case MESSAGE_HELP_MODE:
			SetActionState(ACTIONSTATE_HELP);
			SetHelpHandle(HELP_NONE);
			break;
		case MESSAGE_OPEN_PRIVATE_CONVERSATION:
			MainWindow->Console->OpenChannel(TALK_PRIVATE_MESSAGE, TALK_PRIVATE_MESSAGE, reinterpret_cast<const char *>(Param2), -1, reinterpret_cast<const char *>(Param2));
			delete[] reinterpret_cast<char *>(Param2);
			break;
		case MESSAGE_OPEN_CHANNEL:
			MainWindow->Console->OpenChannel(TALK_CHANNEL_CALL, TALK_CHANNEL_CALL, nullptr, (int)Param1, reinterpret_cast<const char *>(Param2));
			delete[] reinterpret_cast<char *>(Param2);
			break;
		case MESSAGE_OPEN_OWN_CHANNEL:
			MainWindow->Console->OpenChannel(TALK_CHANNEL_CALL, TALK_CHANNEL_CALL, nullptr, (int)Param1, reinterpret_cast<const char *>(Param2))->SetSelected(true);
			delete[] reinterpret_cast<char *>(Param2);
			break;
		case MESSAGE_RULE_VIOLATION_CHANNEL:
			MainWindow->Console->AddChannel(reinterpret_cast<TTalkBuffer *>(Param1), TextRuleViolations);
			delete[] reinterpret_cast<char *>(Param2);
			break;
		case MESSAGE_REPORT_RULE_VIOLATION:
			MainWindow->Console->ReportRuleViolation(reinterpret_cast<const char *>(Param1));
			delete[] reinterpret_cast<char *>(Param1);
			break;
		case MESSAGE_OBJECT_MENU: {
			const TMousePosition Mouse = MousePosition();
			ModalWindow = new TObjectContextMenu(Mouse.x, Mouse.y, MainWindow->Width,
				MainWindow->Height);
			break;
		}
		case MESSAGE_CREATURE_MENU: {
			const TMousePosition Mouse = MousePosition();
			ModalWindow = new TCreatureContextMenu((uint32)Param1, Mouse.x, Mouse.y,
				MainWindow->Width, MainWindow->Height);
			break;
		}
		case MESSAGE_BUDDY_MENU: {
			const TMousePosition Mouse = MousePosition();
			ModalWindow = new TBuddyContextMenu((uint32)Param1, Mouse.x, Mouse.y,
				MainWindow->Width, MainWindow->Height);
			break;
		}
		case MESSAGE_TEXT_MENU: {
			const TMousePosition Mouse = MousePosition();
			ModalWindow = new TTextContextMenu(MainWindow->Console->CurrentConsole->TextWindow, (int)Param1,
				Mouse.x, Mouse.y, MainWindow->Width, MainWindow->Height);
			break;
		}
		case MESSAGE_CHANNEL_MENU: {
			const TMousePosition Mouse = MousePosition();
			ModalWindow = new TChannelContextMenu(MainWindow->Console, Mouse.x, Mouse.y,
				MainWindow->Width, MainWindow->Height);
			break;
		}
		case MESSAGE_REQUEST_MENU: {
			const TMousePosition Mouse = MousePosition();
			ModalWindow = new TRequestContextMenu(MainWindow->Console, (int)Param1, Mouse.x,
				Mouse.y, MainWindow->Width, MainWindow->Height);
			break;
		}
		case MESSAGE_EDIT_MENU: {
			const TMousePosition Mouse = MousePosition();
			ModalWindow = new TEditContextMenu(reinterpret_cast<TEditLine *>(Param1), Mouse.x, Mouse.y,
				MainWindow->Width, MainWindow->Height);
			break;
		}
		case MESSAGE_MARK_MENU: {
			const TMousePosition Mouse = MousePosition();
			ModalWindow = new TMarkContextMenu(reinterpret_cast<TMarkMenuData *>(Param1), Mouse.x, Mouse.y,
				MainWindow->Width, MainWindow->Height);
			break;
		}
		case MESSAGE_SKILL_MENU: {
			const TMousePosition Mouse = MousePosition();
			ModalWindow = new TSkillContextMenu(reinterpret_cast<TSkillsWindowBody *>(Param1), Mouse.x, Mouse.y,
				MainWindow->Width, MainWindow->Height);
			break;
		}
		case MESSAGE_CONNECT_LOGIN_SERVER:
			Redraw(false);
			ConnectToLoginServer();
			break;
		case MESSAGE_CONNECT_GAME_SERVER:
			Redraw(false);
			ConnectToGameServer();
			break;
		case MESSAGE_RECONNECT:
			if (GetConnectionState() != CONNECTION_OFFLINE) {
				Network.Reconnect = true;
				Network.ReconnectTimeout = GetMilliseconds() + 5000;
			} else {
				AddMessage(MESSAGE_CONNECT_LOGIN_SERVER, 0, 0);
				AddMessage(MESSAGE_CONNECTING_LOGIN_DIALOG, 0, 0);
			}
			break;
		default:
			TERROR1("unknown message type", Type);
		}
		if (ModalWindow != nullptr) {
			if (Type < MESSAGE_FIRST_MENU) {
				ModalWindow->SetPosition(MainWindow->Width / 2 - ModalWindow->Width / 2,
					MainWindow->Height / 2 - ModalWindow->Height / 2);
				SetActionState(ACTIONSTATE_DIALOG);
			} else {
				SetActionState(ACTIONSTATE_CONTEXT_MENU);
			}
			MainWindow->SetConsoleActive(false);
		}
	}
	if (GetConnectionState() == CONNECTION_IN_GAME && MoveCreatures())
		SendWalk();
	Redraw(false);
}

// Called when the main frame window is created.
void patella::TGUI::InitWindow(platform::TWindowHandle Window, void (*ExitProgram)(), void (*SetDisplayMode)(int Mode), void (*OpenURL)(const char *URL))
{
	WindowHandle = Window;
	SetNetworkWindow(Window);
	ExitCallback = ExitProgram;
	DisplayModeCallback = SetDisplayMode;
	URLCallback = OpenURL;
	SetMouseCursor(CURSOR_ARROW);
	memset(Communication.LastPacketTypes.data(), 0, sizeof(patella::TCommunication::LastPacketTypes));
	Communication.LastPacketIndex = 0;
	Network.Reconnect = false;
}

// Initialises the modules; on a PROGRAM_ERROR it shuts them down again
// (ignoring any error from that) and passes the error on.
void patella::TGUI::InitControl(bool Gamemaster)
{
	try {
		InitSysinc();
		InitObjects();
		InitMap(HideCreature);
		ResetInventory();
		InitCreatures();
		InitStatus(Gamemaster, ::LinkCopyWarning);
		InitHelp();
		InitPreferences();
		InitTalk();
		InitGraphics(0xa00000);
		InitSkins();
		InitSkinProperties();
		InitNetwork(::ConnectionEstablished, ::ReceivePackets, ::ConnectionClosed);
		InitCommunication();
		::InitHotkeys();
	} catch (const TProgramError &Error) {
		try {
			ExitControl();
		} catch (...) { // NOLINT(bugprone-empty-catch): the original's; the start-up's own error is thrown on
		}
		throw Error;
	}
}

// Called from the main frame's destructor, and from InitControl after a
// failed step. GetSurface creates a surface first if there is none, so one
// may be created only to be deleted. GetSurface makes one only once
// SurfaceInitInstance has run (after InitControl), so after a failed
// InitControl there is none and the surface step is skipped. Tibia.exe's
// GetSurface made a DirectX 5 surface there whose ExitSurfaces faulted on its
// uninitialised members (seen under cdb); its catch wrapper and InitControl's
// catch (...) (MSVC 7.1: an access violation as well) swallowed that, and the
// PROGRAM_ERROR went on to the box.
// Which module each of the five empty Exit functions belongs to follows InitControl's order in reverse (names ours).
void patella::TGUI::ExitControl()
{
	::ExitMainWindow();
	::ExitHotkeys();
	ExitCommunication();
	ExitNetwork();
	ExitSkins();
	ExitGraphics();
	TSurface *const Current = GetSurface();
	if (Current != nullptr)
		Current->ExitSurfaces();
	SurfaceExitInstance();
	ExitPreferences();
	ExitTalk();
	ExitHelp();
	ExitStatus();
	ExitCreatures();
	ExitInventory();
	ExitMap();
	ExitObjects();
	ExitSysinc();
}

// LastPacketTypes, the newest first: the slot before LastPacketIndex, then back round the ring.
std::array<uint8, 10> patella::TGUI::GetLastPacketTypes() const
{
	std::array<uint8, 10> Types{};
	for (std::size_t i = 0; i < Types.size(); i++)
		Types[i] = Communication.LastPacketTypes[(static_cast<std::size_t>(Communication.LastPacketIndex) + 9 - i) % 10];
	return Types;
}

// --- The free functions of the original's names -------------------------------------

// Each forwards to its TGUI member (above): MainFrm.cpp, Player.cpp, Communication.cpp and the test mode call them,
// InitControl passes four of them as callbacks (InitStatus's, InitNetwork's), and the golden tests call and fake them
// by name.

void Resize(int Width, int Height)
{
	patella::bridge::GUI().Resize(Width, Height);
}

void Redraw(bool Force)
{
	patella::bridge::GUI().Redraw(Force);
}

void MouseMove(int x, int y)
{
	patella::bridge::GUI().MouseMove(x, y);
}

void LeftButtonDown(int x, int y, int Flags)
{
	patella::bridge::GUI().LeftButtonDown(x, y, Flags);
}

void LeftButtonUp(int x, int y, int Flags)
{
	patella::bridge::GUI().LeftButtonUp(x, y, Flags);
}

void LeftButtonDoubleClick(int x, int y, int Flags)
{
	patella::bridge::GUI().LeftButtonDoubleClick(x, y, Flags);
}

void RightButtonDown(int x, int y, int Flags)
{
	patella::bridge::GUI().RightButtonDown(x, y, Flags);
}

void RightButtonUp(int x, int y, int Flags)
{
	patella::bridge::GUI().RightButtonUp(x, y, Flags);
}

void MouseWheel(int x, int y, int Direction)
{
	patella::bridge::GUI().MouseWheel(x, y, Direction);
}

void KeyPress(int Key)
{
	patella::bridge::GUI().KeyPress(Key);
}

bool CloseRequest()
{
	return patella::bridge::GUI().CloseRequest();
}

void LinkCopyWarning()
{
	patella::TGUI::LinkCopyWarning();
}

void ConnectionEstablished()
{
	patella::bridge::GUI().ConnectionEstablished();
}

void ReceivePackets()
{
	patella::bridge::GUI().ReceivePackets();
}

void ConnectionClosed()
{
	patella::bridge::GUI().ConnectionClosed();
}

void TimerEvent()
{
	patella::bridge::GUI().TimerEvent();
}

void ProcessEvents()
{
	patella::bridge::GUI().ProcessEvents();
}

void InitWindow(patella::platform::TWindowHandle Window, void (*ExitProgram)(), void (*SetDisplayMode)(int Mode), void (*OpenURL)(const char *URL))
{
	patella::bridge::GUI().InitWindow(Window, ExitProgram, SetDisplayMode, OpenURL);
}

void InitControl(bool Gamemaster)
{
	patella::TGUI::InitControl(Gamemaster);
}

void ExitControl()
{
	patella::TGUI::ExitControl();
}

std::array<uint8, 10> GetLastPacketTypes()
{
	return patella::bridge::GUI().GetLastPacketTypes();
}
// NOLINTEND(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg)
