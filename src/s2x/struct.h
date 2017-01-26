#ifndef S2X_STRUCT_H_INCLUDED
#define S2X_STRUCT_H_INCLUDED

#define S2X_MAX_TRACKS 16
#define S2X_MAX_TRKCHN 8
#define S2X_MAX_SUB_STACK 16
#define S2X_MAX_REPEAT_STACK 12
#define S2X_MAX_LOOP_STACK 12
// PCM instruments
#define S2X_MAX_VOICES_PCM 16
// PCM sound effects / percussion
#define S2X_MAX_VOICES_SE 8
#define S2X_MAX_VOICES_FM 8
#define S2X_MAX_VOICES S2X_MAX_VOICES_PCM+S2X_MAX_VOICES_SE+S2X_MAX_VOICES_FM

#define S2X_MAX_BANK 15

typedef struct S2X_Channel S2X_Channel;
typedef struct S2X_Track S2X_Track;
typedef struct S2X_PCMVoice S2X_PCMVoice;
typedef struct S2X_FMVoice S2X_FMVoice;
typedef struct S2X_SE S2X_SE;
typedef struct S2X_ChannelPriority S2X_ChannelPriority;
typedef struct S2X_State S2X_State;
typedef struct S2X_FMWrite S2X_FMWrite;

struct S2X_Channel {
    uint8_t Enabled;
    uint8_t VoiceNo;

    uint32_t UpdateMask;
    uint8_t Vars[S2X_CHN_MAX];

    struct S2X_Track* Track;
};

struct S2X_Track {
    uint16_t UpdateTime;
    uint8_t BaseTempo;
    uint8_t Tempo;

    uint16_t Position;
    uint32_t PositionBase;

    uint8_t TrackVolume;
    uint16_t Fadeout;

    uint16_t Flags;
    uint16_t TicksLeft;
    uint8_t RestCount;

    uint8_t SubStackPos;
    uint8_t RepeatStackPos;
    uint8_t LoopStackPos;

    struct S2X_Channel Channel[S2X_MAX_TRKCHN];

    uint32_t SubStack[S2X_MAX_SUB_STACK];

    uint32_t RepeatStack[S2X_MAX_REPEAT_STACK];
    uint8_t RepeatCount[S2X_MAX_REPEAT_STACK];

    uint32_t LoopStack[S2X_MAX_LOOP_STACK];
    uint8_t LoopCount[S2X_MAX_LOOP_STACK];

    uint8_t InitFlag; // bitmask of initialized channels
};

struct S2X_ChannelPriority {
    uint16_t priority;
    uint16_t channel;
};

struct S2X_Pitch {
    uint32_t EnvBase; // used to get correct bank for pitch envelopes

    uint16_t Target;
    uint16_t Value;

    uint8_t PortaFlag; // only used as an 'enable' flag
    uint8_t Portamento; // should be updated with track commands

    uint8_t EnvNo;
    uint32_t EnvPos;
    uint32_t EnvLoop;
    uint16_t EnvValue;
    uint8_t EnvSpeed;
    uint8_t EnvDepth;
    uint8_t EnvData;
    uint8_t EnvCounter;
    uint16_t EnvMod;

    uint8_t VolDepth; // Set to enable volume envelope
    uint8_t VolBase;
    uint8_t VolMod;

    S2X_FMVoice* FM;
};

struct S2X_PCMVoice {
    int VoiceNo;

    uint8_t Flag;
    uint16_t ChipFlag;
    uint8_t LinkMode;

    uint8_t Delay;
    uint8_t Length;

    uint8_t WaveNo;
    uint16_t WavePitch;
    uint8_t WaveFlag;
    uint8_t WaveBank;

    uint8_t Volume;

    uint32_t EnvPtr;
    uint32_t EnvPos;
    uint16_t EnvValue;
    uint16_t EnvDelta;
    uint16_t EnvTarget;

    uint8_t EnvAttack;
    uint8_t EnvAttackFine;
    uint8_t EnvDecay;
    uint8_t EnvDecayFine;
    uint8_t EnvSustain;
    uint8_t EnvRelease;
    uint8_t EnvReleaseFine;

