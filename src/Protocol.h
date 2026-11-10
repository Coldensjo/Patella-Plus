// Protocol.h: the numbers of the Tibia 7.72 protocol, named, so that a change of the protocol is made in one place.
// Not part of the original source.
//
// A packet starts with one byte, its type. The two servers and the two directions each have their own numbers, so
// one value can have several names here:
//
//   LOGIN_SEND_*     the client to the login server: BeginCommand's number in SendLogin (Communication.cpp).
//   LOGIN_RECEIVE_*  the login server to the client: the case labels of ReceivePackets (Control.cpp) while the
//                    connection state is 4.
//   GAME_SEND_*      the client to the game server: BeginCommand's numbers in the other Send functions.
//   GAME_RECEIVE_*   the game server to the client: ReceivePackets' other case labels.
//
// SEND and RECEIVE are the client's, as the functions' names. No names of the original are known for the packet
// types: each is the name of the function that sends or handles the packet (SendTalk is GAME_SEND_TALK,
// ReceiveAddObject is GAME_RECEIVE_ADD_OBJECT), without the server's name where the prefix says it already; those
// have their function in a comment.
//
// After the packet types come values inside the packets: CLIENT_VERSION, FIELD_END, and the original's
// UNKNOWNCREATURE and OUTDATEDCREATURE.
//
// Enumerators for the reason GUI.h gives at the modifier flags.

#pragma once

enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	LOGIN_SEND_LOGIN = 0x01,
};

enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	// ReceiveLoginError and ReceiveLoginMessage.
	LOGIN_RECEIVE_ERROR = 0x0a,
	LOGIN_RECEIVE_MESSAGE = 0x0b,
	LOGIN_RECEIVE_MESSAGE_OF_THE_DAY = 0x14,
	// The patch packets, named after the original's handlers (ReceivePatch, ReceiveWholeFile, ReceivePatchBlock):
	// ReceiveUpdateRequest takes all three since Patella-Modern issue #75.
	LOGIN_RECEIVE_PATCH = 0x1e,
	LOGIN_RECEIVE_WHOLE_FILE = 0x1f,
	LOGIN_RECEIVE_PATCH_BLOCK = 0x20,
	LOGIN_RECEIVE_SERVER_OVERLOADED = 0x28, // ReceiveLoginServerOverloaded
	LOGIN_RECEIVE_CHARACTER_LIST = 0x64,
};

enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	// SendGameLogin.
	GAME_SEND_LOGIN = 0x0a,
	GAME_SEND_LOGOUT = 0x14,
	GAME_SEND_PING = 0x1e,
	// SendWalk: GAME_SEND_WALK for several steps (their number and directions follow), else the one step's direction.
	GAME_SEND_WALK = 0x64,
	GAME_SEND_WALK_NORTH = 0x65,
	GAME_SEND_WALK_EAST = 0x66,
	GAME_SEND_WALK_SOUTH = 0x67,
	GAME_SEND_WALK_WEST = 0x68,
	GAME_SEND_STOP_WALK = 0x69,
	GAME_SEND_WALK_NORTH_EAST = 0x6a,
	GAME_SEND_WALK_SOUTH_EAST = 0x6b,
	GAME_SEND_WALK_SOUTH_WEST = 0x6c,
	GAME_SEND_WALK_NORTH_WEST = 0x6d,
	GAME_SEND_TURN_NORTH = 0x6f,
	GAME_SEND_TURN_EAST = 0x70,
	GAME_SEND_TURN_SOUTH = 0x71,
	GAME_SEND_TURN_WEST = 0x72,
	GAME_SEND_MOVE_OBJECT = 0x78,
	GAME_SEND_REQUEST_TRADE = 0x7d,
	GAME_SEND_INSPECT_TRADE = 0x7e,
	GAME_SEND_ACCEPT_TRADE = 0x7f,
	GAME_SEND_REJECT_TRADE = 0x80,
	GAME_SEND_USE_OBJECT = 0x82,
	GAME_SEND_USE_OBJECT_WITH = 0x83,
	GAME_SEND_USE_OBJECT_ON_CREATURE = 0x84,
	GAME_SEND_ROTATE_OBJECT = 0x85,
	GAME_SEND_CLOSE_CONTAINER = 0x87,
	GAME_SEND_CONTAINER_UP = 0x88,
	GAME_SEND_EDIT_TEXT = 0x89,
	GAME_SEND_EDIT_HOUSE_TEXT = 0x8a,
	GAME_SEND_LOOK = 0x8c,
	GAME_SEND_TALK = 0x96, // the three SendTalk
	GAME_SEND_REQUEST_CHANNELS = 0x97,
	GAME_SEND_JOIN_CHANNEL = 0x98,
	GAME_SEND_LEAVE_CHANNEL = 0x99,
	GAME_SEND_OPEN_PRIVATE_CONVERSATION = 0x9a,
	GAME_SEND_PROCESS_RULE_VIOLATION = 0x9b,
	GAME_SEND_CLOSE_RULE_VIOLATION = 0x9c,
	GAME_SEND_CANCEL_RULE_VIOLATION = 0x9d,
	GAME_SEND_FIGHT_MODES = 0xa0,
	GAME_SEND_ATTACK = 0xa1,
	GAME_SEND_FOLLOW = 0xa2,
	GAME_SEND_INVITE_TO_PARTY = 0xa3,
	GAME_SEND_JOIN_PARTY = 0xa4,
	GAME_SEND_REVOKE_PARTY_INVITATION = 0xa5,
	GAME_SEND_PASS_PARTY_LEADERSHIP = 0xa6,
	GAME_SEND_LEAVE_PARTY = 0xa7,
	GAME_SEND_OPEN_PRIVATE_CHANNEL = 0xaa,
	GAME_SEND_INVITE_TO_PRIVATE_CHANNEL = 0xab,
	GAME_SEND_EXCLUDE_FROM_PRIVATE_CHANNEL = 0xac,
	GAME_SEND_STOP_ATTACK_AND_WALK = 0xbe,
	GAME_SEND_REFRESH_FIELD = 0xc9,
	GAME_SEND_REFRESH_CONTAINER = 0xca,
	GAME_SEND_REQUEST_OUTFIT = 0xd2,
	GAME_SEND_OUTFIT = 0xd3,
	GAME_SEND_ADD_BUDDY = 0xdc,
	GAME_SEND_REMOVE_BUDDY = 0xdd,
	GAME_SEND_BUG_REPORT = 0xe6,
	GAME_SEND_RULE_VIOLATION_REPORT = 0xe7,
	// 0xe8, the original's SendDebugReport, is gone since Patella-Modern issue #74.
};

enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	// ReceiveGameLogin.
	GAME_RECEIVE_LOGIN = 0x0a,
	GAME_RECEIVE_GAMEMASTER_ACTIONS = 0x0b,
	GAME_RECEIVE_ERROR = 0x14,   // ReceiveGameError
	GAME_RECEIVE_MESSAGE = 0x15, // ReceiveGameMessage
	GAME_RECEIVE_LOGIN_WAIT = 0x16,
	GAME_RECEIVE_PING = 0x1e,
	GAME_RECEIVE_MAP = 0x64,
	GAME_RECEIVE_MAP_NORTH = 0x65,
	GAME_RECEIVE_MAP_EAST = 0x66,
	GAME_RECEIVE_MAP_SOUTH = 0x67,
	GAME_RECEIVE_MAP_WEST = 0x68,
	GAME_RECEIVE_FIELD = 0x69,
	GAME_RECEIVE_ADD_OBJECT = 0x6a,
	GAME_RECEIVE_CHANGE_OBJECT = 0x6b,
	GAME_RECEIVE_DELETE_OBJECT = 0x6c,
	GAME_RECEIVE_MOVE_CREATURE = 0x6d,
	GAME_RECEIVE_OPEN_CONTAINER = 0x6e,
	GAME_RECEIVE_CLOSE_CONTAINER = 0x6f,
	GAME_RECEIVE_CONTAINER_ADD_OBJECT = 0x70,
	GAME_RECEIVE_CONTAINER_CHANGE_OBJECT = 0x71,
	GAME_RECEIVE_CONTAINER_DELETE_OBJECT = 0x72,
	GAME_RECEIVE_INVENTORY_OBJECT = 0x78,
	GAME_RECEIVE_DELETE_INVENTORY_OBJECT = 0x79,
	GAME_RECEIVE_OWN_TRADE = 0x7d,
	GAME_RECEIVE_COUNTER_TRADE = 0x7e,
	GAME_RECEIVE_CLOSE_TRADE = 0x7f,
	GAME_RECEIVE_WORLD_LIGHT = 0x82,
	GAME_RECEIVE_MAGIC_EFFECT = 0x83,
	GAME_RECEIVE_ANIMATED_TEXT = 0x84,
	GAME_RECEIVE_DISTANCE_EFFECT = 0x85,
	GAME_RECEIVE_CREATURE_MARK = 0x86,
	GAME_RECEIVE_CREATURE_HEALTH = 0x8c,
	GAME_RECEIVE_CREATURE_LIGHT = 0x8d,
	GAME_RECEIVE_CREATURE_OUTFIT = 0x8e,
	GAME_RECEIVE_CREATURE_SPEED = 0x8f,
	GAME_RECEIVE_CREATURE_SKULL = 0x90,
	GAME_RECEIVE_CREATURE_SHIELD = 0x91,
	GAME_RECEIVE_EDIT_TEXT = 0x96,
	GAME_RECEIVE_EDIT_HOUSE_TEXT = 0x97,
	GAME_RECEIVE_PLAYER_STATS = 0xa0,
	GAME_RECEIVE_PLAYER_SKILLS = 0xa1,
	GAME_RECEIVE_PLAYER_ICONS = 0xa2,
	GAME_RECEIVE_CANCEL_ATTACK = 0xa3,
	GAME_RECEIVE_TALK = 0xaa,
	GAME_RECEIVE_CHANNEL_LIST = 0xab,
	GAME_RECEIVE_OPEN_CHANNEL = 0xac,
	GAME_RECEIVE_OPEN_PRIVATE_CONVERSATION = 0xad,
	GAME_RECEIVE_RULE_VIOLATION_CHANNEL = 0xae,
	GAME_RECEIVE_REMOVE_RULE_VIOLATION = 0xaf,
	GAME_RECEIVE_CANCEL_RULE_VIOLATION = 0xb0,
	GAME_RECEIVE_LOCK_RULE_VIOLATION = 0xb1,
	GAME_RECEIVE_OWN_CHANNEL = 0xb2,
	GAME_RECEIVE_CLOSE_CHANNEL = 0xb3,
	GAME_RECEIVE_TEXT_MESSAGE = 0xb4,
	GAME_RECEIVE_CANCEL_WALK = 0xb5,
	GAME_RECEIVE_FLOOR_UP = 0xbe,
	GAME_RECEIVE_FLOOR_DOWN = 0xbf,
	GAME_RECEIVE_OUTFIT = 0xc8,
	GAME_RECEIVE_BUDDY = 0xd2,
	GAME_RECEIVE_BUDDY_LOGIN = 0xd3,
	GAME_RECEIVE_BUDDY_LOGOUT = 0xd4,
};

enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	// The client's version, 7.72, which SendLogin and SendGameLogin send to their servers (name ours).
	CLIENT_VERSION = 772,
	// ReadField: a word of this value or above ends the objects of a map field, and what it is above tells how many
	// of the fields after it have no objects (SkipFields; name ours).
	FIELD_END = 0xff00,
	// What the game server sends where an object's type belongs when the object is a creature (ReadCreature): a
	// creature the client does not know yet, with all of its data, one it knows, with what may have changed, or
	// Objects.h's CREATURE (0x63), one it knows, with its direction only. The names are the original's, from
	// ReceiveMoveCreature's error text. On the map a creature is always a CREATURE.
	UNKNOWNCREATURE = 0x61,
	OUTDATEDCREATURE = 0x62,
};
