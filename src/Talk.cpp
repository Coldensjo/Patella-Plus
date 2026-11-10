// Talk.cpp: reconstructed from Tibia.exe 7.72, in assertion/catch line order.
// Header COMDATs are in Talk.h; Container.h supplies the filter-list vector.
#include "Talk.h"
#include "Text.h"
#include "Utils.h"
#include "graphics.h"
#include "sysinc.h"
#include "types/IndexedVector.h"
#include "Preferences.h"
#include "common.h"
#include "platform/Platform.h"
#include "types/File.h"
#include "types/FixedString.h"
#include "types/Time32.h"
#include "compat/crt71.h"
#include <ctime>
#include <array>
#include <cstddef>
#include <cstdint>
#include <new>
#include <cstdio>
#include <cstdlib>
#include <cstring>

// Issue #111 (clang-tidy, the first look at this file in patella_core): cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory off for the file:
// the original's malloc'd entries and texts, freed by their lists; the original's raw owners (notes/ownership.md):
// the window tree owns its children, the message queue its blocks, the lists their entries
// NOLINTBEGIN(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)

// The data is TTalk's (Talk.h, issue #107).

static_assert(sizeof(TTalkbox) == 0x1c0, "talkbox layout");
static_assert(offsetof(TTalkbox, Text) == 0x20 && offsetof(TTalkbox, Left) == 0x1b0, "talkbox lines (TFixedString, issue #57)");
// No pointers: the same on x64. Text a std::array of TFixedStrings (issue #60).
static_assert(sizeof(TTalkbox) == 0x1c0 && offsetof(TTalkbox, Text) == 0x20 && sizeof(TTalkbox::Text) == 0x190,
	"TTalkbox's layout");
static_assert(sizeof(TTalkLine::Time) == 4, "statement time: the original's 4 bytes (Time32, issue #62)");

bool patella::TTalk::TakeTalkChanged()
{
	const bool Result = TalkChanged;
	TalkChanged = false;
	return Result;
}
int patella::TTalk::GetNumberOfTalkboxes() const
{
	int Count = 0;
	for (int Position = 0; Position < TALKBOXES; Position++)
		if (Talkboxes[Position].State == 1)
			Count++;
	return Count;
}
void patella::TTalk::SetTalkChanged() { TalkChanged = true; }
const char *patella::TTalk::GetStatusMessage() const { return StatusMessageTime != 0 ? StatusMessage.Chars : nullptr; }
void patella::TTalk::SetIgnoreEnabled(bool Value) { IgnoreEnabled = Value; }
bool patella::TTalk::GetIgnoreEnabled() const { return IgnoreEnabled; }
void patella::TTalk::SetBuddyIgnore(bool Value) { BuddyIgnore = Value; }
bool patella::TTalk::GetBuddyIgnore() const { return BuddyIgnore; }
void patella::TTalk::SetIgnoreListEnabled(bool Value) { IgnoreListEnabled = Value; }
bool patella::TTalk::GetIgnoreListEnabled() const { return IgnoreListEnabled; }
void patella::TTalk::StartTalkThrottle() { TalkThrottle = GetMilliseconds() + 1000; }
bool patella::TTalk::IsTalkThrottled()
{
	if (TalkThrottle != 0) {
		if (GetMilliseconds() < TalkThrottle)
			return true;
		TalkThrottle = 0;
	}
	return false;
}
bool patella::TTalk::TakeConsoleChanged()
{
	const bool Result = ConsoleChanged;
	ConsoleChanged = false;
	return Result;
}
void patella::TTalk::SetConsoleChanged() { ConsoleChanged = true; }

