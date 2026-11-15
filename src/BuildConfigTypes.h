// BuildConfigTypes.h: the types of the values a developer fixes at compile time. BuildConfigDefaults.h and
// BuildTextsDefaults.h include it, and so does a developer's own copy of either: keep its #include.
//
// Portable: nothing but the standard library.

#pragma once

#include <optional>

namespace patella::buildconfig {

// An option of Patella.ini (BuildConfigDefaults.h, the Option* values) is either the player's to choose, optional,
// or a value: then the client always uses that value, whatever Patella.ini says, and its control is not shown.
inline constexpr std::nullopt_t optional = std::nullopt;

// The hints ("event helps"): their numbers are the original's and what Patella.ini's KnownHints holds (Help.h HINT_*).
enum class THint : int {
	FirstLogin = 1,
	NpcCipfried = 2,
	RookgaardCrossroad = 3,
	NpcSeymour = 4,
	BuyingAndSelling = 5,
	Underground = 6,
	FirstExperiencePoints = 7,
	Level2 = 8,
	FirstSkillAdvance = 9,
	Level8 = 10,
	Continent = 11,
	Depot = 12,
	PlayerKilling = 13,
	Level20 = 14,
};

// A hint shown the first time a creature of this name (exactly, case included) is next to the player.
struct THintCreature {
	const char *Name;
	THint Hint;
};

// A hint shown the first time the player stands in this area: MinX to MaxX and MinY to MaxY, both ends included, on floor Z.
struct THintArea {
	int MinX;
	int MaxX;
	int MinY;
	int MaxY;
	int Z;
	THint Hint;
};

} // namespace patella::buildconfig
