# VFS - Virtual File System & Workbench Template

**Repository**: `OuluBSD/VFS`  
**Purpose**: Virtual file system, hierarchical node tree (`VfsValue`), component extensions (`VfsValueExt`), dynamic factory registry (`VfsValueExtFactory`), and reusable tree-and-view workbench GUI template (`VfsTreeApp` / `VfsTreeAppWindow`).

---

## Architecture & Decoupling Contract

1. **Headless Twin Separation (`ai/rules/headless-gui-dual-purpose.md`)**:
   - `uppsrc/Vfs/Core`: Pure headless library. Zero GUI dependencies. Only depends on `BazaarCore` (`Core`, `Core/MetaTraits`, `Core/ProcessUtil`, `Core/DataStructures`, `Core/EcsFoundation`).
   - `uppsrc/Vfs/Ctrl`: GUI controls and workbench window template. Depends on `Vfs/Core` and `CtrlLib`.
   - Umbrella `uppsrc/Vfs`: Re-exports `Vfs/Core` and optionally `Vfs/Ctrl` when `flagGUI` is active.

2. **The Workbench Template (`VfsTreeApp` / `VfsTreeAppWindow`)**:
   - **Left Pane**: `TreeCtrl` presenting the `VfsValue` hierarchy with full context menu support (Add Folder, Add Component, Rename, Duplicate, Remove).
   - **Right Pane**: Dynamic container hosting the active `VfsValueExtCtrl` view for the selected node.
   - **Factory Dispatch**: Dispatches on `node.type_hash` using `VfsValueExtFactory::FindTypeHashFactory` and instantiates views via `new_ctrl_fn()`.
   - **Persistence**: Round-trip JSON workspace serialization (`SaveTree` / `LoadTree`).
   - **Inheritance & Reusability**: Downstream applications (such as `Playground` in `OuluBSD/SuperIntelligence` or `OuluBSD/IDE`) inherit `VfsTreeAppWindow` or embed `VfsTreeApp` to gain interactive tree-driven workbenches out of the box.

---

## Package Map

| Package | Path | Type | Purpose |
|---|---|---|---|
| `Vfs/Core` | `uppsrc/Vfs/Core` | Library | Core data structure (`VfsValue`), extensions, registry, and mount points |
| `Vfs/Ctrl` | `uppsrc/Vfs/Ctrl` | Library (GUI) | `WidgetCtrl`, `VfsValueExtCtrl`, `VfsTreeApp`, `VfsTreeAppWindow` |
| `Vfs/Shell` | `uppsrc/Vfs/Shell` | Library / App | Command-line VFS shell interpreter, syntax parser, and ncurses editor |
| `VfsShell` | `uppsrc/VfsShell` | Shim | Compatibility forwarding package redirecting to `Vfs/Shell` |
| `Vfs` | `uppsrc/Vfs` | Umbrella | Umbrella package for VFS Core + GUI + Shell |
| `tests/VfsTests` | `tests/VfsTests` | Console App | Unit test suite for `Vfs/Core` tree, factory, and JSON serialization |
| `tests/VfsShellTests` | `tests/VfsShellTests` | Console App | Unit test suite for `Vfs/Shell` AST parser, path resolution, and command execution |
| `examples/VfsWorkbenchDemo` | `examples/VfsWorkbenchDemo` | GUI App | Interactive and automated verification demo for `VfsTreeAppWindow` |

---

## Build & Test Instructions

Build via `uppbuilder` (requires sibling `BazaarCore`):

```bash
# Build & run headless unit tests
uppbuilder tests/VfsTests -m CLANG
./bin/VfsTests

# Build & run GUI workbench demo (automated self-test)
uppbuilder examples/VfsWorkbenchDemo -m CLANG
./bin/VfsWorkbenchDemo --test
```