TRGBColor *patella::TTalk::GetTalkColor(TRGBColor *Color, int TalkMode)
{
	switch (TalkMode) {
		// clang-format off
	case 0: TERROR("none is talking");
	case TALK_SAY: case TALK_WHISPER: case TALK_YELL: case TALK_CHANNEL_CALL: case TALK_GAMEMASTER_ANSWER: case TALK_PLAYER_ANSWER:
		Color->R = 240; Color->G = 240; Color->B = 0; break;
	case TALK_PRIVATE_MESSAGE:
		Color->R = 96; Color->G = 248; Color->B = 248; break;
	case TALK_GAMEMASTER_REQUEST: case TALK_EVENT_MESSAGE: case 20: case TALK_STATUS_MESSAGE: case 23:
		Color->R = 240; Color->G = 240; Color->B = 240; break;
	case TALK_GAMEMASTER_BROADCAST: case TALK_GAMEMASTER_CHANNELCALL: case TALK_GAMEMASTER_MESSAGE: case TALK_ANONYMOUS_BROADCAST: case TALK_ANONYMOUS_CHANNELCALL: case TALK_ANONYMOUS_MESSAGE: case TALK_ADMIN_MESSAGE: case 25:
		Color->R = 248; Color->G = 96; Color->B = 96; break;
	case 12:
		Color->R = 240; Color->G = 180; Color->B = 0; break;
	case TALK_ANIMAL_LOW: case TALK_ANIMAL_LOUD:
		Color->R = 255; Color->G = 102; Color->B = 0; break;
	case TALK_INFO_MESSAGE:
		Color->R = 0; Color->G = 240; Color->B = 0; break;
	case 24:
		Color->R = 160; Color->G = 160; Color->B = 255; break;
	default: TERROR1("something unknown is talking", TalkMode);
		// clang-format on
	}
	return Color;
}
TTalkbox *patella::TTalk::GetTalkbox(int Number)
{
	PATELLA_ASSERT_VALUES(In(Number,1,GetNumberOfTalkboxes()), Number, GetNumberOfTalkboxes());
	int Position = 0;
	while (Talkboxes[Position].State != 1 || --Number != 0)
		Position++;
	// MSVC's analysis (C28020, issue #111) takes Position as any int: the assertion keeps Number within the open
	// talkboxes, so the loop stops at one of them.
	PATELLA_MSVC_WARNING(suppress : 28020)
	return &Talkboxes[Position];
}
int patella::TTalk::GetTalkboxMode(int Number)
{
	return GetTalkbox(Number)->Mode;
}
void patella::TTalk::GetTalkboxPosition(int Number, int *x, int *y, int *z)
{
	const TTalkbox *Box = GetTalkbox(Number);
	const int Mode = Box->Mode;
	if (Mode == TALK_INFO_MESSAGE || Mode == TALK_EVENT_MESSAGE || Mode == TALK_ADMIN_MESSAGE || Mode == TALK_PRIVATE_MESSAGE || Mode == TALK_GAMEMASTER_BROADCAST || Mode == TALK_GAMEMASTER_MESSAGE)
		*x = *y = *z = 0;
	else {
		*x = Box->x;
		*y = Box->y;
		*z = Box->z;
	}
}
void patella::TTalk::GetTalkboxRectangle(int Number, int *x, int *y, int *Width, int *Height)
{
	const TTalkbox *Box = GetTalkbox(Number);
	*x = Box->Left;
	*y = Box->Top;
	*Width = Box->Width;
	*Height = Box->Height;
}
TRGBColor *patella::TTalk::GetTalkboxColor(TRGBColor *Color, int Number)
{
	GetTalkColor(Color, GetTalkbox(Number)->Mode);
	return Color;
}
int patella::TTalk::GetTalkboxLines(int Number)
{
	return GetTalkbox(Number)->Lines;
}
const char *patella::TTalk::GetTalkboxLine(int Number, int Line)
{
	return GetTalkbox(Number)->Text[Line];
}
void patella::TTalk::RemoveTalkbox(int Position)
{
	PATELLA_ASSERT_VALUES(In(Position,0,TALKBOXES-1), Position);
	TTalkbox &Box = Talkboxes[Position];
	int Oldest = TalkOrder;
	int Replacement = -1;
	for (int Search = 0; Search < TALKBOXES; Search++) {
		TTalkbox &Other = Talkboxes[Search];
		if (Search == Position || Other.State != 2)
			continue;
		bool Match = false;
		if (Box.Mode == TALK_SAY || Box.Mode == TALK_WHISPER || Box.Mode == TALK_YELL)
			Match = Other.Mode == Box.Mode && strcmp(Other.Text[0], Box.Text[0]) == 0
				&& Other.x == Box.x && Other.y == Box.y && Other.z == Box.z;
		if (Box.Mode == TALK_ANIMAL_LOW || Box.Mode == TALK_ANIMAL_LOUD)
			Match = (Other.Mode == TALK_ANIMAL_LOW || Other.Mode == TALK_ANIMAL_LOUD)
				&& Other.x == Box.x && Other.y == Box.y && Other.z == Box.z;
		if (Box.Mode == TALK_EVENT_MESSAGE || Box.Mode == TALK_ADMIN_MESSAGE || Box.Mode == TALK_GAMEMASTER_BROADCAST || Box.Mode == TALK_GAMEMASTER_MESSAGE)
			Match = Other.Mode == TALK_EVENT_MESSAGE || Other.Mode == TALK_ADMIN_MESSAGE || Other.Mode == TALK_GAMEMASTER_BROADCAST || Other.Mode == TALK_GAMEMASTER_MESSAGE;
		if (Box.Mode == TALK_PRIVATE_MESSAGE)
			Match = Other.Mode == TALK_PRIVATE_MESSAGE;
		if (Match && Other.Order < Oldest) {
			Oldest = Other.Order;
			Replacement = Search;
		}
	}
	if (Replacement != -1)
		Talkboxes[Replacement].State = 1;
	Box.State = 0;
	TalkChanged = true;
}
// Returns the free slot in EAX.
int patella::TTalk::FindFreeTalkbox()
{
	int i;
	for (;;) {
		for (i = 0; i < TALKBOXES; i++)
			if (Talkboxes[i].State == 0)
				return i;
		RemoveOldestTalkbox();
	}
}
void patella::TTalk::WrapTalkbox(TTalkbox *Talkbox, const char *Text)
{
	while (Talkbox->Lines < 10 && *Text != 0) {
		int Count = 0, Space = -1;
		while (Count < 40 && Text[Count] != 0 && Text[Count] != '\n') {
			if (IsSpace(Text[Count]))
				Space = Count;
			Count++;
		}
		char *Line = Talkbox->Text[Talkbox->Lines++];
		if (Count < 40) {
			memcpy(Line, Text, Count);
			Line[Count] = 0;
			Text += Count + (Text[Count] == '\n' ? 1 : 0);
		} else if (Space == -1) {
			strncpy(Line, Text, 38);
			Line[38] = '-';
			Line[39] = 0;
			Text += 38;
		} else {
			strncpy(Line, Text, Space);
			Line[Space] = 0;
			Text += Space + 1;
		}
	}
}
void patella::TTalk::AddTalkbox(int Mode, const char *Speaker, int x, int y, int z, const char *Text)
{
	PATELLA_ASSERT_VALUES(Mode==TALK_SAY || Mode==TALK_WHISPER || Mode==TALK_YELL || Mode==TALK_PRIVATE_MESSAGE || Mode==TALK_GAMEMASTER_BROADCAST || Mode==TALK_GAMEMASTER_MESSAGE || Mode==TALK_ANIMAL_LOW || Mode==TALK_ANIMAL_LOUD, Mode);
	PATELLA_ASSERT(Speaker!=nullptr);
	PATELLA_ASSERT(Text!=nullptr);
	if (Mode == TALK_PRIVATE_MESSAGE && !GetPrivateMessages())
		return;
	const int Position = FindFreeTalkbox();
	TTalkbox &Box = Talkboxes[Position];
	Box.Order = TalkOrder++;
	Box.Mode = Mode;
	Box.x = x;
	Box.y = y;
	Box.z = z;
	Box.Lines = 0;
	Box.RemainingTime = ((int)(strlen(Text) >> 1) + 30) * 100;
	// The first line names the speaker: sprintf with "%s says:", "%s whispers:", "%s yells:" or "%s:".
	const char *Verb = nullptr;
	switch (Mode) {
	case TALK_SAY: Verb = " says:"; break;
	case TALK_WHISPER: Verb = " whispers:"; break;
	case TALK_YELL: Verb = " yells:"; break;
	case TALK_PRIVATE_MESSAGE:
	case TALK_GAMEMASTER_BROADCAST:
	case TALK_GAMEMASTER_MESSAGE: Verb = ":"; break;
	default:
		break;
	}
	if (Verb != nullptr) {
		Box.Lines = 1;
		Box.Text[0].Format("{}{}", Speaker, Verb);
	}
	WrapTalkbox(&Box, Text);
	Box.State = 1;
	int Delay = 0;
	for (int Search = 0; Search < TALKBOXES; Search++) {
		TTalkbox &Other = Talkboxes[Search];
		if (Search == Position || Other.State == 0)
			continue;
		if ((Mode == TALK_SAY || Mode == TALK_WHISPER || Mode == TALK_YELL) && Other.Mode == Mode
			&& strcmp(Other.Text[0], Box.Text[0]) == 0 && Other.x == x && Other.y == y && Other.z == z) {
			for (int Line = 1; Line < Box.Lines; Line++)
				if (Other.Lines < 10)
					Other.Text[Other.Lines++].Assign(Box.Text[Line]);
			Box.State = 2;
		}
		if ((Mode == TALK_ANIMAL_LOW || Mode == TALK_ANIMAL_LOUD) && (Other.Mode == TALK_ANIMAL_LOW || Other.Mode == TALK_ANIMAL_LOUD)
			&& Other.x == x && Other.y == y && Other.z == z) {
			for (int Line = 0; Line < Box.Lines; Line++)
				if (Other.Lines < 10)
					Other.Text[Other.Lines++].Assign(Box.Text[Line]);
			Box.State = 2;
		}
		bool Match = (Mode == TALK_GAMEMASTER_BROADCAST || Mode == TALK_GAMEMASTER_MESSAGE)
			&& (Other.Mode == TALK_EVENT_MESSAGE || Other.Mode == TALK_ADMIN_MESSAGE || Other.Mode == TALK_GAMEMASTER_BROADCAST || Other.Mode == TALK_GAMEMASTER_MESSAGE);
		if (Mode == TALK_PRIVATE_MESSAGE && Other.Mode == TALK_PRIVATE_MESSAGE)
			Match = true;
		if (Match) {
			if (Box.State == 1)
				Box.RemainingTime /= 2;
			Box.State = 2;
			if (Delay <= Other.RemainingTime)
				Delay = Other.RemainingTime;
		}
	}
	Box.RemainingTime += Delay;
	TalkChanged = true;
}
void patella::TTalk::AddMessageTalkbox(int Mode, const char *Text)
{
	PATELLA_ASSERT_VALUES(Mode==TALK_INFO_MESSAGE || Mode==TALK_EVENT_MESSAGE || Mode==TALK_ADMIN_MESSAGE, Mode);
	PATELLA_ASSERT(Text!=nullptr);
	if (Mode == TALK_INFO_MESSAGE)
		for (int Position = 0; Position < TALKBOXES; Position++)
			if (Talkboxes[Position].State != 0 && Talkboxes[Position].Mode == TALK_INFO_MESSAGE)
				RemoveTalkbox(Position);
	const int Position = FindFreeTalkbox();
	TTalkbox &Box = Talkboxes[Position];
	Box.Order = TalkOrder++;
	Box.Mode = Mode;
	Box.Lines = 0;
	Box.RemainingTime = ((int)(strlen(Text) >> 1) + 30) * (Mode == TALK_EVENT_MESSAGE || Mode == TALK_ADMIN_MESSAGE ? 200 : 100);
	WrapTalkbox(&Box, Text);
	Box.State = 1;
	int Delay = 0;
	if (Mode != TALK_INFO_MESSAGE) {
		for (int Search = 0; Search < TALKBOXES; Search++) {
			const TTalkbox &Other = Talkboxes[Search];
			if (Search != Position && Other.State != 0
				&& (Other.Mode == TALK_EVENT_MESSAGE || Other.Mode == TALK_ADMIN_MESSAGE || Other.Mode == TALK_GAMEMASTER_BROADCAST || Other.Mode == TALK_GAMEMASTER_MESSAGE)) {
				if (Box.State == 1)
					Box.RemainingTime /= 2;
				Box.State = 2;
				if (Delay <= Other.RemainingTime)
					Delay = Other.RemainingTime;
			}
		}
	}
	Box.RemainingTime += Delay;
	TalkChanged = true;
}
void patella::TTalk::SetTalkboxRectangle(int Number, int x, int y, int Width, int Height)
{
	TTalkbox *Box = GetTalkbox(Number);
	Box->Left = x;
	Box->Top = y;
	Box->Width = Width;
	Box->Height = Height;
}
// When GetMilliseconds throws before the loop, the original reports i uninitialised: MSVC's
// unoptimised build warns (C4701, a code generation warning, which a pragma turns off only
// outside the function); initialising i would change the oracle's code.
PATELLA_MSVC_WARNING(push)
PATELLA_MSVC_WARNING(disable : 4701)
void patella::TTalk::UpdateTalkboxes()
{
	int i;
	if (!LastTimeSet) { // the original's function-local static, initialised on the first call (issue #107)
		LastTime = GetMilliseconds();
		LastTimeSet = true;
	}
	const int DiffTime = GetMilliseconds() - LastTime;
	for (i = 0; i < TALKBOXES; i++)
		// NOLINTNEXTLINE(bugprone-assignment-in-if-condition): the original's countdown in the condition
		if (Talkboxes[i].State != 0 && (Talkboxes[i].RemainingTime -= DiffTime) < 1)
			RemoveTalkbox(i);
	LastTime = GetMilliseconds();
}
PATELLA_MSVC_WARNING(pop)
void patella::TTalk::RemoveOldestTalkbox()
{
	int Oldest = TalkOrder, Position = -1;
	for (int Search = 0; Search < TALKBOXES; Search++)
		if (Talkboxes[Search].State != 0 && Talkboxes[Search].Order < Oldest) {
			Oldest = Talkboxes[Search].Order;
			Position = Search;
		}
	if (Position != -1)
		RemoveTalkbox(Position);
}
void patella::TTalk::SetStatusMessage(const char *Message, bool Permanent)
{
	TalkChanged = true;
	if (Message == nullptr) {
		StatusMessageTime = 0;
		return;
	}
	PATELLA_ASSERT_VALUES(strlen(Message)<STATUSMESSAGE_LENGTH, strlen(Message));
	StatusMessage.Assign(Message);
	StatusMessageTime = Permanent ? -1 : 50;
}
void patella::TTalk::UpdateStatusMessage()
{
	if (StatusMessageTime > 0 && --StatusMessageTime == 0) // NOLINT(bugprone-inc-dec-in-conditions): the original's countdown
		TalkChanged = true;
}
bool TTalkFilter::Matches(int _Mode, const char *_Addressee, int _Channel)
{
	if (_Mode == TALK_GAMEMASTER_MESSAGE || _Mode == TALK_GAMEMASTER_BROADCAST || _Mode == TALK_GAMEMASTER_CHANNELCALL || _Mode == 12)
		return false;
	if ((Mode != _Mode && Mode != 0) || (Channel != _Channel && Channel != -1))
		return false;
	if (strlen(Addressee) != 0 && _Addressee == nullptr)
		return false;
	if (strlen(Addressee) == 0)
		return true;
	PATELLA_ASSERT(_Addressee!=nullptr);
	return CompareStrings(Addressee, _Addressee, -1) == 0;
}
int patella::TTalk::FindTalkFilter(int Mode, const char *Addressee, int Channel, TIndexedVector<TTalkFilter *> &List)
{
	for (int i = List.min; i <= List.max; i++)
		if (List(i) != nullptr && List(i)->Matches(Mode, Addressee, Channel))
			return i;
	return -1;
}
int patella::TTalk::FindTextBuffer(TTalkBuffer *TextBuffer, TIndexedVector<TTalkFilter *> &List)
{
	PATELLA_ASSERT(TextBuffer!=nullptr);
	int i;
	for (i = List.min; i <= List.max; i++)
		if (List(i) != nullptr && List(i)->TextBuffer == TextBuffer)
			return i;
	return -1;
}
void patella::TTalk::AddTalkFilter(int Mode, const char *Addressee, int Channel, TTalkBuffer *TextBuffer,
	TIndexedVector<TTalkFilter *> &List, bool Permanent)
{
	int i = List.min;
	while (i <= List.max && List(i) != nullptr)
		i++;
	TTalkFilter *Filter = new (std::nothrow) TTalkFilter;
	if (Filter != nullptr) {
		Filter->TextBuffer = TextBuffer;
		Filter->Mode = Mode;
		if (Addressee == nullptr)
			Filter->Addressee[0] = 0;
		else
			CopyString(Filter->Addressee.Chars, Addressee);
		Filter->Channel = Channel;
		Filter->Permanent = Permanent;
	}
	List(i) = Filter;
}
// Buddy/temporary exceptions suppress the ignore list.
bool patella::TTalk::IsTalkIgnored(int Mode, const char *Name, int Channel)
{
	if (IgnoreEnabled) {
		if (BuddyIgnore)
			for (int i = TemporaryIgnoreList.min; i <= TemporaryIgnoreList.max; i++)
				if (TemporaryIgnoreList(i) != nullptr && TemporaryIgnoreList(i)->Matches(Mode, Name, Channel))
					return false;
		for (int i = BuddyExemptionList.min; i <= BuddyExemptionList.max; i++)
			if (BuddyExemptionList(i) != nullptr && BuddyExemptionList(i)->Matches(Mode, Name, Channel))
				return false;
	}
	if (IgnoreListEnabled)
		for (int i = IgnoreList.min; i <= IgnoreList.max; i++)
			if (IgnoreList(i) != nullptr && IgnoreList(i)->Matches(Mode, Name, Channel))
				return true;
	return false;
}

