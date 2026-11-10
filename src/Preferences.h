// Preferences.h: the options, stored in Tibia.cfg by the original and in Patella.ini by this client (Preferences.cpp).
// The option names in the comments are the keys that the Tibia.cfg loader uses, which give the names of the members.
//
// The data is TPreferences', which TClient owns (Client.h); a caller that holds no reference to it calls
// Preferences() (Client.cpp).

#pragma once

#include "common.h" // IWYU pragma: keep (first: it configures the CRT for the C headers of types/FixedString.h)
#include "types/IndexedVector.h"
#include "platform/FileSystem.h"
#include "settings/PreferenceKeys.h"
#include "types/FixedString.h"
#include <array>
#include <cstddef>
#include <vector>

// From the assertions.
inline constexpr int HOTKEYS = 36;
inline constexpr int HOTKEY_LENGTH = 256;
inline constexpr int BUDDYDESCRIPTION_LENGTH = 128;
inline constexpr int SKILLS = 7;

// The fight modes of the combat controls (ATTACK_*, CHASE_*) are in settings/PreferenceKeys.h, because the reader
// of the configuration names them too.

// "FullscreenMode = (Width,Height,Depth)"; the fourth value is not saved.
struct TFullscreenMode {
	int Width;
	int Height;
	int Depth;
	int Frequency;
};

// "ContentWindow = (ID,Size)": the stored size of a side-bar window.
struct TContentWindowSize {
	int ID;
	int Size;
};

// "Buddy = (CharacterID,Icon,Description,Notify)", 0x8c bytes. Notify
// shows "%s has logged in."; CharacterID 0 marks a deleted entry.
struct TBuddy {
	// strcpy(Description, Text) as the original's Tibia.cfg reader wrote it: the text and its NUL within the rest of
	// the entry (Description, Notify and the padding, 132 bytes). A description of 128 characters is kept whole, its
	// NUL in Notify, which the reader sets next, and up to 131 characters the bytes are the original's; a longer one,
	// which the original wrote past the entry, is cut at 131.
	void AssignDescriptionRead(const char *Text) noexcept
	{
		TStringBuffer(reinterpret_cast<char *>(this) + offsetof(TBuddy, Description), sizeof(TBuddy) - offsetof(TBuddy, Description))
			.Assign(Text);
	}
	// The description as a C string, at most its 128 characters (a deliberate difference): one of 128 has no NUL of its
	// own (its 129th byte is Notify), and Tibia.exe's readers read on past it (with Notify set, into the padding and
	// the next entry).
	[[nodiscard]] TFixedString<BUDDYDESCRIPTION_LENGTH + 1> DescriptionText() const noexcept
	{
		TFixedString<BUDDYDESCRIPTION_LENGTH + 1> Text;
		Text.Assign(Description.View());
		return Text;
	}

	int CharacterID;
	int Icon;
	TFixedString<BUDDYDESCRIPTION_LENGTH> Description;
	bool Notify;
};
// AssignDescriptionRead writes through the entry's bytes from Description on: Notify and the padding follow it.
static_assert(sizeof(TBuddy) == 0x8c && offsetof(TBuddy, Description) == 0x08 && offsetof(TBuddy, Notify) == 0x88,
	"TBuddy's layout");

namespace patella {

// The two lists of Talk.cpp that the options hold (Tibia.cfg's White and Ignore): TPreferences reaches them only
// through this, so that it does not depend on Talk. The functions are Talk.h's of the same names.
class TTalkLists {
public:
	TTalkLists() = default;
	TTalkLists(const TTalkLists &) = delete;
	TTalkLists &operator=(const TTalkLists &) = delete;
	TTalkLists(TTalkLists &&) = delete;
	TTalkLists &operator=(TTalkLists &&) = delete;
	virtual void ClearIgnoreList() = 0;
	virtual char *GetBuddyIgnoreText() = 0;
	virtual void AddBuddyIgnore(const char *Addressee, bool Permanent) = 0;
	virtual bool IsPermanentBuddyIgnore(const char *Addressee) = 0;
	virtual char *GetIgnoreListText() = 0;
	virtual void IgnoreName(const char *Addressee, bool Permanent) = 0;
	virtual bool IsPermanentIgnore(const char *Addressee) = 0;

protected:
	~TTalkLists() = default;
};

// Preferences.cpp's data and functions: the globals of the original are the members, with the same initial values (the
// others 0), and the functions the members of the same names. The ones that reach Talk's lists take them as Lists. The
// files are the platform's (IFileSystem, platform/FileSystem.h; Tibia.cfg's existence; Script.cpp opens the file
// itself).
class TPreferences {
public:
	explicit TPreferences(patella::platform::IFileSystem &FileSystemRef) : FileSystem(FileSystemRef) {}
	TPreferences(const TPreferences &) = delete;
	TPreferences &operator=(const TPreferences &) = delete;
	TPreferences(TPreferences &&) = delete;
	TPreferences &operator=(TPreferences &&) = delete;
	~TPreferences() = default;

