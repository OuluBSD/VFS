AGENTS

Scope
- Applies to `uppsrc/Vfs/Analysis`.

Purpose
- Hosts AST/code visitors used by IDE/AI tooling to traverse `VfsValue` trees.

Guidelines
- Keep dependencies limited to Vfs core packages; avoid IDE/UI coupling.
- Implementation files must include `Analysis.h` first (BLITZ compliance).

Manifest
- `Analysis.upp` lists `AGENTS.md` and `Analysis.h` first.
