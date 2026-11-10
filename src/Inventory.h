// Inventory.h: the player's body slots, the open containers and the trade
// offers, as the server last sent them.

#pragma once

#include "Objects.h" // TObject
#include "types/FixedString.h"
#include <array>

inline constexpr int BODY_POSITIONS = 10;
inline constexpr int OPEN_CONTAINERS = 16;
inline constexpr int CONTAINER_OBJECTS = 36; // our name
inline constexpr int OBJECTS_PER_OFFER = 100;

struct TContainer {
	bool IsOpen;
	TObject Object; // the container itself
	TFixedString<30> Name;
	int Capacity;
	bool HasParent; // can move up
	int Count;
	std::array<TObject, CONTAINER_OBJECTS> Objects;
};

struct TOffer {
	TFixedString<30> Name; // the trading partner
	int Count;
	std::array<TObject, OBJECTS_PER_OFFER> Objects;
};
namespace patella {

class TObjectTypes;

// Inventory.cpp's data and functions: the body slots, the open containers and the two offers, every member 0 at the
// start; the functions are the members of the same names. TMapState owns it (MapState.h), last; it reads TObjectTypes'
// NoObject. The data is private.
class TInventory {
public:
	explicit TInventory(TObjectTypes &ObjectTypes);
	TInventory(const TInventory &) = delete;
	TInventory &operator=(const TInventory &) = delete;
	TInventory(TInventory &&) = delete;
	TInventory &operator=(TInventory &&) = delete;
	~TInventory() = default;

	void ClearBody();
	void ClearOffers();
	[[nodiscard]] int GetOwnOfferCount() const;
	[[nodiscard]] int GetCounterOfferCount() const;
	void SetBodyObject(int Position, TObject Object);
	void DeleteBodyObject(int Position);
	TObject *GetBodyObject(int Position);
	void OpenContainer(int Container, TObject Object, const char *Name, int Capacity, bool HasParent);
	void CloseContainer(int Container);
	[[nodiscard]] bool IsContainerOpen(int Container) const;
	void InsertContainerObject(int Container, TObject Object);
	void AppendContainerObject(int Container, TObject Object);
	void ChangeContainerObject(int Container, int Position, TObject Object);
	bool DeleteContainerObject(int Container, int Position);
	[[nodiscard]] int GetContainerObjectCount(int Container) const;
	[[nodiscard]] TObject GetContainerObject(int Container) const;
	[[nodiscard]] TObject GetContainerContent(int Container, int Position) const;
	char *GetContainerName(int Container);
	[[nodiscard]] int GetContainerCapacity(int Container) const;
	[[nodiscard]] bool HasContainerParent(int Container) const;
	[[nodiscard]] int GetFreeContainer() const;
	bool SetOwnOffer(const char *Name);
	bool SetCounterOffer(const char *Name);
	void AddOwnOfferObject(TObject Object);
	void AddCounterOfferObject(TObject Object);
	char *GetOwnOfferName();
	char *GetCounterOfferName();
	[[nodiscard]] TObject GetOwnOfferObject(int Position) const;
	[[nodiscard]] TObject GetCounterOfferObject(int Position) const;
	void InitInventory();
	void ResetInventory();
	static void ExitInventory();

private:
	TObjectTypes &ObjectTypes;

	std::array<TObject, BODY_POSITIONS> Body{};
	std::array<TContainer, OPEN_CONTAINERS> Containers{};
	TOffer OwnOffer{};
	TOffer CounterOffer{};
};

} // namespace patella

// The one client's inventory (Client.cpp).
patella::TInventory &Inventory();
