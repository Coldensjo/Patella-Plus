// BuildConfigDefaults.h: the values someone fixes when building the client for their own server. BuildConfig.h includes
// this file and checks every value at compile time; include BuildConfig.h, not this.
//
// To build for your own server, copy this file somewhere outside the repository, change the values in the copy and
// configure with -DPATELLA_BUILD_CONFIG=<your copy>: your file replaces this one, so pulling updates never conflicts with
// your values, and it goes through the same checks. The login servers and the test server are 127.0.0.1, port 7171
// (Patella-Plus); the other values are the original's, byte for byte. README.md, "Building for your own server".
//
// Every value must be ASCII. A bad value fails the compile with a message that says what is wrong. Nothing here is read
// from Patella.ini or the environment, but Patella.ini's ServerAddress, ServerPort and RsaModulus replace those values
// unless the overrides below are set. The values end up as plain text in the program: they are not a secret.
//
// The texts players read (the ones naming the game, the hints, the help) are in BuildTextsDefaults.h, the same way.

#pragma once

#include "BuildConfigTypes.h"

#include <array>
#include <optional>

namespace patella::buildconfig {

// The login servers: always four entries (if you have fewer, repeat one), each at most 99 characters without spaces.
inline constexpr std::array<const char *, 4> LoginServers = {"127.0.0.1", "127.0.0.1", "127.0.0.1", "127.0.0.1"};

// Their port, 1 to 65535.
inline constexpr int LoginPort = 7171;

// The RSA modulus of the login, in decimal digits: the public half of your server's 1024-bit key (306 to 309 digits).
inline constexpr const char *RsaModulus = "124710459426827943004376449897985582167801707960697037164044904862948569380850421396904597686953877022394604239428185498284169068581802277612081027966724336319448537811441719076484340922854929273517308661370727105382899118999403808045846444647284499123164879035103627004668521005328367415259939915284902061793";

// The test server (Network Options' "Test Server Login"): its name and port.
inline constexpr const char *TestServerHost = "127.0.0.1";
inline constexpr int TestServerPort = 7171;

// Patella-Plus: the one server every login goes to, the test server's included, whatever Patella.ini's ServerAddress and
// ServerPort say. An empty address or a port of 0 leaves that value to Patella.ini, and without its key to the values above.
inline constexpr const char *ServerAddressOverride = "";
inline constexpr int ServerPortOverride = 0;

// Patella-Plus: the RSA modulus every login uses, whatever Patella.ini's RsaModulus says. Empty leaves it to Patella.ini,
// and without its key to RsaModulus above.
inline constexpr const char *RsaModulusOverride = "";

// Where the buttons that open a web page go. Each must start with http:// or https:// (ASCII case-insensitive), have
// something after that, and hold no space or control character: a program path or a file: address would be started by a
// button (SDL_OpenURL is ShellExecute's default verb on Windows).
inline constexpr const char *LinkWebsite = "http://www.tibia.com/";                                           // Info's button
inline constexpr const char *LinkManual = "http://www.tibia.com/guide/?subtopic=manual";                      // Help
inline constexpr const char *LinkFAQ = "http://www.tibia.com/guide/?subtopic=faq";                            // Help
inline constexpr const char *LinkOptionsHelp = "http://www.tibia.com/guide/?subtopic=manual&section=options"; // General Options
// Graphics Options and Advanced Graphics
inline constexpr const char *LinkGraphicsHelp = "http://www.tibia.com/guide/?subtopic=manual&section=options#graphicsoptions";
inline constexpr const char *LinkConsoleHelp = "http://www.tibia.com/guide/?subtopic=manual&section=options#consoleoptions"; // Console Options
inline constexpr const char *LinkNetworkHelp = "http://www.tibia.com/guide/?subtopic=manual&section=options#networkoptions"; // Network Options
inline constexpr const char *LinkAccount = "http://www.tibia.com/home/?subtopic=account";                                    // the title screen's Access Account
inline constexpr const char *LinkSignup = "http://www.tibia.com/home/?subtopic=signup";                                      // Login's Create Account

// The caption of Info's website button: at most 49 characters. The button is 86 pixels wide and its text is not clipped,
// so keep it to about 17 characters or it runs over the text beside it.
inline constexpr const char *WebsiteCaption = "www.tibia.com";

// The site the connection errors and the first hint name, and the address the error text gives for bug reports.
inline constexpr const char *SiteName = "www.tibia.com";
inline constexpr const char *BugReportAddress = "cip@tibia.com";

// --- Names ---------------------------------------------------------------------------------------------------------

// The program's name: the executable (<ClientName>.exe on Windows, <ClientName> on Linux), its version information and
// the name the client gives Vulkan. 1 to 31 letters, digits, '-' or '_'.
// cmake/BuildConfig.cmake reads this line from the file: keep it on one line, in this form.
inline constexpr const char *ClientName = "Patella";

// The window's title, and its class name on Windows. The start counts the windows with either and refuses a second
// client (except with "gamemaster"), so a client of another name runs beside this one. 1 to 29 printable ASCII
// characters each (the original's "Tibia" has three spaces after it).
inline constexpr const char *WindowTitle = "Patella";
inline constexpr const char *WindowClassName = "PatellaClient";

// The settings file, and on Linux the folder in ~/.config (or $XDG_CONFIG_HOME) that holds it when the data folder
// cannot, and the start's lock file on Linux ($XDG_RUNTIME_DIR or /tmp, <SettingsFolderName>-client-<uid>.lock), which
// keeps a second client of this name from starting there.
inline constexpr const char *SettingsFileName = "Patella.ini";
inline constexpr const char *SettingsFolderName = "patella";

// The data files the client reads: the objects, the sprites and the pictures. Their contents' formats stay 7.72's.
inline constexpr const char *DataFileName = "Tibia.dat";
inline constexpr const char *SpriteFileName = "Tibia.spr";
inline constexpr const char *PictureFileName = "Tibia.pic";

// The original's options file, read once when the settings file has no options (it is never written). Empty: none is read.
inline constexpr const char *ImportFileName = "Tibia.cfg";

// The file names above: 1 to 63 letters, digits, '.', '-' or '_', not starting with '.'.

// --- Options -------------------------------------------------------------------------------------------------------
//
// Each option of Patella.ini is either optional (the player sets it in the dialog, the menu or the button named
// beside it, starting from the value in brackets) or a value you force: the client then always uses your value,
// whatever Patella.ini says (it is written back there), and the option's check box or menu entry is gone, the rows
// below it moving up. Lights that are always on and cannot be turned off, for example:
//	inline constexpr std::optional<bool> OptionLightEffects = true;
// The value is enforced where the client reads it, not only in its dialog. It is plain data in the program and on the
// player's computer: anything that would give a player an advantage must be checked by the server as well.

// General Options.
inline constexpr std::optional<bool> OptionClassicControl = optional; // (false)
inline constexpr std::optional<bool> OptionAutoChaseOff = optional;   // (true)
inline constexpr std::optional<bool> OptionShowNewHints = optional;   // (true) "Show Hints"
inline constexpr std::optional<int> OptionCreatureInfo = optional;    // (2) "Show Names of Creatures": 2 yes, 0 no; forced, its key does nothing
inline constexpr std::optional<bool> OptionTextualEffects = optional; // (true)
inline constexpr std::optional<bool> OptionDragFullStacks = optional; // (false) Patella-Plus
// Advanced Graphics.
inline constexpr std::optional<bool> OptionLightEffects = optional;        // (true) off: the original's dithered darkness
inline constexpr std::optional<int> OptionNoStretching = optional;         // (chosen at the first start) 1 don't stretch the map, 0 stretch it
inline constexpr std::optional<bool> OptionWindowsMousepointer = optional; // (true)
inline constexpr std::optional<bool> OptionAntialiasing = optional;        // (false) Patella-Plus
// Console Options.
inline constexpr std::optional<bool> OptionInfoMessages = optional;    // (false)
inline constexpr std::optional<bool> OptionEventMessages = optional;   // (true)
inline constexpr std::optional<bool> OptionStatusMessages = optional;  // (false)
inline constexpr std::optional<bool> OptionTimeStamps = optional;      // (false)
inline constexpr std::optional<bool> OptionPrivateMessages = optional; // (true)
// Network Options.
inline constexpr std::optional<bool> OptionTestServerLogin = optional; // (false)
// The login dialog's "Remember login" (Patella-Plus). Forced false also forgets a saved account and password.
inline constexpr std::optional<bool> OptionRememberLogin = optional; // (false)
// The console's WASD check box (Patella-Plus). Forced true: Enter still opens the line for one message. Forced false:
// Enter never switches to walking.
inline constexpr std::optional<bool> OptionWasdWalking = optional; // (false)
// The combat controls: 1 offensive, 2 balanced, 3 defensive; 0 stand, 1 chase; the secure mode. Their buttons stay (the
// window has no room without them), and a click on a forced one changes nothing.
inline constexpr std::optional<int> OptionAttackMode = optional;  // (2)
inline constexpr std::optional<int> OptionChaseMode = optional;   // (0)
inline constexpr std::optional<bool> OptionSecureMode = optional; // (true)
// The skills window's menu: the level's and the magic level's bars, and the skills' (fist, club, sword, axe, distance,
// shielding, fishing), for example std::array{true, true, true, true, true, true, false}.
inline constexpr std::optional<bool> OptionLevelBar = optional;                 // (true)
inline constexpr std::optional<bool> OptionMagLevelBar = optional;              // (true)
inline constexpr std::optional<std::array<bool, 7>> OptionSkillBars = optional; // (all true)
// The VIP list's menu: "Hide offline VIPs".
inline constexpr std::optional<bool> OptionBuddyHideOffline = optional; // (false)
// Only in Patella.ini, no control (Patella-Plus): WalkFrameTime, the shortest time in milliseconds a walking creature
// shows each frame of its walk, 0 to 1000. 0 is the original: a step changes the frame every 8 pixels, however fast, so a
// very fast creature's legs flicker. With a value, a creature whose frames would change sooner changes them every
// WalkFrameTime instead; a slower one looks as before. 60 calms the fastest walkers down a little.
inline constexpr std::optional<int> OptionWalkFrameTime = optional; // (0)

} // namespace patella::buildconfig
