// Help.cpp: the help system: the help bubble of the help mode (the area and
// handle of the window under the magnifying glass, and its text), and the
// event helps ("hints") shown at certain places and NPCs. Patella-Plus: the
// texts, the places and the NPCs are BuildTexts.h's.

#include "Help.h"
#include "BuildTexts.h"
#include "BuildConfig.h"
#include "Preferences.h"
#include "Skins.h"
#include "common.h"
#include "types/FixedString.h"
#include <cstring>


// The data is THelp's (Help.h).

void patella::THelp::SetPosition(int x, int y)
{
	HelpX = x;
	HelpY = y;
}

void patella::THelp::MovePosition(int dx, int dy)
{
	HelpX += dx;
	HelpY += dy;
}

void patella::THelp::SetSize(int Width, int Height)
{
	HelpWidth = Width;
	HelpHeight = Height;
}

void patella::THelp::SetHandle(int Handle)
{
	const bool Changed = HelpHandle != Handle;
	HelpHandle = Handle;
	if (Changed)
		HelpHandleChanged = true;
}

void patella::THelp::GetArea(int *x, int *y, int *Width, int *Height) const
{
	*x = HelpX;
	*y = HelpY;
	*Width = HelpWidth;
	*Height = HelpHeight;
}

int patella::THelp::GetHandle() const
{
	return HelpHandle;
}

// Reports a change of the handle once.
bool patella::THelp::TakeHandleChanged()
{
	const bool Changed = HelpHandleChanged;
	HelpHandleChanged = false;
	return Changed;
}

void patella::THelp::Init()
{
	HelpHandle = HELP_UNSET;
	HelpHandleChanged = false;
	EventHelps.clear();
}

// The next event help to show, 0 if none.
int patella::THelp::GetEventHelp()
{
	if (EventHelps.empty())
		return HINT_NONE;
	return EventHelps.front();
}

// Takes the next event help from the queue.
int patella::THelp::PopEventHelp()
{
	if (EventHelps.empty())
		return HINT_NONE;
	const int Number = EventHelps.front();
	EventHelps.erase(EventHelps.begin());
	return Number;
}

// The help bubble text of a handle: 10-23 parts of the screen, 1000+ things
// in the game world. The texts are BuildTexts.h's (the original's unless a
// developer changed them), in std::format's language; the SKIN arguments are
// the skins of the buttons drawn inside the text, one for each {} of it, as
// BuildTexts.h checks. A text fits HelpText (BuildTexts.h MaxHelpText).
const char *patella::THelp::GetText(int Handle)
{
	switch (Handle) {
	case HELP_AUTOMAP:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpAutomap, int{SKIN_COMPASS_ROSE}, int{SKIN_HIGHER_FLOOR_BUTTON_UP}, int{SKIN_LOWER_FLOOR_BUTTON_UP}, int{SKIN_ZOOM_IN_BUTTON_UP}, int{SKIN_ZOOM_OUT_BUTTON_UP});
		break;
	case HELP_STATUS_BARS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpStatusBars);
		break;
	case HELP_INVENTORY:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpInventory, int{SKIN_MINIMISE_BUTTON_UP});
		break;
	case HELP_MINIMISED_INVENTORY:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpMinimisedInventory, int{SKIN_MAXIMISE_BUTTON_UP});
		break;
	case HELP_COMBAT_CONTROLS_LARGE:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpCombatControlsLarge, int{SKIN_OFFENSIVE_BUTTON_UP}, int{SKIN_BALANCED_BUTTON_UP}, int{SKIN_DEFENSIVE_BUTTON_UP}, int{SKIN_STAND_BUTTON_UP}, int{SKIN_CHASE_BUTTON_UP}, int{SKIN_SECURE_BUTTON_UP}, int{SKIN_SECURE_BUTTON_DOWN});
		break;
	case HELP_COMBAT_CONTROLS_SMALL:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpCombatControlsSmall, int{SKIN_OFFENSIVE_BUTTON_UP}, int{SKIN_BALANCED_BUTTON_UP}, int{SKIN_DEFENSIVE_BUTTON_UP}, int{SKIN_STAND_BUTTON_UP}, int{SKIN_CHASE_BUTTON_UP}, int{SKIN_SECURE_BUTTON_UP}, int{SKIN_SECURE_BUTTON_DOWN}, int{SKIN_OPTIONS_BUTTON_UP}, int{SKIN_HELP_BUTTON_UP});
		break;
	case HELP_CONTROL_BUTTONS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpControlButtons);
		break;
	case HELP_SKILLS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpSkills);
		break;
	case HELP_BATTLE:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpBattle);
		break;
	case HELP_VIP:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpVip);
		break;
	case HELP_OPEN_CONTAINERS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpOpenContainers, int{SKIN_HIGHER_CONTAINER_BUTTON_UP});
		break;
	case HELP_TRADE:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpTrade);
		break;
	case HELP_CONSOLE:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpConsole, int{SKIN_SAY_BUTTON_UP}, int{SKIN_WHISPER_BUTTON_UP}, int{SKIN_YELL_BUTTON_UP});
		break;
	case HELP_CONSOLE_CHANNELS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpConsoleChannels, int{SKIN_NEW_CHANNEL_BUTTON_UP}, int{SKIN_IGNORE_BUTTON_UP}, int{SKIN_CLOSE_CHANNEL_BUTTON_UP}, int{SKIN_SERVER_MESSAGES_BUTTON_UP});
		break;
	case HELP_CREATURES:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpCreatures);
		break;
	case HELP_YOUR_CHARACTER:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpYourCharacter);
		break;
	case HELP_CONTAINERS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpContainers);
		break;
	case HELP_LADDERS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpLadders);
		break;
	case HELP_SEWER_GRATES:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpSewerGrates);
		break;
	case HELP_DUNGEON_FLOOR:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpDungeonFloor);
		break;
	case HELP_LEVERS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpLevers);
		break;
	case HELP_DOORS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpDoors);
		break;
	case HELP_SPECIAL_DOORS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpSpecialDoors);
		break;
	case HELP_STAIRS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpStairs);
		break;
	case HELP_MAILBOXES:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpMailboxes);
		break;
	case HELP_DEPOT_BOXES:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpDepotBoxes);
		break;
	case HELP_DUSTBINS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpDustbins);
		break;
	case HELP_STONE_PILES:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpStonePiles);
		break;
	case HELP_SIGNS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpSigns);
		break;
	case HELP_BOOKS_AND_SCROLLS:
		TStringBuffer(HelpText, sizeof(HelpText)).Format(buildtexts::HelpBooksAndScrolls);
		break;
	default:
		TERROR1("unknown handle", Handle);
	}
	return HelpText;
}

