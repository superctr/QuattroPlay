#ifndef AUDIT_H_INCLUDED
#define AUDIT_H_INCLUDED

#define AUDIT_MAX_COUNT 512
#define AUDIT_MAX_ROMS 8

typedef struct QP_AuditEntry QP_AuditEntry;
typedef struct QP_Audit QP_Audit;

enum {
    AUDIT_PATH_DATA,
    AUDIT_PATH_WAVE
};
struct QP_AuditRom{
    char Path[128];
    int Ok;
};

struct QP_AuditEntry{
    char Name[256];
    char DisplayName[256];
    int HasPlaylist;
    int HasMeta;
    int RomCount;
    struct QP_AuditRom Rom[AUDIT_MAX_ROMS];
    int IniOk;
    int RomOk;
};

struct QP_Audit{
    int Count;
    QP_AuditEntry Entry[AUDIT_MAX_COUNT];
    int AuditFlag;
    int CheckCount;
    int OkCount;
    int BadCount;
};

int AuditGames(void* data);
int AuditRoms(void* data);

// void FreeAudit(QP_Audit* audit);
#endif // AUDIT_H_INCLUDED
