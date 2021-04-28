#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#include "s2x.h"
#include "helper.h"
#include "track.h"
#include "voice.h"
#include "wsg.h"

#define SYSTEM1 (S->ConfigFlags & S2X_CFG_SYSTEM1)
#define SYSTEMNA (S->DriverType == S2X_TYPE_NA)
#define SYSTEM86 (S->DriverType == S2X_TYPE_SYSTEM86)
#define SYSTEMEM (S->DriverType == S2X_TYPE_EM)
#define S1_WSG ((S->DriverType == S2X_TYPE_SYSTEM1 || S->DriverType == S2X_TYPE_SYSTEM1_ALT) && TrackNo > 7)
#define S86_WSG (S->DriverType == S2X_TYPE_SYSTEM86 && TrackNo > 7)

// find song position, taking into account the track type and platform...
static void GetSongPos(S2X_State *S,int TrackNo,S2X_Track *T)
{
    uint16_t SongNo = S->SongRequest[TrackNo]&0x1ff;
    T->PositionBase = S->PCMBase;

    // wsg in tracks 8-15
    if(SYSTEM1)
        T->PositionBase = (TrackNo > 7) ? S->WSGBase : S->FMBase;
    // only FM
    else if(SYSTEMEM)
        T->PositionBase = S->FMBase;
    // fm in tracks 8-15
    else if(!SYSTEMNA)
        T->PositionBase = (TrackNo > 7) ? S->FMBase : S->PCMBase;

    // bit 8 for alternate song bank. (not all games have this actually, maybe we should have config option)
    if(SongNo & 0x100)
        T->PositionBase += (SYSTEMNA) ? 0x10000 : 0x20000;

    // system86 games may store the song table anywhere so we read that from config...
    if(SYSTEM86)
        T->Position = S2X_ReadWord(S,T->PositionBase+S->FMSongTab+(2*(SongNo&0xff)));
    // otherwise they're always in the header
    else
        T->Position = S2X_ReadWord(S,T->PositionBase+S2X_ReadWord(S,T->PositionBase)+(2*(SongNo&0xff)));

}

// Read the first byte of a song and determine if it's valid.
// -1 : Valid but all channels need to be initialized manually
//  0 : Invalid
//  1 : Valid
static int CheckValid(S2X_State *S,int TrackNo,S2X_Track *T)
{
    uint16_t SongNo = S->SongRequest[TrackNo]&0x1ff;
    uint8_t header_byte = 0;

    if(T->Position+T->PositionBase < 0x400000)
        header_byte = S2X_ReadByte(S,T->PositionBase+T->Position)&0x3f;

    // skip detection for system86
    if(SYSTEM86)
        return (SongNo&0xff && T->Position >= -T->PositionBase) ? -1 : 0;
    // NA-1/NA-2 has a song count
    else if(SYSTEMNA && (SongNo&0xff) > S->SongCount[SongNo>>8])
        return 0;
    // dspirit & system86 (songs may begin without an INIT command...)
    else if(SYSTEM1 && (header_byte == 0x01 || header_byte == 0x02))
        return -1;
    // the sound driver does a check to make sure first byte is either 0x20 or 0x21
    else if(!SYSTEMNA && header_byte != 0x20 && header_byte != 0x21 && header_byte != 0x1a)
        return 0;
    return 1;
}

