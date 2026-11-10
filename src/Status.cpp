// Status.cpp: reconstructed from Tibia.exe 7.72.
//
// The functions without line evidence (all of them small accessors) come
// first, in binary order (the error report list's and its vector are gone
// since Patella-Modern issue #74, decision D2); the rest follow the assertion
// and catch line numbers, which are also their binary order.
//
// Of all functions here only CheckClipboardText had a try block (its catch
// wrapper, gone since issue #78); the others have assertions only.

#include "Status.h"
#include "Utils.h"
#include "sysinc.h"
#include "types/Regexp.h"
#include "Objects.h"
#include "common.h"
#include "compat/span.h"
#include "types/FixedString.h"
#include <array>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <vector>

// Issue #111 (clang-tidy, the first look at this file in patella_core): cppcoreguidelines-owning-memory off for the file:
// the original's raw owners (notes/ownership.md): the window tree owns its children, the message queue its blocks,
// the lists their entries
// NOLINTBEGIN(cppcoreguidelines-owning-memory)

// The data is TInteraction's (the user interaction, issue #107) and TSession's (the session, issue #105), both in
// Status.h.

namespace {

// The regular expression of a link (a domain or an IPv4 address), which
// CheckClipboardText looks for in copied text; InitStatus compiles it. The name is ours.
const char *LinkPattern =
	"(^|[ .,:;/@<>|()\"\n\r%])(([a-z0-9-]+(\\.|\\%2e))+(ac|ad|ae|aero|af|ag|ai|al|am|an|ao|aq|a"
	"r|arpa|as|at|au|aw|az|ba|bb|bd|be|bf|bg|bh|bi|biz|bj|bm|bn|bo|br|bs|bt|bv|bw|by|bz|ca|cc|c"
	"d|cf|cg|ch|ci|ck|cl|cm|cn|co|com|coop|cr|cu|cv|cx|cy|cz|de|dj|dk|dm|do|dz|ec|edu|ee|eg|er|"
	"es|et|eu|fi|fj|fk|fm|fo|fr|ga|gb|gd|ge|gf|gg|gh|gi|gl|gm|gn|gov|gp|gq|gr|gs|gt|gu|gw|gy|hk"
	"|hm|hn|hr|ht|hu|id|ie|il|im|in|info|int|io|iq|ir|is|it|je|jm|jo|jobs|jp|ke|kg|kh|ki|km|kn|"
	"kr|kw|ky|kz|la|lb|lc|li|lk|lr|ls|lt|lu|lv|ly|ma|mc|md|mg|mh|mil|mk|ml|mm|mn|mo|mobi|mp|mq|"
	"mr|ms|mt|mu|museum|mv|mw|mx|my|mz|na|name|nc|ne|net|nf|ng|ni|nl|no|np|nr|nu|nz|om|org|pa|p"
	"e|pf|pg|ph|pk|pl|pm|pn|pr|pro|ps|pt|pw|py|qa|re|ro|ru|rw|sa|sb|sc|sd|se|sg|sh|si|sj|sk|sl|"
	"sm|sn|so|sr|st|su|sv|sy|sz|tc|td|tf|tg|th|tj|tk|tl|tm|tn|to|tp|tr|travel|tt|tv|tw|tz|ua|ug"
	"|uk|um|us|uy|uz|va|vc|ve|vg|vi|vn|vu|wf|ws|ye|yt|yu|za|zm|zw)\\.?|[0-9][0-9]?[0-9]?(\\.|\\"
	"%2e)[0-9][0-9]?[0-9]?(\\.|\\%2e)[0-9][0-9]?[0-9]?(\\.|\\%2e)[0-9][0-9]?[0-9]?)([?#& .,:;/@"
	"<>|()\"\r\n%]|$)";

} // namespace

// Only the first 32 rights are checked, the ones GetRightName has names
// for; rights 32-49 never count.
bool patella::TSession::CheckAnyRight() const
{
	for (int i = 0; i < 32; i++) {
		if (Rights[i] != 0)
			return true;
	}
	return false;
}

// Inlined into InitStatus.
void patella::TSession::ClearRights()
{
	for (int i = 0; i < RIGHT_ANY; i++)
		Rights[i] = 0;
}

int patella::TInteraction::GetActionNumber() const
{
	return ActionNumber;
}

void patella::TInteraction::IncrementActionNumber()
{
	ActionNumber++;
}

