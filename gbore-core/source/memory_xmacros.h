#pragma once


//
// Generate memory-section data and lookups via XMACROS
//
#define XMACRO_GENERATE_VIEW(a, b, NAME) gb_memory_section_s NAME;
#define XMACRO_GENERATE_ID_ENUM(a, b, ENUM) gb_##ENUM,
#define XMACRO_INIT_SECTION_STRUCT(abegin, aend, NAME) {abegin,aend, gb_##NAME},
#define XMACRO_GENERATE_TXTLOOKUP(a, b, STRING) #STRING,

enum gb_mem_section //enum must be created before memory ranges!
{
    FOREACH_MEMORY_SECTION(XMACRO_GENERATE_ID_ENUM)
};

// generate memory section ranges
static const struct gb_memory_range_s gb_memory_ranges[] = {
    FOREACH_MEMORY_SECTION(XMACRO_INIT_SECTION_STRUCT)
};


//typedef union gb_memory_view_u 
//{
//    gb_memory_section_s by_index[MEMORY_SECTION_N];
//
//    struct {
//        FOREACH_MEMORY_SECTION(XMACRO_GENERATE_VIEW)
//    } by_name; 
//
//} gb_memory_view_u;




static const char *gb_mem_section2text[] = {
    FOREACH_MEMORY_SECTION(XMACRO_GENERATE_TXTLOOKUP)
};

#undef XMACRO_GENERATE_ID_ENUM
#undef XMACRO_GENERATE_TXTLOOKUP
#undef XMACRO_INIT_SECTION_STRUCT
#undef XMACRO_GENERATE_VIEW




//verify that MEMORY_SECTION_N is correctly set in "memory.h"!
#define MEMSECTIONS_N (sizeof(gb_memory_ranges)/sizeof(gb_memory_ranges[0]))

static_assert(MEMORY_SECTION_N == MEMSECTIONS_N, "MEMORY_SECTION_N does not match actual number of sections!");

#undef MEMORY_SECITON_N