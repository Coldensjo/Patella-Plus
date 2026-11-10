// ConsoleWindow.cpp: the console below the map; see ConsoleWindow.h.
//
// The console's skin properties are SkinProperties.ConsoleWindow (the console),
// Tab (the tabs) and EditLine.Font (the font of the default channel);
// the frames are skins 169-176 (TConsole) and 177-185 (TConsoleWindow). Events
// (201-213, 403) and AddMessage's numbers are named in Events.h; the comments
// give events in decimal, as in ContextMenus.cpp, and messages in hex.

#include "ConsoleWindow.h"
#include "Communication.h"
#include "Creatures.h"
#include "DialogElements.h"
#include "Help.h"
#include "Preferences.h"
#include "Skins.h"
#include "Status.h"
#include "Surface.h"
#include "Talk.h"
#include "Utils.h"
#include "graphics.h"
#include "compat/crt71.h"
#include "GUI.h"
#include "common.h"
#include "compat/span.h"
#include "platform/Platform.h"
#include "types/File.h"
#include "types/FixedString.h"
#include <sstream>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <ctime>
#include <cstdio>
#include <memory>
#include <type_traits>

namespace {

// The texts of the user interface that this file shows. The numbers are their places in the original's table of
// texts, a header every file included.
const char *TextCloseChannel = "Close this Channel (Ctrl+E)";                                  // 12
const char *TextShowServerMessages = "Show server messages in this channel (Ctrl+M)";          // 13
const char *TextHideServerMessages = "Hide server messages in this channel (Ctrl+M)";          // 14
const char *TextIgnorePlayers = "Ignore and unignore other players (Ctrl+I)";                  // 15
const char *TextOpenNewChannel = "Open new channel (Ctrl+O)";                                  // 16
const char *TextAdjustVolume = "Adjust volume";                                                // 17
const char *TextMoveChannelsRight = "Move channels to the right";                              // 18
const char *TextMoveChannelsLeft = "Move channels to the left";                                // 19
const char *TextWaitForGamemaster = "Please wait patiently for a gamemaster to reply.";        // 119
const char *TextRequestSubmitted = "You have already submitted a request. Please be patient."; // 120
const char *TextHelpChannelWelcome =                                                           // 122
	"Welcome to the help channel! Feel free to ask questions concerning client controls, "
	"general game play, use of accounts and the official homepage. In-depth questions about "
	"the content of the game will not be answered. Experienced players will be glad to help "
	"you to the best of their knowledge. Keep in mind that this is not a chat channel for "
	"general conversations. Therefore please limit your statements to relevant questions and "
	"answers.";
const char *TextReportRuleViolation = "Report Rule Violation"; // 146

} // namespace

// cppcoreguidelines-owning-memory is off for the file, for the original's raw owners: the window tree owns its
// children, the message queue its blocks, the lists their entries.
// NOLINTBEGIN(cppcoreguidelines-owning-memory)

// ParseInput's error names its value "(Word) MessageSeparator": the char
// converted to an unsigned 16-bit value.
using Word = uint16;

// --- TTextWindow -----------------------------------------------------------------------------

TTextWindow::TTextWindow() : Lines(0, 0, 100)
{
	LastLineNumber = -1;
	SelectionStartLine = -1;
	SelectionStartOffset = -1;
	SelectionEndLine = -1;
	SelectionEndOffset = -1;
	TextBuffer = nullptr;
	CurrentLineNumber = 0;
	InvisibleLines = 0;
	Selecting = false;
	SelectedText = nullptr;
	AtEnd = true;
}

// Draws the lines from CurrentLineNumber upwards until the window is full,
// the selected part of each on a filled rectangle. A buffer without lines is
// formatted when it has new ones; a buffer whose lines were removed is not
// drawn (and its change flag dropped) until the window is arranged again.
void TTextWindow::Draw(int Surface, int x, int y)
{
	if (TextBuffer == nullptr || TextBuffer->first == nullptr || !TextBuffer->Complete) {
		if (TextBuffer != nullptr && TextBuffer->first != nullptr)
			TextBuffer->LinesChanged = false;
		if (TextBuffer != nullptr) {
			const bool LinesChanged = TextBuffer->LinesChanged;
			TextBuffer->LinesChanged = false;
			if (LinesChanged)
				Arrange();
		}
		if (TextBuffer == nullptr || TextBuffer->first == nullptr)
			return;
		if (!TextBuffer->Complete) {
			TextBuffer->LinesChanged = false;
			return;
		}
	}

	int Bottom = Height;
	bool First = true;
	for (int CurrentLine = CurrentLineNumber; CurrentLine >= GetFirstLineNumber(); CurrentLine--) {
		const TLineFormat *Format = GetLineFormat(CurrentLine);
		PATELLA_ASSERT_VALUES(Format!=nullptr, CurrentLine, GetFirstLineNumber());
		PATELLA_ASSERT_VALUES(Format->LineText!=nullptr, CurrentLine, GetFirstLineNumber());
		int LineHeight;
		if (First) {
			LineHeight = Graphics().GetFontHeight(Format->FontNumber);
			First = false;
		} else {
			LineHeight = Graphics().GetFontLineHeight(Format->FontNumber);
		}
		Bottom -= LineHeight;
		if (Bottom < 0)
			break;
		const int LineX = Format->Indent + x;
		const int LineY = Bottom + y;
		int EndLine, EndOffset, StartLine, StartOffset;
		if (HasSelection() && GetSelectionEnd(&EndLine, &EndOffset) && GetSelectionStart(&StartLine, &StartOffset)
			&& StartLine <= CurrentLine && CurrentLine <= EndLine) {
			int SelectionHeight = Graphics().GetFontLineHeight(Format->FontNumber);
			int SelectionWidth = 0;
			int SelectionX = LineX;
			if (CurrentLine == EndLine) {
				if (CurrentLine == StartLine) {
					SelectionX = Graphics().GetTextPartWidth(Format->FontNumber, Format->LineText, StartOffset);
					SelectionWidth = Graphics().GetTextPartWidth(Format->FontNumber, Format->LineText + StartOffset,
						EndOffset - StartOffset);
					SelectionHeight = Graphics().GetFontHeight(Format->FontNumber);
					SelectionX += LineX;
				} else {
					SelectionWidth = Graphics().GetTextPartWidth(Format->FontNumber, Format->LineText, EndOffset);
					SelectionHeight = Graphics().GetFontHeight(Format->FontNumber);
				}
			} else if (CurrentLine == StartLine) {
				SelectionX = Graphics().GetTextPartWidth(Format->FontNumber, Format->LineText, StartOffset) + LineX;
				SelectionWidth = Graphics().GetTextPartWidth(Format->FontNumber, Format->LineText + StartOffset,
					Format->LineLength - StartOffset);
			} else {
				SelectionWidth = Graphics().GetTextPartWidth(Format->FontNumber, Format->LineText, Format->LineLength);
			}
			GraphicsEngine().GetSurface()->FillRectangle(Surface,
				TRect(SelectionX, LineY, SelectionWidth, SelectionHeight),
				SkinProperties().ConsoleWindow.SelectionColor);
		}
		Graphics().PrintTextPart(Surface, LineX, LineY, Format->FontNumber, Format->Color, Format->LineText,
			Format->LineLength, 0);
	}
}

// Formats the lines and counts those that do not fit above the last one. A
// line without a format would loop for ever (the counter only moves on a
// format); FormatLines leaves none.
void TTextWindow::Arrange()
{
	InvisibleLines = 0;
	LastLineNumber = -1;
	SetCurrentLineNumber(-1);
	FormatLines();
	int Rest = Height;
	bool First = true;
	int CurrentLine = LastLineNumber;
	while (CurrentLine >= GetFirstLineNumber()) {
		const TLineFormat *Format = GetLineFormat(CurrentLine);
		if (Format != nullptr) {
			CurrentLine--;
			if (First) {
				Rest -= Graphics().GetFontHeight(Format->FontNumber);
				First = false;
			} else {
				Rest -= Graphics().GetFontLineHeight(Format->FontNumber);
			}
			if (Rest < 0)
				InvisibleLines++;
		}
	}
}

// Moves the end of the selection while the button is down.
bool TTextWindow::MouseMove(int x, int y)
{
	if (Selecting) {
		int Line, Offset, Length;
		if (GetPosition(x, y, &Line, &Offset, &Length)) {
			if (Length < Offset)
				Offset = Length;
			SelectionEndLine = Line;
			SelectionEndOffset = Offset;
		}
	}
	return true;
}

bool TTextWindow::LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	if (Selecting) {
		ReleaseCapture(nullptr);
		Selecting = false;
	}
	return true;
}

// Starts a selection at the position.
bool TTextWindow::LeftButtonDown(int x, int y, [[maybe_unused]] int Flags)
{
	Selecting = true;
	SetCapture(nullptr);
	int Line, Offset, Length;
	if (GetPosition(x, y, &Line, &Offset, &Length)) {
		SelectionStartLine = Line;
		SelectionStartOffset = Offset;
		SelectionEndLine = Line;
		SelectionEndOffset = Offset;
	}
	return true;
}

// Selects the word at the position. The second loop tests the character
// before the line length.
bool TTextWindow::LeftButtonDoubleClick(int x, int y, [[maybe_unused]] int Flags)
{
	int Length = 0;
	int Line, Offset, LineLength;
	if (GetPosition(x, y, &Line, &Offset, &LineLength)) {
		const TLineFormat *Format = GetLineFormat(Line);
		if (Format != nullptr) {
			while (Offset > 0 && !IsSpace(Format->LineText[Offset - 1]))
				Offset--;
			while (!IsSpace(Format->LineText[Length + Offset]) && Length + Offset < Format->LineLength)
				Length++;
			SelectionStartLine = Line;
			SelectionStartOffset = Offset;
			SelectionEndLine = Line;
			SelectionEndOffset = Length + Offset;
		}
	}
	return true;
}

// Opens the line's context menu (Control.cpp's message 0x37); in the
// rule violation channel a line with a speaker opens the request
// menu (message 0x39).
bool TTextWindow::RightButtonUp(int x, int y, [[maybe_unused]] int Flags)
{
	int Line, Offset, Length;
	if (!GetPosition(x, y, &Line, &Offset, &Length))
		return true;
	int Message;
	if (!TextBuffer->IsRuleViolationChannel()) {
		Message = MESSAGE_TEXT_MENU;
	} else {
		TTalkLine *TextLine = GetTalkLine(Line);
		if (TextLine == nullptr)
			return true;
		if (strlen(TextLine->Addressee) == 0)
			return true;
		Message = MESSAGE_REQUEST_MENU;
	}
	AddMessage(Message, Line, 0);
	return true;
}

