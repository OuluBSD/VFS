AGENTS

Scope
- Applies to `uppsrc/Vfs/Meta`.

Purpose
- Shared metadata helpers used by IDE/AI layers: package-file identifiers, meta file naming, and CodeArgs serialization helpers.
- Provides AstValue registration hooks (`MetaInit.icpp`).

Guidelines
- Keep headers light; avoid pulling heavy IDE dependencies. Prefer forward declarations when possible.
- Implementation files must include `Meta.h` first to satisfy BLITZ.

Manifest
- `Meta.upp` should list `AGENTS.md` first, followed by `Meta.h` for quick discovery.
