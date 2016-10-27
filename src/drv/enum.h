/*
    Quattro - enums
*/
#ifndef ENUM_H_INCLUDED
#define ENUM_H_INCLUDED

typedef enum {
    Q_MCUTYPE_UNIDENTIFIED = 0,
    Q_MCUTYPE_C74 =1, // M37702 System22
    Q_MCUTYPE_C75 =2, // M37702 NB-1/NB-2
    Q_MCUTYPE_C76 =3, // M37702 System11
    Q_MCUTYPE_SS22=4, // M37710S4 SystemSuper22
    Q_MCUTYPE_ND  =5, // H8/3002 ND-1
    Q_MCUTYPE_S12 =6, // H8/3002 System12
    //Q_MCUTYPE_S23 =7, // H8/3002 System23 (Same as System12)
    Q_MCUTYPE_MAX
} Q_McuType;

typedef enum {
    Q_MCUVER_PRE,
    Q_MCUVER_Q00,
    Q_MCUVER_Q01,
    Q_MCUVER_Q02,
} Q_McuDriverVersion;

enum {
    Q_TOFFSET_SONGTABLE = 0,
    Q_TOFFSET_WAVETABLE = 1,
    Q_TOFFSET_PITCHTABLE= 2,
    Q_TOFFSET_ENVTABLE  = 3,
    Q_TOFFSET_PANTABLE  = 4,
    Q_TOFFSET_LFOTABLE  = 5,
    Q_TOFFSET_PRESETMAP = 6,
    Q_TOFFSET_MAX       = 7
};

enum {
    Q_TRACK_STATUS_BUSY = 0x8000,
    Q_TRACK_STATUS_START = 0x4000,
    Q_TRACK_STATUS_FADE = 0x2000,
    Q_TRACK_STATUS_ATTENUATE = 0x1000,
    Q_TRACK_STATUS_SUB = 0x800,
};

enum {
    Q_PANMODE_IMM = 0x7f,
    Q_PANMODE_REG = 0x7d,
    Q_PANMODE_POSREG = 0x7b,

    Q_PANMODE_ENV_SET = 0x7e,
    Q_PANMODE_ENV = 0x7a,

    Q_PANMODE_POSENV_SET = 0x7c,
    Q_PANMODE_POSENV = 0x79,
};

enum {
    Q_GAMEHACK_NONE = 0,
    Q_GAMEHACK_ALPINERD = 0x01,
};

enum {
    Q_EVENTMODE_NOTE = 0,
    Q_EVENTMODE_VOL = 1,
    Q_EVENTMODE_OFFSET = 2,
    Q_EVENTMODE_PAN = 3,
    Q_EVENTMODE_LEGATO = 0x80
};

typedef enum {
    Q_ENV_DISABLE = 0,
    Q_ENV_ATTACK,
    Q_ENV_DECAY,
    Q_ENV_SUSTAIN,
    Q_ENV_STATE_MAX,
} Q_EnvState;

typedef enum {

    Q_PAN_SET,

    Q_PAN_REG,
    Q_PAN_POSREG,

    Q_PAN_ENV,
    Q_PAN_ENV_LEFT,
    Q_PAN_ENV_RIGHT,

    Q_PAN_POSENV,

} Q_PanState;

#endif // ENUM_H_INCLUDED
