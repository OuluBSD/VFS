AGENTS

Scope
- Applies to `uppsrc/Vfs/Overlay` package.

Purpose
- Define overlay/virtual merge interfaces (`SourceRef`, `OverlayView`, precedence helpers).
- Remain header-only until overlay logic migrates from legacy code.

Guidelines
- Include `Vfs/Core/Core.h` for shared types.
- Consumers are responsible for providing a `PackagePrecedenceProvider` implementation.

Manifest
- `Overlay.upp` lists `AGENTS.md`, `VfsOverlay.h`, `Precedence.h`.
