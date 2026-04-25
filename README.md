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

All source files are in the root of the repository (flat structure):

```
mass-spring-simulator/
├── .vscode/                  ← VS Code workspace settings (not required by others)
├── CMakeLists.txt            ← CMake build file
├── firstapp_en_IN.ts         ← Qt translation file
├── README.md
│
├── main.cpp
├── mainwindow.cpp / .h / .ui
├── simulationscene.cpp / .h
├── massitem.cpp / .h
├── springitem.cpp / .h
├── damperitem.cpp / .h
├── wallitem.cpp / .h
├── systembuilder.h
├── forcedialog.cpp / .h
├── forcefunction.h
│
├── qr.cpp                    → compiled as  qr_solver
├── lu.cpp                    → compiled as  lu_solver
└── sensitivity.cpp           → compiled as  sensitivity_solver
```

> **Important:** the GUI launches `qr_solver`, `lu_solver`, and `sensitivity_solver` as child processes. All three compiled executables **must sit in the same directory as the main GUI executable** when you run the application.

---

## Prerequisites

| Requirement  | Minimum version | Notes                                             |
| ------------ | --------------- | ------------------------------------------------- |
| Qt           | 6.2             | Widgets, Charts, QML/JS modules needed            |
| CMake        | 3.16            | Used as the build system                          |
| C++ compiler | C++17           | MSVC 2019+, GCC 10+, or Clang 12+                 |
| VS Code      | any             | With the **CMake Tools** and **C/C++** extensions |

### Windows

Install the [Qt Online Installer](https://www.qt.io/download-qt-installer) and select:

- Qt 6.x → MSVC 2019 64-bit (or MinGW 64-bit)
- Qt Charts component (listed under the same Qt version)
- CMake (can also install separately from **cmake.org**)

### Linux (Ubuntu / Debian)

```bash
sudo apt install qt6-base-dev qt6-charts-dev qt6-declarative-dev \
                 build-essential cmake
```

### macOS

```bash
brew install qt cmake
export PATH="/opt/homebrew/opt/qt/bin:$PATH"
```

---

## Building

### Option A — VS Code + CMake Tools extension (recommended)

1. Install the **CMake Tools** extension and **C/C++** extension in VS Code.
2. Open the project folder: **File → Open Folder** → select the repo root.
3. Press **Ctrl+Shift+P** → type `CMake: Configure` → press Enter.
4. Select your compiler kit (MSVC, GCC, or Clang) when prompted.
5. Press **Ctrl+Shift+P** → `CMake: Build` → Enter.

The GUI executable and all three solver executables will be placed in the `build/` folder automatically (the `CMakeLists.txt` puts them all in the same output directory).

### Option B — Terminal / command line

```bash
# From the repo root
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
```

On **Windows with MSVC**, open a **Developer Command Prompt** before running the above, or use:

```cmd
cmake -B build -G "Visual Studio 17 2022" -A x64
cmake --build build --config Release --parallel
```

---

## Running

```bash
./build/firstapp               # Linux / macOS
build\Release\firstapp.exe     # Windows (MSVC)
build\firstapp.exe             # Windows (MinGW)
```

The three solver executables (`qr_solver`, `lu_solver`, `sensitivity_solver`) are built into the same `build/` folder as the GUI, so they will be found automatically.

---

## Workflow inside the app

1. **Add masses** — click "⬛ Mass", enter mass in kg. Drag to position.
2. **Add walls / ground** — use the wall buttons for fixed boundaries.
3. **Connect springs / dampers** — click "〰 Spring" or "━━ Damper", then click two masses (or a mass and a wall). Enter the stiffness / damping value. If a connection already exists you are offered series or parallel combination.
4. **Add forces** — click "➡ Force", click a mass, define `F(t)` as a math expression.
5. **Export & solve eigenvalues (λ EVP)** — writes `matrix.txt` and `physical_params.txt`, then runs `qr_solver` to produce `natural_frequencies.csv` and `mode_shapes.csv`.
6. **Plot FRF** — runs `lu_solver` to produce `H_output.csv`, then displays |H_ij(ω)|.
7. **Sensitivity analysis** — runs `sensitivity_solver` to produce `sensitivity_output.csv` and `sensitivity_vs_freq.csv`, then shows a ranked bar chart and per-frequency curves.

All output files are written to the **same directory as the GUI executable** (i.e. the `build/` folder).

---

## Generated files

| File                      | Written by           | Contents                                       |
| ------------------------- | -------------------- | ---------------------------------------------- |
| `matrix.txt`              | GUI (Export)         | M, K, C matrices + force expressions           |
| `physical_params.txt`     | GUI (Export)         | Individual m/k/c values with node connectivity |
| `natural_frequencies.csv` | `qr_solver`          | Mode number → ω_n (rad/s)                      |
| `mode_shapes.csv`         | `qr_solver`          | Normalised eigenvectors per mode               |
| `H_output.csv`            | `lu_solver`          | ω → \|H_ij(ω)\| for all i,j                    |
| `sensitivity_output.csv`  | `sensitivity_solver` | Parameter → mean sensitivity, ranked           |
| `sensitivity_vs_freq.csv` | `sensitivity_solver` | Sensitivity vs ω curve per parameter           |

---

## .gitignore recommendations

Make sure your `.gitignore` includes at least:

```
build/
.vscode/
*.user
moc_*
ui_*
qrc_*
*.o
*.obj
*.exe
CMakeCache.txt
CMakeFiles/
```

This keeps compiled binaries and personal editor settings out of the repository.

---

## Troubleshooting

**"Could not start qr_solver.exe / lu_solver.exe / sensitivity_solver.exe"**  
The solver executable is not in the same folder as the GUI binary. Rebuild with CMake — it places all executables in the same output directory automatically.

**Blank FRF plot / no curves shown**  
Run "λ EVP" first to generate `natural_frequencies.csv`. The FRF frequency sweep range is derived from the natural frequencies.

**Spring and damper overlap on canvas**  
This can happen if only one of the pair exists between two nodes. Adding the second component (spring or damper) triggers automatic lateral offset separation.

**Qt Charts not found during CMake configure**  
Install the Qt Charts module for your Qt version — it is a separate component in the Qt Online Installer and in some Linux package managers (`qt6-charts-dev`).

**CMake cannot find Qt6**  
Set `CMAKE_PREFIX_PATH` to your Qt installation directory:

```bash
# Windows example
cmake -B build -DCMAKE_PREFIX_PATH="C:/Qt/6.x.x/msvc2019_64"

# macOS example
cmake -B build -DCMAKE_PREFIX_PATH="/opt/homebrew/opt/qt"
```

---
