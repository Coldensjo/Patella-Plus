// Dialogs.cpp: the dialogs of the client (c:\mud\tibiaplayer\Dialogs.cpp);
// see Dialogs.h.
//
// Order: the functions follow the source's line numbers: the creators, the
// frame TDialog and the dialog bodies. The binary moves the creators, the
// constructors that call functions defined further down (TDialog's, the
// outfit's, the edit list's, the ignore list's, the hotkeys', the error
// report's, the rule violation's) and the rule violation's HandleEvent to
// the end of the object, Creatures.cpp's pattern. The error report dialog
// (CreateErrorReportDialog and TErrorReportDialog) is gone since
// Patella-Modern issue #74 (decision D2).

#include "Dialogs.h"
#include "vulkan/DeviceSettings.h"
#include "vulkan/DeviceChoice.h"

namespace {

// The x64 client's graphics device choice in the advanced options (issue #31): the line above the
// buttons before a device exists, and the buttons' help texts.
const char *TextSelectDevice = "Select graphics device:";
const char *TextUseAutomaticDevice = "Use the best graphics device: a GPU, else the CPU";
const char *TextUseGpuDevice = "Use the graphics card (GPU); with several, each click takes the next";
const char *TextUseCpuDevice = "CPU (compatibility): draw without the graphics card";

} // namespace

#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include "Communication.h"
#include "Creatures.h"
#include "Help.h"
#include "Map.h"
#include "Objects.h"
#include "Preferences.h"
#include "Status.h"
#include "Surface.h"
#include "Talk.h"
#include "Utils.h"
#include "DialogElements.h"
#include "GUI.h"
#include "BuildConfig.h"
#include "Skins.h"
#include "common.h"
#include "compat/span.h"
#include "types/FixedString.h"
#include "graphics.h"
#include <cstddef>
#include <memory>

namespace {

// The texts of the user interface that this file shows. The numbers are their places in the original's table of
// texts, a header every file included.
const char *TextAccountNumber = "Account number:";                                                                          // 58
const char *TextPassword = "Password:";                                                                                     // 59
const char *TextNoAccountYet = "If you don't have\nan account yet:";                                                        // 60
const char *TextSelectCharacter = "Select character:";                                                                      // 61
const char *TextTryingToReconnect = "Trying to reconnect in %d %s.";                                                        // 62
const char *TextSetNumberOfItems = "Set the number of\nitems you want to move:";                                            // 63
const char *TextChooseOutfit = "Choose an outfit and determine the colours of the various\nparts of your character's body"; // 64
const char *TextSelectChannel = "Select a channel:";                                                                        // 65
const char *TextOpenPrivateChannel = "Open a private message channel:";                                                     // 66
const char *TextEnterCharacterName = "Please enter a character name:";                                                      // 67
const char *TextStateRuleViolation =                                                                                        // 68
	"Please state the rule violation in one\n"
	"clear sentence and wait for a reply\n"
	"from a gamemaster.\n"
	"Please note that your message will\n"
	"disappear if you close this channel.";
const char *TextReportBugsOnly = "Please use this dialog only to report bugs.\nDo not report rule violations here!"; // 69
const char *TextSelectType = "Select a type:";                                                                       // 70
const char *TextEnterCharacterName2 = "Please enter a character name:";                                              // 71
const char *TextEnterShortDescription = "Enter a short description:";                                                // 72
const char *TextClientInfo = "Click here to get\ninfo on the client.";                                               // 77
const char *TextShowPreviousHints = "Show previous hints";                                                           // 78
const char *TextAskTutor = "Ask a tutor for help";                                                                   // 79
const char *TextReportRuleViolations = "Report rule violations";                                                     // 80
const char *TextReadManual = "Read the manual on\nthe website";                                                      // 81
const char *TextReadFAQ = "Read the FAQ on\nthe website.";                                                           // 82
const char *TextAvailableHints = "Available hints:";                                                                 // 83
const char *TextChangeGeneralOptions = "Change general\ngame options";                                               // 84
const char *TextChangeGraphicsOptions = "Change graphics and\nperformance settings";                                 // 85
const char *TextCustomiseConsole = "Customise the console";                                                          // 86
const char *TextChangeNetworkSettings = "Change network settings";                                                   // 87
const char *TextEditHotkeys = "Edit your hotkey texts";                                                              // 88
const char *TextShowMotD = "Show the most recent\nMessage of the Day";                                               // 89
const char *TextAdvancedSettings = "Advanced settings";                                                              // 92
const char *TextAvailableResolutions = "Available resolutions:";                                                     // 93
const char *TextAvailableHotkeys = "Available hotkeys:";                                                             // 95
const char *TextEditHotkeyText = "Edit hotkey text:";                                                                // 96
const char *TextClientVersion = "Patella\nVersion 7.72";                                                             // 97
const char *TextOfficialWebsite = "Official Website";                                                                // 98
const char *TextCharactersToIgnore = "Characters you wish to ignore:";                                               // 99
const char *TextGlobalIgnoreSettings = "Global settings for ignoring messages:";                                     // 100
const char *TextCharactersToAllow = "Characters you allow to message you:";                                          // 101
const char *TextGlobalAllowSettings = "Global settings for allowing messages:";                                      // 102
const char *TextEnterOneNamePerLine = "Enter one name per line.";                                                    // 103
const char *TextItIsEmpty = "It is empty.";                                                                          // 104
const char *TextItIsCurrentlyEmpty = "It is currently empty.";                                                       // 105
const char *TextYouReadTheFollowing = "You read the following.";                                                     // 106
const char *TextYouCanEnterNewText = "You can enter new text.";                                                      // 107
const char *TextYouReadWrittenBy = "You read the following, written by";                                             // 108
const char *TextSelectType2 = "Select a type:";                                                                      // 112
const char *TextEnterShortDescription2 = "Enter a short description:";                                               // 113
const char *TextRequestAlreadySubmitted = "You have already submitted a request. Please close the current request."; // 121
const char *TextNoPrivateChannelWithYourself = "You cannot set up a private message channel with yourself.";         // 123
const char *TextNoPlayerName = "No playername given.";                                                               // 128
const char *TextNoReason = "No reason has been selected.";                                                           // 129
const char *TextNoAction = "No action has been selected.";                                                           // 130
const char *TextNoComment = "You must enter a comment.";                                                             // 131
const char *TextNoStatement = "No statement has been selected.";                                                     // 132
const char *TextCannotReportStatement = "You cannot report this statement.";                                         // 133
const char *TextTibiaURL = patella::buildconfig::LinkWebsite;                                                        // 135
const char *TextManualURL = patella::buildconfig::LinkManual;                                                        // 136
const char *TextFAQURL = patella::buildconfig::LinkFAQ;                                                              // 137
const char *TextOptionsURL = patella::buildconfig::LinkOptionsHelp;                                                  // 138
const char *TextGraphicsOptionsURL = patella::buildconfig::LinkGraphicsHelp;                                         // 139
const char *TextAdvancedGraphicsURL = patella::buildconfig::LinkGraphicsHelp;                                        // 140
const char *TextConsoleOptionsURL = patella::buildconfig::LinkConsoleHelp;                                           // 141
const char *TextNetworkOptionsURL = patella::buildconfig::LinkNetworkHelp;                                           // 142
const char *TextSignupURL = patella::buildconfig::LinkSignup;                                                        // 144

} // namespace

// Issue #111 (clang-tidy, the first look at this file in patella_core): cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg off for the file:
// the original's raw owners (notes/ownership.md): the window tree owns its children, the message queue its blocks,
// the lists their entries; the original's varargs (TMultiStateButton's constructor, _snprintf)
// NOLINTBEGIN(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg)


namespace {

// NOLINTNEXTLINE(bugprone-throwing-static-initialization): the binary's global; TCallback's constructor (GUI.cpp) stores two values
TCallback NoCallback(nullptr, 0);

} // namespace

// --- Creators ----------------------------------------------------------------------

// A message; OK and Escape close it.
TDialog *CreateMessageDialog(const char *Caption, const char *Text)
{
	TLabel *Label = new TLabel(0, 0, SkinProperties().Dialog.TextFont, Text);
	TDialog *Dialog = new TDialog(Caption, Label);
	Dialog->SetOkButton("Ok", TCallback(Dialog, 1));
	Dialog->SetCancelButton(nullptr, TCallback(Dialog, 1));
	return Dialog;
}

// A message with formatting tokens.
TDialog *CreateFormattedMessageDialog(const char *Caption, const char *Text)
{
	TFormattedText *FormattedText = new TFormattedText(Text);
	FormattedText->SetSize(SkinProperties().FormattedMessageWidth, SkinProperties().FormattedMessageHeight);
	FormattedText->FitToChildren(TRect(0, 0, 0, 0), false);
	TDialog *Dialog = new TDialog(Caption, FormattedText);
	Dialog->SetOkButton("Ok", TCallback(Dialog, 1));
	Dialog->SetCancelButton(nullptr, TCallback(Dialog, 1));
	return Dialog;
}

// "Connecting": Abort (and Escape) send 29 to the receiver (the main
// window); the next queued message closes it.
TDialog *CreateConnectingDialog(const char *Caption, const char *Text, TComponent *Receiver)
{
	TWaitLabel *Label = new TWaitLabel(Text, true, false);
	TDialog *Dialog = new TDialog(Caption, Label);
	Dialog->SetOkButton("Abort", TCallback(Receiver, EVENT_ABORT));
	Dialog->SetCancelButton(nullptr, TCallback(Receiver, EVENT_ABORT));
	Dialog->CancelCallback = TCallback(Receiver, EVENT_ABORT);
	return Dialog;
}

// "Please wait": Abort clears the message queue; a timeout (tenths of
// a second) closes the dialog.
TDialog *CreateWaitDialog(const char *Caption, const char *Text, int Timeout)
{
	TWaitLabel *Label = new TWaitLabel(Text, false, true);
	TDialog *Dialog = new TDialog(Caption, Label);
	Dialog->SetOkButton("Abort", TCallback(Label, EVENT_ABORT));
	Dialog->SetCancelButton(nullptr, TCallback(Label, EVENT_ABORT));
	Dialog->CancelCallback = TCallback(Label, EVENT_ABORT);
	if (Timeout > 0)
		Label->SetTimer(Timeout, TCallback(Label, EVENT_TIMER));
	return Dialog;
}

// The reconnect countdown (timeout in seconds).
TDialog *CreateReconnectDialog(const char *Caption, const char *Text, int timeout)
{
	TReconnectWindow *Window = new TReconnectWindow(Text, timeout);
	TDialog *Dialog = new TDialog(Caption, Window);
	Dialog->SetOkButton("Abort", TCallback(Window, EVENT_ABORT));
	Dialog->SetCancelButton(nullptr, TCallback(Window, EVENT_ABORT));
	Dialog->CancelCallback = TCallback(Window, EVENT_ABORT);
	return Dialog;
}

// A yes/no question whose answers go to the receiver.
TDialog *CreateQuestionDialog(const char *Caption, const char *Text, TComponent *Receiver, int YesEvent,
	int NoEvent)
{
	TQuestionLabel *Label = new TQuestionLabel(SkinProperties().Dialog.TextFont, Text, TCallback(Receiver, YesEvent),
		TCallback(Receiver, NoEvent));
	TDialog *Dialog = new TDialog(Caption, Label);
	Dialog->SetOkButton("Yes", TCallback(Receiver, YesEvent));
	Dialog->SetCancelButton("No", TCallback(Receiver, NoEvent));
	return Dialog;
}

// Logout (506), Cancel (2) and Exit (7) go to the receiver.
TDialog *CreateLogoutDialog(const char *Caption, const char *Text, TComponent *Receiver)
{
	TLogoutLabel *Label = new TLogoutLabel(SkinProperties().Dialog.TextFont, Text, TCallback(Receiver, EVENT_LOGOUT),
		TCallback(Receiver, EVENT_EXIT_PROGRAM));
	TDialog *Dialog = new TDialog(Caption, Label);
	Dialog->SetOkButton("Logout", TCallback(Receiver, EVENT_LOGOUT));
	Dialog->SetCancelButton("Cancel", TCallback(Receiver, EVENT_CANCEL));
	Dialog->SetExtraButton("Exit", TCallback(Receiver, EVENT_EXIT_PROGRAM));
	return Dialog;
}

// BuildConfig.h's limit for a caption is what TTextButton's text holds.
static_assert(sizeof(TTextButton::Text) == patella::buildconfig::MaxCaption + 1, "TTextButton::Text and MaxCaption");

// "Info": the client version and a link to the web site, in a frame of
// skins.
TDialog *CreateInfoDialog(void (*OpenURL)(const char *URL))
{
	TWindow *Window = new TWindow();
	Window->SetSize(SkinProperties().InfoDialog.Width, SkinProperties().InfoDialog.Height);
	TLabel *Label = new TLabel(0, 0, SkinProperties().InfoDialog.LabelFont, TextClientVersion);
	TLink *Link = new TLink(TextOfficialWebsite, patella::buildconfig::WebsiteCaption, TextTibiaURL, OpenURL,
		SkinProperties().InfoDialog.LinkFont);
	TSeparator *Separator = new TSeparator(true);
	Window->InsertChild(Label);
	Window->InsertChild(Separator);
	Window->InsertChild(Link);
	Window->SetChildArea(Label, SkinProperties().InfoDialog.Label);
	Window->SetChildArea(Separator, SkinProperties().InfoDialog.Separator);
	Window->SetChildArea(Link, SkinProperties().InfoDialog.Link);
	TBorder *Border = new TBorder(Window, SKIN_NONE, SKIN_FRAME_EDGE_TOP, SKIN_NONE, SKIN_FRAME_EDGE_LEFT, SKIN_FRAME_EDGE_RIGHT, SKIN_NONE, SKIN_FRAME_EDGE_BOTTOM, SKIN_NONE, SkinProperties().InfoDialog.Border);
	Border->FitToWindow();
	TDialog *Dialog = new TDialog("Info", Border);
	Dialog->SetOkButton("Ok", TCallback(Dialog, 1));
	Dialog->SetCancelButton(nullptr, TCallback(Dialog, 1));
	return Dialog;
}

// Only for a dragged pile of 1-100 objects; otherwise it ends the drag
// and makes no dialog.
TDialog *CreateMoveObjectsDialog()
{
	int Unused;
	const TObject *Obj = GetDragObject(&Unused, &Unused, &Unused, &Unused);
	if (*Obj != 0 && Obj->Data >= 1 && Obj->Data <= 100) {
		TMoveObjectsDialog *Body = new TMoveObjectsDialog();
		TDialog *Dialog = new TDialog("Move Objects", Body);
		Dialog->SetOkButton("Ok", TCallback(Body, 1));
		Dialog->SetCancelButton("Cancel", TCallback(Body, EVENT_CANCEL));
		return Dialog;
	}
	SetActionState(ACTIONSTATE_NONE);
	return nullptr;
}

