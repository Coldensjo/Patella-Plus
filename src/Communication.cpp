// Communication.cpp: reconstructed from Tibia.exe 7.72.
// The client-to-server packets (Send*) and the handlers of the server's
// packets (Receive*), which ReceivePackets in Control.cpp dispatches.
// Functions are in original source order from the assertion and catch line
// numbers; see notes/Communication.md for the details of this file.
//
// The original's catch wrappers (common.h) are gone since issue #78; the
// functions here have no handlers apart from their own: the encryption
// failures of SendLogin and SendGameLogin (TRSA's const char *) and
// ReceiveMoveCreature's cylinder loop (an error while listing the fields
// goes into the message).

#include "Communication.h"
#include "Help.h"
#include "Inventory.h"
#include "Creatures.h"
#include "Map.h"
#include "MapState.h"
#include "Network.h"
#include "Objects.h"
#include "Utils.h"
#include "TRSA.h"
#include "Preferences.h"
#include "Status.h"
#include "Talk.h"
#include "Text.h"
#include "common.h"
#include "compat/span.h"
#include "types/FixedString.h"
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>

// The data is TCommunication's (Communication.h, issue #105), in the original's order with its addresses.

namespace {

// The login server's request to update the client (issue #75): the patch packets end the login with this text in the
// login error dialog.
constexpr std::string_view TextUpdateRefused = "This server wants to update the client";

} // namespace

patella::TCommunication::TCommunication(TNetwork &NetworkRef, TMapState &MapState, TLoginSignatures &SignaturesRef)
	: Network(NetworkRef), ObjectTypes(MapState.ObjectTypes), Map(MapState.Map), Creatures(MapState.Creatures),
	  PlayerState(MapState.PlayerState), Inventory(MapState.Inventory), Signatures(SignaturesRef)
{
}

static_assert(offsetof(TCreature, Direction) == 0x50, "creature direction");
static_assert(offsetof(TCreature, OutfitType) == 0x60, "creature outfit");
static_assert(offsetof(TCreature, PKFlag) == 0x90, "creature skull");

// Called by InitControl. The original cleared the patch transfer's buffer; since issue #75 (the patcher
// removed) there is nothing left to set up.
void patella::TCommunication::InitCommunication()
{
}

void patella::TCommunication::SendLogin(uint32 AccountID, const char *Password)
{
	PATELLA_ASSERT(Password!=nullptr);
	std::array<uint8, 16> Key;
	for (int i = 0; i < 16; i++)
		Key[i] = (uint8)rand();
	Network.SetSymmetricKey(Key.data());
	std::array<uint8, 128> Data;
	for (int i = 0; i < 128; i++)
		Data[i] = (uint8)rand();
	TWriteBuffer Buffer(Data);
	Buffer.PutByte(0);
	Buffer.PutBytes(Key);
	Buffer.PutQuad(AccountID);
	Buffer.PutString(Password);
	// Raw RSA, no padding: the protocol's block is the plain text (a zero byte, the XTEA key, the login data,
	// rand()'s filler) raised to 65537 modulo the key, as the server expects (notes/decisions.md, "#128").
	try {
		TRSA RSA;
		RSA.SetPublicKey(GetRsaModulus());
		RSA.Encrypt(Data);
	} catch (const char *Error) {
		std::stringstream Text;
		Text << "Asymmetric encryption for login server failed";
		for (int i = 0; i < 128; i++) {
			if (i % 16 == 0)
				Text << std::endl;
			TFixedString<20> Byte;
			Byte.Format("{:03} ", (unsigned)Data[i]);
			Text << Byte.View();
		}
		Text << std::endl;
		throw TMYEXCEPTION(Text.str().c_str(), "", 0, "", 0, Error);
	} catch (...) {
		std::stringstream Text;
		Text << "Asymmetric encryption for login server failed with unknown exception";
		for (int i = 0; i < 128; i++) {
			if (i % 16 == 0)
				Text << std::endl;
			TFixedString<20> Byte;
			Byte.Format("{:03} ", (unsigned)Data[i]);
			Text << Byte.View();
		}
		// No std::endl after the bytes here, unlike the first catch block (the
		// binary goes from the loop to str(), issue #36).
		TERROR(Text.str().c_str());
	}
	Network.BeginCommand(LOGIN_SEND_LOGIN);
	Network.SendWord(2);
	Network.SendWord(CLIENT_VERSION);
	Network.SendQuad(ObjectTypes.DatSignature);
	Network.SendQuad(Signatures.SpriteSignature());
	Network.SendQuad(Signatures.PictureSignature());
	Network.SendBytes(Data);
	Network.SendCommand(false);
}

void patella::TCommunication::SendGameLogin(uint32 AccountID, const char *Name, const char *Password)
{
	PATELLA_ASSERT(Name!=nullptr);
	PATELLA_ASSERT(Password!=nullptr);
	std::array<uint8, 16> Key;
	for (int i = 0; i < 16; i++)
		Key[i] = (uint8)rand();
	Network.SetSymmetricKey(Key.data());
	std::array<uint8, 128> Data;
	for (int i = 0; i < 128; i++)
		Data[i] = (uint8)rand();
	TWriteBuffer Buffer(Data);
	Buffer.PutByte(0);
	Buffer.PutBytes(Key);
	Buffer.PutByte(Session.GetGamemaster() != 0);
	Buffer.PutQuad(AccountID);
	Buffer.PutString(Name);
	Buffer.PutString(Password);
	// Raw RSA, no padding, as in SendLogin (notes/decisions.md, "#128").
	try {
		TRSA RSA;
		RSA.SetPublicKey(GetRsaModulus());
		RSA.Encrypt(Data);
	} catch (const char *Error) {
		std::stringstream Text;
		Text << "Asymmetric encryption for game server failed";
		for (int i = 0; i < 128; i++) {
			if (i % 16 == 0)
				Text << std::endl;
			TFixedString<20> Byte;
			Byte.Format("{:03} ", (unsigned)Data[i]);
			Text << Byte.View();
		}
		Text << std::endl;
		throw TMYEXCEPTION(Text.str().c_str(), "", 0, "", 0, Error);
	} catch (...) {
		std::stringstream Text;
		Text << "Asymmetric encryption for game server failed with unknown exception";
		for (int i = 0; i < 128; i++) {
			if (i % 16 == 0)
				Text << std::endl;
			TFixedString<20> Byte;
			Byte.Format("{:03} ", (unsigned)Data[i]);
			Text << Byte.View();
		}
		// No std::endl after the bytes here, unlike the first catch block (the
		// binary goes from the loop to str(), issue #36).
		TERROR(Text.str().c_str());
	}
	Network.BeginCommand(GAME_SEND_LOGIN);
	Network.SendWord(2);
	Network.SendWord(CLIENT_VERSION);
	Network.SendBytes(Data);
	Network.SendCommand(false);
}

void patella::TCommunication::SendLogout()
{
	Network.BeginCommand(GAME_SEND_LOGOUT);
	Network.SendCommand(true);
}

void patella::TCommunication::SendPing()
{
	Network.BeginCommand(GAME_SEND_PING);
	Network.SendCommand(true);
}

void patella::TCommunication::PrepareMovement()
{
	if (GetAutoChaseOff()) {
		if (Creatures.GetAttackID() != 0) {
			if (GetChaseMode() != CHASE_STAND) {
				SetChaseMode(CHASE_STAND);
				SendFightModes();
			}
		}
	}
	if (Creatures.GetFollowID() != 0) {
		Creatures.SetFollowID(0);
		SendFollow(0);
	}
}

// An invalid single-step direction still sends the existing command buffer.
void patella::TCommunication::SendWalk()
{
	PrepareMovement();
	const int Steps = PlayerState.GetNumberOfPlayerSteps();
	PATELLA_ASSERT_VALUES(Steps>0, Steps);
	if (Steps == 1) {
		switch (PlayerState.GetPlayerStep(0)) {
		case STEP_EAST: Network.BeginCommand(GAME_SEND_WALK_EAST); break;
		case STEP_NORTHEAST: Network.BeginCommand(GAME_SEND_WALK_NORTH_EAST); break;
		case STEP_NORTH: Network.BeginCommand(GAME_SEND_WALK_NORTH); break;
		case STEP_NORTHWEST: Network.BeginCommand(GAME_SEND_WALK_NORTH_WEST); break;
		case STEP_WEST: Network.BeginCommand(GAME_SEND_WALK_WEST); break;
		case STEP_SOUTHWEST: Network.BeginCommand(GAME_SEND_WALK_SOUTH_WEST); break;
		case STEP_SOUTH: Network.BeginCommand(GAME_SEND_WALK_SOUTH); break;
		case STEP_SOUTHEAST: Network.BeginCommand(GAME_SEND_WALK_SOUTH_EAST); break;
		default: break; // another step sends nothing, as without it (bugprone-switch-missing-default-case, issue #110)
		}
	} else {
		Network.BeginCommand(GAME_SEND_WALK);
		Network.SendByte(Steps);
		for (int i = 0; i < Steps; i++)
			Network.SendByte(PlayerState.GetPlayerStep(i));
	}
	Network.SendCommand(true);
}

void patella::TCommunication::SendStop()
{
	PrepareMovement();
	Network.BeginCommand(GAME_SEND_STOP);
	Network.SendCommand(true);
}

void patella::TCommunication::SendTurnNorth()
{
	Network.BeginCommand(GAME_SEND_TURN_NORTH);
	Network.SendCommand(true);
}

void patella::TCommunication::SendTurnEast()
{
	Network.BeginCommand(GAME_SEND_TURN_EAST);
	Network.SendCommand(true);
}

void patella::TCommunication::SendTurnSouth()
{
	Network.BeginCommand(GAME_SEND_TURN_SOUTH);
	Network.SendCommand(true);
}

void patella::TCommunication::SendTurnWest()
{
	Network.BeginCommand(GAME_SEND_TURN_WEST);
	Network.SendCommand(true);
}

void patella::TCommunication::SendMoveObject(int x, int y, int z, int ObjectType, int Position, int DestinationX, int DestinationY, int DestinationZ, int Count)
{
	if (ObjectTypes.GetObjectFlag(ObjectType, 13)) {
		SetStatusMessage("You cannot move this object.", false);
		return;
	}
	if (x != POSITION_NOT_ON_MAP)
		PlayerState.ClearPlayerSteps();
	Network.BeginCommand(GAME_SEND_MOVE_OBJECT);
	Network.SendWord(x);
	Network.SendWord(y);
	Network.SendByte(z);
	Network.SendWord(ObjectType);
	Network.SendByte(Position);
	Network.SendWord(DestinationX);
	Network.SendWord(DestinationY);
	Network.SendByte(DestinationZ);
	Network.SendByte(Count);
	Network.SendCommand(true);
}

void patella::TCommunication::SendRequestTrade(int x, int y, int z, int ObjectType, int Position, uint32 CreatureID)
{
	PlayerState.ClearPlayerSteps();
	Network.BeginCommand(GAME_SEND_REQUEST_TRADE);
	Network.SendWord(x);
	Network.SendWord(y);
	Network.SendByte(z);
	Network.SendWord(ObjectType);
	Network.SendByte(Position);
	Network.SendQuad(CreatureID);
	Network.SendCommand(true);
}

