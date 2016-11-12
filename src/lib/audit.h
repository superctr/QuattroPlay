#ifndef AUDIT_H_INCLUDED
#define AUDIT_H_INCLUDED

#define AUDIT_MAX_COUNT 512
#define AUDIT_MAX_ROMS 8

typedef struct QPAuditEntry QPAuditEntry;
typedef struct QPAudit QPAudit;

enum {
    AUDIT_PATH_DATA,
    AUDIT_PATH_WAVE
};
struct QPAuditRom{
    char Path[128];
    int Ok;
};

struct QPAuditEntry{
    char Name[256];
    char DisplayName[256];
    int HasPlaylist;
    int HasMeta;
    int RomCount;
    struct QPAuditRom Rom[AUDIT_MAX_ROMS];
    int IniOk;
};

struct QPAudit{
    int Count;
    QPAuditEntry Entry[AUDIT_MAX_COUNT];
};

int AuditGames(QPAudit* audit);
void FreeAudit(QPAudit* audit);

#endif // AUDIT_H_INCLUDED