// Values seen: 1 normal, 6 dragging, 7 use with, 8 trade with, 9, 10 and
// 11 (a modal window has the mouse).
void patella::TInteraction::SetActionState(int State)
{
	ActionState = State;
}

int patella::TInteraction::GetActionState() const
{
	return ActionState;
}

void patella::TInteraction::SetMousePosition(int x, int y)
{
	MouseX = x;
	MouseY = y;
}

int patella::TInteraction::GetMouseX() const
{
	return MouseX;
}

int patella::TInteraction::GetMouseY() const
{
	return MouseY;
}

void patella::TInteraction::SetMouseCursor(int Cursor)
{
	MouseCursor = Cursor;
}

// The action state overrides the cursor that was set.
int patella::TInteraction::GetMouseCursor() const
{
	if (ActionState == ACTIONSTATE_HELP)
		return CURSOR_MAGNIFIER;
	if (ActionState == ACTIONSTATE_DRAGGING)
		return CURSOR_DRAG;
	if (ActionState == ACTIONSTATE_USE_WITH || ActionState == ACTIONSTATE_TRADE_WITH)
		return CURSOR_CROSSHAIR;
	return MouseCursor;
}

void patella::TInteraction::SetPendingAction(int State, int Number)
{
	PendingActionState = State;
	PendingActionNumber = Number;
}

int patella::TInteraction::GetPendingActionState() const
{
	return PendingActionState;
}

int patella::TInteraction::GetPendingActionNumber() const
{
	return PendingActionNumber;
}

void patella::TInteraction::SetWindowSize(int Width, int Height)
{
	WindowWidth = Width;
	WindowHeight = Height;
}

int patella::TInteraction::GetWindowWidth() const
{
	return WindowWidth;
}

int patella::TInteraction::GetWindowHeight() const
{
	return WindowHeight;
}

// Clears the drag object; SetDragObject sets it afterwards.
void patella::TInteraction::SetDragStart(int x, int y)
{
	DragStartX = x;
	DragStartY = y;
	DraggedObject.Type = 0;
	DraggedObject.Data = 0;
	DraggedObject.Data2 = 0;
}

void patella::TInteraction::SetDragObject(TObject Obj, int Index, int x, int y, int z)
{
	DraggedObject = Obj;
	DragIndex = Index;
	DragX = x;
	DragY = y;
	DragZ = z;
}

void patella::TInteraction::SetDragTarget(int x, int y, int z)
{
	DragTargetX = x;
	DragTargetY = y;
	DragTargetZ = z;
}

// Starts dragging once the mouse is more than 3 pixels away from where the
// button went down.
void patella::TInteraction::CheckDragStart(int x, int y)
{
	if (ActionState != ACTIONSTATE_DRAGGING && DraggedObject.Type != 0
		&& (abs(x - DragStartX) > 3 || abs(y - DragStartY) > 3))
		ActionState = ACTIONSTATE_DRAGGING;
}

TObject *patella::TInteraction::GetDragObject(int *Index, int *x, int *y, int *z)
{
	*Index = DragIndex;
	*x = DragX;
	*y = DragY;
	*z = DragZ;
	return &DraggedObject;
}

void patella::TInteraction::GetDragTarget(int *x, int *y, int *z) const
{
	*x = DragTargetX;
	*y = DragTargetY;
	*z = DragTargetZ;
}

void patella::TInteraction::SetTradeObject(TObject Obj, int Index, int x, int y, int z)
{
	TradeObject = Obj;
	TradeIndex = Index;
	TradeX = x;
	TradeY = y;
	TradeZ = z;
}

void patella::TInteraction::SetUseObject(TObject Obj, int Index, int x, int y, int z)
{
	UseObject = Obj;
	UseIndex = Index;
	UseX = x;
	UseY = y;
	UseZ = z;
}

// Set together with the trade and use objects; meaning unknown.
void patella::TInteraction::SetTradeInfo(int Info)
{
	TradeInfo = Info;
}

TObject *patella::TInteraction::GetTradeObject(int *Index, int *x, int *y, int *z)
{
	*Index = TradeIndex;
	*x = TradeX;
	*y = TradeY;
	*z = TradeZ;
	return &TradeObject;
}

TObject *patella::TInteraction::GetUseObject(int *Index, int *x, int *y, int *z)
{
	*Index = UseIndex;
	*x = UseX;
	*y = UseY;
	*z = UseZ;
	return &UseObject;
}

