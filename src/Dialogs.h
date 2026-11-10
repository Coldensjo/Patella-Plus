// Dialogs.h: the dialogs of the client.
//
// Every dialog is a TDialog (the frame: caption, separator, the OK, Cancel
// and extra buttons, dragging) around a body window that builds the
// dialog's controls and handles its keys and events. The creators make the
// body and the frame and return the frame as the new modal window
// (Control.cpp's ProcessEvents); a body closes the dialog with
// SetActionState(0), which makes ProcessEvents delete the modal window.

#pragma once

#include "common.h"
#include "types/FixedString.h"
#include "types/IndexedVector.h"
#include "GUI.h"
#include "graphics.h"
#include "DialogElements.h"
#include "Map.h"         // TCoordinate, TMark
#include "Preferences.h" // TFullscreenMode, HOTKEYS
#include <array>
#include <cstddef>
#include <memory>

// cppcoreguidelines-pro-type-vararg is off for the file, for the original's varargs (TMultiStateButton's constructor,
// _snprintf).
// NOLINTBEGIN(cppcoreguidelines-pro-type-vararg)

class TDialog;
struct TTalkLine;

// --- Creators ------------------------------------------------------------------------

// Each makes a dialog and returns it as the new modal window (Control.cpp's
// ProcessEvents); the Move Objects and Report dialogs return NULL when they
// cannot be opened.
TDialog *CreateMessageDialog(const char *Caption, const char *Text);
TDialog *CreateFormattedMessageDialog(const char *Caption, const char *Text);
TDialog *CreateConnectingDialog(const char *Caption, const char *Text, TComponent *Receiver);
TDialog *CreateWaitDialog(const char *Caption, const char *Text, int Timeout);
TDialog *CreateReconnectDialog(const char *Caption, const char *Text, int timeout);
TDialog *CreateQuestionDialog(const char *Caption, const char *Text, TComponent *Receiver, int YesEvent,
	int NoEvent);
TDialog *CreateLogoutDialog(const char *Caption, const char *Text, TComponent *Receiver);
TDialog *CreateInfoDialog(void (*OpenURL)(const char *URL));
TDialog *CreateMoveObjectsDialog();
TDialog *CreateLoginDialog(void (*OpenURL)(const char *URL));
TDialog *CreateCharacterListDialog(int Timeout);
TDialog *CreateTextDialog(const int *ObjectData, const char *Author, const char *Text);
TDialog *CreateEditListDialog(int *HouseData, const char *Text);
TDialog *CreateOutfitDialog(int *Data);
TDialog *CreateBugReportDialog();
TDialog *CreateNewChannelDialog(int NumberOfChannels, int *ChannelIDs, const char (*ChannelNames)[30]); // NOLINT(modernize-avoid-c-arrays): the names as Communication.cpp allocates them, 30 bytes each
TDialog *CreateInviteDialog();
TDialog *CreateExcludeDialog();
TDialog *CreateIgnoreListDialog();
TDialog *CreateReportDialog();
TDialog *CreateAddBuddyDialog();
TDialog *CreateEditBuddyDialog(int CharacterID);
TDialog *CreateHelpDialog(void (*OpenURL)(const char *URL));
TDialog *CreateHintsDialog();
TDialog *CreateRuleViolationDialog(const char *Name, TTalkLine *Statement);
TDialog *CreateEditMarkDialog(TCoordinate *Coordinate, TMark *Mark);
TDialog *CreateOptionsDialog();
TDialog *CreateGeneralOptionsDialog(void (*OpenURL)(const char *URL));
TDialog *CreateGraphicsOptionsDialog(void (*OpenURL)(const char *URL), void (*SetDisplayMode)(int Mode),
	TWindow *Parent);
TDialog *CreateAdvancedGraphicsDialog(void (*OpenURL)(const char *URL), TComponent *Receiver);
TDialog *CreateConsoleOptionsDialog(void (*OpenURL)(const char *URL));
TDialog *CreateNetworkOptionsDialog(void (*OpenURL)(const char *URL));
TDialog *CreateHotkeyDialog();

// --- The frame ------------------------------------------------------------------

