// Inventory.cpp: reconstructed from Tibia.exe 7.72.
//
// The body slots, open containers and trade offers. The functions without line evidence
// come where the binary has them: four at the start and InitInventory before
// ResetInventory. The others are in line order; the compiler placed
// InsertContainerObject last.
//
// Most functions assert before doing anything and have no try block; the
// original's catch wrappers (common.h) around the calls to other functions of this file
// are gone (issue #78).

#include "Inventory.h"
#include "Objects.h"
#include "Utils.h"
#include "common.h"
#include "types/FixedString.h"
#include <array>
#include <cstring>


// The data is TInventory's (Inventory.h, issue #106).

patella::TInventory::TInventory(TObjectTypes &ObjectTypesRef) : ObjectTypes(ObjectTypesRef) {}

void patella::TInventory::ClearBody()
{
	for (int i = 0; i < BODY_POSITIONS; i++)
		Body[i] = ObjectTypes.NoObject;
}

void patella::TInventory::ClearOffers()
{
	OwnOffer.Count = 0;
	CounterOffer.Count = 0;
}

int patella::TInventory::GetOwnOfferCount() const
{
	return OwnOffer.Count;
}

int patella::TInventory::GetCounterOfferCount() const
{
	return CounterOffer.Count;
}

// Positions count from 1.
void patella::TInventory::SetBodyObject(int Position, TObject Object)
{
	const int Pos = Position - 1;
	PATELLA_ASSERT_VALUES(In(Pos,0,BODY_POSITIONS-1), Pos);
	Body[Pos] = Object;
}

void patella::TInventory::DeleteBodyObject(int Position)
{
	SetBodyObject(Position, ObjectTypes.NoObject);
}

TObject *patella::TInventory::GetBodyObject(int Position)
{
	const int Pos = Position - 1;
	PATELLA_ASSERT_VALUES(In(Pos,0,BODY_POSITIONS-1), Pos);
	return &Body[Pos];
}

// The name is stored capitalised.
void patella::TInventory::OpenContainer(int Container, TObject Object, const char *Name, int Capacity, bool HasParent)
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT(!Containers[Container].IsOpen);
	PATELLA_ASSERT(Name!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(Name)<30, strlen(Name));
	Containers[Container].IsOpen = true;
	Containers[Container].Object = Object;
	Containers[Container].Name.Assign(Name);
	CapitalizeString(Containers[Container].Name);
	Containers[Container].HasParent = HasParent;
	Containers[Container].Count = 0;
	Containers[Container].Capacity = Capacity;
}

void patella::TInventory::CloseContainer(int Container)
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT(Containers[Container].IsOpen);
	Containers[Container].IsOpen = false;
}

bool patella::TInventory::IsContainerOpen(int Container) const
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	return Containers[Container].IsOpen;
}

// Puts the object in front; a full container loses its last object
// first.
void patella::TInventory::InsertContainerObject(int Container, TObject Object)
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT(Containers[Container].IsOpen);
	if (Containers[Container].Count >= CONTAINER_OBJECTS)
		DeleteContainerObject(Container, Containers[Container].Count - 1);
	for (int i = Containers[Container].Count; i > 0; i--)
		Containers[Container].Objects[i] = Containers[Container].Objects[i - 1];
	Containers[Container].Objects[0] = Object;
	Containers[Container].Count++;
}

// Ignored when the container is full.
void patella::TInventory::AppendContainerObject(int Container, TObject Object)
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT_VALUES(Containers[Container].IsOpen, Container);
	if (Containers[Container].Count < CONTAINER_OBJECTS) {
		Containers[Container].Objects[Containers[Container].Count] = Object;
		Containers[Container].Count++;
	}
}

// Ignored for a position outside the contents.
void patella::TInventory::ChangeContainerObject(int Container, int Position, TObject Object)
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT_VALUES(Containers[Container].IsOpen, Container);
	if (In(Position, 0, Containers[Container].Count - 1))
		Containers[Container].Objects[Position] = Object;
}

// True when the container was full, so the server can send the
// object that moves up into view.
bool patella::TInventory::DeleteContainerObject(int Container, int Position)
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT_VALUES(Containers[Container].IsOpen, Container);
	if (!In(Position, 0, Containers[Container].Count - 1))
		return false;
	for (int i = Position; i < Containers[Container].Count - 1; i++)
		Containers[Container].Objects[i] = Containers[Container].Objects[i + 1];
	Containers[Container].Count--;
	return Containers[Container].Count == CONTAINER_OBJECTS - 1;
}

int patella::TInventory::GetContainerObjectCount(int Container) const
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT_VALUES(Containers[Container].IsOpen, Container);
	return Containers[Container].Count;
}

// The container itself.
TObject patella::TInventory::GetContainerObject(int Container) const
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT_VALUES(Containers[Container].IsOpen, Container);
	return Containers[Container].Object;
}

TObject patella::TInventory::GetContainerContent(int Container, int Position) const
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT_VALUES(Containers[Container].IsOpen, Container);
	PATELLA_ASSERT_VALUES(In(Position,0,Containers[Container].Count-1), Position, Containers[Container].Count-1);
	return Containers[Container].Objects[Position];
}

char *patella::TInventory::GetContainerName(int Container)
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT_VALUES(Containers[Container].IsOpen, Container);
	return Containers[Container].Name;
}

int patella::TInventory::GetContainerCapacity(int Container) const
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT_VALUES(Containers[Container].IsOpen, Container);
	return Containers[Container].Capacity;
}

