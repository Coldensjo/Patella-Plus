// Events.h: the numbers of the message queue and of the windows' events, named.
//
// MESSAGE_* are the Type of AddMessage and AddMessageFirst (Status.cpp), which Control.cpp's ProcessEvents handles.
// MESSAGE_NONE and MESSAGE_STARTPATCHER, the original's names, are in Status.h. Type 1, a message box, stays a number.
//
// EVENT_* are the Event of TComponent::HandleEvent and of a TCallback, which hands it to its receiver. Each receiver
// class has its own numbers, so one value can have several names here. Event 1 (a dialog's OK, a link's click, a
// trade's accept) stays a number. The windows' comments give the same numbers in decimal or hexadecimal.
//
// CONNECTION_* are the State of SetConnectionState and GetConnectionState (Status.cpp, and Network.h's
// TNetworkSession), at the end of this file: Network.cpp sees this header and not Status.h.
//
// Enumerators for the reason GUI.h gives at the modifier flags. No names of the original are known but the two above;
// each of these is taken from what the code that receives the number does.

#pragma once

enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	MESSAGE_FORMATTED_DIALOG = 0x02,
	MESSAGE_CONNECTING_LOGIN_DIALOG = 0x03,
	MESSAGE_CONNECTING_GAME_DIALOG = 0x04,
	MESSAGE_RECONNECT_DIALOG = 0x05,
	MESSAGE_QUESTION_DIALOG = 0x06,
	MESSAGE_LOGOUT_DIALOG = 0x07,
	MESSAGE_LOGIN_DIALOG = 0x08,
	MESSAGE_WAIT_DIALOG = 0x09,
	MESSAGE_CHARACTER_LIST_DIALOG = 0x0a,
	MESSAGE_LINK_COPY_WARNING = 0x0b,
	MESSAGE_ACCOUNT_DATA_WARNING = 0x0c,
	MESSAGE_MOVE_OBJECTS_DIALOG = 0x0d,
	MESSAGE_TEXT_DIALOG = 0x0e,
	MESSAGE_EDIT_LIST_DIALOG = 0x0f,
	MESSAGE_OUTFIT_DIALOG = 0x10,
	MESSAGE_BUG_REPORT_DIALOG = 0x11,
	MESSAGE_NEW_CHANNEL_DIALOG = 0x12,
	MESSAGE_INVITE_DIALOG = 0x13,
	MESSAGE_EXCLUDE_DIALOG = 0x14,
	MESSAGE_IGNORE_LIST_DIALOG = 0x15,
	MESSAGE_REPORT_DIALOG = 0x16,
	MESSAGE_ADD_BUDDY_DIALOG = 0x17,
	MESSAGE_EDIT_BUDDY_DIALOG = 0x18,
	MESSAGE_TEMPORARY_IGNORE = 0x19,
	MESSAGE_TEMPORARY_UNIGNORE = 0x1a,
	MESSAGE_INFO_DIALOG = 0x1b,
	MESSAGE_RULE_VIOLATION_DIALOG = 0x1c,
	MESSAGE_SET_MARK_DIALOG = 0x1d,
	MESSAGE_EDIT_MARK_DIALOG = 0x1e,
	MESSAGE_HELP_DIALOG = 0x1f,
	MESSAGE_HINTS_DIALOG = 0x20,
	MESSAGE_OPTIONS_DIALOG = 0x21,
	MESSAGE_NETWORK_OPTIONS_DIALOG = 0x22,
	MESSAGE_GRAPHICS_OPTIONS_DIALOG = 0x23,
	MESSAGE_ADVANCED_GRAPHICS_DIALOG = 0x24,
	MESSAGE_CONSOLE_OPTIONS_DIALOG = 0x25,
	MESSAGE_HOTKEY_DIALOG = 0x26,
	MESSAGE_GENERAL_OPTIONS_DIALOG = 0x27,
	MESSAGE_MESSAGE_OF_THE_DAY = 0x28,
	// Not windows: they start an action state or change the console.
	MESSAGE_USE_WITH = 0x2c,   // ACTIONSTATE_USE_WITH
	MESSAGE_TRADE_WITH = 0x2d, // ACTIONSTATE_TRADE_WITH
	MESSAGE_HELP_MODE = 0x2e,  // ACTIONSTATE_HELP
	MESSAGE_OPEN_PRIVATE_CONVERSATION = 0x2f,
	MESSAGE_OPEN_CHANNEL = 0x30,
	MESSAGE_OPEN_OWN_CHANNEL = 0x31, // and selects it
	MESSAGE_RULE_VIOLATION_CHANNEL = 0x32,
	MESSAGE_REPORT_RULE_VIOLATION = 0x33,
	// The context menus, opened at the mouse. Lower message numbers that open a window centre it.
	MESSAGE_OBJECT_MENU = 0x34,
	MESSAGE_FIRST_MENU = MESSAGE_OBJECT_MENU,
	MESSAGE_CREATURE_MENU = 0x35,
	MESSAGE_BUDDY_MENU = 0x36,
	MESSAGE_TEXT_MENU = 0x37,
	MESSAGE_CHANNEL_MENU = 0x38,
	MESSAGE_REQUEST_MENU = 0x39,
	MESSAGE_EDIT_MENU = 0x3a,
	MESSAGE_MARK_MENU = 0x3b,
	MESSAGE_SKILL_MENU = 0x3c,
	MESSAGE_CONNECT_LOGIN_SERVER = 0x3d,
	MESSAGE_CONNECT_GAME_SERVER = 0x3e,
	MESSAGE_RECONNECT = 0x3f, // now, or after the connection closes
};

enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	// A dialog's Cancel button, Escape and the No of a question.
	EVENT_CANCEL = 2,
	// TScrollBar's and THorizontalScrollBar's buttons.
	EVENT_SCROLL_UP = 3,
	EVENT_SCROLL_DOWN = 4,
	EVENT_SCROLL_LEFT = 5,
	EVENT_SCROLL_RIGHT = 6,
	// The clipboard, of TEditLine and TTextWindow.
	EVENT_COPY = 16,
	EVENT_CUT = 17,
	EVENT_PASTE = 18,
	EVENT_DELETE = 19,
	EVENT_SELECT_ALL = 20,
	// A list box's selection changed (the ignore list and the hotkey dialogs).
	EVENT_LIST_SELECTED = 11,
	EVENT_ABORT = 29, // the Abort of a waiting dialog; TMainWindow disconnects
	EVENT_TIMER = 31, // the tick of a dialog's timer
	EVENT_HELP = 701, // a Help button: the main window's opens the help dialog, a dialog's the page of its subject
	EVENT_OPTIONS_HOTKEYS = 403, // TOptionsDialog's button and TConsoleWindow's Ctrl+K

	// TTabBar.
	EVENT_TABS_MOVE_RIGHT = 5,
	EVENT_TABS_MOVE_LEFT = 6,
	EVENT_TABS_MOVE_ALL_RIGHT = 25,
	EVENT_TABS_MOVE_ALL_LEFT = 26,
	// TConsoleWindow.
	EVENT_SHOW_CHANNEL = 201,
	EVENT_CLOSE_CHANNEL = 202,
	EVENT_SERVER_MESSAGES = 203, // the button's state decides
	EVENT_IGNORE_LIST = 204,
	EVENT_REQUEST_CHANNELS = 205,
	EVENT_VOLUME_SAY = 206,
	EVENT_VOLUME_WHISPER = 207,
	EVENT_VOLUME_YELL = 208,
	EVENT_TOGGLE_SERVER_MESSAGES = 210,
	EVENT_DEFAULT_CHANNEL = 211,
	EVENT_NEXT_CHANNEL = 212,
	EVENT_PREVIOUS_CHANNEL = 213,

	// TAutomapWindow.
	EVENT_AUTOMAP_FLOOR_UP = 3,
	EVENT_AUTOMAP_FLOOR_DOWN = 4,
	EVENT_AUTOMAP_CENTRE = 106,
	EVENT_AUTOMAP_ZOOM_IN = 107,
	EVENT_AUTOMAP_ZOOM_OUT = 108,
	EVENT_AUTOMAP_NORTH = 109,
	EVENT_AUTOMAP_EAST = 110,
	EVENT_AUTOMAP_SOUTH = 111,
	EVENT_AUTOMAP_WEST = 112,
	// TCombatControls.
	EVENT_ATTACK_OFFENSIVE = 100,
	EVENT_ATTACK_BALANCED = 101,
	EVENT_ATTACK_DEFENSIVE = 102,
	EVENT_CHASE_STAND = 103,
	EVENT_CHASE_ON = 104,
	EVENT_SECURE_MODE = 105,
	// TContainerWindowBody, TTradeWindowBody and TSubWindow.
	EVENT_CONTAINER_UP = 3,
	EVENT_TRADE_COUNTER_OFFER = 21,
	EVENT_WINDOW_MINIMISE = 9, // minimises or restores it
	EVENT_WINDOW_CLOSE = 10,
	// TMapWindow.
	EVENT_MAP_HINT = 27,
	EVENT_CREATURE_INFO = 600,
	EVENT_REMOVE_TALKBOX = 601,

	// TMainWindow.
	EVENT_EXIT_PROGRAM = 7,
	EVENT_OPTIONS = 8,
	EVENT_REARRANGE = 9,
	EVENT_TOGGLE_SKILLS = 14,
	EVENT_TOGGLE_BATTLE = 15,
	EVENT_TOGGLE_VIP = 22,
	EVENT_MINIMISE_INVENTORY = 23,
	EVENT_MAXIMISE_INVENTORY = 24,
	EVENT_STOP = 32,
	EVENT_COPY_LINK = 33, // the answer Yes to the link copy warning
	EVENT_TOGGLE_FULLSCREEN = 504,
	EVENT_BUG_REPORT = 505,
	EVENT_LOGOUT = 506,
	EVENT_LEAVE_QUESTION = 507,
	EVENT_CHARACTER_LIST = 509,
	EVENT_HELP_MODE = 702,
	EVENT_HINTS = 703,
	EVENT_TUTOR_HELP = 704,
	EVENT_REPORT = 705,
	EVENT_MANUAL = 706,
	EVENT_FAQ = 707,
	EVENT_RULE_VIOLATION = 708,
	EVENT_DEVICE_AUTOMATIC = 800,
	EVENT_DEVICE_GPU = 801,
	EVENT_DEVICE_CPU = 802,
	// TStartWindow.
	EVENT_ENTER_GAME = 501,
	EVENT_INFO = 502,
	EVENT_START_OPTIONS = 503,
	EVENT_ACCOUNT_URL = 508,

	// TOptionsDialog and TGraphicsOptionsDialog.
	EVENT_OPTIONS_NETWORK = 400,
	EVENT_OPTIONS_GRAPHICS = 401,
	EVENT_OPTIONS_CONSOLE = 402,
	EVENT_OPTIONS_GENERAL = 404,
	EVENT_OPTIONS_MOTD = 406,
	EVENT_OPTIONS_ADVANCED = 407,
	// TOutfitDialog: a part is the first plus 0-3, a colour the first plus 0-215.
	EVENT_OUTFIT_NEXT = 6,
	EVENT_OUTFIT_PART = 1000,
	EVENT_OUTFIT_PART_LAST = 1003,
	EVENT_OUTFIT_COLOR = 2000,
	EVENT_OUTFIT_COLOR_LAST = 2215,
	// TIgnoreListDialog.
	EVENT_IGNORE_LIST_OK = 602,
	EVENT_IGNORE_ADD = 709,
	EVENT_IGNORE_DELETE = 710,
	EVENT_WHITELIST_ADD = 712,
	EVENT_WHITELIST_DELETE = 713,
	// TNewChannelDialog, TRuleViolationDialog and TAdvancedGraphicsDialog.
	EVENT_NEW_CHANNEL_JOIN = 301,
	EVENT_NEW_CHANNEL_CLOSE = 302,
	EVENT_REASON_SELECTED = 21,
	EVENT_FRAMERATE = 30,

	// The context menus. EVENT_MENU_PRIVATE_MESSAGE and EVENT_MENU_SERVER_MESSAGES share 10100, in different menus.
	EVENT_MENU_LOOK = 10001,
	EVENT_MENU_USE = 10002,
	EVENT_MENU_USE_WITH = 10003,
	EVENT_MENU_ATTACK = 10004,
	EVENT_MENU_STOP_ATTACK = 10005,
	EVENT_MENU_FOLLOW = 10006,
	EVENT_MENU_STOP_FOLLOW = 10007,
	EVENT_MENU_SET_OUTFIT = 10008,
	EVENT_MENU_MOVE_UP = 10010,
	EVENT_MENU_OPEN = 10011,            // in the window of the container it is in
	EVENT_MENU_OPEN_NEW_WINDOW = 10012,
	EVENT_MENU_TRADE_WITH = 10013,
	EVENT_MENU_ROTATE = 10014,
	EVENT_MENU_ADD_VIP = 10015,
	EVENT_MENU_INVITE_PRIVATE = 10016,
	EVENT_MENU_EXCLUDE_PRIVATE = 10017,
	EVENT_MENU_INVITE_PARTY = 10018,
	EVENT_MENU_JOIN_PARTY = 10019,
	EVENT_MENU_LEAVE_PARTY = 10020,
	EVENT_MENU_REVOKE_PARTY_INVITE = 10021,
	EVENT_MENU_PASS_PARTY_LEADER = 10022,
	EVENT_MENU_RULE_VIOLATION = 10023,
	EVENT_MENU_PRIVATE_MESSAGE = 10100,
	EVENT_MENU_SERVER_MESSAGES = 10100,
	EVENT_MENU_IGNORE = 10101,
	EVENT_MENU_SAVE_WINDOW = 10102,
	EVENT_MENU_CLOSE_CHANNEL = 10103,
	EVENT_MENU_CLEAR_WINDOW = 10105,
	EVENT_MENU_PROCESS_REPORT = 10106,
	EVENT_MENU_REMOVE_REPORT = 10107,
	EVENT_MENU_PASTE = 10200,
	EVENT_MENU_CUT = 10201,
	EVENT_MENU_DELETE = 10202,
	EVENT_MENU_COPY = 10203, // a selection, or the name in the object and creature menus
	EVENT_MENU_COPY_NAME = 10204,
	EVENT_MENU_COPY_MESSAGE = 10205,
	EVENT_MENU_SELECT_ALL = 10206,
	EVENT_MENU_NEW_VIP = 10300,
	EVENT_MENU_REMOVE_VIP = 10301,
	EVENT_MENU_EDIT_VIP = 10302,
	EVENT_MENU_MESSAGE_VIP = 10303,
	EVENT_MENU_SORT_BY_NAME = 10304,
	EVENT_MENU_SORT_BY_TYPE = 10305,
	EVENT_MENU_SORT_BY_STATUS = 10306,
	EVENT_MENU_HIDE_OFFLINE = 10307,
	EVENT_MENU_SET_MARK = 10400,
	EVENT_MENU_EDIT_MARK = 10401,
	EVENT_MENU_GO_TO_MARK = 10402,
	EVENT_MENU_DELETE_MARK = 10403,
	EVENT_MENU_LEVEL_BAR = 10500,
	EVENT_MENU_MAGIC_LEVEL_BAR = 10501,
	EVENT_MENU_FIST_BAR = 10502, // the first skill bar: Event - EVENT_MENU_FIST_BAR is the skill
	EVENT_MENU_CLUB_BAR = 10503,
	EVENT_MENU_SWORD_BAR = 10504,
	EVENT_MENU_AXE_BAR = 10505,
	EVENT_MENU_DISTANCE_BAR = 10506,
	EVENT_MENU_SHIELDING_BAR = 10507,
	EVENT_MENU_FISHING_BAR = 10508,
	EVENT_MENU_SHOW_ALL_BARS = 10509,
	EVENT_MENU_HIDE_ALL_BARS = 10510,
};