TDialog *CreateLoginDialog(void (*OpenURL)(const char *URL))
{
	TLoginDialog *Body = new TLoginDialog(OpenURL);
	TDialog *Dialog = new TDialog("Enter Game", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->SetCancelButton("Cancel", TCallback(Body, EVENT_CANCEL));
	return Dialog;
}

// Timeout in tenths of a second, 0 for none.
TDialog *CreateCharacterListDialog(int Timeout)
{
	TCharacterListDialog *Body = new TCharacterListDialog();
	TDialog *Dialog = new TDialog("Select Character", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	if (Timeout > 0)
		Body->SetTimeout(Timeout);
	return Dialog;
}

// ObjectData holds the server's object number, the object type and the
// maximum length. The text can be edited when the object is writable,
// or writable once and still empty; Return does not close either form
// (OkCallback is cleared).
TDialog *CreateTextDialog(const int *ObjectData, const char *Author, const char *Text)
{
	const int ObjectNumber = ObjectData[0];
	const int Type = ObjectData[1];
	const int MaximumLength = ObjectData[2];
	TFixedString<500> Description;
	Description[0] = 0;
	TTextDialog *Body;
	TDialog *Dialog;
	if (GetObjectFlag(Type, WRITE) || (GetObjectFlag(Type, WRITEONCE) && strlen(Text) == 0)) {
		if (strlen(Text) == 0)
			TStringBuffer(Description, 255).Format("{}\n{}", TextItIsCurrentlyEmpty, TextYouCanEnterNewText);
		else if (strlen(Author) != 0)
			MsvcSnprintf(Description, 255, "%s\n%s.\n%s", TextYouReadWrittenBy, Author, TextYouCanEnterNewText);
		else
			TStringBuffer(Description, 255).Format("{}\n{}", TextYouReadTheFollowing, TextYouCanEnterNewText);
		TObject Obj;
		Obj.Type = Type;
		Body = new TTextDialog(MaximumLength, Text, ObjectNumber, Obj, Description);
		Dialog = new TDialog("Edit Text", Body);
		Dialog->SetOkButton("Ok", TCallback(Body, 1));
	} else {
		if (strlen(Text) == 0)
			TStringBuffer(Description, 255).Assign(TextItIsEmpty);
		else if (strlen(Author) != 0)
			MsvcSnprintf(Description, 255, "%s\n%s.", TextYouReadWrittenBy, Author);
		else
			TStringBuffer(Description, 255).Assign(TextYouReadTheFollowing);
		TObject Obj;
		Obj.Type = Type;
		Body = new TTextDialog(MaximumLength, Text, ObjectNumber, Obj, Description);
		Dialog = new TDialog("Show Text", Body);
		Dialog->SetOkButton("Ok", TCallback(Body, EVENT_CANCEL));
		Dialog->SetCancelButton(nullptr, TCallback(Body, EVENT_CANCEL));
	}
	Dialog->OkCallback = NoCallback;
	return Dialog;
}

// HouseData holds the list's type and identifier.
TDialog *CreateEditListDialog(int *HouseData, const char *Text)
{
	TEditListDialog *Body = new TEditListDialog(2000, Text, HouseData[0], HouseData[1], TextEnterOneNamePerLine);
	TDialog *Dialog = new TDialog("Edit List", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->OkCallback = NoCallback;
	return Dialog;
}

TDialog *CreateOutfitDialog(int *Data)
{
	TOutfitDialog *Body = new TOutfitDialog(Data);
	TDialog *Dialog = new TDialog("Select Outfit", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	return Dialog;
}

TDialog *CreateBugReportDialog()
{
	TBugReportDialog *Body = new TBugReportDialog();
	TDialog *Dialog = new TDialog("Reporting", Body);
	Dialog->SetOkButton("Send", TCallback(Body, 1));
	Dialog->OkCallback = NoCallback;
	return Dialog;
}

TDialog *CreateNewChannelDialog(int NumberOfChannels, int *ChannelIDs, const char (*ChannelNames)[30]) // NOLINT(modernize-avoid-c-arrays): Dialogs.h
{
	TNewChannelDialog *Body = new TNewChannelDialog(NumberOfChannels, ChannelIDs, ChannelNames);
	TDialog *Dialog = new TDialog("New Channel", Body);
	Dialog->SetOkButton("Open", TCallback(Body, 1));
	return Dialog;
}

TDialog *CreateInviteDialog()
{
	TPrivateChannelDialog *Body = new TPrivateChannelDialog(false);
	TDialog *Dialog = new TDialog("Invite player to private chat channel", Body);
	Dialog->SetOkButton("Add", TCallback(Body, 1));
	return Dialog;
}

// The OK button says "Add" here too.
TDialog *CreateExcludeDialog()
{
	TPrivateChannelDialog *Body = new TPrivateChannelDialog(true);
	TDialog *Dialog = new TDialog("Exclude player from private chat channel", Body);
	Dialog->SetOkButton("Add", TCallback(Body, 1));
	return Dialog;
}

// OK sends 602.
TDialog *CreateIgnoreListDialog()
{
	TIgnoreListDialog *Body = new TIgnoreListDialog();
	TDialog *Dialog = new TDialog("Edit Ignore List and White List", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, EVENT_IGNORE_LIST_OK));
	Dialog->OkCallback = NoCallback;
	Dialog->SetCancelButton("Cancel", TCallback(Body, EVENT_CANCEL));
	return Dialog;
}

// Only one open request at a time (a rule violation channel buffer
// exists while one is open).
TDialog *CreateReportDialog()
{
	if (GetTextBuffer(TALK_GAMEMASTER_ANSWER, nullptr, -1) != nullptr) {
		SetStatusMessage(TextRequestAlreadySubmitted, false);
		return nullptr;
	}
	TReportDialog *Body = new TReportDialog();
	TDialog *Dialog = new TDialog("Report Rule Violation", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	return Dialog;
}

TDialog *CreateAddBuddyDialog()
{
	TAddBuddyDialog *Body = new TAddBuddyDialog();
	TDialog *Dialog = new TDialog("Add to VIP list", Body);
	Dialog->SetOkButton("Add", TCallback(Body, 1));
	return Dialog;
}

TDialog *CreateEditBuddyDialog(int CharacterID)
{
	TEditBuddyDialog *Body = new TEditBuddyDialog(CharacterID);
	TDialog *Dialog = new TDialog("Change player in VIP list", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->OkCallback = NoCallback;
	return Dialog;
}

// CreateUpdateDialog is gone with the update dialog since Patella-Modern issue #75 (decision D9).

// OK goes to the frame (which closes the dialog).
TDialog *CreateHelpDialog(void (*OpenURL)(const char *URL))
{
	THelpDialog *Body = new THelpDialog(OpenURL);
	TDialog *Dialog = new TDialog("Help", Body);
	Dialog->SetOkButton("Ok", TCallback(Dialog, 1));
	Dialog->SetCancelButton(nullptr, TCallback(Dialog, 1));
	return Dialog;
}

// Cancel goes to the frame.
TDialog *CreateHintsDialog()
{
	THintsDialog *Body = new THintsDialog();
	TDialog *Dialog = new TDialog("Hints for Tibia", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->SetCancelButton("Cancel", TCallback(Dialog, EVENT_CANCEL));
	return Dialog;
}

// Name and Statement may be NULL.
TDialog *CreateRuleViolationDialog(const char *Name, TTalkLine *Statement)
{
	TRuleViolationDialog *Body = new TRuleViolationDialog(Name, Statement);
	TDialog *Dialog = new TDialog("Rule Violation", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->SetCancelButton("Cancel", TCallback(Body, EVENT_CANCEL));
	return Dialog;
}

// The body deletes the coordinate and the mark.
TDialog *CreateEditMarkDialog(TCoordinate *Coordinate, TMark *Mark)
{
	TEditMarkDialog *Body = new TEditMarkDialog(Coordinate, Mark);
	TDialog *Dialog = new TDialog("Edit Mark", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->SetCancelButton("Cancel", TCallback(Body, EVENT_CANCEL));
	return Dialog;
}

// Escape is OK here (both save the options).
TDialog *CreateOptionsDialog()
{
	TOptionsDialog *Body = new TOptionsDialog();
	TDialog *Dialog = new TDialog("Options", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->SetCancelButton(nullptr, TCallback(Body, 1));
	Dialog->CancelCallback = TCallback(Body, 1);
	return Dialog;
}

TDialog *CreateGeneralOptionsDialog(void (*OpenURL)(const char *URL))
{
	TGeneralOptionsDialog *Body = new TGeneralOptionsDialog(OpenURL);
	TDialog *Dialog = new TDialog("General Options", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->SetExtraButton("Help", TCallback(Body, EVENT_HELP));
	return Dialog;
}

TDialog *CreateGraphicsOptionsDialog(void (*OpenURL)(const char *URL), void (*SetDisplayMode)(int Mode),
	TWindow *Parent)
{
	TGraphicsOptionsDialog *Body = new TGraphicsOptionsDialog(OpenURL, SetDisplayMode, Parent);
	TDialog *Dialog = new TDialog("Graphics Options", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->SetExtraButton("Help", TCallback(Body, EVENT_HELP));
	return Dialog;
}

TDialog *CreateAdvancedGraphicsDialog(void (*OpenURL)(const char *URL), TComponent *Receiver)
{
	TAdvancedGraphicsDialog *Body = new TAdvancedGraphicsDialog(OpenURL, Receiver);
	TDialog *Dialog = new TDialog("Advanced Graphics Options", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->SetCancelButton("Cancel", TCallback(Body, EVENT_CANCEL));
	Dialog->SetExtraButton("Help", TCallback(Body, EVENT_HELP));
	return Dialog;
}

TDialog *CreateConsoleOptionsDialog(void (*OpenURL)(const char *URL))
{
	TConsoleOptionsDialog *Body = new TConsoleOptionsDialog(OpenURL);
	TDialog *Dialog = new TDialog("Console Options", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->SetExtraButton("Help", TCallback(Body, EVENT_HELP));
	return Dialog;
}

TDialog *CreateNetworkOptionsDialog(void (*OpenURL)(const char *URL))
{
	TNetworkOptionsDialog *Body = new TNetworkOptionsDialog(OpenURL);
	TDialog *Dialog = new TDialog("Network Options", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	Dialog->SetExtraButton("Help", TCallback(Body, EVENT_HELP));
	return Dialog;
}

TDialog *CreateHotkeyDialog()
{
	THotkeyDialog *Body = new THotkeyDialog();
	TDialog *Dialog = new TDialog("Hotkey Options", Body);
	Dialog->SetOkButton("Ok", TCallback(Body, 1));
	return Dialog;
}

// --- TDialog ----------------------------------------------------------------------

// The assertions come after the base class and the two callbacks, before the original's try
// block. The frame has nine skins (43-51, Arrange); the caption is copied, the buttons are
// OK and Cancel until the creator changes them.
TDialog::TDialog(const char *_Caption, TComponent *_Dialog)
{
	PATELLA_ASSERT(_Caption!=nullptr);
	PATELLA_ASSERT_VALUES(strlen(_Caption)<100, strlen(_Caption));
	PATELLA_ASSERT(_Dialog!=nullptr);
	DrawingListLength = 9;
	DrawingList.reset(new TDrawing[9]);
	Caption.Assign(_Caption);
	Dragging = false;
	OkCallback = NoCallback;
	CancelCallback = NoCallback;
	OkButton = nullptr;
	CancelButton = nullptr;
	ExtraButton = nullptr;
	Dialog = _Dialog;
	CaptionLabel = new TLabel(0, 0, SkinProperties().Dialog.CaptionFont, Caption);
	Separator = new TSeparator(true);
	InsertChild(Dialog);
	InsertChild(CaptionLabel);
	InsertChild(Separator);
	FitToChild(Dialog, SkinProperties().Dialog.Body, true);
	SetOkButton("Ok", TCallback(this, 1));
	SetCancelButton("Cancel", TCallback(this, EVENT_CANCEL));
}

void TDialog::SetOkButton(const char *Text, TCallback Callback)
{
	if (Text == nullptr) {
		PATELLA_ASSERT(OkButton!=nullptr);
		DeleteChild(OkButton);
		OkButton = nullptr;
		OkCallback = NoCallback;
	} else {
		if (OkButton != nullptr)
			DeleteChild(OkButton);
		OkButton = new TTextButton(0, 0, Text, Callback, nullptr);
		if (Callback.Event == NoCallback.Event && Callback.Receiver == NoCallback.Receiver)
			Callback = TCallback(this, 1);
		OkCallback = Callback;
		InsertChild(OkButton);
	}
	Arrange();
}

// Without a button Escape still sends (this, 2).
void TDialog::SetCancelButton(const char *Text, TCallback Callback)
{
	if (Text == nullptr) {
		PATELLA_ASSERT(CancelButton!=nullptr);
		DeleteChild(CancelButton);
		CancelButton = nullptr;
		CancelCallback = TCallback(this, EVENT_CANCEL);
	} else {
		if (CancelButton != nullptr)
			DeleteChild(CancelButton);
		CancelButton = new TTextButton(0, 0, Text, Callback, nullptr);
		if (Callback.Event == NoCallback.Event && Callback.Receiver == NoCallback.Receiver)
			Callback = TCallback(this, EVENT_CANCEL);
		CancelCallback = Callback;
		InsertChild(CancelButton);
	}
	Arrange();
}

void TDialog::SetExtraButton(const char *Text, TCallback Callback)
{
	if (Text == nullptr) {
		PATELLA_ASSERT(ExtraButton!=nullptr);
		DeleteChild(ExtraButton);
		ExtraButton = nullptr;
	} else {
		if (ExtraButton != nullptr)
			DeleteChild(ExtraButton);
		ExtraButton = new TTextButton(0, 0, Text, Callback, nullptr);
		InsertChild(ExtraButton);
	}
	Arrange();
}

// The buttons take the positions from the right in the order Cancel, OK,
// extra. The bottom edge starts at Skins[45].height (the corner's height
// where its width would fit; kept).
void TDialog::Arrange()
{
	SetChildArea(Dialog, SkinProperties().Dialog.Body);
	SetChildArea(CaptionLabel, SkinProperties().Dialog.Caption);
	SetChildArea(Separator, SkinProperties().Dialog.Separator);
	int Button = 0;
	if (CancelButton != nullptr) {
		SetChildPosition(CancelButton, SkinProperties().Dialog.Buttons[Button].x,
			SkinProperties().Dialog.Buttons[Button].y);
		Button++;
	}
	if (OkButton != nullptr) {
		SetChildPosition(OkButton, SkinProperties().Dialog.Buttons[Button].x,
			SkinProperties().Dialog.Buttons[Button].y);
		Button++;
	}
	if (ExtraButton != nullptr) {
		SetChildPosition(ExtraButton, SkinProperties().Dialog.Buttons[Button].x,
			SkinProperties().Dialog.Buttons[Button].y);
	}
	DrawingList[0] = TDrawing(SKIN_DIALOG_CORNER_TOPLEFT, 0, 0, 0, 0, 0, 0);
	DrawingList[1] = TDrawing(SKIN_DIALOG_EDGE_TOP, Skins()[SKIN_DIALOG_CORNER_TOPLEFT].width, 0, Width - Skins()[SKIN_DIALOG_CORNER_TOPRIGHT].width - Skins()[SKIN_DIALOG_CORNER_TOPLEFT].width,
		Skins()[SKIN_DIALOG_EDGE_TOP].height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_DIALOG_CORNER_TOPRIGHT, Width - Skins()[SKIN_DIALOG_CORNER_TOPRIGHT].width, 0, 0, 0, 0, 0);
	DrawingList[3] = TDrawing(SKIN_DIALOG_EDGE_LEFT, 0, Skins()[SKIN_DIALOG_CORNER_TOPLEFT].height, Skins()[SKIN_DIALOG_EDGE_LEFT].width,
		Height - Skins()[SKIN_DIALOG_CORNER_BOTTOMLEFT].height - Skins()[SKIN_DIALOG_CORNER_TOPLEFT].height, 0, 0);
	DrawingList[4] = TDrawing(SKIN_DIALOG_EDGE_RIGHT, Width - Skins()[SKIN_DIALOG_EDGE_RIGHT].width, Skins()[SKIN_DIALOG_EDGE_TOP].height, Skins()[SKIN_DIALOG_EDGE_RIGHT].width,
		Height - Skins()[SKIN_DIALOG_CORNER_BOTTOMRIGHT].height - Skins()[SKIN_DIALOG_CORNER_TOPRIGHT].height, 0, 0);
	DrawingList[5] = TDrawing(SKIN_DIALOG_CORNER_BOTTOMLEFT, 0, Height - Skins()[SKIN_DIALOG_CORNER_BOTTOMLEFT].height, 0, 0, 0, 0);
	DrawingList[6] = TDrawing(SKIN_DIALOG_EDGE_BOTTOM, Skins()[SKIN_DIALOG_CORNER_BOTTOMLEFT].height, Height - Skins()[SKIN_DIALOG_EDGE_BOTTOM].height,
		Width - Skins()[SKIN_DIALOG_CORNER_BOTTOMRIGHT].width - Skins()[SKIN_DIALOG_CORNER_BOTTOMLEFT].width, Skins()[SKIN_DIALOG_EDGE_BOTTOM].height, 0, 0);
	DrawingList[7] = TDrawing(SKIN_DIALOG_CORNER_BOTTOMRIGHT, Width - Skins()[SKIN_DIALOG_CORNER_BOTTOMRIGHT].width, Height - Skins()[SKIN_DIALOG_CORNER_BOTTOMRIGHT].height, 0, 0, 0, 0);
	DrawingList[8] = TDrawing(SKIN_DIALOG_BACKGROUND, Skins()[SKIN_DIALOG_EDGE_LEFT].width, Skins()[SKIN_DIALOG_EDGE_TOP].height, Width - Skins()[SKIN_DIALOG_EDGE_RIGHT].width - Skins()[SKIN_DIALOG_EDGE_LEFT].width,
		Height - Skins()[SKIN_DIALOG_EDGE_BOTTOM].height - Skins()[SKIN_DIALOG_EDGE_TOP].height, 0, 0);
}

void TDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN) {
		if (OkButton != nullptr
			&& !(OkCallback.Event == NoCallback.Event && OkCallback.Receiver == NoCallback.Receiver)) {
			OkCallback.Send();
			return;
		}
	} else if (Key == KEY_ESCAPE) {
		CancelCallback.Send();
		return;
	}
	Dialog->KeyPress(Key);
}

// A drag keeps the dialog inside the game window. The new position is a value
// of its own: the original's catch wrapper prints the mouse position.
bool TDialog::MouseMove(int x, int y)
{
	if (!Dragging)
		return TWindow::MouseMove(x, y);
	int NewX = OffsetX - DragX + x;
	int NewY = OffsetY - DragY + y;
	Limit(&NewX, 0, GetWindowWidth() - Width);
	Limit(&NewY, 0, GetWindowHeight() - Height);
	SetPosition(NewX, NewY);
	return true;
}

// A press on the caption line that no child takes starts a drag.
bool TDialog::LeftButtonDown(int x, int y, int Flags)
{
	if (!TWindow::LeftButtonDown(x, y, Flags) && In(x, 0, Width - 1) && In(y, 0, Skins()[SKIN_DIALOG_EDGE_TOP].height - 1)) {
		Dragging = true;
		DragX = x;
		DragY = y;
	}
	return true;
}

bool TDialog::LeftButtonUp(int x, int y, int Flags)
{
	if (!Dragging)
		return TWindow::LeftButtonUp(x, y, Flags);
	Dragging = false;
	return true;
}

// Events 1 and 2 (the default callbacks) close the dialog, and so does
// every other.
void TDialog::HandleEvent([[maybe_unused]] int Event)
{
	SetActionState(ACTIONSTATE_NONE);
}

// --- TQuestionLabel, TLogoutLabel ----------------------------------------------

void TQuestionLabel::KeyPress(int Key)
{
	if (Key == KEY_RETURN || Key == 'y' || Key == 'Y')
		YesCallback.Send();
	if (Key == KEY_ESCAPE || Key == 'n' || Key == 'N')
		NoAnswerCallback.Send();
}

void TLogoutLabel::KeyPress(int Key)
{
	if (Key == KEY_RETURN || Key == 'l' || Key == 'L') {
		LogoutCallback.Send();
		SetActionState(ACTIONSTATE_NONE);
	}
	if (Key == 'e' || Key == 'E')
		ExitCallback.Send();
	if (Key == KEY_ESCAPE || Key == 'c' || Key == 'C')
		SetActionState(ACTIONSTATE_NONE);
}

// --- TWaitLabel ---------------------------------------------------------------------

void TWaitLabel::SetTimer(int Time, TCallback Callback)
{
	Timer.reset();
	Timer.reset(new TTimer(Time, Callback));
}

// The timer's ticks wait until it runs out.
void TWaitLabel::HandleEvent(int Event)
{
	if (Event == EVENT_TIMER) {
		if (!Timer->IsExpired())
			return;
	} else if (Event == EVENT_ABORT && ClearOnAbort) {
		ClearMessages();
	}
	SetActionState(ACTIONSTATE_NONE);
}

void TWaitLabel::Draw(int Surface, int x, int y)
{
	TLabel::Draw(Surface, x, y);
	if (CloseOnMessage && PeekMessageType() != 0)
		HandleEvent(EVENT_ABORT);
}

// --- TReconnectWindow ----------------------------------------------------------

// The width is the widest of the text, the bar and the time label with room
// for four digits (each compared twice, as with a max macro).
TReconnectWindow::TReconnectWindow(const char *Text, int timeout)
{
	Timeout = timeout * 10;
	Timer.reset(new TTimer(Timeout, TCallback(this, EVENT_TIMER)));
	SetTime(Timeout + 10);
	TLabel *TextLabel = new TLabel(SkinProperties().Dialog.TextFont, Text);
	ProgressBar = new TProgressBar();
	TimeLabel = new TLabel(SkinProperties().Dialog.TextFont, this->Text);
	int FullWidth = TextLabel->Width > ProgressBar->GetMinimumWidth() ? TextLabel->Width
																	  : ProgressBar->GetMinimumWidth();
	FullWidth = FullWidth > TimeLabel->Width + GetTextWidth(SkinProperties().Dialog.TextFont.FontNumber, "8888")
		? FullWidth
		: TimeLabel->Width + GetTextWidth(SkinProperties().Dialog.TextFont.FontNumber, "8888");
	const int TextHeight = TextLabel->Height;
	InsertChild(TextLabel);
	const int BarY = TextHeight + SkinProperties().ReconnectWindowSpacing;
	InsertChild(ProgressBar);
	const int BarHeight = ProgressBar->GetMinimumHeight();
	const int TimeY = BarY + ProgressBar->GetMinimumHeight() + SkinProperties().ReconnectWindowSpacing;
	InsertChild(TimeLabel);
	const int TimeHeight = TimeLabel->Height;
	TComponent::SetSize(FullWidth, TimeY + TimeHeight);
	SetChildArea(TextLabel, TRect(0, 0, FullWidth, TextHeight));
	SetChildArea(ProgressBar, TRect(0, BarY, FullWidth, BarHeight));
	SetChildArea(TimeLabel, TRect(0, TimeY, FullWidth, TimeHeight));
	ProgressBar->SetMaximum(Timeout);
	ProgressBar->SetCurrent(0);
	Arrange();
}

void TReconnectWindow::SetTime(int Time)
{
	if (Time > 600) {
		const int Minutes = Time / 600 + 1;
		MsvcSnprintf(Text, sizeof(Text), TextTryingToReconnect, Minutes, Minutes < 2 ? "minute" : "minutes");
	} else {
		MsvcSnprintf(Text, sizeof(Text), TextTryingToReconnect, Time / 10, Time / 10 < 2 ? "second" : "seconds");
	}
}

void TReconnectWindow::Draw(int Surface, int x, int y)
{
	TWindow::Draw(Surface, x, y);
	if (PeekMessageType() != 0)
		SetActionState(ACTIONSTATE_NONE);
}

// When the time is up it logs out of a running game (states 7 and 8) or
// connects to the game world again; Abort (29) gives up and shows the
// character list again.
void TReconnectWindow::HandleEvent(int Event)
{
	if (Event == EVENT_TIMER) {
		if (!Timer->IsExpired()) {
			const int Time = Timer->GetTime();
			ProgressBar->SetCurrent(Timeout - Time);
			SetTime(Time + 10);
			TimeLabel->SetText(Text, false);
		} else if (GetConnectionState() == CONNECTION_IN_GAME || GetConnectionState() == CONNECTION_GAME_LOGIN_ANSWERED) {
			AddMessage(MESSAGE_RECONNECT, 0, 0);
			SendLogout();
		} else {
			AddMessage(MESSAGE_CONNECT_GAME_SERVER, 0, 0);
			AddMessage(MESSAGE_CONNECTING_GAME_DIALOG, 0, 0);
		}
	} else {
		if (Event == EVENT_ABORT) {
			AddMessage(MESSAGE_WAIT_DIALOG, 20, 0);
			AddMessage(MESSAGE_CHARACTER_LIST_DIALOG, 300, 0);
		}
		SetActionState(ACTIONSTATE_NONE);
	}
}

// --- TLoginDialog -------------------------------------------------------------------

TLoginDialog::TLoginDialog(void (*OpenURL)(const char *URL))
{
	TComponent::SetSize(SkinProperties().LoginDialogWidth, SkinProperties().LoginDialogHeight);
	FocusGroup.reset(new TFocusGroup());
	Account = new TPasswordEditLine(8);
	Account->SetFocusGroup(FocusGroup.get());
	Password = new TPasswordEditLine(29);
	Password->SetFocusGroup(FocusGroup.get());
	TLink *Link = new TLink(TextNoAccountYet, "Create Account", TextSignupURL, OpenURL, SkinProperties().Dialog.TextFont);
	InsertChild(Account);
	SetChildArea(Account, SkinProperties().LoginDialogAccount);
	InsertChild(Password);
	SetChildArea(Password, SkinProperties().LoginDialogPassword);
	InsertChild(Link);
	SetChildArea(Link, SkinProperties().LoginDialogLink);
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextAccountNumber);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().LoginDialogAccountLabel);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextPassword);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().LoginDialogPasswordLabel);
	FocusGroup->SetActive(Account);
	Arrange();
}

void TLoginDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN) {
		HandleEvent(1);
	} else if (Key == KEY_ESCAPE) {
		HandleEvent(EVENT_CANCEL);
	} else {
		TEditLine *FocusEdit = FocusGroup->Active;
		PATELLA_ASSERT(FocusEdit!=nullptr);
		FocusEdit->KeyPress(Key);
	}
}

// OK (1) logs in with the account and password.
void TLoginDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		SetAccount(atoi(Account->Text.get()), Password->Text.get()); // NOLINT(bugprone-unchecked-string-to-number-conversion): the original's; the field takes digits only
		AddMessage(MESSAGE_CONNECT_LOGIN_SERVER, 0, 0);
		AddMessage(MESSAGE_CONNECTING_LOGIN_DIALOG, 0, 0);
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TCharacterListDialog ------------------------------------------------------

// The selection is the character played last while still in the game,
// else the first.
TCharacterListDialog::TCharacterListDialog()
{
	Timer = nullptr;
	TComponent::SetSize(SkinProperties().CharacterListDialogWidth, SkinProperties().CharacterListDialogHeight);
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextSelectCharacter);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().CharacterListDialogLabel);
	ListBox = new TListBox(NoCallback, TCallback(this, 1));
	InsertChild(ListBox);
	SetChildArea(ListBox, SkinProperties().CharacterListDialogList);
	const int Characters = GetAccountCharacters();
	for (int i = 0; i < Characters; i++) {
		TFixedString<372> Line;
		Line[0] = 0;
		Line.Format("{} ({})", GetCharacterName(i), GetCharacterWorld(i));
		ListBox->AddLine(Line);
	}
	if (GetConnectionState() == CONNECTION_IN_GAME && GetActiveCharacter() < Characters)
		ListBox->SetSelection(GetActiveCharacter());
	else
		ListBox->SetSelection(0);
	TFixedString<256> Status;
	const int PremiumDays = GetPremiumDays();
	if (PremiumDays == 0) {
		Status.Format("<font=4><red={}><green={}><blue={}>Account Status:\nFree Account",
			SkinProperties().Dialog.TextFont.Color.R, SkinProperties().Dialog.TextFont.Color.G,
			SkinProperties().Dialog.TextFont.Color.B);
	} else if (PremiumDays == PREMIUM_DAYS_GRATIS) {
		Status.Format("<font=4><red={}><green={}><blue={}>Account Status:\nGratis Premium Account\n",
			SkinProperties().Dialog.TextFont.Color.R, SkinProperties().Dialog.TextFont.Color.G,
			SkinProperties().Dialog.TextFont.Color.B);
	} else if (PremiumDays < 14) {
		Status.Format(
			"<font=4><red={}><green={}><blue={}>Account Status:\nPremium Account (<red={}><green={}><blue={}>{} day{} left<red={}><green={}><blue={}>)",
			SkinProperties().Dialog.TextFont.Color.R, SkinProperties().Dialog.TextFont.Color.G,
			SkinProperties().Dialog.TextFont.Color.B, 248, 96, 96, PremiumDays, PremiumDays < 2 ? "" : "s",
			SkinProperties().Dialog.TextFont.Color.R, SkinProperties().Dialog.TextFont.Color.G,
			SkinProperties().Dialog.TextFont.Color.B);
	} else {
		Status.Format("<font=4><red={}><green={}><blue={}>Account Status:\nPremium Account ({} day{} left)",
			SkinProperties().Dialog.TextFont.Color.R, SkinProperties().Dialog.TextFont.Color.G,
			SkinProperties().Dialog.TextFont.Color.B, PremiumDays, PremiumDays < 2 ? "" : "s");
	}
	TFormattedText *StatusText = new TFormattedText(Status);
	InsertChild(StatusText);
	SetChildArea(StatusText, SkinProperties().CharacterListDialogStatus);
	Arrange();
}

// Every click starts the timeout again.
bool TCharacterListDialog::LeftButtonDown(int x, int y, int Flags)
{
	SetTimeout(Seconds);
	return TWindow::LeftButtonDown(x, y, Flags);
}

bool TCharacterListDialog::RightButtonUp(int x, int y, int Flags)
{
	SetTimeout(Seconds);
	return TWindow::RightButtonUp(x, y, Flags);
}

bool TCharacterListDialog::MouseWheel(int x, int y, int Code)
{
	SetTimeout(Seconds);
	return TWindow::MouseWheel(x, y, Code);
}

void TCharacterListDialog::PositionChanged()
{
	SetTimeout(Seconds);
}

// Another key stops the timer before the list gets it; SetTimeout starts it
// again.
void TCharacterListDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN) {
		HandleEvent(1);
	} else if (Key == KEY_ESCAPE) {
		HandleEvent(EVENT_CANCEL);
	} else {
		Timer.reset();
		Timer = nullptr;
		ListBox->KeyPress(Key);
	}
	SetTimeout(Seconds);
}

// OK (1) or a double click enters the game with the selected character
// (logging out of a running game first).
void TCharacterListDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		const int Selection = ListBox->GetSelection();
		if (Selection >= 0) {
			SetActiveCharacter(Selection);
			if (GetConnectionState() == CONNECTION_IN_GAME || GetConnectionState() == CONNECTION_GAME_LOGIN_ANSWERED) {
				AddMessage(MESSAGE_RECONNECT, 0, 0);
				SendLogout();
			} else {
				AddMessage(MESSAGE_CONNECT_GAME_SERVER, 0, 0);
				AddMessage(MESSAGE_CONNECTING_GAME_DIALOG, 0, 0);
			}
		}
	} else if (Event == EVENT_TIMER && !Timer->IsExpired()) {
		return;
	}
	SetActionState(ACTIONSTATE_NONE);
}