int patella::TInteraction::GetTradeInfo() const
{
	return TradeInfo;
}

// 0 offline, 1 resolving the login server name, 2 connecting to a login server,
// 4 talking to the login server, 5 connecting to the game server; the game uses
// higher values. All are named in Events.h (CONNECTION_*).
void patella::TSession::SetConnectionState(int State)
{
	ConnectionState = State;
}

int patella::TSession::GetConnectionState() const
{
	return ConnectionState;
}

int patella::TSession::GetAccountNumber() const
{
	return AccountNumber;
}

const char *patella::TSession::GetAccountNumberString() const
{
	return AccountNumberString;
}

const char *patella::TSession::GetAccountPassword() const
{
	return AccountPassword;
}

// Replaces the character list with Number empty entries.
void patella::TSession::SetAccountCharacters(int Number)
{
	AccountCharacters = Number;
	if (Characters != nullptr)
		Characters.reset();
	Characters = nullptr;
	if (Number > 0)
		Characters.reset(new TCharacter[Number]);
}

void patella::TSession::SetPremiumDays(int Days)
{
	PremiumDays = Days;
}

int patella::TSession::GetAccountCharacters() const
{
	return AccountCharacters;
}

int patella::TSession::GetPremiumDays() const
{
	return PremiumDays;
}

int patella::TSession::GetActiveCharacter() const
{
	return ActiveCharacter;
}

bool patella::TSession::IsGamemaster() const
{
	return Gamemaster;
}

// The text is not copied.
void patella::TInteraction::SetMouseOverText(const char *Text)
{
	MouseOverText = Text;
}

void patella::TInteraction::SetMouseOverTextPosition(int x, int y)
{
	MouseOverTextX = x;
	MouseOverTextY = y;
}

const char *patella::TInteraction::GetMouseOverText() const
{
	return MouseOverText;
}

void patella::TInteraction::GetMouseOverTextPosition(int *x, int *y) const
{
	*x = MouseOverTextX;
	*y = MouseOverTextY;
}

// Called once per main loop iteration.
void patella::TInteraction::UpdateBlinkState()
{
	const int Now = GetMilliseconds();
	if (BlinkTime + 300 < Now) {
		BlinkState = !BlinkState;
		BlinkTime = Now;
	}
}

bool patella::TInteraction::GetBlinkState() const
{
	return BlinkState;
}

// Type of the first message, 0 when the queue is empty.
int patella::TInteraction::PeekMessageType()
{
	if (Messages.empty())
		return 0;
	return Messages.front().Type;
}

// Removes the first message; all zero when the queue is empty. The
// parameters now belong to the caller.
void patella::TInteraction::FetchMessage(int *Type, std::intptr_t *Param1, std::intptr_t *Param2)
{
	if (!Messages.empty()) {
		*Type = Messages.front().Type;
		*Param1 = Messages.front().Param1;
		*Param2 = Messages.front().Param2;
		Messages.erase(Messages.begin());
		return;
	}
	*Type = 0;
	*Param1 = 0;
	*Param2 = 0;
}

// Frees the parameters that each message type owns. Type 14's Param2
// points to two more blocks.
void patella::TInteraction::ClearMessages()
{
	for (const TMessage &Message : Messages) {
		const int Type = Message.Type;
		void *Param1 = reinterpret_cast<void *>(Message.Param1);
		void *Param2 = reinterpret_cast<void *>(Message.Param2);
		switch (Type) {
		case 1:
		case 2:
		case 47:
		case 48:
		case 49:
		case 50:
			delete[] static_cast<char *>(Param2);
			break;
		case 5:
		case 6:
		case 7:
		case 16:
		case 25:
		case 26:
		case 51:
			delete[] static_cast<char *>(Param1);
			break;
		case 14:
			delete[] static_cast<char *>(Param1);
			delete[] (static_cast<char **>(Param2))[0];
			delete[] (static_cast<char **>(Param2))[1];
			delete[] static_cast<char *>(Param2);
			break;
		case 15:
			delete[] static_cast<char *>(Param1);
			delete[] static_cast<char *>(Param2);
			break;
		case 18:
			delete[] static_cast<char *>(Param1);
			if (Param2 != nullptr)
				delete[] static_cast<char *>(Param2);
			break;
		case 28:
			if (Param1 != nullptr)
				delete[] static_cast<char *>(Param1);
			if (Param2 != nullptr)
				delete[] static_cast<char *>(Param2);
			break;
		default:
			break;
		}
	}
	Messages.clear();
}