// The edit events of TConsoleWindow::KeyPress and the text menu: 16 copy,
// 17 cut, 18 paste, 19 delete (nothing to do in a text), 20 select all.
void TTextWindow::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_COPY: {
		const char *Text = GetSelectedText();
		if (Text != nullptr)
			Interaction().CopyToClipboard(Text);
		break;
	}
	case EVENT_CUT:
	case EVENT_PASTE:
	case EVENT_DELETE:
		break;
	case EVENT_SELECT_ALL:
		SelectAll();
		break;
	default:
		TERROR1("unknown event", Event);
	}
}

void TTextWindow::SetTextBuffer(TTalkBuffer *_TextBuffer)
{
	TextBuffer = _TextBuffer;
	Arrange();
}

// Breaks every talk line into the parts that fit into the width (font 4), at
// the last blank or at a line break; the text of a part starts after the
// blank. LineEnd is not reset between the parts of a talk line: after its
// first line break every part has it. Entries above the last line are
// cleared. A width in which no character fits would loop for ever (no part
// is ever taken).
void TTextWindow::FormatLines()
{
	const int TextWidth = Width;
	int Count = 0;
	if (TextBuffer != nullptr) {
		TTalkLine *TextLine = TextBuffer->GetFirst();
		while (TextLine != nullptr) {
			TRGBColor Color;
			Talk().GetTalkColor(&Color, TextLine->Mode);
			int NextIndent = 10;
			if (TextLine->Mode >= TALK_ADMIN_MESSAGE && TextLine->Mode <= TALK_INFO_MESSAGE)
				NextIndent = 0;
			const char *Text = TextLine->GetText();
			int Indent = 0;
			bool LineEnd = false;
			bool Last = false;
			int Start = 0;
			while (!Last) {
				int Blank = -1;
				const int End = Graphics().GetFittingTextLength(FONT_SMOOTH, Text + Start, TextWidth - Indent) + Start;
				int i;
				for (i = Start; i < End; i++) {
					const char c = Text[i];
					if (IsSpace(c))
						Blank = i;
					if (c == '\n') {
						LineEnd = true;
						break;
					}
				}
				int Length;
				if (Text[i] == 0) {
					Last = true;
					LineEnd = true;
					Length = i - Start;
				} else if (Blank == -1) {
					Length = i - Start;
				} else {
					Length = Blank - Start;
					i = Blank + 1;
				}
				if (Length > 0)
					Lines(Count++) = TLineFormat(TextLine, Text + Start, Length, Start, Indent, FONT_SMOOTH, Color, LineEnd);
				Indent = NextIndent;
				Start = i;
			}
			TextLine = TextBuffer->GetNext();
		}
		TextBuffer->Complete = true;
	}
	LastLineNumber = Count - 1;
	const TLineFormat Empty;
	for (int i = Count; i <= Lines.max; i++)
		Lines(i) = Empty;
}

// The talk line shown in that line.
TTalkLine *TTextWindow::GetTalkLine(int LineNumber)
{
	if (TextBuffer != nullptr && TextBuffer->first != nullptr) {
		if (!TextBuffer->Complete) {
			TextBuffer->LinesChanged = false;
		} else if (LineNumber <= Lines.max && LineNumber >= Lines.min) {
			const TLineFormat *Format = &Lines(LineNumber);
			if (Format != nullptr)
				return Format->TextLine;
		}
	}
	return nullptr;
}

TLineFormat *TTextWindow::GetLineFormat(int LineNumber)
{
	if (TextBuffer != nullptr && TextBuffer->first != nullptr) {
		if (!TextBuffer->Complete)
			TextBuffer->LinesChanged = false;
		else if (LineNumber <= Lines.max && LineNumber >= Lines.min)
			return &Lines(LineNumber);
	}
	return nullptr;
}

void TTextWindow::SetCurrentLineNumber(int _CurrentLineNumber)
{
	PATELLA_ASSERT_VALUES(In(_CurrentLineNumber, GetFirstLineNumber(), GetLastLineNumber()) || _CurrentLineNumber==-1,
		_CurrentLineNumber, GetLastLineNumber());
	CurrentLineNumber = _CurrentLineNumber;
	if (_CurrentLineNumber != -1)
		AtEnd = _CurrentLineNumber == GetLastLineNumber();
}

// The earlier end of the selection; a selection that ends beyond its
// line is dropped.
bool TTextWindow::GetSelectionStart(int *Line, int *Offset)
{
	if (HasSelection()) {
		int StartLine = SelectionStartLine;
		int StartOffset = SelectionStartOffset;
		const int EndLine = SelectionEndLine;
		const int EndOffset = SelectionEndOffset;
		if (EndLine < StartLine || (EndLine == StartLine && EndOffset < StartOffset)) {
			StartLine = EndLine;
			StartOffset = EndOffset;
		}
		const TLineFormat *Format = GetLineFormat(StartLine);
		if (Format != nullptr && StartOffset <= Format->LineLength) {
			*Line = StartLine;
			*Offset = StartOffset;
			return true;
		}
		SelectionStartLine = -1;
		SelectionStartOffset = -1;
		SelectionEndLine = -1;
		SelectionEndOffset = -1;
	}
	return false;
}

// The later end of the selection.
bool TTextWindow::GetSelectionEnd(int *Line, int *Offset)
{
	if (HasSelection()) {
		int StartLine = SelectionStartLine;
		int StartOffset = SelectionStartOffset;
		const int EndLine = SelectionEndLine;
		const int EndOffset = SelectionEndOffset;
		if (StartLine < EndLine || (StartLine == EndLine && StartOffset <= EndOffset)) {
			StartLine = EndLine;
			StartOffset = EndOffset;
		}
		const TLineFormat *Format = GetLineFormat(StartLine);
		if (Format != nullptr && StartOffset <= Format->LineLength) {
			*Line = StartLine;
			*Offset = StartOffset;
			return true;
		}
		SelectionStartLine = -1;
		SelectionStartOffset = -1;
		SelectionEndLine = -1;
		SelectionEndOffset = -1;
	}
	return false;
}

bool TTextWindow::SelectAll()
{
	if (TextBuffer != nullptr) {
		if (TextBuffer->Complete) {
			SelectionStartLine = 0;
			SelectionStartOffset = 0;
			SelectionEndLine = LastLineNumber;
			const TLineFormat *Format = GetLineFormat(LastLineNumber);
			if (Format != nullptr)
				SelectionEndOffset = Format->LineLength;
			else
				SelectionEndOffset = 0;
			return true;
		}
		TextBuffer->LinesChanged = false;
	}
	return false;
}

// The selected text, with a line break after the lines that end a
// paragraph (not after the last). The window keeps the text. In a one-line
// selection a missing format is not tested (it would crash).
const char *TTextWindow::GetSelectedText()
{
	if (HasSelection()) {
		if (SelectedText != nullptr)
			SelectedText.reset();
		SelectedText = nullptr;
		int StartLine, StartOffset, EndLine, EndOffset;
		if (GetSelectionStart(&StartLine, &StartOffset) && GetSelectionEnd(&EndLine, &EndOffset)) {
			uint32 Size = 1;
			for (int i = StartLine; i <= EndLine; i++) {
				const TLineFormat *Format = GetLineFormat(i);
				if (Format != nullptr && Format->LineText != nullptr)
					Size += 1 + Format->LineLength;
			}
			if (Size != 1) {
				SelectedText.reset(new char[Size]);
				SelectedText[0] = 0;
				TStringBuffer Selected(SelectedText.get(), static_cast<std::size_t>(Size));
				if (StartLine == EndLine) {
					const TLineFormat *Format = GetLineFormat(StartLine);
					if (Format == nullptr || Format->LineText != nullptr)
						CopyString(patella::TSpan<char>(SelectedText.get(), static_cast<std::size_t>(EndOffset - StartOffset + 1)), Format->LineText + StartOffset);
					return SelectedText.get();
				}
				for (int i = StartLine; i <= EndLine; i++) {
					const TLineFormat *Format = GetLineFormat(i);
					if (Format == nullptr || Format->LineText == nullptr)
						continue;
					if (i == EndLine) {
						CopyString(patella::TSpan<char>(SelectedText.get() + strlen(SelectedText.get()), static_cast<std::size_t>(EndOffset + 1)), Format->LineText);
					} else if (i == StartLine) {
						CopyString(patella::TSpan<char>(SelectedText.get() + strlen(SelectedText.get()),
									   static_cast<std::size_t>(Format->LineLength - StartOffset + 1)),
							Format->LineText + StartOffset);
						if (Format->LineEnd)
							Selected.Append("\n");
					} else {
						CopyString(patella::TSpan<char>(SelectedText.get() + strlen(SelectedText.get()), static_cast<std::size_t>(Format->LineLength + 1)), Format->LineText);
						if (Format->LineEnd)
							Selected.Append("\n");
					}
				}
				return SelectedText.get();
			}
		}
	}
	return nullptr;
}

// The line and character under a point (clamped to the window): the lines
// are searched upwards from CurrentLineNumber; above them the first line is
// taken from its start. Length is the line's length. The line height is
// asked for twice for the bottom line (the first answer is dropped).
bool TTextWindow::GetPosition(int x, int y, int *Line, int *Offset, int *Length)
{
	if (TextBuffer != nullptr && TextBuffer->first != nullptr) {
		if (!TextBuffer->Complete) {
			TextBuffer->LinesChanged = false;
		} else {
			if (x < 0)
				x = 0;
			if (x > Width)
				x = Width;
			if (y < 0)
				y = 0;
			int Bottom = Height;
			if (y > Bottom)
				y = Bottom;
			bool First = true;
			bool Found = false;
			int CurrentLine = CurrentLineNumber;
			int LineNumber = 0;
			const TLineFormat *Format = nullptr;
			while (!Found) {
				LineNumber = CurrentLine;
				if (LineNumber < GetFirstLineNumber())
					break;
				Format = GetLineFormat(LineNumber);
				CurrentLine = LineNumber - 1;
				if (Format == nullptr || Format->LineText == nullptr)
					continue;
				int LineHeight = Graphics().GetFontLineHeight(Format->FontNumber);
				if (First) {
					LineHeight = Graphics().GetFontHeight(Format->FontNumber);
					First = false;
				}
				if (In(y, Bottom - LineHeight, Bottom)) {
					Found = true;
					break;
				}
				if (Bottom < 0)
					break;
				Bottom -= LineHeight;
			}
			if (!Found) {
				LineNumber = GetFirstLineNumber();
				Format = GetLineFormat(LineNumber);
				if (Format == nullptr || Format->LineText == nullptr)
					return false;
				x = 0;
			}
			int TextX;
			if (x < Format->Indent)
				TextX = 0;
			else
				TextX = x - Format->Indent;
			*Line = LineNumber;
			int Characters = Graphics().GetFittingTextLength(Format->FontNumber, Format->LineText, TextX);
			if (Characters > Format->LineLength)
				Characters = Format->LineLength;
			*Offset = Characters;
			*Length = Format->LineLength;
			return true;
		}
	}
	return false;
}

