#ifndef CTRL_H
#define CTRL_H

#include <stdint.h>
#include <stdbool.h>

/* 5-bit binary display helpers for memory locations 0..31. */
#define BIN5_FMT "0b%c%c%c%c%c"
#define BIN5_ARG(v) \
    (((v) & 0x10U) ? '1' : '0'), \
    (((v) & 0x08U) ? '1' : '0'), \
    (((v) & 0x04U) ? '1' : '0'), \
    (((v) & 0x02U) ? '1' : '0'), \
    (((v) & 0x01U) ? '1' : '0')

/* ── Configuration ─────────────────────────────── */
#define MEM_SIZE        16      /* number of instruction slots  */
#define MAX_EVENTS      8       /* max registered event types   */
#define PC_BITS         8       /* shift-register width (bits)  */

/* ── Event return codes ────────────────────────── */
typedef enum {
    EVT_RESUME = 0,   /* return control to saved PC */
    EVT_BREAK  = 1    /* terminate program          */
} evt_result_t;

/* ── Instruction (one "memory cell") ──────────── */
typedef struct {
    const char *label;
    void (*execute)(uint8_t pc, const char *label);
} instr_t;

/* ── Event descriptor ─────────────────────────── */
typedef struct event_t {
    const char     *name;
    uint8_t         location;
    evt_result_t  (*handler)(const struct event_t *evt);
} event_t;

/* ── Control block (the "shift register") ────── */
typedef struct {
    uint8_t   pc;       /* current position in memory  */
    uint8_t   saved_pc; /* PC snapshot on event entry  */
    uint8_t   interrupt_location;
    const char *interrupt_name;
    bool      interrupt_active;
    bool      interrupt_resume_guard;
    bool      running;  /* START/STOP bit              */
    bool      active;   /* false = terminated          */
} ctrl_t;

/* ── Public API ───────────────────────────────── */
void ctrl_init   (ctrl_t *c);
void ctrl_start  (ctrl_t *c);
void ctrl_stop   (ctrl_t *c);
bool ctrl_step   (ctrl_t *c, const instr_t *mem, size_t mem_len);
bool ctrl_fire   (ctrl_t *c, const event_t *evt);
bool ctrl_clear_interrupt(ctrl_t *c);

#endif /* CTRL_H */
