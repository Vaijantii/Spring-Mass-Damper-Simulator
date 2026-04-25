# Mass-Spring-Damper Simulator

An interactive graphical tool for building and analysing multi-DOF mass-spring-damper systems. Built with **Qt 6 (Widgets + Charts)** for the GUI and three standalone C++ solvers for the numerical back-end.

---

## Features

- Drag-and-drop canvas to place masses, springs, dampers, walls, and a ground
- Arbitrary force functions `F(t)` defined as JavaScript expressions (e.g. `10*sin(w*t)`)
- **Eigenvalue solver** — natural frequencies and mode shapes via QR iteration
- **FRF viewer** — frequency response |H_ij(ω)| computed by LU decomposition
- **Sensitivity analysis** — ranks every physical parameter (m, k, c) by its influence on H(ω)

---

## Repository layout

```
firstapp/               ← Qt GUI project (open this in Qt Creator)
├── main.cpp
├── mainwindow.cpp / .h
├── simulationscene.cpp / .h
├── massitem.cpp / .h
├── springitem.cpp / .h
├── damperitem.cpp / .h
├── wallitem.cpp / .h
├── systembuilder.h
├── forcedialog.cpp / .h
├── forcefunction.h
└── firstapp.pro        ← Qt project file

solvers/                ← standalone C++ solvers (no Qt dependency)
├── qr.cpp              → compiled as  qr_solver
├── lu.cpp              → compiled as  lu_solver
└── sensitivity.cpp     → compiled as  sensitivity_solver
```

> **Important:** the GUI launches `qr_solver`, `lu_solver`, and `sensitivity_solver` as child processes. All three compiled executables **must sit in the same directory as the main GUI executable** when you run the application.

---

## Prerequisites

| Requirement | Minimum version | Notes |
|---|---|---|
| Qt | 6.2 | Widgets, Charts, QML/JS modules needed |
| Qt Creator | 9.0 (optional) | Easiest way to open `.pro` file |
| CMake **or** qmake | — | Either build system works |
| C++ compiler | C++17 | MSVC 2019+, GCC 10+, or Clang 12+ |

