/***********************************************\
*                     main.c                    *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "main.h"
#include "customer.h"
#include "barber.h"
#include "FIFOqueue.h"
#include "draw.h"


#ifdef _DEBUG
typedef struct _dll_func_address {
    HMODULE to_load;
    void (*DrawBufferToWindow)(HWND, backbuffer_data*);
    void (*DrawToBuffer)(backbuffer_data*);
    void (*UpdateState)(LONG, int*, int);
    void (*CleanupGraphics)(void);
    void (*ScaleGraphics)(int);
    void (*SetBarbershopDoorState)(BOOL);
} dll_func_address;

static dll_func_address drawdll_func = {NULL, NULL, NULL, NULL, NULL};
static BOOL door_open = FALSE;
#endif


TCHAR *ReadyCustomersSemaphoreName = _T("ReadyCustomersSem"),
      *BarberIsReadyMutexName = _T("BarberIsReadyMtx"),
      *WRAccessToSeatsMutexName = _T("WRAccessToSeatsMtx");;

int curr_resolution = SMALL_WND, prev_resolution = SMALL_WND;

LONG numOfFreeSeats = CUSTOMER_CHAIRS;

//the resolutions here have to go from smaller to bigger for the resizing animation to work
POINT resolutions[TOTAL_RESOLUTIONS] = {
    {640, 480},
    {800, 600},
    {1000, 750}
};
LONG total_customers = 0;


//is TRUE when the game is running and false when it's not
static BOOL running = FALSE;
static HINSTANCE g_hInst;


/* Prototypes for functions with local scope */
static backbuffer_data *NewBackbuffer(HWND hwnd);
static BOOL DeleteBackbuffer(backbuffer_data *to_delete);
static HWND CreateSleepingBarberWindow(WNDCLASSEX *wc, LPCTSTR window_title,
                                       LPCTSTR class_name, int window_width,
                                       int window_height, LPVOID WinProcParam);
static BOOL ResizeWindow(HWND hwnd, int new_size, int old_size);
static inline void HandleMessages(HWND hwnd);
static inline BOOL ConvertWinToClientResolutions(void);
static LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam);


static backbuffer_data *NewBackbuffer(HWND hwnd)
{
    if (!hwnd)
        return (backbuffer_data*)NULL;

    HDC hdc = GetDC(hwnd);
    if (!hdc)
        return (backbuffer_data*)NULL;

    backbuffer_data *new = win_malloc(sizeof(backbuffer_data));
    if (new == NULL) {
        ReleaseDC(hwnd, hdc);
        return (backbuffer_data*)NULL;
    }

    GetClientRect(hwnd, &new->coords);

    new->hdc = CreateCompatibleDC(hdc);
    if (!(new->hdc)) {
        win_free(new);
        ReleaseDC(hwnd, hdc);
        return (backbuffer_data*)NULL;
    }

#ifdef _DEBUG
    printf("New client area: %ldx%ld\n", new->coords.right, new->coords.bottom);
#endif
    new->hBitmap = CreateCompatibleBitmap(hdc, new->coords.right, new->coords.bottom);
    if (!(new->hBitmap)) {
        DeleteDC(new->hdc);
        win_free(new);
        ReleaseDC(hwnd, hdc);
        return (backbuffer_data*)NULL;
    }

    HGDIOBJ temp = SelectObject(new->hdc, new->hBitmap);
    DeleteObject(temp);
    ReleaseDC(hwnd, hdc);
    return new;
}

static BOOL DeleteBackbuffer(backbuffer_data *to_delete)
{
    BOOL retvalue = FALSE;

    if (to_delete != NULL) {
        HGDIOBJ prev = SelectObject(to_delete->hdc, to_delete->hBitmap);
        DeleteObject(prev);
        retvalue = ((to_delete->hBitmap != NULL) ? DeleteObject((HGDIOBJ)to_delete->hBitmap) : TRUE) ? TRUE : FALSE;
        retvalue = ((to_delete->hdc != NULL) ? DeleteDC(to_delete->hdc) : TRUE) ? TRUE : FALSE;
        win_free(to_delete);
    }
    return retvalue;
}

