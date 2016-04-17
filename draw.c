/***********************************************\
*                     draw.c                    *
*           George Koskeridis (C) 2016          *
\***********************************************/

#include "draw.h"

#define WALL_VERTICES 10
#define DOOR_VERTICES  4

static const POINT lowres_wall_vertices[WALL_VERTICES] = {
    {600, 450}, {600, 30}, {280, 30},
    {280, 40}, {590, 40}, {590, 440},
    {290, 440}, {290, 110}, {280, 110},
    {280, 450}
}, lowres_door_vertices[DOOR_VERTICES] = {
    {280, 110}, {290, 110}, {290, 40}, {280, 40}
};

static POINT modular_wall[WALL_VERTICES], modular_door[DOOR_VERTICES];

void StretchGraphics(int scaling_idx)
{
    for (int i = 0; i < WALL_VERTICES; i++) {
        if (!scaling_idx) {
            modular_wall[i] = lowres_wall_vertices[i];
            if (i < DOOR_VERTICES)
                modular_door[i] = lowres_door_vertices[i];
        } else {
            double tmp = (double)(lowres_wall_vertices[i].x * (pow(1.25, (double)scaling_idx)));

            modular_wall[i].x = (LONG)tmp;
            tmp = (double)(lowres_wall_vertices[i].y * (pow(1.25, (double)scaling_idx)));
            modular_wall[i].y = (LONG)tmp;
            if (i < DOOR_VERTICES) {
                tmp = (double)(lowres_door_vertices[i].x * (pow(1.25, (double)scaling_idx)));
                modular_door[i].x = (LONG)tmp;
                tmp = (double)(lowres_door_vertices[i].y * (pow(1.25, (double)scaling_idx)));
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

    HBRUSH doorfilling_brush = CreateSolidBrush(RGB_GREEN);
    prev = SelectObject(buf->hdc, doorfilling_brush);
    //Polygon(buf->hdc, modular_door, DOOR_VERTICES);
    Rectangle(buf->hdc, 280, 110, 290, 40);

    SelectObject(buf->hdc, prev);
    DeleteObject((HGDIOBJ)doorfilling_brush);
    DeleteObject((HGDIOBJ)boundary_pen);
}

void UpdateState(void)
{
    
}