void TCharacterListDialog::SetTimeout(int seconds)
{
	Timer.reset();
	Timer.reset(new TTimer(seconds, TCallback(this, EVENT_TIMER)));
	Seconds = seconds;
}

// --- TMoveObjectsDialog ---------------------------------------------------------

TMoveObjectsDialog::TMoveObjectsDialog()
{
	LastDigit = 0;
	DraggedObject = *GetDragObject(&SourceIndex, &SourceX, &SourceY, &SourceZ);
	GetDragTarget(&DestinationX, &DestinationY, &DestinationZ);
	PATELLA_ASSERT(DraggedObject!=0);
	PATELLA_ASSERT_VALUES(In(DraggedObject.Data,1,100), DraggedObject.Data, DraggedObject);
	TComponent::SetSize(SkinProperties().MoveObjectsDialogWidth, SkinProperties().MoveObjectsDialogHeight);
	TLabel *Label = new TLabel(0, 0, SkinProperties().Dialog.TextFont, TextSetNumberOfItems);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().MoveObjectsDialogLabel);
	ScrollBar = new THorizontalScrollBar(1, DraggedObject.Data, DraggedObject.Data, 11, 1, NoCallback);
	InsertChild(ScrollBar);
	SetChildArea(ScrollBar, SkinProperties().MoveObjectsDialogScrollBar);
	Arrange();
}

// The pile is drawn with the chosen count; the cumulative pattern
// is computed and not used.
void TMoveObjectsDialog::Draw(int Surface, int x, int y)
{
	DraggedObject.Data = ScrollBar->Range->Value;
	int PatternX = 1;
	int PatternY = 1;
	if (GetObjectFlag(DraggedObject, CUMULATIVE))
		GetCumulativePattern(DraggedObject, &PatternX, &PatternY);
	DrawObject(Surface, SkinProperties().MoveObjectsDialogObject.x + x, SkinProperties().MoveObjectsDialogObject.y + y, 32, DraggedObject,
		TRGBColor(0, 0, 0), TRect(SkinProperties().MoveObjectsDialogObject.x + x, SkinProperties().MoveObjectsDialogObject.y + y, 32, 32),
		SkinProperties().ObjectCountFont, true);
	TWindow::Draw(Surface, x, y);
}