// --- TConsole --------------------------------------------------------------------------------

TConsole::TConsole()
{
	ScrollToEnd = false;
	DrawingListLength = 8;
	DrawingList.reset(new TDrawing[8]);
	TextWindow = new TTextWindow;
	ScrollBar = new TScrollBar(TRect(0, 0, SkinProperties().ScrollBarWidth, SkinProperties().ScrollBarMinimumHeight), 0, 0, 0, 0, 1);
	InsertChild(ScrollBar);
	InsertChild(TextWindow);
}

int TConsole::GetMinimumHeight()
{
	const int ScrollBarHeight = ScrollBar->GetMinimumHeight();
	const int TextHeight = TextWindow->GetMinimumHeight();
	return TextHeight > ScrollBarHeight ? TextHeight : ScrollBarHeight;
}

// The text follows the scroll bar.
void TConsole::Draw(int Surface, int x, int y)
{
	TextWindow->SetCurrentLineNumber(ScrollBar->Range->Value);
	TWindow::Draw(Surface, x, y);
}

// The wheel (Code 1 up, 2 down) moves the scroll bar.
bool TConsole::MouseWheel(int x, int y, int Code)
{
	if (Code != 1 && Code != 2)
		return TWindow::MouseWheel(x, y, Code);
	return ScrollBar->MouseWheel(x, y, Code);
}

// Places the scroll bar at the right edge (the skin 173 width cancels
// out), lays out the text, gives the scroll bar the range of bottom lines
// that fill the window and keeps the current line unless the window is to
// show the end; then the frame (skins 169-176).
void TConsole::Arrange()
{
	const int WindowWidth = Width;
	const int Top = Skins()[SKIN_CONSOLETEXT_EDGE_TOP].height;
	const int RightEdge = Skins()[SKIN_CONSOLETEXT_EDGE_RIGHT].width;
	const int LeftEdge = Skins()[SKIN_CONSOLETEXT_EDGE_LEFT].width;
	ScrollBar->SetSize(ScrollBar->Width, Height - Skins()[SKIN_CONSOLETEXT_EDGE_BOTTOM].height - Top);
	ScrollBar->SetPosition(WindowWidth - RightEdge - LeftEdge - ScrollBar->Width + LeftEdge, Top);
	SetChildArea(TextWindow, SkinProperties().ConsoleWindow.TextWindow);
	TWindow::Arrange();
	const int Visible = TextWindow->GetLastLineNumber() - TextWindow->GetFirstLineNumber() - TextWindow->InvisibleLines;
	const int Minimum = TextWindow->GetFirstLineNumber() + Visible;
	const int Maximum = TextWindow->GetLastLineNumber() - TextWindow->GetFirstLineNumber();
	if (Maximum < Minimum) {
		std::stringstream Error;
		Error << "invalid range for console text window scroll bar" << " (minimum: " << Minimum
			  << ", maximum: " << Maximum << ", visible: " << Visible << ", TotalLines: "
			  << TextWindow->GetLastLineNumber() - TextWindow->GetFirstLineNumber() + 1
			  << ", InvisibleLines: " << TextWindow->InvisibleLines << ", FirstLineNumber: "
			  << TextWindow->GetFirstLineNumber() << ", TotalLines: "
			  << TextWindow->GetLastLineNumber() - TextWindow->GetFirstLineNumber() + 1 << ")";
		TERROR(Error.str().c_str());
	}
	int Value = Maximum;
	if (!ScrollToEnd && !TextWindow->AtEnd)
		Value = ScrollBar->Range->Value;
	ScrollToEnd = false;
	ScrollBar->Range->SetRange(Minimum, Maximum, Value, Visible);
	TextWindow->SetCurrentLineNumber(ScrollBar->Range->Value);

	DrawingList[0] = TDrawing(SKIN_CONSOLETEXT_CORNER_TOPLEFT, 0, 0, 0, 0, 0, 0);
	DrawingList[1] = TDrawing(SKIN_CONSOLETEXT_EDGE_TOP, Skins()[SKIN_CONSOLETEXT_CORNER_TOPLEFT].width, 0, Width - Skins()[SKIN_CONSOLETEXT_CORNER_TOPRIGHT].width - Skins()[SKIN_CONSOLETEXT_CORNER_TOPLEFT].width,
		Skins()[SKIN_CONSOLETEXT_EDGE_TOP].height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_CONSOLETEXT_CORNER_TOPRIGHT, Width - Skins()[SKIN_CONSOLETEXT_CORNER_TOPRIGHT].width, 0, 0, 0, 0, 0);
	DrawingList[3] = TDrawing(SKIN_CONSOLETEXT_EDGE_LEFT, 0, Skins()[SKIN_CONSOLETEXT_CORNER_TOPLEFT].height, Skins()[SKIN_CONSOLETEXT_EDGE_LEFT].width,
		Height - Skins()[SKIN_CONSOLETEXT_CORNER_BOTTOMLEFT].height - Skins()[SKIN_CONSOLETEXT_CORNER_TOPLEFT].height, 0, 0);
	DrawingList[4] = TDrawing(SKIN_CONSOLETEXT_EDGE_RIGHT, Width - Skins()[SKIN_CONSOLETEXT_EDGE_RIGHT].width, Skins()[SKIN_CONSOLETEXT_CORNER_TOPRIGHT].height, Skins()[SKIN_CONSOLETEXT_EDGE_RIGHT].width,
		Height - Skins()[SKIN_CONSOLETEXT_CORNER_BOTTOMRIGHT].height - Skins()[SKIN_CONSOLETEXT_CORNER_TOPRIGHT].height, 0, 0);
	DrawingList[5] = TDrawing(SKIN_CONSOLETEXT_CORNER_BOTTOMLEFT, 0, Height - Skins()[SKIN_CONSOLETEXT_CORNER_BOTTOMLEFT].height, 0, 0, 0, 0);
	DrawingList[6] = TDrawing(SKIN_CONSOLETEXT_EDGE_BOTTOM, Skins()[SKIN_CONSOLETEXT_CORNER_BOTTOMLEFT].width, Height - Skins()[SKIN_CONSOLETEXT_EDGE_BOTTOM].height,
		Width - Skins()[SKIN_CONSOLETEXT_CORNER_BOTTOMRIGHT].width - Skins()[SKIN_CONSOLETEXT_CORNER_BOTTOMLEFT].width, Skins()[SKIN_CONSOLETEXT_EDGE_BOTTOM].height, 0, 0);
	DrawingList[7] = TDrawing(SKIN_CONSOLETEXT_CORNER_BOTTOMRIGHT, Width - Skins()[SKIN_CONSOLETEXT_CORNER_BOTTOMRIGHT].width, Height - Skins()[SKIN_CONSOLETEXT_CORNER_BOTTOMRIGHT].height, 0, 0, 0, 0);
}

// --- TTab ------------------------------------------------------------------------------------

// The caption is shortened to the tab's width less its margins.
TTab::TTab(const char *_Caption, TTalkBuffer *_TextBuffer, TCallback _Callback)
{
	PATELLA_ASSERT(_Caption!=nullptr);
	PATELLA_ASSERT(_TextBuffer!=nullptr);
	TextBuffer = _TextBuffer;
	LineCount = -1;
	BlinkEnd = 0;
	Active = false;
	Dragging = 0;
	DragX = 0;
	Unread = false;
	const std::size_t Size = strlen(_Caption) + 1;
	Caption.reset(new char[Size]);
	FullCaption.reset(new char[Size]);
	TrimString(Caption.get(), _Caption);
	TStringBuffer(FullCaption.get(), Size).Assign(Caption.get());
	Callback = _Callback;
	Shortened = false;
	const int Length = Graphics().GetFittingTextLength(SkinProperties().Tab.ActiveFont.FontNumber, Caption.get(),
		SkinProperties().Tab.Width - SkinProperties().Tab.CaptionMargin * 2);
	if (ShortenString(Caption.get(), Length))
		Shortened = true;
	SetSize(SkinProperties().Tab.Width, SkinProperties().Tab.Height);
}

// The destructor frees the captions; the close (OnClose) does the rest.
TTab::~TTab()
{
	CheckClosed("TTab");
	Caption.reset();
	if (FullCaption != nullptr)
		FullCaption.reset();
}

static_assert(std::is_nothrow_destructible_v<TTab> && std::is_nothrow_destructible_v<TConsoleWindow>,
	"issue #108: the console's destructors are noexcept");

// Leaves the channel on the server: the player's own request is cancelled, a
// gamemaster's open request closed, a public channel left; the player's own
// private channel only when it was closed by the server (Locked) and private
// channels only when the player may write. Then the buffer is deleted
// (Talk.cpp). An exception of the calls passes out of the close.
void TTab::OnClose()
{
	if (TextBuffer->IsRuleViolation() && TextBuffer->GetReceiveTalkMode() == TALK_GAMEMASTER_ANSWER) {
		SendCancelRuleViolation();
	} else if (TextBuffer->IsRuleViolation() && TextBuffer->GetReceiveTalkMode() == TALK_PLAYER_ANSWER
		&& TextBuffer->CanWrite()) {
		const char *PlayerName = TextBuffer->GetAddressee();
		PATELLA_ASSERT(PlayerName!=nullptr);
		SendCloseRuleViolation(PlayerName);
	} else if (!TextBuffer->IsPrivateChannel()) {
		if (TextBuffer->GetChannelID() != -1)
			SendLeaveChannel(TextBuffer->GetChannelID());
	} else if (!TextBuffer->IsSelected() || TextBuffer->IsLocked()) {
		if (TextBuffer->CanWrite())
			SendLeaveChannel(TextBuffer->GetChannelID());
	}
	Talk().DeleteTextBuffer(TextBuffer);
	TextBuffer = nullptr;
	TElement::OnClose();
}

void TTab::SetActive(bool _Active)
{
	Active = _Active;
	Unread = false;
	if (_Active == true)
		Arrange();
}

// Drags the tab along the bar.
bool TTab::MouseMove(int x, [[maybe_unused]] int y)
{
	if (Dragging) {
		TTabBar *Bar = static_cast<TTabBar *>(GetParent());
		x = OffsetX - DragX + x;
		if (x < 0)
			x = 0;
		if (x > Bar->Width - Width)
			x = Bar->Width - Width;
		SetPosition(x, OffsetY);
		Bar->Rearrange(this);
	}
	return true;
}

// Makes the tab current and starts dragging it.
bool TTab::LeftButtonDown(int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	Dragging = 1;
	SetCapture(nullptr);
	DragX = x;
	(static_cast<TTabBar *>(GetParent()))->SetCurrentTab(this);
	Callback.Send();
	return true;
}

