## CLI Human Emulation

### Purpose

Drive and verify command-line programs exactly as a human user would. CLI is the easy end of the human-emulation ladder (`ai/rules/human-user-emulation.md`): direct invocation, no daemon needed.

### Argument Conventions

Programs used and built by agents must be drivable by a human without reading the source:

- **Named flags first**: `--some-param <value>` / `-p <value>`; long flag + short alias.
- **Positional arguments are scarce**: one positional only for the single, always-required, self-evident main focus (e.g. `build <package>`); two when both are equally self-evident (e.g. `copy <src> <dst>`); beyond that, switch to flags.
- **`key=value` pairs** are acceptable where Linux software culture uses them for similar programs (e.g. `make VAR=value`).
- Provide `--help` and exit-code discipline: 0 = success, non-zero = failure with a one-line reason on stderr.

### Emulating the Human Workflow

A human at a terminal does more than run one command:

1. **Inspect before acting**: `--list-*` / `--dry-run` modes so the human can see what would happen (see `bin/build.exe --list-mainconfigs`, `--list-methods` as the local pattern).
2. **Run with sensible defaults**: no required flags for the common case; flags exist to deviate.
3. **Read output like a human**: check that the final lines actually indicate success; do not treat exit 0 as proof when the log says otherwise.
4. **Verify state afterwards**: run a status/list command to confirm the effect landed (files built, rows written, service up).

### When CLI Is Not Enough

If the program is GUI-only or the verification requires pixels/layout, escalate to `gui-human-emulation.md`. If the program lacks a CLI but has daemon-able state, prefer adding CLI commands over screen-scraping.

### Local Examples

- `bin/build.exe` (`-M`, `-m`, `-r`, `-j` flags, `--list-*` introspection) — the repo's canonical CLI conventions.
