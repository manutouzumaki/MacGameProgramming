//
//  common.h
//  
//
//  Created by Manuel Cabrerizo on 13/02/2024.
//

#ifndef common_h
#define common_h

#include <stdint.h>

typedef int64_t int64;
typedef int32_t int32;
typedef int16_t int16;
typedef int8_t  int8;

typedef uint64_t uint64;
typedef uint32_t uint32;
typedef uint16_t uint16;
typedef uint8_t  uint8;

typedef float  float32;
typedef double float64;

typedef uint32_t bool32;

#define ARRAY_LENGTH(array) (sizeof(array)/sizeof(array[0]))

#ifdef HANDMADE_DEBUG
#define ASSERT(condition) if(!(condition)) { *(volatile int *)0 = 0;} 
#else
#define ASSERT(condition) 
#endif

#define Max(a, b) ((a) >= (b) ? (a) : (b))
#define Min(a, b) ((a) <= (b) ? (a) : (b))

#define KB(value) ((value)*1024LL)
#define MB(value) (KB(value)*1024LL)
#define GB(value) (MB(value)*1024LL)
#define TB(value) (GB(value)*1024LL)

#define IS_POWER_OF_TWO(expr) (((expr) & (expr - 1)) == 0)

#endif /* common_h */
