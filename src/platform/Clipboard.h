// Clipboard.h: IClipboard, the platform's clipboard. Not part of the original source. Portable: the standard library
// only.
//
// sysinc.cpp's IsClipboardEmpty, GetClipboardText and SetClipboardText (the paste, CopyToClipboard,
// CheckClipboardText, CopyTaintedClipboardText) ask the process's IClipboard (platform/Platform.h). The text is the
// client's (Windows-1252) on both sides; the implementation converts it to and from the platform's:
//
//	TSdlClipboard     SDL3's clipboard text, UTF-8 ('?' for a character Windows-1252 does not have; SdlPlatform.h):
//	                  the client's on every system
//
// sysinc.cpp keeps its buffer and the original's odd CR removal; only the clipboard's own calls are here.

#pragma once

#include <optional>
#include <string>

namespace patella::platform {

class IClipboard {
public:
	IClipboard() = default;
	IClipboard(const IClipboard &) = delete;
	IClipboard &operator=(const IClipboard &) = delete;
	IClipboard(IClipboard &&) = delete;
	IClipboard &operator=(IClipboard &&) = delete;

	// True when the clipboard has text (Win32: IsClipboardFormatAvailable(CF_UNICODETEXT)).
	[[nodiscard]] virtual bool HasText() = 0;
	// The clipboard's text up to its first NUL, as the client's text: nullopt where the clipboard cannot be opened,
	// empty where it has no text.
	[[nodiscard]] virtual std::optional<std::string> Text() = 0;
	// Puts Text (the client's) on the clipboard instead of what it had: false where that failed.
	virtual bool SetText(const char *Text) = 0;

protected:
	// Never destroyed through the interface (Clock.h's IClock).
	~IClipboard() = default;
};

} // namespace patella::platform