// The frame of every dialog: a caption over a separator, the body Dialog
// and up to three buttons in the bottom right corner (Cancel, OK, the extra
// button, from the right). Return sends OkCallback while there is an OK
// button with a callback, Escape sends CancelCallback; every other key goes
// to the body. The caption line drags the dialog inside the game window.
class TDialog : public TWindow {
public:
	TDialog(const char *_Caption, TComponent *_Dialog);
	int GetMinimumWidth() override { return SkinProperties().Dialog.MinimumWidth; }
	int GetMinimumHeight() override { return SkinProperties().Dialog.MinimumHeight; }
	void Arrange() override;
	bool MouseMove(int x, int y) override;
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;
	// A NULL text removes the button. The OK button's callback NoCallback
	// becomes (this, 1) for Return; the buttons themselves keep the callback
	// given.
	void SetOkButton(const char *Text, TCallback Callback);
	void SetCancelButton(const char *Text, TCallback Callback);
	void SetExtraButton(const char *Text, TCallback Callback);

	TComponent *Dialog; // the body (a window or a label)
	TTextButton *OkButton;
	TTextButton *CancelButton;
	TTextButton *ExtraButton;
	TLabel *CaptionLabel;
	TSeparator *Separator;
	bool Dragging;
	int DragX; // where the drag started
	int DragY;
	TFixedString<100> Caption;
	TCallback OkCallback;     // Return
	TCallback CancelCallback; // Escape
};

// --- Question labels -------------------------------------------------------------

// A question whose keys answer it: Return, y and Y send YesCallback,
// Escape, n and N send NoAnswerCallback.
class TQuestionLabel final : public TLabel {
public:
	TQuestionLabel(TFont &_Font, const char *_Text, TCallback _YesCallback, TCallback _NoAnswerCallback)
		: TLabel(_Font, _Text)
	{
		YesCallback = _YesCallback;
		NoAnswerCallback = _NoAnswerCallback;
	}
	void KeyPress(int Key) override;

	TCallback YesCallback;
	TCallback NoAnswerCallback;
};

// The logout question: Return, l and L send LogoutCallback and close the
// dialog, e and E send ExitCallback, Escape, c and C close it.
class TLogoutLabel final : public TLabel {
public:
	TLogoutLabel(TFont &_Font, const char *_Text, TCallback _LogoutCallback, TCallback _ExitCallback)
		: TLabel(_Font, _Text)
	{
		LogoutCallback = _LogoutCallback;
		ExitCallback = _ExitCallback;
	}
	void KeyPress(int Key) override;

	TCallback LogoutCallback;
	TCallback ExitCallback;
};

// A message to wait on: the "Connecting" dialogs. Event 31 is its
// timer, 29 the Abort button; both close the dialog (an Abort first
// clears the message queue when ClearOnAbort is set). With
// CloseOnMessage, the next queued message closes it.
class TWaitLabel final : public TLabel {
public:
	TWaitLabel(const TWaitLabel &) = delete;
	TWaitLabel &operator=(const TWaitLabel &) = delete;
	TWaitLabel(const char *_Text, bool _CloseOnMessage, bool _ClearOnAbort)
		: TLabel(SkinProperties().Dialog.TextFont, _Text)
	{
		ClearOnAbort = _ClearOnAbort;
		Timer = nullptr;
		CloseOnMessage = _CloseOnMessage;
	}
	~TWaitLabel() override { Timer.reset(); }
	void Draw(int Surface, int x, int y) override;
	void HandleEvent(int Event) override;
	virtual void SetTimer(int Time, TCallback Callback);

	std::unique_ptr<TTimer> Timer;
	bool CloseOnMessage;
	bool ClearOnAbort;
};

// --- Connecting -----------------------------------------------------------------

// The reconnect countdown: a text, a progress bar and the time left. When
// the timer runs out it connects again.
class TReconnectWindow final : public TWindow {
public:
	TReconnectWindow(const TReconnectWindow &) = delete;
	TReconnectWindow &operator=(const TReconnectWindow &) = delete;
	TReconnectWindow(const char *Text, int timeout);
	~TReconnectWindow() override { Timer.reset(); }
	void Draw(int Surface, int x, int y) override;
	void HandleEvent(int Event) override;
	// Time in tenths of a second.
	void SetTime(int Time);

	int Timeout;                   // tenths of a second
	TFixedString<512> Text;        // the time left
	std::unique_ptr<TTimer> Timer; // event 31
	TProgressBar *ProgressBar;
	TLabel *TimeLabel;
};

// --- Login and character list ------------------------------------------------

