# Hana-no-saku-ki

Real-time 2D water simulation using cellular automata, written in C++ with SFML, ImGui, and Tracy.

This project is part of a Bachelor thesis. Its goal is to design, implement, measure, and optimize an interactive grid-based fluid simulation without relying on a full physics engine.

## Goal

The project simulates water behavior on a 2D grid. Each grid cell can be empty, contain a wall, or contain a certain amount of water mass.

The global fluid behavior emerges from local rules:

- downward gravity;
- horizontal spreading;
- compression;
- upward pressure;
- interaction with obstacles;
- approximate mass conservation, except in demonstration scenes that intentionally use a source.

Cellular automata were chosen because they provide a simple, visual, and understandable model that can still produce convincing fluid-like behavior in real time.

## Technologies

- C++20
- SFML 3 for the window, events, and rendering
- ImGui / ImGui-SFML for the debug interface
- Tracy for profiling
- Visual Studio / MSBuild
- vcpkg for dependencies

## Architecture

The project is split into several modules:

- `test 1.cpp`: application entry point, window creation, and main loop.
- `Simulation.hpp/.cpp`: grid, cells, fluid rules, and predefined scenes.
- `Rendu.hpp/.cpp`: grid rendering, view handling, and mouse-to-grid mapping.
- `Interface.hpp/.cpp`: ImGui interface, controls, settings, and measurements.
- `Settings.hpp/.cpp`: runtime simulation parameters.
- `Benchmark.hpp/.cpp`: performance measurements, CSV export, and Tracy integration.

The `main` file intentionally stays small. It orchestrates the application loop and delegates simulation, rendering, interface, and benchmarking logic to dedicated modules.

## Features

- Interactive 2D water simulation.
- Add water with the left mouse button.
- Add walls with the right mouse button.
- Erase cells with the middle mouse button.
- Adjustable brush size.
- Optional circular brush.
- Pause and step-by-step simulation.
- Optional pressure colors.
- Runtime simulation settings:
  - maximum flow;
  - viscosity;
  - compressibility;
  - alternating update direction;
  - stable water noise.
- Configurable grid size.
- Resizable window with correct grid scaling.
- Correct mouse alignment after window resizing.
- Adjustable FPS limiter.
- Benchmark scene.
- Pressure demonstration scene with two connected reservoirs.
- Automatic 10-second benchmark run.
- Benchmark results exported to timestamped folders.
- Tracy instrumentation.

## Simulation Model

The grid is stored as a contiguous 1D array:

```cpp
std::vector<Cell> grid;
```

2D coordinates are converted to a 1D index:

```cpp
index = y * hauteur + x;
```

Each cell stores a type and a water mass:

```cpp
struct Cell
{
    float mass;
    CellType type;
};
```

Available cell types:

- `EMPTY`
- `WATER`
- `WALL`

Each frame, the simulation applies local flow rules to all water cells. A double-buffering approach is used: the current state is read from `grid`, the next state is written to `nextGrid`, and both buffers are swapped at the end of the update.

## Rendering

Rendering uses an `sf::VertexArray` instead of drawing one SFML rectangle per cell. This greatly reduces the number of draw calls.

Each visible cell is converted into two triangles, and the whole grid is rendered with a single draw call:

```cpp
window.draw(vertices);
```

The SFML view is recalculated when the window size changes so that the grid keeps its aspect ratio. Mouse input uses `mapPixelToCoords`, which keeps cell selection accurate after resizing.

## Benchmarking

The `Run 10s benchmark` button starts an automatic 10-second benchmark. Results are stored in:

```text
benchmarks/YYYY-MM-DD_HH-MM-SS/
```

The benchmark can generate:

- a summary CSV file;
- a frame-by-frame CSV file;
- an information file;
- a Tracy log;
- a Tracy capture if `tracy-capture.exe` successfully connects.

Main measured values:

- FPS;
- total frame time;
- simulation time;
- rendering time;
- interface time;
- number of water cells;
- number of wall cells;
- total water mass.

## Implemented Optimizations

Three main optimizations have already been implemented and compared through benchmark runs:

1. Reusing a persistent double buffer instead of recreating a temporary grid every frame.
2. Replacing `std::vector<std::vector<Cell>>` with a contiguous 1D array.
3. Replacing per-cell draw calls with batched rendering through `sf::VertexArray`.

These optimizations target:

- memory allocations;
- cache locality and CPU memory access;
- rendering overhead and draw call count.

## Predefined Scenes

### Benchmark scene

A stable test scene used to compare performance between different code versions. It contains water, walls, and obstacles, providing a repeatable workload for benchmark runs.

### Pressure demo

A demonstration scene with two same-sized reservoirs connected at the bottom. A temporary water source fills the left reservoir for a duration based on the grid size. The source then stops automatically, allowing the simulation to show pressure and water-level balancing between both reservoirs.

## Current State

The project currently builds in `Debug|x64`. The latest checks produced `0 errors`; only external SFML warnings remain.

The next recommended step is to continue optimization incrementally, keeping a benchmark result before and after each change.
