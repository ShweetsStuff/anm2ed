#pragma once

#include <limits.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef __STDC_VERSION_STDCKDINT_H__
  #define __STDC_VERSION_STDCKDINT_H__ 202311L

  #define __CKD_ADD_UNSIGNED(name, type)                                                                               \
    static inline bool ckd_add_##name(type* result, type a, type b)                                                    \
    {                                                                                                                  \
      type tmp = (type)(a + b);                                                                                        \
      *result = tmp;                                                                                                   \
      return tmp < a;                                                                                                  \
    }

  #define __CKD_SUB_UNSIGNED(name, type)                                                                               \
    static inline bool ckd_sub_##name(type* result, type a, type b)                                                    \
    {                                                                                                                  \
      *result = (type)(a - b);                                                                                         \
      return a < b;                                                                                                    \
    }

  #define __CKD_ADD_SIGNED(name, type, min_v, max_v)                                                                   \
    static inline bool ckd_add_##name(type* result, type a, type b)                                                    \
    {                                                                                                                  \
      const type minValue = (type)(min_v);                                                                             \
      const type maxValue = (type)(max_v);                                                                             \
      const type tmp = (type)(a + b);                                                                                  \
      *result = tmp;                                                                                                   \
      return (b > 0 && a > (type)(maxValue - b)) || (b < 0 && a < (type)(minValue - b));                               \
    }

  #define __CKD_SUB_SIGNED(name, type, min_v, max_v)                                                                   \
    static inline bool ckd_sub_##name(type* result, type a, type b)                                                    \
    {                                                                                                                  \
      const type minValue = (type)(min_v);                                                                             \
      const type maxValue = (type)(max_v);                                                                             \
      const type tmp = (type)(a - b);                                                                                  \
      *result = tmp;                                                                                                   \
      return (b > 0 && a < (type)(minValue + b)) || (b < 0 && a > (type)(maxValue + b));                               \
    }

__CKD_ADD_UNSIGNED(u8, uint8_t)
__CKD_ADD_UNSIGNED(u16, uint16_t)
__CKD_ADD_UNSIGNED(u32, uint32_t)
__CKD_ADD_UNSIGNED(u64, uint64_t)
__CKD_ADD_UNSIGNED(usz, size_t)

__CKD_SUB_UNSIGNED(u8, uint8_t)
__CKD_SUB_UNSIGNED(u16, uint16_t)
__CKD_SUB_UNSIGNED(u32, uint32_t)
__CKD_SUB_UNSIGNED(u64, uint64_t)
__CKD_SUB_UNSIGNED(usz, size_t)

__CKD_ADD_SIGNED(i8, int8_t, INT8_MIN, INT8_MAX)
__CKD_ADD_SIGNED(i16, int16_t, INT16_MIN, INT16_MAX)
__CKD_ADD_SIGNED(i32, int32_t, INT32_MIN, INT32_MAX)
__CKD_ADD_SIGNED(i64, int64_t, INT64_MIN, INT64_MAX)

__CKD_SUB_SIGNED(i8, int8_t, INT8_MIN, INT8_MAX)
__CKD_SUB_SIGNED(i16, int16_t, INT16_MIN, INT16_MAX)
__CKD_SUB_SIGNED(i32, int32_t, INT32_MIN, INT32_MAX)
__CKD_SUB_SIGNED(i64, int64_t, INT64_MIN, INT64_MAX)

  #undef __CKD_ADD_UNSIGNED
  #undef __CKD_SUB_UNSIGNED
  #undef __CKD_ADD_SIGNED
  #undef __CKD_SUB_SIGNED

  #define __CKD_DISPATCH(result, a, b, add_fn, sub_fn)                                                                 \
    (_Generic((result),                                                                                                \
         uint8_t*: add_fn##_u8,                                                                                        \
         const uint8_t*: add_fn##_u8,                                                                                  \
         unsigned char*: add_fn##_u8,                                                                                  \
         uint16_t*: add_fn##_u16,                                                                                      \
         uint32_t*: add_fn##_u32,                                                                                      \
         uint64_t*: add_fn##_u64,                                                                                      \
         size_t*: add_fn##_usz,                                                                                        \
         int8_t*: add_fn##_i8,                                                                                         \
         int16_t*: add_fn##_i16,                                                                                       \
         int32_t*: add_fn##_i32,                                                                                       \
         int64_t*: add_fn##_i64)(result, a, b))

  #define ckd_add(result, a, b) __CKD_DISPATCH(result, a, b, ckd_add, ckd_sub)
  #define ckd_sub(result, a, b) __CKD_DISPATCH(result, a, b, ckd_add, ckd_sub)

#endif /* __STDC_VERSION_STDCKDINT_H__ */