char *patella::TTalk::GetBuddyIgnoreText()
{
	int Length = 0;
	for (int i = BuddyExemptionList.min; i <= BuddyExemptionList.max; i++)
		if (BuddyExemptionList(i) != nullptr)
			Length += (int)strlen(BuddyExemptionList(i)->Addressee) + 1;
	if (Length == 0)
		return nullptr;
	char *Text = new (std::nothrow) char[Length + 1];
	// MSVC's analysis (C28182, issue #111): the original's unchecked allocation (VC7.1's new returned null), kept.
	PATELLA_MSVC_WARNING(suppress : 28182)
	Text[0] = 0;
	// The original wrote each name and its line break at Text + Length, the text's length so far.
	TStringBuffer Buffer(Text, static_cast<std::size_t>(Length) + 1);
	for (int i = BuddyExemptionList.min; i <= BuddyExemptionList.max; i++)
		if (BuddyExemptionList(i) != nullptr && strlen(BuddyExemptionList(i)->Addressee) != 0)
			Buffer.AppendFormat("{}\n", BuddyExemptionList(i)->Addressee);
	return Text;
}

void patella::TTalk::ClearBuddyIgnore()
{
	for (int i = BuddyExemptionList.min; i <= BuddyExemptionList.max; i++)
		if (BuddyExemptionList(i) != nullptr) {
			free(BuddyExemptionList(i));
			BuddyExemptionList(i) = nullptr;
		}
}