void patella::TCommunication::SendInspectTrade(int Side, int Position)
{
	Network.BeginCommand(GAME_SEND_INSPECT_TRADE);
	Network.SendByte(Side);
	Network.SendByte(Position);
	Network.SendCommand(true);
}

void patella::TCommunication::SendAcceptTrade()
{
	Network.BeginCommand(GAME_SEND_ACCEPT_TRADE);
	Network.SendCommand(true);
}

void patella::TCommunication::SendRejectTrade()
{
	Network.BeginCommand(GAME_SEND_REJECT_TRADE);
	Network.SendCommand(true);
}

void patella::TCommunication::SendUseObject(int x, int y, int z, int ObjectType, int Position, int Container)
{
	if (x != POSITION_NOT_ON_MAP)
		PlayerState.ClearPlayerSteps();
	Network.BeginCommand(GAME_SEND_USE_OBJECT);
	Network.SendWord(x);
	Network.SendWord(y);
	Network.SendByte(z);
	Network.SendWord(ObjectType);
	Network.SendByte(Position);
	Network.SendByte(Container);
	Network.SendCommand(true);
}

void patella::TCommunication::SendUseObjectWith(int x1, int y1, int z1, int ObjectType1, int Position1, int x2, int y2, int z2, int ObjectType2, int Position2)
{
	if (x1 != POSITION_NOT_ON_MAP || x2 != POSITION_NOT_ON_MAP)
		PlayerState.ClearPlayerSteps();
	Network.BeginCommand(GAME_SEND_USE_OBJECT_WITH);
	Network.SendWord(x1);
	Network.SendWord(y1);
	Network.SendByte(z1);
	Network.SendWord(ObjectType1);
	Network.SendByte(Position1);
	Network.SendWord(x2);
	Network.SendWord(y2);
	Network.SendByte(z2);
	Network.SendWord(ObjectType2);
	Network.SendByte(Position2);
	Network.SendCommand(true);
}

void patella::TCommunication::SendUseObjectOnCreature(int x, int y, int z, int ObjectType, int Position, uint32 CreatureID)
{
	PlayerState.ClearPlayerSteps();
	Network.BeginCommand(GAME_SEND_USE_OBJECT_ON_CREATURE);
	Network.SendWord(x);
	Network.SendWord(y);
	Network.SendByte(z);
	Network.SendWord(ObjectType);
	Network.SendByte(Position);
	Network.SendQuad(CreatureID);
	Network.SendCommand(true);
}

void patella::TCommunication::SendRotateObject(int x, int y, int z, int ObjectType, int Position)
{
	Network.BeginCommand(GAME_SEND_ROTATE_OBJECT);
	Network.SendWord(x);
	Network.SendWord(y);
	Network.SendByte(z);
	Network.SendWord(ObjectType);
	Network.SendByte(Position);
	Network.SendCommand(true);
}

void patella::TCommunication::SendCloseContainer(int Container)
{
	Network.BeginCommand(GAME_SEND_CLOSE_CONTAINER);
	Network.SendByte(Container);
	Network.SendCommand(true);
}

void patella::TCommunication::SendContainerUp(int Container)
{
	Network.BeginCommand(GAME_SEND_CONTAINER_UP);
	Network.SendByte(Container);
	Network.SendCommand(true);
}

void patella::TCommunication::SendEditText(uint32 ObjectNumber, const char *Text)
{
	Network.BeginCommand(GAME_SEND_EDIT_TEXT);
	Network.SendQuad(ObjectNumber);
	Network.SendString(Text);
	Network.SendCommand(true);
}

void patella::TCommunication::SendEditHouseText(int Type, uint32 Identifier, const char *Text)
{
	Network.BeginCommand(GAME_SEND_EDIT_HOUSE_TEXT);
	Network.SendByte(Type);
	Network.SendQuad(Identifier);
	Network.SendString(Text);
	Network.SendCommand(true);
}

void patella::TCommunication::SendLook(int x, int y, int z, int ObjectType, int Position)
{
	Network.BeginCommand(GAME_SEND_LOOK);
	Network.SendWord(x);
	Network.SendWord(y);
	Network.SendByte(z);
	Network.SendWord(ObjectType);
	Network.SendByte(Position);
	Network.SendCommand(true);
}

void patella::TCommunication::SendTalk(int Mode, const char *Text)
{
	PATELLA_ASSERT_VALUES(Mode==TALK_SAY || Mode==TALK_WHISPER || Mode==TALK_YELL || Mode==TALK_GAMEMASTER_REQUEST || Mode==TALK_PLAYER_ANSWER || Mode==TALK_GAMEMASTER_BROADCAST || Mode==TALK_ANONYMOUS_BROADCAST, Mode);
	PATELLA_ASSERT(Text!=nullptr);
	Network.BeginCommand(GAME_SEND_TALK);
	Network.SendByte(Mode);
	Network.SendString(Text);
	Network.SendCommand(true);
}

void patella::TCommunication::SendTalk(int Mode, const char *Addressee, const char *Text)
{
	PATELLA_ASSERT_VALUES(Mode==TALK_PRIVATE_MESSAGE || Mode==TALK_GAMEMASTER_ANSWER || Mode==TALK_GAMEMASTER_MESSAGE || Mode==TALK_ANONYMOUS_MESSAGE, Mode);
	PATELLA_ASSERT(Addressee!=nullptr);
	PATELLA_ASSERT(Text!=nullptr);
	Network.BeginCommand(GAME_SEND_TALK);
	Network.SendByte(Mode);
	Network.SendString(Addressee);
	Network.SendString(Text);
	Network.SendCommand(true);
}

void patella::TCommunication::SendTalk(int Mode, int ChannelID, const char *Text)
{
	PATELLA_ASSERT_VALUES(Mode==TALK_CHANNEL_CALL || Mode==TALK_GAMEMASTER_CHANNELCALL || Mode==TALK_ANONYMOUS_CHANNELCALL, Mode);
	PATELLA_ASSERT(Text!=nullptr);
	Network.BeginCommand(GAME_SEND_TALK);
	Network.SendByte(Mode);
	Network.SendWord(ChannelID);
	Network.SendString(Text);
	Network.SendCommand(true);
}

void patella::TCommunication::SendRequestChannels()
{
	Network.BeginCommand(GAME_SEND_REQUEST_CHANNELS);
	Network.SendCommand(true);
}

void patella::TCommunication::SendJoinChannel(int ChannelID)
{
	Network.BeginCommand(GAME_SEND_JOIN_CHANNEL);
	Network.SendWord(ChannelID);
	Network.SendCommand(true);
}

void patella::TCommunication::SendLeaveChannel(int ChannelID)
{
	Network.BeginCommand(GAME_SEND_LEAVE_CHANNEL);
	Network.SendWord(ChannelID);
	Network.SendCommand(true);
}

void patella::TCommunication::SendOpenPrivateConversation(const char *Name)
{
	Network.BeginCommand(GAME_SEND_OPEN_PRIVATE_CONVERSATION);
	Network.SendString(Name);
	Network.SendCommand(true);
}

void patella::TCommunication::SendProcessRuleViolation(const char *Name)
{
	Network.BeginCommand(GAME_SEND_PROCESS_RULE_VIOLATION);
	Network.SendString(Name);
	Network.SendCommand(true);
}

void patella::TCommunication::SendCloseRuleViolation(const char *Name)
{
	Network.BeginCommand(GAME_SEND_CLOSE_RULE_VIOLATION);
	Network.SendString(Name);
	Network.SendCommand(true);
}

void patella::TCommunication::SendCancelRuleViolation()
{
	Network.BeginCommand(GAME_SEND_CANCEL_RULE_VIOLATION);
	Network.SendCommand(true);
}

void patella::TCommunication::SendFightModes()
{
	Network.BeginCommand(GAME_SEND_FIGHT_MODES);
	Network.SendByte(GetAttackMode());
	Network.SendByte(GetChaseMode());
	Network.SendByte(GetSecureMode());
	Network.SendCommand(true);
}

void patella::TCommunication::SendAttack(uint32 CreatureID)
{
	if (CreatureID != 0)
		PlayerState.ClearPlayerSteps();
	Network.BeginCommand(GAME_SEND_ATTACK);
	Network.SendQuad(CreatureID);
	Network.SendCommand(true);
}

void patella::TCommunication::SendFollow(uint32 CreatureID)
{
	if (CreatureID != 0)
		PlayerState.ClearPlayerSteps();
	Network.BeginCommand(GAME_SEND_FOLLOW);
	Network.SendQuad(CreatureID);
	Network.SendCommand(true);
}

void patella::TCommunication::SendOpenPrivateChannel()
{
	Network.BeginCommand(GAME_SEND_OPEN_PRIVATE_CHANNEL);
	Network.SendCommand(true);
}

void patella::TCommunication::SendInviteToPrivateChannel(const char *Name)
{
	Network.BeginCommand(GAME_SEND_INVITE_TO_PRIVATE_CHANNEL);
	Network.SendString(Name);
	Network.SendCommand(true);
}

void patella::TCommunication::SendExcludeFromPrivateChannel(const char *Name)
{
	Network.BeginCommand(GAME_SEND_EXCLUDE_FROM_PRIVATE_CHANNEL);
	Network.SendString(Name);
	Network.SendCommand(true);
}

void patella::TCommunication::SendInviteToParty(uint32 CharacterID)
{
	Network.BeginCommand(GAME_SEND_INVITE_TO_PARTY);
	Network.SendQuad(CharacterID);
	Network.SendCommand(true);
}

void patella::TCommunication::SendJoinParty(uint32 CharacterID)
{
	Network.BeginCommand(GAME_SEND_JOIN_PARTY);
	Network.SendQuad(CharacterID);
	Network.SendCommand(true);
}

void patella::TCommunication::SendRevokePartyInvitation(uint32 CharacterID)
{
	Network.BeginCommand(GAME_SEND_REVOKE_PARTY_INVITATION);
	Network.SendQuad(CharacterID);
	Network.SendCommand(true);
}

void patella::TCommunication::SendPassPartyLeadership(uint32 CharacterID)
{
	Network.BeginCommand(GAME_SEND_PASS_PARTY_LEADERSHIP);
	Network.SendQuad(CharacterID);
	Network.SendCommand(true);
}

void patella::TCommunication::SendLeaveParty()
{
	Network.BeginCommand(GAME_SEND_LEAVE_PARTY);
	Network.SendCommand(true);
}

void patella::TCommunication::SendCancel()
{
	Network.BeginCommand(GAME_SEND_CANCEL);
	Network.SendCommand(true);
}

void patella::TCommunication::SendRefreshField(int x, int y, int z)
{
	Network.BeginCommand(GAME_SEND_REFRESH_FIELD);
	Network.SendWord(x);
	Network.SendWord(y);
	Network.SendByte(z);
	Network.SendCommand(true);
	Map.WorldToMap(&x, &y, &z);
	Map.RequestFieldRefresh(x, y, z);
}

void patella::TCommunication::SendRefreshContainer(int Container)
{
	Network.BeginCommand(GAME_SEND_REFRESH_CONTAINER);
	Network.SendByte(Container);
	Network.SendCommand(true);
}

