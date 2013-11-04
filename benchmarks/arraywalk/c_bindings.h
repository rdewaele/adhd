#pragma once

#ifdef __cplusplus
#include <cstdint>
#include <type_traits>
#define SIZE_T std::size_t
#else
#include <stdint.h>
#include <stddef.h>
#define SIZE_T size_t
#endif /* __cplusplus */

#include "arraywalk_def.h"

#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#ifdef __cplusplus
#define ENUM(DEC) arraywalk::DEC
#else
#define ENUM(DEC) enum DEC
#endif

enum awTypeWidth { AW8, AW16, AW32, AW64, AW128 };

void * aw_alloc(enum awTypeWidth, SIZE_T, ENUM(pattern));

void aw_timedwalk_loc(void *, unsigned, uint_fast32_t, uint64_t *, uint64_t *);

void aw_free(void *);

#ifdef __cplusplus
}
#endif /* __cplusplus */