void patella::TTalk::AddBuddyIgnore(const char *Addressee, bool Permanent)
{
	const int Filter = FindTalkFilter(0, Addressee, -1, BuddyExemptionList);
	if (Filter != -1) {
		free(BuddyExemptionList(Filter));
		BuddyExemptionList(Filter) = nullptr;
	}
	AddTalkFilter(0, Addressee, -1, nullptr, BuddyExemptionList, Permanent);
}
bool patella::TTalk::IsPermanentBuddyIgnore(const char *Addressee)
{
	const int Filter = FindTalkFilter(0, Addressee, -1, BuddyExemptionList);
	return Filter != -1 ? BuddyExemptionList(Filter)->Permanent : false;
}

void patella::TTalk::ClearTemporaryIgnore()
{
	for (int i = TemporaryIgnoreList.min; i <= TemporaryIgnoreList.max; i++)
		if (TemporaryIgnoreList(i) != nullptr) {
			free(TemporaryIgnoreList(i));
			TemporaryIgnoreList(i) = nullptr;
		}
}

void patella::TTalk::AddTemporaryIgnore(const char *Addressee)
{
	const int Filter = FindTalkFilter(0, Addressee, -1, TemporaryIgnoreList);
	if (Filter != -1) {
		free(TemporaryIgnoreList(Filter));
		TemporaryIgnoreList(Filter) = nullptr;
	}
	AddTalkFilter(0, Addressee, -1, nullptr, TemporaryIgnoreList, false);
}
void patella::TTalk::RemoveTemporaryIgnore(const char *Addressee)
{
	const int Filter = FindTalkFilter(0, Addressee, -1, TemporaryIgnoreList);
	if (Filter != -1) {
		free(TemporaryIgnoreList(Filter));
		TemporaryIgnoreList(Filter) = nullptr;
	}
}
char *patella::TTalk::GetIgnoreListText()
{
	int Length = 0;
	for (int i = IgnoreList.min; i <= IgnoreList.max; i++)
		if (IgnoreList(i) != nullptr)
			Length += (int)strlen(IgnoreList(i)->Addressee) + 1;
	if (Length == 0)
		return nullptr;
	char *Text = new (std::nothrow) char[Length + 1];
	// MSVC's analysis (C28182, issue #111): the original's unchecked allocation (VC7.1's new returned null), kept.
	PATELLA_MSVC_WARNING(suppress : 28182)
	Text[0] = 0;
	// The original wrote each name and its line break at Text + Length, the text's length so far.
	TStringBuffer Buffer(Text, static_cast<std::size_t>(Length) + 1);
	for (int i = IgnoreList.min; i <= IgnoreList.max; i++)
		if (IgnoreList(i) != nullptr && strlen(IgnoreList(i)->Addressee) != 0)
			Buffer.AppendFormat("{}\n", IgnoreList(i)->Addressee);
	return Text;
}

bool patella::TTalk::IsModeIgnored(int Mode)
{
	return FindTalkFilter(Mode, nullptr, -1, IgnoreList) != -1;
}

void patella::TTalk::ClearIgnoreList()
{
	for (int i = IgnoreList.min; i <= IgnoreList.max; i++)
		if (IgnoreList(i) != nullptr) {
			free(IgnoreList(i));
			IgnoreList(i) = nullptr;
		}
}