// The request flag is set before sending and is not rolled back on an exception.
void patella::TCommunication::SendRequestOutfit()
{
	if (!OutfitRequested) {
		OutfitRequested = true;
		Network.BeginCommand(GAME_SEND_REQUEST_OUTFIT);
		Network.SendCommand(true);
	}
}

void patella::TCommunication::SendOutfit(int OutfitType, const int *Colors)
{
	PATELLA_ASSERT(Colors!=nullptr);
	Network.BeginCommand(GAME_SEND_OUTFIT);
	Network.SendWord(OutfitType);
	Network.SendByte(Colors[0]);
	Network.SendByte(Colors[1]);
	Network.SendByte(Colors[2]);
	Network.SendByte(Colors[3]);
	Network.SendCommand(true);
}

void patella::TCommunication::SendAddBuddy(const char *Name)
{
	PATELLA_ASSERT(Name!=nullptr);
	Network.BeginCommand(GAME_SEND_ADD_BUDDY);
	Network.SendString(Name);
	Network.SendCommand(true);
}

void patella::TCommunication::SendRemoveBuddy(uint32 CharacterID)
{
	Network.BeginCommand(GAME_SEND_REMOVE_BUDDY);
	Network.SendQuad(CharacterID);
	Network.SendCommand(true);
}

void patella::TCommunication::SendBugReport(const char *Text)
{
	Network.BeginCommand(GAME_SEND_BUG_REPORT);
	Network.SendString(Text);
	Network.SendCommand(true);
}

void patella::TCommunication::SendRuleViolationReport(const char *Name, uint8 Reason, uint8 Action, const char *Comment, uint32 StatementID, bool IPBanishment)
{
	Network.BeginCommand(GAME_SEND_RULE_VIOLATION_REPORT);
	Network.SendString(Name);
	Network.SendByte(Reason);
	Network.SendByte(Action);
	Network.SendString(Comment);
	Network.SendQuad(StatementID);
	Network.SendByte(IPBanishment);
	Network.SendCommand(true);
}

// SendDebugReport (client packet 0xe8: an error report of error.txt) is gone since Patella-Modern
// issue #74 (decision D2).

void patella::TCommunication::ReceiveLoginError()
{
	const char *Text = Network.GetString();
	AddMessage(1, reinterpret_cast<std::intptr_t>("Sorry"), reinterpret_cast<std::intptr_t>(Text));
	Network.Disconnect(false);
	Session.SetConnectionState(CONNECTION_OFFLINE);
}

void patella::TCommunication::ReceiveLoginMessage()
{
	const char *Text = Network.GetString();
	AddMessage(1, reinterpret_cast<std::intptr_t>("For Your Information"), reinterpret_cast<std::intptr_t>(Text));
}

void patella::TCommunication::ReceiveMessageOfTheDay()
{
	const char *Text = Network.GetString();
	if (SetMotD(Text))
		AddMessage(MESSAGE_MESSAGE_OF_THE_DAY, 0, 0);
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): GetString's text, the caller's (notes/ownership.md)
	delete[] Text;
}

// ReceivePatch (packet 0x1e: a difference patch of a file), ReceiveWholeFile (0x1f: a whole file) and
// ReceivePatchBlock (0x20: a block of either) are gone since Patella-Modern issue #75 (decision D9): with them a
// login server could rewrite Tibia.exe (as Tibia.bak), Tibia.dat, Tibia.spr, Tibia.pic and Patch.exe, and have
// Patch.exe run. ReceiveUpdateRequest takes all three: the login ends as a login error does, in the same dialog
// ("Sorry"), with TextUpdateRefused. The rest of what the server sent (the patch's header or block, the packets after
// it in this frame and in the frames already received) is read and dropped, so that nothing of it reaches the game
// server's table once the connection state is 0.
void patella::TCommunication::ReceiveUpdateRequest()
{
	while (Network.GetNextCommand() != -1) {
	}
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader (notes/ownership.md)
	char *Text = new char[TextUpdateRefused.size() + 1];
	memcpy(Text, TextUpdateRefused.data(), TextUpdateRefused.size());
	Text[TextUpdateRefused.size()] = 0;
	AddMessage(1, reinterpret_cast<std::intptr_t>("Sorry"), reinterpret_cast<std::intptr_t>(Text));
	Network.Disconnect(false);
	Session.SetConnectionState(CONNECTION_OFFLINE);
}

void patella::TCommunication::ReceiveCharacterList()
{
	int Count = 0;
	Count = Network.GetByte();
	Session.SetAccountCharacters(Count);
	for (int i = 0; i < Count; i++) {
		TFixedString<40> Name;
		TFixedString<32> World;
		std::array<uint8, 8> Address;
		Network.GetString(patella::TSpan<char>(Name.Chars).first(30));
		Network.GetString(patella::TSpan<char>(World.Chars).first(30));
		for (int j = 0; j < 4; j++)
			Address[j] = Network.GetByte();
		const int Port = Network.GetWord();
		Session.SetCharacter(i, Name, World, Address.data(), Port);
	}
	const int PremiumDays = Network.GetWord();
	Session.SetPremiumDays(PremiumDays);
	AddMessage(MESSAGE_CHARACTER_LIST_DIALOG, 300, 0);
}

// CancelPatch (the update dialog's Cancel, the transfer dropped and the connection closed), is gone with
// the update dialog since Patella-Modern issue #75.

void patella::TCommunication::ReceiveLoginServerOverloaded()
{
	Network.SetConnectionTimeout(0);
	Network.Disconnect(false);
	Session.SetConnectionState(CONNECTION_OFFLINE);
	Network.SetServerError(ERROR_LOGINSERVER_OVERLOADED);
	Network.ConnectToLoginServer();
}

void patella::TCommunication::ReceiveGameLogin()
{
	Session.ClearRights();
	const uint32 CreatureID = Network.GetQuad();
	PlayerState.SetPlayerID(CreatureID);
	const int Beat = Network.GetWord();
	Creatures.SetBeat(Beat);
	const uint8 value = Network.GetByte();
	Session.SetRight(40, value);
	SetActionState(ACTIONSTATE_NONE);
}

void patella::TCommunication::ReceiveGamemasterActions()
{
	int i;
	uint8 value = 0;
	for (i = 0; i < 32; i++) {
		value = Network.GetByte();
		Session.SetRight(i, value);
	}
}

void patella::TCommunication::ReceiveGameError()
{
	const char *Text = Network.GetString();
	AddMessage(1, reinterpret_cast<std::intptr_t>("Sorry"), reinterpret_cast<std::intptr_t>(Text));
	Network.Disconnect(false);
	Session.SetConnectionState(CONNECTION_OFFLINE);
}

void patella::TCommunication::ReceiveGameMessage()
{
	const char *Text = Network.GetString();
	AddMessage(1, reinterpret_cast<std::intptr_t>("For Your Information"), reinterpret_cast<std::intptr_t>(Text));
}

void patella::TCommunication::ReceiveLoginWait()
{
	const char *Text = Network.GetString();
	const int Delay = Network.GetByte();
	AddMessage(MESSAGE_RECONNECT_DIALOG, reinterpret_cast<std::intptr_t>(Text), Delay);
}

void patella::TCommunication::ReceivePing()
{
	SendPing();
}

// A missing known creature consumes a byte, not a word, for outfit type and skips no skull/shield bytes.
uint32 patella::TCommunication::ReadCreature(int Type, int x, int y, int z)
{
	uint32 CreatureID = 0;
	TCreature *Creature;
	if (Type == UNKNOWNCREATURE) {
		const uint32 RemoveID = Network.GetQuad();
		CreatureID = Network.GetQuad();
		Creature = Creatures.AddCreature(RemoveID, CreatureID);
		PATELLA_ASSERT_VALUES(Creature!=nullptr, CreatureID);
		Network.GetString(patella::TSpan<char>(Creature->Name.Chars).first(30));
		if (!Creatures.IsPlayer(CreatureID))
			CapitalizeString(Creature->Name);
		Creature->Health = Network.GetByte();
		Creature->Direction = Network.GetByte();
		Creature->OutfitType = Network.GetWord();
		if (Creature->OutfitType == 0) {
			Creature->Colors[0] = Network.GetWord();
		} else {
			Creature->Colors[0] = Network.GetByte();
			Creature->Colors[1] = Network.GetByte();
			Creature->Colors[2] = Network.GetByte();
			Creature->Colors[3] = Network.GetByte();
		}
		Creature->LightStrength = Network.GetByte();
		Creature->LightColor = Network.GetByte();
		Creature->Speed = Network.GetWord();
		const int Skull = Network.GetByte();
		Creature->SetSkull(Skull, false);
		Creatures.NotifySkull(Creature->PKFlag);
		Creature->PARTYFlag = Network.GetByte();
	} else if (Type == OUTDATEDCREATURE) {
		CreatureID = Network.GetQuad();
		Creature = Creatures.GetCreature(CreatureID);
		if (Creature == nullptr) {
			Network.GetByte();
			Network.GetByte();
			if (Network.GetByte() == 0) {
				Network.GetWord();
				Network.GetByte();
				Network.GetByte();
				Network.GetWord();
			} else {
				Network.GetByte();
				Network.GetByte();
				Network.GetByte();
				Network.GetByte();
				Network.GetByte();
				Network.GetByte();
				Network.GetWord();
			}
		} else {
			Creature->Health = Network.GetByte();
			Creature->Direction = Network.GetByte();
			Creature->OutfitType = Network.GetWord();
			if (Creature->OutfitType == 0) {
				Creature->Colors[0] = Network.GetWord();
			} else {
				Creature->Colors[0] = Network.GetByte();
				Creature->Colors[1] = Network.GetByte();
				Creature->Colors[2] = Network.GetByte();
				Creature->Colors[3] = Network.GetByte();
			}
			Creature->LightStrength = Network.GetByte();
			Creature->LightColor = Network.GetByte();
			Creature->Speed = Network.GetWord();
			const int Skull = Network.GetByte();
			Creature->SetSkull(Skull, false);
			Creatures.NotifySkull(Creature->PKFlag);
			Creature->PARTYFlag = Network.GetByte();
		}
	} else {
		if (Type != CREATURE)
			return 0;
		CreatureID = Network.GetQuad();
		Creature = Creatures.GetCreature(CreatureID);
		if (Creature == nullptr)
			Network.GetByte();
		else
			Creature->Direction = Network.GetByte();
	}
	if (Creature != nullptr) {
		Map.MapToWorld(&x, &y, &z);
		Creature->SetPosition(x, y, z);
		Creature->Visible = true;
	}
	return CreatureID;
}