// "Enter Game": account number and password, both shown as stars, and a
// link to the account creation.
class TLoginDialog final : public TWindow {
public:
	TLoginDialog(const TLoginDialog &) = delete;
	TLoginDialog &operator=(const TLoginDialog &) = delete;
	TLoginDialog(void (*OpenURL)(const char *URL));
	~TLoginDialog() override { FocusGroup.reset(); }
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	TPasswordEditLine *Account;
	TPasswordEditLine *Password;
	std::unique_ptr<TFocusGroup> FocusGroup;
};

// "Select Character": the account's characters and its premium state. With a
// timeout the selection is confirmed when the timer runs out; every key and
// click starts it again.
class TCharacterListDialog final : public TWindow {
public:
	TCharacterListDialog(const TCharacterListDialog &) = delete;
	TCharacterListDialog &operator=(const TCharacterListDialog &) = delete;
	TCharacterListDialog();
	~TCharacterListDialog() override
	{
		Timer.reset();
		Timer = nullptr;
	}
	bool LeftButtonDown(int x, int y, int Flags) override;
	bool RightButtonUp(int x, int y, int Flags) override;
	bool MouseWheel(int x, int y, int Code) override;
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;
	void PositionChanged() override;
	virtual void SetTimeout(int seconds);

	TListBox *ListBox;
	std::unique_ptr<TTimer> Timer; // event 31
	int Seconds;
};

// --- Moving objects -------------------------------------------------------------

// "Move Objects": the number of a dragged pile to move, chosen with a scroll
// bar or typed.
class TMoveObjectsDialog final : public TWindow {
public:
	TMoveObjectsDialog();
	void Draw(int Surface, int x, int y) override;
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	TObject DraggedObject;
	int SourceIndex; // GetDragObject's Index
	int SourceX;
	int SourceY;
	int SourceZ;
	int DestinationX;
	int DestinationY;
	int DestinationZ;
	THorizontalScrollBar *ScrollBar;
	int LastDigit; // the digit typed before
};

// --- Outfit -------------------------------------------------------------------

// "Select Outfit": the outfit in a frame, a Next button, the four parts
// (head, body, legs, feet) as a radio group and the 133 colours as another.
// Data holds the outfit, its four colours and the first and last outfit the
// player may choose.
class TOutfitDialog final : public TWindow {
public:
	TOutfitDialog(const TOutfitDialog &) = delete;
	TOutfitDialog &operator=(const TOutfitDialog &) = delete;
	TOutfitDialog(const int *Data);
	~TOutfitDialog() override
	{
		ColorGroup.reset();
		PartGroup.reset();
	}
	void HandleEvent(int Event) override;

	TOutfitIcon *OutfitIcon;
	std::array<TTextButton *, 4> PartButtons; // events 1000-1003
	std::unique_ptr<TRadioButtonGroup> PartGroup;
	std::array<TColorButton *, 133> ColorButtons; // events 2000-2132
	std::unique_ptr<TRadioButtonGroup> ColorGroup;
	std::array<int, 4> Colors;
	int Outfit;
	int FirstOutfit;
	int LastOutfit;
	int Part; // the part whose colour the buttons show
};

// --- Texts --------------------------------------------------------------------

// "Show Text" and "Edit Text": the text of a readable or writable object,
// with its icon and a line about who wrote it. The text can be edited when
// the object is writable, or writable once and still empty.
class TTextDialog final : public TWindow {
public:
	TTextDialog(int MaximumLength, const char *Text, uint32 _ObjectNumber, TObject _Obj,
		const char *Description);
	void KeyPress(int Key) override { EditText->KeyPress(Key); }
	void HandleEvent(int Event) override;

	uint32 ObjectNumber; // the server's number for SendEditText
	TObject Obj;
	TObjectIcon *ObjectIcon;
	TEditText *EditText;
};

// "Edit List": a house list, one name per line.
class TEditListDialog final : public TWindow {
public:
	TEditListDialog(int MaxLength, const char *Text, int _Type, uint32 _Identifier,
		const char *Description);
	void Arrange() override;
	void KeyPress(int Key) override { EditText->KeyPress(Key); }
	void HandleEvent(int Event) override;

	int Type; // for SendEditHouseText
	uint32 Identifier;
	TEditText *EditText;
};

// --- Ignore list --------------------------------------------------------------

