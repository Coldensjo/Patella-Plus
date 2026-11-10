// Communication.h: the senders of the client's packets and the handlers of the server's (Tibia 7.72's protocol).
//
// The data is TCommunication's, which TClient owns (Client.h); the free functions of the original's names are the
// bridge that the callers use until they hold the object (Communication(), defined in Client.cpp).

#pragma once

#include "common.h"
// Nothing of Talk.h is used here: it is included to keep the order in which the files that include this header declare
// its names.
#include "Talk.h" // IWYU pragma: keep
#include "Status.h"
#include "Protocol.h" // the packet types of the Send functions and of ReceivePackets (Control.cpp), named
#include <array>

namespace patella {

class TCreatures;
class TInventory;
class TMap;
class TMapState;
class TNetwork;
class TObjectTypes;
class TPlayerState;

// The signatures of Tibia.spr and Tibia.pic that the login packet sends (SendLogin): TGraphics' data (graphics.h), and
// TGraphics is made after TCommunication, so an interface this header declares, which Client.cpp's adapter implements
// by reading the one TClient's TGraphics. Communication.cpp does not include graphics.h (whose engine part is the
// platform's surface): patella_core compiles it.
class TLoginSignatures {
public:
	TLoginSignatures() = default;
	TLoginSignatures(const TLoginSignatures &) = delete;
	TLoginSignatures &operator=(const TLoginSignatures &) = delete;
	TLoginSignatures(TLoginSignatures &&) = delete;
	TLoginSignatures &operator=(TLoginSignatures &&) = delete;

	virtual uint32 SpriteSignature() = 0;
	virtual uint32 PictureSignature() = 0;

protected:
	~TLoginSignatures() = default;
};

// Communication.cpp's data and functions: the globals of the original, with Control.cpp's LastPacketIndex and
// LastPacketTypes, are the members, with the same initial values, and the functions the members of the same names;
// Status.cpp's session is its TSession (Status.h). It sends and reads through its TNetwork; the packet handlers write
// the game state through TMapState's five objects and call the windows through those files' functions.
class TCommunication {
public:
	TCommunication(TNetwork &NetworkRef, TMapState &MapState, TLoginSignatures &SignaturesRef);
	TCommunication(const TCommunication &) = delete;
	TCommunication &operator=(const TCommunication &) = delete;
	TCommunication(TCommunication &&) = delete;
	TCommunication &operator=(TCommunication &&) = delete;
	~TCommunication() = default;

