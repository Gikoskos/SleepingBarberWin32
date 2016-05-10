/***********************************************\
*                     draw.h                    *
*           George Koskeridis (C) 2016          *
\***********************************************/

#pragma once

#ifndef __DRAW_H
#define __DRAW_H

#include "common.h"
#include "main.h"



void ScaleGraphics(int scaling_exp);
void DrawBufferToWindow(HWND to, backbuffer_data *from);
void DrawToBuffer(backbuffer_data *buf);
void UpdateState(LONG num_of_customers, int *customer_states, int barber_state,
                 BOOL enable_animations, BOOL barbershop_door_is_open);
void CleanupGraphics(void);
#endif //__DRAW_H