// The connection states, in the order a login goes through them. ConnectToLoginServer (Network.cpp) sets 1 or 2 and
// ConnectToGameServer 5; ConnectionEstablished (Control.cpp) sends the login packet and sets 3 or 6; ReceivePackets
// (Control.cpp) moves 3 to 4 and 6 to 7 at the first packet, reads the login server's packets in state 4 and the game
// server's otherwise, and sets 8 at the game server's packet 0x0a. A failure and a closed connection set 0.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	CONNECTION_OFFLINE = 0,
	CONNECTION_RESOLVING_LOGIN_SERVER = 1,  // the lookup of the login server's name is under way (OnHostResolved)
	CONNECTION_CONNECTING_LOGIN_SERVER = 2, // the socket connects to a login server
	CONNECTION_LOGIN_SENT = 3,              // connected, SendLogin's packet is out and nothing has come back
	CONNECTION_LOGIN_ANSWERED = 4,          // the login server's packets arrive
	CONNECTION_CONNECTING_GAME_SERVER = 5,  // the socket connects to the active character's game server
	CONNECTION_GAME_LOGIN_SENT = 6,         // connected, SendGameLogin's packet is out and nothing has come back
	CONNECTION_GAME_LOGIN_ANSWERED = 7,     // the game server's packets arrive, the character is not in the game yet
	CONNECTION_IN_GAME = 8,                 // from packet 0x0a (EnterGame) until the connection closes
};
