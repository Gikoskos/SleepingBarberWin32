/***********************************************\
*                     draw.h                    *
*           George Koskeridis (C) 2016          *
\***********************************************/

#pragma once

#ifndef __DRAW_H
#define __DRAW_H

#include "common.h"
#include "main.h"


void DrawBufferToWindow(HWND to, backbuffer_data *from);
void DrawToBuffer(backbuffer_data *buf);
void UpdateState(void);
void StretchGraphics(int scaling_idx);
#endif //__DRAW_H