// Queues an event help that was not shown before.
void patella::THelp::AddEventHelp(int Number)
{
	PATELLA_ASSERT_VALUES(In(Number,1,EVENTHELPS), Number);
	if (!Preferences().IsKnownHint(Number)) {
		Preferences().AddKnownHint(Number);
		EventHelps.push_back(Number);
	}
}

// The hint numbers of BuildConfigTypes.h, which the texts' trigger lists name, are Help.h's.
static_assert(static_cast<int>(patella::buildconfig::THint::FirstLogin) == HINT_FIRST_LOGIN
		&& static_cast<int>(patella::buildconfig::THint::NpcCipfried) == HINT_NPC_CIPFRIED
		&& static_cast<int>(patella::buildconfig::THint::RookgaardCrossroad) == HINT_ROOKGAARD_CROSSROAD
		&& static_cast<int>(patella::buildconfig::THint::NpcSeymour) == HINT_NPC_SEYMOUR
		&& static_cast<int>(patella::buildconfig::THint::BuyingAndSelling) == HINT_BUYING_AND_SELLING
		&& static_cast<int>(patella::buildconfig::THint::Underground) == HINT_UNDERGROUND
		&& static_cast<int>(patella::buildconfig::THint::FirstExperiencePoints) == HINT_FIRST_EXPERIENCE_POINTS
		&& static_cast<int>(patella::buildconfig::THint::Level2) == HINT_LEVEL_2
		&& static_cast<int>(patella::buildconfig::THint::FirstSkillAdvance) == HINT_FIRST_SKILL_ADVANCE
		&& static_cast<int>(patella::buildconfig::THint::Level8) == HINT_LEVEL_8
		&& static_cast<int>(patella::buildconfig::THint::Continent) == HINT_CONTINENT
		&& static_cast<int>(patella::buildconfig::THint::Depot) == HINT_DEPOT
		&& static_cast<int>(patella::buildconfig::THint::PlayerKilling) == HINT_PLAYER_KILLING
		&& static_cast<int>(patella::buildconfig::THint::Level20) == HINT_LEVEL_20
		&& patella::buildtexts::FirstHint == HINT_FIRST && patella::buildtexts::LastHint == HINT_LAST,
	"BuildConfigTypes.h THint and Help.h HINT_*");

// Event helps for the creatures next to the player: BuildTexts.h's HintCreatures (the original's NPCs of Rookgaard),
// every entry tested on its own.
void patella::THelp::CheckEventHelpName(const char *Name)
{
	PATELLA_ASSERT(Name!=nullptr);
	for (const patella::buildconfig::THintCreature &Entry : buildtexts::HintCreatures) {
		if (strcmp(Name, Entry.Name) == 0)
			AddEventHelp(static_cast<int>(Entry.Hint));
	}
}

