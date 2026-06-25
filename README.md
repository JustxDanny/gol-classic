# gol-classic — Conway's Game of Life (heat-map edition)

![ci](https://github.com/JustxDanny/gol-classic/actions/workflows/ci.yml/badge.svg)

A terminal Game of Life in C with **ncurses**. This variation keeps two fixed
80×25 boards and colours each living cell by how long it has survived, so colonies
glow like a little heat map: fresh births are white, long-lived cells drift through
cyan, green, yellow and finally red.

```
   *          ****           *
  * *                       * *
  * *                       * *
  ***                       * *
   *                         *      gen:42  pop:118  delay:120ms
   *                         *      A=faster  Z=slower  Space=quit
```

## Rules

Conway's standard **B3/S23**: a living cell with 2–3 living neighbours survives,
a dead cell with exactly 3 living neighbours is born, everything else dies. The
80×25 board is **toroidal** — it wraps around at every edge.

## Build & run

```sh
cd src
make
./game_of_life < ../seeds/glider.txt
```

The starting board is read from **standard input** (one screen character per cell).
Living cells are any of `* O o X x # @ + 1`; everything else is empty. Five-plus
ready-made patterns live in [`seeds/`](seeds).

## Controls

| Key     | Action          |
|---------|-----------------|
| `A`     | speed up        |
| `Z`     | slow down       |
| `Space` | quit            |

Running with no input (a bare terminal) drops in a single glider so there is always
something to watch.

## Patterns (`seeds/`)

| File | Kind | Notes |
|------|------|-------|
| `block.txt` | still life | never changes (population 4) |
| `blinker_toad.txt` | oscillators | period-2 figures |
| `pulsar.txt` | oscillator | period-3, 48 cells |
| `glider.txt` | spaceship | travels diagonally forever |
| `gosper_gun.txt` | gun | fires gliders that wrap and collide |
| `r_pentomino.txt` | methuselah | tiny seed, long chaotic life |
| `still_lifes.txt` | still lifes | block + beehive + loaf |

## How it works (architecture)

- **State:** two `int[25][80]` boards (`cur`, `nxt`). Each living cell stores its
  *age*; the next board is computed from the current one, then copied back.
- **Wrap:** neighbour coordinates use modulo arithmetic `(r + dr + H) % H`, so the
  board behaves like the surface of a torus.
- **Input handover:** the seed is read from stdin; afterwards the program reopens the
  controlling terminal so ncurses can read live keypresses.
- **Tested:** a hidden headless mode (set `GOL_FRAMES`) runs the simulation headless (no screen) so
  CI can valgrind it for leaks and assert oracle-verified populations.

## Tests

GitHub Actions runs, on every push, the exact graders this project is checked
against — **clang-format-18** (Google style), **cppcheck 2.13**, a `-Wall -Wextra
-Werror` build, a **valgrind** leak check, and population invariants. See
[`.github/workflows/ci.yml`](.github/workflows/ci.yml).

## Family

One of three independent takes on the same task:
[gol-classic](https://github.com/JustxDanny/gol-classic) ·
[gol-world](https://github.com/JustxDanny/gol-world) ·
[gol-automaton](https://github.com/JustxDanny/gol-automaton).
