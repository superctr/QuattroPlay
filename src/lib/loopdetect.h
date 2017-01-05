#ifndef LOOPDETECT_H_INCLUDED
#define LOOPDETECT_H_INCLUDED

#define LOOPDETECT_MAX_STACK 4

typedef struct QP_LoopDetect QP_LoopDetect;

struct QP_LoopDetectSong
{
    int StackPos;
    int LoopId[LOOPDETECT_MAX_STACK];
    int LoopCnt;
};
struct QP_LoopDetectTrack
{
    int SongId;
    int Parent;
};

struct QP_LoopDetect
{
    int NextLoopId;
    int DataSize;
    int *Data;
    void *Driver;
    int SongCnt;
    int TrackCnt;
    struct QP_LoopDetectSong *Song;
    struct QP_LoopDetectTrack *Track;

    int (*CheckValid)(void *driver,int trackid);
};

// initialize loop detection
int  QP_LoopDetectInit(QP_LoopDetect *ld);
void QP_LoopDetectFree(QP_LoopDetect *ld);
void QP_LoopDetectReset(QP_LoopDetect *ld);

void QP_LoopDetectStart(QP_LoopDetect *ld,int trackid,int parent,int songid); // start loop detection for a songid
void QP_LoopDetectStop(QP_LoopDetect *ld,int trackid); // stop loop detection for a track
void QP_LoopDetectCheck(QP_LoopDetect *ld,int trackid,unsigned int position); // check address
void QP_LoopDetectJump(QP_LoopDetect *ld,int trackid,unsigned int position); // check for position overlap
void QP_LoopDetectPush(QP_LoopDetect *ld,int trackid); // push loopid
void QP_LoopDetectPop(QP_LoopDetect *ld,int trackid); // pop loopid

int  QP_LoopDetectGetCount(QP_LoopDetect *ld,int trackid);

#endif // LOOPDETECT_H_INCLUDED