// Digits type the count (the last two digits); PageUp/PageDown change it
// by 10, Home/End and the arrow keys as they do in a list.
void TMoveObjectsDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN) {
		HandleEvent(1);
	} else if (Key == KEY_ESCAPE) {
		HandleEvent(EVENT_CANCEL);
	} else if (Key >= '0' && Key <= '9') {
		ScrollBar->Range->SetValue(LastDigit * 10 + Key - '0');
		LastDigit = Key - '0';
	} else if (Key == KEY_PAGE_UP) {
		ScrollBar->Range->SetValue(ScrollBar->Range->Value + 10);
	} else if (Key == KEY_PAGE_DOWN) {
		ScrollBar->Range->SetValue(ScrollBar->Range->Value - 10);
	} else if (Key == KEY_HOME) {
		ScrollBar->Range->SetValue(ScrollBar->Range->Max);
	} else if (Key == KEY_END) {
		ScrollBar->Range->SetValue(1);
	} else if (Key == KEY_UP) {
		ScrollBar->Range->SetValue(ScrollBar->Range->Value + 1);
	} else if (Key == KEY_DOWN) {
		ScrollBar->Range->SetValue(ScrollBar->Range->Value - 1);
	} else {
		ScrollBar->KeyPress(Key);
	}
}

void TMoveObjectsDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		const int Count = ScrollBar->Range->Value;
		if (Count > 0)
			SendMoveObject(SourceX, SourceY, SourceZ, DraggedObject.Type, SourceIndex, DestinationX, DestinationY,
				DestinationZ, Count);
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TOutfitDialog ------------------------------------------------------------------

namespace {

// The colour of an outfit colour button: GetOutfitColor runs inside the
// new-expression (after the allocation), so the binary has it in the
// argument list (inlined; the name is ours).
inline TRGBColor OutfitColor(int Color)
{
	int R, G, B;
	GetOutfitColor(Color, &R, &G, &B);
	return TRGBColor(R, G, B);
}

} // namespace

// The colour buttons are laid out in rows of 19 from
// SkinProperties.OutfitDialogColorButtons; the head is the first part shown.
TOutfitDialog::TOutfitDialog(const int *Data)
{
	PATELLA_ASSERT(Data!=nullptr);
	Outfit = Data[0];
	FirstOutfit = Data[5];
	LastOutfit = Data[6];
	Limit(&Outfit, FirstOutfit, LastOutfit);
	Colors[0] = Data[1];
	Colors[1] = Data[2];
	Colors[2] = Data[3];
	Colors[3] = Data[4];
	TComponent::SetSize(SkinProperties().OutfitDialogWidth, SkinProperties().OutfitDialogHeight);
	OutfitIcon = new TOutfitIcon(Outfit, DIRECTION_SOUTH, Colors.data(), &SkinProperties().OutfitDialogIconHighlight);
	TBorder *Border = new TBorder(OutfitIcon, SKIN_NONE, SKIN_FRAME_EDGE_TOP, SKIN_NONE, SKIN_FRAME_EDGE_LEFT, SKIN_FRAME_EDGE_RIGHT, SKIN_NONE, SKIN_FRAME_EDGE_BOTTOM, SKIN_NONE, SkinProperties().OutfitDialogIconBorder);
	InsertChild(Border);
	SetChildArea(Border, SkinProperties().OutfitDialogIcon);
	TTextButton *NextButton = new TTextButton(0, 0, "Next", TCallback(this, EVENT_OUTFIT_NEXT), nullptr);
	InsertChild(NextButton);
	SetChildPosition(NextButton, SkinProperties().OutfitDialogNextButton.x, SkinProperties().OutfitDialogNextButton.y);
	PartGroup.reset(new TRadioButtonGroup);
	for (int i = 0; i < 4; i++) {
		const char *PartName = "";
		switch (i) {
		case 0:
			PartName = "Head";
			break;
		case 1:
			PartName = "Body";
			break;
		case 2:
			PartName = "Legs";
			break;
		case 3:
			PartName = "Feet";
			break;
		default:
			break;
		}
		PartButtons[i] = new TTextButton(70, i * 25, PartName, TCallback(this, i + EVENT_OUTFIT_PART), nullptr);
		InsertChild(PartButtons[i]);
		SetChildPosition(PartButtons[i], SkinProperties().OutfitDialogPartButtons[i].x, SkinProperties().OutfitDialogPartButtons[i].y);
		PartButtons[i]->SetRadioButtonGroup(PartGroup.get());
	}
	ColorGroup.reset(new TRadioButtonGroup);
	for (int i = 0; i < 133; i++) {
		ColorButtons[i] = new TColorButton(0, 0, OutfitColor(i), SKIN_COLOR_BUTTON_UP, SKIN_COLOR_BUTTON_DOWN, TCallback(this, i + EVENT_OUTFIT_COLOR), nullptr);
		ColorButtons[i]->SetRadioButtonGroup(ColorGroup.get());
		InsertChild(ColorButtons[i]);
		SetChildPosition(ColorButtons[i], SkinProperties().OutfitDialogColorButtons.x, SkinProperties().OutfitDialogColorButtons.y);
		ColorButtons[i]->SetPosition(i % 19 * SkinProperties().OutfitDialogColorColumnWidth + ColorButtons[i]->OffsetX,
			i / 19 * SkinProperties().OutfitDialogColorRowHeight + ColorButtons[i]->OffsetY);
	}
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextChooseOutfit);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().OutfitDialogLabel.x, SkinProperties().OutfitDialogLabel.y);
	PartGroup->Selected = PartButtons[0];
	HandleEvent(EVENT_OUTFIT_PART);
}

// A part button selects the colour button of its colour; the loop runs
// over 216 colours and reads past the 133 buttons (into the members
// after them) when a part has a higher colour (kept).
void TOutfitDialog::HandleEvent(int Event)
{
	if (Event >= EVENT_OUTFIT_PART && Event <= EVENT_OUTFIT_PART_LAST) {
		Part = Event - EVENT_OUTFIT_PART;
		for (int i = 0; i < 216; i++) {
			if (Colors[Part] == i)
				ColorGroup->Selected = ColorButtons[i];
		}
	} else if (Event >= EVENT_OUTFIT_COLOR && Event <= EVENT_OUTFIT_COLOR_LAST) {
		Colors[Part] = Event - EVENT_OUTFIT_COLOR;
		OutfitIcon->SetColor(Part + 1, Colors[Part]);
		OutfitIcon->Arrange();
	} else if (Event == EVENT_OUTFIT_NEXT) {
		Outfit++;
		if (Outfit > LastOutfit)
			Outfit = FirstOutfit;
		OutfitIcon->SetOutfit(Outfit);
		OutfitIcon->Arrange();
	} else if (Event == 1) {
		SendOutfit(Outfit, Colors.data());
		SetActionState(ACTIONSTATE_NONE);
	} else if (Event == EVENT_CANCEL) {
		SetActionState(ACTIONSTATE_NONE);
	}
}

// --- TTextDialog ----------------------------------------------------------------

// A read-only text keeps its scroll position (SetText's second
// argument).
TTextDialog::TTextDialog(int MaximumLength, const char *Text, uint32 _ObjectNumber, TObject _Obj,
	const char *Description)
{
	PATELLA_ASSERT(Text!=nullptr);
	ObjectNumber = _ObjectNumber;
	Obj = _Obj;
	TComponent::SetSize(SkinProperties().TextDialogWidth, SkinProperties().TextDialogHeight);
	const bool ReadOnly = !GetObjectFlag(Obj, WRITE) && !(GetObjectFlag(Obj, WRITEONCE) && strlen(Text) == 0);
	EditText = new TEditText(MaximumLength, ReadOnly);
	InsertChild(EditText);
	SetChildArea(EditText, SkinProperties().TextDialogText);
	EditText->SetActive(true);
	EditText->SetText(Text, ReadOnly);
	ObjectIcon = new TObjectIcon(Obj, &SkinProperties().TextDialogObjectHighlight, false);
	InsertChild(ObjectIcon);
	SetChildArea(ObjectIcon, SkinProperties().TextDialogObjectIcon);
	if (Description != nullptr) {
		TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, Description);
		InsertChild(Label);
		SetChildArea(Label, SkinProperties().TextDialogDescription);
	}
	Arrange();
}

void TTextDialog::HandleEvent(int Event)
{
	if (Event == 1)
		SendEditText(ObjectNumber, EditText->Text.get());
	SetActionState(ACTIONSTATE_NONE);
}

// --- TEditListDialog ----------------------------------------------------------

TEditListDialog::TEditListDialog(int MaxLength, const char *Text, int _Type, uint32 _Identifier,
	const char *Description)
{
	PATELLA_ASSERT(Text!=nullptr);
	PATELLA_ASSERT_VALUES((int)strlen(Text)<MaxLength, strlen(Text), MaxLength);
	Type = _Type;
	Identifier = _Identifier;
	TComponent::SetSize(SkinProperties().EditListDialogWidth, SkinProperties().EditListDialogHeight);
	EditText = new TEditText(MaxLength - 1, false);
	InsertChild(EditText);
	SetChildArea(EditText, SkinProperties().EditListDialogText);
	EditText->SetActive(true);
	EditText->SetText(Text, false);
	DrawingListLength = 1;
	DrawingList.reset(new TDrawing[1]);
	if (Description != nullptr) {
		TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, Description);
		InsertChild(Label);
		SetChildArea(Label, SkinProperties().EditListDialogDescription);
	}
	Arrange();
}

// The children keep their areas (TWindow::Arrange is not called).
void TEditListDialog::Arrange()
{
	DrawingList[0] = TDrawing(SKIN_EDIT_LIST_PICTURE, SkinProperties().EditListDialogPicture.x, SkinProperties().EditListDialogPicture.y, Skins()[SKIN_EDIT_LIST_PICTURE].width,
		Skins()[SKIN_EDIT_LIST_PICTURE].height, 0, 0);
}

void TEditListDialog::HandleEvent(int Event)
{
	if (Event == 1)
		SendEditHouseText(Type, Identifier, EditText->Text.get());
	SetActionState(ACTIONSTATE_NONE);
}

// --- TIgnoreListDialog --------------------------------------------------------

// Talk.cpp's lists come as one text with a name on every line; the names
// are sorted, each into an entry and a line.
TIgnoreListDialog::TIgnoreListDialog()
	: IgnoreEntries(0, 9, 10), WhiteEntries(0, 9, 10)
{
	TComponent::SetSize(SkinProperties().IgnoreListDialog.Width, SkinProperties().IgnoreListDialog.Height);
	IgnoreListCheckBox = new TCheckBox("Activate Ignore List");
	InsertChild(IgnoreListCheckBox);
	SetChildArea(IgnoreListCheckBox, SkinProperties().IgnoreListDialog.IgnoreListCheckBox);
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextCharactersToIgnore);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().IgnoreListDialog.IgnoreListLabel);
	IgnoreList = new TFontListBox(TCallback(this, EVENT_LIST_SELECTED), NoCallback);
	IgnoreList->SetEditable(true);
	InsertChild(IgnoreList);
	SetChildArea(IgnoreList, SkinProperties().IgnoreListDialog.IgnoreList);
	AddIgnoreButton = new TTextButton(SkinProperties().IgnoreListDialog.AddIgnoreButton.x, SkinProperties().IgnoreListDialog.AddIgnoreButton.y, "Add",
		SKIN_SMALL_BUTTON_UP, SKIN_SMALL_BUTTON_DOWN, TCallback(this, EVENT_IGNORE_ADD), nullptr);
	InsertChild(AddIgnoreButton);
	DeleteIgnoreButton = new TTextButton(SkinProperties().IgnoreListDialog.DeleteIgnoreButton.x, SkinProperties().IgnoreListDialog.DeleteIgnoreButton.y,
		"Delete", SKIN_SMALL_BUTTON_UP, SKIN_SMALL_BUTTON_DOWN, TCallback(this, EVENT_IGNORE_DELETE), nullptr);
	InsertChild(DeleteIgnoreButton);
	IgnorePermanently = new TCheckBox("Ignore Permanently");
	InsertChild(IgnorePermanently);
	SetChildArea(IgnorePermanently, SkinProperties().IgnoreListDialog.IgnorePermanently);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextGlobalIgnoreSettings);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().IgnoreListDialog.GlobalIgnoreLabel);
	IgnoreYelling = new TCheckBox("Ignore Yelling");
	InsertChild(IgnoreYelling);
	SetChildArea(IgnoreYelling, SkinProperties().IgnoreListDialog.IgnoreYelling);
	IgnorePrivate = new TCheckBox("Ignore Private Messages");
	InsertChild(IgnorePrivate);
	SetChildArea(IgnorePrivate, SkinProperties().IgnoreListDialog.IgnorePrivate);
	IgnoreListCheckBox->SetChecked(GetIgnoreListEnabled());
	IgnoreYelling->SetChecked(IsModeIgnored(TALK_YELL));
	IgnorePrivate->SetChecked(IsModeIgnored(TALK_PRIVATE_MESSAGE));
	char *Text = GetIgnoreListText();
	if (Text != nullptr) {
		const int Length = (int)strlen(Text);
		int AllocationNumber = 0;
		for (int i = 0; i < Length; i++) {
			if (Text[i] == '\n')
				AllocationNumber++;
		}
		char **Names = new char *[AllocationNumber];
		memset(static_cast<void *>(Names), 0, AllocationNumber * sizeof(char *));
		int Number = 0;
		bool NewLine = true;
		for (int i = 0; i < Length; i++) {
			if (Text[i] == '\n') {
				Text[i] = 0;
				NewLine = true;
			} else if (NewLine) {
				PATELLA_ASSERT_VALUES(Number < AllocationNumber, Number, AllocationNumber);
				Names[Number] = Text + i;
				Number++;
				NewLine = false;
			}
		}
		qsort(static_cast<void *>(Names), Number, sizeof(char *), CompareNames);
		for (int i = 0; i < Number; i++) {
			CopyString(patella::TSpan<char>(IgnoreEntries(i).Name.Chars).first(29), Names[i]);
			IgnoreEntries(i).Permanent = IsPermanentIgnore(Names[i]);
			TFont Font;
			if (IgnoreEntries(i).Permanent)
				Font.Color = SkinProperties().IgnoreListDialog.PermanentColor;
			IgnoreList->AddLine(Names[i], Font);
		}
		delete[] Names;
		if (Number > 0)
			IgnoreList->SetSelection(0);
	}
	if (Text != nullptr)
		delete[] Text;
	WhiteListCheckBox = new TCheckBox("Activate White List");
	InsertChild(WhiteListCheckBox);
	SetChildArea(WhiteListCheckBox, SkinProperties().IgnoreListDialog.WhiteListCheckBox);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextCharactersToAllow);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().IgnoreListDialog.WhiteListLabel);
	WhiteList = new TFontListBox(TCallback(this, EVENT_LIST_SELECTED), NoCallback);
	WhiteList->SetEditable(true);
	InsertChild(WhiteList);
	SetChildArea(WhiteList, SkinProperties().IgnoreListDialog.WhiteList);
	AddWhiteButton = new TTextButton(SkinProperties().IgnoreListDialog.AddWhiteButton.x, SkinProperties().IgnoreListDialog.AddWhiteButton.y, "Add",
		SKIN_SMALL_BUTTON_UP, SKIN_SMALL_BUTTON_DOWN, TCallback(this, EVENT_WHITELIST_ADD), nullptr);
	InsertChild(AddWhiteButton);
	DeleteWhiteButton = new TTextButton(SkinProperties().IgnoreListDialog.DeleteWhiteButton.x, SkinProperties().IgnoreListDialog.DeleteWhiteButton.y,
		"Delete", SKIN_SMALL_BUTTON_UP, SKIN_SMALL_BUTTON_DOWN, TCallback(this, EVENT_WHITELIST_DELETE), nullptr);
	InsertChild(DeleteWhiteButton);
	AllowPermanently = new TCheckBox("Allow Permanently");
	InsertChild(AllowPermanently);
	SetChildArea(AllowPermanently, SkinProperties().IgnoreListDialog.AllowPermanently);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextGlobalAllowSettings);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().IgnoreListDialog.GlobalAllowLabel);
	AllowBuddies = new TCheckBox("Allow VIPs to message you");
	InsertChild(AllowBuddies);
	SetChildArea(AllowBuddies, SkinProperties().IgnoreListDialog.AllowBuddies);
	WhiteListCheckBox->SetChecked(GetIgnoreEnabled());
	AllowBuddies->SetChecked(GetBuddyIgnore());
	Text = GetBuddyIgnoreText();
	if (Text != nullptr) {
		const int Length = (int)strlen(Text);
		int AllocationNumber = 0;
		for (int i = 0; i < Length; i++) {
			if (Text[i] == '\n')
				AllocationNumber++;
		}
		char **Names = new char *[AllocationNumber];
		memset(static_cast<void *>(Names), 0, AllocationNumber * sizeof(char *));
		int Number = 0;
		bool NewLine = true;
		for (int i = 0; i < Length; i++) {
			if (Text[i] == '\n') {
				Text[i] = 0;
				NewLine = true;
			} else if (NewLine) {
				PATELLA_ASSERT_VALUES(Number < AllocationNumber, Number, AllocationNumber);
				Names[Number] = Text + i;
				Number++;
				NewLine = false;
			}
		}
		qsort(static_cast<void *>(Names), Number, sizeof(char *), CompareNames);
		for (int i = 0; i < Number; i++) {
			CopyString(patella::TSpan<char>(WhiteEntries(i).Name.Chars).first(29), Names[i]);
			WhiteEntries(i).Permanent = IsPermanentBuddyIgnore(Names[i]);
			TFont Font;
			if (WhiteEntries(i).Permanent)
				Font.Color = SkinProperties().IgnoreListDialog.PermanentColor;
			WhiteList->AddLine(Names[i], Font);
		}
		delete[] Names;
		if (Number > 0)
			WhiteList->SetSelection(0);
	}
	if (Text != nullptr)
		delete[] Text;
	Arrange();
	HandleEvent(EVENT_LIST_SELECTED);
}

