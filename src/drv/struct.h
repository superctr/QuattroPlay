/*
    Quattro - data structures
*/
#ifndef STRUCT_H_INCLUDED
#define STRUCT_H_INCLUDED

#define Q_MAX_TRKCHN 8
#define Q_MAX_SUB_STACK 6
#define Q_MAX_REPEAT_STACK 4
#define Q_MAX_LOOP_STACK 3
#define Q_MAX_DJUMP_STACK 3     /* Temporary!! */
#define Q_MAX_VOICES 32

typedef struct Q_Channel Q_Channel;
typedef struct Q_Track Q_Track;
typedef struct Q_ChannelPriority Q_ChannelPriority;
typedef struct Q_VoiceEvent Q_VoiceEvent;
typedef struct Q_Voice Q_Voice;
typedef struct Q_State Q_State;

struct Q_Channel {
    uint16_t WaveNo;
    uint8_t Volume;
    uint8_t Pan;
    uint8_t Detune;
    uint8_t BaseNote;
    uint16_t EnvNo;
    uint16_t PitchEnvNo;
    uint8_t NoteDelay;
    uint8_t GateTime;
    uint8_t SampleOffset;
    uint8_t Transpose;
    uint8_t LfoNo;
    uint8_t Portamento;
    uint8_t PanMode;
    uint8_t PitchReg;
    uint8_t PresetMap;
    uint8_t VoiceNo;
    uint8_t Legato;
    uint8_t Enabled;
    uint8_t ChannelLink;
    uint8_t Preset;
    Q_Voice* Voice;
    Q_Channel* Source;
    uint8_t KeyOnNote;
    uint8_t KeyOnFlag;
    uint8_t VolumeReg;
    uint8_t CutoffMode;

    uint16_t Unused;
    uint8_t KeyOnType; // for display
};

struct Q_Track {
    uint16_t UpdateTime; // 0
    uint32_t TempoSeqStart;
    uint32_t TempoSeqPos;
    uint16_t TrackVolume; // This is actually 8-bits but we use 16-bit since it's pointed at by VolumeSource.
    uint8_t BaseTempo;

    uint8_t TempoFraction;
    uint8_t Tempo;
    uint16_t Flags;
    uint8_t TempoMulFactor;
    uint8_t TempoMode;
    uint16_t Unused1;

    uint32_t Position;

    uint8_t KeyOnBuffer;
    uint8_t SubStackPos;
    uint8_t RepeatStackPos;
    uint8_t LoopStackPos;
    uint8_t SkipTrack;

    uint16_t Fadeout;
    uint8_t RestCount;
    uint8_t TempoReg;
    uint16_t* TempoSource;
    uint16_t* VolumeSource;

    uint16_t GlobalVolume;

    uint16_t TicksLeft;
    uint8_t Unused2;

    uint32_t SubStack[Q_MAX_SUB_STACK];

    uint32_t RepeatStack[Q_MAX_REPEAT_STACK];
    uint8_t RepeatCount[Q_MAX_REPEAT_STACK];

    uint32_t LoopStack[Q_MAX_DJUMP_STACK];
    uint8_t LoopCount[Q_MAX_DJUMP_STACK];

    Q_Channel Channel[Q_MAX_TRKCHN];

};

struct Q_ChannelPriority {
    uint16_t priority;
    uint16_t channel;
};

struct Q_VoiceEvent {
    uint16_t Time;
    uint8_t Value;
    uint8_t Mode;
    Q_Channel *Channel;
    uint16_t *Volume; // ??
};

struct Q_Voice {
    uint16_t WaveNo;
    uint8_t Volume;
    uint8_t Pan;
    uint8_t Detune;
    uint8_t BaseNote;
    uint16_t EnvNo;
    uint16_t PitchEnvNo;
    uint8_t NoteDelay;
    uint8_t GateTime;
    uint8_t SampleOffset;
    uint8_t Transpose;
    uint8_t LfoNo;
    uint8_t Portamento;
    uint8_t PanMode;
    uint8_t PitchReg;

    // wave
    uint16_t WaveFlags;
    uint16_t WaveBank;
    uint16_t WaveTranspose;
    uint8_t WaveLinkFlag;
    uint32_t WavePos; // WaveIndexPtr
    uint32_t WaveLinkPos; // WaveNextPtr

    // volume
    uint16_t VolumeFront;
    uint16_t VolumeRear;
    uint16_t *TrackVol;
    uint8_t VolumeMod;

    // volume envelope
    Q_EnvState EnvState;

    uint32_t EnvPos;
    uint32_t EnvIndexPos;

    uint16_t EnvValue;
    uint16_t EnvTarget;
    uint16_t EnvDelta;

