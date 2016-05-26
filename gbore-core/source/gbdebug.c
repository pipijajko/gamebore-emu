#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "gamebore.h"

#ifdef GBDEBUG

#define GBDBG_MAX_LEN 1024


void    gbdbg_build_decoder_table(struct gb_instruction_s slots[], const size_t slots_n);
errno_t gbdbg_build_text_lookup(const char* filename, char* arr[], const size_t arr_size);
void    gbdbg_free_text_lookup(char* arr[],const size_t arr_size);


typedef struct gbdbg_context_s {
    struct gb_machine_s* instance;
    
    bool              print_enabled;
    bool              log_enabled;
    FILE*             fp_out;
    struct            gb_instruction_s op_decoders[GB_OPCODES_N];
    char*             op_txtlookup[GB_OPCODES_N];
}gbdbg_context_s;


errno_t
gbdbg_initialize(gb_machine_s* instance, gb_debugdata_h* handle)
{
    errno_t err    = 0;
    const char* output_fn = "dbg_output.txt"; //sucks for tests, one log that gets overwritten on every test
    gbdbg_context_s* dbg = calloc(1, sizeof(gbdbg_context_s));

    if (!handle || !dbg) { goto cleanup_generic; }
    //
    // for now, set all to defaults - add config params later
    //
    dbg->print_enabled = true;
    err = fopen_s(&dbg->fp_out, output_fn, "w");
    if (err) goto cleanup_generic;
    //
    // Build static function table for decoders
    //
    gbdbg_build_decoder_table(dbg->op_decoders, GB_OPCODES_N);
    //
    // Read decoded opcodes from file for debug prints
    //
    err = gbdbg_build_text_lookup("opcodes.txt", dbg->op_txtlookup, GB_OPCODES_N);
    StopIf(err != 0, goto cleanup_file;, "Can't find opcodes.txt!");
    dbg->instance = instance;

cleanup_file:
    if (err && dbg->fp_out) fclose(dbg->fp_out);

cleanup_generic:
    if (err) { free(dbg); dbg = NULL; }
    if (handle) handle->unused = (void*)dbg;
    return err;
}


void
gbdbg_destroy(gb_debugdata_h  handle)
{
    if (!handle.unused) return;
    struct gbdbg_context_s* dbg = (struct gbdbg_context_s*)handle.unused;

    gbdbg_free_text_lookup(dbg->op_txtlookup, GB_OPCODES_N);
    if (dbg->fp_out) fclose(dbg->fp_out);
    free(dbg);

}

int gdbg_trace(gb_debugdata_h handle, char *fmtstr, ...) 
{
    assert(handle.unused);
    struct gbdbg_context_s* dbg = (struct gbdbg_context_s*)handle.unused;
    va_list argp;
    va_start(argp, fmtstr);

    if (dbg->print_enabled) {
        vprintf_s(fmtstr, argp);
    }
    if (dbg->fp_out && dbg->log_enabled) {
        vfprintf_s(dbg->fp_out, fmtstr, argp);
        fflush(dbg->fp_out);
    }
    va_end(argp);
    return 0;
}


errno_t
gbdbg_build_text_lookup(const char* filename, char* arr[], const size_t arr_size) 
{
    FILE* fp = NULL;
    errno_t e = 0;
    char linebuf[1024];
    memset(arr, sizeof(char*), arr_size);

    e = fopen_s(&fp, filename, "r");
    if (e || !fp) goto cleanup_file;

    for (size_t i = 0; i < arr_size; i++) { //for each array entry read a line

        if (fgets(linebuf, 1024, fp)) {

            size_t slen = strlen(linebuf);
            assert(slen);
            char*  sdbuf = calloc(slen, sizeof(char));
            if (!sdbuf) goto cleanup_array;

            e = memcpy_s(sdbuf, slen - 1, linebuf, slen - 1);

            if (e) {
                free(sdbuf);
                goto cleanup_array;
            }
            arr[i] = sdbuf;
            linebuf[0] = '\0';

        }
        else {
            //no more lines
            assert(0);
            break;
        }


    }

cleanup_array:
    if (e) gbdbg_free_text_lookup(arr, arr_size);

cleanup_file:
    if (fp) fclose(fp);
    return e;
}


