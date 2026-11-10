// Status.cpp: the client's shared state. Account, character list and
// connection state; the message queue that the network code and the GUI use
// to talk to the main loop; mouse and drag state; the player's rights; the
// clipboard link check. (The error reports read from error.txt are gone
// since Patella-Modern issue #74, decision D2.)
//
// Names are ours unless a comment says they come from the binary.
//
// Patella-Modern issue #105 (M7, notes/decisions.md, "#104: TClient and the subsystem objects" and "#105"): the
// session (the connection state, the account, the character list, the rights) is TSession's, which TCommunication
// owns (Communication.h); the free functions of the original's names are the bridge that the callers use until they
// hold the object (bridge::Session(), defined in Client.cpp). Issue #107: the rest of the file's data, the user
// interaction (the action, drag, trade and use state, the mouse, the message queue, the blink, the mouse-over text, the
// clipboard's link check), is TInteraction's, which TGUI owns (GUI.h); bridge::Interaction() the same way.

#pragma once

#include "common.h"
#include "compat/span.h"
#include "types/FixedString.h"
#include "Objects.h" // TObject
#include "types/Regexp.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <vector>

// "In(rightid,0,RIGHT_ANY-1)"; CheckRight treats RIGHT_ANY as "any right".
inline constexpr int RIGHT_ANY = 50;

// "In(Type,MESSAGE_NONE,MESSAGE_STARTPATCHER)". Other values seen: 1 shows
// a message box (Param1 title, Param2 text, which the queue owns), 10 with
// Param1 300 follows a failed game server connection.
// The other numbers' names: Events.h.
inline constexpr int MESSAGE_NONE = 0;
inline constexpr int MESSAGE_STARTPATCHER = 72;

// One queued message. Type is named in the assertions.
struct TMessage {
	int Type;
	std::intptr_t Param1;
	std::intptr_t Param2;
};

// One entry of the character list. The 30-byte limits come
// from "strlen(Name)<30" and "strlen(World)<30".
struct TCharacter {
	TFixedString<30> Name;
	TFixedString<30> World;
	uint32 IP;                  // as received
	TFixedString<16> IPAddress; // dotted text
	int Port;
};

namespace patella {

// Status.cpp's session (issue #105): the globals of the original are the members, in its order
// with the same initial values (0, as the original's .bss), and the functions that read and write only them the
// members of the same names. InitSession and ExitSession are InitStatus's and ExitStatus's part of them (not functions
// of the original: InitStatus inlined ClearRights).
class TSession {
public:
	TSession() = default;
	TSession(const TSession &) = delete;
	TSession &operator=(const TSession &) = delete;
	TSession(TSession &&) = delete;
	TSession &operator=(TSession &&) = delete;
	~TSession() = default;

	[[nodiscard]] bool CheckAnyRight() const;
	void ClearRights();
	void SetConnectionState(int State);
	[[nodiscard]] int GetConnectionState() const;
	[[nodiscard]] int GetAccountNumber() const;
	[[nodiscard]] const char *GetAccountNumberString() const;
	[[nodiscard]] const char *GetAccountPassword() const;
	void SetAccountCharacters(int Number);
	void SetPremiumDays(int Days);
	[[nodiscard]] int GetAccountCharacters() const;
	[[nodiscard]] int GetPremiumDays() const;
	[[nodiscard]] int GetActiveCharacter() const;
	[[nodiscard]] bool IsGamemaster() const;
	void SetRight(int rightid, int Value);
	[[nodiscard]] bool CheckRight(int rightid, int Bit) const;
	void SetAccount(int Number, const char *Password);
	void SetCharacter(int Number, const char *Name, const char *World, const uint8 *Address, int Port);
	void SetActiveCharacter(int Number);
	[[nodiscard]] const char *GetCharacterName(int Number) const;
	[[nodiscard]] const char *GetCharacterWorld(int Number) const;
	[[nodiscard]] const char *GetCharacterIPAddress(int Number) const;
	[[nodiscard]] int GetCharacterPort(int Number) const;
	void InitSession(bool Value);
	void ExitSession();

private:
	// Data, in the original's .bss order.
	std::array<int, RIGHT_ANY> Rights{};
	bool Gamemaster{}; // sent in the game login packet
	int PremiumDays{};
	int ActiveCharacter{};
	std::unique_ptr<TCharacter[]> Characters; // issue #58

public:
	int AccountCharacters{}; // name from an assertion; public: the golden Errors test sets it (TestErrorStatics)

private:
	TFixedString<30> AccountPassword{};
	TFixedString<12> AccountNumberString{};
	int AccountNumber{};
	int ConnectionState{}; // see SetConnectionState

	// SetAccount copies up to 30 bytes of the password and ends the number's text at its twelfth byte.
	static_assert(sizeof(AccountPassword) == 30 && sizeof(AccountNumberString) == 12, "SetAccount's buffers");
};

// Status.cpp's user interaction (issue #107): the globals that are not the session's are the
// members, in the original's .bss order with the same initial values (0, as its .bss), and the functions that read and
// write them the members of the same names. TGUI owns it (GUI.h). GetRightName and GetActionName read no data and
// stay free functions.
class TInteraction {
public:
	TInteraction() = default;
	TInteraction(const TInteraction &) = delete;
	TInteraction &operator=(const TInteraction &) = delete;
	TInteraction(TInteraction &&) = delete;
	TInteraction &operator=(TInteraction &&) = delete;
	~TInteraction() = default;

