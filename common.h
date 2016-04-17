/***********************************************\
*                    common.h                   *
*           George Koskeridis (C) 2016          *
\***********************************************/

#pragma once

#ifndef __COMMON_H
#define __COMMON_H

#ifndef WINVER
# define WINVER 0x0600
#endif
#ifndef _WIN32_WINNT
# define _WIN32_WINNT 0x0600
#endif
#include <SDKDDKVer.h> //API versioning

#ifndef UNICODE
# define UNICODE
#endif

#ifndef _UNICODE
# define _UNICODE
#endif

#define NOCRYPT
#include <windows.h>
#undef NOCRYPT

#include <process.h>
#include <windowsx.h>
#include <commctrl.h>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <tchar.h>

//wrappers for the wordy winapi alloc functions
#define win_free(x) HeapFree(GetProcessHeap(), 0, x)
//this one isn't actually a wrapper for stdlib malloc since malloc doesn't 0 out the allocated bytes
#define win_malloc(x) HeapAlloc(GetProcessHeap(), HEAP_GENERATE_EXCEPTIONS | HEAP_ZERO_MEMORY, x)
#define win_realloc(NULL, x) win_malloc(x)
#define win_calloc(x, y) win_malloc(x * y)

//RGB color macros
#define RGB_WHITE RGB(255, 255, 255)
#define RGB_BLACK RGB(0, 0, 0)
#define RGB_ORANGE RGB(255, 116, 53)
#define RGB_PURPLE RGB(64, 0, 255)
#define RGB_GREEN RGB(0, 204, 0)


#define IDI_SMALLBARBERWIN_ICON 210
#define IDI_LARGEBARBERWIN_ICON 211

#endif //__COMMON_H
