#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct NodHandle NodHandle;

typedef enum NodResult {
    NOD_RESULT_OK = 0,
    NOD_RESULT_ERR_IO = 1,
    NOD_RESULT_ERR_FORMAT = 2,
    NOD_RESULT_ERR_UNSUPPORTED = 3,
} NodResult;

typedef struct NodDiscStream {
    void* user_data;
    int64_t (*read_at)(void* user_data, uint64_t offset, void* out, size_t len);
    int64_t (*stream_len)(void* user_data);
    void (*close)(void* user_data);
} NodDiscStream;

typedef struct NodDiscHeader {
    char game_id[6];
} NodDiscHeader;

static inline NodResult nod_disc_open_stream(const NodDiscStream* stream, void* opts, NodHandle** out) {
    (void)stream;
    (void)opts;
    if (out) *out = (NodHandle*)0;
    return NOD_RESULT_ERR_UNSUPPORTED;
}

static inline void nod_free(NodHandle* handle) {
    (void)handle;
}

static inline uint64_t nod_disc_size(NodHandle* handle) {
    (void)handle;
    return 0;
}

static inline NodResult nod_disc_header(NodHandle* handle, NodDiscHeader* out) {
    (void)handle;
    if (out) {
        for (int i = 0; i < 6; ++i) out->game_id[i] = 0;
    }
    return NOD_RESULT_ERR_UNSUPPORTED;
}

static inline const void* nod_buf_read(NodHandle* handle, size_t* bytesAvail) {
    (void)handle;
    if (bytesAvail) *bytesAvail = 0;
    return (const void*)0;
}

static inline void nod_buf_consume(NodHandle* handle, size_t bytes) {
    (void)handle;
    (void)bytes;
}

#ifdef __cplusplus
}
#endif