// OldType only the original's catch wrapper read (issue #78).
void patella::TCommunication::ReadField(int rx, int ry, int rz)
{
	uint16 Type = 0;
	[[maybe_unused]] uint16 OldType = 0;
	if (SkipFields < 1) {
		for (;;) {
			OldType = Type;
			Type = Network.GetWord();
			if (Type >= FIELD_END) {
				SkipFields = Type - FIELD_END;
				break;
			}
			if (Type == 0) {
				std::stringstream Text;
				Text << "client recieved field with type 0 (" << rx << "," << ry << "," << rz << ")";
				TERROR(Text.str().c_str());
			}
			if (Type == UNKNOWNCREATURE || Type == OUTDATEDCREATURE || Type == CREATURE) {
				const uint32 CreatureID = ReadCreature(Type, rx, ry, rz);
				Map.AppendObject(rx, ry, rz, TObject{CREATURE, (int)CreatureID, 0});
			} else {
				int Data = 0;
				if (ObjectTypes.GetObjectFlag(Type, 5))
					Data = Network.GetByte();
				if (ObjectTypes.GetObjectFlag(Type, 11))
					Data = Network.GetByte();
				if (ObjectTypes.GetObjectFlag(Type, 10))
					Data = Network.GetByte();
				Map.AppendObject(rx, ry, rz, TObject{Type, Data, 0});
			}
		}
	} else {
		SkipFields--;
	}
	if (rz == Map.GetMapLevel())
		Map.UpdateAutomap(rx, ry);
}

void patella::TCommunication::ReadMap(int xmin, int ymin, int xmax, int ymax)
{
	int PlayerX;
	int PlayerY;
	int PlayerZ;
	Map.GetMapPosition(&PlayerX, &PlayerY, &PlayerZ);
	int Floor;
	int EndFloor;
	int Step;
	if (PlayerZ < 8) {
		Floor = 0;
		EndFloor = 8;
		Step = 1;
	} else {
		Floor = 4;
		EndFloor = PlayerZ - 14;
		if (EndFloor < -1)
			EndFloor = -1;
		Step = -1;
	}
	SkipFields = 0;
	for (; Floor != EndFloor; Floor += Step) {
		for (int x = xmin; x <= xmax; x++) {
			for (int y = ymin; y <= ymax; y++)
				ReadField(x, y, Floor);
		}
	}
}

void patella::TCommunication::ReceiveMap()
{
	uint16 PlayerX = 0;
	uint16 PlayerY = 0;
	PlayerX = Network.GetWord();
	PlayerY = Network.GetWord();
	const int PlayerZ = Network.GetByte();
	Map.SetMapPosition(PlayerX, PlayerY, PlayerZ);
	Map.ClearMap();
	Creatures.HideAllCreatures();
	ReadMap(0, 0, 17, 13);
	SetTalkChanged();
	PlayerState.StopPlayer();
	CheckEventHelpPosition(PlayerX, PlayerY, PlayerZ);
}

void patella::TCommunication::ReceiveMapNorth()
{
	Map.ShiftMap(0, 1);
	ReadMap(0, 0, 17, 0);
	SetTalkChanged();
	int x;
	int y;
	int z;
	Map.GetMapPosition(&x, &y, &z);
	CheckEventHelpPosition(x, y, z);
}

void patella::TCommunication::ReceiveMapEast()
{
	Map.ShiftMap(-1, 0);
	ReadMap(17, 0, 17, 13);
	SetTalkChanged();
	int x;
	int y;
	int z;
	Map.GetMapPosition(&x, &y, &z);
	CheckEventHelpPosition(x, y, z);
}

void patella::TCommunication::ReceiveMapSouth()
{
	Map.ShiftMap(0, -1);
	ReadMap(0, 13, 17, 13);
	SetTalkChanged();
	int x;
	int y;
	int z;
	Map.GetMapPosition(&x, &y, &z);
	CheckEventHelpPosition(x, y, z);
}

void patella::TCommunication::ReceiveMapWest()
{
	Map.ShiftMap(1, 0);
	ReadMap(0, 0, 0, 13);
	SetTalkChanged();
	int x;
	int y;
	int z;
	Map.GetMapPosition(&x, &y, &z);
	CheckEventHelpPosition(x, y, z);
}

// ReadField already refreshes visible-floor lighting; this handler deliberately does it again.
void patella::TCommunication::ReceiveField()
{
	int rx = 0;
	int ry = 0;
	int rz = 0;
	rx = Network.GetWord();
	ry = Network.GetWord();
	rz = Network.GetByte();
	Map.WorldToMap(&rx, &ry, &rz);
	Map.ClearField(rx, ry, rz, false);
	SkipFields = 0;
	ReadField(rx, ry, rz);
	if (rz == Map.GetMapLevel())
		Map.UpdateAutomap(rx, ry);
}

void patella::TCommunication::ReceiveAddObject()
{
	int rx = 0;
	int ry = 0;
	int rz = 0;
	rx = Network.GetWord();
	ry = Network.GetWord();
	rz = Network.GetByte();
	Map.WorldToMap(&rx, &ry, &rz);
	const int Type = Network.GetWord();
	TObject Object;
	if (Type == UNKNOWNCREATURE || Type == OUTDATEDCREATURE || Type == CREATURE) {
		Object.Type = CREATURE;
		Object.Data = ReadCreature(Type, rx, ry, rz);
		Object.Data2 = 0;
	} else {
		int Data = 0;
		if (ObjectTypes.GetObjectFlag(Type, 5))
			Data = Network.GetByte();
		if (ObjectTypes.GetObjectFlag(Type, 11))
			Data = Network.GetByte();
		if (ObjectTypes.GetObjectFlag(Type, 10))
			Data = Network.GetByte();
		Object.Type = Type;
		Object.Data = Data;
		Object.Data2 = 0;
	}
	Map.AddObject(rx, ry, rz, Object);
	if (rz == Map.GetMapLevel())
		Map.UpdateAutomap(rx, ry);
}

void patella::TCommunication::ReceiveChangeObject()
{
	int rx = 0;
	int ry = 0;
	int rz = 0;
	rx = Network.GetWord();
	ry = Network.GetWord();
	rz = Network.GetByte();
	const int Position = Network.GetByte(); // read before the conversion
	Map.WorldToMap(&rx, &ry, &rz);
	const TObject OldObject = Map.GetMapObject(rx, ry, rz, Position);
	if (OldObject.Type == ObjectTypes.NoObject.Type) {
		PATELLA_ASSERT_VALUES(Map.FieldRefreshRequested(rx,ry,rz), rx, ry);
		const int Type = Network.GetWord();
		if (Type == UNKNOWNCREATURE || Type == OUTDATEDCREATURE || Type == CREATURE) {
			ReadCreature(Type, rx, ry, rz);
			return;
		}
		if (ObjectTypes.GetObjectFlag(Type, 5))
			Network.GetByte();
		if (ObjectTypes.GetObjectFlag(Type, 11))
			Network.GetByte();
		if (ObjectTypes.GetObjectFlag(Type, 10))
			Network.GetByte();
	} else {
		if (OldObject.Type == CREATURE) {
			TCreature *Creature = Creatures.GetCreature(OldObject.Data);
			if (Creature != nullptr)
				Creature->Visible = false;
		}
		const int Type = Network.GetWord();
		TObject Object;
		if (Type == UNKNOWNCREATURE || Type == OUTDATEDCREATURE || Type == CREATURE) {
			Object.Type = CREATURE;
			Object.Data = ReadCreature(Type, rx, ry, rz);
			Object.Data2 = 0;
		} else {
			int Data = 0;
			if (ObjectTypes.GetObjectFlag(Type, 5))
				Data = Network.GetByte();
			if (ObjectTypes.GetObjectFlag(Type, 11))
				Data = Network.GetByte();
			if (ObjectTypes.GetObjectFlag(Type, 10))
				Data = Network.GetByte();
			Object.Type = Type;
			Object.Data = Data;
			Object.Data2 = 0;
		}
		Map.ChangeObject(rx, ry, rz, Position, Object);
		if (rz == Map.GetMapLevel())
			Map.UpdateAutomap(rx, ry);
	}
}

void patella::TCommunication::ReceiveDeleteObject()
{
	int rx = 0;
	int ry = 0;
	int rz = 0;
	rx = Network.GetWord();
	ry = Network.GetWord();
	rz = Network.GetByte();
	const int Position = Network.GetByte();
	Map.WorldToMap(&rx, &ry, &rz);
	const TObject Object = Map.GetMapObject(rx, ry, rz, Position);
	if (Object.Type == ObjectTypes.NoObject.Type) {
		PATELLA_ASSERT_VALUES(Map.FieldRefreshRequested(rx,ry,rz), rx, ry);
	} else {
		if (Object.Type == CREATURE) {
			TCreature *Creature = Creatures.GetCreature(Object.Data);
			if (Creature != nullptr)
				Creature->Visible = false;
		}
		const bool Refresh = Map.DeleteObject(rx, ry, rz, Position);
		if (rz == Map.GetMapLevel())
			Map.UpdateAutomap(rx, ry);
		Map.MapToWorld(&rx, &ry, &rz);
		if (Refresh)
			SendRefreshField(rx, ry, rz);
	}
}