void TIgnoreListDialog::Arrange()
{
	TWindow::Arrange();
}

// OK (602) replaces Talk.cpp's lists with the lines of the list boxes
// (except empty lines and "new entry") and sets the global switches; OK
// then closes the dialog like every event not handled below. Add (709, 712)
// keeps the line being edited in its entry and appends "new entry" for
// editing; Delete (710, 713) removes the selected entry and rebuilds the
// list from the entries (the loops stop one short of the vector's last
// index, so they never see the entry moved last). A selection (11) shows
// the selected entries' permanent flags; it computes each entry once more
// without using it (kept).
void TIgnoreListDialog::HandleEvent(int Event)
{
	if (Event == EVENT_IGNORE_LIST_OK) {
		IgnoreList->StopEdit();
		ClearIgnoreList();
		for (int i = 0; i < IgnoreList->GetNumberOfLines(); i++) {
			TFont Font;
			const char *Name = IgnoreList->GetLine(i, &Font);
			if (strlen(Name) != 0 && strcmp(Name, "new entry") != 0)
				IgnoreName(Name, IgnoreEntries(i).Permanent);
		}
		SetIgnoreListEnabled(IgnoreListCheckBox->IsChecked());
		if (IgnoreYelling->IsChecked())
			IgnoreMode(TALK_YELL);
		else
			UnignoreMode(TALK_YELL);
		if (IgnorePrivate->IsChecked())
			IgnoreMode(TALK_PRIVATE_MESSAGE);
		else
			UnignoreMode(TALK_PRIVATE_MESSAGE);
		WhiteList->StopEdit();
		ClearBuddyIgnore();
		for (int i = 0; i < WhiteList->GetNumberOfLines(); i++) {
			TFont Font;
			const char *Name = WhiteList->GetLine(i, &Font);
			if (strlen(Name) != 0 && strcmp(Name, "new entry") != 0)
				AddBuddyIgnore(Name, WhiteEntries(i).Permanent);
		}
		SetIgnoreEnabled(WhiteListCheckBox->IsChecked());
		SetBuddyIgnore(AllowBuddies->IsChecked());
	}
	if (Event == EVENT_IGNORE_ADD) {
		WhiteList->StopEdit();
		IgnoreList->StopEdit();
		const int Selection = IgnoreList->GetSelection();
		if (Selection > -1) {
			TFont Font;
			const char *Name = IgnoreList->GetLine(Selection, &Font);
			IgnoreEntries(Selection).AssignName(Name);
			IgnoreEntries(Selection).Permanent = IgnorePermanently->IsChecked();
		}
		const TFont Font;
		TIgnoreEntry Entry;
		CopyString(Entry.Name.Chars, "new entry");
		IgnoreEntries(IgnoreList->GetNumberOfLines()) = Entry;
		IgnoreList->AddLine("new entry", Font);
		IgnoreList->SetSelection(IgnoreList->GetNumberOfLines() - 1);
		IgnorePermanently->SetChecked(false);
		IgnoreList->StartEdit();
	} else if (Event == EVENT_WHITELIST_ADD) {
		IgnoreList->StopEdit();
		WhiteList->StopEdit();
		const int Selection = WhiteList->GetSelection();
		if (Selection > -1) {
			TFont Font;
			const char *Name = WhiteList->GetLine(Selection, &Font);
			WhiteEntries(Selection).AssignName(Name);
			WhiteEntries(Selection).Permanent = AllowPermanently->IsChecked();
		}
		const TFont Font;
		TIgnoreEntry Entry;
		CopyString(Entry.Name.Chars, "new entry");
		WhiteEntries(WhiteList->GetNumberOfLines()) = Entry;
		WhiteList->AddLine("new entry", Font);
		WhiteList->SetSelection(WhiteList->GetNumberOfLines() - 1);
		AllowPermanently->SetChecked(false);
		WhiteList->StartEdit();
	} else if (Event == EVENT_IGNORE_DELETE) {
		IgnoreList->StopEdit();
		const int Selection = IgnoreList->GetSelection();
		if (Selection > -1) {
			TFont Font;
			const char *Name = IgnoreList->GetLine(Selection, &Font);
			IgnoreEntries(Selection).AssignName(Name);
			IgnoreEntries(Selection).Permanent = IgnorePermanently->IsChecked();
			IgnoreList->Clear();
			int i;
			for (i = 0; i < IgnoreEntries.max - IgnoreEntries.min; i++) {
				if (i >= Selection)
					IgnoreEntries(i) = IgnoreEntries(i + 1);
			}
			IgnoreEntries(i).Name[0] = 0;
			for (i = 0; i < IgnoreEntries.max - IgnoreEntries.min; i++) {
				if (IgnoreEntries(i).Name.Length() != 0) {
					TFont LineFont;
					if (IgnoreEntries(i).Permanent)
						LineFont.Color = SkinProperties().IgnoreListDialog.PermanentColor;
					IgnoreList->AddLine(IgnoreEntries(i).NameText(), LineFont);
				}
			}
		}
		IgnoreList->SetSelection(-1);
	} else if (Event == EVENT_WHITELIST_DELETE) {
		WhiteList->StopEdit();
		const int Selection = WhiteList->GetSelection();
		if (Selection > -1) {
			TFont Font;
			const char *Name = WhiteList->GetLine(Selection, &Font);
			WhiteEntries(Selection).AssignName(Name);
			WhiteEntries(Selection).Permanent = AllowPermanently->IsChecked();
			WhiteList->Clear();
			int i;
			for (i = 0; i < WhiteEntries.max - WhiteEntries.min; i++) {
				if (i >= Selection)
					WhiteEntries(i) = WhiteEntries(i + 1);
			}
			WhiteEntries(i).Name[0] = 0;
			for (i = 0; i < WhiteEntries.max - WhiteEntries.min; i++) {
				if (WhiteEntries(i).Name.Length() != 0) {
					TFont LineFont;
					if (WhiteEntries(i).Permanent)
						LineFont.Color = SkinProperties().IgnoreListDialog.PermanentColor;
					WhiteList->AddLine(WhiteEntries(i).NameText(), LineFont);
				}
			}
		}
		WhiteList->SetSelection(-1);
	} else if (Event == EVENT_LIST_SELECTED) {
		int Selection = IgnoreList->GetSelection();
		if (Selection != -1) {
			IgnoreEntries(Selection);
			IgnorePermanently->SetChecked(IgnoreEntries(Selection).Permanent);
		}
		Selection = WhiteList->GetSelection();
		if (Selection != -1) {
			WhiteEntries(Selection);
			AllowPermanently->SetChecked(WhiteEntries(Selection).Permanent);
		}
	} else {
		SetActionState(ACTIONSTATE_NONE);
	}
}

// A click that changes a permanent check box recolours the selected line
// and stores the flag in its entry.
bool TIgnoreListDialog::LeftButtonUp(int x, int y, int Flags)
{
	const bool Result = TWindow::LeftButtonUp(x, y, Flags);
	int Selection = IgnoreList->GetSelection();
	if (Result && Selection > -1 && IgnoreEntries(Selection).Permanent != IgnorePermanently->IsChecked()) {
		TFont Font;
		IgnoreList->StopEdit();
		TFixedString<30> Name;
		CopyString(Name.Chars, IgnoreList->GetLine(Selection, &Font));
		TFont LineFont;
		if (IgnorePermanently->IsChecked())
			LineFont.Color = SkinProperties().IgnoreListDialog.PermanentColor;
		IgnoreList->SetLine(Selection, Name, LineFont);
		TIgnoreEntry Entry;
		Entry.Permanent = IgnorePermanently->IsChecked();
		CopyString(Entry.Name.Chars, Name);
		IgnoreEntries(Selection) = Entry;
	}
	Selection = WhiteList->GetSelection();
	if (Result && Selection > -1 && WhiteEntries(Selection).Permanent != AllowPermanently->IsChecked()) {
		TFont Font;
		WhiteList->StopEdit();
		TFixedString<30> Name;
		CopyString(Name.Chars, WhiteList->GetLine(Selection, &Font));
		TFont LineFont;
		if (AllowPermanently->IsChecked())
			LineFont.Color = SkinProperties().IgnoreListDialog.PermanentColor;
		WhiteList->SetLine(Selection, Name, LineFont);
		TIgnoreEntry Entry;
		Entry.Permanent = AllowPermanently->IsChecked();
		CopyString(Entry.Name.Chars, Name);
		WhiteEntries(Selection) = Entry;
	}
	return Result;
}

void TIgnoreListDialog::KeyPress(int Key)
{
	IgnoreList->KeyPress(Key);
	WhiteList->KeyPress(Key);
}

// --- TBugReportDialog ------------------------------------------------------------

TBugReportDialog::TBugReportDialog()
{
	TComponent::SetSize(SkinProperties().BugReportDialogWidth, SkinProperties().BugReportDialogHeight);
	EditText = new TEditText(1024, false);
	InsertChild(EditText);
	SetChildArea(EditText, SkinProperties().BugReportDialogText);
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextReportBugsOnly);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().BugReportDialogLabel);
	EditText->SetActive(true);
	Arrange();
}

void TBugReportDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		const char *Text = EditText->Text.get();
		if (strlen(Text) != 0)
			SendBugReport(Text);
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TNewChannelDialog -----------------------------------------------------------

TNewChannelDialog::TNewChannelDialog(int _NumberOfChannels, const int *_ChannelIDs, const char (*ChannelNames)[30]) // NOLINT(modernize-avoid-c-arrays): Dialogs.h
{
	PATELLA_ASSERT(_ChannelIDs!=nullptr);
	PATELLA_ASSERT_VALUES(_NumberOfChannels>0, _NumberOfChannels);
	TComponent::SetSize(SkinProperties().NewChannelDialogWidth, SkinProperties().NewChannelDialogHeight);
	ListBox = new TListBox(NoCallback, TCallback(this, EVENT_NEW_CHANNEL_JOIN));
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextSelectChannel);
	InsertChild(ListBox);
	InsertChild(Label);
	SetChildArea(ListBox, SkinProperties().NewChannelDialogList);
	SetChildArea(Label, SkinProperties().NewChannelDialogListLabel);
	NameEdit = new TEditLine(29, false);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextOpenPrivateChannel);
	InsertChild(NameEdit);
	InsertChild(Label);
	SetChildArea(NameEdit, SkinProperties().NewChannelDialogName);
	SetChildArea(Label, SkinProperties().NewChannelDialogNameLabel);
	NumberOfChannels = _NumberOfChannels;
	ChannelIDs.reset(new int[_NumberOfChannels]);
	for (int i = 0; i < NumberOfChannels; i++) {
		ChannelIDs[i] = _ChannelIDs[i];
		ListBox->AddLine(ChannelNames[i]);
	}
	NameEdit->SetActive(true);
	Arrange();
}

// Return sends 301 (join the selected channel) without a name and 302
// (nothing but closing) with one; the frame's OK button takes Return
// first.
void TNewChannelDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN) {
		if (strlen(NameEdit->Text.get()) != 0)
			HandleEvent(EVENT_NEW_CHANNEL_CLOSE);
		else
			HandleEvent(EVENT_NEW_CHANNEL_JOIN);
	} else if (Key == KEY_ESCAPE) {
		HandleEvent(EVENT_CANCEL);
	} else {
		NameEdit->KeyPress(Key);
	}
}

// OK (1) with a name opens a private conversation (not with
// oneself), without one it joins the selected channel like a double
// click (301).
void TNewChannelDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		const char *Name = NameEdit->Text.get();
		if (strlen(Name) != 0) {
			TCreature *Creature = GetCreature(GetPlayerID());
			PATELLA_ASSERT(Creature!=nullptr);
			if (CompareStrings(Creature->Name, Name, -1) == 0)
				SetStatusMessage(TextNoPrivateChannelWithYourself, false);
			else
				SendOpenPrivateConversation(Name);
		} else {
			Event = EVENT_NEW_CHANNEL_JOIN;
		}
	}
	if (Event == EVENT_NEW_CHANNEL_JOIN) {
		const int Selection = ListBox->GetSelection();
		if (Selection != -1) {
			const int ChannelID = ChannelIDs[Selection];
			if (ChannelID == CHANNEL_NEW_PRIVATE)
				SendOpenPrivateChannel();
			else
				SendJoinChannel(ChannelID);
		}
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TPrivateChannelDialog -------------------------------------------------------

TPrivateChannelDialog::TPrivateChannelDialog(bool _Exclude)
{
	Exclude = _Exclude;
	TComponent::SetSize(SkinProperties().PrivateChannelDialogWidth, SkinProperties().PrivateChannelDialogHeight);
	NameEdit = new TEditLine(29, false);
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextEnterCharacterName);
	InsertChild(NameEdit);
	InsertChild(Label);
	SetChildArea(NameEdit, SkinProperties().PrivateChannelDialogName);
	SetChildArea(Label, SkinProperties().PrivateChannelDialogLabel);
	NameEdit->SetActive(true);
	Arrange();
}

void TPrivateChannelDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN)
		HandleEvent(1);
	else if (Key == KEY_ESCAPE)
		HandleEvent(EVENT_CANCEL);
	else
		NameEdit->KeyPress(Key);
}

void TPrivateChannelDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		const char *Name = NameEdit->Text.get();
		if (strlen(Name) != 0) {
			if (!Exclude)
				SendInviteToPrivateChannel(Name);
			else
				SendExcludeFromPrivateChannel(Name);
		}
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TReportDialog ----------------------------------------------------------------

TReportDialog::TReportDialog()
{
	TComponent::SetSize(SkinProperties().ReportDialogWidth, SkinProperties().ReportDialogHeight);
	TextEdit = new TEditLine(255, false);
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextStateRuleViolation);
	InsertChild(TextEdit);
	InsertChild(Label);
	SetChildArea(TextEdit, SkinProperties().ReportDialogText);
	SetChildArea(Label, SkinProperties().ReportDialogLabel);
	TextEdit->SetActive(true);
	Arrange();
}

void TReportDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN)
		HandleEvent(1);
	else if (Key == KEY_ESCAPE)
		HandleEvent(EVENT_CANCEL);
	else
		TextEdit->KeyPress(Key);
}

