/***********************************************\
*                    barber.h                   *
*           George Koskeridis (C) 2016          *
\***********************************************/

#pragma once

#ifndef __BARBER_H
#define __BARBER_H

#include "common.h"

enum barber_state {
    SLEEPING = 0x0000cccc,
    CUTTING_HAIR,
    CHECKING_WAITING_ROOM,
    BARBER_DONE
};

typedef struct _barber_data {
    HANDLE hthrd; //the handle of the barber's thread
    LONG state;
} barber_data;


barber_data *InitBarber(int InitialState);
LONG GetBarberState(barber_data *barber);
void SetBarberState(barber_data *barber, LONG new_state);
BOOL DeleteBarber(barber_data *to_delete);
#endif //__BARBER_H