	// Empty, so static.
	static void InitCommunication();
	static void ExitCommunication();
	void SendLogin(uint32 AccountID, const char *Password);
	void SendGameLogin(uint32 AccountID, const char *Name, const char *Password);
	void SendLogout();
	void SendPing();
	void PrepareMovement();
	void SendWalk();
	void SendStopWalk();
	void SendTurnNorth();
	void SendTurnEast();
	void SendTurnSouth();
	void SendTurnWest();
	void SendMoveObject(int x, int y, int z, int ObjectType, int Position, int DestinationX, int DestinationY, int DestinationZ, int Count);
	void SendRequestTrade(int x, int y, int z, int ObjectType, int Position, uint32 CreatureID);
	void SendInspectTrade(int Side, int Position);
	void SendAcceptTrade();
	void SendRejectTrade();
	void SendUseObject(int x, int y, int z, int ObjectType, int Position, int Container);
	void SendUseObjectWith(int x1, int y1, int z1, int ObjectType1, int Position1, int x2, int y2, int z2, int ObjectType2, int Position2);
	void SendUseObjectOnCreature(int x, int y, int z, int ObjectType, int Position, uint32 CreatureID);
	void SendRotateObject(int x, int y, int z, int ObjectType, int Position);
	void SendCloseContainer(int Container);
	void SendContainerUp(int Container);
	void SendEditText(uint32 ObjectNumber, const char *Text);
	void SendEditHouseText(int Type, uint32 Identifier, const char *Text);
	void SendLook(int x, int y, int z, int ObjectType, int Position);
	void SendTalk(int Mode, const char *Text);
	void SendTalk(int Mode, const char *Addressee, const char *Text);
	void SendTalk(int Mode, int ChannelID, const char *Text);
	void SendRequestChannels();
	void SendJoinChannel(int ChannelID);
	void SendLeaveChannel(int ChannelID);
	void SendOpenPrivateConversation(const char *Name);
	void SendProcessRuleViolation(const char *Name);
	void SendCloseRuleViolation(const char *Name);
	void SendCancelRuleViolation();
	void SendFightModes();
	void SendAttack(uint32 CreatureID);
	void SendFollow(uint32 CreatureID);
	void SendOpenPrivateChannel();
	void SendInviteToPrivateChannel(const char *Name);
	void SendExcludeFromPrivateChannel(const char *Name);
	void SendInviteToParty(uint32 CharacterID);
	void SendJoinParty(uint32 CharacterID);
	void SendRevokePartyInvitation(uint32 CharacterID);
	void SendPassPartyLeadership(uint32 CharacterID);
	void SendLeaveParty();
	void SendStopAttackAndWalk();
	void SendRefreshField(int x, int y, int z);
	void SendRefreshContainer(int Container);
	void SendRequestOutfit();
	void SendOutfit(int OutfitType, const int *Colors);
	void SendAddBuddy(const char *Name);
	void SendRemoveBuddy(uint32 CharacterID);
	void SendBugReport(const char *Text);
	void SendRuleViolationReport(const char *Name, uint8 Reason, uint8 Action, const char *Comment, uint32 StatementID, bool IPBanishment);
	void ReceiveLoginError();
	void ReceiveLoginMessage();
	void ReceiveMessageOfTheDay();
	void ReceiveUpdateRequest(); // packets 0x1e, 0x1f and 0x20 (the patcher removed)
	void ReceiveCharacterList();
	void ReceiveLoginServerOverloaded();
	void ReceiveGameLogin();
	void ReceiveGamemasterActions();
	void ReceiveGameError();
	void ReceiveGameMessage();
	void ReceiveLoginWait();
	void ReceivePing();
	uint32 ReadCreature(int Type, int x, int y, int z);
	void ReadField(int rx, int ry, int rz);
	void ReadMap(int xmin, int ymin, int xmax, int ymax);
	void ReceiveMap();
	void ReceiveMapNorth();
	void ReceiveMapEast();
	void ReceiveMapSouth();
	void ReceiveMapWest();
	void ReceiveField();
	void ReceiveAddObject();
	void ReceiveChangeObject();
	void ReceiveDeleteObject();
	void ReceiveMoveCreature();
	void ReceiveOpenContainer(int &Container);
	void ReceiveCloseContainer(int &Container);
	void ReceiveContainerAddObject();
	void ReceiveContainerChangeObject();
	void ReceiveContainerDeleteObject();
	void ReceiveInventoryObject();
	void ReceiveDeleteInventoryObject();
	void ReceiveOwnTrade();
	void ReceiveCounterTrade();
	void ReceiveCloseTrade();
	void ReceiveWorldLight();
	void ReceiveMagicEffect();
	void ReceiveAnimatedText();
	void ReceiveDistanceEffect();
	void ReceiveCreatureMark();
	void ReceiveCreatureHealth();
	void ReceiveCreatureLight();
	void ReceiveCreatureOutfit();
	void ReceiveCreatureSpeed();
	void ReceiveCreatureSkull();
	void ReceiveCreatureShield();
	void ReceiveEditText();
	void ReceiveEditHouseText();
	void ReceivePlayerStats();
	void ReceivePlayerSkills();
	void ReceivePlayerIcons();
	void ReceiveCancelAttack();
	void ReceiveTalk();
	void ReceiveChannelList();
	void ReceiveOpenChannel();
	void ReceiveOpenPrivateConversation();
	void ReceiveRuleViolationChannel();
	void ReceiveRemoveRuleViolation();
	void ReceiveCancelRuleViolation();
	void ReceiveLockRuleViolation();
	void ReceiveOwnChannel();
	void ReceiveCloseChannel();
	void ReceiveTextMessage();
	void ReceiveCancelWalk();
	void ReadFloor(int rz);
	void ReceiveFloorUp();
	void ReceiveFloorDown();
	void ReceiveOutfit();
	void ReceiveBuddy();
	void ReceiveBuddyLogin();
	void ReceiveBuddyLogout();

	// Data. Public: Control.cpp writes OutfitRequested and the packet types (ReceivePackets, InitWindow); Status.cpp's
	// functions reach Session through Session().
	bool OutfitRequested{};                  // also reset by Control.cpp
	int LastPacketIndex{};                   // Control.cpp's: next slot in LastPacketTypes
	std::array<uint8, 10> LastPacketTypes{}; // Control.cpp's: the last ten packet types, a ring
	TSession Session;                        // Status.cpp's

private:
	int SkipFields{};
	int CancelCount{};
	int LastCancelZ{};
	int LastCancelY{};
	int LastCancelX{};

	TNetwork &Network;
	// TMapState's objects.
	TObjectTypes &ObjectTypes;
	TMap &Map;
	TCreatures &Creatures;
	TPlayerState &PlayerState;
	TInventory &Inventory;
	TLoginSignatures &Signatures;

	// Control.cpp clears LastPacketTypes with a memset over its size.
	static_assert(sizeof(LastPacketTypes) == 10, "LastPacketTypes");
};

} // namespace patella

// The one client's communication (Client.cpp).
patella::TCommunication &Communication();

