/*
    Loop detection library
*/
#include <stdlib.h>
#include <string.h>
#include <limits.h>

#include "loopdetect.h"

// Initialize loop detection and allocate memory for the loop detection state.
// You should not really be calling any other loop detection functions if this fails (returns nonzero).
int QP_LoopDetectInit(QP_LoopDetect *ld)
{
    if(!ld->Driver || !ld->DataSize || !ld->SongCnt || !ld->TrackCnt || !ld->CheckValid)
    {
        ld->NextLoopId = 0;
        return -1;
    }

    ld->Data = malloc(ld->DataSize*sizeof(*ld->Data));
    memset(ld->Data,0,ld->DataSize*sizeof(*ld->Data));
    ld->Song = malloc(ld->SongCnt*sizeof(*ld->Song));
    memset(ld->Song,0,ld->SongCnt*sizeof(*ld->Song));
    ld->Track = malloc(ld->TrackCnt*sizeof(*ld->Track));
    memset(ld->Song,0,ld->SongCnt*sizeof(*ld->Track));
    QP_LoopDetectReset(ld);
    ld->NextLoopId = 1;
    return 0;
}
// Free allocated memory
void QP_LoopDetectFree(QP_LoopDetect *ld)
{
    free(ld->Data);
    free(ld->Song);
    free(ld->Track);
}
static int GetNextId(QP_LoopDetect *ld)
{
    int id = ++ld->NextLoopId;
    if(!ld->NextLoopId)
        ld->NextLoopId++;
    return id;
}
// Reset loop detection state
void QP_LoopDetectReset(QP_LoopDetect *ld)
{
    memset(ld->Song,0,ld->SongCnt*sizeof(*ld->Song));
    memset(ld->Track,0,ld->TrackCnt*sizeof(*ld->Track));
    ld->NextLoopId = GetNextId(ld);
}
// Call this when starting the track.
void QP_LoopDetectStart(QP_LoopDetect *ld,int trackid,int parent,int songid) // start loop detection for a songid
{
    ld->Track[trackid].SongId = songid;
    ld->Track[trackid].Parent = parent+1;
    ld->Song[ld->Track[trackid].SongId].StackPos=0;
}
// Call this when stopping the track.
void QP_LoopDetectStop(QP_LoopDetect *ld,int trackid) // stop loop detection for a track
{
    ld->Song[ld->Track[trackid].SongId].LoopCnt = -1;
    ld->Track[trackid].Parent=0;
}
// Check for a loop at the specified track position.
void QP_LoopDetectCheck(QP_LoopDetect *ld,int trackid,unsigned int position) // check address
{
    if(ld->NextLoopId == 0)
        return;

    struct QP_LoopDetectSong *S = &ld->Song[ld->Track[trackid].SongId];

    if(S->StackPos >= LOOPDETECT_MAX_STACK)
        return;

    if(ld->CheckValid(ld->Driver,trackid))
        return;

    if(S->LoopId[S->StackPos] == 0)
        S->LoopId[S->StackPos] = GetNextId(ld);

    int* data = &ld->Data[position];

    if(*data == S->LoopId[S->StackPos] && S->LoopCnt>=0 && S->LoopCnt < INT_MAX)
    {
        S->LoopCnt++;
        S->LoopId[S->StackPos] = GetNextId(ld);
    }
    *data = S->LoopId[S->StackPos];
}
// Check the jump address if conflicting with another track.
// Call this when jumping to a position
// (Unless you don't expect to check for loop, such as when your CheckValid returns nonzero)
void QP_LoopDetectJump(QP_LoopDetect *ld,int trackid,unsigned int position) // check for position overlap
{
    if(ld->NextLoopId == 0)
        return;

    struct QP_LoopDetectSong *S = &ld->Song[ld->Track[trackid].SongId];
    struct QP_LoopDetectSong *S2;

    if(S->LoopCnt<0)
        return;

    int* data = &ld->Data[position];

    if(!*data)
        return;

    int i, j;

    for(i=0;i<ld->TrackCnt;i++)
    {
        if(i==trackid)
            continue;
        if(ld->Track[i].Parent != ld->Track[trackid].Parent && (ld->Track[i].Parent-1) != trackid)
            continue;

        S2 = &ld->Song[ld->Track[i].SongId];
        for(j=0;j<=S2->StackPos;j++)
        {
            if(*data == S2->LoopId[j])
            {
                // subroutine- we just take the same loop ID
                if(S->StackPos)
                    S->LoopId[j] = *data;
                S->LoopCnt = -2;
                return;
            }
        }
    }
}
// Pop the loop ID. Call this when entering subroutines (along with QP_LoopDetectJump)
void QP_LoopDetectPush(QP_LoopDetect *ld,int trackid) // push loopid
{
    struct QP_LoopDetectSong *S = &ld->Song[ld->Track[trackid].SongId];
    if(S->StackPos >= LOOPDETECT_MAX_STACK)
        return;
    S->LoopId[++S->StackPos] = GetNextId(ld);
}
// Pop the loop ID. Call this when returning from subroutines (along with QP_LoopDetectJump)
void QP_LoopDetectPop(QP_LoopDetect *ld,int trackid) // pop loopid
{
    struct QP_LoopDetectSong *S = &ld->Song[ld->Track[trackid].SongId];
    if(!S->StackPos || S->StackPos > LOOPDETECT_MAX_STACK)
        return;
    S->StackPos--;
}
// Get amount of loops
int  QP_LoopDetectGetCount(QP_LoopDetect *ld,int trackid)
{
    int loopcount;
    int lowest = ld->Song[ld->Track[trackid].SongId].LoopCnt;
    int i;
    for(i=0;i<ld->TrackCnt;i++)
    {
        if((ld->Track[i].Parent-1) == trackid)
        {
            loopcount = ld->Song[ld->Track[i].SongId].LoopCnt;
            if(loopcount >= 0 && loopcount < lowest)
                lowest = loopcount;
        }
    }
    return lowest;
}
