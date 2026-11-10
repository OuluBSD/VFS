# Vfs Storage Schema (Draft)

## Goals
- Persist per-file Vfs fragments without physical merging.
- Preserve provenance (package/file hashes) for overlay reconstruction.
- Allow streaming/partial loading for large trees and incremental updates.
- Maintain backward compatibility with existing IDE dumps.

## Fragment Format
```json
{
  "version": 1,
  "pkg_hash": 123456789,
  "file_hash": 987654321,
  "root": { "...": "Node" }
}
```

`root` is the logical fragment root, serialized recursively.

### Node Object
- `id` (string): identifier within parent scope.
- `type_hash` (uint64): identifies `VfsValueExt` type or 0 for plain Value/Ast.
- `value` (object):
  - `kind`: `plain` | `ast` | `ext`.
  - `payload`: storage-specific data (mirrors Upp::Value/AstValue/jsonized ext).
- `ext` (object, optional): additional extension payload keyed by `type_hash` when `kind == "ext"`.
- `flags` (uint32): bits for disabled/temp/etc.
- `children` (array<Node>): child nodes.
- `sources` (array<SourceRef>): optional provenance overrides per node.

### SourceRef
- `pkg_hash` (uint64)
- `file_hash` (uint64)
- `local_path` (string)
- `priority` (int)
- `flags` (uint32)

## Overlay Index
To avoid merging/unmerging, persist a lightweight index mapping logical paths to contributing fragments:
```json
{
  "version": 1,
  "nodes": [
    {
      "path": "Ecs|Player",
      "sources": [ { "pkg_hash": ..., "file_hash": ... } ],
      "metadata": {
        "router": { "...": "RouterSchema" }
      }
    }
  ]
}
```
The index can live alongside fragment files or be recomputed on load. For now we expect IDE to recompute if missing.

## Binary Envelope (Current Implementation)
- Helper APIs `VfsSaveFragmentBinary`/`VfsLoadFragmentBinary` and `VfsSaveOverlayIndexBinary`/`VfsLoadOverlayIndexBinary` wrap the JSON schema inside a small header:
  - `magic`: four-byte tag differentiating fragments (`VFSF`) vs overlay indexes (`VFOI`).
  - `version`: matches the JSON schema version (currently `1`; `0` is treated as “unspecified” and accepted).
  - `payload_size`: length of the embedded JSON buffer (guarded to 32 MiB for sanity).
  - `payload`: UTF‑8 JSON identical to what the text helpers emit.
- This keeps binary and JSON artifacts in sync immediately while we flesh out the chunked/binary writer described below.

## Chunked Overlay Writer
- `.overlay.vfsch` files persist overlay indexes incrementally: the IDE builder calls `BuildRouterOverlayIndex(fragment, sink)` with a multiplexer that feeds both the in-memory index and an `OverlayIndexChunkWriter`.
- File layout:
  - Header: `magic = VFOC`, `version = 1`, reserved 16-bit field.
  - Repeated chunks: `{ chunk_type = 'NODE', size, payload }`. Payload encodes the logical path, an array of `SourceRef` entries, and the metadata JSON blob (serialized as a compact string). Unknown chunk types are skipped by size so future extensions remain forward compatible.
 - Loader: `VfsLoadOverlayIndexChunked` rebuilds a `VfsOverlayIndex` directly from the chunk file and is now the first artifact IDE attempts to read/caches; binary/json fallbacks remain for older snapshots.
- Benefit: overlay metadata (router descriptors today) streams to disk without rewalking the fragment tree. Builders produce JSON, `.vfsbin`, and `.vfsch` in a single traversal so tooling can diff/check them independently.

## Backward Compatibility
Implement `VfsLoadLegacy` to parse previous binary/JSON dumps, transform them into the new fragment structure, and mark nodes as coming from a single-source fragment.

## Next Steps
1. Thread the binary envelope helpers into IDE overlay/fragment save flows so both artifacts ship side by side.
2. Promote the headerized payload into a chunked binary writer once router schema stabilizes (dedicated RouterPorts / RouterConnections chunks).
3. Keep expanding regression coverage (Router console suite today) and document the eventual chunk layout plus versioning strategy.
