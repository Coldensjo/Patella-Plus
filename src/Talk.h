#pragma once
#include "common.h"
#include "types/File.h"
#include "types/FixedString.h"
#include "types/IndexedVector.h"
#include "Preferences.h"
#include <array>
#include <cstring>

struct TRGBColor;

// The talk modes: the mode byte of the talk and text message packets, and the mode of a talk box, a console line, a
// filter and a text buffer. The names without a comment are the original's, from its assertions. The seven with one
// are ours, each taken from what the client does with the mode. No code uses mode 26.
enum TALK_MODE : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants, the packets' talk modes ("The other ten stay unscoped")
	TALK_NONE = 0,     // no mode: a filter with it matches every mode, a buffer with it as send mode sends nothing
	TALK_SAY = 1,
	TALK_WHISPER = 2,
	TALK_YELL = 3,
	TALK_PRIVATE_MESSAGE = 4,
	TALK_CHANNEL_CALL = 5,
	TALK_GAMEMASTER_REQUEST = 6,
	TALK_GAMEMASTER_ANSWER = 7,
	TALK_PLAYER_ANSWER = 8,
	TALK_GAMEMASTER_BROADCAST = 9,
	TALK_GAMEMASTER_CHANNELCALL = 10,
	TALK_GAMEMASTER_MESSAGE = 11,
	TALK_ORANGE_CHANNELCALL = 12, // a channel call drawn in orange; like a gamemaster's, no ignore list holds it back
	TALK_ANONYMOUS_BROADCAST = 13,
	TALK_ANONYMOUS_CHANNELCALL = 14,
	TALK_ANONYMOUS_MESSAGE = 15,
	TALK_ANIMAL_LOW = 16,
	TALK_ANIMAL_LOUD = 17,
	TALK_ADMIN_MESSAGE = 18,
	TALK_EVENT_MESSAGE = 19,
	TALK_EVENT_STATUS_MESSAGE = 20, // on the status line, and in the console where GetEventMessages allows it
	TALK_STATUS_MESSAGE = 21,       // on the status line, and in the console where GetStatusMessages allows it
	TALK_INFO_MESSAGE = 22,
	TALK_STATUS_ONLY_MESSAGE = 23,  // on the status line and never in the console
	TALK_BLUE_CONSOLE_MESSAGE = 24, // in the console only, in blue; AddChannelNotice's, for what the player sent
	TALK_RED_CONSOLE_MESSAGE = 25,  // in the console only, in red
	TALK_MODE_BEYONDLAST = 27
};
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants ("The other ten stay unscoped")
	TALKBOXES = 10,
	STATUSMESSAGE_LENGTH = 512,
};
struct TTalkbox {
	int State, Order, RemainingTime, Mode;
	int x, y, z, Lines;
	std::array<TFixedString<40>, 10> Text;
	int Left, Top, Width, Height;
};
struct TTalkLine {
	char *Line;
	uint32 StatementID;
	int Mode;
	TTalkLine *Previous, *Next;
	TFixedString<30> Addressee;
	char *WithoutTime, *Text;
	TTalkLine(uint32 _StatementID, int _Mode, const char *_Line, int DiffTime);
	TTalkLine(uint32 _StatementID, int _Mode, const char *_Addressee, const char *_Line, int DiffTime);
	TTalkLine(const TTalkLine &) = delete;
	TTalkLine &operator=(const TTalkLine &) = delete;
	[[nodiscard]] const char *GetText() const { return Preferences().GetTimeStamps() ? Line : WithoutTime; }
};
class TTalkBuffer;
class TTalkBuffer {
public:
	TTalkBuffer(const TTalkBuffer &) = delete;
	TTalkBuffer &operator=(const TTalkBuffer &) = delete;
	// Closed is deliberately not initialized.
	TTalkBuffer(int TalkMode) : ReceiveTalkMode(TalkMode), SendTalkMode(TalkMode),
								Changed(true), first(nullptr), last(nullptr), cursor(nullptr), lines(0), limit(1000),
								ShowMessages(false), Enabled(true), Complete(true), LinesChanged(true), Locked(false), Selected(false) {}
	~TTalkBuffer();
	virtual bool TakeChanged()
	{
		const bool Result = Changed;
		Changed = false;
		return Result;
	}
	virtual bool TakeClosed()
	{
		// Closed is never set by the constructor (as in the original), so it holds whatever the heap had: read as the
		// byte the original tested (a load of a bool that holds another value is undefined, UBSan's report on Linux).
		unsigned char Byte = 0;
		std::memcpy(&Byte, &Closed, sizeof(Byte));
		const bool Result = Byte != 0;
		Closed = false;
		return Result;
	}
	virtual void Close() { Closed = true; }
	virtual bool CanWrite() { return true; }
	virtual bool SetWriteEnabled([[maybe_unused]] bool Value) { return true; }
	virtual void Append(TTalkLine *line);
	virtual void AddText(int Mode, const char *text, int DiffTime);
	virtual void Remove(TTalkLine *line);
	virtual void Remove(int lineNumber);
	virtual void Clear();
	// IsDefault: ConsoleWindow.cpp's assertion "GetTextBuffer()->IsDefault()==false".
	virtual bool IsDefault(); // TTalk's DefaultTextBuffer (below)
	virtual bool AcceptMessages() { return IsDefault(); }
	virtual bool IsRuleViolation() { return false; }
	virtual bool IsRuleViolationChannel() { return false; }
	virtual bool IsPrivateChannel() { return false; }
	virtual bool IsSelected() { return Selected; }
	virtual void SetSelected(bool Value) { Selected = Value; }
	virtual void SetLocked(bool Value) { Locked = Value; }
	virtual bool IsLocked() { return Locked; }
	virtual int GetReceiveTalkMode() { return ReceiveTalkMode; }
	virtual int GetSendTalkMode() { return SendTalkMode; }
	virtual const char *GetAddressee() { return nullptr; }
	virtual int GetChannelID() { return -1; }
	TTalkLine *GetLine(int lineNumber);
	TTalkLine *GetNext();
	// The cursor moves to the first or the last line.
	TTalkLine *GetFirst()
	{
		cursor = first;
		return cursor;
	}
	TTalkLine *GetLast()
	{
		cursor = last;
		return cursor;
	}
	int Write(TFile &F) const;
	int ReceiveTalkMode, SendTalkMode;
	bool Changed, Closed;
	TTalkLine *first, *last, *cursor;
	int lines, limit;
	bool ShowMessages, Enabled, Complete, LinesChanged, Locked, Selected;
};
class TPrivateTalkBuffer final : public TTalkBuffer {
public:
	TPrivateTalkBuffer(int TalkMode, const char *_Addressee);
	const char *GetAddressee() override { return Addressee; }
	TFixedString<30> Addressee;
};
class TChannelTalkBuffer : public TTalkBuffer {
public:
	TChannelTalkBuffer(int TalkMode, int _ChannelID);
	int GetChannelID() override { return ChannelID; }
	int ChannelID;
};
class TRuleViolationTalkBuffer final : public TTalkBuffer {
public:
	TRuleViolationTalkBuffer(int ReceiveTalkMode, int SendTalkMode, const char *_Addressee);
	bool SetWriteEnabled(bool Value) override
	{
		WriteEnabled = Value;
		return Value;
	}
	bool CanWrite() override { return WriteEnabled; }
	void Append(TTalkLine *line) override;
	void AddText(int Mode, const char *text, int DiffTime) override;
	bool IsRuleViolation() override { return true; }
	const char *GetAddressee() override { return Addressee; }
	TFixedString<30> Addressee;
	bool WriteEnabled;
};
class TRuleViolationChannel final : public TTalkBuffer {
public:
	TRuleViolationChannel(int TalkMode, int _ChannelID);
	int GetChannelID() override { return ChannelID; }
	int ChannelID;
	void Append(TTalkLine *line) override;
	void AddText([[maybe_unused]] int Mode, [[maybe_unused]] const char *text, [[maybe_unused]] int DiffTime) override
	{
		TERROR("not possible");
	}
	void Remove(int lineNumber) override;
	void Remove(TTalkLine *line) override;
	bool AcceptMessages() override { return true; }
	bool IsRuleViolationChannel() override { return true; }
};
class TPrivateChannel final : public TChannelTalkBuffer {
public:
	TPrivateChannel(int TalkMode, int _ChannelID);
	bool SetWriteEnabled(bool Value) override
	{
		WriteEnabled = Value;
		return Value;
	}
	bool CanWrite() override { return WriteEnabled; }
	bool IsPrivateChannel() override { return GetChannelID() >= 8; }
	bool WriteEnabled;
};
struct TTalkFilter {
	TTalkBuffer *TextBuffer;
	TFixedString<30> Addressee;
	int Mode, Channel;
	bool Permanent;
	bool Matches(int _Mode, const char *_Addressee, int _Channel);
};
namespace patella {

// Talk.cpp's state: the globals of the original are the members, with the same initial values, and the file's functions
// the members of the same names. TGUI owns it (GUI.h). It is TPreferences' TTalkLists: the White and Ignore lists that
// the options read and write.
class TTalk final : public TTalkLists {
public:
	TTalk() = default;
	TTalk(const TTalk &) = delete;
	TTalk &operator=(const TTalk &) = delete;
	TTalk(TTalk &&) = delete;
	TTalk &operator=(TTalk &&) = delete;
	~TTalk() = default;