bool TTab::LeftButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	if (Dragging) {
		ReleaseCapture(nullptr);
		(static_cast<TTabBar *>(GetParent()))->Rearrange(nullptr);
	}
	return true;
}

// Makes the tab current and opens the channel menu (Control.cpp's
// message 0x38).
bool TTab::RightButtonUp([[maybe_unused]] int x, [[maybe_unused]] int y, [[maybe_unused]] int Flags)
{
	static_cast<TTabBar *>(GetParent())->SetCurrentTab(this);
	Callback.Send();
	AddMessage(MESSAGE_CHANNEL_MENU, 0, 0);
	return true;
}

// A shortened caption shows the full one.
void TTab::MouseHover([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	if (Shortened)
		Interaction().SetMouseOverText(FullCaption.get());
}

// New lines light the tab up (blinking when the number of lines changed);
// a buffer that was read (TakeClosed) puts it out.
void TTab::Arrange()
{
	if (TextBuffer->TakeChanged()) {
		if (TextBuffer->lines != LineCount)
			BlinkEnd = GetActionNumber() + 10;
		LineCount = TextBuffer->lines;
		Unread = true;
	}
	if (TextBuffer->TakeClosed()) {
		Unread = false;
		BlinkEnd = 0;
	}
}

// Skin 186 for the active tab, 187 for the others.
void TTab::Draw(int Surface, int x, int y)
{
	PATELLA_ASSERT_VALUES(x>=0, x);
	PATELLA_ASSERT_VALUES(y>=0, y);
	DrawSkin(Surface, x, y, Width, Height, SKIN_TAB_INACTIVE - (Active ? 1 : 0), 0, 0);
	const TFont *Font;
	if (Active)
		Font = &SkinProperties().Tab.ActiveFont;
	else if (Unread && BlinkEnd == 0)
		Font = &SkinProperties().Tab.UnreadFont;
	else if (Unread && BlinkEnd != 0)
		Font = &SkinProperties().Tab.BlinkingFont;
	else
		Font = &SkinProperties().Tab.InactiveFont;
	Graphics().PrintFormattedText(Surface, SkinProperties().Tab.Caption.x + x, SkinProperties().Tab.Caption.y + y,
		*Font, Caption.get());
	if (BlinkEnd != 0 && BlinkEnd < GetActionNumber())
		BlinkEnd = 0;
}

// --- TTabBar ---------------------------------------------------------------------------------

// Arrange runs before Busy is set (it reads the uninitialised value);
// Changed stays uninitialised.
TTabBar::TTabBar()
{
	Moved = nullptr;
	CurrentTab = nullptr;
	LeftTabs.reset(new TWindow);
	RightTabs.reset(new TWindow);
	Arrange();
	Busy = false;
}

// The dragged tab is drawn last, over the others.
void TTabBar::Draw(int Surface, int x, int y)
{
	// NOLINTNEXTLINE(bugprone-parent-virtual-call): the original's call of the base's base (the image's)
	TComponent::Draw(Surface, x, y);
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->GetNext()) {
		if (Child != Moved)
			Child->Draw(Surface, Child->OffsetX + x, Child->OffsetY + y);
	}
	if (Moved != nullptr)
		Moved->Draw(Surface, Moved->OffsetX + x, Moved->OffsetY + y);
}

void TTabBar::SetSize(int Width, int Height)
{
	TComponent::SetSize(Width, Height);
	Rearrange(nullptr);
	Changed = true;
}

void TTabBar::Arrange()
{
	TWindow::Arrange();
	RightTabs->Arrange();
	LeftTabs->Arrange();
	Rearrange(nullptr);
}

void TTabBar::SetCurrentTab(TTab *Tab)
{
	PATELLA_ASSERT(Tab!=nullptr);
	const TWindow *Container = GetContainer(Tab);
	PATELLA_ASSERT(Container!=nullptr);
	Changed = true;
	if (CurrentTab != nullptr)
		CurrentTab->SetActive(false);
	Tab->SetActive(true);
	CurrentTab = Tab;
	ShowTab(Tab);
}

void TTabBar::InsertChild(TComponent *Child)
{
	TWindow::InsertChild(Child);
	Changed = true;
	Rearrange(nullptr);
	Child->Arrange();
}

void TTabBar::AppendChild(TComponent *Child)
{
	TWindow::AppendChild(Child);
	Changed = true;
	Rearrange(nullptr);
	Child->Arrange();
}

void TTabBar::DeleteChild(TComponent *Child)
{
	TWindow *Container = GetContainer(Child);
	if (Container == this)
		TWindow::DeleteChild(Child);
	else
		Container->DeleteChild(Child);
	Rearrange(nullptr);
	Changed = true;
}

void TTabBar::InsertChildAfter(TComponent *Previous, TComponent *Child)
{
	TWindow::InsertChildAfter(Previous, Child);
	Rearrange(nullptr);
	Child->Arrange();
	Changed = true;
}

void TTabBar::SwapChildren(TComponent *Child1, TComponent *Child2)
{
	TWindow::SwapChildren(Child1, Child2);
	Changed = true;
}

void TTabBar::RemoveChild(TComponent *Child)
{
	TWindow *Container = GetContainer(Child);
	if (Container == this)
		TWindow::RemoveChild(Child);
	else
		Container->RemoveChild(Child);
	Rearrange(nullptr);
	Changed = true;
}

bool TTabBar::IsChild(TComponent *Child)
{
	return TWindow::IsChild(Child) || LeftTabs->IsChild(Child) || RightTabs->IsChild(Child);
}

// The window that holds the tab.
TWindow *TTabBar::GetContainer(TComponent *Child)
{
	if (TWindow::IsChild(Child))
		return this;
	if (LeftTabs->IsChild(Child))
		return LeftTabs.get();
	if (RightTabs->IsChild(Child))
		return RightTabs.get();
	TERROR("failed to get composite");
}

// The help of the console (handle 23) for the whole parent.
bool TTabBar::HelpMouseMove([[maybe_unused]] int x, [[maybe_unused]] int y)
{
	Help().SetHelpHandle(HELP_CONSOLE_CHANNELS);
	const TComponent *Parent = GetParent();
	PATELLA_ASSERT(Parent!=nullptr);
	Help().SetHelpSize(Parent->Width, Parent->Height);
	Help().MoveHelpPosition(-OffsetX, -OffsetY);
	return true;
}

// 5 brings the last hidden tab on the left back, 6 hides the first tab on
// the left while tabs are hidden on the right; 25 and 26 do the same until
// no tab is hidden on that side.
void TTabBar::HandleEvent(int Event)
{
	if (Event == EVENT_TABS_MOVE_LEFT) {
		if (RightTabs->GetFirstChild() == nullptr)
			return;
		TComponent *Tab = GetFirstChild();
		if (Tab == nullptr)
			return;
		TWindow::RemoveChild(Tab);
		LeftTabs->AppendChild(Tab);
		Changed = true;
	} else if (Event == EVENT_TABS_MOVE_RIGHT) {
		TComponent *Tab = LeftTabs->GetLastChild();
		if (Tab == nullptr)
			return;
		LeftTabs->RemoveChild(Tab);
		InsertChild(Tab);
		Changed = true;
	} else if (Event == EVENT_TABS_MOVE_ALL_RIGHT) {
		while (LeftTabs->GetFirstChild() != nullptr)
			HandleEvent(EVENT_TABS_MOVE_RIGHT);
	} else if (Event == EVENT_TABS_MOVE_ALL_LEFT) {
		while (RightTabs->GetFirstChild() != nullptr)
			HandleEvent(EVENT_TABS_MOVE_LEFT);
	}
	Rearrange(nullptr);
}

// The tab of a buffer, shown or hidden.
TTab *TTabBar::FindTab(TTalkBuffer *TextBuffer)
{
	for (TComponent *Child = FirstChild; Child != nullptr; Child = Child->GetNext()) {
		if ((static_cast<TTab *>(Child))->TextBuffer == TextBuffer)
			return static_cast<TTab *>(Child);
	}
	for (TComponent *Child = LeftTabs->GetFirstChild(); Child != nullptr; Child = Child->GetNext()) {
		if ((static_cast<TTab *>(Child))->TextBuffer == TextBuffer)
			return static_cast<TTab *>(Child);
	}
	for (TComponent *Child = RightTabs->GetFirstChild(); Child != nullptr; Child = Child->GetNext()) {
		if ((static_cast<TTab *>(Child))->TextBuffer == TextBuffer)
			return static_cast<TTab *>(Child);
	}
	return nullptr;
}

// A hidden tab comes back as the last but one shown (it is appended and
// swapped with the last).
void TTabBar::ShowTab(TTab *Tab)
{
	if (GetContainer(Tab) != this) {
		RemoveChild(Tab);
		TComponent *Last = GetLastChild();
		TWindow::AppendChild(Tab);
		SwapChildren(Tab, Last);
		Rearrange(nullptr);
		Changed = true;
	}
}

// Closes the left window, then the right one (each closes its tabs), then the
// bar's own tabs. An exception out of the left window's tabs leaves the right
// one (the original's delete of it did not run) and closes the bar's own tabs
// first.
void TTabBar::OnClose()
{
	try {
		LeftTabs->Close();
		RightTabs->Close();
	} catch (...) {
		CloseWhileUnwinding([this] { TWindow::OnClose(); });
		throw;
	}
	TWindow::OnClose();
}

// The public channels of all tabs go to the preferences (Patella.ini's
// OpenChannel).
void TTabBar::StoreOpenChannels()
{
	ClearOpenChannels();
	for (int i = 0; i < 3; i++) {
		TWindow *Container;
		if (i == 0)
			Container = LeftTabs.get();
		else if (i == 2)
			Container = RightTabs.get();
		else
			Container = this;
		TComponent *Child = Container->GetFirstChild();
		while (Child != nullptr) {
			TComponent *Next = Child->GetNext();
			TTalkBuffer *TextBuffer = (static_cast<TTab *>(Child))->TextBuffer;
			Child = Next;
			if (TextBuffer != nullptr && TextBuffer->GetChannelID() != -1 && !TextBuffer->IsPrivateChannel())
				AddOpenChannel(TextBuffer->GetChannelID());
		}
	}
}