// The request goes to Control.cpp's event queue as a copy (event 0x33).
void TReportDialog::HandleEvent(int Event)
{
	if (Event == 1 && strlen(TextEdit->Text.get()) != 0) {
		const std::size_t Size = strlen(TextEdit->Text.get()) + 1;
		char *Text = new char[Size];
		TStringBuffer(Text, Size).Assign(TextEdit->Text.get());
		AddMessage(MESSAGE_REPORT_RULE_VIOLATION, reinterpret_cast<std::intptr_t>(Text), 0);
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TAddBuddyDialog ------------------------------------------------------------

TAddBuddyDialog::TAddBuddyDialog()
{
	TComponent::SetSize(SkinProperties().AddBuddyDialogWidth, SkinProperties().AddBuddyDialogHeight);
	NameEdit = new TEditLine(29, false);
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextEnterCharacterName2);
	InsertChild(NameEdit);
	InsertChild(Label);
	SetChildArea(NameEdit, SkinProperties().AddBuddyDialogName);
	SetChildArea(Label, SkinProperties().AddBuddyDialogLabel);
	NameEdit->SetActive(true);
	Arrange();
}

void TAddBuddyDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN)
		HandleEvent(1);
	else if (Key == KEY_ESCAPE)
		HandleEvent(EVENT_CANCEL);
	else
		NameEdit->KeyPress(Key);
}

void TAddBuddyDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		const char *Name = NameEdit->Text.get();
		if (strlen(Name) != 0)
			SendAddBuddy(Name);
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TEditBuddyDialog -----------------------------------------------------------

// The icons are skins 218-227 after "none".
TEditBuddyDialog::TEditBuddyDialog(int _CharacterID)
{
	CharacterID = _CharacterID;
	const int Position = GetBuddyPosition(_CharacterID);
	PATELLA_ASSERT_VALUES(Position >= 0, Position, CharacterID);
	TComponent::SetSize(SkinProperties().EditBuddyDialog.Width, SkinProperties().EditBuddyDialog.Height);
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, GetBuddyListName(Position));
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().EditBuddyDialog.Name);
	TSeparator *Separator = new TSeparator(true);
	InsertChild(Separator);
	SetChildArea(Separator, SkinProperties().EditBuddyDialog.Separator);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextSelectType);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().EditBuddyDialog.TypeLabel);
	IconGroup.reset(new TRadioButtonGroup);
	for (int i = 0; i < 11; i++) {
		int Icon;
		if (i == 0)
			Icon = SKIN_NONE;
		else
			Icon = i + SKIN_BUDDY_ICON_BASE;
		IconButtons[i] = new TIconButton(0, 0, Icon, SKIN_ICON_BUTTON_UP, SKIN_ICON_BUTTON_DOWN, NoCallback, nullptr);
		InsertChild(IconButtons[i]);
		IconButtons[i]->SetRadioButtonGroup(IconGroup.get());
		if (GetBuddyListIcon(Position) == i)
			IconGroup->Selected = IconButtons[i];
		SetChildPosition(IconButtons[i], SkinProperties().EditBuddyDialog.IconButtons[i].x, SkinProperties().EditBuddyDialog.IconButtons[i].y);
	}
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextEnterShortDescription);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().EditBuddyDialog.DescriptionLabel);
	Description = new TEditText(127, false);
	InsertChild(Description);
	SetChildArea(Description, SkinProperties().EditBuddyDialog.Description);
	if (GetBuddyListDescription(Position) != nullptr)
		Description->SetText(GetBuddyListDescription(Position), false);
	Notify = new TCheckBox("Notify at login");
	InsertChild(Notify);
	SetChildArea(Notify, SkinProperties().EditBuddyDialog.Notify);
	Notify->SetChecked(GetBuddyListNotify(Position));
	Description->SetActive(true);
	Arrange();
}

// Without a selected icon the icon is 0.
void TEditBuddyDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		int Icon = 0;
		for (int i = 0; i < 11; i++) {
			if (IconGroup->Selected == IconButtons[i]) {
				Icon = i;
				break;
			}
		}
		EditBuddy(CharacterID, Icon, Description->Text.get(), Notify->IsChecked());
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TUpdateDialog -----------------------------------------------------------------

// TUpdateDialog ("Updating ... Please wait": the progress of a patch from the login server) is gone with the patcher
// since Patella-Modern issue #75 (decision D9).

// --- THelpDialog -------------------------------------------------------------------

// No Arrange at the end.
THelpDialog::THelpDialog(void (*_OpenURL)(const char *URL))
{
	OpenURL = _OpenURL;
	TComponent::SetSize(SkinProperties().HelpDialog.Width, SkinProperties().HelpDialog.Height);
	TTextButton *Button = new TTextButton(0, 0, "Client Help", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_HELP_MODE), nullptr);
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextClientInfo);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().HelpDialog.ClientHelpButton.x, SkinProperties().HelpDialog.ClientHelpButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().HelpDialog.ClientHelpLabel.x, SkinProperties().HelpDialog.ClientHelpLabel.y);
	Button = new TTextButton(0, 0, "Hints", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_HINTS), nullptr);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextShowPreviousHints);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().HelpDialog.HintsButton.x, SkinProperties().HelpDialog.HintsButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().HelpDialog.HintsLabel.x, SkinProperties().HelpDialog.HintsLabel.y);
	Button = new TTextButton(0, 0, "Tutor Help", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_TUTOR_HELP), nullptr);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextAskTutor);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().HelpDialog.TutorHelpButton.x, SkinProperties().HelpDialog.TutorHelpButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().HelpDialog.TutorHelpLabel.x, SkinProperties().HelpDialog.TutorHelpLabel.y);
	Button = new TTextButton(0, 0, "Rule Violation", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_REPORT), nullptr);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextReportRuleViolations);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().HelpDialog.RuleViolationButton.x, SkinProperties().HelpDialog.RuleViolationButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().HelpDialog.RuleViolationLabel.x, SkinProperties().HelpDialog.RuleViolationLabel.y);
	Button = new TTextButton(0, 0, "Manual", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_MANUAL), nullptr);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextReadManual);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().HelpDialog.ManualButton.x, SkinProperties().HelpDialog.ManualButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().HelpDialog.ManualLabel.x, SkinProperties().HelpDialog.ManualLabel.y);
	Button = new TTextButton(0, 0, "FAQ", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_FAQ), nullptr);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextReadFAQ);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().HelpDialog.FAQButton.x, SkinProperties().HelpDialog.FAQButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().HelpDialog.FAQLabel.x, SkinProperties().HelpDialog.FAQLabel.y);
}

void THelpDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN)
		HandleEvent(1);
	else if (Key == KEY_ESCAPE)
		HandleEvent(EVENT_CANCEL);
}

// Client Help (702) closes first and then asks for the client info; Hints
// (703) opens the hints dialog (0x1f) after closing the help menu (0x20).
void THelpDialog::HandleEvent(int Event)
{
	switch (Event) {
	case 1:
	case EVENT_CANCEL:
		SetActionState(ACTIONSTATE_NONE);
		break;
	case EVENT_HELP_MODE:
		SetActionState(ACTIONSTATE_NONE);
		AddMessage(MESSAGE_HELP_MODE, 0, 0);
		break;
	case EVENT_HINTS:
		AddMessage(MESSAGE_HINTS_DIALOG, 0, 0);
		AddMessage(MESSAGE_HELP_DIALOG, 0, 0);
		SetActionState(ACTIONSTATE_NONE);
		break;
	case EVENT_TUTOR_HELP:
		SendJoinChannel(7);
		SetActionState(ACTIONSTATE_NONE);
		break;
	case EVENT_REPORT:
		AddMessage(MESSAGE_REPORT_DIALOG, 0, 0);
		SetActionState(ACTIONSTATE_NONE);
		break;
	case EVENT_MANUAL:
		OpenURL(TextManualURL);
		SetActionState(ACTIONSTATE_NONE);
		break;
	case EVENT_FAQ:
		OpenURL(TextFAQURL);
		SetActionState(ACTIONSTATE_NONE);
		break;
	default:
		TERROR1("unknown event", Event);
	}
}

// --- THintsDialog ------------------------------------------------------------------

// The hints 1-14 the player has seen.
THintsDialog::THintsDialog()
{
	TComponent::SetSize(SkinProperties().HintsDialogWidth, SkinProperties().HintsDialogHeight);
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextAvailableHints);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().HintsDialogLabel);
	ListBox = new TListBox(NoCallback, TCallback(this, 1));
	InsertChild(ListBox);
	SetChildArea(ListBox, SkinProperties().HintsDialogList);
	for (int i = HINT_FIRST; i <= HINT_LAST; i++) {
		if (IsKnownHint(i))
			ListBox->AddLine(GetEventHelpTitle(i));
	}
}

void THintsDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN)
		HandleEvent(1);
	else if (Key == KEY_ESCAPE)
		HandleEvent(EVENT_CANCEL);
	else
		ListBox->KeyPress(Key);
}

// The selected hint is shown again: the help menu (0x20) and the hint
// (2) are put first in the event queue.
void THintsDialog::HandleEvent(int Event)
{
	if (Event == 1 && ListBox->GetSelection() != -1) {
		const char *Title = ListBox->GetLine(ListBox->GetSelection());
		int Number = -1;
		for (int i = HINT_FIRST; i <= HINT_LAST; i++) {
			if (CompareStrings(Title, GetEventHelpTitle(i), -1) == 0) {
				Number = i;
				break;
			}
		}
		PATELLA_ASSERT(Number!=-1);
		const char *HelpText = GetEventHelpText(Number);
		const std::size_t Size = strlen(HelpText) + 1;
		char *Text = new char[Size];
		TStringBuffer(Text, Size).Assign(HelpText);
		AddMessageFirst(MESSAGE_HINTS_DIALOG, 0, 0);
		AddMessageFirst(MESSAGE_FORMATTED_DIALOG, reinterpret_cast<std::intptr_t>(GetEventHelpTitle(Number)), reinterpret_cast<std::intptr_t>(Text));
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TRuleViolationDialog --------------------------------------------------------

// A given name (or the statement's author) cannot be changed; the
// statement line is read-only and as long as its text.
TRuleViolationDialog::TRuleViolationDialog(const char *Name, TTalkLine *Statement)
{
	for (int i = 0; i < 50; i++)
		Reasons[i] = -1;
	StatementID = Statement != nullptr ? Statement->StatementID : 0;
	TComponent::SetSize(SkinProperties().RuleViolationDialog.Width, SkinProperties().RuleViolationDialog.Height);
	FocusGroup.reset(new TFocusGroup());
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, "Name");
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().RuleViolationDialog.NameLabel);
	NameEdit = new TEditLine(29, Name != nullptr || Statement != nullptr);
	if (Name != nullptr)
		NameEdit->SetText(Name);
	if (Statement != nullptr)
		NameEdit->SetText(Statement->Addressee);
	NameEdit->SetFocusGroup(FocusGroup.get());
	InsertChild(NameEdit);
	SetChildArea(NameEdit, SkinProperties().RuleViolationDialog.Name);
	Label = new TLabel(SkinProperties().Dialog.TextFont, "Reason");
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().RuleViolationDialog.ReasonLabel);
	ReasonList = new TListBox(TCallback(this, EVENT_REASON_SELECTED), NoCallback);
	InsertChild(ReasonList);
	SetChildArea(ReasonList, SkinProperties().RuleViolationDialog.ReasonList);
	Label = new TLabel(SkinProperties().Dialog.TextFont, "Action");
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().RuleViolationDialog.ActionLabel);
	ActionList = new TListBox(NoCallback, NoCallback);
	InsertChild(ActionList);
	SetChildArea(ActionList, SkinProperties().RuleViolationDialog.ActionList);
	Label = new TLabel(SkinProperties().Dialog.TextFont, "Comment");
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().RuleViolationDialog.CommentLabel);
	CommentEdit = new TEditLine(100, false);
	CommentEdit->SetFocusGroup(FocusGroup.get());
	InsertChild(CommentEdit);
	SetChildArea(CommentEdit, SkinProperties().RuleViolationDialog.Comment);
	Label = new TLabel(SkinProperties().Dialog.TextFont, "Statement");
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().RuleViolationDialog.StatementLabel);
	StatementEdit = new TEditLine(Statement != nullptr ? (int)strlen(Statement->Text) : 0, true);
	if (Statement != nullptr)
		StatementEdit->SetText(Statement->Text);
	StatementEdit->SetFocusGroup(FocusGroup.get());
	InsertChild(StatementEdit);
	SetChildArea(StatementEdit, SkinProperties().RuleViolationDialog.Statement);
	IPBanishment = new TCheckBox("IP Address Banishment");
	IPBanishment->SetChecked(false);
	IPBanishment->SetEnabled(false);
	InsertChild(IPBanishment);
	SetChildArea(IPBanishment, SkinProperties().RuleViolationDialog.IPBanishment);
	int Number = 0;
	for (int i = 0; i < 32; i++) {
		if (CheckRight(i, -1)) {
			ReasonList->AddLine(GetRightName(i));
			Reasons[Number] = i;
			Number++;
		}
	}
	UpdateActions();
	FocusGroup->SetActive(NameEdit->ReadOnly ? CommentEdit : NameEdit);
	Arrange();
}

void TRuleViolationDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN) {
		HandleEvent(1);
	} else if (Key == KEY_ESCAPE) {
		HandleEvent(EVENT_CANCEL);
	} else if (FocusGroup->Active != nullptr) {
		FocusGroup->Active->KeyPress(Key);
	}
}

// Every missing part gets a status message; the dialog closes in any case.
// A disabled statement line sends statement 0.
void TRuleViolationDialog::HandleEvent(int Event)
{
	switch (Event) {
	case 1: {
		bool Valid = true;
		const int Reason = GetReason();
		const int Action = GetAction();
		if (strlen(NameEdit->Text.get()) == 0) {
			SetStatusMessage(TextNoPlayerName, false);
			Valid = false;
		}
		if (Reason == -1) {
			SetStatusMessage(TextNoReason, false);
			Valid = false;
		}
		if (Action == -1) {
			SetStatusMessage(TextNoAction, false);
			Valid = false;
		}
		if (strlen(CommentEdit->Text.get()) == 0) {
			SetStatusMessage(TextNoComment, false);
			Valid = false;
		}
		if (Action == 6) {
			if (strlen(StatementEdit->Text.get()) == 0) {
				SetStatusMessage(TextNoStatement, false);
				Valid = false;
			} else if (StatementID == 0) {
				SetStatusMessage(TextCannotReportStatement, false);
				Valid = false;
			}
		}
		if (Valid) {
			const uint32 ID = StatementEdit->IsDisabled() ? 0 : StatementID;
			SendRuleViolationReport(NameEdit->Text.get(), Reason, Action, CommentEdit->Text.get(), ID,
				IPBanishment->IsChecked());
		}
		break;
	}
	case EVENT_CANCEL:
		break;
	case EVENT_REASON_SELECTED:
		UpdateActions();
		return;
	default:
		TERROR1("unknown event", Event);
	}
	SetActionState(ACTIONSTATE_NONE);
}

// The actions the reason allows; the statement line is enabled for reasons
// with action 0, 2 or 6 among the statement reasons 9-27 and 29.
void TRuleViolationDialog::UpdateActions()
{
	const int Reason = GetReason();
	if (Reason != -1) {
		ActionList->Clear();
		for (int i = 0; i < 7; i++) {
			if (CheckRight(Reason, i))
				ActionList->AddLine(GetActionName(i));
		}
		const bool IPBanishmentAllowed = CheckRight(Reason, 7);
		IPBanishment->SetChecked(false);
		IPBanishment->SetEnabled(IPBanishmentAllowed);
		StatementEdit->SetDisabled(!(CheckRight(Reason, 0) || CheckRight(Reason, 2) || CheckRight(Reason, 6))
			|| ((Reason < 9 || Reason > 27) && Reason != 29));
	}
}

int TRuleViolationDialog::GetReason()
{
	const int Selection = ReasonList->GetSelection();
	if (Selection == -1)
		return -1;
	return Reasons[Selection];
}

// The action of the selected line, by its name.
int TRuleViolationDialog::GetAction() const
{
	const int Selection = ActionList->GetSelection();
	if (Selection == -1)
		return -1;
	for (int i = 0; i < 7; i++) {
		if (strcmp(ActionList->GetLine(Selection), GetActionName(i)) == 0)
			return i;
	}
	return -1;
}

// --- TEditMarkDialog ------------------------------------------------------------