// A name of the ignore or white list with its permanent flag (31 bytes).
struct TIgnoreEntry {
	// Inline: the vector's constructor and operator() clear every new entry
	// (and init) this way.
	TIgnoreEntry()
	{
		Name[0] = 0;
		Permanent = false;
	}
	// strcpy(Name, Text) as the original wrote it: the text and its NUL within the entry's 31 bytes. A name of 30
	// characters, the most the lists' edit line takes, is kept whole, and its NUL lands in Permanent, which every
	// caller sets next (Name.Assign would keep 29 characters).
	void AssignName(const char *Text) noexcept
	{
		TStringBuffer(reinterpret_cast<char *>(this), sizeof(TIgnoreEntry)).Assign(Text);
	}
	// The name as a C string, at most its 30 characters: one of 30 has no NUL of its own (its 31st byte is Permanent).
	// Tibia.exe read on past it, so with Permanent set the rebuilt list's line had the byte 1 and the next entry's name
	// after it.
	[[nodiscard]] TFixedString<31> NameText() const noexcept
	{
		TFixedString<31> Text;
		Text.Assign(Name.View());
		return Text;
	}

	TFixedString<30> Name;
	bool Permanent;
};
// AssignName writes through the entry's bytes: the name first, Permanent right after it, and nothing else.
static_assert(sizeof(TIgnoreEntry) == 0x1f && offsetof(TIgnoreEntry, Name) == 0 && offsetof(TIgnoreEntry, Permanent) == 0x1e,
	"TIgnoreEntry's layout");

// "Edit Ignore List and White List": both lists as editable font list boxes
// (permanent entries in their own colour), the Add and Delete buttons and the
// global settings. The entries are kept in two vectors next to the lists; OK
// (602) writes everything back to Talk.cpp.
class TIgnoreListDialog final : public TWindow {
public:
	TIgnoreListDialog();
	void Arrange() override;
	bool LeftButtonUp(int x, int y, int Flags) override;
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	TCheckBox *IgnoreListCheckBox;   // "Activate Ignore List"
	TFontListBox *IgnoreList;        // event 11
	TTextButton *AddIgnoreButton;    // event 709
	TTextButton *DeleteIgnoreButton; // event 710
	TCheckBox *IgnorePermanently;
	TCheckBox *IgnoreYelling;
	TCheckBox *IgnorePrivate;
	TCheckBox *WhiteListCheckBox;   // "Activate White List"
	TFontListBox *WhiteList;        // event 11
	TTextButton *AddWhiteButton;    // event 712
	TTextButton *DeleteWhiteButton; // event 713
	TCheckBox *AllowPermanently;
	TCheckBox *AllowBuddies; // "Allow VIPs to message you"
	TIndexedVector<TIgnoreEntry> IgnoreEntries;
	TIndexedVector<TIgnoreEntry> WhiteEntries;
};

// The qsort order of the names (an inline function: its address is
// taken).
inline int CompareNames(const void *Name1, const void *Name2)
{
	return MsvcStricmp(*static_cast<const char *const *>(Name1), *static_cast<const char *const *>(Name2));
}

// --- Bug report and channels ----------------------------------------------------

// "Reporting": a bug report.
class TBugReportDialog final : public TWindow {
public:
	TBugReportDialog();
	void KeyPress(int Key) override { EditText->KeyPress(Key); }
	void HandleEvent(int Event) override;

	TEditText *EditText;
};

// The channel ID of the offer that opens an own private channel, and the premium days of a gratis premium account.
// Enumerators for the reason in GUI.h.
enum : int { // NOLINT(cppcoreguidelines-use-enum-class): int constants
	CHANNEL_NEW_PRIVATE = 0xffff,
	PREMIUM_DAYS_GRATIS = 0xffff,
};

// "New Channel": the channels the server offers (ID 0xffff opens an own
// private channel) and a name for a private conversation.
class TNewChannelDialog final : public TWindow {
public:
	TNewChannelDialog(const TNewChannelDialog &) = delete;
	TNewChannelDialog &operator=(const TNewChannelDialog &) = delete;
	TNewChannelDialog(int _NumberOfChannels, const int *_ChannelIDs, const char (*ChannelNames)[30]); // NOLINT(modernize-avoid-c-arrays): as CreateNewChannelDialog
	~TNewChannelDialog() override { ChannelIDs.reset(); }
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	TListBox *ListBox; // double click 301
	TEditLine *NameEdit;
	int NumberOfChannels;
	std::unique_ptr<int[]> ChannelIDs;
};