    // pan / pan envelope
    Q_PanState PanState;
    uint16_t *PanSource;

    uint32_t PanEnvPos;
    uint32_t PanEnvLoop;

    uint16_t PanEnvTarget;
    uint16_t PanEnvValue;
    uint16_t PanEnvValue2;
    uint16_t PanEnvDelta;
    uint8_t PanEnvDelay;

    uint8_t PanMode2;
    uint8_t PanUpdateFlag;

    // pitch
    uint16_t PitchTarget;
    uint16_t Pitch;

    // pitch envelope
    uint32_t PitchEnvPos;
    uint32_t PitchEnvLoop;
    uint16_t PitchEnvValue;

    uint8_t PitchEnvSpeed;
    uint8_t PitchEnvDepth;
    uint8_t PitchEnvData;
    uint8_t PitchEnvCounter;

    uint16_t PitchEnvMod;

    uint16_t FreqReg;
    //...

    // LFO
    uint8_t LfoEnable;
    uint16_t LfoMod;
    uint16_t LfoPhase;
    uint8_t LfoWaveform;
    uint8_t LfoDelay;
    uint16_t LfoFreq;
    uint16_t LfoFreqTarget;
    uint16_t LfoFreqDelta;
    uint16_t LfoDepth;
    uint16_t LfoDepthTarget;
    uint16_t LfoDepthDelta;

    // event/general
    uint8_t GateTimeLeft;
    uint8_t Enabled;
    uint8_t CurrEvent; // current event position
    uint8_t LastEvent; // last event position
    Q_Channel* EventCh; // used when adding events
    Q_Channel* Channel;
    Q_VoiceEvent Event[8];

    // display (not present in original driver)
    uint8_t TrackNo;
    uint8_t ChannelNo;
};

struct Q_State {

// ========================================================================= //
// System state etc

    // These are set by loader.c
    uint8_t *McuData;
    Q_McuType McuType;
    Q_McuDriverVersion McuVer;

    uint32_t McuPosBase;        // base position (address of data ROM in MCU address space)
    uint32_t McuHeaderPos;      // position of header offset table.
    uint32_t McuDataPosBase;    // usually bank position of header offset
    uint32_t McuDrvStartPos;
    uint32_t McuDrvEndPos;
    uint16_t SongCount;

    uint32_t ChipClock;
    C352 Chip;
    DelayDSP Delay;

// ========================================================================= //
// Game hacks - hopefully we'll see as little of these as possible.
//    uint32_t GameHacks;

// ========================================================================= //
// QuattroPlay added variables

    uint32_t MuteMask;
    uint32_t SoloMask;

    double SongTimer[Q_MAX_TRACKS];
#ifndef Q_DISABLE_LOOP_DETECTION
    uint32_t* LoopCounterFlags;
    uint32_t TrackLoopId[0x800];
    uint8_t TrackLoopCount[0x800];
    uint16_t NextLoopId; // set 0 to disable loop detection
#endif

    // controls startup sound (ie Tekken "Good Morning!" sample)
    // 0=don't play/done, 1=play, 2=silent (just to set initial registers/pitch)
    uint8_t BootSong;

    // if set, voice pitch is set when the sound driver is reset,
    // fixing songs that begin with portamentos
    uint8_t PortaFix;

// ========================================================================= //
// Quattro variables

    uint16_t SongRequest[Q_MAX_TRACKS+1];
    uint16_t ParentSong[Q_MAX_TRACKS];
    uint16_t TrackParam[Q_MAX_TRACKS];
    uint16_t Register[Q_MAX_REGISTER];
    char SongMessage[128];

    // drv_08 uses this
    // this is the current allocated channel on the voice
    Q_Channel* ActiveChannel[Q_MAX_VOICES];
    // List of allocated voices for each track and the associated priority.
    Q_ChannelPriority ChannelPriority[Q_MAX_VOICES][Q_MAX_TRACKS];

    uint16_t BasePitch;
    uint8_t BaseFadeout;
    uint8_t BaseAttenuation;
    uint16_t FrameCnt;

    uint16_t LFSR1;
    uint16_t LFSR2;
    uint8_t SetRegFlags; // flags set by "set register" command

    uint8_t PanMask;

    uint32_t TableOffset[Q_TOFFSET_MAX];

    uint16_t TrackCount;
    Q_Track Track[Q_MAX_TRACKS];

    uint16_t VoiceCount;
    Q_Voice Voice[Q_MAX_VOICES];

    Q_Channel ChannelPreset[256];

    uint16_t PitchTable[256];
};

#endif // STRUCT_H_INCLUDED