// The symbols are skins 74-87.
TEditMarkDialog::TEditMarkDialog(TCoordinate *_Coordinate, TMark *_Mark)
{
	Coordinate.x = 0;
	Coordinate.y = 0;
	Coordinate.z = 0;
	TComponent::SetSize(SkinProperties().EditMarkDialogWidth, SkinProperties().EditMarkDialogHeight);
	PATELLA_ASSERT(_Coordinate != nullptr);
	Coordinate = *_Coordinate;
	if (_Mark != nullptr)
		Mark = *_Mark;
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextEnterShortDescription2);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().EditMarkDialogDescriptionLabel);
	TextEdit = new TEditLine(100, false);
	if (Mark.Text != nullptr)
		TextEdit->SetText(Mark.Text);
	InsertChild(TextEdit);
	SetChildArea(TextEdit, SkinProperties().EditMarkDialogDescription);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextSelectType2);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().EditMarkDialogTypeLabel);
	TypeGroup.reset(new TRadioButtonGroup);
	for (int i = 0; i < 14; i++) {
		TypeButtons[i] = new TIconButton(0, 0, i + SKIN_MARK_0, SKIN_ICON_BUTTON_UP, SKIN_ICON_BUTTON_DOWN, NoCallback, nullptr);
		InsertChild(TypeButtons[i]);
		TypeButtons[i]->SetRadioButtonGroup(TypeGroup.get());
		if (Mark.Type == i)
			TypeGroup->Selected = TypeButtons[i];
		SetChildPosition(TypeButtons[i], SkinProperties().EditMarkDialogTypeButtons[i].x, SkinProperties().EditMarkDialogTypeButtons[i].y);
	}
	TextEdit->SetActive(true);
	Arrange();
	delete _Coordinate;
	if (_Mark != nullptr)
		delete _Mark;
}

void TEditMarkDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN)
		HandleEvent(1);
	else if (Key == KEY_ESCAPE)
		HandleEvent(EVENT_CANCEL);
	else
		TextEdit->KeyPress(Key);
}

// An edited mark is deleted and added again at its own position.
void TEditMarkDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		CopyString(Mark.Text.Chars, TextEdit->Text.get());
		int Type = 0;
		for (int i = 0; i < 14; i++) {
			if (TypeGroup->Selected == TypeButtons[i]) {
				Type = i;
				break;
			}
		}
		if (Mark.x != 0 && Mark.y != 0) {
			DeleteAutomapMark(Mark, Coordinate.z);
			Coordinate.x = Mark.x;
			Coordinate.y = Mark.y;
		}
		AddAutomapMark(Coordinate, Mark.Text, Type);
	} else if (Event != EVENT_CANCEL) {
		TERROR1("unknown event", Event);
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TOptionsDialog -----------------------------------------------------------------

// The labels of Network and Motd use the label constructor with a
// position.
TOptionsDialog::TOptionsDialog()
{
	TComponent::SetSize(SkinProperties().OptionsDialog.Width, SkinProperties().OptionsDialog.Height);
	TTextButton *Button = new TTextButton(0, 0, "General", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_OPTIONS_GENERAL), nullptr);
	TLabel *Label = new TLabel(SkinProperties().Dialog.TextFont, TextChangeGeneralOptions);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().OptionsDialog.GeneralButton.x, SkinProperties().OptionsDialog.GeneralButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().OptionsDialog.GeneralLabel.x, SkinProperties().OptionsDialog.GeneralLabel.y);
	Button = new TTextButton(0, 0, "Graphics", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_OPTIONS_GRAPHICS), nullptr);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextChangeGraphicsOptions);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().OptionsDialog.GraphicsButton.x, SkinProperties().OptionsDialog.GraphicsButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().OptionsDialog.GraphicsLabel.x, SkinProperties().OptionsDialog.GraphicsLabel.y);
	Button = new TTextButton(0, 0, "Console", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_OPTIONS_CONSOLE), nullptr);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextCustomiseConsole);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().OptionsDialog.ConsoleButton.x, SkinProperties().OptionsDialog.ConsoleButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().OptionsDialog.ConsoleLabel.x, SkinProperties().OptionsDialog.ConsoleLabel.y);
	Button = new TTextButton(0, 0, "Network", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_OPTIONS_NETWORK), nullptr);
	Label = new TLabel(0, 0, SkinProperties().Dialog.TextFont, TextChangeNetworkSettings);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().OptionsDialog.NetworkButton.x, SkinProperties().OptionsDialog.NetworkButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().OptionsDialog.NetworkLabel.x, SkinProperties().OptionsDialog.NetworkLabel.y);
	Button = new TTextButton(0, 0, "Hotkeys", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_OPTIONS_HOTKEYS), nullptr);
	Label = new TLabel(SkinProperties().Dialog.TextFont, TextEditHotkeys);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().OptionsDialog.HotkeysButton.x, SkinProperties().OptionsDialog.HotkeysButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().OptionsDialog.HotkeysLabel.x, SkinProperties().OptionsDialog.HotkeysLabel.y);
	Button = new TTextButton(0, 0, "Motd", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_OPTIONS_MOTD), nullptr);
	Label = new TLabel(0, 0, SkinProperties().Dialog.TextFont, TextShowMotD);
	InsertChild(Button);
	SetChildPosition(Button, SkinProperties().OptionsDialog.MotdButton.x, SkinProperties().OptionsDialog.MotdButton.y);
	InsertChild(Label);
	SetChildPosition(Label, SkinProperties().OptionsDialog.MotdLabel.x, SkinProperties().OptionsDialog.MotdLabel.y);
	TSeparator *Separator = new TSeparator(true);
	InsertChild(Separator);
	SetChildArea(Separator, SkinProperties().OptionsDialog.Separator);
	Arrange();
}

void TOptionsDialog::KeyPress(int Key)
{
	if (Key == KEY_RETURN)
		HandleEvent(1);
	else if (Key == KEY_ESCAPE)
		HandleEvent(EVENT_CANCEL);
}