### Windows
Install the [Qt Online Installer](https://www.qt.io/download-qt-installer) and select:
- Qt 6.x → MSVC 2019 64-bit (or MinGW 64-bit)
- Qt Charts component (under the same Qt version)

### Linux (Ubuntu / Debian)
```bash
sudo apt install qt6-base-dev qt6-charts-dev qt6-declarative-dev \
                 build-essential cmake
```

### macOS
```bash
brew install qt
export PATH="/opt/homebrew/opt/qt/bin:$PATH"
```

---

## Building

### Option A — Qt Creator (recommended for beginners)

1. Open Qt Creator.
2. **File → Open File or Project…** → select `firstapp/firstapp.pro`.
3. Configure the kit (Qt 6, your compiler). Click **Configure Project**.
4. Build the three solvers (see Option B or C below) and copy the resulting executables into the Qt build output folder (the folder that contains `firstapp.exe` / `firstapp`).
5. Press **Build** (Ctrl+B), then **Run** (Ctrl+R).

### Option B — qmake + make

```bash
# 1. Build the GUI
cd firstapp
qmake firstapp.pro
make -j$(nproc)          # Linux/macOS
# or: mingw32-make       # Windows with MinGW

# 2. Build the solvers (plain g++ / cl, no Qt needed)
cd ../solvers

g++ -std=c++17 -O2 -o qr_solver        qr.cpp
g++ -std=c++17 -O2 -o lu_solver        lu.cpp
g++ -std=c++17 -O2 -o sensitivity_solver sensitivity.cpp

# 3. Copy solvers next to the GUI binary
cp qr_solver lu_solver sensitivity_solver ../firstapp/build/   # adjust path as needed
```

On **Windows with MSVC** replace `g++` with:
```cmd
cl /std:c++17 /O2 /Fe:qr_solver.exe        qr.cpp
cl /std:c++17 /O2 /Fe:lu_solver.exe        lu.cpp
cl /std:c++17 /O2 /Fe:sensitivity_solver.exe sensitivity.cpp
```

### Option C — CMake

Create a `CMakeLists.txt` at the repo root (or use the one provided if present):

```cmake
cmake_minimum_required(VERSION 3.16)
project(MassSpringSimulator)

set(CMAKE_CXX_STANDARD 17)
find_package(Qt6 REQUIRED COMPONENTS Widgets Charts Qml)

# GUI target
qt_add_executable(firstapp
    firstapp/main.cpp
    firstapp/mainwindow.cpp
    firstapp/simulationscene.cpp
    firstapp/massitem.cpp
    firstapp/springitem.cpp
    firstapp/damperitem.cpp
    firstapp/wallitem.cpp
    firstapp/forcedialog.cpp
)
target_link_libraries(firstapp PRIVATE Qt6::Widgets Qt6::Charts Qt6::Qml)

# Solver targets
add_executable(qr_solver        solvers/qr.cpp)
add_executable(lu_solver        solvers/lu.cpp)
add_executable(sensitivity_solver solvers/sensitivity.cpp)

# Copy solvers to the same output directory as the GUI
set_target_properties(qr_solver lu_solver sensitivity_solver PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)
```

Then:
```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

---

## Running

```bash
./build/firstapp          # Linux / macOS
build\firstapp.exe        # Windows
```

The three solver executables must be in the **same directory** as `firstapp` / `firstapp.exe`. The GUI will show an error dialog if it cannot find them.

---

## Workflow inside the app

1. **Add masses** — click "⬛ Mass", enter mass in kg. Drag to position.
2. **Add walls / ground** — use the wall buttons for fixed boundaries.
3. **Connect springs / dampers** — click "〰 Spring" or "━━ Damper", then click two masses (or a mass and a wall). Enter the stiffness / damping value. If a connection already exists you are offered series or parallel combination.
4. **Add forces** — click "➡ Force", click a mass, define `F(t)` as a math expression.
5. **Export & solve eigenvalues (λ EVP)** — writes `matrix.txt` and `physical_params.txt`, then runs `qr_solver` to produce `natural_frequencies.csv` and `mode_shapes.csv`.
6. **Plot FRF** — runs `lu_solver` to produce `H_output.csv`, then displays |H_ij(ω)|.
7. **Sensitivity analysis** — runs `sensitivity_solver` to produce `sensitivity_output.csv` and `sensitivity_vs_freq.csv`, then shows a ranked bar chart and per-frequency curves.

All output files are written to the **same directory as `matrix.txt`** (the application working directory / build folder).

---

## Generated files

| File | Written by | Contents |
|---|---|---|
| `matrix.txt` | GUI (Export) | M, K, C matrices + force expressions |
| `physical_params.txt` | GUI (Export) | Individual m/k/c values with node connectivity |
| `natural_frequencies.csv` | `qr_solver` | Mode number → ω_n (rad/s) |
| `mode_shapes.csv` | `qr_solver` | Normalised eigenvectors per mode |
| `H_output.csv` | `lu_solver` | ω → |H_ij(ω)| for all i,j |
| `sensitivity_output.csv` | `sensitivity_solver` | Parameter → mean sensitivity, ranked |
| `sensitivity_vs_freq.csv` | `sensitivity_solver` | Sensitivity vs ω curve per parameter |

---

## Troubleshooting

**"Could not start qr_solver.exe / lu_solver.exe / sensitivity_solver.exe"**  
The solver executable is not in the same folder as the GUI binary. Build all three solvers and copy them there.

**Blank FRF plot / no curves shown**  
Run "λ EVP" first to generate `natural_frequencies.csv`. The FRF sweep range is derived from the natural frequencies.

**Spring and damper overlap on canvas**  
This is a known visual artefact if only one of the pair is present between two nodes. Adding the second component (spring or damper) triggers automatic offset separation.

**Qt Charts not found during build**  
Install the Qt Charts module for your Qt version (it is a separate component in the Qt installer and in some Linux package managers).

---

## License

MIT — see `LICENSE` file.