void patella::TTalk::AddIgnoreFilter(int Mode, const char *Addressee, bool Permanent)
{
	const int Filter = FindTalkFilter(Mode, Addressee, -1, IgnoreList);
	if (Filter != -1) {
		free(IgnoreList(Filter));
		IgnoreList(Filter) = nullptr;
	}
	AddTalkFilter(Mode, Addressee, -1, nullptr, IgnoreList, Permanent);
}
void patella::TTalk::IgnoreMode(int Mode)
{
	AddIgnoreFilter(Mode, nullptr, false);
}
void patella::TTalk::IgnoreName(const char *Addressee, bool Permanent)
{
	AddIgnoreFilter(0, Addressee, Permanent);
}
bool patella::TTalk::IsPermanentIgnore(const char *Addressee)
{
	const int Filter = FindTalkFilter(0, Addressee, -1, IgnoreList);
	return Filter != -1 ? IgnoreList(Filter)->Permanent : false;
}
void patella::TTalk::RemoveIgnoreFilter(int Mode, const char *Addressee)
{
	const int Filter = FindTalkFilter(Mode, Addressee, -1, IgnoreList);
	if (Filter != -1) {
		free(IgnoreList(Filter));
		IgnoreList(Filter) = nullptr;
	}
}
void patella::TTalk::UnignoreMode(int Mode)
{
	RemoveIgnoreFilter(Mode, nullptr);
}
void patella::TTalk::UnignoreName(const char *Addressee)
{
	RemoveIgnoreFilter(0, Addressee);
}
void patella::TTalk::ShowPermanentIgnoreWarning()
{
	int o, Counter = 0;
	for (o = IgnoreList.min; o <= IgnoreList.max; o++)
		if (IgnoreList(o) != nullptr && strlen(IgnoreList(o)->Addressee) != 0 && IgnoreList(o)->Permanent)
			Counter++;
	if (Counter > 0) {
		PATELLA_ASSERT(DefaultTextBuffer!=nullptr);
		AddTextBufferLine(DefaultTextBuffer, 0, TextPermanentlyIgnoring, 20, nullptr, 0);
		ConsoleChanged = true;
	}
}
void patella::TTalk::AddTextBufferLine(TTalkBuffer *TargetBuffer, uint32 StatementID, const char *Text,
	int TalkMode, const char *Name, int Time)
{
	PATELLA_ASSERT(TargetBuffer!=nullptr);
	TTalkLine *Line;
	if (Name == nullptr)
		Line = new (std::nothrow) TTalkLine(StatementID, TalkMode, Text, 0);
	else
		Line = new (std::nothrow) TTalkLine(StatementID, TalkMode, Name, Text, Time);
	TargetBuffer->Append(Line);
	ConsoleChanged = true;
}
void patella::TTalk::AddConsoleText(uint32 StatementID, const char *Text, int TalkMode, const char *Name, int Channel, int Time)
{
	PATELLA_ASSERT(Text!=nullptr);
	bool Broadcast;
	switch (TalkMode) {
	case TALK_ADMIN_MESSAGE: Broadcast = true; break;
	case TALK_EVENT_MESSAGE:
	case 20: Broadcast = GetEventMessages(); break;
	case TALK_STATUS_MESSAGE: Broadcast = GetStatusMessages(); break;
	case TALK_INFO_MESSAGE: Broadcast = GetInfoMessages(); break;
	case 23: return;
	default: {
		TTalkBuffer *TargetBuffer = DefaultTextBuffer;
		if (TalkMode == TALK_CHANNEL_CALL || TalkMode == TALK_GAMEMASTER_REQUEST || TalkMode == TALK_GAMEMASTER_ANSWER || TalkMode == TALK_PLAYER_ANSWER
			|| TalkMode == TALK_GAMEMASTER_CHANNELCALL || TalkMode == TALK_ANONYMOUS_CHANNELCALL || TalkMode == 12)
			TargetBuffer = nullptr;
		if (TalkMode == TALK_ANONYMOUS_CHANNELCALL)
			Name = nullptr;
		for (int i = ConsoleFilterList.min; i <= ConsoleFilterList.max; i++) {
			TTalkFilter *Filter = ConsoleFilterList(i);
			const int Mode = TalkMode == TALK_GAMEMASTER_CHANNELCALL || TalkMode == TALK_ANONYMOUS_CHANNELCALL || TalkMode == 12 ? TALK_CHANNEL_CALL : TalkMode;
			if (Filter != nullptr && Filter->Matches(Mode, Name, Channel))
				TargetBuffer = Filter->TextBuffer;
		}
		if (TargetBuffer != nullptr)
			AddTextBufferLine(TargetBuffer, StatementID, Text, TalkMode, Name, Time);
		return;
	}
	}
	if (Broadcast) {
		TTalkBuffer *TargetBuffer = DefaultTextBuffer;
		PATELLA_ASSERT(TargetBuffer!=nullptr);
		AddTextBufferLine(TargetBuffer, StatementID, Text, TalkMode, Name, Time);
		for (int i = ConsoleFilterList.min; i <= ConsoleFilterList.max; i++) {
			const TTalkFilter *Filter = ConsoleFilterList(i);
			if (Filter != nullptr) {
				TargetBuffer = Filter->TextBuffer;
				PATELLA_ASSERT(TargetBuffer!=nullptr);
				if (TargetBuffer->ShowMessages && !TargetBuffer->IsRuleViolationChannel())
					AddTextBufferLine(TargetBuffer, StatementID, Text, TalkMode, Name, Time);
			}
		}
	}
}
void patella::TTalk::AddChannelNotice(const char *Addressee, const char *Text, int Mode, const char *Name, int Channel)
{
	TTalkBuffer *TargetBuffer = Mode == TALK_CHANNEL_CALL ? nullptr : DefaultTextBuffer;
	for (int i = ConsoleFilterList.min; i <= ConsoleFilterList.max; i++) {
		TTalkFilter *Filter = ConsoleFilterList(i);
		if (Filter != nullptr && Filter->Matches(Mode, Name, Channel))
			TargetBuffer = Filter->TextBuffer;
	}
	if (TargetBuffer != nullptr)
		AddTextBufferLine(TargetBuffer, 0, Text, 24, Addressee, 0);
}
void patella::TTalk::ClearDefaultTextBuffer()
{
	if (DefaultTextBuffer != nullptr)
		DefaultTextBuffer->Clear();
	ConsoleChanged = true;
}
void patella::TTalk::ClearTextBuffer(TTalkBuffer *TextBuffer)
{
	PATELLA_ASSERT(TextBuffer!=nullptr);
	TextBuffer->Clear();
	ConsoleChanged = true;
}
TTalkBuffer *patella::TTalk::GetTextBuffer(int Mode, const char *Name, int Channel)
{
	const int Filter = FindTalkFilter(Mode, Name, Channel, ConsoleFilterList);
	if (Filter == -1)
		return nullptr;
	PATELLA_ASSERT_VALUES(ConsoleFilterList(Filter)!=nullptr, Filter);
	return ConsoleFilterList(Filter)->TextBuffer;
}
TTalkBuffer *patella::TTalk::CreateTextBuffer(int ReceiveTalkMode, int SendTalkMode, const char *Name, int Channel)
{
	if (FindTalkFilter(ReceiveTalkMode, Name, Channel, ConsoleFilterList) != -1)
		return nullptr;
	TTalkBuffer *TextBuffer;
	if (Name == nullptr && Channel == -1 && ReceiveTalkMode == SendTalkMode)
		TextBuffer = new (std::nothrow) TTalkBuffer(ReceiveTalkMode);
	else if (Name == nullptr && Channel != -1 && ReceiveTalkMode == SendTalkMode) {
		if (Channel < 8)
			TextBuffer = new (std::nothrow) TChannelTalkBuffer(ReceiveTalkMode, Channel);
		else
			TextBuffer = new (std::nothrow) TPrivateChannel(ReceiveTalkMode, Channel);
	} else if (Name != nullptr && Channel == -1 && ReceiveTalkMode == SendTalkMode)
		TextBuffer = new (std::nothrow) TPrivateTalkBuffer(ReceiveTalkMode, Name);
	else if (ReceiveTalkMode == TALK_GAMEMASTER_ANSWER || ReceiveTalkMode == TALK_PLAYER_ANSWER)
		TextBuffer = new (std::nothrow) TRuleViolationTalkBuffer(ReceiveTalkMode, SendTalkMode, Name);
	else {
		if (Name != nullptr || Channel == -1 || ReceiveTalkMode != TALK_GAMEMASTER_REQUEST)
			TERROR("can't create text buffer");
		if (FindTalkFilter(TALK_GAMEMASTER_REQUEST, nullptr, -1, ConsoleFilterList) != -1)
			return nullptr;
		TextBuffer = new (std::nothrow) TRuleViolationChannel(TALK_GAMEMASTER_REQUEST, Channel);
		Channel = -1;
	}
	AddTalkFilter(ReceiveTalkMode, Name, Channel, TextBuffer, ConsoleFilterList, false);
	return TextBuffer;
}
void patella::TTalk::DeleteTextBuffer(TTalkBuffer *TextBuffer)
{
	if (TextBuffer != DefaultTextBuffer) {
		const int Filter = FindTextBuffer(TextBuffer, ConsoleFilterList);
		PATELLA_ASSERT(Filter!=-1);
		free(ConsoleFilterList(Filter));
		ConsoleFilterList(Filter) = nullptr;
		if (TextBuffer != nullptr) {
			TextBuffer->TTalkBuffer::~TTalkBuffer(); // non-virtual, by its qualified name (issue #111: Clang's warning)
			free(TextBuffer);
		}
	}
}
// The player owns a private channel (the selected one; ConsoleWindow.cpp's
// "#I"/"#X" commands work only then).
bool patella::TTalk::HasOwnPrivateChannel()
{
	int i;
	for (i = ConsoleFilterList.min; i <= ConsoleFilterList.max; i++) {
		const TTalkFilter *Filter = ConsoleFilterList(i);
		if (Filter != nullptr && Filter->TextBuffer != nullptr
			&& Filter->TextBuffer->IsSelected() && Filter->TextBuffer->IsPrivateChannel())
			return true;
	}
	return false;
}
// noexcept (issue #81; noexcept(false) until then): Clear removes the
// first line until there is none, and Remove of the first line takes its
// first two branches, never the search with its assertion; free does not
// throw. In a destructor the virtual calls are TTalkBuffer's own.
TTalkBuffer::~TTalkBuffer() { Clear(); }
void TTalkBuffer::Clear()
{
	while (first != nullptr)
		Remove(first);
}
void TTalkBuffer::AddText(int Mode, const char *text, int DiffTime)
{
	if (Enabled) {
		PATELLA_ASSERT(text!=nullptr);
		Append(new (std::nothrow) TTalkLine(0, Mode, text, DiffTime));
	}
}
// The original frees with free() what AddText allocates with new (VC7.1's new was malloc; notes/ownership.md): GCC's
// -Wmismatched-new-delete (mingw-w64, where it inlines AddText here) names it, allowed for this function (issue #111).
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic push")
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic ignored \"-Wmismatched-new-delete\"")
void TTalkBuffer::Append(TTalkLine *line)
{
	if (!Enabled) {
		if (line != nullptr) {
			if (line->Line != nullptr)
				free(line->Line);
			free(line);
		}
		return;
	}
	PATELLA_ASSERT(line!=nullptr);
	if (last == nullptr && first == nullptr) {
		last = first = line;
		line->Next = line->Previous = nullptr;
	} else {
		line->Next = nullptr;
		line->Previous = last;
		// MSVC's analysis (C6011, issue #111): first and last are null together (the list's ends), so last is not
		// null when first is not.
		PATELLA_MSVC_WARNING(suppress : 6011)
		last->Next = line;
		last = line;
	}
	if (++lines > limit)
		Remove(first);
	Changed = LinesChanged = true;
}
PATELLA_GCC_DIAGNOSTIC("GCC diagnostic pop")
void TTalkBuffer::Remove(TTalkLine *line)
{
	if (line == first && line == last)
		first = last = nullptr;
	else if (line == first) {
		first = line->Next;
		first->Previous = nullptr;
	} else if (line == last) {
		last = line->Previous;
		last->Next = nullptr;
	} else {
		const TTalkLine *Search = first;
		for (;;) {
			PATELLA_ASSERT(Search!=nullptr);
			if (Search == line)
				break;
			Search = Search->Next;
		}
		line->Previous->Next = line->Next;
		line->Next->Previous = line->Previous;
	}
	if (cursor == line)
		cursor = line->Next;
	if (line != nullptr) {
		if (line->Line != nullptr)
			free(line->Line);
		free(line);
	}
	lines--;
	if (cursor == nullptr)
		cursor = first;
	Complete = false;
	Changed = LinesChanged = true;
}
void TTalkBuffer::Remove(int lineNumber)
{
	PATELLA_ASSERT(first!=nullptr);
	// Not const (issue #39): as a constant the compiler folds i away; the binary keeps it in a stack slot.
	int i = 0; // NOLINT(misc-const-correctness)
	TTalkLine *Search = first;
	while (i != lineNumber) {
		Search = Search->Next;
		PATELLA_ASSERT(Search!=nullptr);
		// The binary never increments i: nonzero indices eventually assert.
	}
	PATELLA_ASSERT(Search!=nullptr);
	Remove(Search);
}
TTalkLine *TTalkBuffer::GetLine(int lineNumber)
{
	int i = 0;
	TTalkLine *Search = first;
	if (Search == nullptr) {
		cursor = nullptr;
		return nullptr;
	}
	while (i != lineNumber) {
		Search = Search->Next;
		PATELLA_ASSERT(Search!=nullptr);
		i++;
	}
	cursor = Search;
	return Search;
}
// Returns the advanced cursor, not the previous line.
TTalkLine *TTalkBuffer::GetNext()
{
	PATELLA_ASSERT(cursor!=nullptr);
	cursor = cursor->Next;
	return cursor;
}
// Bytes is the original catch name for F's slot (the FILE's address; only its
// catch wrapper named it). One line each, in the file's mode: SaveChannel's
// text file ends them in CR LF.
int TTalkBuffer::Write(TFile &F) const
{
	PATELLA_ASSERT(F!=nullptr);
	[[maybe_unused]] const std::intptr_t bytes = static_cast<std::intptr_t>(F.Address());
	for (const TTalkLine *Line = first; Line != nullptr; Line = Line->Next)
		F.Print("{}\n", Line->GetText());
	return 0;
}
TPrivateTalkBuffer::TPrivateTalkBuffer(int TalkMode, const char *_Addressee) : TTalkBuffer(TalkMode)
{
	PATELLA_ASSERT(_Addressee!=nullptr);
	CopyString(Addressee.Chars, _Addressee);
}
TChannelTalkBuffer::TChannelTalkBuffer(int TalkMode, int _ChannelID) : TTalkBuffer(TalkMode)
{
	PATELLA_ASSERT(_ChannelID!=-1);
	ChannelID = _ChannelID;
}
TRuleViolationTalkBuffer::TRuleViolationTalkBuffer(int _ReceiveTalkMode, int _SendTalkMode,
	const char *_Addressee) : TTalkBuffer(_ReceiveTalkMode)
{
	SendTalkMode = _SendTalkMode;
	Closed = false;
	if (_Addressee == nullptr)
		Addressee[0] = 0;
	else
		CopyString(Addressee.Chars, _Addressee);
	WriteEnabled = true;
}
void TRuleViolationTalkBuffer::AddText(int Mode, const char *text, int DiffTime)
{
	WriteEnabled = true;
	TTalkBuffer::AddText(Mode, text, DiffTime);
}
void TRuleViolationTalkBuffer::Append(TTalkLine *line)
{
	WriteEnabled = true;
	TTalkBuffer::Append(line);
}
TRuleViolationChannel::TRuleViolationChannel(int TalkMode, int _ChannelID) : TTalkBuffer(TalkMode)
{
	SendTalkMode = 0;
	Closed = false;
	PATELLA_ASSERT(_ChannelID!=-1);
	ChannelID = _ChannelID;
}
void TRuleViolationChannel::Remove(TTalkLine *line)
{
	const bool OldChanged = Changed;
	TTalkBuffer::Remove(line);
	Changed = OldChanged;
	if (first == nullptr)
		Closed = true;
}
void TRuleViolationChannel::Remove(int lineNumber)
{
	const bool OldChanged = Changed;
	TTalkBuffer::Remove(lineNumber);
	Changed = OldChanged;
	if (first == nullptr)
		Closed = true;
}
void TRuleViolationChannel::Append(TTalkLine *line)
{
	PATELLA_ASSERT(line!=nullptr);
	cursor = first;
	TTalkLine *Search = first;
	while (Search != nullptr) {
		TTalkLine *Next = GetNext();
		if (CompareStrings(line->Addressee, Search->Addressee, -1) == 0)
			Remove(Search);
		Search = Next;
	}
	TTalkBuffer::Append(line);
}
namespace {

// Ours (issue #62): the local time of a talk line's 64-bit time, for its stamp: the C library's localtime
// (compat/crt71.h), which up to 2038 gives the fields _localtime32 gave. Where it gives none (before 1970 or after
// 3000, which only a time offset from the server reaches) the original read localtime's NULL and crashed; here the
// stamp is "00:00".
std::tm LineLocalTime(std::int64_t Time)
{
	std::tm Local{};
	if (!patella::compat::CRT71LocalTime(Time, Local))
		Local = std::tm{};
	return Local;
}

} // namespace

