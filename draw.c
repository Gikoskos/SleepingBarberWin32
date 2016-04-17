/***********************************************\
*                     draw.c                    *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "draw.h"

#define WALL_VERTICES 12
#define DOOR_VERTICES  4

static const POINT lowres_wall_vertices[WALL_VERTICES] = {
    {600, 450}, {600, 30}, {280, 30}, 
    {280, 60}, {290, 60}, {290, 40},
    {590, 40}, {590, 440}, {290, 440}, 
    {290, 130}, {280, 130}, {280, 450}
}, lowres_closed_door_vertices[DOOR_VERTICES] = {
    {300, 130}, {290, 130}, {290, 60}, {300, 60}
}, lowres_opened_door_vertices[DOOR_VERTICES] = {
    {358, 41}, {359, 51}, {290, 60}, {288, 50}
};


static POINT modular_wall[WALL_VERTICES], modular_door[DOOR_VERTICES];
static BOOL barbershop_door_open = FALSE;


static POINT RotatePoint(POINT axis, POINT vertex, double angle);

static POINT RotatePoint(POINT axis, POINT vertex, double angle)
{
    double sine = sin(angle), cosine = cos(angle);

    vertex.x -= axis.x;
    vertex.y -= axis.y;

    double xnew = vertex.x * cosine - vertex.y * sine;
    double ynew = vertex.x * sine + vertex.y * cosine;

    vertex.x = (LONG)(xnew + axis.x);
    vertex.y = (LONG)(ynew + axis.y);

    return vertex;
}

#if 0
static void RotateDoor(double angle)
{
    lowres_opened_door_vertices[0] = RotatePoint(lowres_closed_door_vertices[2], lowres_closed_door_vertices[0], angle);
    lowres_opened_door_vertices[1] = RotatePoint(lowres_closed_door_vertices[2], lowres_closed_door_vertices[1], angle);
    lowres_opened_door_vertices[3] = RotatePoint(lowres_closed_door_vertices[2], lowres_closed_door_vertices[3], angle);
    printf("0 %ld:%ld\n", lowres_opened_door_vertices[0].x, lowres_opened_door_vertices[0].y);
    printf("1 %ld:%ld\n", lowres_opened_door_vertices[1].x, lowres_opened_door_vertices[1].y);
    printf("3 %ld:%ld\n", lowres_opened_door_vertices[3].x, lowres_opened_door_vertices[3].y);
}
#endif

void SetBarbershopDoorState(BOOL new_state)
{
    barbershop_door_open = new_state;
}

BOOL GetBarbershopDoorState(void)
{
    return barbershop_door_open;
}

void StretchGraphics(int scaling_idx)
{
    for (int i = 0; i < WALL_VERTICES; i++) {
        if (!scaling_idx) {
            modular_wall[i] = lowres_wall_vertices[i];

            if (i < DOOR_VERTICES) {
                modular_door[i] = (barbershop_door_open) ? lowres_opened_door_vertices[i] : lowres_closed_door_vertices[i];
            }
        } else {
            double tmp = (double)(lowres_wall_vertices[i].x * (pow(1.25, (double)scaling_idx)));

            modular_wall[i].x = (LONG)tmp;
            tmp = (double)(lowres_wall_vertices[i].y * (pow(1.25, (double)scaling_idx)));
            modular_wall[i].y = (LONG)tmp;

            if (i < DOOR_VERTICES) {
                tmp = (double)(((barbershop_door_open) ? lowres_opened_door_vertices[i].x : lowres_closed_door_vertices[i].x)
                               * (pow(1.25, (double)scaling_idx)));

                modular_door[i].x = (LONG)tmp;

                tmp = (double)(((barbershop_door_open) ? lowres_opened_door_vertices[i].y : lowres_closed_door_vertices[i].y)
                               * (pow(1.25, (double)scaling_idx)));

                modular_door[i].y = (LONG)tmp;
            }
        }
    }
}

void DrawBufferToWindow(HWND to, backbuffer_data *from)
{
    HDC hdc = GetDC(to);

    if (!hdc) return;

    BitBlt(hdc, 0, 0, from->coords.right, from->coords.bottom, from->hdc, 0, 0, SRCCOPY);
    ReleaseDC(to, hdc);
}

void DrawToBuffer(backbuffer_data *buf)
{
    if (buf == NULL) return;

    HGDIOBJ prev;

    //background color
    PatBlt(buf->hdc, 0, 0, buf->coords.right, buf->coords.bottom, BLACKNESS);
    FillRect(buf->hdc, &buf->coords, (HBRUSH)GetStockObject(DKGRAY_BRUSH));

    //barbershop
    LOGBRUSH wall_line_attr = {BS_SOLID, RGB_BLACK, NULL};
    HPEN boundary_pen = ExtCreatePen(PS_GEOMETRIC | PS_INSIDEFRAME | PS_ENDCAP_ROUND | PS_JOIN_BEVEL,
                                     5, &wall_line_attr, 0, NULL);
    HBRUSH wallfilling_brush = CreateHatchBrush(HS_DIAGCROSS, RGB_BLACK);
    //draw the walls
    prev = SelectObject(buf->hdc, boundary_pen);
    SelectObject(buf->hdc, prev);
    prev = SelectObject(buf->hdc, wallfilling_brush);
    SetBkColor(buf->hdc, RGB_PURPLE);
    Polygon(buf->hdc, modular_wall, WALL_VERTICES);
    SelectObject(buf->hdc, prev);
    DeleteObject((HGDIOBJ)wallfilling_brush);
    //draw the door
    HBRUSH doorfilling_brush = CreateSolidBrush(RGB_GREEN);
    prev = SelectObject(buf->hdc, doorfilling_brush);
    Polygon(buf->hdc, modular_door, DOOR_VERTICES);
    

    SelectObject(buf->hdc, prev);
    DeleteObject((HGDIOBJ)doorfilling_brush);
    DeleteObject((HGDIOBJ)boundary_pen);
}

void UpdateState(void)
{
    
}