// A dragged tab changes places with a neighbour it covers by more than
// half; then the tabs are laid out from the left: those that do not fit
// move to the right window, hidden tabs on the right come back while they
// fit (the position also counts the tabs that stay hidden), and an empty
// bar takes the last tab hidden on the left.
void TTabBar::Rearrange(TTab *_Moved)
{
	// The constructor's Arrange calls this before it sets Busy, as the original's does: Busy's byte as the original
	// reads it (not 0: busy), whatever the memory held (a bool that is neither false nor true is undefined, and GCC and
	// Clang may test one bit of it; UBSan reports it).
	uint8 BusyByte = 0;
	std::memcpy(&BusyByte, &Busy, sizeof(BusyByte));
	if (BusyByte == 0) {
		Busy = true;
		Moved = _Moved;
		if (_Moved != nullptr) {
			TComponent *Previous = nullptr;
			TComponent *Search = FirstChild;
			TComponent *Next;
			for (;;) {
				PATELLA_ASSERT(Search!=nullptr);
				Next = Search->GetNext();
				if (Moved == Search)
					break;
				Previous = Search;
				Search = Next;
			}
			if (Previous != nullptr && Moved->OffsetX < Previous->Width / 2 + Previous->OffsetX)
				SwapChildren(Previous, Moved);
			if (Next != nullptr && Next->Width / 2 + Next->OffsetX < Moved->Width + Moved->OffsetX)
				SwapChildren(Next, Moved);
		}
		int x = 0;
		TComponent *Child = FirstChild;
		while (Child != nullptr) {
			TComponent *Next = Child->GetNext();
			if (Moved != Child)
				Child->SetPosition(x, Child->OffsetY);
			// MSVC's analysis (C6011): Child is not null inside the loop (its condition); the path it reports takes the
			// assertion's loop above as leaving Next null and Child with it.
			PATELLA_MSVC_WARNING(suppress : 6011)
			x += Child->Width;
			if (x > Width) {
				RemoveChild(Child);
				RightTabs->InsertChild(Child);
			}
			Child = Next;
		}
		Child = RightTabs->GetFirstChild();
		while (Child != nullptr) {
			TComponent *Next = Child->GetNext();
			if (Child->Width + x <= Width) {
				RightTabs->RemoveChild(Child);
				AppendChild(Child);
				Child->SetPosition(x, Child->OffsetY);
			}
			x += Child->Width;
			Child = Next;
		}
		Child = LeftTabs->GetLastChild();
		if (Child != nullptr && x == 0) {
			LeftTabs->RemoveChild(Child);
			InsertChild(Child);
			Child->SetPosition(0, Child->OffsetY);
		}
		Busy = false;
	}
}

// --- TConsoleWindow --------------------------------------------------------------------------

// The close and server message buttons and the arrows are children only
// while UpdateButtons shows them. Hotkeys: Ctrl+I ignore list, Ctrl+O new
// channel, Ctrl+E close, Ctrl+M server messages, Ctrl+K hotkey options,
// Ctrl+D the default channel. The channels of the last session are joined
// again.
TConsoleWindow::TConsoleWindow()
{
	DrawingListLength = 9;
	DrawingList.reset(new TDrawing[9]);
	TalkMode = TALK_SAY;
	DefaultTab = nullptr;
	HistoryPosition = -1;
	History.reset(new TTalkBuffer(TALK_NONE));
	Input = new TEditLine(TRect(0, 0, 0, 0), 255, false);
	InsertChild(Input);
	TabBar = new TTabBar;
	InsertChild(TabBar);
	CurrentConsole = new TConsole;
	InsertChild(CurrentConsole);
	VolumeButton = new TMultiStateButton(0, 0, TextAdjustVolume, 3, SKIN_SAY_BUTTON_UP, SKIN_SAY_BUTTON_DOWN, TCallback(this, EVENT_VOLUME_YELL), SKIN_YELL_BUTTON_UP, SKIN_YELL_BUTTON_DOWN,
		TCallback(this, EVENT_VOLUME_WHISPER), SKIN_WHISPER_BUTTON_UP, SKIN_WHISPER_BUTTON_DOWN, TCallback(this, EVENT_VOLUME_SAY));
	InsertChild(VolumeButton);
	CloseButton = new TSkinButton(0, 0, SKIN_CLOSE_CHANNEL_BUTTON_UP, SKIN_CLOSE_CHANNEL_BUTTON_DOWN, TCallback(this, EVENT_CLOSE_CHANNEL), TextCloseChannel, false);
	ServerMessagesButton = new TToggleButton(0, 0, SKIN_SERVER_MESSAGES_BUTTON_UP, SKIN_SERVER_MESSAGES_BUTTON_DOWN, TCallback(this, EVENT_SERVER_MESSAGES), TextShowServerMessages,
		TextHideServerMessages, false);
	IgnoreButton = new TSkinButton(0, 0, SKIN_IGNORE_BUTTON_UP, SKIN_IGNORE_BUTTON_DOWN, TCallback(this, EVENT_IGNORE_LIST), TextIgnorePlayers, false);
	InsertChild(IgnoreButton);
	NewChannelButton = new TSkinButton(0, 0, SKIN_NEW_CHANNEL_BUTTON_UP, SKIN_NEW_CHANNEL_BUTTON_DOWN, TCallback(this, EVENT_REQUEST_CHANNELS), TextOpenNewChannel, false);
	InsertChild(NewChannelButton);
	MoveRightButton = new TSkinButton(0, 0, SKIN_TABS_RIGHT_BUTTON_UP, SKIN_TABS_RIGHT_BUTTON_DOWN, TCallback(TabBar, EVENT_TABS_MOVE_RIGHT), TextMoveChannelsRight, false);
	InsertChild(MoveRightButton);
	MoveLeftButton = new TSkinButton(0, 0, SKIN_TABS_LEFT_BUTTON_UP, SKIN_TABS_LEFT_BUTTON_DOWN, TCallback(TabBar, EVENT_TABS_MOVE_LEFT), TextMoveChannelsLeft, false);
	InsertChild(MoveLeftButton);
	AddHotkey(KEY_CONTROL | 'I', TCallback(this, EVENT_IGNORE_LIST));
	AddHotkey(KEY_CONTROL | 'O', TCallback(this, EVENT_REQUEST_CHANNELS));
	AddHotkey(KEY_CONTROL | 'E', TCallback(this, EVENT_CLOSE_CHANNEL));
	AddHotkey(KEY_CONTROL | 'M', TCallback(this, EVENT_TOGGLE_SERVER_MESSAGES));
	AddHotkey(KEY_CONTROL | 'K', TCallback(this, EVENT_OPTIONS_HOTKEYS));
	AddHotkey(KEY_CONTROL | 'D', TCallback(this, EVENT_DEFAULT_CHANNEL));
	SwitchTime = GetActionNumber() + 50;
	int *Channels;
	const int Count = GetOpenChannels(&Channels);
	for (int i = 0; i < Count; i++)
		SendJoinChannel(Channels[i]);
	if (Count > 0)
		delete[] Channels;
}

// Frees the history and deletes the buttons that are not children (the close
// closed them); the close (OnClose) does the rest.
TConsoleWindow::~TConsoleWindow()
{
	CheckClosed("TConsoleWindow");
	History.reset();
	if (!IsChild(VolumeButton) && VolumeButton != nullptr)
		delete VolumeButton;
	if (!IsChild(CloseButton) && CloseButton != nullptr)
		delete CloseButton;
	if (!IsChild(ServerMessagesButton) && ServerMessagesButton != nullptr)
		delete ServerMessagesButton;
	if (!IsChild(MoveRightButton) && MoveRightButton != nullptr)
		delete MoveRightButton;
	if (!IsChild(MoveLeftButton) && MoveLeftButton != nullptr)
		delete MoveLeftButton;
}

// Stores the open channels and the console height, closes the buttons that
// are not children, removes the hotkeys; then the children (the tab bar with
// its tabs). An exception of the calls passes out of the close after the
// children's closes.
void TConsoleWindow::OnClose()
{
	try {
		TabBar->StoreOpenChannels();
		SetConsoleHeight(Height);
		if (!IsChild(VolumeButton) && VolumeButton != nullptr)
			VolumeButton->Close();
		if (!IsChild(CloseButton) && CloseButton != nullptr)
			CloseButton->Close();
		if (!IsChild(ServerMessagesButton) && ServerMessagesButton != nullptr)
			ServerMessagesButton->Close();
		if (!IsChild(MoveRightButton) && MoveRightButton != nullptr)
			MoveRightButton->Close();
		if (!IsChild(MoveLeftButton) && MoveLeftButton != nullptr)
			MoveLeftButton->Close();
		RemoveHotkey(KEY_CONTROL | 'I');
		RemoveHotkey(KEY_CONTROL | 'O');
		RemoveHotkey(KEY_CONTROL | 'E');
		RemoveHotkey(KEY_CONTROL | 'M');
		RemoveHotkey(KEY_CONTROL | 'K');
		RemoveHotkey(KEY_CONTROL | 'D');
	} catch (...) {
		CloseWhileUnwinding([this] { TWindow::OnClose(); });
		throw;
	}
	TWindow::OnClose();
}

void TConsoleWindow::SetSize(int Width, int Height)
{
	TComponent::SetSize(Width, Height);
	SetChildArea(TabBar, SkinProperties().ConsoleWindow.TabBar);
	SetChildArea(Input, SkinProperties().ConsoleWindow.Input);
	SetChildArea(CurrentConsole, SkinProperties().ConsoleWindow.CurrentConsole);
	if (IsChild(VolumeButton))
		SetChildPosition(VolumeButton, SkinProperties().ConsoleWindow.VolumeButton.x, SkinProperties().ConsoleWindow.VolumeButton.y);
	SetChildPosition(IgnoreButton, SkinProperties().ConsoleWindow.IgnoreButton.x, SkinProperties().ConsoleWindow.IgnoreButton.y);
	SetChildPosition(NewChannelButton, SkinProperties().ConsoleWindow.NewChannelButton.x, SkinProperties().ConsoleWindow.NewChannelButton.y);
	if (IsChild(CloseButton))
		SetChildPosition(CloseButton, SkinProperties().ConsoleWindow.CloseButton.x, SkinProperties().ConsoleWindow.CloseButton.y);
	if (IsChild(ServerMessagesButton))
		SetChildPosition(ServerMessagesButton, SkinProperties().ConsoleWindow.ServerMessagesButton.x, SkinProperties().ConsoleWindow.ServerMessagesButton.y);
	if (MoveRightButton != nullptr && IsChild(MoveRightButton))
		SetChildPosition(MoveRightButton, SkinProperties().ConsoleWindow.MoveRightButton.x, SkinProperties().ConsoleWindow.MoveRightButton.y);
	if (MoveLeftButton != nullptr && IsChild(MoveLeftButton))
		SetChildPosition(MoveLeftButton, SkinProperties().ConsoleWindow.MoveLeftButton.x, SkinProperties().ConsoleWindow.MoveLeftButton.y);
	Arrange();
}