    //uint8_t PitchEnvNo;
    struct S2X_Pitch Pitch;

    uint8_t Key;
    //uint8_t Portamento; // only used as an 'enable' flag

    uint8_t Pan;

    uint8_t PanSlide; //only used as an 'enable' flag
    uint8_t PanSlideSpeed;
    uint32_t PanSlidePtr;
    uint8_t PanSlideDelay;

    uint8_t Legato;

    uint32_t BaseAddr;

    S2X_Track* Track;
    S2X_Channel* Channel;
    int TrackNo;
    int ChannelNo;
};

struct S2X_FMVoice {
    int VoiceNo;

    uint8_t Flag;
    uint8_t Delay;
    uint8_t Length;

    uint8_t Lfo;
    uint8_t LfoFlag;
    uint32_t LfoDepthCounter;

    uint32_t InsPtr;
    uint8_t InsNo;
    uint8_t InsLfo; // PMS/AMS sensitivity setting
    uint8_t Carrier;
    uint8_t TL[4];

    uint8_t Volume;
    // volume 'envelopes' work by piggybacking on pitch envelopes
    // also only does attenuation.
    //uint8_t EnvDepth;
    //uint8_t EnvBase;
    //uint8_t EnvMod;

    struct S2X_Pitch Pitch;

    uint8_t Key; // we'll see if needed
    uint8_t ChipFlags; // pan flags

    uint32_t BaseAddr;
    S2X_Track* Track;
    S2X_Channel* Channel;
    int TrackNo;
    int ChannelNo;
};

// used for display
struct S2X_SE {
    int8_t Type;
    uint8_t Track;
    uint8_t Channel;
    uint8_t Voice;
    uint8_t Wave;
};

struct S2X_FMWrite {
    uint8_t Reg;
    uint8_t Data;
};

struct S2X_State {

    // Audio configuration
    double SoundRate;
    double FMDelta;
    double FMTicks;
    double FMWriteTicks;
    double FMWriteRate;

    uint32_t SoloMask;
    uint32_t MuteMask;

    // Sound chips
    uint32_t FMClock;
    YM2151 FMChip;
    uint32_t PCMClock;
    C352 PCMChip; // instead of C140

    // ROM data
    uint8_t *Data;

    // misc
    char *BankName[S2X_MAX_BANK];

    // Sound driver configuration
    int DriverType;
    uint32_t ConfigFlags;
    uint32_t PCMBase;
    uint32_t FMBase;
    uint32_t WaveBase[S2X_MAX_BANK][8];

    // sound driver globals
    uint16_t FrameCnt;
    int BankSelect;
    int CJump;
    uint8_t WaveBank[4];
    uint8_t SongCount[2];

    uint8_t FMLfo;
    uint8_t FMLfoPms;
    uint8_t FMLfoAms;
    uint16_t FMLfoDepthDelta;

    uint16_t FMQueueWrite;
    uint16_t FMQueueRead;
    S2X_FMWrite FMQueue[512];

    // track vars
    uint16_t SongRequest[S2X_MAX_TRACKS+1];
    uint16_t ParentSong[S2X_MAX_TRACKS];
    S2X_Track Track[S2X_MAX_TRACKS];
    double SongTimer[S2X_MAX_TRACKS];
    QP_LoopDetect LoopDetect;

    // voice vars
    S2X_Channel* ActiveChannel[S2X_MAX_VOICES];
    S2X_PCMVoice PCM[S2X_MAX_VOICES_PCM];
    S2X_FMVoice FM[S2X_MAX_VOICES_FM];
//  uint16_t SEWave[S2X_MAX_VOICES_SE];
//  int8_t SEVoice[S2X_MAX_VOICES_SE];
    S2X_SE SE[S2X_MAX_VOICES_SE];

    // List of allocated voices for each track and the associated priority.
    S2X_ChannelPriority ChannelPriority[S2X_MAX_VOICES][S2X_MAX_TRACKS];
};




#endif // STRUCT_H_INCLUDED
