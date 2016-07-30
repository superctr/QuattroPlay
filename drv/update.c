/*
    Quattro - update functions
*/
#include "quattro.h"
#include "update.h"
#include "track.h"
#include "voice.h"

// call 0x04 - update tracks and execute song requests
// source: 0x4a56
void Q_UpdateTracks(Q_State *Q)
{
    int TrackNo;
    for(TrackNo=0;TrackNo<Q->TrackCount;TrackNo++)
    {
        if(Q->SongRequest[TrackNo] & Q_TRACK_STATUS_START)
        {
            Q_TrackInit(Q,TrackNo);
        }
        else if(Q->Track[TrackNo].Flags & Q_TRACK_STATUS_BUSY)
        {
            Q->Track[TrackNo].Flags = Q->SongRequest[Q->ParentSong[TrackNo]];
            Q_TrackUpdate(Q,TrackNo);
            Q_TrackCalcVolume(Q,TrackNo);
        }
    }
}

// call 0x26 - update all voices
// source: 0x6238
void Q_UpdateVoices(Q_State* Q)
{
    Q_Voice *V;
    Q_VoiceEvent *E;
    int16_t timeleft;
    int VoiceNo;
    for(VoiceNo=0;VoiceNo<Q->VoiceCount;VoiceNo++)
    {
        V = &Q->Voice[VoiceNo];

        timeleft = (int16_t)(V->Event[(V->CurrEvent)&0x07].Time - Q->FrameCnt);

        if(V->CurrEvent != V->LastEvent && timeleft < 0)
        {
            while(1)
            {
                E = &V->Event[(V->CurrEvent++)&0x07];
                if(V->CurrEvent == V->LastEvent)
                    break;
                timeleft = (int16_t)(V->Event[(V->CurrEvent)&0x07].Time - Q->FrameCnt);
                if(timeleft > 0)
                    break;
            }
            Q_VoiceProcessEvent(Q,VoiceNo,V,E);
        }

        if(V->Enabled)
            Q_VoiceUpdate(Q,VoiceNo,V);
    }
    C352_write(&Q->Chip,0x202,VoiceNo); // update key-ons
}
