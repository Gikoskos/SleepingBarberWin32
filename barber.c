/***********************************************\
*                    barber.c                   *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "main.h"
#include "customer.h"
#include "barber.h"
#include "FIFOqueue.h"


#define TIMEOUT 2000



static BOOL BarberIsInitialized = FALSE;
static LONG barbershop_door_open = FALSE;

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
BREAK_IF_TRUE(WaitForSingleObject(KillAllThreadsEvt, TIMEOUT) == WAIT_OBJECT_0);

static UINT CALLBACK BarberThread(LPVOID args)
{
    if (!ReadyCustomersSem || !BarberIsReadySem || !KillAllThreadsEvt || !BarberIsDoneSem) {
        return 1;
    }

    barber_data *barber = (barber_data*)args;
    //BOOL done = FALSE;
    HANDLE ReadyCustomersOrDieObj[2] = {KillAllThreadsEvt, ReadyCustomersSem};


    while (/*!done && */(WaitForSingleObject(KillAllThreadsEvt, 0L) == WAIT_TIMEOUT)) {
        SetBarberState(barber, SLEEPING);
        BREAK_IF_TRUE(WaitForMultipleObjects(2, ReadyCustomersOrDieObj, FALSE, INFINITE) == WAIT_OBJECT_0);
        IncFreeCustomerSeats();
        SetBarberState(barber, CHECKING_WAITING_ROOM);

        BLOCK_UNTIL_TIMEOUT_OR_BREAK();
        ReleaseSemaphore(BarberIsReadySem, 1, NULL);
        printf("Barber is ready!\n");
        SetBarberState(barber, CUTTING_HAIR);
        BLOCK_UNTIL_TIMEOUT_OR_BREAK();
        ReleaseSemaphore(BarberIsDoneSem, 1, NULL);
        printf("Barber is done!\n");
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

void SetBarbershopDoorState(BOOL new_state)
{
    InterlockedExchange(&barbershop_door_open, (LONG)new_state);
}

BOOL GetBarbershopDoorState(void)
{
    LONG retvalue, tmp = (LONG)barbershop_door_open;

    InterlockedExchange(&retvalue, tmp);
    return (BOOL)retvalue;
}
