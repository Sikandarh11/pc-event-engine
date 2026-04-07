#include <stdio.h>
#include "ctrl.h"

void ctrl_init(ctrl_t *c)
{
    c->pc       = 0;
    c->saved_pc = 0;
    c->interrupt_location = 0;
    c->interrupt_name = NULL;
    c->interrupt_active = false;
    c->interrupt_resume_guard = false;
    c->running  = false;
    c->active   = true;
}

void ctrl_start(ctrl_t *c)
{
    if (!c->active) return;
    printf("[CTRL] START  -> PC=" BIN5_FMT "\n", BIN5_ARG(c->pc));
    c->running = true;
}

void ctrl_stop(ctrl_t *c)
{
    if (!c->active) return;
    printf("[CTRL] STOP   -> PC frozen at " BIN5_FMT "\n", BIN5_ARG(c->pc));
    c->running = false;
}

/*
 * Advance one step.
 * Returns true while the program can continue, false when it ends.
 */
bool ctrl_step(ctrl_t *c, const instr_t *mem, size_t mem_len)
{
    if (!c->active || !c->running || c->interrupt_active)
        return c->active;

    if (mem_len == 0) {
        printf("[CTRL] END    -> empty memory map\n");
        c->active  = false;
        c->running = false;
        return false;
    }

    if (c->pc >= mem_len)
        c->pc = 0;

    if (mem[c->pc].execute == NULL) {
        printf("[CTRL] SKIP   -> PC=" BIN5_FMT " has no executable instruction\n", BIN5_ARG(c->pc));
        c->pc = (uint8_t)((c->pc + 1U) % mem_len);
        return true;
    }

    printf("[CTRL] EXEC   -> PC=" BIN5_FMT "  [%s]\n", BIN5_ARG(c->pc), mem[c->pc].label);
    mem[c->pc].execute(c->pc, mem[c->pc].label);
    c->pc = (uint8_t)((c->pc + 1U) % mem_len);
    return true;
}

/*
 * Fire an event:
 *   • saves PC
 *   • runs handler
 *   • EVT_RESUME  → restore PC, continue
 *   • EVT_BREAK   → terminate
 */
bool ctrl_fire(ctrl_t *c, const event_t *evt)
{
    if (!c->active) return false;

    c->saved_pc = c->pc;
    c->interrupt_location = evt->location;
    c->interrupt_name = evt->name;
    c->interrupt_active = true;
        printf("[INT]  FIRE   -> '%s'  (saved PC=" BIN5_FMT ", location=" BIN5_FMT ")\n",
            evt->name,
            BIN5_ARG(c->saved_pc),
            BIN5_ARG(c->interrupt_location));

    evt_result_t result = evt->handler(evt);

    if (result == EVT_BREAK) {
        printf("[INT]  BREAK  -> program terminated by '%s'\n", evt->name);
        c->active  = false;
        c->running = false;
        c->interrupt_active = false;
        return false;
    }

    printf("[INT]  FLAG   -> interrupt bit set; press Q to clear\n");
    return true;
}

bool ctrl_clear_interrupt(ctrl_t *c)
{
    if (!c->active) return false;

    if (!c->interrupt_active) {
        printf("[INT]  INFO   -> no interrupt pending\n");
        return true;
    }

    c->interrupt_active = false;
    c->interrupt_resume_guard = true;
    c->pc = c->saved_pc;
        printf("[INT]  CLEAR  -> interrupt bit low, PC restored to " BIN5_FMT " (%s @ " BIN5_FMT ")\n",
            BIN5_ARG(c->pc),
           c->interrupt_name ? c->interrupt_name : "interrupt",
            BIN5_ARG(c->interrupt_location));
    return true;
}