// Diagnostics may follow creature mutation; a missing creature record still moves the map object.
void patella::TCommunication::ReceiveMoveCreature()
{
	int sx = Network.GetWord();
	int sy = Network.GetWord();
	int sz = Network.GetByte();
	const int Position = Network.GetByte();
	int zx = Network.GetWord();
	int zy = Network.GetWord();
	int zz = Network.GetByte();
	const int dx = zx - sx;
	const int dy = zy - sy;
	const int dz = zz - sz;
	const int abs_sx = sx;
	const int abs_sy = sy;
	const int abs_sz = sz;
	const int abs_zx = zx;
	const int abs_zy = zy;
	const int abs_zz = zz;
	const bool Teleport = dz != 0 || abs(dx) >= 2 || abs(dy) >= 2;
	Map.WorldToMap(&sx, &sy, &sz);
	Map.WorldToMap(&zx, &zy, &zz);
	const TObject Object = Map.GetMapObject(sx, sy, sz, Position);
	std::string Error;
	TCreature *Creature = nullptr;
	if (Object.Type == ObjectTypes.NoObject.Type || Object.Type != CREATURE) {
		if (Map.FieldRefreshRequested(sx, sy, sz)) {
			Map.MapToWorld(&zx, &zy, &zz);
			SendRefreshField(zx, zy, zz);
			return;
		}
		std::stringstream Text;
		Text << "MoveCreature has been received for a coordinate where there is no creature anymore [bug0000017]";
		if (Object.Type != ObjectTypes.NoObject.Type) {
			if (Object.Type == UNKNOWNCREATURE)
				Text << " (UNKNOWNCREATURE, ";
			else if (Object.Type == OUTDATEDCREATURE)
				Text << " (OUTDATEDCREATURE, ";
			else if (Object.Type == CREATURE)
				Text << " (CREATURE, ";
			else
				Text << " (" << Object.Type << ", ";
			Text << Object.Data << ")";
		}
		Error = Text.str();
	} else {
		Creature = Creatures.GetCreature(Object.Data);
		if (Creature != nullptr) {
			if (Teleport) {
				if (Creature->CreatureID == PlayerState.GetPlayerID())
					PlayerState.StopPlayer();
				if (dx > 0)
					Creature->Direction = DIRECTION_EAST;
				else if (dx < 0)
					Creature->Direction = DIRECTION_WEST;
				else if (dy < 0)
					Creature->Direction = DIRECTION_NORTH;
				else if (dy > 0)
					Creature->Direction = DIRECTION_SOUTH;
				if (Creature->CreatureID != PlayerState.GetPlayerID())
					Creature->StartMovement(0, 0, 0);
			} else {
				const TObject Bank = Map.GetFirstObject(zx, zy, zz);
				if (ObjectTypes.GetObjectFlag(Bank.Type, 0))
					Creature->StartMovement(dx, dy, ObjectTypes.GetObjectWaypoints(Bank.Type));
				else
					Error = "MoveCreature has been received for a coordinate where there is no bank as first object [bug0000484]";
			}
			Creature->RealX += dx;
			Creature->RealY += dy;
			Creature->RealZ += dz;
			Creatures.CheckMonsterHint(Creature);
		}
	}
	if (!Error.empty()) {
		std::stringstream Text;
		Text << " sx: " << sx << " sy: " << sy << " sz: " << sz << std::endl;
		Text << " Position: " << Position << std::endl;
		Text << " zx: " << zx << " zy: " << zy << " zz: " << zz << std::endl;
		Text << " dx: " << dx << " dy: " << dy << " dz: " << dz << std::endl;
		Text << " abs_sx: " << abs_sx << " abs_sy: " << abs_sy << " abs_sz: " << abs_sz << std::endl;
		Text << " abs_zx: " << abs_zx << " abs_zy: " << abs_zy << " abs_zz: " << abs_zz << std::endl;
		Text << " Start Cylinder (1): ";
		try {
			TObject Current = Map.GetFirstObject(sx, sy, sz);
			while (Current.Type != 0) {
				Text << " (" << Current.Type << "," << Current.Data << ")";
				Current = Map.GetNextObject();
			}
			Text << std::endl
				 << " Start Cylinder (2): ";
			for (int i = 0; i < 10; i++) {
				Current = Map.GetMapObject(sx, sy, sz, i);
				if (Current.Type == ObjectTypes.NoObject.Type)
					Text << "(-)";
				else
					Text << " (" << Current.Type << "," << Current.Data << ")";
			}
			Text << std::endl
				 << " Destination Cylinder (1): ";
			Current = Map.GetFirstObject(zx, zy, zz);
			while (Current.Type != 0) {
				Text << " (" << Current.Type << "," << Current.Data << ")";
				Current = Map.GetNextObject();
			}
			Text << std::endl
				 << " Destination Cylinder (2): ";
			for (int i = 0; i < 10; i++) {
				Current = Map.GetMapObject(zx, zy, zz, i);
				if (Current.Type == ObjectTypes.NoObject.Type)
					Text << "(-)";
				else
					Text << " (" << Current.Type << "," << Current.Data << ")";
			}
		} catch (TMyException &Exception) {
			Text << std::endl
				 << "Exception during cylinder looping: " << std::endl
				 << Exception.GetMessage();
		}
		if (Creature != nullptr) {
			Text << std::endl
				 << " ID: " << Creature->CreatureID << " Name: " << Creature->Name.Chars;
			Text << std::endl
				 << " RealX: " << Creature->RealX << " RealY: " << Creature->RealY << " RealZ: " << Creature->RealZ;
			Text << std::endl
				 << " DeltaX: " << Creature->DeltaX << " DeltaY: " << Creature->DeltaY;
		}
		throw TMYEXCEPTION(Error.c_str(), "", 0, "", 0, Text.str().c_str());
	}
	if (Map.DeleteObject(sx, sy, sz, Position)) {
		int x = sx;
		int y = sy;
		int z = sz;
		Map.MapToWorld(&x, &y, &z);
		SendRefreshField(x, y, z);
	}
	if (sz == Map.GetMapLevel())
		Map.UpdateAutomap(sx, sy);
	Map.AddObject(zx, zy, zz, Object);
	if (zz == Map.GetMapLevel())
		Map.UpdateAutomap(zx, zy);
}

void patella::TCommunication::ReceiveOpenContainer(int &Container)
{
	Container = Network.GetByte();
	if (Inventory.IsContainerOpen(Container))
		Inventory.CloseContainer(Container);
	const int Type = Network.GetWord();
	TFixedString<40> Name;
	Network.GetString(patella::TSpan<char>(Name.Chars).first(30));
	const int Capacity = Network.GetByte();
	const bool HasParent = Network.GetByte() != 0;
	Inventory.OpenContainer(Container, TObject{Type, 0, 0}, Name, Capacity, HasParent);
	const int Count = Network.GetByte();
	for (int i = 0; i < Count; i++) {
		const int ObjectType = Network.GetWord(); // Type until issue #110 (GCC's -Wshadow in patella_core)
		int Data = 0;
		if (ObjectTypes.GetObjectFlag(ObjectType, 5))
			Data = Network.GetByte();
		if (ObjectTypes.GetObjectFlag(ObjectType, 11))
			Data = Network.GetByte();
		if (ObjectTypes.GetObjectFlag(ObjectType, 10))
			Data = Network.GetByte();
		Inventory.AppendContainerObject(Container, TObject{ObjectType, (int)Data, 0});
	}
}

void patella::TCommunication::ReceiveCloseContainer(int &Container)
{
	Container = Network.GetByte();
	if (Inventory.IsContainerOpen(Container))
		Inventory.CloseContainer(Container);
}

void patella::TCommunication::ReceiveContainerAddObject()
{
	int Container = 0;
	int Type = 0;
	Container = Network.GetByte();
	Type = Network.GetWord();
	int Data = 0;
	if (ObjectTypes.GetObjectFlag(Type, 5))
		Data = Network.GetByte();
	if (ObjectTypes.GetObjectFlag(Type, 11))
		Data = Network.GetByte();
	if (ObjectTypes.GetObjectFlag(Type, 10))
		Data = Network.GetByte();
	if (Inventory.IsContainerOpen(Container))
		Inventory.InsertContainerObject(Container, TObject{Type, (int)Data, 0});
}

void patella::TCommunication::ReceiveContainerChangeObject()
{
	int Container = 0;
	int Type = 0;
	Container = Network.GetByte();
	const int Position = Network.GetByte();
	Type = Network.GetWord();
	int Data = 0;
	if (ObjectTypes.GetObjectFlag(Type, 5))
		Data = Network.GetByte();
	if (ObjectTypes.GetObjectFlag(Type, 11))
		Data = Network.GetByte();
	if (ObjectTypes.GetObjectFlag(Type, 10))
		Data = Network.GetByte();
	if (Inventory.IsContainerOpen(Container))
		Inventory.ChangeContainerObject(Container, Position, TObject{Type, (int)Data, 0});
}

void patella::TCommunication::ReceiveContainerDeleteObject()
{
	int Container = 0;
	int Position = 0;
	Container = Network.GetByte();
	Position = Network.GetByte();
	if (Inventory.IsContainerOpen(Container)) {
		if (Inventory.DeleteContainerObject(Container, Position))
			SendRefreshContainer(Container);
	}
}

void patella::TCommunication::ReceiveInventoryObject()
{
	int Position = 0;
	int Type = 0;
	Position = Network.GetByte();
	Type = Network.GetWord();
	int Data = 0;
	if (ObjectTypes.GetObjectFlag(Type, 5))
		Data = Network.GetByte();
	if (ObjectTypes.GetObjectFlag(Type, 11))
		Data = Network.GetByte();
	if (ObjectTypes.GetObjectFlag(Type, 10))
		Data = Network.GetByte();
	Inventory.SetBodyObject(Position, TObject{Type, (int)Data, 0});
}

void patella::TCommunication::ReceiveDeleteInventoryObject()
{
	int Position = 0;
	Position = Network.GetByte();
	Inventory.DeleteBodyObject(Position);
}

void patella::TCommunication::ReceiveOwnTrade()
{
	TFixedString<40> Name;
	Network.GetString(patella::TSpan<char>(Name.Chars).first(30));
	Inventory.SetOwnOffer(Name);
	const int Count = Network.GetByte();
	for (int i = 0; i < Count; i++) {
		const int Type = Network.GetWord();
		int Data = 0;
		if (ObjectTypes.GetObjectFlag(Type, 5))
			Data = Network.GetByte();
		if (ObjectTypes.GetObjectFlag(Type, 11))
			Data = Network.GetByte();
		if (ObjectTypes.GetObjectFlag(Type, 10))
			Data = Network.GetByte();
		Inventory.AddOwnOfferObject(TObject{Type, (int)Data, 0});
	}
}

void patella::TCommunication::ReceiveCounterTrade()
{
	TFixedString<40> Name;
	Network.GetString(patella::TSpan<char>(Name.Chars).first(30));
	const bool Open = Inventory.SetCounterOffer(Name);
	const int Count = Network.GetByte();
	for (int i = 0; i < Count; i++) {
		const int Type = Network.GetWord();
		int Data = 0;
		if (ObjectTypes.GetObjectFlag(Type, 5))
			Data = Network.GetByte();
		if (ObjectTypes.GetObjectFlag(Type, 11))
			Data = Network.GetByte();
		if (ObjectTypes.GetObjectFlag(Type, 10))
			Data = Network.GetByte();
		if (Open)
			Inventory.AddCounterOfferObject(TObject{Type, (int)Data, 0});
	}
}

void patella::TCommunication::ReceiveCloseTrade()
{
	Inventory.ClearOffers();
}

void patella::TCommunication::ReceiveWorldLight()
{
	const int Brightness = Network.GetByte();
	const int Color = Network.GetByte();
	Map.SetAmbientLight(Brightness, Color);
}

void patella::TCommunication::ReceiveMagicEffect()
{
	int rx = 0;
	int ry = 0;
	int rz = 0;
	rx = Network.GetWord();
	ry = Network.GetWord();
	rz = Network.GetByte();
	Map.WorldToMap(&rx, &ry, &rz);
	const int Effect = Network.GetByte();
	Map.AddGraphicalEffect(rx, ry, rz, Effect);
}

void patella::TCommunication::ReceiveAnimatedText()
{
	int rx = 0;
	int ry = 0;
	int rz = 0;
	rx = Network.GetWord();
	ry = Network.GetWord();
	rz = Network.GetByte();
	Map.WorldToMap(&rx, &ry, &rz);
	const int Color = Network.GetByte();
	TFixedString<24> Text;
	Network.GetString(patella::TSpan<char>(Text.Chars).first(10));
	Map.AddTextualEffect(rx, ry, rz, Color, Text);
}

void patella::TCommunication::ReceiveDistanceEffect()
{
	int sx = 0;
	int sy = 0;
	int sz = 0;
	sx = Network.GetWord();
	sy = Network.GetWord();
	sz = Network.GetByte();
	Map.WorldToMap(&sx, &sy, &sz);
	int dx = Network.GetWord();
	int dy = Network.GetWord();
	int dz = Network.GetByte();
	Map.WorldToMap(&dx, &dy, &dz);
	const int Effect = Network.GetByte();
	Map.AddMissileEffect(sx, sy, sz, dx, dy, dz, Effect);
}

void patella::TCommunication::ReceiveCreatureMark()
{
	uint32 CreatureID = 0;
	CreatureID = Network.GetQuad();
	TCreature *Creature = Creatures.GetCreature(CreatureID);
	if (Creature != nullptr) {
		const int Value = Network.GetByte();
		Creature->SetMark(Value);
		return;
	}
	Network.GetByte();
}