bool patella::TInventory::HasContainerParent(int Container) const
{
	PATELLA_ASSERT_VALUES(In(Container,0,OPEN_CONTAINERS-1), Container);
	PATELLA_ASSERT_VALUES(Containers[Container].IsOpen, Container);
	return Containers[Container].HasParent;
}

// The first closed container; 15 when all are open.
int patella::TInventory::GetFreeContainer() const
{
	int i = 0;
	for (i = 0; i < OPEN_CONTAINERS; i++) {
		if (!IsContainerOpen(i))
			return i;
	}
	return OPEN_CONTAINERS - 1;
}

bool patella::TInventory::SetOwnOffer(const char *Name)
{
	PATELLA_ASSERT_VALUES(OwnOffer.Count==0, OwnOffer.Count);
	PATELLA_ASSERT(Name!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(Name)<30, strlen(Name));
	OwnOffer.Name.Assign(Name);
	return true;
}

// False while a counter offer is shown.
bool patella::TInventory::SetCounterOffer(const char *Name)
{
	if (CounterOffer.Count != 0)
		return false;
	PATELLA_ASSERT(Name!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(Name)<30, strlen(Name));
	CounterOffer.Name.Assign(Name);
	return true;
}

void patella::TInventory::AddOwnOfferObject(TObject Object)
{
	PATELLA_ASSERT_VALUES(OwnOffer.Count<OBJECTS_PER_OFFER, OwnOffer.Count);
	OwnOffer.Objects[OwnOffer.Count] = Object;
	OwnOffer.Count++;
}

void patella::TInventory::AddCounterOfferObject(TObject Object)
{
	PATELLA_ASSERT_VALUES(CounterOffer.Count<OBJECTS_PER_OFFER, CounterOffer.Count);
	CounterOffer.Objects[CounterOffer.Count] = Object;
	CounterOffer.Count++;
}

char *patella::TInventory::GetOwnOfferName()
{
	PATELLA_ASSERT_VALUES(OwnOffer.Count>0, OwnOffer.Count);
	return OwnOffer.Name;
}

char *patella::TInventory::GetCounterOfferName()
{
	PATELLA_ASSERT_VALUES(CounterOffer.Count>0, CounterOffer.Count);
	return CounterOffer.Name;
}

TObject patella::TInventory::GetOwnOfferObject(int Position) const
{
	PATELLA_ASSERT_VALUES(In(Position,0,OwnOffer.Count-1), Position, OwnOffer.Count-1);
	return OwnOffer.Objects[Position];
}

TObject patella::TInventory::GetCounterOfferObject(int Position) const
{
	PATELLA_ASSERT_VALUES(In(Position,0,CounterOffer.Count-1), Position, CounterOffer.Count-1);
	return CounterOffer.Objects[Position];
}

void patella::TInventory::InitInventory()
{
	ClearBody();
	for (int i = 0; i < OPEN_CONTAINERS; i++)
		Containers[i].IsOpen = false;
	OwnOffer.Count = 0;
	CounterOffer.Count = 0;
}

void patella::TInventory::ResetInventory()
{
	InitInventory();
}

// Empty. ExitControl calls it where Inventory.cpp's exit belongs in the reverse
// order of InitControl (name ours, low confidence).
void patella::TInventory::ExitInventory()
{
}

// The original's functions (issue #106): each calls its member on the one TClient's TInventory.

void ClearOffers()
{
	patella::bridge::Inventory().ClearOffers();
}

int GetOwnOfferCount()
{
	return patella::bridge::Inventory().GetOwnOfferCount();
}

int GetCounterOfferCount()
{
	return patella::bridge::Inventory().GetCounterOfferCount();
}

TObject *GetBodyObject(int Position)
{
	return patella::bridge::Inventory().GetBodyObject(Position);
}

bool IsContainerOpen(int Container)
{
	return patella::bridge::Inventory().IsContainerOpen(Container);
}

int GetContainerObjectCount(int Container)
{
	return patella::bridge::Inventory().GetContainerObjectCount(Container);
}

TObject GetContainerObject(int Container)
{
	return patella::bridge::Inventory().GetContainerObject(Container);
}

TObject GetContainerContent(int Container, int Position)
{
	return patella::bridge::Inventory().GetContainerContent(Container, Position);
}

char *GetContainerName(int Container)
{
	return patella::bridge::Inventory().GetContainerName(Container);
}

int GetContainerCapacity(int Container)
{
	return patella::bridge::Inventory().GetContainerCapacity(Container);
}

bool HasContainerParent(int Container)
{
	return patella::bridge::Inventory().HasContainerParent(Container);
}

int GetFreeContainer()
{
	return patella::bridge::Inventory().GetFreeContainer();
}

char *GetOwnOfferName()
{
	return patella::bridge::Inventory().GetOwnOfferName();
}

char *GetCounterOfferName()
{
	return patella::bridge::Inventory().GetCounterOfferName();
}

TObject GetOwnOfferObject(int Position)
{
	return patella::bridge::Inventory().GetOwnOfferObject(Position);
}

TObject GetCounterOfferObject(int Position)
{
	return patella::bridge::Inventory().GetCounterOfferObject(Position);
}

void InitInventory()
{
	patella::bridge::Inventory().InitInventory();
}

void ResetInventory()
{
	patella::bridge::Inventory().ResetInventory();
}

void ExitInventory()
{
	patella::bridge::Inventory().ExitInventory();
}
