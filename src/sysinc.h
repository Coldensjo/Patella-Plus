// sysinc.h: system helpers (time, files, the clipboard, error texts).

#pragma once

#include <array>

int GetMilliseconds();
bool IsClipboardEmpty();
const char *GetErrorText(int Error);
const char *GetErrorHint(int Error);
void InitSysinc();
void ExitSysinc();
void ResizeClipboardBuffer(unsigned int Size);
char *GetClipboardText();
bool SetClipboardText(const char *Text);