void patella::TCommunication::ReceiveCreatureHealth()
{
	uint32 CreatureID = 0;
	CreatureID = Network.GetQuad();
	TCreature *Creature = Creatures.GetCreature(CreatureID);
	if (Creature != nullptr) {
		const int Value = Network.GetByte();
		Creature->Health = Value;
		return;
	}
	Network.GetByte();
}

void patella::TCommunication::ReceiveCreatureLight()
{
	uint32 CreatureID = 0;
	CreatureID = Network.GetQuad();
	TCreature *Creature = Creatures.GetCreature(CreatureID);
	if (Creature != nullptr) {
		Creature->LightStrength = Network.GetByte();
		Creature->LightColor = Network.GetByte();
		return;
	}
	Network.GetByte();
	Network.GetByte();
}

void patella::TCommunication::ReceiveCreatureOutfit()
{
	uint32 CreatureID = 0;
	CreatureID = Network.GetQuad();
	TCreature *Creature = Creatures.GetCreature(CreatureID);
	const int Type = Network.GetWord();
	if (Creature == nullptr) {
		if (Type != 0) {
			Network.GetByte();
			Network.GetByte();
			Network.GetByte();
			Network.GetByte();
			return;
		}
		Network.GetWord();
		return;
	}
	Creature->OutfitType = Type;
	if (Type != 0) {
		Creature->Colors[0] = Network.GetByte();
		Creature->Colors[1] = Network.GetByte();
		Creature->Colors[2] = Network.GetByte();
		Creature->Colors[3] = Network.GetByte();
		return;
	}
	Creature->Colors[0] = Network.GetWord();
}

void patella::TCommunication::ReceiveCreatureSpeed()
{
	uint32 CreatureID = 0;
	CreatureID = Network.GetQuad();
	TCreature *Creature = Creatures.GetCreature(CreatureID);
	if (Creature != nullptr) {
		const int Value = Network.GetWord();
		Creature->Speed = Value;
		return;
	}
	Network.GetWord();
}

void patella::TCommunication::ReceiveCreatureSkull()
{
	uint32 CreatureID = 0;
	CreatureID = Network.GetQuad();
	TCreature *Creature = Creatures.GetCreature(CreatureID);
	if (Creature != nullptr) {
		const int Skull = Network.GetByte();
		Creature->SetSkull(Skull, true);
		Creatures.NotifySkull(Creature->PKFlag);
		return;
	}
	Network.GetByte();
}

void patella::TCommunication::ReceiveCreatureShield()
{
	uint32 CreatureID = 0;
	CreatureID = Network.GetQuad();
	TCreature *Creature = Creatures.GetCreature(CreatureID);
	if (Creature != nullptr) {
		const int Value = Network.GetByte();
		Creature->PARTYFlag = Value;
		return;
	}
	Network.GetByte();
}

void patella::TCommunication::ReceiveEditText()
{
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader (notes/ownership.md)
	uint32 *Info = new uint32[3];
	Info[0] = Network.GetQuad();
	Info[1] = Network.GetWord();
	Info[2] = Network.GetWord();
	char *Text = Network.GetString();
	char *Writer = Network.GetString();
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader (notes/ownership.md)
	char **Strings = new char *[2];
	Strings[0] = Writer;
	Strings[1] = Text;
	AddMessage(MESSAGE_TEXT_DIALOG, reinterpret_cast<std::intptr_t>(Info), reinterpret_cast<std::intptr_t>(Strings));
}

void patella::TCommunication::ReceiveEditHouseText()
{
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader (notes/ownership.md)
	uint32 *Info = new uint32[2];
	Info[0] = Network.GetByte();
	Info[1] = Network.GetQuad();
	const char *Text = Network.GetString();
	AddMessage(MESSAGE_EDIT_LIST_DIALOG, reinterpret_cast<std::intptr_t>(Info), reinterpret_cast<std::intptr_t>(Text));
}

void patella::TCommunication::ReceivePlayerStats()
{
	const int Health = Network.GetWord();
	const int MaxHealth = Network.GetWord();
	const int Capacity = Network.GetWord();
	const uint32 Experience = Network.GetQuad();
	const int Level = Network.GetWord();
	const int LevelPercent = Network.GetByte();
	const int Mana = Network.GetWord();
	const int MaxMana = Network.GetWord();
	const int MagicLevel = Network.GetByte();
	const int MagicLevelPercent = Network.GetByte();
	const int Soul = Network.GetByte();
	PlayerState.SetPlayerHealth(Health, MaxHealth);
	PlayerState.SetPlayerCapacity(Capacity);
	PlayerState.SetPlayerMana(Mana, MaxMana);
	PlayerState.SetPlayerSoul(Soul);
	PlayerState.SetPlayerExperience(Experience, Level, LevelPercent);
	PlayerState.SetPlayerMagicLevel(MagicLevel, MagicLevelPercent);
}

void patella::TCommunication::ReceivePlayerSkills()
{
	int i;
	for (i = 0; i < 7; i++) {
		const int Level = Network.GetByte();
		const int Percent = Network.GetByte();
		PlayerState.SetPlayerSkill(i, Level, Percent);
	}
}

void patella::TCommunication::ReceivePlayerIcons()
{
	const int Icons = Network.GetByte();
	PlayerState.SetPlayerState(Icons);
}

void patella::TCommunication::ReceiveCancelAttack()
{
	Creatures.SetAttackID(0);
}

void patella::TCommunication::ReceiveTalk()
{
	int Mode = TALK_SAY;
	int Channel = 0;
	const uint32 StatementID = Network.GetQuad();
	TFixedString<36> Name;
	Network.GetString(patella::TSpan<char>(Name.Chars).first(30));
	Mode = Network.GetByte();
	TFixedString<632> Text;
	int Time = 0;
	int TargetChannel = -1;
	switch (Mode) {
	case TALK_SAY:
	case TALK_WHISPER:
	case TALK_YELL: {
		const int x = Network.GetWord();
		const int y = Network.GetWord();
		const int z = Network.GetByte();
		Network.GetString(patella::TSpan<char>(Text.Chars).first(512));
		if (IsTalkIgnored(Mode, Name, -1)) {
			StartTalkThrottle();
			return;
		}
		AddTalkbox(Mode, Name, x, y, z, Text);
		break;
	}
	case TALK_PRIVATE_MESSAGE:
	case TALK_GAMEMASTER_BROADCAST:
	case TALK_GAMEMASTER_MESSAGE:
		Network.GetString(patella::TSpan<char>(Text.Chars).first(512));
		if (IsTalkIgnored(Mode, Name, -1)) {
			StartTalkThrottle();
			return;
		}
		AddTalkbox(Mode, Name, 0, 0, 0, Text);
		break;
	case TALK_CHANNEL_CALL:
	case TALK_GAMEMASTER_CHANNELCALL:
	case 12:
	case TALK_ANONYMOUS_CHANNELCALL:
		Channel = Network.GetWord();
		Network.GetString(patella::TSpan<char>(Text.Chars).first(512));
		if (IsTalkIgnored(Mode, Name, -1)) {
			StartTalkThrottle();
			return;
		}
		TargetChannel = Channel;
		break;
	case TALK_GAMEMASTER_REQUEST:
		Time = -(int)Network.GetQuad();
		Network.GetString(patella::TSpan<char>(Text.Chars).first(512));
		break;
	case TALK_GAMEMASTER_ANSWER:
	case TALK_PLAYER_ANSWER:
		Network.GetString(patella::TSpan<char>(Text.Chars).first(512));
		break;
	case TALK_ANIMAL_LOW:
	case TALK_ANIMAL_LOUD: {
		const int x = Network.GetWord();
		const int y = Network.GetWord();
		const int z = Network.GetByte();
		Network.GetString(patella::TSpan<char>(Text.Chars).first(512));
		if (!IsTalkIgnored(Mode, Name, -1)) {
			AddTalkbox(Mode, Name, x, y, z, Text);
			return;
		}
		StartTalkThrottle();
		return;
	}
	default:
		TERROR1_REASON("unknown talk mode", Mode, "[bug0000649]");
	}
	AddConsoleText(StatementID, Text, Mode, Name, TargetChannel, Time);
}

void patella::TCommunication::ReceiveChannelList()
{
	int Count = 0;
	Count = Network.GetByte();
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader (notes/ownership.md)
	int *Channels = new int[Count + 1];
	char *Names = nullptr;
	if (Count != 0)
		// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader (notes/ownership.md)
		Names = new char[Count * 30];
	Channels[0] = Count;
	for (int i = 0; i < Count; i++) {
		Channels[i + 1] = Network.GetWord();
		Network.GetString(patella::TSpan<char>(Names + i * 30, 30));
	}
	AddMessage(MESSAGE_NEW_CHANNEL_DIALOG, reinterpret_cast<std::intptr_t>(Channels), reinterpret_cast<std::intptr_t>(Names));
}

void patella::TCommunication::ReceiveOpenChannel()
{
	int ChannelID = 0;
	ChannelID = Network.GetWord();
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader (notes/ownership.md)
	char *Name = new char[30];
	Network.GetString(patella::TSpan<char>(Name, 30));
	AddMessage(MESSAGE_OPEN_CHANNEL, ChannelID, reinterpret_cast<std::intptr_t>(Name));
}

void patella::TCommunication::ReceiveOpenPrivateConversation()
{
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader (notes/ownership.md)
	char *Name = new char[30];
	Network.GetString(patella::TSpan<char>(Name, 30));
	AddMessage(MESSAGE_OPEN_PRIVATE_CONVERSATION, 0, reinterpret_cast<std::intptr_t>(Name));
}

void patella::TCommunication::ReceiveRuleViolationChannel()
{
	int ChannelID = 0;
	ChannelID = Network.GetWord();
	const TTalkBuffer *Channel = CreateTextBuffer(TALK_GAMEMASTER_REQUEST, TALK_SAY, nullptr, ChannelID);
	if (Channel == nullptr)
		Channel = GetTextBuffer(TALK_GAMEMASTER_REQUEST, nullptr, -1);
	AddMessage(MESSAGE_RULE_VIOLATION_CHANNEL, reinterpret_cast<std::intptr_t>(Channel), 0);
}

void patella::TCommunication::ReceiveRemoveRuleViolation()
{
	TFixedString<44> Name;
	Network.GetString(patella::TSpan<char>(Name.Chars).first(30));
	TTalkBuffer *Channel = GetTextBuffer(TALK_GAMEMASTER_REQUEST, nullptr, -1);
	if (Channel != nullptr) {
		Channel->cursor = Channel->first;
		TTalkLine *Statement = Channel->first;
		while (Statement != nullptr) {
			TTalkLine *Current = Statement;
			Statement = Channel->GetNext();
			if (CompareStrings(Current->Addressee, Name, -1) == 0) {
				Channel->Remove(Current);
			}
		}
		SetConsoleChanged();
	}
}

