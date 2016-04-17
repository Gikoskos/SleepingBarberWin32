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
    void (*UpdateState)(void);
    void (*StretchGraphics)(int);
} dll_func_address;
#endif


int curr_resolution = SMALL_WND, prev_resolution = SMALL_WND;

//the resolutions here have to go from smaller to bigger for the resizing animation to work
POINT resolutions[TOTAL_RESOLUTIONS] = {
    {640, 480},
    {800, 600},
    {1000, 750}
};


//is TRUE when the game is running and false when it's not
static BOOL running = FALSE;
static HINSTANCE g_hInst;
BOOL barbershop_door_open = FALSE;


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
                    barbershop_door_open = !barbershop_door_open;
                    fprintf(stderr, (barbershop_door_open) ? "Door is open\n" : "Door is closed\n");
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

            AppendMenu(ResolutionMenu, MF_STRING | MF_GRAYED, 0, TEXT("Resolutions"));
            AppendMenu(ResolutionMenu, MF_SEPARATOR, 0, NULL);
            AppendMenu(ResolutionMenu,
                       MF_STRING | ((curr_resolution == SMALL_WND) ? MF_CHECKED : MF_UNCHECKED),
                       IDM_SMALL_RES, TEXT("Small (640x480)"));
            AppendMenu(ResolutionMenu,
                       MF_STRING | ((curr_resolution == MEDIUM_WND) ? MF_CHECKED : MF_UNCHECKED),
                       IDM_MEDIUM_RES, TEXT("Medium (800x600)"));
            AppendMenu(ResolutionMenu,
                       MF_STRING | ((curr_resolution == LARGE_WND) ? MF_CHECKED : MF_UNCHECKED),
                       IDM_LARGE_RES, TEXT("Large (1000x750)"));
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
static BOOL LoadDrawDLL(dll_func_address *drawdll_func)
{
    LPCTSTR drawdll_loaded_fname = TEXT("draw-loaded.dll");

    if (drawdll_func->to_load) FreeLibrary(drawdll_func->to_load);

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

    if ((drawdll_func->to_load = LoadLibrary(drawdll_loaded_fname)) != NULL) {
        drawdll_func->DrawBufferToWindow = (void(*)(HWND, backbuffer_data*))GetProcAddress(drawdll_func->to_load, "DrawBufferToWindow");
        drawdll_func->DrawToBuffer = (void(*)(backbuffer_data*))GetProcAddress(drawdll_func->to_load, "DrawToBuffer");
        drawdll_func->UpdateState = (void(*)(void))GetProcAddress(drawdll_func->to_load, "UpdateState");
        drawdll_func->StretchGraphics = (void(*)(int))GetProcAddress(drawdll_func->to_load, "StretchGraphics");
        return TRUE;
    } else {
        fprintf(stderr, "Failed loading library! Errcode %ld\n", GetLastError());
    }

    return FALSE;
}
#endif

int APIENTRY WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
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
    int dll_load_counter = 0;
    dll_func_address drawdll_func = {NULL, NULL, NULL, NULL, NULL};

    if (!LoadDrawDLL(&drawdll_func))
        return 1;
#endif
    /*FIFOqueue *customer_queue = newFIFOqueue();

    for (int i = 0; i < 10; i++) {
        customer *new = win_malloc(sizeof(customer));
        FIFOenqueue(customer_queue, (void*)new);
    }
    FIFOenqueue(customer_queue, NULL);*/

    if (!ConvertWinToClientResolutions()) {
        MessageBox(NULL, TEXT("Failed converting resolutions!"), TEXT("Something happened!"), MB_ICONERROR | MB_OK);
        return 1;
    }
    SBarberMainWindow.width = resolutions[curr_resolution].x;
    SBarberMainWindow.height = resolutions[curr_resolution].y;
#ifdef _DEBUG
    drawdll_func.StretchGraphics(curr_resolution);
#else
    StretchGraphics(curr_resolution);
#endif

#if defined(_DEBUG) && defined(_MSC_VER)
    //Create the console for debugging
    AllocConsole();
    AttachConsole(GetCurrentProcessId());
    //https://en.wikipedia.org/wiki/Device_file#DOS.2C_OS.2F2.2C_and_Windows
    freopen("CON", "w", stdout);
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
            if (drawdll_func.StretchGraphics)
                drawdll_func.StretchGraphics(curr_resolution);
#else
            StretchGraphics(curr_resolution);
#endif
            ResizeWindow(SBarberMainWindow.hwnd, curr_resolution, prev_resolution);
            backbuff = NewBackbuffer(SBarberMainWindow.hwnd);
            prev_resolution = curr_resolution;
        }
#ifdef _DEBUG
        dll_load_counter++;
        if (dll_load_counter > 4000) {
            LoadDrawDLL(&drawdll_func);
            dll_load_counter = 0;
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
#ifdef _DEBUG
            if (drawdll_func.UpdateState) drawdll_func.UpdateState();
#else
            UpdateState();
#endif
            next_game_tick += SKIP_TICKS;
            loops++;
        }
    }

    if (!DeleteBackbuffer(backbuff)) {
        fprintf(stderr, "Error deleting backbuff!\n");
    }
    //deleteFIFOqueue(customer_queue, WIN_MALLOC);

#if defined(_DEBUG) && defined(_MSC_VER)
    FreeConsole();
#endif
    return 0;
}
