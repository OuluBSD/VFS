AGENTS

Scope
- Applies to `uppsrc/Vfs/Core` package.

Purpose
- Core Vfs types and services shared by the Vfs meta-language tools and by every
  consumer that browses or serves a virtual filesystem: `VfsPath`, `VFS`,
  `MountManager`, `VfsItem`, `VfsValue`/`AstValue` aliases.

MountManager
- `MountManager::System()` is a process-wide singleton serving several kinds of
  consumer: a plain file manager, a standalone virtual filesystem, an attachable
  asset debugger, or a GUI widget hierarchy debugger.
- A mount is therefore **not** assumed to be a browsable filesystem. Every VFS
  carries `MountCapabilities` (role + capability flags); callers select mounts by
  role via `MountManager::Enumerate(role)` and ask `IsBrowsable(path)` before
  treating a path as filesystem content.
- **Resolution is longest-prefix.** `FindDeepest()` returns the deepest mount whose
  path is a prefix of the queried path, and `mounts` is kept sorted deepest-first on
  every mount. Mounting below an existing mount is an overlay and is allowed by
  default; `MOUNT_REJECT_SHADOW` refuses a mount that would hide an existing one.
- **Ownership is explicit, never inferred.** `Core/Ptr.h` defines `Ptr<T>` as a
  weak reference: it shares the object's reference block but *never deletes* the
  pointee (see `PteBase::PtrRelease`). So `Mount(path, VFS*)` **borrows** and the
  caller must keep the VFS alive; the owning half is `MountPoint::owned_vfs`
  (`One<VFS>`), which only `MountOwned(path, One<VFS>)` fills. Handing a raw
  `new VFS` to `Mount()` leaks it silently -- that regression is covered by
  VfsMountTest ("Mount borrows a caller-owned VFS").
- `Unmount(path)` detaches a mount and releases its owned VFS.
- Beware `VfsPath::IsLeft(a)`: it reads **"a is a prefix of me"**, so it looks
  inverted at the call site. `p.IsLeft(mp.path)` is the "mp.path is a prefix of p"
  test that resolution uses. Getting this backwards silently breaks overlays and
  shadow detection.
- `MountManager` also exposes path-level mutators -- `CreateDir`, `CreateFile`,
  `Remove`, `Rename`, `Copy`, `Move` -- so callers never resolve a mount by hand to
  change something. Each resolves the deepest owning mount and checks the matching
  capability flag. `VfsPath` has **no converting constructor**: build one with
  `p.Set(str)`, or use the `VP()` helper in `uppsrc/VfsMountTest/MountTest.h`.

## Mutation, and who is allowed to ask for it

- `VFS` gained `CreateDir` / `CreateFile` / `Remove(recursive)` / `Rename` /
  `Transfer(from, to, move)`. **Every default implementation refuses** and fills
  `last_error`, so read-only VFSes (`ValueFS`, and the test's `FakeFS`) need no
  changes and a VFS cannot appear writable without opting in. Only `SystemFS`
  overrides them.
- `MountCapabilities::MUTABLE` means "writable", **not "writable anything"**: the
  individual operation is gated on `CREATE_DIR` / `CREATE_FILE` as well. Both must
  be set for a mutation to be attempted, and a refusal names the missing flag via
  the new `MountCapabilities::FlagName(int)`.
- **The Vfs layer has no opinion about permissions and must not grow one.**
  `ExplorerPolicy` in the Explorer package is the thing that authorizes, and it is
  supposed to run *before* a call reaches here. A mutation that arrives
  unauthorized is a bug in the caller, not a gap in this interface.
- `Rename` / `Copy` / `Move` require both endpoints on the **same** mount and refuse
  across mounts: `VFS` can list a tree but exposes no way to read an entry's bytes,
  so moving data between two filesystems is not expressible at this layer. They say
  so rather than half-copying.
- `Remove(rel, recursive)` refuses a non-empty directory unless `recursive` is
  explicitly set, so a stray flag cannot silently eat a subtree.
- `VFS::GetDiskSpace(rel, total, free)` defaults to refusing, and `SystemFS`
  implements it over `statvfs` / `GetDiskFreeSpaceExW`. It is **not** gated on
  `MUTABLE`: a read-only mount still sits on a device of known size. A filesystem
  reporting a **zero total** (procfs, sysfs, some network mounts) is reported as
  *unknown* rather than "0 B free of 0 B", which would read as a full disk.

### Two traps that produced silent wrong answers

- `RealizePath()` creates only the **parent** of the path it is given
  (`RealizePath -> RealizeDirectory(GetFileFolder(file))`). Passing a directory to
  it creates the parent, reports success, and leaves the directory absent. Use
  `RealizeDirectory()` to create a directory.
- `Transfer(..., move=false)` must not try `FileMove` first. `FileMove` succeeding
  consumes the source, so a "copy" silently becomes a move. `FileMove` is a
  rename(2) underneath and fails across a filesystem boundary, which is why *moves*
  fall back to copy-then-delete.

Conventions
- Follow BLITZ policy: implementation files in dependent packages must include
  `Core.h` first.
- `MountManager` methods that hand out `MountPoint*` are intentionally non-const
  (the containers in this fork are not const-iterable); do not add `const_cast`.

Where to start
- `Mount.h` / `Mount.cpp` — the mount registry and resolution.
- `VFS.h` / `VFS.cpp` — `VFS` interface, `SystemFS`, `ValueFS`, `VfsPath` algebra.

Tests
- `uppsrc/VfsMountTest` covers resolution, overlays, unmount, role enumeration,
  capabilities, mount validation, and Mount/MountOwned ownership (51 checks), plus
  the mutation layer: default-deny on a read-only mount, per-operation capability
  gating, the full create/rename/copy/move/remove cycle against a real temp tree,
  the non-empty-directory refusal, the cross-mount refusal, and disk space including
  the read-only-mount and zero-total cases (82 checks total).
  Run `bin/build -m CLANG VfsMountTest` then `bin/CLANG/VfsMountTest`; it prints one
  line per check and a pass count. The suite **clears its temp tree both before and
  after** the mutation block -- cleaning up only on the way out makes the next run
  report a filesystem bug for a leftover file.
