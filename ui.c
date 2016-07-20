/*
    General window/graphics routines and initialization
*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "vgm.h"
#include "SDL2/SDL.h"
#include "SDL2/SDL_audio.h"
#include "SDL2/SDL_events.h"
#include "SDL2/SDL_keyboard.h"
#include "SDL2/SDL_render.h"
#include "ui.h"
#include "ui_menu.h"
#include "loader.h"

#ifdef DEBUG
#define RENDER_PROFILING 1
#endif // DEBUG

#ifdef RENDER_PROFILING

#define RP_START(_rpvar)      _rpvar[1] = SDL_GetPerformanceCounter();
#define RP_END(_rpvar,_rpres) _rpvar[0] = SDL_GetPerformanceCounter();\
                              _rpres = (double)((_rpvar[0] - _rpvar[1])*1000) / SDL_GetPerformanceFrequency();
#else

#define RP_START(_rpvar)
#define RP_END(_rpvar,_rpres)

#endif // RENDER_PROFILING

color_t Colors[14] = {
 {0x00,0x00,0x00}, // Black
 {0xff,0xff,0xff}, // White
 {0x3f,0x3f,0x3f}, // D Grey
 {0x7f,0x7f,0x7f}, // Grey
 {0xbf,0xbf,0xbf}, // L Grey
 {0x7f,0x00,0x00}, // D Red
 {0xff,0x00,0x00}, // Red
 {0xff,0x7f,0x7f}, // L Red
 {0x00,0x7f,0x00}, // D Green
 {0x00,0xff,0x00}, // Green
 {0x7f,0xff,0x7f}, // L Green
 {0x00,0x00,0x7f}, // D Blue
 {0x00,0x00,0xff}, // Blue
 {0x7f,0x7f,0xff}, // L Blue
};

void redraw_text()
{
    //SDL_SetRenderDrawColor(rend,0x10,0,0,255);
    //SDL_RenderClear(rend);

    SDL_Rect fntp;
    SDL_Rect scrp;

    int yshift;
    int yshift_25 = FSIZE_Y/4;
    int yshift_50 = FSIZE_Y/2;
    int yshift_75 = yshift_50 + yshift_25;

    int y,x;
    uint8_t c;
    color_t tc;
    color_t bc;

    rect_count=0;
    draw_count=0;

    scrp.x = 0;
    scrp.y = FSIZE_X*(FROWS-1);
    scrp.w = FSIZE_X;
    scrp.h = FSIZE_Y;

    fntp.h = FSIZE_X;
    fntp.w = FSIZE_Y;

    colorsel_t bgc = 0;

    for(y=FROWS;y--;)
    {
        for(x=0;x<FCOLUMNS;x++)
        {
            yshift=0;
            bgc = bgcolor[y][x];
            if(bgc & CFLAG_YSHIFT_25)
                yshift = yshift_25;
            else if(bgc & CFLAG_YSHIFT_50)
                yshift = yshift_50;
            else if(bgc & CFLAG_YSHIFT_75)
                yshift = yshift_75;

            if(yshift)
                scrp.y += yshift;

            // Draw background
            bc = Colors[bgc&0x7f];

            SDL_SetRenderDrawColor(rend,bc.red,bc.green,bc.blue,255);
            SDL_RenderFillRect(rend,&scrp);
            rect_count++;

            // Draw text
            c = (uint8_t)text[y][x];
            if(c != 0x20 && c != 0x00)
            {
                tc = Colors[textcolor[y][x]&0x7f];
                //SDL_SetRenderDrawColor(rend,0,0x10,0,255);
                SDL_SetTextureColorMod(font,tc.red,tc.green,tc.blue);
                fntp.x = (c%32)*FSIZE_X;
                fntp.y = (c/32)*FSIZE_Y;
                SDL_RenderCopy(rend,font,&fntp,&scrp);
                draw_count++;
            }

            if(yshift)
                scrp.y -= yshift;

            scrp.x += FSIZE_X;
        }
        scrp.x = 0;

        scrp.y -= FSIZE_Y;
    }
}

void set_color(int y,int x,int h,int w,colorsel_t bg,colorsel_t fg)
{
    int i,j,k,l;
    k=h+y;
    l=w+x;
    for(i=y;i<k;i++)
    {
        for(j=x;j<l;j++)
        {
            textcolor[i][j]=fg;
            bgcolor[i][j]=bg;
        }
    }
}

void ui_main()
{
    char windowtitle[256];
    strcpy(windowtitle,"QuattroPlay");

    if(strlen(L_GameTitle))
        sprintf(windowtitle,"%s - QuattroPlay",L_GameTitle);

    Audio->state.MuteRear = muterear;
    Audio->state.Gain = gain*gamegain;
    vol = 1.0;

    #ifdef DEBUG
    printf("Base gain is %.3f\n",gain);
    printf("Game gain is %.3f\n",gamegain);

    printf("Chip Rate is %d Hz\n",QDrv->Chip.rate);
    #endif

    QPAudio_SetPause(Audio,0);
    Audio->state.UpdateRequest = QPAUDIO_CHIP_PLAY|QPAUDIO_DRV_PLAY;
    //Audio->state.UpdateRequest = QPAUDIO_DRV_PLAY;

    set_color(0,0,FROWS,FCOLUMNS,COLOR_D_BLUE,COLOR_N_GREEN);

    window = SDL_CreateWindow(windowtitle,SDL_WINDOWPOS_UNDEFINED,SDL_WINDOWPOS_UNDEFINED,FCOLUMNS*FSIZE_X,FROWS*FSIZE_Y,0);
    rend = SDL_CreateRenderer(window,-1,SDL_RENDERER_ACCELERATED|SDL_RENDERER_PRESENTVSYNC);
    #ifdef DEBUG
    SDL_RendererInfo info;
    SDL_GetRendererInfo(rend,&info);
    printf("Using renderer: %s\n",info.name);
    #endif

    SDL_SetRenderDrawColor(rend,0,0,0,255);
    SDL_RenderClear(rend);
    SDL_RenderPresent(rend);

    SDL_Surface *surface;
    surface = SDL_LoadBMP("font.bmp");

    if(!surface)
    {
        SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR,"Error","could not load 'font.bmp'",NULL);
        return;
    }

    SDL_SetColorKey(surface,SDL_TRUE,0);
    font = SDL_CreateTextureFromSurface(rend,surface);
    SDL_FreeSurface(surface);

    SDL_Event event;

    running = 1;
    debug_stat = 0;

    frame_cnt= 0;
    lasttick= SDL_GetTicks();
    #ifdef RENDER_PROFILING

    lasttick= SDL_GetTicks();

    //uint64_t p1, p2, q1, q2, r1, r2;

    uint64_t rp1[2],rp2[2],rp3[2];
    double rp1r=0, rp2r=0, rp3r=0;
    #endif

    while(running)
    {
        //SDL_WaitEvent(NULL);

        while(SDL_PollEvent(&event))
        {
            switch(event.type)
            {
            case SDL_QUIT:
                running=0;
                break;
            case SDL_KEYDOWN:

                ui_handleinput(&event.key.keysym);
                break;
            default:
                break;
            }
        }

        frame_cnt++;
        if(frame_cnt%UI_FPS_SAMPLES == 0)
        {
            fps_cnt = 1000.0 / ((double)(SDL_GetTicks() - lasttick) / UI_FPS_SAMPLES);
            lasttick = SDL_GetTicks();
        }
        // Redraw the screen


        RP_START(rp1);
        ui_drawscreen();
        RP_END(rp1,rp1r);

        RP_START(rp2);

        if(debug_stat)
        {
            #ifdef RENDER_PROFILING
                sprintf(&text[0][0],"FPS = %6.2f, Draws: %6d, Frame Speed: %6.2f ms, %6.2f ms, %6.2f ms",fps_cnt,draw_count,rp1r,rp2r,rp3r);
            #else
                sprintf(&text[0][0],"FPS = %6.2f, Draws: %6d",fps_cnt,draw_count);
            #endif // RENDER_PROFILING
        }
        else
        {
            sprintf(&text[0][0],"FPS = %6.2f",fps_cnt);
        }
        redraw_text();
        RP_END(rp2,rp2r);

        RP_START(rp3)
        SDL_RenderPresent(rend);
        RP_END(rp3,rp3r);
        //ui_drawscreen();
        //sprintf(&text[0][0],"ID = %04x",count);
        //update_text();
        SDL_Delay(1);
    }

    SDL_DestroyTexture(font);
    SDL_DestroyRenderer(rend);
    SDL_DestroyWindow(window);
}