static HWND CreateSleepingBarberWindow(WNDCLASSEX *wc, LPCTSTR window_title,
                                       LPCTSTR class_name, int window_width, 
                                       int window_height, LPVOID WinProcParam)
{
    HWND retvalue = NULL;

    wc->cbSize = sizeof(WNDCLASSEX);
    wc->style = wc->cbClsExtra = wc->cbWndExtra = 0;
    wc->lpfnWndProc = MainWindowProcedure;
    wc->hInstance = g_hInst;
    wc->hIconSm = (HICON)LoadImage(g_hInst, MAKEINTRESOURCEW(IDI_SMALLBARBERWIN_ICON),
                                   IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                   GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    wc->hIcon = (HICON)LoadImage(g_hInst, MAKEINTRESOURCEW(IDI_LARGEBARBERWIN_ICON),
                                 IMAGE_ICON, GetSystemMetrics(SM_CXSMICON),
                                 GetSystemMetrics(SM_CYSMICON), LR_DEFAULTCOLOR);
    wc->hCursor = LoadCursor(NULL, IDC_ARROW);
    wc->hbrBackground = NULL;
    wc->lpszMenuName = NULL;
    wc->lpszClassName = class_name;

    if (!RegisterClassEx(wc)) {
        MessageBox(NULL, TEXT("Main Window class registration Failed!"), TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        return retvalue;
    }

    retvalue = CreateWindowEx(WS_EX_OVERLAPPEDWINDOW, class_name, window_title,
                              WS_BORDER | WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                              CW_USEDEFAULT, CW_USEDEFAULT, window_width, window_height,
                              NULL, NULL, g_hInst, WinProcParam);

    return retvalue;
}

static BOOL ResizeWindow(HWND hwnd, int new_size, int old_size)
{
    if ((new_size == old_size) || !hwnd) return FALSE;

    int i, j, width, height;
    POINT up_to;

    width = resolutions[old_size].x;
    height = resolutions[old_size].y;
    up_to.x = labs(resolutions[new_size].x - resolutions[old_size].x);
    up_to.y = labs(resolutions[new_size].y - resolutions[old_size].y);

    //no idea why 'i' needs to be less or equal than 'up_to.x' and 
    //'j' just less than 'up_to.y', but the resolutions work correctly like this
    for (i = 1, j = 1; (i <= up_to.x) || (j < up_to.y);) {
        HDWP winnum = BeginDeferWindowPos(1);
        if (!winnum) continue;

        if (old_size < new_size) {
            DeferWindowPos(winnum, hwnd, HWND_TOP, 0, 0, width + i, height + j,
                           SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
        } else {
            DeferWindowPos(winnum, hwnd, HWND_TOP, 0, 0, width - i, height - j,
                           SWP_NOMOVE | SWP_NOOWNERZORDER | SWP_NOZORDER);
        }
        EndDeferWindowPos(winnum);

        i += (i <= up_to.x) ? 1 : 0;
        j += (j < up_to.y) ? 1 : 0;
    }

    return TRUE;
}

static inline BOOL ConvertWinToClientResolutions(void)
{
    for (int i = 0; i < TOTAL_RESOLUTIONS; i++) {
        RECT tmp = {.left = 0, .top = 0,
                    .right = resolutions[i].x,
                    .bottom = resolutions[i].y};
#ifdef _DEBUG
        printf("%ldx%ld converted to ", tmp.right, tmp.bottom);
#endif
        if (!AdjustWindowRectEx(&tmp, WS_BORDER | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                                FALSE, WS_EX_OVERLAPPEDWINDOW)) {
            return FALSE;
        }
        resolutions[i].x = tmp.right - tmp.left;
        resolutions[i].y = tmp.bottom - tmp.top;
#ifdef _DEBUG
        printf("%ldx%ld\n", resolutions[i].x, resolutions[i].y);
#endif
    }
    return TRUE;
}

static inline void HandleMessages(HWND hwnd)
{
    MSG msg;

    if (PeekMessage(&msg, hwnd, 0, 0, PM_REMOVE)) {
        TranslateMessage(&msg);
        DispatchMessage(&msg);
    }
}

static LRESULT CALLBACK MainWindowProcedure(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
#define IDM_EXIT       2000
#define IDM_ABOUT      2001
#define IDM_SMALL_RES  3000
#define IDM_MEDIUM_RES 3001
#define IDM_LARGE_RES  3002
    HMENU ResolutionMenu;
    POINT cursor_pos;

    switch (msg) {
        case WM_CREATE:
        {
            break;
        }
        case WM_KEYUP:
        {
            switch (wParam) {
                case VK_ESCAPE:
                    SendMessage(hwnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
                    break;
                case VK_SPACE:
                case 0x4F: //'O' key
#ifdef _DEBUG
                    door_open = !door_open;
                    drawdll_func.SetBarbershopDoorState(door_open);
                    drawdll_func.ScaleGraphics(curr_resolution);
#else
                    SetBarbershopDoorState(!GetBarbershopDoorState());
                    ScaleGraphics(curr_resolution);
#endif
                    break;
                default:
                    break;
            }
            break;
        }
#ifdef _DEBUG
        case WM_MOUSEMOVE:
        {
            cursor_pos.x = GET_X_LPARAM(lParam);
            cursor_pos.y = GET_Y_LPARAM(lParam);

            fprintf(stderr, "x: %ld, y: %ld\n", cursor_pos.x, cursor_pos.y);
        }
#endif
        case WM_PAINT:
        {
            HDC hdc;
            PAINTSTRUCT ps;

            if ((hdc = BeginPaint(hwnd, &ps))) {
                EndPaint(hwnd, &ps);
            }
            break;
        }
        case WM_COMMAND:
        {
            switch (LOWORD(wParam)) {
                case IDM_ABOUT:
                    MessageBox(hwnd, TEXT("            ")
                               TEXT("SleepingBarber Win32\n")
                               TEXT("        ")
                               TEXT("George Koskeridis (c) 2016 "),
                               TEXT("About"), MB_OK);
                    break;
                case IDM_EXIT:
                    SendMessage(hwnd, WM_CLOSE, (WPARAM)0, (LPARAM)0);
                    break;
                case IDM_SMALL_RES:
                case IDM_MEDIUM_RES:
                case IDM_LARGE_RES:
                    prev_resolution = curr_resolution;
                    curr_resolution = LOWORD(wParam) - 3000;
                    break;
                default:
                    break;
            }
            switch (HIWORD(wParam)) {
            }
            break;
        }
        case WM_RBUTTONUP:
        {
            ResolutionMenu = CreatePopupMenu();

            AppendMenu(ResolutionMenu, MF_STRING | MF_GRAYED, 0, TEXT("Resolution"));
            AppendMenu(ResolutionMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(ResolutionMenu,
                       MF_STRING | ((curr_resolution == SMALL_WND) ? MF_CHECKED : MF_UNCHECKED),
                       IDM_SMALL_RES, TEXT("Low (640x480)"));
            AppendMenu(ResolutionMenu,
                       MF_STRING | ((curr_resolution == MEDIUM_WND) ? MF_CHECKED : MF_UNCHECKED),
                       IDM_MEDIUM_RES, TEXT("Medium (800x600)"));
            AppendMenu(ResolutionMenu,
                       MF_STRING | ((curr_resolution == LARGE_WND) ? MF_CHECKED : MF_UNCHECKED),
                       IDM_LARGE_RES, TEXT("High (1000x750)"));

            AppendMenu(ResolutionMenu, MF_STRING | MF_GRAYED | MF_MENUBREAK, 0, TEXT("Help"));
            AppendMenu(ResolutionMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(ResolutionMenu, MF_STRING | MF_UNCHECKED, IDM_ABOUT, TEXT("About"));
            AppendMenu(ResolutionMenu, MF_STRING | MF_UNCHECKED, IDM_EXIT, TEXT("Exit"));
            GetCursorPos(&cursor_pos);
            TrackPopupMenuEx(ResolutionMenu,
                             TPM_CENTERALIGN | TPM_TOPALIGN | TPM_LEFTBUTTON | TPM_NOANIMATION,
                             cursor_pos.x, cursor_pos.y, hwnd, NULL);
            break;
        }
        case WM_CLOSE:
        case WM_DESTROY:
        {
            running = FALSE;
            DestroyWindow(hwnd);
            PostQuitMessage(0);
            break;
        }
        default:
            return DefWindowProc(hwnd, msg, wParam, lParam);
    }
    return TRUE;
}

#ifdef _DEBUG
static BOOL LoadDrawDLL(void)
{
    LPCTSTR drawdll_loaded_fname = TEXT("draw-loaded-0.dll");

    //don't leak memory while reloading the DLL
    if (drawdll_func.CleanupGraphics) drawdll_func.CleanupGraphics();

    if (!CopyFile(TEXT("draw.dll"), drawdll_loaded_fname, FALSE)) {
        if (GetLastError() == ERROR_FILE_NOT_FOUND) {
            fprintf(stderr, "Copying of DLL failed! File not found.\n");
        } else if (GetLastError() == ERROR_ACCESS_DENIED) {
            fprintf(stderr, "Copying of DLL failed! Access denied.\n");
        } else if (GetLastError() == ERROR_SHARING_VIOLATION) {
            fprintf(stderr, "Copying of DLL failed! File is used by another process.\n");
        } else {
            fprintf(stderr, "Copying of DLL failed! Error code %lu\n", GetLastError());
        }
        return FALSE;
    }

    if ((drawdll_func.to_load = LoadLibrary(drawdll_loaded_fname)) != NULL) {
        drawdll_func.DrawBufferToWindow = (void(*)(HWND, backbuffer_data*))GetProcAddress(drawdll_func.to_load, "DrawBufferToWindow");
        drawdll_func.DrawToBuffer = (void(*)(backbuffer_data*))GetProcAddress(drawdll_func.to_load, "DrawToBuffer");
        drawdll_func.UpdateState = (void(*)(LONG, int*, int))GetProcAddress(drawdll_func.to_load, "UpdateState");
        drawdll_func.ScaleGraphics = (void(*)(int))GetProcAddress(drawdll_func.to_load, "ScaleGraphics");
        drawdll_func.SetBarbershopDoorState = (void(*)(BOOL))GetProcAddress(drawdll_func.to_load, "SetBarbershopDoorState");
        drawdll_func.CleanupGraphics = (void(*)(void))GetProcAddress(drawdll_func.to_load, "CleanupGraphics");

        drawdll_func.SetBarbershopDoorState(door_open);
        drawdll_func.ScaleGraphics(curr_resolution);
        return TRUE;
    } else {
        fprintf(stderr, "Failed loading library! Errcode %ld\n", GetLastError());
    }

    return FALSE;
}

static FILETIME GetDrawDLLLastWriteTime()
{
    FILETIME retvalue = {0, 0};
    WIN32_FIND_DATA drawdll_data;
    HANDLE hdrawdll = FindFirstFile(TEXT("draw.dll"), &drawdll_data);
    if (hdrawdll != INVALID_HANDLE_VALUE) {
        FindClose(hdrawdll);
        retvalue = drawdll_data.ftLastWriteTime;
    } else {
        fprintf(stderr, "Failed loading draw.dll! Errcode %ld\n", GetLastError());
    }
    return retvalue;
}
#endif

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    const int max_customers = 6;

    window_data SBarberMainWindow = {
        .title = TEXT("SleepingBarber Win32")
    };
    backbuffer_data *backbuff = NULL;

    const int TICKS_PER_SECOND = 20;
    const int SKIP_TICKS = 1000 / TICKS_PER_SECOND;
    const int MAX_FRAMESKIP = 10;
    DWORD next_game_tick = GetTickCount();
    int loops;

#ifdef _DEBUG
    FILETIME currLastWriteTime, prevLastWriteTime;
    currLastWriteTime = prevLastWriteTime = GetDrawDLLLastWriteTime();

    if (!LoadDrawDLL())
        return 1;
#endif

    HANDLE ReadyCustomersSem = CreateSemaphore(NULL, 0, 1, ReadyCustomersSemaphoreName);
    HANDLE BarberIsReadyMtx = CreateMutex(NULL, FALSE, BarberIsReadyMutexName);
    HANDLE WRAccessToSeatsMtx = CreateMutex(NULL, FALSE, WRAccessToSeatsMutexName);

    customer_data **customer_array = win_malloc(sizeof(customer_data) * max_customers);
    int customer_states[max_customers];
    FIFOqueue *customer_queue = newFIFOqueue();

    barber_data *barber = InitBarber(0);
    if (!barber || !ReadyCustomersSem || !BarberIsReadyMtx || !WRAccessToSeatsMtx) {
        PRINT_ERR_DEBUG();
        return 1;
    }

    for (int i = 0; i < max_customers; i++) {
        customer_array[i] = NewCustomer(0);
        FIFOenqueue(customer_queue, (void*)customer_array[i]);
    }


    if (!ConvertWinToClientResolutions()) {
        MessageBox(NULL, TEXT("Failed converting resolutions!"), TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        return 1;
    }
    SBarberMainWindow.width = resolutions[curr_resolution].x;
    SBarberMainWindow.height = resolutions[curr_resolution].y;
#ifdef _DEBUG
    drawdll_func.ScaleGraphics(curr_resolution);
#else
    ScaleGraphics(curr_resolution);
#endif

    g_hInst = hInstance;

    SBarberMainWindow.hwnd = CreateSleepingBarberWindow(&SBarberMainWindow.wc,
                                    SBarberMainWindow.title, TEXT("SleepingBarberWin32Class"),
                                    SBarberMainWindow.width, SBarberMainWindow.height, NULL);
    if (!SBarberMainWindow.hwnd) {
        MessageBox(NULL, TEXT("Window creation failed!"), TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        return 1;
    }

    ShowWindow(SBarberMainWindow.hwnd, nCmdShow);
    UpdateWindow(SBarberMainWindow.hwnd);
    running = TRUE;
    backbuff = NewBackbuffer(SBarberMainWindow.hwnd);

    while (running && backbuff) {
        if (curr_resolution != prev_resolution) {
            if (!DeleteBackbuffer(backbuff)) {
                fprintf(stderr, "Error deleting backbuff!\n");
                return 1;
            }
#ifdef _DEBUG
            if (drawdll_func.ScaleGraphics)
                drawdll_func.ScaleGraphics(curr_resolution);
#else
            ScaleGraphics(curr_resolution);
#endif
            ResizeWindow(SBarberMainWindow.hwnd, curr_resolution, prev_resolution);
            backbuff = NewBackbuffer(SBarberMainWindow.hwnd);
            prev_resolution = curr_resolution;
        }
#ifdef _DEBUG
        currLastWriteTime = GetDrawDLLLastWriteTime();
        if (CompareFileTime(&currLastWriteTime, &prevLastWriteTime) && currLastWriteTime.dwLowDateTime) {
            Sleep(200);
            FreeLibrary(drawdll_func.to_load);
            printf("RELOADING\n");
            LoadDrawDLL();
            prevLastWriteTime.dwLowDateTime = currLastWriteTime.dwLowDateTime;
            prevLastWriteTime.dwHighDateTime = currLastWriteTime.dwHighDateTime;
        }

        drawdll_func.DrawToBuffer(backbuff);
        drawdll_func.DrawBufferToWindow(SBarberMainWindow.hwnd, backbuff);
#else
        DrawToBuffer(backbuff);
        DrawBufferToWindow(SBarberMainWindow.hwnd, backbuff);
#endif
        loops = 0;

        while (GetTickCount() > next_game_tick && loops < MAX_FRAMESKIP) {
            HandleMessages(SBarberMainWindow.hwnd);
            for (int i = 0; i < max_customers; i++) {
                if (*(customer_array + i) != NULL) {
                    customer_states[i] = (int)(*(customer_array + i))->state;
                } else {
                    customer_states[i] = CUSTOMER_DONE;
                }
            }
#ifdef _DEBUG
            if (drawdll_func.UpdateState) drawdll_func.UpdateState(total_customers, customer_states, GetBarberState(barber));
#else
            UpdateState(total_customers, customer_states, GetBarberState(barber));
#endif
            next_game_tick += SKIP_TICKS;
            loops++;
        }
    }

                /* Cleanup goes here */
    for (int i = 0; i < max_customers; i++) {
        DeleteCustomer(*(customer_array + i));
    }
    win_free(customer_array);
    deleteFIFOqueue(customer_queue, DONT_DELETE_DATA);
#ifdef _DEBUG
    drawdll_func.CleanupGraphics();
    FreeLibrary(drawdll_func.to_load);
    DeleteFile(TEXT("draw-loaded-0.dll"));
#else
    CleanupGraphics();
#endif
    if (!DeleteBarber(barber)) fprintf(stderr, "Error freeing up barber resources!\n");

    if (!DeleteBackbuffer(backbuff)) {
        fprintf(stderr, "Error deleting backbuff!\n");
    }
    return 0;
}
