// SdlClipboard.cpp: TSdlClipboard, the platform's clipboard over SDL3 (platform/SdlPlatform.h).

#include "platform/SdlPlatform.h"

#include "types/Cp1252.h"

#include <SDL3/SDL_clipboard.h>
#include <SDL3/SDL_init.h>
#include <SDL3/SDL_stdinc.h>

#include <cstddef>
#include <iterator>
#include <memory>
#include <optional>
#include <string>

namespace patella::platform {

namespace {

bool VideoStarted()
{
	return SDL_WasInit(SDL_INIT_VIDEO) != 0;
}

// An empty text's data: one NUL byte. The size must be 1, not 0, for the text to exist: Windows' driver writes no
// CF_UNICODETEXT for size 0 and X11's says it has no text. The driver stores it as an empty text there: a lone
// terminator, as the original's CF_TEXT had.
const void *SDLCALL EmptyTextData(void * /*Userdata*/, const char * /*MimeType*/, std::size_t *Size)
{
	*Size = 1;
	return "";
}

// What SDL_SetClipboardText offers for a text on each driver (the drivers' GetTextMimeTypes, which SDL does not expose).
const char *const TextMimeTypes[] = {"text/plain;charset=utf-8", "text/plain", "UTF8_STRING", "TEXT", "STRING"};

} // namespace

// Win32's IsClipboardFormatAvailable(CF_UNICODETEXT): on Windows SDL asks the same.
bool TSdlClipboard::HasText()
{
	return VideoStarted() && SDL_HasClipboardText();
}

// The clipboard's text up to its first NUL (SDL's is a C string), converted to the client's. SDL gives an
// empty text where the clipboard cannot be opened (Win32's nullopt): the paste then has nothing, as with no text.
std::optional<std::string> TSdlClipboard::Text()
{
	if (!VideoStarted())
		return std::nullopt;
	const std::unique_ptr<char, void(SDLCALL *)(void *)> Utf8(SDL_GetClipboardText(), SDL_free);
	if (Utf8 == nullptr)
		return std::nullopt;
	return patella::text::Utf8ToCp1252(Utf8.get());
}

// Puts the text on the clipboard. One difference of SDL's, kept by the owner's choice: on Windows each line feed
// without a carriage return before it is written as CR LF (the console's selection of several lines). An empty text is
// put there as the original did (sysinc.cpp's SetClipboardText): SDL_SetClipboardText("") would empty the clipboard.
bool TSdlClipboard::SetText(const char *Text)
{
	if (!VideoStarted())
		return false;
	if (Text[0] == '\0')
		return SDL_SetClipboardData(EmptyTextData, nullptr, nullptr, TextMimeTypes, std::size(TextMimeTypes));
	const std::string Utf8 = patella::text::Cp1252ToUtf8(Text);
	return SDL_SetClipboardText(Utf8.c_str());
}

} // namespace patella::platform