TTalkLine::TTalkLine(uint32 _StatementID, int _Mode, const char *_Line, int DiffTime)
{
	PATELLA_ASSERT_VALUES(_Mode>=0&&_Mode<TALK_MODE_BEYONDLAST, _Mode);
	PATELLA_ASSERT(_Line!=nullptr);
	Mode = _Mode;
	StatementID = _StatementID;
	const std::int64_t LineTime = patella::platform::Platform().Clock.WallSeconds() + DiffTime;
	Time = StoreTime32(LineTime);
	const std::tm LocalTime = LineLocalTime(LineTime);
	TFixedString<8> Stamp;
	Stamp.Format("{:02}:{:02} ", LocalTime.tm_hour, LocalTime.tm_min);
	Addressee[0] = 0;
	const std::size_t Size = strlen(Stamp) + strlen(_Line) + 1;
	Line = new (std::nothrow) char[Size];
	TStringBuffer(Line, Size).Assign(Stamp);
	TrimString(Line + strlen(Stamp), _Line);
	WithoutTime = Text = Line + strlen(Stamp);
}
TTalkLine::TTalkLine(uint32 _StatementID, int _Mode, const char *_Addressee, const char *_Line, int DiffTime)
{
	PATELLA_ASSERT_VALUES(_Mode>=0&&_Mode<TALK_MODE_BEYONDLAST, _Mode);
	PATELLA_ASSERT(_Line!=nullptr);
	PATELLA_ASSERT(_Addressee!=nullptr);
	Mode = _Mode;
	StatementID = _StatementID;
	const std::int64_t LineTime = patella::platform::Platform().Clock.WallSeconds() + DiffTime;
	Time = StoreTime32(LineTime);
	const std::tm LocalTime = LineLocalTime(LineTime);
	TFixedString<8> Stamp;
	Stamp.Format("{:02}:{:02} ", LocalTime.tm_hour, LocalTime.tm_min);
	CopyString(Addressee.Chars, _Addressee);
	const std::size_t Size = strlen(Stamp) + strlen(Addressee) + strlen(_Line) + 3;
	Line = new (std::nothrow) char[Size];
	TStringBuffer(Line, Size).Assign(Stamp);
	WithoutTime = Line + strlen(Stamp);
	TStringBuffer(WithoutTime, Size - strlen(Stamp)).Format("{}: ", Addressee);
	Text = WithoutTime + strlen(Addressee) + 2;
	TrimString(Text, _Line);
}
void patella::TTalk::ResetTalk()
{
	for (int Position = 0; Position < TALKBOXES; Position++)
		Talkboxes[Position].State = 0;
	TalkChanged = true;
	TalkOrder = 0;
	SetStatusMessage(nullptr, false);
	ClearDefaultTextBuffer();
}
void patella::TTalk::InitTalk()
{
	IgnoreEnabled = true;
	BuddyIgnore = false;
	IgnoreListEnabled = true;
	ResetTalk();
	DefaultTextBuffer = new (std::nothrow) TTalkBuffer(TALK_SAY);
}
TPrivateChannel::TPrivateChannel(int TalkMode, int _ChannelID) : TChannelTalkBuffer(TalkMode, _ChannelID)
{
	WriteEnabled = true;
}
void patella::TTalk::ExitTalk()
{
	ClearBuddyIgnore();
	ClearTemporaryIgnore();
	ClearIgnoreList();
	if (DefaultTextBuffer != nullptr) {
		DefaultTextBuffer->TTalkBuffer::~TTalkBuffer(); // non-virtual, by its qualified name (issue #111)
		free(DefaultTextBuffer);
	}
	DefaultTextBuffer = nullptr;
}

