/***********************************************\
*                    barber.c                   *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "main.h"
#include "customer.h"
#include "barber.h"
#include "FIFOqueue.h"



static BOOL BarberIsInitialized = FALSE;

/* Prototypes for functions with local scope */
static UINT CALLBACK BarberThread(void *args);



barber_data *InitBarber(int InitialState)
{
    if (BarberIsInitialized) {
        fprintf(stderr, "Can't create more than one barber!\n");
        return (barber_data*)NULL;
    }

    barber_data *new = win_malloc(sizeof(barber_data));
    if (!new) {
        MessageBox(NULL, TEXT("Failed allocating memory for the barber!"), 
                   TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        return (barber_data*)NULL;
    }

    new->state = SLEEPING;
    new->hthrd = (HANDLE)_beginthreadex(NULL, 0, BarberThread, (LPVOID)new, InitialState, NULL);
    return new;
}

static UINT CALLBACK BarberThread(LPVOID args)
{
    barber_data *barber = (barber_data*)args;

    if (!ReadyCustomersSem || !BarberIsReadyMtx || !WRAccessToSeatsMtx || !KillAllThreadsEvt) {
        return 1;
    }

    while (GetBarberState(barber) != BARBER_DONE &&
           WaitForSingleObject(KillAllThreadsEvt, 0L) == WAIT_TIMEOUT) {

        SetBarberState(barber, SLEEPING);
        WaitForSingleObject(ReadyCustomersSem, INFINITE);
        WaitForSingleObject(WRAccessToSeatsMtx, INFINITE);
        if (numOfFreeSeats != CUSTOMER_CHAIRS) {
            SetBarberState(barber, CHECKING_WAITING_ROOM);
            WAIT_UNTIL_TIMEOUT_OR_DIE(1);
        } else {
            numOfFreeSeats++;
        }
        ReleaseMutex(BarberIsReadyMtx);
        ReleaseMutex(WRAccessToSeatsMtx);
        SetBarberState(barber, CUTTING_HAIR);
        WAIT_UNTIL_TIMEOUT_OR_DIE(1);
        //SetBarberState(barber, BARBER_DONE);
    }

    return 0;
}

LONG GetBarberState(barber_data *barber)
{
    if (!barber) return BARBER_DONE;

    LONG retvalue;

    InterlockedExchange(&retvalue, barber->state);
    return retvalue;
}

void SetBarberState(barber_data *barber, LONG new_state)
{
    if (barber) {
        InterlockedExchange(&barber->state, new_state);
    }
}

BOOL DeleteBarber(barber_data *to_delete)
{
    BOOL retvalue = FALSE;

    if (to_delete != NULL) {
        retvalue = ((to_delete->hthrd != NULL) ? CloseHandle(to_delete->hthrd) : TRUE) ? TRUE : FALSE;
        win_free(to_delete);
        to_delete = NULL;
    }
    return retvalue;
}