void patella::TCommunication::ReceiveCancelRuleViolation()
{
	TFixedString<44> Name;
	Network.GetString(patella::TSpan<char>(Name.Chars).first(30));
	TTalkBuffer *Channel = GetTextBuffer(TALK_PLAYER_ANSWER, Name, -1);
	if (Channel != nullptr) {
		TFixedString<444> Text;
		Text.Format("{} has finished the request.", Name);
		AddTextBufferLine(Channel, 0, Text, TALK_ADMIN_MESSAGE, nullptr, 0);
		Channel->SetWriteEnabled(false);
	}
}

void patella::TCommunication::ReceiveLockRuleViolation()
{
	TTalkBuffer *Channel = GetTextBuffer(TALK_GAMEMASTER_ANSWER, nullptr, -1);
	if (Channel != nullptr) {
		const char *Text = TextRequestClosed;
		AddTextBufferLine(Channel, 0, Text, TALK_ADMIN_MESSAGE, nullptr, 0);
		Channel->SetWriteEnabled(false);
	}
}

void patella::TCommunication::ReceiveOwnChannel()
{
	int ChannelID = 0;
	ChannelID = Network.GetWord();
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader (notes/ownership.md)
	char *Name = new char[40];
	Network.GetString(patella::TSpan<char>(Name, 40));
	AddMessage(MESSAGE_OPEN_OWN_CHANNEL, ChannelID, reinterpret_cast<std::intptr_t>(Name));
}

void patella::TCommunication::ReceiveCloseChannel()
{
	int ChannelID = 0;
	ChannelID = Network.GetWord();
	TTalkBuffer *Channel = GetTextBuffer(TALK_CHANNEL_CALL, nullptr, ChannelID);
	if (Channel != nullptr) {
		const char *Text = TextChannelClosed;
		AddTextBufferLine(Channel, 0, Text, TALK_ADMIN_MESSAGE, nullptr, 0);
		Channel->SetWriteEnabled(false);
	}
}

void patella::TCommunication::ReceiveTextMessage()
{
	int Mode = 0;
	Mode = Network.GetByte();
	PATELLA_ASSERT_VALUES(Mode>=0&&Mode<TALK_MODE_BEYONDLAST, Mode);
	TFixedString<632> Text;
	Network.GetString(patella::TSpan<char>(Text.Chars).first(512));
	if (Mode == TALK_ADMIN_MESSAGE || Mode == TALK_EVENT_MESSAGE || Mode == TALK_INFO_MESSAGE)
		AddMessageTalkbox(Mode, Text);
	if (Mode == 23 || Mode == TALK_STATUS_MESSAGE || Mode == 20)
		SetStatusMessage(Text, false);
	if (Mode != 23)
		AddConsoleText(0, Text, Mode, nullptr, -1, 0);
}

void patella::TCommunication::ReceiveCancelWalk()
{
	const int Direction = Network.GetByte();
	int x;
	int y;
	int z;
	PlayerState.GetPlayerPosition(&x, &y, &z);
	if (x == LastCancelX && y == LastCancelY && z == LastCancelZ)
		CancelCount++;
	else
		CancelCount = 0;
	LastCancelX = x;
	LastCancelY = y;
	LastCancelZ = z;
	if (CancelCount > 15) {
		PlayerState.ClearPlayerSteps();
		Creatures.SetAttackID(0);
		SendCancel();
		CancelCount = 0;
	}
	if (PlayerState.CancelPlayerWalk(Direction))
		SendWalk();
}

void patella::TCommunication::ReadFloor(int rz)
{
	// Interior loop, not a separately callable function.
	for (int rx = 0; rx < 18; rx++) {
		for (int ry = 0; ry < 14; ry++)
			ReadField(rx, ry, rz);
	}
}

void patella::TCommunication::ReceiveFloorUp()
{
	int x;
	int y;
	int z;
	Map.GetMapPosition(&x, &y, &z);
	z += -1;
	x += 1;
	y += 1;
	Map.SetMapPosition(x, y, z);
	if (z < 8) {
		if (z == 7) {
			Map.ShiftMapLevel(-3);
			SkipFields = 0;
			for (int rz = 2; rz < 8; rz++)
				ReadFloor(rz);
		}
	} else {
		Map.ShiftMapLevel(-1);
		SkipFields = 0;
		ReadFloor(4);
	}
	SetTalkChanged();
	PlayerState.StopPlayer();
	CheckEventHelpPosition(x, y, z);
	for (int rx = 0; rx < 18; rx++) {
		for (int ry = 0; ry < 14; ry++)
			Map.UpdateAutomap(rx, ry);
	}
}

void patella::TCommunication::ReceiveFloorDown()
{
	int x;
	int y;
	int z;
	Map.GetMapPosition(&x, &y, &z);
	z += 1;
	x += -1;
	y += -1;
	Map.SetMapPosition(x, y, z);
	if (z < 9) {
		if (z == 8) {
			Map.ShiftMapLevel(3);
			SkipFields = 0;
			for (int rz = 2; rz >= 0; rz--)
				ReadFloor(rz);
		}
	} else {
		Map.ShiftMapLevel(1);
		SkipFields = 0;
		if (z < 14)
			ReadFloor(0);
	}
	SetTalkChanged();
	PlayerState.StopPlayer();
	CheckEventHelpPosition(x, y, z);
	for (int rx = 0; rx < 18; rx++) {
		for (int ry = 0; ry < 14; ry++)
			Map.UpdateAutomap(rx, ry);
	}
}

void patella::TCommunication::ReceiveOutfit()
{
	// NOLINTNEXTLINE(cppcoreguidelines-owning-memory): a block of the message queue (AddMessage), freed by its reader (notes/ownership.md)
	int *Outfit = new int[7];
	Outfit[0] = Network.GetWord();
	Outfit[1] = Network.GetByte();
	Outfit[2] = Network.GetByte();
	Outfit[3] = Network.GetByte();
	Outfit[4] = Network.GetByte();
	Outfit[5] = Network.GetWord();
	Outfit[6] = Network.GetWord();
	AddMessage(MESSAGE_OUTFIT_DIALOG, reinterpret_cast<std::intptr_t>(Outfit), 0);
}

// Only the byte value 1 means online, not every nonzero value.
void patella::TCommunication::ReceiveBuddy()
{
	uint32 CharacterID = 0;
	CharacterID = Network.GetQuad();
	TFixedString<44> Name;
	Network.GetString(patella::TSpan<char>(Name.Chars).first(30));
	const int State = Network.GetByte();
	Creatures.AddBuddy(CharacterID, Name, State == 1);
	const char *Copy = MsvcStrdup(Name);
	AddMessage(MESSAGE_TEMPORARY_IGNORE, reinterpret_cast<std::intptr_t>(Copy), 0);
}

void patella::TCommunication::ReceiveBuddyLogin()
{
	uint32 CharacterID = 0;
	CharacterID = Network.GetQuad();
	Creatures.SetBuddyState(CharacterID, true);
}

void patella::TCommunication::ReceiveBuddyLogout()
{
	uint32 CharacterID = 0;
	CharacterID = Network.GetQuad();
	Creatures.SetBuddyState(CharacterID, false);
}

// Empty. ExitControl calls it where Communication.cpp's exit belongs in the reverse
// order of InitControl (name ours, low confidence).
void patella::TCommunication::ExitCommunication()
{
}

// The original's functions (issue #105): each calls its member on the one TClient's TCommunication.

void InitCommunication()
{
	patella::TCommunication::InitCommunication();
}

void ExitCommunication()
{
	patella::TCommunication::ExitCommunication();
}

void SendLogin(uint32 AccountID, const char *Password)
{
	patella::bridge::Communication().SendLogin(AccountID, Password);
}

void SendGameLogin(uint32 AccountID, const char *Name, const char *Password)
{
	patella::bridge::Communication().SendGameLogin(AccountID, Name, Password);
}

void SendLogout()
{
	patella::bridge::Communication().SendLogout();
}

void SendPing()
{
	patella::bridge::Communication().SendPing();
}

void PrepareMovement()
{
	patella::bridge::Communication().PrepareMovement();
}

void SendWalk()
{
	patella::bridge::Communication().SendWalk();
}

void SendStop()
{
	patella::bridge::Communication().SendStop();
}

void SendTurnNorth()
{
	patella::bridge::Communication().SendTurnNorth();
}

void SendTurnEast()
{
	patella::bridge::Communication().SendTurnEast();
}

void SendTurnSouth()
{
	patella::bridge::Communication().SendTurnSouth();
}

void SendTurnWest()
{
	patella::bridge::Communication().SendTurnWest();
}

void SendMoveObject(int x, int y, int z, int ObjectType, int Position, int DestinationX, int DestinationY, int DestinationZ, int Count)
{
	patella::bridge::Communication().SendMoveObject(x, y, z, ObjectType, Position, DestinationX, DestinationY, DestinationZ, Count);
}

void SendRequestTrade(int x, int y, int z, int ObjectType, int Position, uint32 CreatureID)
{
	patella::bridge::Communication().SendRequestTrade(x, y, z, ObjectType, Position, CreatureID);
}

void SendInspectTrade(int Side, int Position)
{
	patella::bridge::Communication().SendInspectTrade(Side, Position);
}

void SendAcceptTrade()
{
	patella::bridge::Communication().SendAcceptTrade();
}

void SendRejectTrade()
{
	patella::bridge::Communication().SendRejectTrade();
}

void SendUseObject(int x, int y, int z, int ObjectType, int Position, int Container)
{
	patella::bridge::Communication().SendUseObject(x, y, z, ObjectType, Position, Container);
}

void SendUseObjectWith(int x1, int y1, int z1, int ObjectType1, int Position1, int x2, int y2, int z2, int ObjectType2, int Position2)
{
	patella::bridge::Communication().SendUseObjectWith(x1, y1, z1, ObjectType1, Position1, x2, y2, z2, ObjectType2, Position2);
}

void SendUseObjectOnCreature(int x, int y, int z, int ObjectType, int Position, uint32 CreatureID)
{
	patella::bridge::Communication().SendUseObjectOnCreature(x, y, z, ObjectType, Position, CreatureID);
}

void SendRotateObject(int x, int y, int z, int ObjectType, int Position)
{
	patella::bridge::Communication().SendRotateObject(x, y, z, ObjectType, Position);
}

void SendCloseContainer(int Container)
{
	patella::bridge::Communication().SendCloseContainer(Container);
}

void SendContainerUp(int Container)
{
	patella::bridge::Communication().SendContainerUp(Container);
}

void SendEditText(uint32 ObjectNumber, const char *Text)
{
	patella::bridge::Communication().SendEditText(ObjectNumber, Text);
}

void SendEditHouseText(int Type, uint32 Identifier, const char *Text)
{
	patella::bridge::Communication().SendEditHouseText(Type, Identifier, Text);
}

void SendLook(int x, int y, int z, int ObjectType, int Position)
{
	patella::bridge::Communication().SendLook(x, y, z, ObjectType, Position);
}

void SendTalk(int Mode, const char *Text)
{
	patella::bridge::Communication().SendTalk(Mode, Text);
}

void SendTalk(int Mode, const char *Addressee, const char *Text)
{
	patella::bridge::Communication().SendTalk(Mode, Addressee, Text);
}