// Talk.cpp's changes rearrange the tabs and the text; the ignore button's
// skin shows the talk throttle; the buttons follow the tab bar's changes.
void TConsoleWindow::Draw(int Surface, int x, int y)
{
	if (Talk().TakeConsoleChanged()) {
		TabBar->Arrange();
		PATELLA_ASSERT(CurrentConsole!=nullptr);
		TTalkBuffer *TextBuffer = CurrentConsole->GetTextBuffer();
		PATELLA_ASSERT(TextBuffer!=nullptr);
		CurrentConsole->Arrange();
		SetActive(TextBuffer->CanWrite());
		Input->SetDisabled(!TextBuffer->CanWrite());
	}
	IgnoreButton->SetSkins(Talk().IsTalkThrottled() ? SKIN_IGNORE_BUTTON_UP_THROTTLED : SKIN_IGNORE_BUTTON_UP, SKIN_IGNORE_BUTTON_DOWN);
	const bool Changed = TabBar->Changed;
	TabBar->Changed = false;
	if (Changed)
		UpdateButtons();
	TWindow::Draw(Surface, x, y);
}

// The frame: skins 177-184 and the tiled background 185, aligned to the
// window's position.
void TConsoleWindow::Arrange()
{
	TWindow::Arrange();
	DrawingList[0] = TDrawing(SKIN_CONSOLE_CORNER_TOPLEFT, 0, 0, 0, 0, 0, 0);
	DrawingList[1] = TDrawing(SKIN_CONSOLE_EDGE_TOP, Skins()[SKIN_CONSOLE_CORNER_TOPLEFT].width, 0, Width - Skins()[SKIN_CONSOLE_CORNER_TOPRIGHT].width - Skins()[SKIN_CONSOLE_CORNER_TOPLEFT].width,
		Skins()[SKIN_CONSOLE_EDGE_TOP].height, 0, 0);
	DrawingList[2] = TDrawing(SKIN_CONSOLE_CORNER_TOPRIGHT, Width - Skins()[SKIN_CONSOLE_CORNER_TOPRIGHT].width, 0, 0, 0, 0, 0);
	DrawingList[3] = TDrawing(SKIN_CONSOLE_EDGE_LEFT, 0, Skins()[SKIN_CONSOLE_CORNER_TOPLEFT].height, Skins()[SKIN_CONSOLE_EDGE_LEFT].width,
		Height - Skins()[SKIN_CONSOLE_CORNER_BOTTOMLEFT].height - Skins()[SKIN_CONSOLE_CORNER_TOPLEFT].height, 0, 0);
	DrawingList[4] = TDrawing(SKIN_CONSOLE_EDGE_RIGHT, Width - Skins()[SKIN_CONSOLE_EDGE_RIGHT].width, Skins()[SKIN_CONSOLE_CORNER_TOPRIGHT].height, Skins()[SKIN_CONSOLE_EDGE_RIGHT].width,
		Height - Skins()[SKIN_CONSOLE_CORNER_BOTTOMRIGHT].height - Skins()[SKIN_CONSOLE_CORNER_TOPRIGHT].height, 0, 0);
	DrawingList[5] = TDrawing(SKIN_CONSOLE_CORNER_BOTTOMLEFT, 0, Height - Skins()[SKIN_CONSOLE_CORNER_BOTTOMLEFT].height, 0, 0, 0, 0);
	DrawingList[6] = TDrawing(SKIN_CONSOLE_EDGE_BOTTOM, Skins()[SKIN_CONSOLE_CORNER_BOTTOMLEFT].width, Height - Skins()[SKIN_CONSOLE_EDGE_BOTTOM].height,
		Width - Skins()[SKIN_CONSOLE_CORNER_BOTTOMRIGHT].width - Skins()[SKIN_CONSOLE_CORNER_BOTTOMLEFT].width, Skins()[SKIN_CONSOLE_EDGE_BOTTOM].height, 0, 0);
	DrawingList[7] = TDrawing(SKIN_CONSOLE_CORNER_BOTTOMRIGHT, Width - Skins()[SKIN_CONSOLE_CORNER_BOTTOMRIGHT].width, Height - Skins()[SKIN_CONSOLE_CORNER_BOTTOMRIGHT].height, 0, 0, 0, 0);
	DrawingList[8] = TDrawing(SKIN_CONSOLE_BACKGROUND, Skins()[SKIN_CONSOLE_EDGE_LEFT].width, Skins()[SKIN_CONSOLE_EDGE_TOP].height, Width - Skins()[SKIN_CONSOLE_EDGE_RIGHT].width - Skins()[SKIN_CONSOLE_EDGE_LEFT].width,
		Height - Skins()[SKIN_CONSOLE_EDGE_BOTTOM].height - Skins()[SKIN_CONSOLE_EDGE_TOP].height, 0, OffsetY % Skins()[SKIN_CONSOLE_BACKGROUND].height);
}

// A click clears both selections.
bool TConsoleWindow::LeftButtonDown(int x, int y, int Flags)
{
	TTextWindow *TextWindow = CurrentConsole->TextWindow;
	TextWindow->SelectionStartLine = -1;
	TextWindow->SelectionStartOffset = -1;
	TextWindow->SelectionEndLine = -1;
	TextWindow->SelectionEndOffset = -1;
	Input->SelectionLength = 0;
	return TWindow::LeftButtonDown(x, y, Flags);
}

// The console's own help (handle 22) where no child has one.
bool TConsoleWindow::HelpMouseMove(int x, int y)
{
	if (!TWindow::HelpMouseMove(x, y)) {
		Help().SetHelpHandle(HELP_CONSOLE);
		Help().SetHelpSize(Width, Height);
	}
	return true;
}

// Tab and Shift+Tab switch channels, Ctrl+A selects all of the edit line (of
// the text when the line is empty); the copy, cut and delete keys work on
// the selection (the text's first). In a channel the player may write in the
// key goes to the edit line: Enter sends it, F1-F12 (+12 with Shift, +24
// with Ctrl) insert a hotkey text (and send it when it is an automatic one),
// the arrow keys 0x10f and 0x10c walk through the sent lines (one step below
// the last stores the edit line).
void TConsoleWindow::KeyPress(int Key)
{
	if (Key == KEY_TAB)
		HandleEvent(EVENT_NEXT_CHANNEL);
	if (Key == (KEY_SHIFT | KEY_TAB))
		HandleEvent(EVENT_PREVIOUS_CHANNEL);
	TComponent *Selection;
	if (CurrentConsole->TextWindow->HasSelection())
		Selection = CurrentConsole->TextWindow;
	else
		Selection = Input->HasSelection() ? Input : nullptr;
	if (Key == (KEY_CONTROL | 'a')) {
		TTextWindow *TextWindow = CurrentConsole->TextWindow;
		TextWindow->SelectionStartLine = -1;
		TextWindow->SelectionStartOffset = -1;
		TextWindow->SelectionEndLine = -1;
		TextWindow->SelectionEndOffset = -1;
		Input->SelectionLength = 0;
		TComponent *Target = Input;
		if (strlen(Input->Text.get()) == 0)
			Target = CurrentConsole->TextWindow;
		Target->HandleEvent(EVENT_SELECT_ALL);
		return;
	}
	if (Selection != nullptr) {
		bool Done = false;
		if (Key == (KEY_CONTROL | KEY_INSERT) || Key == (KEY_CONTROL | 'c')) {
			Selection->HandleEvent(EVENT_COPY);
			Done = true;
		}
		if (Key == (KEY_SHIFT | KEY_DELETE) || Key == (KEY_CONTROL | 'x')) {
			Selection->HandleEvent(EVENT_CUT);
			Done = true;
		}
		if (Key == (KEY_CONTROL | KEY_DELETE)) {
			Selection->HandleEvent(EVENT_DELETE);
			return;
		}
		if (Done)
			return;
	}
	TTalkBuffer *TextBuffer = CurrentConsole->GetTextBuffer();
	PATELLA_ASSERT(TextBuffer!=nullptr);
	if (!TextBuffer->CanWrite())
		return;
	Input->KeyPress(Key);
	if (Key == KEY_RETURN)
		SendInput();
	int Number;
	switch (Key & KEY_CODE_MASK) {
	case KEY_F1: Number = 0; break;
	case KEY_F2: Number = 1; break;
	case KEY_F3: Number = 2; break;
	case KEY_F4: Number = 3; break;
	case KEY_F5: Number = 4; break;
	case KEY_F6: Number = 5; break;
	case KEY_F7: Number = 6; break;
	case KEY_F8: Number = 7; break;
	case KEY_F9: Number = 8; break;
	case KEY_F10: Number = 9; break;
	case KEY_F11: Number = 10; break;
	case KEY_F12: Number = 11; break;
	default: {
		bool Moved = false;
		const int Code = Key % (KEY_CODE_MASK + 1);
		if (Code == KEY_DOWN && HistoryPosition > -2) {
			HistoryPosition--;
			Moved = true;
		}
		if (Code == KEY_UP && HistoryPosition < History->lines - 1)
			HistoryPosition++;
		else if (!Moved)
			return;
		if (HistoryPosition >= 0) {
			const TTalkLine *Line = History->GetLine(History->lines - HistoryPosition - 1);
			if (Line == nullptr)
				return;
			Input->SetText(Line->Text);
			return;
		}
		if (HistoryPosition == -2) {
			AddToHistory();
			Input->Clear();
			return;
		}
		Input->Clear();
		return;
	}
	}
	if ((Key & KEY_SHIFT) != 0)
		Number += 12;
	else if ((Key & KEY_CONTROL) != 0)
		Number += 24;
	bool AutoSend;
	const char *Text = GetHotkey(Number, &AutoSend);
	if (Text != nullptr) {
		Input->SetText(Text);
		if (AutoSend)
			SendInput();
	}
}

