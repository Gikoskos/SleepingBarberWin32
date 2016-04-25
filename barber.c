/***********************************************\
*                    barber.c                   *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "main.h"
#include "customer.h"
#include "barber.h"
#include "FIFOqueue.h"


#define TIMEOUT 2000

#define EXIT_LOOP_IF_ASSERT(x) \
if (x) {\
    break;\
}



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

#define BLOCK_UNTIL_TIMEOUT_OR_BREAK() \
EXIT_LOOP_IF_ASSERT(WaitForSingleObject(KillAllThreadsEvt, TIMEOUT) == WAIT_OBJECT_0);

static UINT CALLBACK BarberThread(LPVOID args)
{
    if (!ReadyCustomersSem || !BarberIsReadyMtx || !KillAllThreadsEvt) {
        return 1;
    }

    barber_data *barber = (barber_data*)args;
    //BOOL done = FALSE;
    HANDLE ReadyCustomersOrDieObj[2] = {KillAllThreadsEvt, ReadyCustomersSem};


    while (/*!done && */(WaitForSingleObject(KillAllThreadsEvt, 0L) == WAIT_TIMEOUT)) {

        SetBarberState(barber, SLEEPING);
        EXIT_LOOP_IF_ASSERT(WaitForMultipleObjects(2, ReadyCustomersOrDieObj, FALSE, INFINITE) == WAIT_OBJECT_0);

        IncFreeCustomerSeats();
        SetBarberState(barber, CHECKING_WAITING_ROOM);

        BLOCK_UNTIL_TIMEOUT_OR_BREAK();

        ReleaseMutex(BarberIsReadyMtx);
        SetBarberState(barber, CUTTING_HAIR);
        //BLOCK_UNTIL_TIMEOUT_OR_BREAK();
    }

    return 0;
}

LONG GetBarberState(barber_data *barber)
{
    if (!barber) return SLEEPING;

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
