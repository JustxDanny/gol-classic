// The Game of Life - "classic" variation.
// Two fixed 80x25 integer boards, copy-swap each tick, wrap-around (toroidal)
// edges via modulo arithmetic, and an ncurses screen that colours each living
// cell by how long it has stayed alive (a little "heat map").

// Ask the standard library to expose POSIX helpers (isatty and /dev/tty access).
#define _POSIX_C_SOURCE 200809L

#include <ncurses.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Board is 80 columns wide and 25 rows tall, as the task requires.
#define W 80
#define H 25

// Speed is expressed as a delay between frames in milliseconds.
#define START_DELAY 120
#define MIN_DELAY 20
#define MAX_DELAY 1000
#define SPEED_STEP 20

// A living cell stores its "age" (ticks survived), capped so colours settle.
#define AGE_MAX 9

// Decide whether one input character means a living cell.
static int is_alive_char(int ch) {
    return ch == '*' || ch == 'O' || ch == 'o' || ch == 'X' || ch == 'x' || ch == '#' || ch == '@' ||
           ch == '+' || ch == '1';
}

// Reset every cell of a board to dead (0).
static void clear_grid(int grid[H][W]) {
    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            grid[r][c] = 0;
        }
    }
}

// Read the starting board from standard input, one screen character per cell.
static void read_seed(int grid[H][W]) {
    clear_grid(grid);
    // Each line of input fills one row; characters past the width are ignored.
    for (int r = 0; r < H; r++) {
        char line[256];
        if (fgets(line, (int)sizeof(line), stdin) == NULL) {
            break;
        }
        for (int c = 0; c < W && line[c] != '\0' && line[c] != '\n'; c++) {
            if (is_alive_char((unsigned char)line[c])) {
                grid[r][c] = 1;
            }
        }
        // If the row was longer than the buffer, skip the rest so rows stay aligned.
        if (strchr(line, '\n') == NULL) {
            int ch;
            while ((ch = getchar()) != '\n' && ch != EOF) {
            }
        }
    }
}

// When no input is redirected, drop a single glider so the screen is not empty.
static void load_default(int grid[H][W]) {
    clear_grid(grid);
    int r = H / 2;
    int c = W / 2;
    grid[r - 1][c] = 1;
    grid[r][c + 1] = 1;
    grid[r + 1][c - 1] = 1;
    grid[r + 1][c] = 1;
    grid[r + 1][c + 1] = 1;
}

// Count the living cells among the eight wrap-around neighbours of (r, c).
static int count_live_neighbors(const int grid[H][W], int r, int c) {
    int live = 0;
    for (int dr = -1; dr <= 1; dr++) {
        for (int dc = -1; dc <= 1; dc++) {
            if (dr == 0 && dc == 0) {
                continue;
            }
            // Modulo keeps neighbours on the board, so edges wrap like a torus.
            int nr = (r + dr + H) % H;
            int nc = (c + dc + W) % W;
            if (grid[nr][nc] > 0) {
                live++;
            }
        }
    }
    return live;
}

// Build the next generation from the current one using the B3/S23 rules.
static void step(const int cur[H][W], int nxt[H][W]) {
    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            int neighbors = count_live_neighbors(cur, r, c);
            int alive = cur[r][c] > 0;
            if (alive && (neighbors == 2 || neighbors == 3)) {
                // Survivor: grow its age but stop at the cap.
                int age = cur[r][c] + 1;
                nxt[r][c] = age > AGE_MAX ? AGE_MAX : age;
            } else if (!alive && neighbors == 3) {
                nxt[r][c] = 1;  // a new cell is born
            } else {
                nxt[r][c] = 0;  // dies or stays dead
            }
        }
    }
}

// Count how many cells are alive on the whole board.
static int population(const int grid[H][W]) {
    int count = 0;
    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            if (grid[r][c] > 0) {
                count++;
            }
        }
    }
    return count;
}

