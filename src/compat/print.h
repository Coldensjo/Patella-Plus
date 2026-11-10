// print.h: printf on stdout with std::format's language, for the client's one diagnostic line on stdout
// (ContentWindows.cpp's MouseHover). Not part of the original source.
//
// patella::compat::PrintLine(Format, Values...) writes the text formatted with std::format's language and a '\n'
// to stdout, with printf's semantics: the conversions the client used give the same characters (%s: {}, %d: {}),
// stdout's own mode does the rest (the Microsoft C runtime's text mode writes "\r\n"), and a failed write is
// ignored. Not std::println: that throws std::system_error where the write fails (C++23 [print.fun]), and a GUI
// program like the client has no valid stdout unless it is redirected, so every call would throw inside the
// caller and end the client; on Windows it would also write UTF-16 to a console where printf wrote
// the bytes. The text comes from std::format; FormatLine is the text alone.

#pragma once

#include <cstdio>
#include <format>
#include <string>
#include <utility>

namespace patella::compat {

// The text PrintLine writes, '\n' included.
template <typename... Arguments>
std::string FormatLine(std::format_string<Arguments...> Format, Arguments &&...Values)
{
	std::string Text = std::format(Format, std::forward<Arguments>(Values)...);
	Text.push_back('\n');
	return Text;
}

template <typename... Arguments>
void PrintLine(std::format_string<Arguments...> Format, Arguments &&...Values)
{
	const std::string Text = FormatLine(Format, std::forward<Arguments>(Values)...);
	std::fwrite(Text.data(), 1, Text.size(), stdout);
}

} // namespace patella::compat