// The functions of the original (issue #107): each calls its member on the one TClient's TTalk.

bool TakeTalkChanged()
{
	return patella::bridge::Talk().TakeTalkChanged();
}

int GetNumberOfTalkboxes()
{
	return patella::bridge::Talk().GetNumberOfTalkboxes();
}

const char *GetStatusMessage()
{
	return patella::bridge::Talk().GetStatusMessage();
}

void SetIgnoreEnabled(bool Value)
{
	patella::bridge::Talk().SetIgnoreEnabled(Value);
}

bool GetIgnoreEnabled()
{
	return patella::bridge::Talk().GetIgnoreEnabled();
}

void SetBuddyIgnore(bool Value)
{
	patella::bridge::Talk().SetBuddyIgnore(Value);
}

bool GetBuddyIgnore()
{
	return patella::bridge::Talk().GetBuddyIgnore();
}

void SetIgnoreListEnabled(bool Value)
{
	patella::bridge::Talk().SetIgnoreListEnabled(Value);
}

bool GetIgnoreListEnabled()
{
	return patella::bridge::Talk().GetIgnoreListEnabled();
}

bool IsTalkThrottled()
{
	return patella::bridge::Talk().IsTalkThrottled();
}

bool TakeConsoleChanged()
{
	return patella::bridge::Talk().TakeConsoleChanged();
}

TRGBColor *GetTalkColor(TRGBColor *Color, int TalkMode)
{
	return patella::bridge::Talk().GetTalkColor(Color, TalkMode);
}

TTalkbox *GetTalkbox(int Number)
{
	return patella::bridge::Talk().GetTalkbox(Number);
}

int GetTalkboxMode(int Number)
{
	return patella::bridge::Talk().GetTalkboxMode(Number);
}

void GetTalkboxPosition(int Number, int *x, int *y, int *z)
{
	patella::bridge::Talk().GetTalkboxPosition(Number, x, y, z);
}

void GetTalkboxRectangle(int Number, int *x, int *y, int *Width, int *Height)
{
	patella::bridge::Talk().GetTalkboxRectangle(Number, x, y, Width, Height);
}

TRGBColor *GetTalkboxColor(TRGBColor *Color, int Number)
{
	return patella::bridge::Talk().GetTalkboxColor(Color, Number);
}

int GetTalkboxLines(int Number)
{
	return patella::bridge::Talk().GetTalkboxLines(Number);
}