void
gbdbg_free_text_lookup(char*  arr[], const size_t arr_size) 
{
    if (arr)
        for (size_t i = 0; i < arr_size && arr[i]; i++) {
            free(arr[i]);
            arr[i] = NULL;
        }
}



typedef  byte_t(*gbdbg_handler_pfn)(uint8_t, uint16_t, void*); //extended opcode handler

void
gbdbg_CPU_trace(gb_debugdata_h handle, gb_opcode_t op, gb_dword_t d16,gb_dword_t progcount)
{
    assert(handle.unused);
    __declspec (thread) static char linebuf[GBDBG_MAX_LEN]; //not thread-safe
    gb_bytebuf_t bufferdata;
    int offset0;
    struct gbdbg_context_s* dbg = (struct gbdbg_context_s*)handle.unused;
    if (!dbg->print_enabled && !dbg->log_enabled) return;
    //
    // Decode operation - use text lookup "opcodes.txt"
    //
    if (dbg->op_txtlookup[op]) {
        
        offset0 = sprintf_s(linebuf, GBDBG_MAX_LEN, "[0x%x] %s\t", progcount, dbg->op_txtlookup[op]);
    }
    else {
        offset0 = sprintf_s(linebuf, GBDBG_MAX_LEN, "0x%x :", progcount);
    }

    if (offset0 < 0) { printf("Fatal Handler Error!"); exit(1); }

    //
    // Decode operation - use internals built via parsing of "opcodes.inc"
    //
    bufferdata.buffer = (byte_t*)&linebuf[offset0];
    bufferdata.size   = GBDBG_MAX_LEN - offset0;
    if (dbg->op_decoders[op].handler) {
        ((gbdbg_handler_pfn)dbg->op_decoders[op].handler)(op, d16, &bufferdata);
    }

    if (dbg->print_enabled) {
        printf("%s", linebuf);
    }
    if (dbg->fp_out && dbg->log_enabled) {
        fprintf_s(dbg->fp_out, "%s", linebuf);
        fflush(dbg->fp_out);
    }
}


void
gbdbg_CPU_dumpregs( gb_debugdata_h handle) 
{
    assert(handle.unused);
    __declspec(thread) static char linebuf[GBDBG_MAX_LEN]; //not thread-safe

    int offset0;
    struct gbdbg_context_s* dbg = (struct gbdbg_context_s*)handle.unused;
    struct gb_cpu_registers_s* rr = &dbg->instance->r;
    
    byte_t const IE = *GB_MMU_ACCESS_INTERNAL(GB_IO_IE);
    byte_t const IF = *GB_MMU_ACCESS_INTERNAL(GB_IO_IF);
    byte_t const IME = (byte_t)dbg->instance->interrupts.IME;

    if (!dbg->print_enabled && !dbg->log_enabled) return;
    
    offset0 = sprintf_s(linebuf, GBDBG_MAX_LEN, "AF:%04X, BC:%04X, DE:%04X, HL:%04X, SP:%04X, PC:%04X, IF:%02X, IE:%02X, IME:%u"
                        " FLGS: %c%c%c%c ",
              rr->AF, rr->BC, rr->DE, rr->HL, rr->SP, rr->PC, IF, IE, IME, 
                        
              (rr->F & GB_FLAG_Z) ? ('Z') : ('-'),
              (rr->F & GB_FLAG_N) ? ('N') : ('-'),
              (rr->F & GB_FLAG_H) ? ('H') : ('-'),
              (rr->F & GB_FLAG_C) ? ('C') : ('-')
    );
    if (dbg->print_enabled) {
        fprintf(stdout, "%s", linebuf);
    }
    if (dbg->fp_out && dbg->log_enabled) {
        fprintf_s(dbg->fp_out, "%s", linebuf);
        fflush(dbg->fp_out);
    }
}


void
gbdbg_mute_print(gb_debugdata_h handle, bool set_mute) 
{
    assert(handle.unused);
    struct gbdbg_context_s* dbg = (struct gbdbg_context_s*)handle.unused;
    dbg->print_enabled = !set_mute;
    dbg->log_enabled   = !set_mute;

}



#endif