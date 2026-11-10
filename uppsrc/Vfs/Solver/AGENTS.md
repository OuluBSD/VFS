AGENTS

Scope
- Applies to `uppsrc/Vfs/Solver`.

Purpose
- Provides the generic `SolverBase` infrastructure used by AI/Vfs runtimes to model long-running multi-phase tasks.

Guidelines
- Keep implementation free of UI dependencies; rely on callbacks/events already provided by Core.
- Implementation files must include `Solver.h` first (BLITZ compliance).

Manifest
- `Solver.upp` lists `AGENTS.md` followed by `Solver.h`.
