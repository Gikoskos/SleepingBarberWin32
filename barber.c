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



barber_data *InitBarber(void)
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
    new->hthrd = (HANDLE)_beginthreadex(BarberThread, 0, NULL, NULL, CREATE_SUSPENDED, NULL);
    return new;
}

static UINT CALLBACK BarberThread(void *args)
{
    UNREFERENCED_PARAMETER(args);
    //barber logic goes here
    _endthreadex(0);
    return 0;
}

BOOL DeleteBarber(barber_data *to_delete)
{
    BOOL retvalue = FALSE;

    if (to_delete != NULL) {
        retvalue = ((to_delete->hthrd != NULL) ? CloseHandle(to_delete->hthrd) : TRUE) ? TRUE : FALSE;
        win_free(to_delete);
    }
    return retvalue;
}