// TaintedClipboardText is freed but not cleared, and the queued messages are
// not freed. The original cleared the error reports first (gone since issue
// #74). LinkRegexp is freed and, since issue #129 (TRegexp), cleared too:
// the original left it dangling until InitStatus.
void patella::TInteraction::ExitStatus()
{
	patella::bridge::Session().ExitSession(); // the character list (issue #105)
	if (TaintedClipboardText != nullptr)
		delete[] TaintedClipboardText;
	LinkRegexp.Reset();
}

void patella::TSession::SetRight(int rightid, int Value)
{
	PATELLA_ASSERT_VALUES(In(rightid,0,RIGHT_ANY-1), rightid);
	Rights[rightid] = Value;
}

// Bit -1 asks for any bit. The RIGHT_ANY branch cannot be reached after
// the assertion; it is in the binary.
bool patella::TSession::CheckRight(int rightid, int Bit) const
{
	PATELLA_ASSERT_VALUES(In(rightid,0,RIGHT_ANY-1), rightid);
	if (rightid == RIGHT_ANY)
		return CheckAnyRight();
	if (Bit == -1)
		return Rights[rightid] != 0;
	return (Rights[rightid] & (1 << Bit)) != 0;
}

// Reasons of the gamemaster report dialog.
const char *GetRightName(int rightid)
{
	switch (rightid) {
	case 0: return "1a) Offensive name";
	case 1: return "1b) Name containing part of sentence";
	case 2: return "1b) Name with nonsensical letter combination";
	case 3: return "1b) Invalid name format";
	case 4: return "1c) Name not describing person";
	case 5: return "1c) Name of celebrity";
	case 6: return "1c) Name referring to country";
	case 7: return "1c) Name to fake player identity";
	case 8: return "1c) Name to fake official position";
	case 9: return "2a) Offensive statement";
	case 10: return "2b) Spamming";
	case 11: return "2c) Advertisement not related to game";
	case 12: return "2c) Real money advertisement";
	case 13: return "2d) Non-English public statement";
	case 14: return "2d) Off-topic public statement";
	case 15: return "2e) Inciting rule violation";
	case 16: return "3a) Bug abuse";
	case 17: return "3b) Game weakness abuse";
	case 18: return "3c) Macro use";
	case 19: return "3d) Using unofficial software to play";
	case 20: return "3e) Hacking";
	case 21: return "3f) Multi-clienting";
	case 22: return "3g) Account trading";
	case 23: return "3g) Account sharing";
	case 24: return "4a) Threatening gamemaster";
	case 25: return "4b) Pretending to have official position";
	case 26: return "4b) Pretending to have influence on gamemaster";
	case 27: return "4c) False report to gamemaster";
	case 28: return "Excessive unjustified player killing";
	case 29: return "Destructive behaviour";
	case 30: return "Spoiling auction";
	case 31: return "Invalid payment";
	default:
		TERROR1("unknown right", rightid);
	}
}

const char *GetActionName(int actionid)
{
	switch (actionid) {
	case 0: return "Notation";
	case 1: return "Name Report";
	case 2: return "Banishment";
	case 3: return "Name Report + Banishment";
	case 4: return "Banishment + Final Warning";
	case 5: return "Name Report + Banishment + Final Warning";
	case 6: return "Statement Report";
	case 7: return "IP Banishment";
	default:
		TERROR1("unknown action", actionid);
	}
}

// Appends a message; the queue takes ownership of the parameters that
// ClearMessages frees.
void patella::TInteraction::AddMessage(int Type, std::intptr_t Param1, std::intptr_t Param2)
{
	PATELLA_ASSERT_VALUES(In(Type,MESSAGE_NONE,MESSAGE_STARTPATCHER), Type);
	Messages.push_back({Type, Param1, Param2});
}

// Inserts a message at the front of the queue.
void patella::TInteraction::AddMessageFirst(int Type, std::intptr_t Param1, std::intptr_t Param2)
{
	PATELLA_ASSERT_VALUES(In(Type,MESSAGE_NONE,MESSAGE_STARTPATCHER), Type);
	Messages.insert(Messages.begin(), {Type, Param1, Param2});
}