void S2X_TrackInit(S2X_State* S, int TrackNo)
{
    S->SongTimer[TrackNo] = 0;

    S2X_Track* T = &S->Track[TrackNo];

    if(T->Flags & S2X_TRACK_STATUS_BUSY)
        S2X_TrackStop(S,TrackNo);

    uint16_t SongNo = S->SongRequest[TrackNo]&0x1ff;

    //T->Position = S2X_GetSongPos(S,SongNo);
    int valid=0;
    if(S1_WSG)
    {
        valid = S2X_S1WSGTrackStart(S,TrackNo,T,SongNo);
    }
    else if(S86_WSG)
    {
        valid = S2X_S86WSGTrackStart(S,TrackNo,T,SongNo);
    }
    else
    {
        GetSongPos(S,TrackNo,T);
        valid = CheckValid(S,TrackNo,T);
    }

    if(!valid)
    {
        Q_DEBUG("Track %02x, song id %04x invalid (start pos: %06x)\n",
                TrackNo,
                SongNo,
                T->PositionBase+T->Position);
        S->SongRequest[TrackNo] &= ~(S2X_TRACK_STATUS_START);
        return;
    }
    T->Flags = S->SongRequest[TrackNo];

    if(!(T->Flags & S2X_TRACK_STATUS_SUB))
        S->ParentSong[TrackNo] = TrackNo;

    // Set track vars...
    T->Flags = (T->Flags&~(S2X_TRACK_STATUS_START))|S2X_TRACK_STATUS_BUSY;
    S->SongRequest[TrackNo] = T->Flags;

    Q_DEBUG("T=%02x playing song %04x at %06x\n",
            TrackNo,
            SongNo,
            T->PositionBase+T->Position);

    if(!S1_WSG && !S86_WSG)
        QP_LoopDetectStart(&S->LoopDetect,TrackNo,S->ParentSong[TrackNo],SongNo+((TrackNo&8)<<6));

    T->SubStackPos=0;
    memset(T->SubStack,0,sizeof(T->SubStack));
    T->RepeatStackPos=0;
    memset(T->RepeatStack,0,sizeof(T->RepeatStack));
    T->LoopStackPos=0;
    memset(T->LoopStack,0,sizeof(T->LoopStack));

    T->Fadeout = 0;
    T->RestCount = 0;
    T->SubStackPos = 0;
    T->LoopStackPos = 0;
    T->InitFlag = 0;
    T->SyncFlag = 0;

    T->BaseTempo=1;

    if(SYSTEM86)
    {
        if(TrackNo < 8)
        {
            T->UpdateTime = -1;
            T->Tempo = 1;
        }
    }
    else
    {
        T->UpdateTime = S->FrameCnt;
        T->Tempo = 0;
    }

    int i;
    for(i=0;i<S2X_MAX_TRKCHN;i++)
    {
        T->Channel[i].VoiceNo = 0xff; // some games do not allocate voices for percussion tracks.
        T->Channel[i].Enabled = 0;
        T->Channel[i].LastEvent = 0;
    }
    // initialize channels if needed
    if(valid == -1)
        S2X_ChannelInit(S,T,TrackNo,0x18,0xff);
}

void S2X_TrackStop(S2X_State* S,int TrackNo)
{
    int i;
    for(i=0;i<S2X_MAX_TRACKS;i++)
    {
        if(S->ParentSong[i] == TrackNo)
        {
            S2X_TrackDisable(S,i);
        }
    }

}

void S2X_TrackUpdate(S2X_State* S,int TrackNo)
{
    // TODO: add a track type variable...
    struct S2X_TrackCommandEntry* CmdTab = S2X_TrackCommandTable[S->DriverType];

    S->SongTimer[TrackNo] += 1.0/S2X_ITickRate(S);

    S2X_Track* T = &S->Track[TrackNo];
    uint8_t Command,CmdIndex;

    if(~T->Flags & S2X_TRACK_STATUS_BUSY)
        return S2X_TrackDisable(S,TrackNo);

    if(S1_WSG)
        return S2X_S1WSGTrackUpdate(S,TrackNo,T);
    else if(S86_WSG)
        return S2X_S86WSGTrackUpdate(S,TrackNo,T);

    if(SYSTEM86 && T->Tempo)
    {
        uint32_t counter = T->UpdateTime + T->Tempo*T->BaseTempo;
        T->UpdateTime=counter&0xffff;
        if(counter < 0x10000)
            return;
    }
    else if((int16_t)(T->UpdateTime-S->FrameCnt) > 0)
        return;

    if(T->RestCount)
    {
        // we're resting...
        T->RestCount--;
    }
    else
    {
        // Parse commands...
        // max amount of commands before switching tracks... (Not a feature of the original driver)
        T->TicksLeft = 1000;

        while(T->TicksLeft)
        {
            T->TicksLeft--;

            QP_LoopDetectCheck(&S->LoopDetect,TrackNo,T->PositionBase+T->Position);

            Command = S2X_ReadByte(S,T->PositionBase+T->Position);
            T->Position++;

            if(Command&0x80)
            {
                T->TicksLeft=0;
                T->RestCount = Command&0x7f;
            }
            else
            {
                CmdIndex=Command&0x3f;
                if(CmdIndex < S2X_MAX_TRKCMD)
                {
                    CmdTab[CmdIndex].cmd(S,TrackNo,T,Command,CmdTab[CmdIndex].param);
                }
                else
                {
                    Q_DEBUG("unrecognized command %02x at %06x\n",Command,T->PositionBase+T->Position-1);
                    S2X_TrackDisable(S,TrackNo);
                }
            }
        }
    }
    // Calculate tempo
    if(!SYSTEM86)
        T->UpdateTime += T->Tempo*T->BaseTempo;
}