// 201 shows the current tab's channel, 202 closes it first, 203 and 210
// (Ctrl+M, which toggles the button) show or hide the server messages, 204
// opens the ignore list (Control.cpp's message 0x15), 205 asks for the
// channel list, 206-208 set the volume, 211 (Ctrl+D) goes to the default
// channel, 212 and 213 (Tab, Shift+Tab) to the next and the previous one,
// scrolling the hidden tabs in (all of them at the ends), 403 (Ctrl+K) opens
// the hotkey options (message 0x26).
void TConsoleWindow::HandleEvent(int Event)
{
	switch (Event) {
	case EVENT_CLOSE_CHANNEL:
		CloseChannel();
		[[fallthrough]];
	case EVENT_SHOW_CHANNEL:
		CurrentConsole->TextWindow->SetTextBuffer(TabBar->CurrentTab->TextBuffer);
		CurrentConsole->ScrollToEnd = true;
		CurrentConsole->Arrange();
		UpdateButtons();
		SetSize(Width, Height);
		CurrentConsole->Arrange();
		break;
	case EVENT_TOGGLE_SERVER_MESSAGES:
		ServerMessagesButton->State = !ServerMessagesButton->State;
		[[fallthrough]];
	case EVENT_SERVER_MESSAGES:
		CurrentConsole->GetTextBuffer()->ShowMessages = ServerMessagesButton->State;
		break;
	case EVENT_IGNORE_LIST:
		AddMessage(MESSAGE_IGNORE_LIST_DIALOG, 0, 0);
		break;
	case EVENT_REQUEST_CHANNELS:
		SendRequestChannels();
		break;
	case EVENT_VOLUME_SAY:
		TalkMode = TALK_SAY;
		break;
	case EVENT_VOLUME_WHISPER:
		TalkMode = TALK_WHISPER;
		break;
	case EVENT_VOLUME_YELL:
		TalkMode = TALK_YELL;
		break;
	case EVENT_DEFAULT_CHANNEL:
		TabBar->SetCurrentTab(DefaultTab);
		HandleEvent(EVENT_SHOW_CHANNEL);
		break;
	case EVENT_NEXT_CHANNEL: {
		TTab *Tab = TabBar->CurrentTab;
		if (Tab->GetNext() == nullptr)
			TabBar->HandleEvent(EVENT_TABS_MOVE_LEFT);
		TComponent *NewTab = Tab->GetNext();
		if (NewTab == nullptr) {
			TabBar->HandleEvent(EVENT_TABS_MOVE_ALL_RIGHT);
			NewTab = TabBar->GetFirstChild();
		}
		TabBar->SetCurrentTab(static_cast<TTab *>(NewTab));
		HandleEvent(EVENT_SHOW_CHANNEL);
		break;
	}
	case EVENT_PREVIOUS_CHANNEL: {
		TTab *Tab = TabBar->CurrentTab;
		if (Tab->GetPrevious() == nullptr)
			TabBar->HandleEvent(EVENT_TABS_MOVE_RIGHT);
		TComponent *NewTab = Tab->GetPrevious();
		if (NewTab == nullptr) {
			TabBar->HandleEvent(EVENT_TABS_MOVE_ALL_LEFT);
			NewTab = TabBar->GetLastChild();
		}
		TabBar->SetCurrentTab(static_cast<TTab *>(NewTab));
		HandleEvent(EVENT_SHOW_CHANNEL);
		break;
	}
	case EVENT_OPTIONS_HOTKEYS:
		AddMessage(MESSAGE_HOTKEY_DIALOG, 0, 0);
		break;
	default:
		TERROR1("unknown event", Event);
	}
}

// Activates the edit line, never in a channel the player may not write in;
// it is not deactivated while a context menu is open (action state 11).
void TConsoleWindow::SetActive(bool Active) const
{
	TTalkBuffer *TextBuffer = CurrentConsole->GetTextBuffer();
	PATELLA_ASSERT(TextBuffer!=nullptr);
	if (!TextBuffer->CanWrite())
		Active = false;
	PATELLA_ASSERT(Input!=nullptr);
	if (!Active && Interaction().GetActionState() == ACTIONSTATE_CONTEXT_MENU)
		return;
	Input->SetActive(Active);
}

// The buttons of the current channel: the arrows while tabs are hidden on
// their side, the close and server message buttons in every channel but the
// default one, the volume in the default and the request channels (which
// have the default channel's font).
void TConsoleWindow::UpdateButtons()
{
	TTalkBuffer *TextBuffer = CurrentConsole->GetTextBuffer();
	PATELLA_ASSERT(TextBuffer!=nullptr);
	if (IsChild(VolumeButton))
		RemoveChild(VolumeButton);
	if (IsChild(CloseButton))
		RemoveChild(CloseButton);
	if (IsChild(ServerMessagesButton))
		RemoveChild(ServerMessagesButton);
	if (IsChild(MoveRightButton) && TabBar->LeftTabs->GetFirstChild() == nullptr)
		RemoveChild(MoveRightButton);
	if (!IsChild(MoveRightButton) && TabBar->LeftTabs->GetFirstChild() != nullptr) {
		InsertChild(MoveRightButton);
		SetChildPosition(MoveRightButton, SkinProperties().ConsoleWindow.MoveRightButton.x, SkinProperties().ConsoleWindow.MoveRightButton.y);
	}
	if (IsChild(MoveLeftButton) && TabBar->RightTabs->GetFirstChild() == nullptr)
		RemoveChild(MoveLeftButton);
	if (!IsChild(MoveLeftButton) && TabBar->RightTabs->GetFirstChild() != nullptr) {
		InsertChild(MoveLeftButton);
		SetChildPosition(MoveLeftButton, SkinProperties().ConsoleWindow.MoveLeftButton.x, SkinProperties().ConsoleWindow.MoveLeftButton.y);
	}
	if (!TextBuffer->IsDefault()) {
		InsertChild(CloseButton);
		SetChildPosition(CloseButton, SkinProperties().ConsoleWindow.CloseButton.x, SkinProperties().ConsoleWindow.CloseButton.y);
		InsertChild(ServerMessagesButton);
		SetChildPosition(ServerMessagesButton, SkinProperties().ConsoleWindow.ServerMessagesButton.x, SkinProperties().ConsoleWindow.ServerMessagesButton.y);
		ServerMessagesButton->State = TextBuffer->ShowMessages;
	}
	TFont Font = SkinProperties().ConsoleWindow.InputFont;
	if (TextBuffer->AcceptMessages()) {
		InsertChild(VolumeButton);
		SetChildPosition(VolumeButton, SkinProperties().ConsoleWindow.VolumeButton.x, SkinProperties().ConsoleWindow.VolumeButton.y);
		Font = SkinProperties().EditLine.Font;
	}
	Input->SetFont(Font);
	Input->SetDisabled(!TextBuffer->CanWrite());
	SetActive(TextBuffer->CanWrite());
}

// MainWindow.cpp opens the default channel ("Default").
void TConsoleWindow::OpenDefaultChannel(TTalkBuffer *TextBuffer, const char *Caption)
{
	DefaultTab = new TTab(Caption, TextBuffer, TCallback(this, EVENT_SHOW_CHANNEL));
	TabBar->InsertChild(DefaultTab);
	TabBar->SetCurrentTab(DefaultTab);
	HandleEvent(EVENT_SHOW_CHANNEL);
}

// Creates the buffer of a channel (or takes the existing one) and gives it
// a tab.
TTalkBuffer *TConsoleWindow::OpenChannel(int RecvMode, int SendMode, const char *Name, int Channel,
	const char *Caption)
{
	TTalkBuffer *TextBuffer = Talk().CreateTextBuffer(RecvMode, SendMode, Name, Channel);
	if (TextBuffer == nullptr) {
		TextBuffer = Talk().GetTextBuffer(RecvMode, Name, Channel);
		PATELLA_ASSERT(TextBuffer!=nullptr);
	}
	AddChannel(TextBuffer, Caption);
	return TextBuffer;
}

// A new tab goes to the end, the help channel (7) gets its welcome text
// (marked as read); the tab becomes current unless the console is younger
// than 50 actions (the channels of the last session), when only a new tab
// is made inactive. The assertion on NewTab comes after its use.
void TConsoleWindow::AddChannel(TTalkBuffer *TextBuffer, const char *Caption)
{
	PATELLA_ASSERT(TextBuffer!=nullptr);
	TTab *NewTab = TabBar->FindTab(TextBuffer);
	if (NewTab == nullptr) {
		NewTab = new TTab(Caption, TextBuffer, TCallback(this, EVENT_SHOW_CHANNEL));
		TabBar->AppendChild(NewTab);
		if (TextBuffer->GetChannelID() == 7) {
			AddTextBufferLine(TextBuffer, 0, TextHelpChannelWelcome, TALK_ADMIN_MESSAGE, nullptr, 0);
			TextBuffer->Close();
		}
		PATELLA_ASSERT(NewTab!=nullptr);
	}
	const TTab *CurrentTab = TabBar->CurrentTab;
	TextBuffer->SetWriteEnabled(true);
	if (GetActionNumber() >= SwitchTime) {
		TabBar->SetCurrentTab(NewTab);
		HandleEvent(EVENT_SHOW_CHANNEL);
	} else if (CurrentTab != nullptr) {
		NewTab->SetActive(false);
	}
}

// Closes the current tab (never the default one): its buffer is locked, the
// next tab (else the previous, else the first) becomes current, the tab is
// deleted.
void TConsoleWindow::CloseChannel()
{
	TTab *CurrentTab = TabBar->CurrentTab;
	if (DefaultTab != CurrentTab) {
		PATELLA_ASSERT(DefaultTab!=nullptr);
		TComponent *NewTab = CurrentTab->GetNext();
		if (NewTab == nullptr)
			NewTab = CurrentTab->GetPrevious();
		CurrentTab->TextBuffer->SetLocked(true);
		TabBar->RemoveChild(CurrentTab);
		if (NewTab == nullptr) {
			NewTab = TabBar->GetFirstChild();
			PATELLA_ASSERT(NewTab!=nullptr);
		}
		TabBar->SetCurrentTab(static_cast<TTab *>(NewTab));
		DeleteComponent(CurrentTab);
		HandleEvent(EVENT_SHOW_CHANNEL);
	}
}

// Opens the player's request channel with the question (Control.cpp's
// message 0x33); with a request open it is shown (without event 201) and
// the player is told to wait.
void TConsoleWindow::ReportRuleViolation(const char *Question)
{
	TTalkBuffer *TextBuffer = Talk().GetTextBuffer(TALK_GAMEMASTER_ANSWER, nullptr, -1);
	if (TextBuffer != nullptr) {
		TTab *NewTab = TabBar->FindTab(TextBuffer);
		PATELLA_ASSERT(NewTab!=nullptr);
		TabBar->SetCurrentTab(NewTab);
		SetStatusMessage(TextRequestSubmitted, false);
		return;
	}
	PATELLA_ASSERT(Question!=nullptr);
	PATELLA_ASSERT(strlen(Question)>0);
	OpenChannel(TALK_GAMEMASTER_ANSWER, TALK_PLAYER_ANSWER, nullptr, -1, TextReportRuleViolation);
	TextBuffer = Talk().GetTextBuffer(TALK_GAMEMASTER_ANSWER, nullptr, -1);
	PATELLA_ASSERT(TextBuffer!=nullptr);
	PATELLA_ASSERT_VALUES(Creatures().GetCreature(PlayerState().GetPlayerID()), PlayerState().GetPlayerID());
	AddTextBufferLine(TextBuffer, 0, Question, TALK_PLAYER_ANSWER,
		Creatures().GetCreature(PlayerState().GetPlayerID())->Name, 0);
	AddTextBufferLine(TextBuffer, 0, TextWaitForGamemaster, TALK_ADMIN_MESSAGE, nullptr, 0);
	SendTalk(TALK_GAMEMASTER_REQUEST, Question);
	TextBuffer->SetWriteEnabled(false);
}

bool TConsoleWindow::GetServerMessages() const
{
	PATELLA_ASSERT(CurrentConsole->GetTextBuffer()!=nullptr);
	PATELLA_ASSERT(CurrentConsole->GetTextBuffer()->IsDefault()==false);
	return ServerMessagesButton->State;
}