void patella::TSession::SetAccount(int Number, const char *Password)
{
	PATELLA_ASSERT(Password!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(Password)<30, strlen(Password));
	AccountNumber = Number;
	TStringBuffer(AccountNumberString, sizeof(AccountNumberString)).Format("{}", Number); // at most 11 characters
	AccountNumberString[11] = 0;
	// The original's copy, up to the buffer's 30 bytes (the assertion keeps the password shorter; GCC's
	// -Wstringop-truncation sees the bound only, issue #111).
	PATELLA_GCC_DIAGNOSTIC("GCC diagnostic push")
	PATELLA_GCC_DIAGNOSTIC("GCC diagnostic ignored \"-Wstringop-truncation\"")
	strncpy(AccountPassword, Password, 30);
	PATELLA_GCC_DIAGNOSTIC("GCC diagnostic pop")
}

// Address is the IPv4 address in network order.
void patella::TSession::SetCharacter(int Number, const char *Name, const char *World, const uint8 *Address, int Port)
{
	PATELLA_ASSERT_VALUES(In(Number,0,AccountCharacters-1), Number);
	PATELLA_ASSERT(Name!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(Name)<30, strlen(Name));
	PATELLA_ASSERT(World!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(World)<30, strlen(World));
	TCharacter *Character = &Characters[Number];
	Character->Name.Assign(Name);
	Character->World.Assign(World);
	Character->IP = *reinterpret_cast<const uint32 *>(Address);
	TStringBuffer(Character->IPAddress, sizeof(Character->IPAddress))
		.Format("{}.{}.{}.{}", Address[0], Address[1], Address[2], Address[3]); // at most 15 characters
	Character->IPAddress[15] = 0;
	Character->Port = Port;
}

void patella::TSession::SetActiveCharacter(int Number)
{
	PATELLA_ASSERT_VALUES(In(Number,0,AccountCharacters-1), Number);
	ActiveCharacter = Number;
}

const char *patella::TSession::GetCharacterName(int Number) const
{
	PATELLA_ASSERT_VALUES(In(Number,0,AccountCharacters-1), Number);
	return Characters[Number].Name;
}

const char *patella::TSession::GetCharacterWorld(int Number) const
{
	PATELLA_ASSERT_VALUES(In(Number,0,AccountCharacters-1), Number);
	return Characters[Number].World;
}

const char *patella::TSession::GetCharacterIPAddress(int Number) const
{
	PATELLA_ASSERT_VALUES(In(Number,0,AccountCharacters-1), Number);
	return Characters[Number].IPAddress;
}

int patella::TSession::GetCharacterPort(int Number) const
{
	PATELLA_ASSERT_VALUES(In(Number,0,AccountCharacters-1), Number);
	return Characters[Number].Port;
}

// Keeps a copy of the text; if its lower-case form contains something like a web or IP address, the
// link warning handler is called instead of copying (it can copy later with CopyTaintedClipboardText).
// Text and Length are the span's pointer and its size as an int (issue #69).
void patella::TInteraction::CheckClipboardText(patella::TSpan<const char> Characters)
{
	const char *const Text = Characters.data();
	const int Length = static_cast<int>(Characters.size());
	if (TaintedClipboardText != nullptr)
		delete[] TaintedClipboardText;
	TaintedClipboardText = new char[Length + 1];
	memcpy(TaintedClipboardText, Text, Length);
	TaintedClipboardText[Length] = 0;
	char *Lower = new char[Length + 1];
	memcpy(Lower, TaintedClipboardText, Length + 1);
	LowerString(Lower);
	const int Match = LinkRegexp.Execute(Lower);
	delete[] Lower;
	if (Match == 1) {
		LinkWarningEventHandler();
		return;
	}
	SetClipboardText(TaintedClipboardText);
}

void patella::TInteraction::CopyToClipboard(const char *text)
{
	PATELLA_ASSERT(text);
	CheckClipboardText(patella::TSpan<const char>(text, strlen(text)));
}

void patella::TInteraction::CopyTaintedClipboardText()
{
	PATELLA_ASSERT(TaintedClipboardText);
	SetClipboardText(TaintedClipboardText);
}

