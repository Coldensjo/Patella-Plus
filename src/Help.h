// Help.h: the help bubble of the help mode and the event helps (hints).

#pragma once

#include "types/FixedString.h"
#include <vector>

inline constexpr int EVENTHELPS = 14;

// The help numbers, named. Enumerators for the reason GUI.h gives at the modifier flags. No names of the original are
// known; each is taken from the title of the text Help.cpp returns for the number, or from the code that passes it.
//
// HELP_* are the Handle of SetHelpHandle, GetHelpHandle and GetHelpText: what the help bubble of the help mode
// explains. 10-23 are parts of the screen, which the windows' HelpMouseMove set. 1000-1002 are things on the map,
// which TMapWindow::HelpMouseMove sets. 1100-1112 are the lens help of an object type, read from Tibia.dat (LENSHELP,
// Objects.h), so only GetHelpText's case labels have them; a type without one has HELP_NONE (Objects.cpp).
enum : int {       // NOLINT(cppcoreguidelines-use-enum-class): int constants
	HELP_NONE = 0, // no bubble: an element that explains nothing, a field with nothing to explain
	// InitHelp's, until the first SetHelpHandle. Nothing sets it and it has no text, so that first call is always a
	// change, also when it sets HELP_NONE.
	HELP_UNSET = 1,
	HELP_AUTOMAP = 10,
	HELP_STATUS_BARS = 11,
	HELP_INVENTORY = 12,
	HELP_MINIMISED_INVENTORY = 13,
	HELP_COMBAT_CONTROLS_LARGE = 14, // TCombatControls with IsLarge: "Options" and "Help" are text buttons
	HELP_COMBAT_CONTROLS_SMALL = 15, // without IsLarge: the two are picture buttons
	HELP_CONTROL_BUTTONS = 16,       // Skills, Battle, VIP, Logout
	HELP_SKILLS = 17,
	HELP_BATTLE = 18,
	HELP_VIP = 19,
	HELP_OPEN_CONTAINERS = 20, // a container's window
	HELP_TRADE = 21,
	HELP_CONSOLE = 22,
	HELP_CONSOLE_CHANNELS = 23, // the console's tab bar
	HELP_CREATURES = 1000,      // a creature that is not the player
	HELP_YOUR_CHARACTER = 1001,
	HELP_CONTAINERS = 1002, // a container on the map without a lens help
	HELP_LADDERS = 1100,
	HELP_SEWER_GRATES = 1101,
	HELP_DUNGEON_FLOOR = 1102,
	HELP_LEVERS = 1103,
	HELP_DOORS = 1104,
	HELP_SPECIAL_DOORS = 1105,
	HELP_STAIRS = 1106,
	HELP_MAILBOXES = 1107,
	HELP_DEPOT_BOXES = 1108,
	HELP_DUSTBINS = 1109,
	HELP_STONE_PILES = 1110,
	HELP_SIGNS = 1111,
	HELP_BOOKS_AND_SCROLLS = 1112,
};

// HINT_* are the Number of AddEventHelp, GetEventHelpTitle and GetEventHelpText, what GetEventHelp and PopEventHelp
// return and what Preferences.cpp's KnownHints holds: the event helps, 1 to EVENTHELPS. The prefix is the original's
// other word for them ("unknown hint", the Hint of MapWindow.cpp's assertion), because EVENT_HELP and EVENT_HINTS
// are event numbers (Events.h). The names follow GetEventHelpTitle's titles. HINT_FIRST and HINT_LAST bound the loops
// of THintsDialog (Dialogs.cpp).
enum : int {       // NOLINT(cppcoreguidelines-use-enum-class): int constants
	HINT_NONE = 0, // GetEventHelp and PopEventHelp: the queue is empty
	HINT_FIRST_LOGIN = 1,
	HINT_FIRST = HINT_FIRST_LOGIN,
	HINT_NPC_CIPFRIED = 2,
	HINT_ROOKGAARD_CROSSROAD = 3,
	HINT_NPC_SEYMOUR = 4,
	HINT_BUYING_AND_SELLING = 5, // the shopkeepers of Rookgaard
	HINT_UNDERGROUND = 6,
	HINT_FIRST_EXPERIENCE_POINTS = 7,
	HINT_LEVEL_2 = 8,
	HINT_FIRST_SKILL_ADVANCE = 9,
	HINT_LEVEL_8 = 10,
	HINT_CONTINENT = 11, // the arrival on the mainland
	HINT_DEPOT = 12,
	HINT_PLAYER_KILLING = 13, // a creature with a skull
	HINT_LEVEL_20 = 14,       // "Level 20 and Security Information"
	HINT_LAST = HINT_LEVEL_20,
};

namespace patella {

// Help.cpp's state, with the file's functions as its members. TGUI owns it (GUI.h).
class THelp {
public:
	THelp() = default;
	THelp(const THelp &) = delete;
	THelp &operator=(const THelp &) = delete;
	THelp(THelp &&) = delete;
	THelp &operator=(THelp &&) = delete;
	~THelp() = default;

	void SetHelpPosition(int x, int y);
	void MoveHelpPosition(int dx, int dy);
	void SetHelpSize(int Width, int Height);
	void SetHelpHandle(int Handle);
	void GetHelpArea(int *x, int *y, int *Width, int *Height) const;
	[[nodiscard]] int GetHelpHandle() const;
	bool TakeHelpHandleChanged();
	void InitHelp();
	// Empty, so static (as TCommunication::ExitCommunication).
	static void ExitHelp();
	int GetEventHelp();
	int PopEventHelp();
	const char *GetHelpText(int Handle);
	void AddEventHelp(int Number);
	void CheckEventHelpName(const char *Name);
	void CheckEventHelpPosition(int x, int y, int z);
	const char *GetEventHelpTitle(int Number);
	const char *GetEventHelpText(int Number);
	const char *GetCurrentHelpText();

private:
	bool HelpHandleChanged{};
	int HelpHandle{}; // what the help bubble explains
	int HelpHeight{};
	int HelpWidth{};
	int HelpY{};
	int HelpX{};
	// A queue that appends at its end and pops at 0.
	std::vector<int> EventHelps;
	TFixedString<2048> HelpText{};
};

// The transition's access path: the one TClient's THelp (Client.cpp). It goes when no free function needs it.
namespace bridge {
THelp &Help();
} // namespace bridge

} // namespace patella

// Help.cpp's functions: each calls THelp's member of the same name on bridge::Help().
void SetHelpPosition(int x, int y);
void MoveHelpPosition(int dx, int dy);
void SetHelpSize(int Width, int Height);
void SetHelpHandle(int Handle);
void GetHelpArea(int *x, int *y, int *Width, int *Height);
int GetHelpHandle();
bool TakeHelpHandleChanged();
void InitHelp();
void ExitHelp();
int GetEventHelp();
int PopEventHelp();
void AddEventHelp(int Number);
void CheckEventHelpName(const char *Name);
void CheckEventHelpPosition(int x, int y, int z);
const char *GetEventHelpTitle(int Number);
const char *GetEventHelpText(int Number);
const char *GetCurrentHelpText();
