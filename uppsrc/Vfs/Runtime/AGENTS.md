AGENTS

Scope
- Applies to `uppsrc/Vfs/Runtime`.

Purpose
- Convenience umbrella over runtime-facing Vfs helpers (meta data, dataset wiring, solver base classes, and code visitors).

Guidelines
- Header-only aggregator; avoid adding sources here—contribute to the underlying packages instead.
- Include `Runtime.h` when legacy code expects the old `Vfs2/Vfs.h` surface.

Manifest
- `Runtime.upp` lists `AGENTS.md` and `Runtime.h`.