// "Invite player to private chat channel" and "Exclude player from private
// chat channel".
class TPrivateChannelDialog final : public TWindow {
public:
	TPrivateChannelDialog(bool _Exclude);
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	bool Exclude;
	TEditLine *NameEdit;
};

// "Report Rule Violation": a player's request to the gamemasters, one
// sentence.
class TReportDialog final : public TWindow {
public:
	TReportDialog();
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	TEditLine *TextEdit;
};

// --- VIP list -----------------------------------------------------------------

// "Add to VIP list".
class TAddBuddyDialog final : public TWindow {
public:
	TAddBuddyDialog();
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	TEditLine *NameEdit;
};

// "Change player in VIP list": the icon, the description and the login
// notification of a VIP.
class TEditBuddyDialog final : public TWindow {
public:
	TEditBuddyDialog(const TEditBuddyDialog &) = delete;
	TEditBuddyDialog &operator=(const TEditBuddyDialog &) = delete;
	TEditBuddyDialog(int _CharacterID);
	~TEditBuddyDialog() override { IconGroup.reset(); }
	void KeyPress(int Key) override { Description->KeyPress(Key); }
	void HandleEvent(int Event) override;

	int CharacterID;
	std::unique_ptr<TRadioButtonGroup> IconGroup;
	TCheckBox *Notify;
	TEditText *Description;
	std::array<TIconButton *, 11> IconButtons;
};

// --- Help and hints -------------------------------------------------------------

// "Help": six buttons with their texts.
class THelpDialog final : public TWindow {
public:
	THelpDialog(void (*_OpenURL)(const char *URL));
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	void (*OpenURL)(const char *URL);
};

// "Hints for Tibia": the hints shown so far.
class THintsDialog final : public TWindow {
public:
	THintsDialog();
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	TListBox *ListBox; // double click 1
};

// --- Rule violations (gamemasters) ----------------------------------------------

// "Rule Violation": a gamemaster's report of a player, optionally about a
// statement: the reasons are the rights the player has (Reasons maps the
// list's lines to them), the actions those the selected reason allows.
class TRuleViolationDialog final : public TWindow {
public:
	TRuleViolationDialog(const TRuleViolationDialog &) = delete;
	TRuleViolationDialog &operator=(const TRuleViolationDialog &) = delete;
	TRuleViolationDialog(const char *Name, TTalkLine *Statement);
	~TRuleViolationDialog() override { FocusGroup.reset(); }
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;
	void UpdateActions();
	int GetReason();
	[[nodiscard]] int GetAction() const;

	std::unique_ptr<TFocusGroup> FocusGroup;
	TEditLine *NameEdit;
	TListBox *ReasonList; // event 21
	TListBox *ActionList;
	TEditLine *CommentEdit;
	TEditLine *StatementEdit;
	TCheckBox *IPBanishment;
	uint32 StatementID;
	std::array<int, 50> Reasons; // the right of each reason line, -1 after the last
};

// --- Automap marks ---------------------------------------------------------------

// "Edit Mark": the text and the symbol of an automap mark. It takes the
// coordinate and the mark (if any) and deletes them.
class TEditMarkDialog final : public TWindow {
public:
	TEditMarkDialog(const TEditMarkDialog &) = delete;
	TEditMarkDialog &operator=(const TEditMarkDialog &) = delete;
	TEditMarkDialog(TCoordinate *_Coordinate, TMark *_Mark);
	~TEditMarkDialog() override { TypeGroup.reset(); }
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	TCoordinate Coordinate;
	TMark Mark;
	TEditLine *TextEdit;
	std::unique_ptr<TRadioButtonGroup> TypeGroup;
	std::array<TIconButton *, 14> TypeButtons;
};

// --- Options -----------------------------------------------------------------------

// "Options": buttons that open the option dialogs (each reopens this one
// when it closes) and the message of the day.
class TOptionsDialog final : public TWindow {
public:
	TOptionsDialog();
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;
};

// "General Options".
class TGeneralOptionsDialog final : public TWindow {
public:
	TGeneralOptionsDialog(void (*_OpenURL)(const char *URL));
	void HandleEvent(int Event) override;

	TCheckBox *TextualEffects;
	TCheckBox *CreatureNames;
	TCheckBox *ShowHints;
	TCheckBox *ClassicControl;
	TCheckBox *AutoChaseOff;
	void (*OpenURL)(const char *URL); // the Help button (701)
	TCheckBox *DragFullStacks;        // Patella-Plus, not in the original
};