// Event helps for places: BuildTexts.h's HintAreas (the original's Rookgaard crossroad, Rookgaard underground, arrivals
// on the mainland and depots, in its order), every area tested on its own, both ends included.
void patella::THelp::CheckEventHelpPosition(int x, int y, int z)
{
	for (const patella::buildconfig::THintArea &Entry : buildtexts::HintAreas) {
		if (x >= Entry.MinX && x <= Entry.MaxX && y >= Entry.MinY && y <= Entry.MaxY && z == Entry.Z)
			AddEventHelp(static_cast<int>(Entry.Hint));
	}
}

const char *patella::THelp::GetEventHelpTitle(int Number)
{
	PATELLA_ASSERT_VALUES(In(Number,1,EVENTHELPS), Number);
	switch (Number) {
	case HINT_FIRST_LOGIN:
		return buildtexts::HintTitleFirstLogin;
	case HINT_NPC_CIPFRIED:
		return buildtexts::HintTitleNpcCipfried;
	case HINT_ROOKGAARD_CROSSROAD:
		return buildtexts::HintTitleRookgaardCrossroad;
	case HINT_NPC_SEYMOUR:
		return buildtexts::HintTitleNpcSeymour;
	case HINT_BUYING_AND_SELLING:
		return buildtexts::HintTitleBuyingAndSelling;
	case HINT_UNDERGROUND:
		return buildtexts::HintTitleUnderground;
	case HINT_FIRST_EXPERIENCE_POINTS:
		return buildtexts::HintTitleFirstExperiencePoints;
	case HINT_LEVEL_2:
		return buildtexts::HintTitleLevel2;
	case HINT_FIRST_SKILL_ADVANCE:
		return buildtexts::HintTitleFirstSkillAdvance;
	case HINT_LEVEL_8:
		return buildtexts::HintTitleLevel8;
	case HINT_CONTINENT:
		return buildtexts::HintTitleContinent;
	case HINT_DEPOT:
		return buildtexts::HintTitleDepot;
	case HINT_PLAYER_KILLING:
		return buildtexts::HintTitlePlayerKilling;
	case HINT_LEVEL_20:
		return buildtexts::HintTitleLevel20;
	default:
		TERROR1("unknown hint", Number);
	}
}

namespace {

// The first hint names the site (BuildConfig.h): BuildTexts.h's text with SiteName in it.
constexpr auto WelcomeHelpText = patella::buildconfig::ComposeText<patella::buildconfig::ComposedSize(
	patella::buildtexts::HintFirstLoginBeforeSite, patella::buildconfig::SiteName, patella::buildtexts::HintFirstLoginAfterSite)>(
	patella::buildtexts::HintFirstLoginBeforeSite, patella::buildconfig::SiteName, patella::buildtexts::HintFirstLoginAfterSite);

} // namespace

const char *patella::THelp::GetEventHelpText(int Number)
{
	PATELLA_ASSERT_VALUES(In(Number,1,EVENTHELPS), Number);
	switch (Number) {
	case HINT_FIRST_LOGIN:
		return WelcomeHelpText.data();
	case HINT_NPC_CIPFRIED:
		return buildtexts::HintTextNpcCipfried;
	case HINT_ROOKGAARD_CROSSROAD:
		return buildtexts::HintTextRookgaardCrossroad;
	case HINT_NPC_SEYMOUR:
		return buildtexts::HintTextNpcSeymour;
	case HINT_BUYING_AND_SELLING:
		return buildtexts::HintTextBuyingAndSelling;
	case HINT_UNDERGROUND:
		return buildtexts::HintTextUnderground;
	case HINT_FIRST_EXPERIENCE_POINTS:
		return buildtexts::HintTextFirstExperiencePoints;
	case HINT_LEVEL_2:
		return buildtexts::HintTextLevel2;
	case HINT_FIRST_SKILL_ADVANCE:
		return buildtexts::HintTextFirstSkillAdvance;
	case HINT_LEVEL_8:
		return buildtexts::HintTextLevel8;
	case HINT_CONTINENT:
		return buildtexts::HintTextContinent;
	case HINT_DEPOT:
		return buildtexts::HintTextDepot;
	case HINT_PLAYER_KILLING:
		return buildtexts::HintTextPlayerKilling;
	case HINT_LEVEL_20:
		return buildtexts::HintTextLevel20;
	default:
		TERROR1("unknown hint", Number);
	}
}

// The text for the current help handle.
const char *patella::THelp::GetCurrentText()
{
	return GetText(HelpHandle);
}

// Empty. ExitControl calls it where Help.cpp's exit belongs in the reverse
// order of InitControl.
void patella::THelp::Exit()
{
}