// Not everything is reset: the mouse, window and drag positions keep their
// values.
void patella::TInteraction::InitStatus(bool Gamemaster, void (*linkwarningeventhandler)())
{
	PATELLA_ASSERT(linkwarningeventhandler!=nullptr);
	LinkWarningEventHandler = linkwarningeventhandler;
	TaintedClipboardText = nullptr;
	// The original set LinkRegexp to NULL first, so a second InitStatus without ExitStatus leaked the program;
	// TRegexp's assignment frees it (issue #129).
	LinkRegexp = patella::TRegexp(LinkPattern);
	patella::bridge::Session().InitSession(Gamemaster); // the session's part (issue #105), in its order below
	Messages.clear();                                   // the parameters not freed, as the original
	ActionNumber = 0;
	DraggedObject.Type = 0;
	DraggedObject.Data = 0;
	DraggedObject.Data2 = 0;
	TradeObject.Type = 0;
	TradeObject.Data = 0;
	TradeObject.Data2 = 0;
	UseObject.Type = 0;
	UseObject.Data = 0;
	UseObject.Data2 = 0;
	TradeInfo = 0;
	ActionState = ACTIONSTATE_NONE;
	MouseOverText = nullptr;
	PendingActionState = 0;
	PendingActionNumber = 0;
}

// InitStatus's part (issue #105): the session's members in the order InitStatus wrote them (Gamemaster and
// the rights first, ClearRights inlined there; the rest after its other data).
void patella::TSession::InitSession(bool Value)
{
	Gamemaster = Value;
	ClearRights();
	ConnectionState = CONNECTION_OFFLINE;
	AccountNumber = 0;
	AccountPassword[0] = 0;
	AccountCharacters = 0;
	Characters = nullptr;
	PremiumDays = 0;
	ActiveCharacter = 0;
}

// ExitStatus's part (issue #105): the character list, freed first.
void patella::TSession::ExitSession()
{
	if (Characters != nullptr)
		Characters.reset();
	Characters = nullptr;
}

// The session's functions of the original (issue #105): each calls its member on the one TClient's TSession.

bool CheckAnyRight()
{
	return patella::bridge::Session().CheckAnyRight();
}

void SetConnectionState(int State)
{
	patella::bridge::Session().SetConnectionState(State);
}

int GetConnectionState()
{
	return patella::bridge::Session().GetConnectionState();
}

int GetAccountNumber()
{
	return patella::bridge::Session().GetAccountNumber();
}

const char *GetAccountNumberString()
{
	return patella::bridge::Session().GetAccountNumberString();
}

const char *GetAccountPassword()
{
	return patella::bridge::Session().GetAccountPassword();
}

int GetAccountCharacters()
{
	return patella::bridge::Session().GetAccountCharacters();
}

int GetPremiumDays()
{
	return patella::bridge::Session().GetPremiumDays();
}

int GetActiveCharacter()
{
	return patella::bridge::Session().GetActiveCharacter();
}

bool CheckRight(int rightid, int Bit)
{
	return patella::bridge::Session().CheckRight(rightid, Bit);
}

void SetAccount(int Number, const char *Password)
{
	patella::bridge::Session().SetAccount(Number, Password);
}

void SetActiveCharacter(int Number)
{
	patella::bridge::Session().SetActiveCharacter(Number);
}

const char *GetCharacterName(int Number)
{
	return patella::bridge::Session().GetCharacterName(Number);
}

const char *GetCharacterWorld(int Number)
{
	return patella::bridge::Session().GetCharacterWorld(Number);
}

const char *GetCharacterIPAddress(int Number)
{
	return patella::bridge::Session().GetCharacterIPAddress(Number);
}

int GetCharacterPort(int Number)
{
	return patella::bridge::Session().GetCharacterPort(Number);
}

// The functions of the original (issue #107): each calls its member on the one TClient's TInteraction.

int GetActionNumber()
{
	return patella::bridge::Interaction().GetActionNumber();
}

void IncrementActionNumber()
{
	patella::bridge::Interaction().IncrementActionNumber();
}

void SetActionState(int State)
{
	patella::bridge::Interaction().SetActionState(State);
}

int GetActionState()
{
	return patella::bridge::Interaction().GetActionState();
}

void SetMousePosition(int x, int y)
{
	patella::bridge::Interaction().SetMousePosition(x, y);
}

int GetMouseX()
{
	return patella::bridge::Interaction().GetMouseX();
}

int GetMouseY()
{
	return patella::bridge::Interaction().GetMouseY();
}

void SetMouseCursor(int Cursor)
{
	patella::bridge::Interaction().SetMouseCursor(Cursor);
}

int GetMouseCursor()
{
	return patella::bridge::Interaction().GetMouseCursor();
}

void SetPendingAction(int State, int Number)
{
	patella::bridge::Interaction().SetPendingAction(State, Number);
}

