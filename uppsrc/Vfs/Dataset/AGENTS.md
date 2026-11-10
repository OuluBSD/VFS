AGENTS

Scope
- Applies to `uppsrc/Vfs/Dataset`.

Purpose
- Centralize dataset pointer wiring between `VfsValueExt` instances and domain components.
- Maintains the macro lists consumed by AI/IDE packages when enumerating registrable datasets.

Guidelines
- Keep this package free of GUI dependencies; rely on forward declarations when possible.
- Implementation files must include `Dataset.h` first (BLITZ compliance).
- Avoid adding heavy headers here—downstream packages are expected to include their own definitions.

Manifest
- `Dataset.upp` lists `AGENTS.md` followed by `Dataset.h`.
