# OuluBSD/VFS

Virtual File System, Hierarchical Data Trees, Component Extensions, and Dynamic Workbench Architecture for Ultimate++.

## Overview

`OuluBSD/VFS` provides:
1. **`Vfs/Core`**: A lightweight, robust headless virtual file system and hierarchical data tree engine:
   - `VfsValue`: Recursive node tree holding metadata, hashes, values, and component extensions.
   - `VfsValueExt`: Polymorphic extension base for attaching custom state and business logic to any node.
   - `VfsValueExtFactory`: Runtime type-hash factory registry for dynamic component and controller instantiation.
   - Full JSON and binary serialization support.
2. **`Vfs/Ctrl`**: A plug-and-play GUI workbench template:
   - `VfsTreeApp`: Horizontal splitter control featuring a `TreeCtrl` left pane and a dynamically instantiated `VfsValueExtCtrl` right pane.
   - `VfsTreeAppWindow`: Ready-to-use `TopWindow` with menu bar, status bar, and workspace persistence.

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
```

### Build Workbench Demo
```bash
uppbuilder examples/VfsWorkbenchDemo -m CLANG
./bin/VfsWorkbenchDemo --test
```

## License

BSD 3-Clause License. See [LICENSE](LICENSE) for details.