int GetPendingActionState()
{
	return patella::bridge::Interaction().GetPendingActionState();
}

int GetPendingActionNumber()
{
	return patella::bridge::Interaction().GetPendingActionNumber();
}

void SetWindowSize(int Width, int Height)
{
	patella::bridge::Interaction().SetWindowSize(Width, Height);
}

int GetWindowWidth()
{
	return patella::bridge::Interaction().GetWindowWidth();
}

int GetWindowHeight()
{
	return patella::bridge::Interaction().GetWindowHeight();
}

void SetDragStart(int x, int y)
{
	patella::bridge::Interaction().SetDragStart(x, y);
}

void SetDragObject(TObject Obj, int Index, int x, int y, int z)
{
	patella::bridge::Interaction().SetDragObject(Obj, Index, x, y, z);
}

void SetDragTarget(int x, int y, int z)
{
	patella::bridge::Interaction().SetDragTarget(x, y, z);
}

void CheckDragStart(int x, int y)
{
	patella::bridge::Interaction().CheckDragStart(x, y);
}

TObject *GetDragObject(int *Index, int *x, int *y, int *z)
{
	return patella::bridge::Interaction().GetDragObject(Index, x, y, z);
}

void GetDragTarget(int *x, int *y, int *z)
{
	patella::bridge::Interaction().GetDragTarget(x, y, z);
}

void SetTradeObject(TObject Obj, int Index, int x, int y, int z)
{
	patella::bridge::Interaction().SetTradeObject(Obj, Index, x, y, z);
}

void SetUseObject(TObject Obj, int Index, int x, int y, int z)
{
	patella::bridge::Interaction().SetUseObject(Obj, Index, x, y, z);
}

void SetTradeInfo(int Info)
{
	patella::bridge::Interaction().SetTradeInfo(Info);
}

TObject *GetTradeObject(int *Index, int *x, int *y, int *z)
{
	return patella::bridge::Interaction().GetTradeObject(Index, x, y, z);
}

TObject *GetUseObject(int *Index, int *x, int *y, int *z)
{
	return patella::bridge::Interaction().GetUseObject(Index, x, y, z);
}

int GetTradeInfo()
{
	return patella::bridge::Interaction().GetTradeInfo();
}

void SetMouseOverText(const char *Text)
{
	patella::bridge::Interaction().SetMouseOverText(Text);
}

void SetMouseOverTextPosition(int x, int y)
{
	patella::bridge::Interaction().SetMouseOverTextPosition(x, y);
}

const char *GetMouseOverText()
{
	return patella::bridge::Interaction().GetMouseOverText();
}

void GetMouseOverTextPosition(int *x, int *y)
{
	patella::bridge::Interaction().GetMouseOverTextPosition(x, y);
}

void UpdateBlinkState()
{
	patella::bridge::Interaction().UpdateBlinkState();
}

bool GetBlinkState()
{
	return patella::bridge::Interaction().GetBlinkState();
}

int PeekMessageType()
{
	return patella::bridge::Interaction().PeekMessageType();
}

void FetchMessage(int *Type, std::intptr_t *Param1, std::intptr_t *Param2)
{
	patella::bridge::Interaction().FetchMessage(Type, Param1, Param2);
}

void ClearMessages()
{
	patella::bridge::Interaction().ClearMessages();
}

void ExitStatus()
{
	patella::bridge::Interaction().ExitStatus();
}

void AddMessage(int Type, std::intptr_t Param1, std::intptr_t Param2)
{
	patella::bridge::Interaction().AddMessage(Type, Param1, Param2);
}

void AddMessageFirst(int Type, std::intptr_t Param1, std::intptr_t Param2)
{
	patella::bridge::Interaction().AddMessageFirst(Type, Param1, Param2);
}

void CheckClipboardText(patella::TSpan<const char> Characters)
{
	patella::bridge::Interaction().CheckClipboardText(Characters);
}

void CopyToClipboard(const char *text)
{
	patella::bridge::Interaction().CopyToClipboard(text);
}

void CopyTaintedClipboardText()
{
	patella::bridge::Interaction().CopyTaintedClipboardText();
}

void InitStatus(bool Gamemaster, void (*linkwarningeventhandler)())
{
	patella::bridge::Interaction().InitStatus(Gamemaster, linkwarningeventhandler);
}
// NOLINTEND(cppcoreguidelines-owning-memory)
