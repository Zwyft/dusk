#if defined(__SWITCH__)

#include <aurora/dvd.h>
#include <dolphin/dvd.h>

extern "C" {

bool aurora_dvd_open(const char* /*disc_path*/) { return false; }
void aurora_dvd_close(void) {}

void DVDInit(void) {}
BOOL DVDCheckDisk(void) { return FALSE; }
BOOL DVDOpen(const char* /*fileName*/, DVDFileInfo* /*fileInfo*/) { return FALSE; }
BOOL DVDFastOpen(s32 /*entrynum*/, DVDFileInfo* /*fileInfo*/) { return FALSE; }
BOOL DVDClose(DVDFileInfo* /*fileInfo*/) { return FALSE; }
BOOL DVDChangeDir(const char* /*dirName*/) { return FALSE; }
s32 DVDConvertPathToEntrynum(const char* /*pathPtr*/) { return -1; }
BOOL DVDLowReadDiskID(DVDDiskID* /*diskID*/, DVDLowCallback callback) {
    if (callback) callback(0);
    return FALSE;
}
s32 DVDGetDriveStatus(void) { return -1; }
s32 DVDGetCommandBlockStatus(const DVDCommandBlock* /*block*/) { return -1; }
BOOL DVDReadAsyncPrio(DVDFileInfo* /*fileInfo*/, void* /*addr*/, s32 /*length*/, s32 /*offset*/, DVDCallback callback, s32 /*prio*/) {
    if (callback) callback(-1, nullptr);
    return FALSE;
}
s32 DVDReadPrio(DVDFileInfo* /*fileInfo*/, void* /*addr*/, s32 /*length*/, s32 /*offset*/, s32 /*prio*/) { return -1; }
s32 DVDCancel(volatile DVDCommandBlock* /*block*/) { return -1; }
int DVDOpenDir(const char* /*dirName*/, DVDDir* /*dir*/) { return -1; }
int DVDReadDir(DVDDir* /*dir*/, DVDDirEntry* /*dirent*/) { return -1; }
int DVDCloseDir(DVDDir* /*dir*/) { return -1; }
const u8* DVDGetDOLLocation(s32* out_size) { if (out_size) *out_size = 0; return nullptr; }

}

#endif
