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
    HANDLE ReadyCustomersSem = OpenSemaphore(SEMAPHORE_ALL_ACCESS, FALSE, ReadyCustomersSemaphoreName);
    HANDLE BarberIsReadyMtx = OpenMutex(MUTEX_ALL_ACCESS, FALSE, BarberIsReadyMutexName);
    HANDLE WRAccessToSeatsMtx = OpenMutex(MUTEX_ALL_ACCESS, FALSE, WRAccessToSeatsMutexName);

    if (!ReadyCustomersSem || !BarberIsReadyMtx || !WRAccessToSeatsMtx) {
        PRINT_ERR_DEBUG();
       _endthreadex(1);
       return 1;
    }

#define TIMEOUT 2000
    while (GetBarberState(barber) != BARBER_DONE) {
        SetBarberState(barber, SLEEPING);
        Sleep(TIMEOUT);
        while (WaitForSingleObject(ReadyCustomersSem, 0L) == WAIT_OBJECT_0) {
            WaitForSingleObject(WRAccessToSeatsMtx, INFINITE);

            if (numOfFreeSeats != CUSTOMER_CHAIRS) {
                SetBarberState(barber, CHECKING_WAITING_ROOM);
            } else {
                SetBarberState(barber, CUTTING_HAIR);
                numOfFreeSeats++;
            }
            ReleaseMutex(BarberIsReadyMtx);
            ReleaseMutex(WRAccessToSeatsMtx);
            Sleep(TIMEOUT);
            SetBarberState(barber, CUTTING_HAIR);
        }
        //SetBarberState(barber, BARBER_DONE);
    }
    CloseHandle(barber->hthrd);
    barber->hthrd = NULL;
    _endthreadex(0);
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
