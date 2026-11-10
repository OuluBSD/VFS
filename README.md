# OuluBSD/VFS

Virtual File System, Hierarchical Data Trees, Component Extensions, and Dynamic Workbench Architecture for Ultimate++.

## Overview

`OuluBSD/VFS` provides:
1. **`Vfs/Core`**: A lightweight, robust headless virtual file system and hierarchical data tree engine:
   - `VfsValue`: Recursive node tree holding metadata, hashes, values, and component extensions.
   - `VfsValueExt`: Polymorphic extension base for attaching custom state and business logic to any node.
   - `VfsValueExtFactory`: Runtime type-hash factory registry for dynamic component and controller instantiation.
   - Full JSON and binary serialization support.
2. **`Vfs/Storage` & `Vfs/Overlay`**:
   - `SourceRef`, `VfsOverlay`, and `OverlayManager` for multi-source layered VFS provenance.
   - Binary and JSON fragment persistence (`VfsSaveFragment`, `VfsLoadFragment`, `VfsSaveFragmentBinary`, `VfsLoadFragmentBinary`).
   - Overlay index serialization (`VfsSaveOverlayIndex`, `VfsLoadOverlayIndex`, `VfsOverlayIndex`).
3. **`Vfs/Analysis` & `Vfs/Solver`**:
   - `CodeVisitor` and `CodeVisitorProfile` AST analysis engine.
   - `SolverBase` constraint solver and `Dataset` type indexing.
4. **`Vfs/Ctrl` & `MetaCtrl`**:
   - `VfsTreeApp`: Horizontal splitter control featuring a `TreeCtrl` left pane and a dynamically instantiated `VfsValueExtCtrl` right pane.
   - `VfsTreeAppWindow`: Ready-to-use `TopWindow` with menu bar, status bar, and workspace persistence.
   - `MetaProcessCtrl` (`ProcessCtrl`): Visual AST inspector and task processing workbench.
5. **`Vfs/Shell`**: Interactive command-line shell and script interpreter for VFS operations:
   - Dual-mount filesystem (`/` for system files, `/vfs` for virtual file system).
   - Core file utilities: `cd`, `pwd`, `ls`, `tree`, `mkdir`, `touch`, `rm`, `mv`, `link`, `export`, `cat`, `grep`, `rg`, `head`, `tail`, `uniq`, `count`, `echo`.
   - csh/tcsh syntax engine: pipelines (`|`), logical operators (`&&`, `||`), sequences (`;`), and I/O redirection.
   - Interactive ncurses-based editor (`NcursesEditor`) when compiled with `flagNCURSES`.
   - Forwarding compatibility shim `VfsShell`.

## Architecture

```
         +-------------------------------------+
         |         VfsTreeAppWindow            |
         |  (TopWindow with Menus & StatusBar) |
         +-------------------------------------+
                           |
         +-------------------------------------+
         |             VfsTreeApp              |
         +------------------+------------------+
         | Left Pane:       | Right Pane:      |
         | TreeCtrl         | Dynamic Active   |
         | (VfsValue Tree)  | VfsValueExtCtrl  |
         +------------------+------------------+
                            |
         +-------------------------------------+
         |        VfsValueExtFactory           |
         | (TypeHash -> Component & View Ctrls)|
         +-------------------------------------+
                            |
         +-------------------------------------+
         |             Vfs/Core                |
         |  (VfsValue, VfsValueExt, Mounts)    |
         +-------------------------------------+
```

## Quick Start

### Build Headless Tests
```bash
uppbuilder tests/VfsTests -m CLANG
./bin/VfsTests

uppbuilder tests/VfsShellTests -m CLANG
./bin/VfsShellTests

uppbuilder tests/VfsExtTests -m CLANG
./bin/VfsExtTests
```

### Build and Run VFS Shell
```bash
uppbuilder Vfs/Shell -m CLANG
./bin/Shell "echo VFS Shell Online"
```

### Build Workbench & AST Demos
```bash
uppbuilder examples/VfsWorkbenchDemo -m CLANG
./bin/VfsWorkbenchDemo --test

uppbuilder examples/MetaCtrlDemo -m CLANG
./bin/MetaCtrlDemo --test
```

## License

BSD 3-Clause License. See [LICENSE](LICENSE) for details.