// calculate track volume incl fade out and attenuation
void S2X_TrackCalcVolume(S2X_State* S,int TrackNo)
{
    S2X_Track* T = &S->Track[TrackNo];

    if(T->Flags & S2X_TRACK_STATUS_ATTENUATE)
        T->Fadeout = 0x10;//S2X->BaseAttenuation;
    else if(T->Flags & S2X_TRACK_STATUS_FADE)
    {
        T->Fadeout += 0x40;//Q->BaseFadeout;
        if(T->Fadeout > 0xd000) // above threshold?
        {
            // Disable track
            S->SongRequest[TrackNo] &= ~(S2X_TRACK_STATUS_BUSY);
            return;
        }
    }
    else
        T->Fadeout = 0;
}

void S2X_TrackDisable(S2X_State *S,int TrackNo)
{
    S2X_Track* T = &S->Track[TrackNo];
    S2X_Channel* c;
    int i;
    //uint16_t cflags;
    for(i=0;i<S2X_MAX_VOICES;i++)
    {
        S->ChannelPriority[i][TrackNo].channel = 0;
        S->ChannelPriority[i][TrackNo].priority = 0;
    }
    for(i=0;i<S2X_MAX_TRKCHN;i++)
    {
        c = &T->Channel[i];
        c->WSG.Active=0;

        if(c->Enabled)
        {
            c->Enabled=0;
            //c->KeyOnType=0;

            S2X_VoiceClearChannel(S,c->VoiceNo);
            S2X_VoiceClear(S,c->VoiceNo);

            // Switch to next track on the voice
            int next_track = 0;
            int next_channel = 0;
            if(S2X_VoiceGetPriority(S,c->VoiceNo,&next_track,&next_channel))
            {
                Q_DEBUG("Voice %02x free, Now enabling trk %02x ch %02x\n",c->VoiceNo,next_track,next_channel);
                S2X_VoiceSetChannel(S,c->VoiceNo,next_track,next_channel);

                S->Track[next_track].Channel[next_channel].UpdateMask=~((1<<S2X_CHN_PANENV)|(1<<S2X_CHN_FRQ));
                S2X_VoiceCommand(S,&S->Track[next_track].Channel[next_channel],0,0);
            }
        }
    }

    S->SongRequest[TrackNo] &= ~(S2X_TRACK_STATUS_BUSY);
    T->Flags &= S2X_TRACK_STATUS_FADE; // prevent race condition
    S->ParentSong[TrackNo] = S2X_MAX_TRACKS;
    T->TicksLeft = 0;

    Q_DEBUG("Track %02x terminated\n",TrackNo);
}

void S2X_ChannelClear(S2X_State *S,int TrackNo,int ChannelNo)
{
    S2X_Channel *C = &S->Track[TrackNo].Channel[ChannelNo];
    uint8_t VoiceNo = C->VoiceNo;
    uint8_t VoiceNo2 = C->Vars[S2X_CHN_VNO];

    memset(C,0,sizeof(S2X_Channel));
    C->VoiceNo = VoiceNo;
    C->Track = &S->Track[TrackNo];
    C->Vars[S2X_CHN_LFO] = 0xff; // disables LFO
    C->Vars[S2X_CHN_PAN] = 0x80;
    C->Vars[S2X_CHN_WAV] = 0xff;
    C->Vars[S2X_CHN_VNO] = VoiceNo2;
}