// Closing saves the options; each button opens its dialog and this one
// again after it (0x21).
void TOptionsDialog::HandleEvent(int Event)
{
	switch (Event) {
	case 1:
	case EVENT_CANCEL:
		SavePreferences();
		break;
	case EVENT_OPTIONS_NETWORK:
		AddMessage(MESSAGE_NETWORK_OPTIONS_DIALOG, 0, 0);
		AddMessage(MESSAGE_OPTIONS_DIALOG, 0, 0);
		break;
	case EVENT_OPTIONS_GRAPHICS:
		AddMessage(MESSAGE_GRAPHICS_OPTIONS_DIALOG, 0, 0);
		AddMessage(MESSAGE_OPTIONS_DIALOG, 0, 0);
		break;
	case EVENT_OPTIONS_CONSOLE:
		AddMessage(MESSAGE_CONSOLE_OPTIONS_DIALOG, 0, 0);
		AddMessage(MESSAGE_OPTIONS_DIALOG, 0, 0);
		break;
	case EVENT_OPTIONS_HOTKEYS:
		AddMessage(MESSAGE_HOTKEY_DIALOG, 0, 0);
		AddMessage(MESSAGE_OPTIONS_DIALOG, 0, 0);
		break;
	case EVENT_OPTIONS_GENERAL:
		AddMessage(MESSAGE_GENERAL_OPTIONS_DIALOG, 0, 0);
		AddMessage(MESSAGE_OPTIONS_DIALOG, 0, 0);
		break;
	case EVENT_OPTIONS_MOTD:
		AddMessage(MESSAGE_MESSAGE_OF_THE_DAY, 0, 0);
		AddMessage(MESSAGE_OPTIONS_DIALOG, 0, 0);
		break;
	default:
		TERROR1("unknown event", Event);
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TGeneralOptionsDialog ------------------------------------------------------

TGeneralOptionsDialog::TGeneralOptionsDialog(void (*_OpenURL)(const char *URL))
{
	OpenURL = _OpenURL;
	// Patella-Plus: Drag Full Stacks is a sixth row below the original five, one row's pitch further down. The body
	// grows by that pitch, and TDialog fits itself and its bottom-anchored buttons around the body.
	const int RowPitch = SkinProperties().GeneralOptionsDialogAutoChaseOff.y - SkinProperties().GeneralOptionsDialogClassicControl.y;
	TRect DragFullStacksArea = SkinProperties().GeneralOptionsDialogTextualEffects;
	DragFullStacksArea.y += RowPitch;
	TComponent::SetSize(SkinProperties().GeneralOptionsDialogWidth, SkinProperties().GeneralOptionsDialogHeight + RowPitch);
	ClassicControl = new TCheckBox("Tibia Classic Control");
	AutoChaseOff = new TCheckBox("Auto Chase Off");
	ShowHints = new TCheckBox("Show Hints");
	CreatureNames = new TCheckBox("Show Names of Creatures");
	TextualEffects = new TCheckBox("Show Textual Effects");
	DragFullStacks = new TCheckBox("Drag Full Stacks");
	InsertChild(ClassicControl);
	InsertChild(AutoChaseOff);
	InsertChild(ShowHints);
	InsertChild(CreatureNames);
	InsertChild(TextualEffects);
	InsertChild(DragFullStacks);
	SetChildArea(ClassicControl, SkinProperties().GeneralOptionsDialogClassicControl);
	SetChildArea(AutoChaseOff, SkinProperties().GeneralOptionsDialogAutoChaseOff);
	SetChildArea(ShowHints, SkinProperties().GeneralOptionsDialogShowHints);
	SetChildArea(CreatureNames, SkinProperties().GeneralOptionsDialogCreatureNames);
	SetChildArea(TextualEffects, SkinProperties().GeneralOptionsDialogTextualEffects);
	SetChildArea(DragFullStacks, DragFullStacksArea);
	ClassicControl->SetChecked(GetClassicControl());
	AutoChaseOff->SetChecked(GetAutoChaseOff());
	ShowHints->SetChecked(GetShowNewHints());
	CreatureNames->SetChecked(GetCreatureInfo() != 0);
	TextualEffects->SetChecked(GetTextualEffects());
	DragFullStacks->SetChecked(GetDragFullStacks());
	Arrange();
}

// The creature names are creature info 2 (names and bars) or 0. Help (701)
// keeps the dialog open.
void TGeneralOptionsDialog::HandleEvent(int Event)
{
	switch (Event) {
	case 1:
		SetClassicControl(ClassicControl->IsChecked());
		SetAutoChaseOff(AutoChaseOff->IsChecked());
		SetShowNewHints(ShowHints->IsChecked());
		SetCreatureInfo(CreatureNames->IsChecked() ? 2 : 0);
		SetTextualEffects(TextualEffects->IsChecked());
		SetDragFullStacks(DragFullStacks->IsChecked());
		SetActionState(ACTIONSTATE_NONE);
		break;
	case EVENT_CANCEL:
		SetActionState(ACTIONSTATE_NONE);
		break;
	case EVENT_HELP:
		OpenURL(TextOptionsURL);
		break;
	default:
		TERROR1("unknown event", Event);
	}
}

// --- TGraphicsOptionsDialog ------------------------------------------------------

// The selection is the configured fullscreen mode (by size and depth),
// else the first.
TGraphicsOptionsDialog::TGraphicsOptionsDialog(void (*_OpenURL)(const char *URL), void (*_SetDisplayMode)(int Mode),
	TWindow *_Parent)
{
	OpenURL = _OpenURL;
	SetDisplayMode = _SetDisplayMode;
	Parent = _Parent;
	Fullscreen = new TCheckBox("Fullscreen");
	ResolutionList = new TListBox(NoCallback, NoCallback);
	AdvancedButton = new TTextButton(0, 0, "Advanced", SKIN_LARGE_BUTTON_UP, SKIN_LARGE_BUTTON_DOWN, TCallback(this, EVENT_OPTIONS_ADVANCED), nullptr);
	TLabel *ResolutionLabel = new TLabel(0, 0, SkinProperties().Dialog.TextFont, TextAvailableResolutions);
	TLabel *AdvancedLabel = new TLabel(0, 0, SkinProperties().Dialog.TextFont, TextAdvancedSettings);
	InsertChild(Fullscreen);
	InsertChild(ResolutionList);
	InsertChild(ResolutionLabel);
	InsertChild(AdvancedLabel);
	InsertChild(AdvancedButton);
	TComponent::SetSize(SkinProperties().GraphicsOptionsDialogWidth, SkinProperties().GraphicsOptionsDialogHeight);
	SetChildArea(Fullscreen, SkinProperties().GraphicsOptionsDialogFullscreen);
	SetChildArea(ResolutionList, SkinProperties().GraphicsOptionsDialogResolutionList);
	SetChildArea(ResolutionLabel, SkinProperties().GraphicsOptionsDialogResolutionLabel);
	SetChildArea(AdvancedLabel, SkinProperties().GraphicsOptionsDialogAdvancedLabel);
	SetChildPosition(AdvancedButton, SkinProperties().GraphicsOptionsDialogAdvancedButton.x, SkinProperties().GraphicsOptionsDialogAdvancedButton.y);
	Fullscreen->SetChecked(GetViewMode() == 2);
	// GetFullscreenModes stores its new[] list (or NULL) through the pointer it is given; Modes takes it over (issue
	// #58). Every engine writes the pointer before anything else.
	TFullscreenMode *List = nullptr;
	NumberOfModes = GetSurface()->GetFullscreenModes(&List);
	Modes.reset(List);
	const TFullscreenMode *Current = GetFullscreenMode();
	for (int i = 0; i < NumberOfModes; i++) {
		TFixedString<16> Frequency;
		Frequency.Format(", {} Hz", Modes[i].Frequency);
		TFixedString<244> Line;
		Line.Format("{}*{}, {} Bits{}", Modes[i].Width, Modes[i].Height, Modes[i].Depth,
			Modes[i].Frequency != 0 ? Frequency.Chars : "");
		ResolutionList->AddLine(Line);
		if (i == 0
			|| (Current->Width == Modes[i].Width && Current->Height == Modes[i].Height
				&& Current->Depth == Modes[i].Depth))
			ResolutionList->SetSelection(i);
	}
	Arrange();
}

// Only the arrow keys up and down reach the list.
void TGraphicsOptionsDialog::KeyPress(int Key)
{
	switch (Key) {
	case KEY_RETURN:
		HandleEvent(1);
		break;
	case KEY_ESCAPE:
		HandleEvent(EVENT_CANCEL);
		break;
	case KEY_UP:
		ResolutionList->KeyPress(KEY_UP);
		break;
	case KEY_DOWN:
		ResolutionList->KeyPress(KEY_DOWN);
		break;
	default:
		break;
	}
}

// OK stores the selected mode and switches the display mode when the
// fullscreen setting changed (to fullscreen only with a selected mode); with
// fullscreen checked the windows are then arranged for the screen's size.
// Advanced (407) closes and opens the advanced options and this dialog again;
// Help (701) keeps it open.
void TGraphicsOptionsDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		const int ViewMode = GetViewMode();
		const bool FullscreenChecked = Fullscreen->IsChecked();
		if (ResolutionList->GetSelection() != -1)
			SetFullscreenMode(&Modes[ResolutionList->GetSelection()]);
		if ((ViewMode == 2) != FullscreenChecked
			&& (!Fullscreen->IsChecked() || ResolutionList->GetSelection() != -1))
			SetDisplayMode(Fullscreen->IsChecked() + 1);
		if (Fullscreen->IsChecked())
			patella::bridge::GUI().Resize(0, 0); // Control.cpp's (issues #107, #118)
	} else if (Event == EVENT_OPTIONS_ADVANCED) {
		AddMessageFirst(MESSAGE_GRAPHICS_OPTIONS_DIALOG, 0, 0);
		AddMessageFirst(MESSAGE_ADVANCED_GRAPHICS_DIALOG, reinterpret_cast<std::intptr_t>(Parent), 0);
	} else if (Event == EVENT_HELP) {
		OpenURL(TextGraphicsOptionsURL);
		return;
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TAdvancedGraphicsDialog ------------------------------------------------------

// Fullscreen allows no change of the graphics device; otherwise the GPU
// button needs a GPU and the CPU button the CPU driver. ShowDevice selects
// the button of the device choice.
TAdvancedGraphicsDialog::TAdvancedGraphicsDialog(void (*_OpenURL)(const char *URL), TComponent *_Receiver)
{
	OpenURL = _OpenURL;
	Receiver = _Receiver;
	EngineGroup.reset(new TRadioButtonGroup);
	LightEffects = new TCheckBox("Show Light Effects");
	NoStretching = new TCheckBox("Don't stretch/shrink the Map");
	WindowsMousepointer = new TCheckBox("Windows Mouse Pointer");
	Framerate = new THorizontalScrollBar(10, 201, 10, 11, 1, TCallback(this, EVENT_FRAMERATE));
	// The x64 client (issue #31): the graphics device choice in the engine buttons' places, with the
	// original's events (MainWindow.cpp: 800, 801, 802; Control.cpp ChangeEngine). The buttons are 58
	// pixels wide (skin 16), which "CPU (compatibility)" does not fit: its help text says it. The line
	// above them is as wide as the dialog; a longer device name is shortened, the whole one is its help.
	DeviceLabelName[0] = 0;
	DeviceLabel = new TLabel(SkinProperties().AdvancedGraphicsDialog.DeviceLabel.x, SkinProperties().AdvancedGraphicsDialog.DeviceLabel.y,
		SkinProperties().AdvancedGraphicsDialog.Width - SkinProperties().AdvancedGraphicsDialog.DeviceLabel.x,
		GetFontLineHeight(SkinProperties().Dialog.TextFont.FontNumber), SkinProperties().Dialog.TextFont,
		TextSelectDevice, true);
	TLabel *Label = DeviceLabel;
	DirectX5Button = new TTextButton(SkinProperties().AdvancedGraphicsDialog.DirectX5Button.x, SkinProperties().AdvancedGraphicsDialog.DirectX5Button.y,
		"Automatic", SKIN_MEDIUM_BUTTON_UP, SKIN_MEDIUM_BUTTON_DOWN, TCallback(_Receiver, EVENT_DEVICE_AUTOMATIC), TextUseAutomaticDevice);
	DirectX5Button->SetRadioButtonGroup(EngineGroup.get());
	DirectX9Button = new TTextButton(SkinProperties().AdvancedGraphicsDialog.DirectX9Button.x, SkinProperties().AdvancedGraphicsDialog.DirectX9Button.y,
		"GPU", SKIN_MEDIUM_BUTTON_UP, SKIN_MEDIUM_BUTTON_DOWN, TCallback(_Receiver, EVENT_DEVICE_GPU), TextUseGpuDevice);
	DirectX9Button->SetRadioButtonGroup(EngineGroup.get());
	OpenGLButton = new TTextButton(SkinProperties().AdvancedGraphicsDialog.OpenGLButton.x, SkinProperties().AdvancedGraphicsDialog.OpenGLButton.y,
		"CPU", SKIN_MEDIUM_BUTTON_UP, SKIN_MEDIUM_BUTTON_DOWN, TCallback(_Receiver, EVENT_DEVICE_CPU), TextUseCpuDevice);
	OpenGLButton->SetRadioButtonGroup(EngineGroup.get());
	InsertChild(Label);
	InsertChild(DirectX5Button);
	InsertChild(DirectX9Button);
	InsertChild(OpenGLButton);
	InsertChild(LightEffects);
	InsertChild(NoStretching);
	InsertChild(WindowsMousepointer);
	InsertChild(Framerate);
	TComponent::SetSize(SkinProperties().AdvancedGraphicsDialog.Width, SkinProperties().AdvancedGraphicsDialog.Height);
	SetChildArea(LightEffects, SkinProperties().AdvancedGraphicsDialog.LightEffects);
	SetChildArea(NoStretching, SkinProperties().AdvancedGraphicsDialog.NoStretching);
	SetChildArea(WindowsMousepointer, SkinProperties().AdvancedGraphicsDialog.WindowsMousepointer);
	SetChildArea(Framerate, SkinProperties().AdvancedGraphicsDialog.Framerate);
	LightEffects->SetChecked(GetLightEffects());
	if (!GetSurface()->CanShadeRectangles())
		LightEffects->SetEnabled(false);
	NoStretching->SetChecked(GetNoStretching());
	WindowsMousepointer->SetChecked(GetWindowsMousepointer());
	int FramerateLimit = GetConfineFramerate();
	if (FramerateLimit < 10)
		FramerateLimit = 201;
	Framerate->Range->SetValue(FramerateLimit);
	if (GetViewMode() == 2) {
		DirectX5Button->SetEnabled(false);
		DirectX9Button->SetEnabled(false);
		OpenGLButton->SetEnabled(false);
	} else {
		// GPU without a GPU, CPU without the CPU driver next to the executable (vulkan/DeviceSettings.h).
		if (GetNumberOfGpus() == 0)
			DirectX9Button->SetEnabled(false);
		if (GetCpuDriverManifest().empty())
			OpenGLButton->SetEnabled(false);
	}
	ShowDevice();
	Arrange();
}

// The device choice's button selected, and the line above the buttons naming the device in use, when
// that changed.
void TAdvancedGraphicsDialog::ShowDevice()
{
	switch (patella::vulkan::ButtonOf(GetDeviceChoice())) {
	case patella::vulkan::TDeviceButton::Automatic:
		EngineGroup->Selected = DirectX5Button;
		break;
	case patella::vulkan::TDeviceButton::Gpu:
		EngineGroup->Selected = DirectX9Button;
		break;
	case patella::vulkan::TDeviceButton::Cpu:
		EngineGroup->Selected = OpenGLButton;
		break;
	}
	const char *Name = GetDeviceName();
	if (strncmp(Name, DeviceLabelName, sizeof(DeviceLabelName) - 1) == 0)
		return;
	snprintf(DeviceLabelName, sizeof(DeviceLabelName), "%s", Name);
	TFixedString<sizeof(DeviceLabelName) + 16> Text;
	if (DeviceLabelName[0] != 0)
		snprintf(Text, sizeof(Text), "Device: %s", DeviceLabelName.Chars);
	else
		snprintf(Text, sizeof(Text), "%s", TextSelectDevice);
	DeviceLabel->SetText(Text, true);
}

// Draws the frame rate texts over the children and shows the device
// choice's button as selected and the device in use (ShowDevice).
void TAdvancedGraphicsDialog::Draw(int Surface, int x, int y)
{
	TWindow::Draw(Surface, x, y);
	TFixedString<612> Text;
	if (Framerate->Range->Value < 201)
		Text.Format("Adjust framerate limit: {}", Framerate->Range->Value);
	else
		Text.Assign("Adjust framerate limit: max");
	PrintFormattedText(Surface, SkinProperties().AdvancedGraphicsDialog.FramerateLimitText.x + x, SkinProperties().AdvancedGraphicsDialog.FramerateLimitText.y + y,
		SkinProperties().Dialog.TextFont, Text);
	const double FPS = GetSurface()->GetFPS();
	Text.Format("Current framerate: {:2.2f}", FPS);
	PrintFormattedText(Surface, SkinProperties().AdvancedGraphicsDialog.CurrentFramerateText.x + x, SkinProperties().AdvancedGraphicsDialog.CurrentFramerateText.y + y,
		SkinProperties().Dialog.TextFont, Text);
	ShowDevice();
}

// The frame rate bar (30) sets the limit while it moves; Cancel (2)
// restores the configured one. Turning on "don't stretch" makes the
// receiver arrange the game window again (event 9).
void TAdvancedGraphicsDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		const bool Rearrange = !GetNoStretching() && NoStretching->IsChecked();
		SetNoStretching(NoStretching->IsChecked());
		SetWindowsMousepointer(WindowsMousepointer->IsChecked());
		SetLightEffects(LightEffects->IsChecked());
		SetConfineFramerate(Framerate->Range->Value);
		GetSurface()->SetFramerateLimit(GetConfineFramerate());
		if (Rearrange)
			Receiver->HandleEvent(EVENT_REARRANGE);
	} else if (Event == EVENT_HELP) {
		OpenURL(TextAdvancedGraphicsURL);
		return;
	} else if (Event == EVENT_FRAMERATE) {
		GetSurface()->SetFramerateLimit(Framerate->Range->Value);
		return;
	} else if (Event == EVENT_CANCEL) {
		GetSurface()->SetFramerateLimit(GetConfineFramerate());
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TConsoleOptionsDialog --------------------------------------------------------

TConsoleOptionsDialog::TConsoleOptionsDialog(void (*_OpenURL)(const char *URL))
{
	OpenURL = _OpenURL;
	TComponent::SetSize(SkinProperties().ConsoleOptionsDialogWidth, SkinProperties().ConsoleOptionsDialogHeight);
	InfoMessages = new TCheckBox("Show Info Messages in Console");
	EventMessages = new TCheckBox("Show Event Messages in Console");
	StatusMessages = new TCheckBox("Show Status Messages in Console");
	TimeStamps = new TCheckBox("Show Timestamp in Console");
	PrivateMessages = new TCheckBox("Show Private Messages in Game Window");
	InsertChild(InfoMessages);
	InsertChild(EventMessages);
	InsertChild(StatusMessages);
	InsertChild(TimeStamps);
	InsertChild(PrivateMessages);
	SetChildArea(InfoMessages, SkinProperties().ConsoleOptionsDialogInfoMessages);
	SetChildArea(EventMessages, SkinProperties().ConsoleOptionsDialogEventMessages);
	SetChildArea(StatusMessages, SkinProperties().ConsoleOptionsDialogStatusMessages);
	SetChildArea(TimeStamps, SkinProperties().ConsoleOptionsDialogTimeStamps);
	SetChildArea(PrivateMessages, SkinProperties().ConsoleOptionsDialogPrivateMessages);
	InfoMessages->SetChecked(GetInfoMessages());
	EventMessages->SetChecked(GetEventMessages());
	StatusMessages->SetChecked(GetStatusMessages());
	TimeStamps->SetChecked(GetTimeStamps());
	PrivateMessages->SetChecked(GetPrivateMessages());
	Arrange();
}

void TConsoleOptionsDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		SetInfoMessages(InfoMessages->IsChecked());
		SetEventMessages(EventMessages->IsChecked());
		SetStatusMessages(StatusMessages->IsChecked());
		SetTimeStamps(TimeStamps->IsChecked());
		SetPrivateMessages(PrivateMessages->IsChecked());
		SetConsoleChanged();
	} else if (Event == EVENT_HELP) {
		OpenURL(TextConsoleOptionsURL);
		return;
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- TNetworkOptionsDialog ---------------------------------------------------------

TNetworkOptionsDialog::TNetworkOptionsDialog(void (*_OpenURL)(const char *URL))
{
	OpenURL = _OpenURL;
	TComponent::SetSize(SkinProperties().NetworkOptionsDialogWidth, SkinProperties().NetworkOptionsDialogHeight);
	TestServerLogin = new TCheckBox("Test Server Login");
	InsertChild(TestServerLogin);
	SetChildArea(TestServerLogin, SkinProperties().NetworkOptionsDialogTestServerLogin);
	TestServerLogin->SetChecked(GetTestServerLogin());
	Arrange();
}

void TNetworkOptionsDialog::HandleEvent(int Event)
{
	if (Event == 1) {
		SetTestServerLogin(TestServerLogin->IsChecked());
	} else if (Event == EVENT_HELP) {
		OpenURL(TextNetworkOptionsURL);
		return;
	}
	SetActionState(ACTIONSTATE_NONE);
}

// --- THotkeyDialog -------------------------------------------------------------------

// Every hotkey gets a line (271 characters at most) that UpdateLine
// fills.
THotkeyDialog::THotkeyDialog()
{
	TComponent::SetSize(SkinProperties().HotkeyDialogWidth, SkinProperties().HotkeyDialogHeight);
	TLabel *Label = new TLabel(0, 0, SkinProperties().Dialog.TextFont, TextAvailableHotkeys);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().HotkeyDialogListLabel);
	Label = new TLabel(0, 0, SkinProperties().Dialog.TextFont, TextEditHotkeyText);
	InsertChild(Label);
	SetChildArea(Label, SkinProperties().HotkeyDialogTextLabel);
	ListBox = new TFontListBox(TCallback(this, EVENT_LIST_SELECTED), NoCallback);
	InsertChild(ListBox);
	SetChildArea(ListBox, SkinProperties().HotkeyDialogList);
	for (int i = 0; i < HOTKEYS; i++) {
		Texts[i].Assign(GetHotkey(i, &AutoSend[i]));
		ListBox->AddLine(271);
		UpdateLine(i);
	}
	TextEdit = new TEditLine(255, false);
	InsertChild(TextEdit);
	SetChildArea(TextEdit, SkinProperties().HotkeyDialogText);
	AutoSendCheckBox = new TCheckBox("Send automatically");
	InsertChild(AutoSendCheckBox);
	SetChildArea(AutoSendCheckBox, SkinProperties().HotkeyDialogAutoSend);
	Selected = -1;
	Arrange();
}

// Hotkeys sent automatically are drawn in their own colour.
void THotkeyDialog::UpdateLine(int Number)
{
	PATELLA_ASSERT_VALUES(In(Number,0,HOTKEYS-1), Number);
	TFixedString<732> Line;
	if (Number < 12)
		Line.Format("F{}{}: {}", Number + 1, "", Texts[Number]);
	if (Number >= 12 && Number <= 23)
		Line.Format("Shift+F{}{}: {}", Number - 11, "", Texts[Number]);
	if (Number >= 24)
		Line.Format("Control+F{}{}: {}", Number - 23, "", Texts[Number]);
	TFont Font = SkinProperties().ListBox.Font;
	if (AutoSend[Number])
		Font.Color = SkinProperties().HotkeyDialogAutoSendColor;
	ListBox->SetLine(Number, Line, Font);
}

// Every other key edits the selected hotkey's text; an empty text cannot be
// sent automatically.
void THotkeyDialog::KeyPress(int Key)
{
	switch (Key) {
	case KEY_RETURN:
		HandleEvent(1);
		break;
	case KEY_ESCAPE:
		HandleEvent(EVENT_CANCEL);
		break;
	case KEY_UP:
		ListBox->KeyPress(KEY_UP);
		break;
	case KEY_DOWN:
		ListBox->KeyPress(KEY_DOWN);
		break;
	default:
		if (Selected != -1) {
			TextEdit->KeyPress(Key);
			Texts[Selected].Assign(TextEdit->Text.get());
			AutoSendCheckBox->SetEnabled(strlen(Texts[Selected]) != 0);
			if (strlen(Texts[Selected]) == 0) {
				AutoSend[Selected] = false;
				AutoSendCheckBox->SetChecked(false);
			}
			UpdateLine(Selected);
		}
		break;
	}
}

// OK stores the hotkeys and saves the options unless the options dialog
// comes next (it saves them itself). A selection (11) stores the check box
// of the hotkey left and edits the new one; no selection (-1) reads before
// the texts (kept).
void THotkeyDialog::HandleEvent(int Event)
{
	switch (Event) {
	case 1:
		if (Selected != -1)
			AutoSend[Selected] = AutoSendCheckBox->IsChecked();
		for (int i = 0; i < HOTKEYS; i++)
			SetHotkey(i, Texts[i], AutoSend[i]);
		SetActionState(ACTIONSTATE_NONE);
		if (PeekMessageType() != MESSAGE_OPTIONS_DIALOG)
			SavePreferences();
		break;
	case EVENT_CANCEL:
		SetActionState(ACTIONSTATE_NONE);
		break;
	case EVENT_LIST_SELECTED:
		if (Selected != -1)
			AutoSend[Selected] = AutoSendCheckBox->IsChecked();
		Selected = ListBox->GetSelection();
		TextEdit->SetText(Texts[Selected]);
		TextEdit->SetActive(true);
		AutoSendCheckBox->SetChecked(AutoSend[Selected]);
		AutoSendCheckBox->SetEnabled(strlen(Texts[Selected]) != 0);
		if (strlen(Texts[Selected]) == 0)
			AutoSendCheckBox->SetChecked(false);
		break;
	default:
		TERROR1("unknown event", Event);
	}
}

// A click may have changed the check box.
bool THotkeyDialog::LeftButtonUp(int x, int y, int Flags)
{
	const bool Result = TWindow::LeftButtonUp(x, y, Flags);
	if (Selected != -1) {
		AutoSend[Selected] = AutoSendCheckBox->IsChecked();
		UpdateLine(Selected);
	}
	return Result;
}
// NOLINTEND(cppcoreguidelines-owning-memory,cppcoreguidelines-pro-type-vararg)
