#include "string.h"
#include "../qp.h"

#include "ui.h"
#include "scr_main.h"

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

void ui_drawscreen()
{
    int i;
    set_color(0,0,FROWS,FCOLUMNS,COLOR_BLACK,COLOR_L_GREY);

    memset(screen.text,0,sizeof(screen.text));

    if(!debug_stat && gameloaded)
    {
        i=SCRN(0,14,60,"Volume: %4.2f [%s] %s",vol,
                 Audio->state.MuteRear ? "Stereo" : " Quad ",
                 Audio->state.FastForward ? "Fast Forward" : "");
        if(Audio->state.FileLogging)
            SCRN(0,15+i,20,"Logging %8d ...",Audio->state.LogSamples);
    }

    switch(screen_mode)
    {
    case SCR_ABOUT:
        scr_about();
        break;
    case SCR_PLAYLIST:
        scr_playlist();
        break;
    case SCR_SELECT:
        scr_select();
        break;
    default:
        scr_main();
        break;
    }

    got_input=0;
}

void ui_handleinput(SDL_Keysym* ks)
{
    keycode = ks->sym;

    switch(keycode)
    {
    case SDLK_u:
        Q_UpdateTick(QDrv);
        break;
    case SDLK_q:
        if(screen_mode == SCR_MAIN || screen_mode == SCR_SELECT)
            running=0;
        else
            screen_mode = SCR_MAIN;
        break;
    case SDLK_p:
        if(gameloaded)
            QPAudio_TogglePause(Audio);
        break;
    case SDLK_F1:
        if(screen_mode == SCR_ABOUT)
        {
            screen_mode = last_scrmode;
        }
        else
        {
            last_scrmode = screen_mode;
            screen_mode = SCR_ABOUT;
        }
        break;
    case SDLK_SPACE:
        if(gameloaded)
        {
            if(screen_mode == SCR_PLAYLIST)
                screen_mode = SCR_MAIN;
            else
                screen_mode = SCR_PLAYLIST;
        }
        break;
    case SDLK_F3:
        if(gameloaded)
        {
            Game->PlaylistControl = 0;
            SDL_LockAudioDevice(Audio->dev);
            QDrv->BootSong=Game->BootSong;
            Q_Reset(QDrv);
            SDL_UnlockAudioDevice(Audio->dev);
        }
        break;
    case SDLK_F5:
        Audio->state.MuteRear ^= 1;
        break;
    case SDLK_F6:
        if(gameloaded)
        {
            QDrv->MuteMask=0;
            QDrv->SoloMask=0;
            Q_UpdateMuteMask(QDrv);
        }
        break;
    case SDLK_F7:
        vol -= 0.05;
    case SDLK_F8:
        if(keycode==SDLK_F8)
            vol += 0.05;
        if(vol>9.95)
            vol=9.95;
        if(vol<0)
            vol=0;
        Audio->state.Gain = Game->BaseGain*Game->Gain*vol;
        break;
    case SDLK_F10:
        Audio->state.FastForward ^= 1;
        break;
    case SDLK_F11:
        if(gameloaded)
        {
            SDL_LockAudioDevice(Audio->dev);
            if(Audio->state.FileLogging == 0)
                QPAudio_WavOpen(Audio,"qp_log.wav");
            else
                QPAudio_WavClose(Audio);
            SDL_UnlockAudioDevice(Audio->dev);
        }
        break;
    case SDLK_F12:
        debug_stat ^= 1;
        break;
    default:
        got_input=1;
        break;
    }
}

int ui_main(screen_mode_t sm)
{
    gameloaded=1;
    if(sm == SCR_SELECT)
        gameloaded=0;

    char windowtitle[256];
    strcpy(windowtitle,"QuattroPlay");

    if(strlen(Game->Title) && gameloaded)
        sprintf(windowtitle,"%s - QuattroPlay",Game->Title);

    vol = 1.0;

    #ifdef DEBUG
    printf("Base gain is %.3f\n",Game->BaseGain);
    printf("Game gain is %.3f\n",Game->Gain);

    printf("Chip Rate is %d Hz\n",QDrv->Chip.rate);
    #endif

    SDL_SetWindowTitle(window,windowtitle);

    SDL_Event event;

    screen_mode = sm;
    running = 1;
    returncode=0;
    refresh=-1;
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
        SDL_SetRenderTarget(rend,dispbuf);
        ui_drawscreen();
        if(debug_stat)
        {
            #ifdef RENDER_PROFILING
                SCR(0,0,"FPS = %6.2f, Draws: %6d, Frame Speed: %6.2f ms, %6.2f ms, %6.2f ms",fps_cnt,draw_count,rp1r,rp2r,rp3r);
            #else
                sprintf(&screen.text[0][0],"FPS = %6.2f, Draws: %6d",fps_cnt,draw_count);
            #endif // RENDER_PROFILING
        }
        else
        {
            SCR(0,0,"FPS = %6.2f",fps_cnt);
        }
        RP_END(rp1,rp1r);

        RP_START(rp2);
        ui_update();
        RP_END(rp2,rp2r);

        RP_START(rp3)
        ui_refresh();
        RP_END(rp3,rp3r);
        //ui_drawscreen();
        //sprintf(&screen.text[0][0],"ID = %04x",count);
        //update_text();
        SDL_Delay(1);
    }

    return returncode;
}