// "Graphics Options": fullscreen and its resolution; Advanced (407)
// opens the advanced options.
class TGraphicsOptionsDialog final : public TWindow {
public:
	TGraphicsOptionsDialog(const TGraphicsOptionsDialog &) = delete;
	TGraphicsOptionsDialog &operator=(const TGraphicsOptionsDialog &) = delete;
	TGraphicsOptionsDialog(void (*_OpenURL)(const char *URL), void (*_SetDisplayMode)(int Mode),
		TWindow *_Parent);
	~TGraphicsOptionsDialog() override { Modes.reset(); }
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;

	TCheckBox *Fullscreen;
	TListBox *ResolutionList;
	int NumberOfModes;
	std::unique_ptr<TFullscreenMode[]> Modes; // the surface's list
	void (*SetDisplayMode)(int Mode);         // 1 windowed, 2 fullscreen
	TWindow *Parent;                          // passed on to the advanced options
	TTextButton *AdvancedButton;
	void (*OpenURL)(const char *URL);
};

// "Advanced Graphics Options": light, stretching, the mouse pointer, the
// frame rate limit and the graphics device. The device buttons send 800-802
// to Receiver (the main window).
class TAdvancedGraphicsDialog final : public TWindow {
public:
	TAdvancedGraphicsDialog(const TAdvancedGraphicsDialog &) = delete;
	TAdvancedGraphicsDialog &operator=(const TAdvancedGraphicsDialog &) = delete;
	TAdvancedGraphicsDialog(void (*_OpenURL)(const char *URL), TComponent *_Receiver);
	~TAdvancedGraphicsDialog() override { DeviceGroup.reset(); }
	void Draw(int Surface, int x, int y) override;
	void HandleEvent(int Event) override;

	TComponent *Receiver;
	TCheckBox *LightEffects;
	TCheckBox *NoStretching;
	TCheckBox *Antialiasing; // Patella-Plus, not in the original
	int RowPitch;            // Patella-Plus: how far the Anti-aliasing row moves the frame rate's bar and texts down
	TCheckBox *WindowsMousepointer;
	THorizontalScrollBar *Framerate; // 10-201 (201 is no limit), event 30
	void (*OpenURL)(const char *URL);
	TTextButton *AutomaticButton;
	TTextButton *GpuButton;
	TTextButton *CpuButton;
	std::unique_ptr<TRadioButtonGroup> DeviceGroup;
	// The three buttons choose the graphics device ("Automatic", "GPU", "CPU", in the places of the original's
	// DirectX 5, DirectX 9 and OpenGL buttons), and the line above them names the device in use.
	void ShowDevice();                 // the selected button and the device's name
	TLabel *DeviceLabel;               // the line above the buttons (the original's "Select graphics engine:")
	TFixedString<128> DeviceLabelName; // the device the line names
};

// "Console Options".
class TConsoleOptionsDialog final : public TWindow {
public:
	TConsoleOptionsDialog(void (*_OpenURL)(const char *URL));
	void HandleEvent(int Event) override;

	TCheckBox *InfoMessages;
	TCheckBox *EventMessages;
	TCheckBox *StatusMessages;
	TCheckBox *TimeStamps;
	TCheckBox *PrivateMessages;
	void (*OpenURL)(const char *URL);
};

// "Network Options".
class TNetworkOptionsDialog final : public TWindow {
public:
	TNetworkOptionsDialog(void (*_OpenURL)(const char *URL));
	void HandleEvent(int Event) override;

	TCheckBox *TestServerLogin;
	void (*OpenURL)(const char *URL);
};

// "Hotkey Options": the texts of the 36 hotkeys (F1-F12 with Shift and
// Control), edited one at a time.
class THotkeyDialog final : public TWindow {
public:
	THotkeyDialog();
	bool LeftButtonUp(int x, int y, int Flags) override;
	void KeyPress(int Key) override;
	void HandleEvent(int Event) override;
	void UpdateLine(int Number);

	std::array<TFixedString<256>, HOTKEYS> Texts;
	std::array<bool, HOTKEYS> AutoSend;
	TFontListBox *ListBox; // event 11
	TEditLine *TextEdit;
	TCheckBox *AutoSendCheckBox;
	int Selected; // -1 for none
};
// NOLINTEND(cppcoreguidelines-pro-type-vararg)
