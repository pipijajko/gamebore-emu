#pragma once
////////////////////////////////////////////////////////////
// Generic Macros & Small Inlines. Mostly bit-twiddling.
////////////////////////////////////////////////////////////

// Unused Parameter Macro
#define UNUSED(...) (void)(__VA_ARGS__)



// Is N in <A,B) range?
#define IN_RANGE(N, A, B) ( (N) >= (A) && (N) < (B))

// Create Opaque Handle
#define DECLARE_HANDLE_TYPE(name) struct name##__ { void* unused; }; \
                             typedef struct name##__ *name

//Create bit mask of <0,7> bits 
#define BIT_MASK8(a, b)  \
     (((uint8_t) -1 >> (7 - (b))) & ~(((uint8_t)1 << (a)) - 1))

#define BIT_MASK32(a, b)  \
     (((uint32_t) -1 >> (31 - (b))) & ~(((uint32_t)1 << (a)) - 1))

//Fetch bits from given range (aligned to LSB)
#define BIT_RANGE_GET(val, a,b) \
    (((val) & BIT_MASK8((a),(b))) >> (a))

// Set or Clear a given flag in a variable according to condition outcome
#define SETCLR(cond, var, flag) \
    ((cond)?((var) |= (flag)):((var) &= ~(flag)))

#define BIT_SETCLR_IF SETCLR


#define BIT_RLCARRY_8(v, Flags) (((v) << 1) | (((Flags) & GB_FLAG_C) >> (GB_FLAG_C_OFFSET)))
#define BIT_RRCARRY_8(v, Flags) (((v) >> 1) | (((Flags) & GB_FLAG_C) << (7 - GB_FLAG_C_OFFSET)))
#define BIT_RL_8(x) (((x) << 1) | ((x) >> (7)))
#define BIT_RR_8(x) (((x) >> 1) | ((x) << (7)))
#define BIT_GET_N(x,n) (((x) >> (n)) & 0x1)
#define BIT_SET_N(x,n) ((x) | (1<<(n)))
#define BIT_RES_N(x,n) ((x) & ~(1<<(n)))

// StopIf macro - Use for generic (system) error handling,
// For application specific debug and tracing use gdebug.h functions

extern char error_mode;     // Set to "s" to call abort on error
extern FILE *error_log;     /** To where should I write errors? If this is \c NULL, write to \c stderr. */

#define StopIf(assertion, error_action, ...) {                    \
        if (assertion){                                           \
            fprintf(error_log ? error_log : stderr, __VA_ARGS__); \
            fprintf(error_log ? error_log : stderr, "\n");        \
            if (error_mode=='s') abort();                         \
            else                 {error_action;}                  \
        } }



//////////////////////////////////////////////////////
// Byteswapping Functions
/////////////////////////////////////////////////////

__forceinline uint16_t bswap_u16(uint16_t val) { return (val << 8) | (val >> 8); }
__forceinline int16_t  bswap_i16(int16_t  val) { return (val << 8) | ((val >> 8) & 0xFF); }

__forceinline  uint32_t bswap_u32(uint32_t val) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | (val >> 16);
}
__forceinline  int32_t bswap_i32(int32_t  val) {
    val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
    return (val << 16) | ((val >> 16) & 0xFFFF);
}
__forceinline int64_t bswap_i64(int64_t val) {
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
}
__forceinline uint64_t bswap_u64(uint64_t val) {
    val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
    val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
    return (val << 32) | (val >> 32);
}





int asprintf(char **str, char* fmt, ...); //__attribute__((format(printf, 2, 3)));


//free the original ptr to write_to after use
//used for easy string extension
#define rasprintf(write_to,  ...) {           \
    char *src= (write_to); \
    asprintf(&(write_to), __VA_ARGS__);       \
    if(src) free(src);              \
}


