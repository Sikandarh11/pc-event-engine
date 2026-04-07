#include <stdio.h>
#include <windows.h>
#include <conio.h>
#include "ctrl.h"

#define MEM_LOCATIONS 11
#define MEMORY_TICK_MS 1000
#define IDLE_POLL_MS 100

typedef enum {
    ITEM_UNKNOWN = 0,
    ITEM_BAG,
    ITEM_NOT_BAG
} item_state_t;

static item_state_t g_item_state = ITEM_UNKNOWN;
static unsigned int g_item_id = 1;
static bool g_interrupt_raised = false;
static bool g_waiting_for_answer = false;
static uint8_t g_question_pc = 0;

static const char *item_state_name(item_state_t s)
{
    switch (s) {
    case ITEM_BAG: return "BAG";
    case ITEM_NOT_BAG: return "NOT BAG";
    default: return "UNKNOWN";
    }
}

static void start_bag_prompt(uint8_t current_pc)
{
    if (g_waiting_for_answer) {
        puts("[INT]  INFO   -> already waiting for answer; press Y or N");
        return;
    }

    g_interrupt_raised = true;
    g_waiting_for_answer = true;
    g_question_pc = current_pc;
    printf("[INT]  INPUT  -> item %u detected near conveyor %u. Press Y (bag) or N (not bag). Conveyor keeps moving.\n",
           g_item_id,
           (unsigned int)g_question_pc);
}

static void resolve_bag_prompt(int key, uint8_t current_pc)
{
    if (!g_waiting_for_answer) {
        puts("[INT]  INFO   -> no pending question; press A first");
        return;
    }

    if (key == 'y' || key == 'Y') {
        g_item_state = ITEM_BAG;
        printf("[INT]  RESUME -> BAG confirmed for item %u at conveyor %u (question raised at conveyor %u)\n",
               g_item_id,
               (unsigned int)current_pc,
               (unsigned int)g_question_pc);
    } else if (key == 'n' || key == 'N') {
        g_item_state = ITEM_NOT_BAG;
        printf("[INT]  BREAK  -> NOT BAG confirmed for item %u at conveyor %u (question raised at conveyor %u)\n",
               g_item_id,
               (unsigned int)current_pc,
               (unsigned int)g_question_pc);
    } else {
        puts("[INT]  INFO   -> use Y or N");
        return;
    }

    g_waiting_for_answer = false;
}

static void exec_location(uint8_t pc, const char *label)
{
    printf("[MEM]  CLK    -> 1s tick | item %u | location " BIN5_FMT " | %s | state=%s\n",
           g_item_id,
           BIN5_ARG(pc),
           label,
           item_state_name(g_item_state));
}

static const instr_t program[MEM_LOCATIONS] = {
    { "entity @ conveyor 0",  exec_location },
    { "entity @ conveyor 1",  exec_location },
    { "entity @ conveyor 2",  exec_location },
    { "entity @ conveyor 3",  exec_location },
    { "entity @ conveyor 4",  exec_location },
    { "entity @ conveyor 5",  exec_location },
    { "entity @ conveyor 6",  exec_location },
    { "entity @ conveyor 7",  exec_location },
    { "entity @ conveyor 8",  exec_location },
    { "entity @ conveyor 9",  exec_location },
    { "entity @ conveyor 10", exec_location }
};

int main(void)
{
    ctrl_t c;
    ctrl_init(&c);
    ctrl_start(&c);

    puts("\n=== Conveyor Loop (0..10) ===");
    puts("Clock: each normal location print occurs every 1 second.");
    puts("Normal execution runs continuously.");
    puts("Press A when an item appears to raise a bag-check question.");
    puts("Then press Y or N anytime while it moves; system does not wait for input.");
    puts("If unresolved, item continues moving and conveyor 10 applies default: NOT BAG.");
    puts("Press X to quit.\n");

    ULONGLONG last_step_tick = GetTickCount64();

    while (c.active) {
        if (_kbhit()) {
            int key = _getch();

            if (key == 'a' || key == 'A') {
                start_bag_prompt(c.pc);
            }

            if (key == 'y' || key == 'Y' || key == 'n' || key == 'N') {
                resolve_bag_prompt(key, c.pc);
            }

            if (key == 'x' || key == 'X') {
                puts("[CTRL] USER   -> quit requested");
                ctrl_stop(&c);
                break;
            }
        }

        ULONGLONG now = GetTickCount64();
        if (now - last_step_tick >= MEMORY_TICK_MS) {
            uint8_t current_pc = c.pc;

            if (current_pc == 0 && !g_interrupt_raised) {
                puts("[INT]  EVENT  -> conveyor 0 interrupt: bag check");
                puts("[INT]  INFO   -> press A to open bag-check; then Y or N while conveyor keeps moving");
            }

            ctrl_step(&c, program, MEM_LOCATIONS);

            if (current_pc == 10) {
                if (g_item_state == ITEM_UNKNOWN) {
                    g_item_state = ITEM_NOT_BAG;
                    puts("[INT]  BREAK  -> reached conveyor 10 with no confirmation: THIS IS NOT A BAG");
                } else {
                    printf("[INT]  INFO   -> final state at conveyor 10: %s\n", item_state_name(g_item_state));
                }

                printf("[CTRL] NEXT   -> item %u complete, loading next item\n", g_item_id);
                g_item_id++;
                g_item_state = ITEM_UNKNOWN;
                g_interrupt_raised = false;
                g_waiting_for_answer = false;
                g_question_pc = 0;
            }

            last_step_tick = now;
        }

        Sleep(IDLE_POLL_MS);
    }

    puts("\n=== Program Stopped ===");
    return 0;
}
