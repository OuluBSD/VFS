## GUI Human Emulation (Stateful GUI Application Daemon)

### Purpose

Emulate a human user driving a GUI program. GUI is the hard end of the human-emulation ladder (`ai/rules/human-user-emulation.md`). The technique: run the ordinary GUI program as a **stateful daemon with automation capabilities**, and send human-style commands to it from the CLI.

### Architecture

```
CLI commands ──► command channel ──► GUI daemon (real program, real rendering)
                      │                        │
              shared memory / FIFO /      state + Access dump tree +
              socket                      actually-drawn verification
```

1. **Daemon startup**: launch the normal GUI program with an embedded automation layer (command listener, state serialization). It stays stateful across many commands — like a human session, not one-shot screenshots.
2. **Command channel** (pick per environment):
   - **FIFO pipe files** — simple, no firewall involvement; preferred on Linux.
   - **Shared memory** — fast, no firewall; good for high-frequency state exchange.
   - **Sockets** — flexible, but may trigger firewall prompts; harmful when the user is absent to approve. Avoid when unattended.
3. **Command vocabulary**: human actions as commands (`open <dialog>`, `click <widget-path>`, `type <text>`, `read-state`, `dump-tree`, `screenshot`).

### Decomposed Human Mouse Use (2D/3D Area Tools)

For tool-based editing (canvas, 2D editors, 3D views), decompose human interaction into emulatable parts — all WITHOUT moving the real cursor:

| Part | Emulation |
|------|-----------|
| Tool button press | invoke toolbar action directly through the automation layer |
| Mouse movement (inside/outside window) | inject logical coordinates into the widget's event stream |
| Left-down / up click | synthesized button events at logical coordinates |
| Tool execution at clicked 2D coordinate | dispatch tool logic with the coordinate; project 2D → 3D where the view is 3D |
| Graphical bridge update + change detection | re-render state, diff before/after, report what changed |

Setup is considerably more laborious than a 100% CLI program, but after setup usage is effortless: call the list of functions that emulate human mouse use.

### Verification Against What Is Actually Drawn

Use `uppsrc/CtrlCore/` machinery:

- **Access-type functions** build a hierarchical index of GUI content — the "dump tree" shows what is on screen;
- verify whether elements are **ACTUALLY drawn** and what was ACTUALLY DRAWN. Mandatory when a user reports that something "doesn't show": the dump may prove the widget exists but is clipped, hidden, or empty.

### Escalation Ladder

1. Can the task be done via a CLI command on the same program? → `cli-human-emulation.md`.
2. GUI interaction needed → daemon + command channel (this skill).
3. 2D/3D area-tool editing → daemon foundation + decomposed mouse emulation (this skill).
4. 3D/VR immersive verification → hardest; build on the daemon and extend with camera/projection emulation.

### Related

- `ai/rules/human-user-emulation.md` — the governing rule.
- `toon-file-format.md` — TOON text form of GUI state used by the gui-user-gate action planner.