void SendTalk(int Mode, int ChannelID, const char *Text)
{
	patella::bridge::Communication().SendTalk(Mode, ChannelID, Text);
}

void SendRequestChannels()
{
	patella::bridge::Communication().SendRequestChannels();
}

void SendJoinChannel(int ChannelID)
{
	patella::bridge::Communication().SendJoinChannel(ChannelID);
}

void SendLeaveChannel(int ChannelID)
{
	patella::bridge::Communication().SendLeaveChannel(ChannelID);
}

void SendOpenPrivateConversation(const char *Name)
{
	patella::bridge::Communication().SendOpenPrivateConversation(Name);
}

void SendProcessRuleViolation(const char *Name)
{
	patella::bridge::Communication().SendProcessRuleViolation(Name);
}

void SendCloseRuleViolation(const char *Name)
{
	patella::bridge::Communication().SendCloseRuleViolation(Name);
}

void SendCancelRuleViolation()
{
	patella::bridge::Communication().SendCancelRuleViolation();
}

void SendFightModes()
{
	patella::bridge::Communication().SendFightModes();
}

void SendAttack(uint32 CreatureID)
{
	patella::bridge::Communication().SendAttack(CreatureID);
}

void SendFollow(uint32 CreatureID)
{
	patella::bridge::Communication().SendFollow(CreatureID);
}

void SendOpenPrivateChannel()
{
	patella::bridge::Communication().SendOpenPrivateChannel();
}

void SendInviteToPrivateChannel(const char *Name)
{
	patella::bridge::Communication().SendInviteToPrivateChannel(Name);
}

void SendExcludeFromPrivateChannel(const char *Name)
{
	patella::bridge::Communication().SendExcludeFromPrivateChannel(Name);
}

void SendInviteToParty(uint32 CharacterID)
{
	patella::bridge::Communication().SendInviteToParty(CharacterID);
}

void SendJoinParty(uint32 CharacterID)
{
	patella::bridge::Communication().SendJoinParty(CharacterID);
}

void SendRevokePartyInvitation(uint32 CharacterID)
{
	patella::bridge::Communication().SendRevokePartyInvitation(CharacterID);
}

void SendPassPartyLeadership(uint32 CharacterID)
{
	patella::bridge::Communication().SendPassPartyLeadership(CharacterID);
}

void SendLeaveParty()
{
	patella::bridge::Communication().SendLeaveParty();
}

void SendCancel()
{
	patella::bridge::Communication().SendCancel();
}

void SendRefreshField(int x, int y, int z)
{
	patella::bridge::Communication().SendRefreshField(x, y, z);
}

void SendRefreshContainer(int Container)
{
	patella::bridge::Communication().SendRefreshContainer(Container);
}

void SendRequestOutfit()
{
	patella::bridge::Communication().SendRequestOutfit();
}

void SendOutfit(int OutfitType, const int *Colors)
{
	patella::bridge::Communication().SendOutfit(OutfitType, Colors);
}

void SendAddBuddy(const char *Name)
{
	patella::bridge::Communication().SendAddBuddy(Name);
}

void SendRemoveBuddy(uint32 CharacterID)
{
	patella::bridge::Communication().SendRemoveBuddy(CharacterID);
}

void SendBugReport(const char *Text)
{
	patella::bridge::Communication().SendBugReport(Text);
}

void SendRuleViolationReport(const char *Name, uint8 Reason, uint8 Action, const char *Comment, uint32 StatementID, bool IPBanishment)
{
	patella::bridge::Communication().SendRuleViolationReport(Name, Reason, Action, Comment, StatementID, IPBanishment);
}

void ReceiveLoginError()
{
	patella::bridge::Communication().ReceiveLoginError();
}

void ReceiveLoginMessage()
{
	patella::bridge::Communication().ReceiveLoginMessage();
}

void ReceiveMessageOfTheDay()
{
	patella::bridge::Communication().ReceiveMessageOfTheDay();
}

void ReceiveUpdateRequest()
{
	patella::bridge::Communication().ReceiveUpdateRequest();
}

void ReceiveCharacterList()
{
	patella::bridge::Communication().ReceiveCharacterList();
}

void ReceiveLoginServerOverloaded()
{
	patella::bridge::Communication().ReceiveLoginServerOverloaded();
}

void ReceiveGameLogin()
{
	patella::bridge::Communication().ReceiveGameLogin();
}

void ReceiveGamemasterActions()
{
	patella::bridge::Communication().ReceiveGamemasterActions();
}

void ReceiveGameError()
{
	patella::bridge::Communication().ReceiveGameError();
}

void ReceiveGameMessage()
{
	patella::bridge::Communication().ReceiveGameMessage();
}

void ReceiveLoginWait()
{
	patella::bridge::Communication().ReceiveLoginWait();
}

void ReceivePing()
{
	patella::bridge::Communication().ReceivePing();
}

uint32 ReadCreature(int Type, int x, int y, int z)
{
	return patella::bridge::Communication().ReadCreature(Type, x, y, z);
}

void ReadField(int rx, int ry, int rz)
{
	patella::bridge::Communication().ReadField(rx, ry, rz);
}

void ReadMap(int xmin, int ymin, int xmax, int ymax)
{
	patella::bridge::Communication().ReadMap(xmin, ymin, xmax, ymax);
}

void ReceiveMap()
{
	patella::bridge::Communication().ReceiveMap();
}

void ReceiveMapNorth()
{
	patella::bridge::Communication().ReceiveMapNorth();
}

void ReceiveMapEast()
{
	patella::bridge::Communication().ReceiveMapEast();
}

void ReceiveMapSouth()
{
	patella::bridge::Communication().ReceiveMapSouth();
}

void ReceiveMapWest()
{
	patella::bridge::Communication().ReceiveMapWest();
}

void ReceiveField()
{
	patella::bridge::Communication().ReceiveField();
}

void ReceiveAddObject()
{
	patella::bridge::Communication().ReceiveAddObject();
}

void ReceiveChangeObject()
{
	patella::bridge::Communication().ReceiveChangeObject();
}

void ReceiveDeleteObject()
{
	patella::bridge::Communication().ReceiveDeleteObject();
}

void ReceiveMoveCreature()
{
	patella::bridge::Communication().ReceiveMoveCreature();
}

void ReceiveOpenContainer(int &Container)
{
	patella::bridge::Communication().ReceiveOpenContainer(Container);
}

void ReceiveCloseContainer(int &Container)
{
	patella::bridge::Communication().ReceiveCloseContainer(Container);
}

void ReceiveContainerAddObject()
{
	patella::bridge::Communication().ReceiveContainerAddObject();
}

void ReceiveContainerChangeObject()
{
	patella::bridge::Communication().ReceiveContainerChangeObject();
}

void ReceiveContainerDeleteObject()
{
	patella::bridge::Communication().ReceiveContainerDeleteObject();
}

void ReceiveInventoryObject()
{
	patella::bridge::Communication().ReceiveInventoryObject();
}

void ReceiveDeleteInventoryObject()
{
	patella::bridge::Communication().ReceiveDeleteInventoryObject();
}

void ReceiveOwnTrade()
{
	patella::bridge::Communication().ReceiveOwnTrade();
}

void ReceiveCounterTrade()
{
	patella::bridge::Communication().ReceiveCounterTrade();
}

void ReceiveCloseTrade()
{
	patella::bridge::Communication().ReceiveCloseTrade();
}

void ReceiveWorldLight()
{
	patella::bridge::Communication().ReceiveWorldLight();
}

void ReceiveMagicEffect()
{
	patella::bridge::Communication().ReceiveMagicEffect();
}

void ReceiveAnimatedText()
{
	patella::bridge::Communication().ReceiveAnimatedText();
}

void ReceiveDistanceEffect()
{
	patella::bridge::Communication().ReceiveDistanceEffect();
}

void ReceiveCreatureMark()
{
	patella::bridge::Communication().ReceiveCreatureMark();
}

void ReceiveCreatureHealth()
{
	patella::bridge::Communication().ReceiveCreatureHealth();
}

void ReceiveCreatureLight()
{
	patella::bridge::Communication().ReceiveCreatureLight();
}

void ReceiveCreatureOutfit()
{
	patella::bridge::Communication().ReceiveCreatureOutfit();
}

void ReceiveCreatureSpeed()
{
	patella::bridge::Communication().ReceiveCreatureSpeed();
}

void ReceiveCreatureSkull()
{
	patella::bridge::Communication().ReceiveCreatureSkull();
}

void ReceiveCreatureShield()
{
	patella::bridge::Communication().ReceiveCreatureShield();
}

void ReceiveEditText()
{
	patella::bridge::Communication().ReceiveEditText();
}

void ReceiveEditHouseText()
{
	patella::bridge::Communication().ReceiveEditHouseText();
}

void ReceivePlayerStats()
{
	patella::bridge::Communication().ReceivePlayerStats();
}

void ReceivePlayerSkills()
{
	patella::bridge::Communication().ReceivePlayerSkills();
}

void ReceivePlayerIcons()
{
	patella::bridge::Communication().ReceivePlayerIcons();
}

void ReceiveCancelAttack()
{
	patella::bridge::Communication().ReceiveCancelAttack();
}

void ReceiveTalk()
{
	patella::bridge::Communication().ReceiveTalk();
}

void ReceiveChannelList()
{
	patella::bridge::Communication().ReceiveChannelList();
}

void ReceiveOpenChannel()
{
	patella::bridge::Communication().ReceiveOpenChannel();
}

void ReceiveOpenPrivateConversation()
{
	patella::bridge::Communication().ReceiveOpenPrivateConversation();
}

void ReceiveRuleViolationChannel()
{
	patella::bridge::Communication().ReceiveRuleViolationChannel();
}

void ReceiveRemoveRuleViolation()
{
	patella::bridge::Communication().ReceiveRemoveRuleViolation();
}

void ReceiveCancelRuleViolation()
{
	patella::bridge::Communication().ReceiveCancelRuleViolation();
}

void ReceiveLockRuleViolation()
{
	patella::bridge::Communication().ReceiveLockRuleViolation();
}

void ReceiveOwnChannel()
{
	patella::bridge::Communication().ReceiveOwnChannel();
}

void ReceiveCloseChannel()
{
	patella::bridge::Communication().ReceiveCloseChannel();
}

void ReceiveTextMessage()
{
	patella::bridge::Communication().ReceiveTextMessage();
}

void ReceiveCancelWalk()
{
	patella::bridge::Communication().ReceiveCancelWalk();
}

void ReadFloor(int rz)
{
	patella::bridge::Communication().ReadFloor(rz);
}

void ReceiveFloorUp()
{
	patella::bridge::Communication().ReceiveFloorUp();
}

void ReceiveFloorDown()
{
	patella::bridge::Communication().ReceiveFloorDown();
}

void ReceiveOutfit()
{
	patella::bridge::Communication().ReceiveOutfit();
}

void ReceiveBuddy()
{
	patella::bridge::Communication().ReceiveBuddy();
}

void ReceiveBuddyLogin()
{
	patella::bridge::Communication().ReceiveBuddyLogin();
}

void ReceiveBuddyLogout()
{
	patella::bridge::Communication().ReceiveBuddyLogout();
}