	[[nodiscard]] int GetActionNumber() const;
	void IncrementActionNumber();
	void SetActionState(int State);
	[[nodiscard]] int GetActionState() const;
	void SetMousePosition(int x, int y);
	[[nodiscard]] int GetMouseX() const;
	[[nodiscard]] int GetMouseY() const;
	void SetMouseCursor(int Cursor);
	[[nodiscard]] int GetMouseCursor() const;
	void SetPendingAction(int State, int Number);
	[[nodiscard]] int GetPendingActionState() const;
	[[nodiscard]] int GetPendingActionNumber() const;
	void SetWindowSize(int Width, int Height);
	[[nodiscard]] int GetWindowWidth() const;
	[[nodiscard]] int GetWindowHeight() const;
	void SetDragStart(int x, int y);
	void SetDragObject(TObject Obj, int Index, int x, int y, int z);
	void SetDragTarget(int x, int y, int z);
	void CheckDragStart(int x, int y);
	TObject *GetDragObject(int *Index, int *x, int *y, int *z);
	void GetDragTarget(int *x, int *y, int *z) const;
	void SetTradeObject(TObject Obj, int Index, int x, int y, int z);
	void SetUseObject(TObject Obj, int Index, int x, int y, int z);
	void SetTradeInfo(int Info);
	TObject *GetTradeObject(int *Index, int *x, int *y, int *z);
	TObject *GetUseObject(int *Index, int *x, int *y, int *z);
	[[nodiscard]] int GetTradeInfo() const;
	void SetMouseOverText(const char *Text);
	void SetMouseOverTextPosition(int x, int y);
	[[nodiscard]] const char *GetMouseOverText() const;
	void GetMouseOverTextPosition(int *x, int *y) const;
	void UpdateBlinkState();
	[[nodiscard]] bool GetBlinkState() const;
	int PeekMessageType();
	void FetchMessage(int *Type, std::intptr_t *Param1, std::intptr_t *Param2);
	void ClearMessages();
	void ExitStatus();
	void AddMessage(int Type, std::intptr_t Param1, std::intptr_t Param2);
	void AddMessageFirst(int Type, std::intptr_t Param1, std::intptr_t Param2);
	void CheckClipboardText(TSpan<const char> Characters);
	void CopyToClipboard(const char *text);
	void CopyTaintedClipboardText();
	void InitStatus(bool Gamemaster, void (*linkwarningeventhandler)());

private:
	// Data, in the original's .bss order.
	TRegexp LinkRegexp;                  // LinkPattern compiled (issue #129: owned, TRegexp)
	char *TaintedClipboardText{};        // name from an assertion
	void (*LinkWarningEventHandler)(){}; // from "linkwarningeventhandler!=NULL"
	bool BlinkState{};                   // toggled every 300 ms
	int BlinkTime{};
	int MouseOverTextY{};
	int MouseOverTextX{};
	const char *MouseOverText{};
	int TradeInfo{};
	int UseZ{};
	int UseY{};
	int UseX{};
	int UseIndex{};
	int TradeZ{};
	int TradeY{};
	int TradeX{};
	int TradeIndex{};
	int DragTargetZ{};
	int DragTargetY{};
	int DragTargetX{};
	int DragZ{};
	int DragY{};
	int DragX{};
	int DragIndex{};
	int DragStartY{};
	int DragStartX{};
	int PendingActionNumber{};
	int PendingActionState{};
	int WindowHeight{};
	int WindowWidth{};
	int MouseCursor{};
	int ActionState{}; // from "GetActionState()" in Control.cpp
	int ActionNumber{};
	// NumberOfMessages and the original's vector<TMessage>(0, 9, 10): a
	// queue that appends at its count (AddMessage), inserts at 0 (AddMessageFirst), pops at 0 and reads below the
	// count only, so a std::vector, its size the count (issue #100; notes/decisions.md, "#100").
	std::vector<TMessage> Messages;
	int MouseX{};
	int MouseY{};
	TObject DraggedObject{};
	TObject TradeObject{}; // from "TradeObject!=0" in MapWindow.cpp
	TObject UseObject{};   // "SrcObj" in MapWindow.cpp
};

// The transition's access path (issue #105; notes/decisions.md, "#104: TClient and the subsystem objects"): the
// session of the one TClient's TCommunication (Client.cpp). It goes when no free function needs it.
namespace bridge {
TSession &Session();
TInteraction &Interaction(); // issue #107: the one TClient's TGUI's
} // namespace bridge

} // namespace patella

