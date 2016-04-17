/***********************************************\
*                    barber.c                   *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "main.h"
#include "customer.h"
#include "barber.h"
#include "FIFOqueue.h"

static BOOL BarberIsInitialized = FALSE;
static const int BarberWidth = 50, BarberHeight = 100;

/* Prototypes for functions with local scope */
static UINT CALLBACK BarberThread(void *args);



barber_data *InitBarber(HWND hwnd)
{
    if (!hwnd) return (barber_data*)NULL;

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

    HDC main_hdc = GetDC(hwnd);
    if (!main_hdc) {
        MessageBox(NULL, TEXT("Failed getting DC for the barber!"), 
                   TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        win_free(new);
        return (barber_data*)NULL;
    }

    new->hdc = CreateCompatibleDC(main_hdc);
    if (!(new->hdc)) {
        MessageBox(NULL, TEXT("Failed creating compatible DC for the barber!"), 
                   TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        win_free(new);
        ReleaseDC(hwnd, main_hdc);
        return (barber_data*)NULL;
    }

    new->width = BarberWidth;
    new->height = BarberHeight;
    new->hBitmap = CreateCompatibleBitmap(main_hdc, new->width, new->height);
    if (!(new->hBitmap)) {
        MessageBox(NULL, TEXT("Failed creating compatible bitmap for the barber!"), 
                   TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        DeleteDC(new->hdc);
        win_free(new);
        ReleaseDC(hwnd, main_hdc);
        return (barber_data*)NULL;
    }
    HGDIOBJ temp = SelectObject(new->hdc, new->hBitmap);
    DeleteObject(temp);
    ReleaseDC(hwnd, main_hdc);

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
        HGDIOBJ prev = SelectObject(to_delete->hdc, to_delete->hBitmap);
        DeleteObject(prev);
        retvalue = ((to_delete->hthrd != NULL) ? CloseHandle(to_delete->hthrd) : TRUE) ? TRUE : FALSE;
        retvalue = ((to_delete->hBitmap != NULL) ? DeleteObject((HGDIOBJ)to_delete->hBitmap) : TRUE) ? TRUE : FALSE;
        retvalue = ((to_delete->hdc != NULL) ? DeleteDC(to_delete->hdc) : TRUE) ? TRUE : FALSE;
        win_free(to_delete);
    }
    return retvalue;
}
