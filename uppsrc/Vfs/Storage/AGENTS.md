AGENTS

Scope
- Applies to `uppsrc/Vfs/Storage` package.

Purpose
- Declare serialization APIs for Vfs fragments and legacy loaders.
- Header-only until implementations migrate from IDE/Core2.

Guidelines
- Include `Vfs/Core/Core.h` for shared types.
- Keep format documentation alongside implementation when added.

Manifest
- `Storage.upp` lists `AGENTS.md`, `VfsStorage.h` (and future sources when ready).