	bool TakeTalkChanged();
	[[nodiscard]] int GetNumberOfTalkboxes() const;
	[[nodiscard]] const char *GetStatusMessage() const;
	void SetIgnoreEnabled(bool Value);
	[[nodiscard]] bool GetIgnoreEnabled() const;
	void SetBuddyIgnore(bool Value);
	[[nodiscard]] bool GetBuddyIgnore() const;
	void SetIgnoreListEnabled(bool Value);
	[[nodiscard]] bool GetIgnoreListEnabled() const;
	bool IsTalkThrottled();
	bool TakeConsoleChanged();
	TRGBColor *GetTalkColor(TRGBColor *Color, int TalkMode);
	TTalkbox *GetTalkbox(int Number);
	int GetTalkboxMode(int Number);
	void GetTalkboxPosition(int Number, int *x, int *y, int *z);
	void GetTalkboxRectangle(int Number, int *x, int *y, int *Width, int *Height);
	TRGBColor *GetTalkboxColor(TRGBColor *Color, int Number);
	int GetTalkboxLines(int Number);
	const char *GetTalkboxLine(int Number, int Line);
	void RemoveTalkbox(int Position);
	int FindFreeTalkbox();
	void WrapTalkbox(TTalkbox *Talkbox, const char *Text);
	void SetTalkboxRectangle(int Number, int x, int y, int Width, int Height);
	void RemoveOldestTalkbox();
	int FindTalkFilter(int Mode, const char *Addressee, int Channel, TIndexedVector<TTalkFilter *> &List);
	int FindTextBuffer(TTalkBuffer *TextBuffer, TIndexedVector<TTalkFilter *> &List);
	void AddTalkFilter(int Mode, const char *Addressee, int Channel, TTalkBuffer *TextBuffer,
		TIndexedVector<TTalkFilter *> &List, bool Permanent);
	char *GetBuddyIgnoreText() override;
	void ClearBuddyIgnore();
	void AddBuddyIgnore(const char *Addressee, bool Permanent) override;
	bool IsPermanentBuddyIgnore(const char *Addressee) override;
	void ClearTemporaryIgnore();
	char *GetIgnoreListText() override;
	bool IsModeIgnored(int Mode);
	void AddIgnoreFilter(int Mode, const char *Addressee, bool Permanent);
	void IgnoreMode(int Mode);
	void IgnoreName(const char *Addressee, bool Permanent) override;
	bool IsPermanentIgnore(const char *Addressee) override;
	void RemoveIgnoreFilter(int Mode, const char *Addressee);
	void UnignoreMode(int Mode);
	void UnignoreName(const char *Addressee);
	void ClearDefaultTextBuffer();
	void ClearTextBuffer(TTalkBuffer *TextBuffer);
	bool HasOwnPrivateChannel();
	void AddChannelNotice(const char *Addressee, const char *Text, int Mode, const char *Name, int Channel);
	void DeleteTextBuffer(TTalkBuffer *TextBuffer);
	TTalkBuffer *CreateTextBuffer(int ReceiveTalkMode, int SendTalkMode, const char *Name, int Channel);
	void SetTalkChanged();
	void StartTalkThrottle();
	void SetConsoleChanged();
	void AddTalkbox(int Mode, const char *Speaker, int x, int y, int z, const char *Text);
	void AddMessageTalkbox(int Mode, const char *Text);
	void UpdateTalkboxes();
	void SetStatusMessage(const char *Message, bool Permanent);
	void UpdateStatusMessage();
	bool IsTalkIgnored(int Mode, const char *Name, int Channel);
	void AddTemporaryIgnore(const char *Addressee);
	void RemoveTemporaryIgnore(const char *Addressee);
	void ClearIgnoreList() override;
	void ShowPermanentIgnoreWarning();
	void AddTextBufferLine(TTalkBuffer *TargetBuffer, uint32 StatementID, const char *Text,
		int TalkMode, const char *Name, int Time);
	void AddConsoleText(uint32 StatementID, const char *Text, int TalkMode, const char *Name, int Channel, int Time);
	TTalkBuffer *GetTextBuffer(int Mode, const char *Name, int Channel);
	void ResetTalk();
	void InitTalk();
	void ExitTalk();


private:
	bool IgnoreListEnabled{};
	bool BuddyIgnore{};
	bool IgnoreEnabled{};
	bool ConsoleChanged{};
	int StatusMessageTime{};
	TFixedString<STATUSMESSAGE_LENGTH> StatusMessage{};
	int TalkOrder{};
	bool TalkChanged{};
	std::array<TTalkbox, TALKBOXES> Talkboxes{};

public:
	// Public: MainWindow.cpp, the map's console (Client.cpp) and TTalkBuffer::IsDefault read it.
	TTalkBuffer *DefaultTextBuffer{};

private:
	int TalkThrottle{};
	TIndexedVector<TTalkFilter *> BuddyExemptionList{0, 9, 10, nullptr};
	TIndexedVector<TTalkFilter *> TemporaryIgnoreList{0, 9, 10, nullptr};
	TIndexedVector<TTalkFilter *> IgnoreList{0, 9, 10, nullptr};
	TIndexedVector<TTalkFilter *> ConsoleFilterList{0, 4, 5, nullptr};
	int LastTime{};     // UpdateTalkboxes' static
	bool LastTimeSet{}; // its guard
};

} // namespace patella

// The one client's talk (Client.cpp).
patella::TTalk &Talk();

// Whether this is the console's default buffer.
inline bool TTalkBuffer::IsDefault()
{
	return this == Talk().DefaultTextBuffer;
}