// Map a cell's age to one of the colour pairs defined below.
static int age_color(int age) {
    if (age <= 1) {
        return 1;
    }
    if (age <= 2) {
        return 2;
    }
    if (age <= 4) {
        return 3;
    }
    if (age <= 7) {
        return 4;
    }
    return 5;
}

// Register the colour pairs used for the age-based "heat map".
static void init_colors(void) {
    start_color();
    init_pair(1, COLOR_WHITE, COLOR_BLACK);
    init_pair(2, COLOR_CYAN, COLOR_BLACK);
    init_pair(3, COLOR_GREEN, COLOR_BLACK);
    init_pair(4, COLOR_YELLOW, COLOR_BLACK);
    init_pair(5, COLOR_RED, COLOR_BLACK);
}

// Draw every cell: a coloured star for life, a blank for empty space.
static void draw_board(const int grid[H][W]) {
    for (int r = 0; r < H; r++) {
        for (int c = 0; c < W; c++) {
            if (grid[r][c] > 0) {
                int pair = age_color(grid[r][c]);
                attron(COLOR_PAIR(pair));
                mvaddch(r, c, '*');
                attroff(COLOR_PAIR(pair));
            } else {
                mvaddch(r, c, ' ');
            }
        }
    }
}

// Show a status line under the board if the terminal has a spare row.
static void draw_hud(int gen, int pop, int delay) {
    if (LINES <= H) {
        return;
    }
    mvprintw(H, 0, "gen:%-6d pop:%-4d delay:%dms   A=faster  Z=slower  Space=quit   ", gen, pop, delay);
}

// React to a key press: adjust the frame delay, or report a request to quit.
static int apply_key(int ch, int *delay) {
    if (ch == 'a' || ch == 'A') {
        *delay -= SPEED_STEP;
        if (*delay < MIN_DELAY) {
            *delay = MIN_DELAY;
        }
    } else if (ch == 'z' || ch == 'Z') {
        *delay += SPEED_STEP;
        if (*delay > MAX_DELAY) {
            *delay = MAX_DELAY;
        }
    } else if (ch == ' ') {
        return 1;
    }
    return 0;
}

// The interactive loop: set up ncurses, then draw/advance until the user quits.
static void run_ncurses(int cur[H][W], int nxt[H][W]) {
    int delay = START_DELAY;
    int gen = 0;
    initscr();
    cbreak();
    noecho();
    curs_set(0);
    keypad(stdscr, TRUE);
    if (has_colors()) {
        init_colors();
    }
    while (1) {
        timeout(delay);  // getch waits at most "delay" ms, so it sets the speed
        if (apply_key(getch(), &delay)) {
            break;
        }
        erase();
        draw_board(cur);
        draw_hud(gen, population(cur), delay);
        refresh();
        step(cur, nxt);
        memcpy(cur, nxt, (size_t)H * W * sizeof(int));
        gen++;
    }
    endwin();
}

// Advance the simulation with no screen output (used by the headless test mode).
static void run_headless(int cur[H][W], int nxt[H][W], int frames) {
    for (int i = 0; i < frames; i++) {
        step(cur, nxt);
        memcpy(cur, nxt, (size_t)H * W * sizeof(int));
    }
    // Print one summary line so the run has visible output and a clean exit.
    printf("frames=%d population=%d\n", frames, population(cur));
}

int main(void) {
    static int cur[H][W];
    static int nxt[H][W];
    // Headless test mode: if GOL_FRAMES is set, run that many ticks with no screen.
    const char *frames_env = getenv("GOL_FRAMES");
    int frames = frames_env != NULL ? atoi(frames_env) : -1;
    // Take the seed from a redirected file, or fall back to a built-in glider.
    if (isatty(STDIN_FILENO)) {
        load_default(cur);
    } else {
        read_seed(cur);
    }
    if (frames >= 0) {
        run_headless(cur, nxt, frames);
        return 0;
    }
    // The seed arrived on stdin; reconnect the keyboard so keys can be read.
    if (freopen("/dev/tty", "r", stdin) == NULL) {
        fprintf(stderr, "cannot open terminal for input\n");
        return 1;
    }
    run_ncurses(cur, nxt);
    return 0;
}