void TConsoleWindow::SetServerMessages(bool Show)
{
	PATELLA_ASSERT(CurrentConsole->GetTextBuffer()!=nullptr);
	PATELLA_ASSERT(CurrentConsole->GetTextBuffer()->IsDefault()==false);
	ServerMessagesButton->State = Show;
	HandleEvent(EVENT_SERVER_MESSAGES);
}

TTalkBuffer *TConsoleWindow::GetTextBuffer() const
{
	PATELLA_ASSERT(CurrentConsole->GetTextBuffer()!=nullptr);
	return CurrentConsole->GetTextBuffer();
}

// Sends the edit line: in the default and the request channels with the
// volume (reset to "say" afterwards), elsewhere with the channel's mode (0,
// nothing, when the player may not write), then as ParseInput reads it. In
// the player's own private channel "#I name" and "#X name" invite and
// exclude players. Private messages are noted in the console (to oneself
// they are not sent); after one the edit line starts with "*name* " again,
// unless the channel is that player's.
void TConsoleWindow::SendInput()
{
	int ChannelID = -1;
	int TalkMode;
	char *Addressee;
	if (!CurrentConsole->GetTextBuffer()->AcceptMessages()) {
		TTalkBuffer *TextBuffer = CurrentConsole->GetTextBuffer();
		TalkMode = TextBuffer->GetSendTalkMode();
		if (!TextBuffer->CanWrite())
			TalkMode = TALK_NONE;
		ChannelID = TextBuffer->GetChannelID();
		Addressee = const_cast<char *>(TextBuffer->GetAddressee());
	} else {
		TalkMode = this->TalkMode;
		Addressee = nullptr;
		VolumeButton->Reset();
		this->TalkMode = TALK_SAY;
	}
	if (strlen(Input->Text.get()) != 0)
		AddToHistory();
	char *Text;
	char MessageSeparator;
	ParseInput(&Text, &Addressee, &TalkMode, &MessageSeparator);
	if (ChannelID == -1 && (TalkMode == TALK_GAMEMASTER_CHANNELCALL || TalkMode == TALK_ANONYMOUS_CHANNELCALL)) {
		SetStatusMessage("This is not a chat channel.", false);
		Text = nullptr;
	}
	if (Talk().HasOwnPrivateChannel() && Text != nullptr && strlen(Text) != 0) {
		if (MessageSeparator == 'I')
			SendInviteToPrivateChannel(Text);
		if (MessageSeparator == 'X')
			SendExcludeFromPrivateChannel(Text);
	}
	if (Text != nullptr) {
		switch (TalkMode) {
		case TALK_NONE:
			break;
		case TALK_SAY:
		case TALK_WHISPER:
		case TALK_YELL:
		case TALK_GAMEMASTER_BROADCAST:
		case TALK_ANONYMOUS_BROADCAST:
			SendTalk(TalkMode, Text);
			break;
		case TALK_PRIVATE_MESSAGE:
		case TALK_GAMEMASTER_MESSAGE:
		case TALK_ANONYMOUS_MESSAGE:
			PATELLA_ASSERT(Addressee!=nullptr);
			PATELLA_ASSERT_VALUES(Creatures().GetCreature(PlayerState().GetPlayerID()), PlayerState().GetPlayerID());
			Talk().AddChannelNotice(Creatures().GetCreature(PlayerState().GetPlayerID())->Name, Text, TalkMode,
				Addressee, ChannelID);
			if (CompareStrings(Addressee, Creatures().GetCreature(PlayerState().GetPlayerID())->Name, -1) != 0)
				SendTalk(TalkMode, Addressee, Text);
			break;
		case TALK_CHANNEL_CALL:
		case TALK_GAMEMASTER_CHANNELCALL:
		case TALK_ANONYMOUS_CHANNELCALL:
			PATELLA_ASSERT(ChannelID!=-1);
			SendTalk(TalkMode, ChannelID, Text);
			break;
		case TALK_GAMEMASTER_ANSWER:
			PATELLA_ASSERT(Addressee!=nullptr);
			PATELLA_ASSERT_VALUES(Creatures().GetCreature(PlayerState().GetPlayerID()), PlayerState().GetPlayerID());
			SendTalk(TALK_GAMEMASTER_ANSWER, Addressee, Text);
			Talk().AddChannelNotice(Creatures().GetCreature(PlayerState().GetPlayerID())->Name, Text,
				TALK_PLAYER_ANSWER, Addressee, -1);
			break;
		case TALK_PLAYER_ANSWER:
			PATELLA_ASSERT_VALUES(Creatures().GetCreature(PlayerState().GetPlayerID()), PlayerState().GetPlayerID());
			SendTalk(TALK_PLAYER_ANSWER, Text);
			Talk().AddChannelNotice(Creatures().GetCreature(PlayerState().GetPlayerID())->Name, Text,
				TALK_GAMEMASTER_ANSWER, "", -1);
			break;
		default:
			TERROR1("unknown talk mode", TalkMode);
		}
	}
	Input->Clear();
	if (TalkMode == TALK_PRIVATE_MESSAGE && Addressee != nullptr) {
		if (CurrentConsole->GetTextBuffer()->GetAddressee() != nullptr
			&& CompareStrings(CurrentConsole->GetTextBuffer()->GetAddressee(), Addressee, -1) == 0)
			return;
		if (strlen(Addressee) > 29)
			Addressee[29] = 0;
		TFixedString<100> Prefix;
		Prefix.Format("*{}* ", Addressee);
		Input->SetText(Prefix);
	}
}

// Keeps the edit line (unless it repeats the last one) and goes back
// to it.
void TConsoleWindow::AddToHistory()
{
	const char *Text = Input->Text.get();
	if (strlen(Text) != 0) {
		if (History->GetLast() == nullptr || strcmp(History->GetLast()->Text, Text) != 0)
			History->Append(new TTalkLine(0, TALK_SAY, Text, 0));
	}
	HistoryPosition = -1;
}

// Reads the edit line: "#X ..." is a command (MessageSeparator is the letter:
// A anonymous broadcast, B broadcast, C channel call, D anonymous channel
// call, S, W, Y say, whisper, yell, I and X invite and exclude; other letters
// keep the mode), "*name*", "@name@" and "$name$" address a player (private,
// gamemaster and anonymous message; the separator is cut out of the edit
// line). Text is the rest without leading blanks, NULL if there is none or a
// separator is not closed. A text with the account number or the password (if
// one is set) is not sent.
void TConsoleWindow::ParseInput(char **Text, char **Addressee, int *TalkMode, char *Separator) const
{
	int i = 0;
	*Text = nullptr;
	char *Line = Input->Text.get();
	*Separator = 0;
	while (IsSpace(Line[i]))
		i++;
	const char MessageSeparator = Line[i];
	if (MessageSeparator == 0)
		return;
	if (MessageSeparator == '#') {
		const char Command = (char)ToUpper(Line[i + 1]);
		do {
			i++;
			if (IsSpace(Line[i]))
				break;
		} while (Line[i] != 0);
		*Separator = Command;
		switch (Command) {
		case 'A': *TalkMode = TALK_ANONYMOUS_BROADCAST; break;
		case 'B': *TalkMode = TALK_GAMEMASTER_BROADCAST; break;
		case 'C': *TalkMode = TALK_GAMEMASTER_CHANNELCALL; break;
		case 'D': *TalkMode = TALK_ANONYMOUS_CHANNELCALL; break;
		case 'I': *TalkMode = TALK_NONE; break;
		case 'S': *TalkMode = TALK_SAY; break;
		case 'W': *TalkMode = TALK_WHISPER; break;
		case 'X': *TalkMode = TALK_NONE; break;
		case 'Y': *TalkMode = TALK_YELL; break;
		default:
			break;
		}
	} else if (MessageSeparator == '*' || MessageSeparator == '@' || MessageSeparator == '$') {
		i++;
		*Addressee = Line + i;
		while (Line[i] != 0 && Line[i] != MessageSeparator)
			i++;
		if (Line[i] == 0) {
			*Addressee = nullptr;
			return;
		}
		Line[i] = 0;
		if (strlen(*Addressee) == 0) {
			*Addressee = nullptr;
			return;
		}
		i++;
		*Separator = MessageSeparator;
		if (MessageSeparator == '$') {
			*TalkMode = TALK_ANONYMOUS_MESSAGE;
		} else if (MessageSeparator == '*') {
			*TalkMode = TALK_PRIVATE_MESSAGE;
		} else {
			if (MessageSeparator != '@')
				TERROR1("unknown message separator", (Word) MessageSeparator);
			*TalkMode = TALK_GAMEMASTER_MESSAGE;
		}
	}
	if (strstr(Line + i, Session().GetAccountNumberString()) != nullptr
		|| (strstr(Line + i, Session().GetAccountPassword()) != nullptr
			&& strlen(Session().GetAccountPassword()) != 0)) {
		*Text = nullptr;
		*Addressee = nullptr;
		*TalkMode = TALK_NONE;
		*Separator = 0;
		AddMessage(MESSAGE_ACCOUNT_DATA_WARNING, reinterpret_cast<std::intptr_t>("Account Data Warning"), 0);
		return;
	}
	while (IsSpace(Line[i]))
		i++;
	if (Line[i] != 0)
		*Text = Line + i;
}

// "Save Window" of the channel menu: appends the channel to "<caption>.txt"
// in the working directory.
bool TConsoleWindow::SaveChannel() const
{
	const TTab *CurrentTab = TabBar->CurrentTab;
	PATELLA_ASSERT(CurrentTab!=nullptr);
	PATELLA_ASSERT(CurrentTab->GetCaption()!=nullptr);
	TFixedString<1024> FileName;
	FileName.Format("{}.txt", CurrentTab->GetCaption());
	TFile File;
	if (!patella::platform::Platform().FileSystem.Open(File, FileName, TFile::TAccess::Append, TFile::TContent::Text))
		return false;
	const TTalkBuffer *TextBuffer = GetTextBuffer();
	PATELLA_ASSERT(TextBuffer!=nullptr);
	const std::int64_t Now = patella::platform::Platform().Clock.WallSeconds();
	// ctime's text, the day as MSVC 7.1 writes it, of the 64-bit time (after 2038 too, where the original's 32-bit time
	// had none); where there is none (before 1970), printf wrote "(null)"
	TCRT71TimeText Stamp;
	const char *Saved = FormatCRT71Time(Stamp, Now);
	File.Print("\nChannel saved at {}", Saved != nullptr ? Saved : "(null)");
	TextBuffer->Write(File);
	File.Close();
	TFixedString<100> Message;
	Message.Format("Channel appended to {}", FileName);
	SetStatusMessage(Message, false);
	return true;
}
// NOLINTEND(cppcoreguidelines-owning-memory)