	[[nodiscard]] bool GetTestServerLogin() const;
	void SetTestServerLogin(bool Value);
	void SetViewMode(int Value);
	void SetFullscreenMode(const TFullscreenMode *Mode);
	void SetWindowsMousepointer(bool Value);
	void SetLightEffects(bool Value);
	void SetTextualEffects(bool Value);
	void SetCreatureInfo(int Value);
	void SetNoStretching(bool Value);
	void SetDefaultNoStretching(bool Value);
	void SetConfineFramerate(int Value);
	void SetEngine(int Value);
	[[nodiscard]] int GetViewMode() const;
	TFullscreenMode *GetFullscreenMode();
	[[nodiscard]] bool GetWindowsMousepointer() const;
	[[nodiscard]] bool GetLightEffects() const;
	[[nodiscard]] bool GetTextualEffects() const;
	[[nodiscard]] int GetCreatureInfo() const;
	[[nodiscard]] bool GetNoStretching() const;
	[[nodiscard]] int GetConfineFramerate() const;
	[[nodiscard]] int GetEngine() const;
	void SetWindowedMode(int x, int y, int Width, int Height, bool Maximized);
	void GetWindowedMode(int *x, int *y, int *Width, int *Height, bool *Maximized) const;
	void SetConsoleHeight(int Value);
	[[nodiscard]] int GetConsoleHeight() const;
	void SetAutomapZoom(int Value);
	[[nodiscard]] int GetAutomapZoom() const;
	void SetBuddySortmethods(const int *Methods);
	void GetBuddySortmethods(int *Methods) const;
	void SetBuddyHideOffline(bool Value);
	[[nodiscard]] bool GetBuddyHideOffline() const;
	void ClearOpenDialogs();
	[[nodiscard]] const char *GetMessageOfTheDay() const;
	void SetInfoMessages(bool Value);
	void SetEventMessages(bool Value);
	void SetStatusMessages(bool Value);
	void SetTimeStamps(bool Value);
	void SetPrivateMessages(bool Value);
	[[nodiscard]] bool GetInfoMessages() const;
	[[nodiscard]] bool GetEventMessages() const;
	[[nodiscard]] bool GetStatusMessages() const;
	[[nodiscard]] bool GetTimeStamps() const;
	[[nodiscard]] bool GetPrivateMessages() const;
	void ClearOpenChannels();
	void SetClassicControl(bool Value);
	void SetAutoChaseOff(bool Value);
	[[nodiscard]] bool GetClassicControl() const;
	[[nodiscard]] bool GetAutoChaseOff() const;
	void SetDragFullStacks(bool Value);
	[[nodiscard]] bool GetDragFullStacks() const;
	[[nodiscard]] const patella::settings::TServerValues &GetServerValues() const;
	[[nodiscard]] const char *GetRsaModulus() const;
	void SetAttackMode(int Value);
	void SetChaseMode(int Value);
	void SetSecureMode(bool Value);
	[[nodiscard]] int GetAttackMode() const;
	[[nodiscard]] int GetChaseMode() const;
	[[nodiscard]] bool GetSecureMode() const;
	void SetShowNewHints(bool Value);
	[[nodiscard]] bool GetShowNewHints() const;
	[[nodiscard]] bool GetLevelBar() const;
	[[nodiscard]] bool GetMagLevelBar() const;
	void SetLevelBar(bool Value);
	void SetMagLevelBar(bool Value);
	void SetDefaults(TTalkLists &Lists);
	void SetContentWindowSize(int ID, int Size);
	int GetContentWindowSize(int ID);
	int GetOpenDialogs(int **List);
	int GetOpenChannels(int **List);
	void AddKnownHint(int Hint);
	bool IsKnownHint(int Hint);
	void SetHotkey(int Number, const char *Text, bool AutoSend);
	const char *GetHotkey(int Number, bool *AutoSend) const;
	void AddOpenDialog(int Identifier);
	bool SetMessageOfTheDay(const char *Text);
	void AddOpenChannel(int Identifier);
	int FindBuddy(int CharacterID);
	void SetBuddy(int CharacterID, int Icon, const char *Description, bool Notify);
	void DeleteBuddy(int CharacterID);
	int GetBuddyIcon(int CharacterID);
	const char *GetBuddyDescription(int CharacterID);
	bool GetBuddyNotify(int CharacterID);
	[[nodiscard]] bool GetSkillBar(int SkillNumber) const;
	void SetSkillBar(int SkillNumber, bool Value);
	void ReadTibiaCfg(TTalkLists &Lists);
	patella::settings::TPreferenceValues GetValues(TTalkLists &Lists);
	void SetValues(const patella::settings::TPreferenceValues &Values, TTalkLists &Lists);
	// Init, SavePreferences and Exit read and write Patella.ini (settings/PreferenceKeys.h); the
	// client never writes Tibia.cfg. Lists is the one client's TTalk.
	void Init(TTalkLists &Lists);
	void SavePreferences(TTalkLists &Lists);
	void Exit(TTalkLists &Lists);

private:
	int WindowedWidth = 600; // "WindowedMode = (x,y,Width,Height,Maximized)"
	int WindowedHeight = 400;
	std::array<bool, SKILLS> SkillBars{}; // "SkillBars = {...}"
	bool MagLevelBar{};
	bool LevelBar{};
	bool ShowNewHints{};
	// NumberOfKnownHints and NumberOfBuddyEntries: KnownHints.size() and Buddies.size().
	bool SecureMode{};
	int ChaseMode{};
	int AttackMode{};
	bool AutoChaseOff{};
	bool ClassicControl{};
	bool DragFullStacks{}; // Patella-Plus, not in the original
	// NumberOfOpenChannels: OpenChannels.size().
	bool PrivateMessages{};
	bool TimeStamps{};
	bool StatusMessages{};
	bool EventMessages{};
	bool InfoMessages{};
	char *MotD = nullptr;      // text of the message of the day
	int LastMotD{};            // its number
	int NumberOfOpenDialogs{}; // "OpenDialogs = ..."
	bool BuddyHideOffline{};
	std::array<int, 3> BuddySortmethods{};
	// NumberOfContentWindows: ContentWindows.size().
	int AutomapZoom{};
	int ConsoleHeight{};
	bool WindowedMaximized{};
	std::array<bool, HOTKEYS> HotkeyAutoSend{};                 // saved as a '\n' after the text
	std::array<TFixedString<HOTKEY_LENGTH>, HOTKEYS> Hotkeys{}; // "Hotkey = (Number,Text)"
	int Engine{};                                               // -1 = not chosen
	int ConfineFramerate{};
	int NoStretching{}; // 0 or 1, 2 = not set yet
	int CreatureInfo{};
	bool TextualEffects{};
	bool LightEffects{};
	bool WindowsMousepointer{};
	TFullscreenMode FullscreenMode{}; // Width 0 = not set
	int ViewMode{};
	bool TestServerLogin{};   // "TestServer = yes"
	bool PreferencesLoaded{}; // set by Init; SavePreferences writes only then
	int WindowedX{};
	int WindowedY{};
	// The original's vector<T>(0, 9, 10) each, with the counts above: the lists that append at their count and read
	// below it are std::vectors, their size the count. OpenDialogs stays a TIndexedVector: ReadTibiaCfg's duplicate
	// check reads its entries from min to max - 1, the init fill (-1) and the entries past NumberOfOpenDialogs that
	// ClearOpenDialogs left.
	std::vector<TContentWindowSize> ContentWindows; // "ContentWindow = (...)"
	TIndexedVector<int> OpenDialogs{0, 9, 10, -1};
	std::vector<int> OpenChannels; // "OpenChannel = ..."
	std::vector<TBuddy> Buddies;   // "Buddy = (...)", deleted entries included
	std::vector<int> KnownHints;   // "KnownHints = {...}"

	patella::platform::IFileSystem &FileSystem;
	patella::settings::TServerValues ServerValues; // Patella-Plus, not in the original
	// Not the original's: GetBuddyDescription's text, the entry's description with a NUL.
	TFixedString<BUDDYDESCRIPTION_LENGTH + 1> BuddyDescription{};

	// SetDefaults clears HotkeyAutoSend with a memset over its size.
	static_assert(sizeof(HotkeyAutoSend) == HOTKEYS, "HotkeyAutoSend");
};

} // namespace patella

// The one client's preferences (Client.cpp).
patella::TPreferences &Preferences();
