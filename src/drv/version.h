#ifndef VERSION_H_INCLUDED
#define VERSION_H_INCLUDED

// Version capabilities/behavior enums.
typedef enum {
    Q_CAP_PAN_ENV_RESET     = 1<<0, // Pan envelope reset behavior (voice_pan.c)
    Q_CAP_CUTOFF_MODE       = 1<<1, // Cutoff mode behavior (track.c)
    Q_CAP_PRESET_EVENT_MODE = 1<<2, // Ignore event mode if Note mapped preset enabled (track_cmd.c)
    Q_CAP_WRITE_CMD         = 1<<3, // Byte write commands ignored (track_cmd.c)
} Q_Capability;

#define QC_PAN_ENV_RESET(var) (var&Q_CAP_PAN_ENV_RESET)
#define QC_CUTOFF_MODE(var) (var&Q_CAP_CUTOFF_MODE)
#define QC_PRESET_EVENT_MODE(var) (var&Q_CAP_PRESET_EVENT_MODE)
#define QC_WRITE_CMD(var) (var&Q_CAP_WRITE_CMD)

typedef struct Q_Version Q_Version;

struct Q_Version {
    Q_McuType McuType;
    int StartOffset;
    int Size;
    uint32_t CRC32;
    char* VersionString;
    char* ShortName;
    Q_Capability Capability;
};

#endif // VERSION_H_INCLUDED