// The original's functions (the callers' names, and the golden tests'): each calls TCommunication's member of the
// same name on Communication().
void InitCommunication();
void ExitCommunication();
void SendLogin(uint32 AccountID, const char *Password);
void SendGameLogin(uint32 AccountID, const char *Name, const char *Password);
void SendLogout();
void SendWalk();
void SendStopWalk();
void SendTurnNorth();
void SendTurnEast();
void SendTurnSouth();
void SendTurnWest();
void SendMoveObject(int x, int y, int z, int ObjectType, int Position, int DestinationX, int DestinationY, int DestinationZ, int Count);
void SendRequestTrade(int x, int y, int z, int ObjectType, int Position, uint32 CreatureID);
void SendInspectTrade(int Side, int Position);
void SendAcceptTrade();
void SendRejectTrade();
void SendUseObject(int x, int y, int z, int ObjectType, int Position, int Container);
void SendUseObjectWith(int x1, int y1, int z1, int ObjectType1, int Position1, int x2, int y2, int z2, int ObjectType2, int Position2);
void SendUseObjectOnCreature(int x, int y, int z, int ObjectType, int Position, uint32 CreatureID);
void SendRotateObject(int x, int y, int z, int ObjectType, int Position);
void SendCloseContainer(int Container);
void SendContainerUp(int Container);
void SendEditText(uint32 ObjectNumber, const char *Text);
void SendEditHouseText(int Type, uint32 Identifier, const char *Text);
void SendLook(int x, int y, int z, int ObjectType, int Position);
void SendTalk(int Mode, const char *Text);
void SendTalk(int Mode, const char *Addressee, const char *Text);
void SendTalk(int Mode, int ChannelID, const char *Text);
void SendRequestChannels();
void SendJoinChannel(int ChannelID);
void SendLeaveChannel(int ChannelID);
void SendOpenPrivateConversation(const char *Name);
void SendProcessRuleViolation(const char *Name);
void SendCloseRuleViolation(const char *Name);
void SendCancelRuleViolation();
void SendFightModes();
void SendAttack(uint32 CreatureID);
void SendFollow(uint32 CreatureID);
void SendOpenPrivateChannel();
void SendInviteToPrivateChannel(const char *Name);
void SendExcludeFromPrivateChannel(const char *Name);
void SendInviteToParty(uint32 CharacterID);
void SendJoinParty(uint32 CharacterID);
void SendRevokePartyInvitation(uint32 CharacterID);
void SendPassPartyLeadership(uint32 CharacterID);
void SendLeaveParty();
void SendStopAttackAndWalk();
void SendRequestOutfit();
void SendOutfit(int OutfitType, const int *Colors);
void SendAddBuddy(const char *Name);
void SendRemoveBuddy(uint32 CharacterID);
void SendBugReport(const char *Text);
void SendRuleViolationReport(const char *Name, uint8 Reason, uint8 Action, const char *Comment, uint32 StatementID, bool IPBanishment);
void ReceiveLoginError();
void ReceiveLoginMessage();
void ReceiveMessageOfTheDay();
void ReceiveUpdateRequest(); // packets 0x1e, 0x1f and 0x20 (the patcher removed)
void ReceiveCharacterList();
void ReceiveLoginServerOverloaded();
void ReceiveGameLogin();
void ReceiveGamemasterActions();
void ReceiveGameError();
void ReceiveGameMessage();
void ReceiveLoginWait();
void ReceivePing();
void ReceiveMap();
void ReceiveMapNorth();
void ReceiveMapEast();
void ReceiveMapSouth();
void ReceiveMapWest();
void ReceiveField();
void ReceiveAddObject();
void ReceiveChangeObject();
void ReceiveDeleteObject();
void ReceiveMoveCreature();
void ReceiveOpenContainer(int &Container);
void ReceiveCloseContainer(int &Container);
void ReceiveContainerAddObject();
void ReceiveContainerChangeObject();
void ReceiveContainerDeleteObject();
void ReceiveInventoryObject();
void ReceiveDeleteInventoryObject();
void ReceiveOwnTrade();
void ReceiveCounterTrade();
void ReceiveCloseTrade();
void ReceiveWorldLight();
void ReceiveMagicEffect();
void ReceiveAnimatedText();
void ReceiveDistanceEffect();
void ReceiveCreatureMark();
void ReceiveCreatureHealth();
void ReceiveCreatureLight();
void ReceiveCreatureOutfit();
void ReceiveCreatureSpeed();
void ReceiveCreatureSkull();
void ReceiveCreatureShield();
void ReceiveEditText();
void ReceiveEditHouseText();
void ReceivePlayerStats();
void ReceivePlayerSkills();
void ReceivePlayerIcons();
void ReceiveCancelAttack();
void ReceiveTalk();
void ReceiveChannelList();
void ReceiveOpenChannel();
void ReceiveOpenPrivateConversation();
void ReceiveRuleViolationChannel();
void ReceiveRemoveRuleViolation();
void ReceiveCancelRuleViolation();
void ReceiveLockRuleViolation();
void ReceiveOwnChannel();
void ReceiveCloseChannel();
void ReceiveTextMessage();
void ReceiveCancelWalk();
void ReceiveFloorUp();
void ReceiveFloorDown();
void ReceiveOutfit();
void ReceiveBuddy();
void ReceiveBuddyLogin();
void ReceiveBuddyLogout();