// The session's functions (issue #105): each calls TSession's member of the same name on bridge::Session(); the
// others are Status.cpp's own.
bool CheckAnyRight();
int GetActionNumber();
void IncrementActionNumber();
// The action states. ACTIONSTATE_NONE, no action, is the original's name
// (MainWindow.cpp's "GetActionState()==ACTIONSTATE_NONE"); the other states
// are named from what the code does with them. 4 and 5: both buttons were
// down (classic control) and one has gone up; the release of the other
// sends the left button up of BOTH_BUTTONS_RELEASED.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants, enumerators for the reason in GUI.h
	ACTIONSTATE_NONE = 0,
	ACTIONSTATE_LEFT_BUTTON_DOWN = 1, // on the main window; moving the mouse may start a drag (CheckDragStart)
	ACTIONSTATE_RIGHT_BUTTON_DOWN = 2,
	ACTIONSTATE_BOTH_BUTTONS_DOWN = 3,
	ACTIONSTATE_RIGHT_RELEASED = 4, // the left button is still down
	ACTIONSTATE_LEFT_RELEASED = 5,  // the right button is still down
	ACTIONSTATE_DRAGGING = 6,
	ACTIONSTATE_USE_WITH = 7,   // the player chooses what to use the use object on (message 0x2c)
	ACTIONSTATE_TRADE_WITH = 8, // the player chooses whom to trade with (message 0x2d)
	ACTIONSTATE_HELP = 9,
	ACTIONSTATE_DIALOG = 10,       // a centred dialog is the modal window
	ACTIONSTATE_CONTEXT_MENU = 11, // a context menu is the modal window
};
void SetActionState(int State);
int GetActionState();
void SetMousePosition(int x, int y);
int GetMouseX();
int GetMouseY();
// The mouse cursors of SetMouseCursor and GetMouseCursor. The number is also the skin the client draws as its own
// cursor (Control.cpp) and chooses the Windows cursor otherwise (SdlFrame.cpp's UpdateMouseCursor), whose resource
// identifiers the names follow: IDC_ARROW, IDC_SIZENS, IDC_WAIT, and res/resource.h's IDC_CROSSHAIR and IDC_MAGNIFIER.
// GetMouseCursor gives the last three for an action state, whatever was set. Enumerators for the reason in GUI.h.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	CURSOR_ARROW = 1,
	CURSOR_SIZENS = 2,    // the vertical size arrow of the splitter and the resizers
	CURSOR_WAIT = 3,      // never set
	CURSOR_CROSSHAIR = 4, // the player chooses what to use an object on or whom to trade with
	CURSOR_DRAG = 5,      // dragging: the crosshair's picture, as skin and as Windows cursor
	CURSOR_MAGNIFIER = 6, // help mode
};
void SetMouseCursor(int Cursor);
int GetMouseCursor();
void SetPendingAction(int State, int Number);
int GetPendingActionState();
int GetPendingActionNumber();
void SetWindowSize(int Width, int Height);
int GetWindowWidth();
int GetWindowHeight();
void SetDragStart(int x, int y);
void SetDragObject(TObject Obj, int Index, int x, int y, int z);
void SetDragTarget(int x, int y, int z);
void CheckDragStart(int x, int y);
TObject *GetDragObject(int *Index, int *x, int *y, int *z);
void GetDragTarget(int *x, int *y, int *z);
void SetTradeObject(TObject Obj, int Index, int x, int y, int z);
void SetUseObject(TObject Obj, int Index, int x, int y, int z);
void SetTradeInfo(int Info);
TObject *GetTradeObject(int *Index, int *x, int *y, int *z);
TObject *GetUseObject(int *Index, int *x, int *y, int *z);
int GetTradeInfo();
void SetConnectionState(int State);
int GetConnectionState();
int GetAccountNumber();
const char *GetAccountNumberString();
const char *GetAccountPassword();
int GetAccountCharacters();
int GetPremiumDays();
int GetActiveCharacter();
void SetMouseOverText(const char *Text);
void SetMouseOverTextPosition(int x, int y);
const char *GetMouseOverText();
void GetMouseOverTextPosition(int *x, int *y);
void UpdateBlinkState();
bool GetBlinkState();
int PeekMessageType();
void FetchMessage(int *Type, std::intptr_t *Param1, std::intptr_t *Param2);
void ClearMessages();
void ExitStatus();
bool CheckRight(int rightid, int Bit);
const char *GetRightName(int rightid);
const char *GetActionName(int actionid);
void AddMessage(int Type, std::intptr_t Param1, std::intptr_t Param2);
void AddMessageFirst(int Type, std::intptr_t Param1, std::intptr_t Param2);
void SetAccount(int Number, const char *Password);
void SetActiveCharacter(int Number);
const char *GetCharacterName(int Number);
const char *GetCharacterWorld(int Number);
const char *GetCharacterIPAddress(int Number);
int GetCharacterPort(int Number);

void CheckClipboardText(patella::TSpan<const char> Characters); // the copied characters, no NUL needed
void CopyToClipboard(const char *text);
void CopyTaintedClipboardText();
void InitStatus(bool Gamemaster, void (*linkwarningeventhandler)());

// Events.h, the message and event numbers, comes last on purpose: included at the top of this file, its enumerators
// moved data in the executable (measured; the Release build is compared byte for byte).
#include "Events.h"
