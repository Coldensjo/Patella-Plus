// BuildTexts.h: the texts a build for another server fixes at compile time, and their checks.
//
// The values are BuildTextsDefaults.h's, or those of a developer's own copy named by the CMake option PATELLA_BUILD_TEXTS
// (cmake/BuildConfig.cmake). Every consumer includes this header, so a value cannot skip the checks below: a bad one fails
// the compile with a message that says what is wrong. The help mode's texts are std::format strings with a button's
// picture for each {}: the checks count them, so a text cannot ask for an argument the client does not pass.
//
// Portable: nothing but the standard library.

#pragma once

#if defined(PATELLA_BUILD_TEXTS_FILE)
#include PATELLA_BUILD_TEXTS_FILE // IWYU pragma: export
#else
#include "BuildTextsDefaults.h" // IWYU pragma: export
#endif

#include "BuildConfigTypes.h"

#include <cstddef>
#include <string_view>

namespace patella::buildtexts {

// The help mode's buffer (Help.h THelp::HelpText, a TFixedString<2048>).
inline constexpr std::size_t MaxHelpText = 2047;
// The hints (Help.h EVENTHELPS), the longest creature name the client keeps (Creatures.h TCreature::Name, 32 bytes with
// the NUL), and the protocol's coordinates (16 bits) and floors (0 to 15).
inline constexpr int FirstHint = 1;
inline constexpr int LastHint = 14;
inline constexpr std::size_t MaxCreatureName = 31;
inline constexpr int MaxCoordinate = 65535;
inline constexpr int MaxFloor = 15;

namespace check {

// ASCII without the controls but the line feed (the client draws \n as a line break).
constexpr bool IsTextOrEmpty(const char *Value) noexcept
{
	if (Value == nullptr)
		return false;
	for (const char c : std::string_view(Value)) {
		if ((c < ' ' && c != '\n') || c > '~')
			return false;
	}
	return true;
}

constexpr bool IsText(const char *Value) noexcept
{
	return IsTextOrEmpty(Value) && Value[0] != '\0';
}

// The number of {} of a help text, or -1 when it is not text, is longer than MaxHelpText or has a brace that is not
// part of {}, {{ or }}.
constexpr int FormatPictures(const char *Value) noexcept
{
	if (!IsText(Value) || std::string_view(Value).size() > MaxHelpText)
		return -1;
	const std::string_view Text(Value);
	int Count = 0;
	for (std::size_t i = 0; i < Text.size(); i++) {
		if (Text[i] != '{' && Text[i] != '}')
			continue;
		if (i + 1 >= Text.size())
			return -1;
		if (Text[i] == '{' && Text[i + 1] == '}')
			Count++;
		else if (Text[i] != Text[i + 1])
			return -1;
		i++;
	}
	return Count;
}

// One line that a settings file reads as a comment.
constexpr bool IsComment(const char *Value) noexcept
{
	if (!IsText(Value) || (Value[0] != '#' && Value[0] != ';'))
		return false;
	for (const char c : std::string_view(Value)) {
		if (c == '\n')
			return false;
	}
	return true;
}

constexpr bool IsHint(buildconfig::THint Hint) noexcept
{
	return static_cast<int>(Hint) >= FirstHint && static_cast<int>(Hint) <= LastHint;
}

template <typename TList>
constexpr bool AreHintCreatures(const TList &List) noexcept
{
	for (const buildconfig::THintCreature &Entry : List) {
		if (!IsText(Entry.Name) || std::string_view(Entry.Name).size() > MaxCreatureName || !IsHint(Entry.Hint))
			return false;
		for (const char c : std::string_view(Entry.Name)) {
			if (c == '\n')
				return false;
		}
	}
	return true;
}

constexpr bool IsRange(int Min, int Max, int Limit) noexcept
{
	return Min >= 0 && Min <= Max && Max <= Limit;
}

template <typename TList>
constexpr bool AreHintAreas(const TList &List) noexcept
{
	for (const buildconfig::THintArea &Entry : List) {
		if (!IsRange(Entry.MinX, Entry.MaxX, MaxCoordinate) || !IsRange(Entry.MinY, Entry.MaxY, MaxCoordinate)
			|| !IsRange(Entry.Z, Entry.Z, MaxFloor) || !IsHint(Entry.Hint))
			return false;
	}
	return true;
}

} // namespace check

static_assert(check::IsText(ClientVersion), "ClientVersion must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(ErrorCaption), "ErrorCaption must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(InternalErrorCaption), "InternalErrorCaption must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(InternalErrorText), "InternalErrorText must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(StartErrorCaption), "StartErrorCaption must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(StartCorruptedText), "StartCorruptedText must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(StartAlreadyRunningText), "StartAlreadyRunningText must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(LeaveQuestion), "LeaveQuestion must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintsCaption), "HintsCaption must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(ClassicControlLabel), "ClassicControlLabel must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(NeverGiveAccountWarning), "NeverGiveAccountWarning must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleFirstLogin), "HintTitleFirstLogin must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleNpcCipfried), "HintTitleNpcCipfried must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleRookgaardCrossroad), "HintTitleRookgaardCrossroad must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleNpcSeymour), "HintTitleNpcSeymour must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleBuyingAndSelling), "HintTitleBuyingAndSelling must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleUnderground), "HintTitleUnderground must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleFirstExperiencePoints), "HintTitleFirstExperiencePoints must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleLevel2), "HintTitleLevel2 must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleFirstSkillAdvance), "HintTitleFirstSkillAdvance must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleLevel8), "HintTitleLevel8 must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleContinent), "HintTitleContinent must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleDepot), "HintTitleDepot must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitlePlayerKilling), "HintTitlePlayerKilling must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTitleLevel20), "HintTitleLevel20 must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintFirstLoginBeforeSite), "HintFirstLoginBeforeSite must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextNpcCipfried), "HintTextNpcCipfried must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextRookgaardCrossroad), "HintTextRookgaardCrossroad must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextNpcSeymour), "HintTextNpcSeymour must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextBuyingAndSelling), "HintTextBuyingAndSelling must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextUnderground), "HintTextUnderground must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextFirstExperiencePoints), "HintTextFirstExperiencePoints must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextLevel2), "HintTextLevel2 must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextFirstSkillAdvance), "HintTextFirstSkillAdvance must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextLevel8), "HintTextLevel8 must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextContinent), "HintTextContinent must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextDepot), "HintTextDepot must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextPlayerKilling), "HintTextPlayerKilling must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsText(HintTextLevel20), "HintTextLevel20 must be ASCII text that is not empty, with no control character but \n");
static_assert(check::IsTextOrEmpty(HintFirstLoginAfterSite), "HintFirstLoginAfterSite must be ASCII text, with no control character but \n");
static_assert(check::FormatPictures(HelpAutomap) == 5, "HelpAutomap must be text with exactly 5 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpStatusBars) == 0, "HelpStatusBars must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpInventory) == 1, "HelpInventory must be text with exactly 1 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpMinimisedInventory) == 1, "HelpMinimisedInventory must be text with exactly 1 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpCombatControlsLarge) == 7, "HelpCombatControlsLarge must be text with exactly 7 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpCombatControlsSmall) == 9, "HelpCombatControlsSmall must be text with exactly 9 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpControlButtons) == 0, "HelpControlButtons must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpSkills) == 0, "HelpSkills must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpBattle) == 0, "HelpBattle must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpVip) == 0, "HelpVip must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpOpenContainers) == 1, "HelpOpenContainers must be text with exactly 1 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpTrade) == 0, "HelpTrade must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpConsole) == 3, "HelpConsole must be text with exactly 3 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpConsoleChannels) == 4, "HelpConsoleChannels must be text with exactly 4 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpCreatures) == 0, "HelpCreatures must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpYourCharacter) == 0, "HelpYourCharacter must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpContainers) == 0, "HelpContainers must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpLadders) == 0, "HelpLadders must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpSewerGrates) == 0, "HelpSewerGrates must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpDungeonFloor) == 0, "HelpDungeonFloor must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpLevers) == 0, "HelpLevers must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpDoors) == 0, "HelpDoors must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpSpecialDoors) == 0, "HelpSpecialDoors must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpStairs) == 0, "HelpStairs must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpMailboxes) == 0, "HelpMailboxes must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpDepotBoxes) == 0, "HelpDepotBoxes must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpDustbins) == 0, "HelpDustbins must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpStonePiles) == 0, "HelpStonePiles must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpSigns) == 0, "HelpSigns must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::FormatPictures(HelpBooksAndScrolls) == 0, "HelpBooksAndScrolls must be text with exactly 0 {} (a button's picture each; {{ and }} for a brace) and at most 2047 characters");
static_assert(check::IsComment(SettingsFileComment), "SettingsFileComment must be one line of ASCII text starting with '#' or ';'");
static_assert(check::AreHintCreatures(HintCreatures), "HintCreatures: every name must be 1 to 31 ASCII characters without control characters, every hint a THint");
static_assert(check::AreHintAreas(HintAreas), "HintAreas: every area must have MinX <= MaxX and MinY <= MaxY within 0 to 65535, a floor from 0 to 15 and a THint");

} // namespace patella::buildtexts