const char *GetTalkboxLine(int Number, int Line)
{
	return patella::bridge::Talk().GetTalkboxLine(Number, Line);
}

void RemoveTalkbox(int Position)
{
	patella::bridge::Talk().RemoveTalkbox(Position);
}

int FindFreeTalkbox()
{
	return patella::bridge::Talk().FindFreeTalkbox();
}

void WrapTalkbox(TTalkbox *Talkbox, const char *Text)
{
	patella::bridge::Talk().WrapTalkbox(Talkbox, Text);
}

void SetTalkboxRectangle(int Number, int x, int y, int Width, int Height)
{
	patella::bridge::Talk().SetTalkboxRectangle(Number, x, y, Width, Height);
}

void RemoveOldestTalkbox()
{
	patella::bridge::Talk().RemoveOldestTalkbox();
}

int FindTalkFilter(int Mode, const char *Addressee, int Channel, TIndexedVector<TTalkFilter *> &List)
{
	return patella::bridge::Talk().FindTalkFilter(Mode, Addressee, Channel, List);
}

int FindTextBuffer(TTalkBuffer *TextBuffer, TIndexedVector<TTalkFilter *> &List)
{
	return patella::bridge::Talk().FindTextBuffer(TextBuffer, List);
}

void AddTalkFilter(int Mode, const char *Addressee, int Channel, TTalkBuffer *TextBuffer,
	TIndexedVector<TTalkFilter *> &List, bool Permanent)
{
	patella::bridge::Talk().AddTalkFilter(Mode, Addressee, Channel, TextBuffer, List, Permanent);
}

char *GetBuddyIgnoreText()
{
	return patella::bridge::Talk().GetBuddyIgnoreText();
}

void ClearBuddyIgnore()
{
	patella::bridge::Talk().ClearBuddyIgnore();
}

void AddBuddyIgnore(const char *Addressee, bool Permanent)
{
	patella::bridge::Talk().AddBuddyIgnore(Addressee, Permanent);
}

bool IsPermanentBuddyIgnore(const char *Addressee)
{
	return patella::bridge::Talk().IsPermanentBuddyIgnore(Addressee);
}

void ClearTemporaryIgnore()
{
	patella::bridge::Talk().ClearTemporaryIgnore();
}

char *GetIgnoreListText()
{
	return patella::bridge::Talk().GetIgnoreListText();
}

bool IsModeIgnored(int Mode)
{
	return patella::bridge::Talk().IsModeIgnored(Mode);
}

void AddIgnoreFilter(int Mode, const char *Addressee, bool Permanent)
{
	patella::bridge::Talk().AddIgnoreFilter(Mode, Addressee, Permanent);
}

void IgnoreMode(int Mode)
{
	patella::bridge::Talk().IgnoreMode(Mode);
}

void IgnoreName(const char *Addressee, bool Permanent)
{
	patella::bridge::Talk().IgnoreName(Addressee, Permanent);
}

bool IsPermanentIgnore(const char *Addressee)
{
	return patella::bridge::Talk().IsPermanentIgnore(Addressee);
}

void RemoveIgnoreFilter(int Mode, const char *Addressee)
{
	patella::bridge::Talk().RemoveIgnoreFilter(Mode, Addressee);
}

void UnignoreMode(int Mode)
{
	patella::bridge::Talk().UnignoreMode(Mode);
}

void UnignoreName(const char *Addressee)
{
	patella::bridge::Talk().UnignoreName(Addressee);
}

void ClearDefaultTextBuffer()
{
	patella::bridge::Talk().ClearDefaultTextBuffer();
}

void ClearTextBuffer(TTalkBuffer *TextBuffer)
{
	patella::bridge::Talk().ClearTextBuffer(TextBuffer);
}

bool HasOwnPrivateChannel()
{
	return patella::bridge::Talk().HasOwnPrivateChannel();
}

void AddChannelNotice(const char *Addressee, const char *Text, int Mode, const char *Name, int Channel)
{
	patella::bridge::Talk().AddChannelNotice(Addressee, Text, Mode, Name, Channel);
}

void DeleteTextBuffer(TTalkBuffer *TextBuffer)
{
	patella::bridge::Talk().DeleteTextBuffer(TextBuffer);
}

TTalkBuffer *CreateTextBuffer(int ReceiveTalkMode, int SendTalkMode, const char *Name, int Channel)
{
	return patella::bridge::Talk().CreateTextBuffer(ReceiveTalkMode, SendTalkMode, Name, Channel);
}

void SetTalkChanged()
{
	patella::bridge::Talk().SetTalkChanged();
}

void StartTalkThrottle()
{
	patella::bridge::Talk().StartTalkThrottle();
}

void SetConsoleChanged()
{
	patella::bridge::Talk().SetConsoleChanged();
}

void AddTalkbox(int Mode, const char *Speaker, int x, int y, int z, const char *Text)
{
	patella::bridge::Talk().AddTalkbox(Mode, Speaker, x, y, z, Text);
}

void AddMessageTalkbox(int Mode, const char *Text)
{
	patella::bridge::Talk().AddMessageTalkbox(Mode, Text);
}

void UpdateTalkboxes()
{
	patella::bridge::Talk().UpdateTalkboxes();
}

void SetStatusMessage(const char *Message, bool Permanent)
{
	patella::bridge::Talk().SetStatusMessage(Message, Permanent);
}

void UpdateStatusMessage()
{
	patella::bridge::Talk().UpdateStatusMessage();
}

bool IsTalkIgnored(int Mode, const char *Name, int Channel)
{
	return patella::bridge::Talk().IsTalkIgnored(Mode, Name, Channel);
}

void AddTemporaryIgnore(const char *Addressee)
{
	patella::bridge::Talk().AddTemporaryIgnore(Addressee);
}

void RemoveTemporaryIgnore(const char *Addressee)
{
	patella::bridge::Talk().RemoveTemporaryIgnore(Addressee);
}

void ClearIgnoreList()
{
	patella::bridge::Talk().ClearIgnoreList();
}

void ShowPermanentIgnoreWarning()
{
	patella::bridge::Talk().ShowPermanentIgnoreWarning();
}

void AddTextBufferLine(TTalkBuffer *TargetBuffer, uint32 StatementID, const char *Text,
	int TalkMode, const char *Name, int Time)
{
	patella::bridge::Talk().AddTextBufferLine(TargetBuffer, StatementID, Text, TalkMode, Name, Time);
}

void AddConsoleText(uint32 StatementID, const char *Text, int TalkMode, const char *Name, int Channel, int Time)
{
	patella::bridge::Talk().AddConsoleText(StatementID, Text, TalkMode, Name, Channel, Time);
}

TTalkBuffer *GetTextBuffer(int Mode, const char *Name, int Channel)
{
	return patella::bridge::Talk().GetTextBuffer(Mode, Name, Channel);
}

void ResetTalk()
{
	patella::bridge::Talk().ResetTalk();
}

void InitTalk()
{
	patella::bridge::Talk().InitTalk();
}

void ExitTalk()
{
	patella::bridge::Talk().ExitTalk();
}
// NOLINTEND(cppcoreguidelines-no-malloc,cppcoreguidelines-owning-memory)
