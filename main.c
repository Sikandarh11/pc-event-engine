#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include "ctrl.h"

#define MEM_LOCATIONS 21
#define MEMORY_TICK_MS 1000
#define INTERRUPT_REPEAT_MS 3000
#define IDLE_POLL_MS 100

static void exec_location(uint8_t pc, const char *label)
{
    printf("[MEM]  CLK    -> 1s tick | location " BIN5_FMT " | %s\n",
           BIN5_ARG(pc),
           label);
}

static const instr_t program[MEM_LOCATIONS] = {
    { "entity @ location 0",  exec_location },
    { "entity @ location 1",  exec_location },
    { "entity @ location 2",  exec_location },
    { "entity @ location 3",  exec_location },
    { "entity @ location 4",  exec_location },
    { "entity @ location 5",  exec_location },
    { "entity @ location 6",  exec_location },
    { "entity @ location 7",  exec_location },
    { "entity @ location 8",  exec_location },
    { "entity @ location 9",  exec_location },
    { "entity @ location 10", exec_location },
    { "entity @ location 11", exec_location },
    { "entity @ location 12", exec_location },
    { "entity @ location 13", exec_location },
    { "entity @ location 14", exec_location },
    { "entity @ location 15", exec_location },
    { "entity @ location 16", exec_location },
    { "entity @ location 17", exec_location },
    { "entity @ location 18", exec_location },
    { "entity @ location 19", exec_location },
    { "entity @ location 20", exec_location }
};

static evt_result_t handle_interrupt(const event_t *evt)
{
    printf("[INT]  EVENT  -> %s requested\n", evt->name);
    printf("[INT]  FLAG   -> interrupt bit set\n");
    return EVT_RESUME;
}

static const event_t INT_A = { "interrupt A", 0b10010, handle_interrupt }; /* 18 */
static const event_t INT_B = { "interrupt B", 0b00110, handle_interrupt }; /* 6  */
static const event_t INT_C = { "interrupt C", 0b01100, handle_interrupt }; /* 12 */
static const event_t INT_D = { "interrupt D", 0b10100, handle_interrupt }; /* 20 */
static const event_t INT_E = { "interrupt E", 0b00011, handle_interrupt }; /* 3  */
static const event_t INT_F = { "interrupt F", 0b01001, handle_interrupt }; /* 9  */

static void fire_key_interrupt(ctrl_t *c, int key)
{
    switch (key) {
    case 'a': case 'A': ctrl_fire(c, &INT_A); break;
    case 'b': case 'B': ctrl_fire(c, &INT_B); break;
    case 'c': case 'C': ctrl_fire(c, &INT_C); break;
    case 'd': case 'D': ctrl_fire(c, &INT_D); break;
    case 'e': case 'E': ctrl_fire(c, &INT_E); break;
    case 'f': case 'F': ctrl_fire(c, &INT_F); break;
    default:
        printf("[INT]  INFO   -> key '%c' has no mapped interrupt\n", key);
        break;
    }
}

int main(void)
{
    ctrl_t c;
    ctrl_init(&c);
    ctrl_start(&c);

    puts("\n=== Infinite Memory Loop (0..20) ===");
    puts("Clock: each normal location print occurs every 1 second.");
    puts("Interrupt keys: A B C D E F");
    puts("Interrupt flag is set only when an interrupt key is pressed.");
    puts("While the flag is set, the interrupt location message repeats every 3 seconds.");
    puts("Press Q to clear the interrupt bit and resume normal operation from the saved point.");
    puts("Press X to quit.\n");

    ULONGLONG last_step_tick = GetTickCount64();
    ULONGLONG last_interrupt_tick = 0;
    bool interrupt_banner_printed = false;

    while (c.active) {
        if (_kbhit()) {
            int key = _getch();

            if (key == 'q' || key == 'Q') {
                if (c.interrupt_active) {
                    ctrl_clear_interrupt(&c);
                    interrupt_banner_printed = false;
                    last_interrupt_tick = 0;
                } else {
                    puts("[INT]  INFO   -> no interrupt active to clear");
                }
                continue;
            }

            if (key == 'x' || key == 'X') {
                puts("[CTRL] USER   -> quit requested");
                ctrl_stop(&c);
                break;
            }

            fire_key_interrupt(&c, key);
        }

        if (c.interrupt_active) {
            ULONGLONG now = GetTickCount64();

            if (!interrupt_banner_printed) {
                  printf("[INT]  HOLD   -> interrupt active at location " BIN5_FMT "; %s at location " BIN5_FMT "; press Q to clear\n",
                      BIN5_ARG(c.saved_pc),
                       c.interrupt_name ? c.interrupt_name : "interrupt",
                      BIN5_ARG(c.interrupt_location));
                interrupt_banner_printed = true;
                last_interrupt_tick = now;
            } else if (now - last_interrupt_tick >= INTERRUPT_REPEAT_MS) {
                  printf("[INT]  HOLD   -> interrupt active at location " BIN5_FMT "; %s at location " BIN5_FMT "; press Q to clear\n",
                      BIN5_ARG(c.saved_pc),
                       c.interrupt_name ? c.interrupt_name : "interrupt",
                      BIN5_ARG(c.interrupt_location));
                last_interrupt_tick = now;
            }

            Sleep(IDLE_POLL_MS);
            continue;
        }

        ULONGLONG now = GetTickCount64();
        if (now - last_step_tick >= MEMORY_TICK_MS) {
            ctrl_step(&c, program, MEM_LOCATIONS);
            if (c.interrupt_resume_guard && !c.interrupt_active) {
                c.interrupt_resume_guard = false;
            }
            last_step_tick = now;
        }

        Sleep(IDLE_POLL_MS);
    }

    puts("\n=== Program Stopped ===");
    return 0;
}
