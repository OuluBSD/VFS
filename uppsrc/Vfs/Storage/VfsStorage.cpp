#include "VfsStorage.h"

#include <Vfs/Core/VfsValue.h>

NAMESPACE_UPP

void Jsonize(JsonIO& io, SourceRef& ref) {
	int64 pkg_h = (int64)ref.pkg_hash;
	int64 file_h = (int64)ref.file_hash;
	int flags = (int)ref.flags;
	io
		("pkg_hash", pkg_h)
		("file_hash", file_h)
		("local_path", ref.local_path)
		("priority", ref.priority)
		("flags", flags);
	if (io.IsLoading()) {
		ref.pkg_hash = (hash_t)pkg_h;
		ref.file_hash = (hash_t)file_h;
		ref.flags = (dword)flags;
	}
}

namespace {

static constexpr int kVfsFragmentVersion = 1;
static constexpr int kVfsOverlayIndexVersion = 1;

static constexpr dword MakeMagic(char a, char b, char c, char d) {
	return (dword)(byte(a)) |
	       ((dword)(byte(b)) << 8) |
	       ((dword)(byte(c)) << 16) |
	       ((dword)(byte(d)) << 24);
}

static constexpr dword kFragmentBinaryMagic = MakeMagic('V', 'F', 'S', 'F');
static constexpr dword kOverlayBinaryMagic = MakeMagic('V', 'F', 'O', 'I');
static constexpr dword kOverlayChunkMagic = MakeMagic('V', 'F', 'O', 'C');
static constexpr dword kOverlayChunkNodeTag = MakeMagic('N', 'O', 'D', 'E');
static constexpr dword kOverlayChunkUnknownTag = MakeMagic('S', 'K', 'I', 'P');
static constexpr dword kOverlayChunkStringLimit = 1 << 20; // 1 MiB guard per string payload

struct FragmentDocument : Moveable<FragmentDocument> {
	int      version = kVfsFragmentVersion;
	hash_t   pkg_hash = 0;
	hash_t   file_hash = 0;
	VfsValue root;

	void Jsonize(JsonIO& io) {
		io
			("version", version)
			("pkg_hash", pkg_hash)
			("file_hash", file_hash)
			("root", root);
	}
};

struct OverlayNodeDocument : Moveable<OverlayNodeDocument> {
	String            path;
	Vector<SourceRef> sources;
	ValueMap          metadata;

	void Jsonize(JsonIO& io) {
		io
			("path", path)
			("sources", sources)
			("metadata", metadata);
	}
};

struct OverlayIndexDocument : Moveable<OverlayIndexDocument> {
	int                       version = kVfsOverlayIndexVersion;
	Vector<OverlayNodeDocument> nodes;

	void Jsonize(JsonIO& io) {
		io
			("version", version)
			("nodes", nodes);
	}
};

static String NormalizeOverlayPath(const String& path) {
	return path.IsEmpty() ? String("<root>") : path;
}

static String AppendOverlaySegment(const String& base, const String& id) {
	if (id.IsEmpty())
		return base;
	if (base.IsEmpty())
		return id;
	return base + "|" + id;
}

static void CollectRouterOverlayEntries(const VfsValue& node, const String& current_path, OverlayIndexSink& sink) {
	String path = current_path;
	if (!node.id.IsEmpty())
		path = AppendOverlaySegment(current_path, node.id);

	Value router_value;
	if (node.value.Is<ValueMap>()) {
		ValueMap map = node.value;
		router_value = RouterLookupValue(map, "router");
	}
	if (router_value.Is<ValueMap>()) {
		OverlayNodeRecord rec;
		rec.path = NormalizeOverlayPath(path);
		SourceRef ref;
		ref.pkg_hash = node.pkg_hash;
		ref.file_hash = node.file_hash;
		ref.local_path = rec.path;
		rec.sources.Add(ref);
		rec.metadata.Set("router", router_value);
		sink.AddRecord(rec);
	}

	for (const VfsValue& child : node.sub)
		CollectRouterOverlayEntries(child, path, sink);
}

static void PropagateFragmentHashes(VfsValue& node, hash_t pkg_hash, hash_t file_hash) {
	if (pkg_hash)
		node.pkg_hash = pkg_hash;
	if (file_hash)
		node.file_hash = file_hash;
	for (VfsValue& child : node.sub)
		PropagateFragmentHashes(child, pkg_hash, file_hash);
}

static void FillDocumentFromFragment(const VfsValue& fragment, FragmentDocument& doc) {
	doc.pkg_hash = fragment.pkg_hash;
	doc.file_hash = fragment.file_hash;
	doc.root.Assign(nullptr, fragment);
	PropagateFragmentHashes(doc.root, doc.pkg_hash, doc.file_hash);
}

static void FillDocumentFromIndex(const VfsOverlayIndex& index, OverlayIndexDocument& doc) {
	doc.nodes.SetCount(index.nodes.GetCount());
	for (int i = 0; i < index.nodes.GetCount(); i++) {
		const OverlayNodeRecord& src = index.nodes[i];
		OverlayNodeDocument& dst = doc.nodes[i];
		dst.path = src.path;
		dst.sources <<= src.sources;
		dst.metadata = src.metadata;
	}
}

static void AssignIndexFromDocument(const OverlayIndexDocument& doc, VfsOverlayIndex& index) {
	index.nodes.SetCount(doc.nodes.GetCount());
	for (int i = 0; i < doc.nodes.GetCount(); i++) {
		const OverlayNodeDocument& src = doc.nodes[i];
		OverlayNodeRecord& dst = index.nodes[i];
		dst.path = src.path;
		dst.sources <<= src.sources;
		dst.metadata = src.metadata;
	}
}

static bool StoreBinaryEnvelope(const String& path, dword magic, int version, const String& payload) {
	try {
		if (!RealizePath(GetFileFolder(path)))
			return false;
		FileOut out(path);
		if (!out.IsOpen())
			return false;
		out.Put32le(magic);
		out.Put16le((uint16)version);
		out.Put16le(0);
		out.Put32le(payload.GetCount());
		out.Put(~payload, payload.GetCount());
		out.Close();
		return out.IsOK();
	}
	catch (const Exc& e) {
		RLOG("StoreBinaryEnvelope: failed to save '" << path << "': " << e);
	}
	catch (...) {
		RLOG("StoreBinaryEnvelope: unknown error while saving '" << path << "'");
	}
	return false;
}

static bool LoadBinaryEnvelope(const String& path, dword expected_magic, int expected_version, String& out_payload) {
	FileIn in(path);
	if (!in.IsOpen())
		return false;
	dword magic = in.Get32le();
	if (magic != expected_magic) {
		RLOG("LoadBinaryEnvelope: unexpected magic in '" << path << "'");
		return false;
	}
	int version = in.Get16le();
	in.Get16le(); // reserved
	dword payload_size = in.Get32le();
	if (payload_size > 0x2000000) { // 32 MiB guard
		RLOG("LoadBinaryEnvelope: payload too large (" << payload_size << ") in '" << path << "'");
		return false;
	}
	String payload = in.Get((int)payload_size);
	if (payload.GetLength() != (int)payload_size || !in.IsOK()) {
		RLOG("LoadBinaryEnvelope: failed to read payload from '" << path << "'");
		return false;
	}
	if (version != 0 && version != expected_version) {
		RLOG("LoadBinaryEnvelope: unsupported version " << version << " for '" << path << "'");
		return false;
	}
	out_payload = pick(payload);
	return true;
}

} // namespace

static void WriteChunkString(Stream& out, const String& value) {
	uint32 len = (uint32)value.GetCount();
	out.Put32le(len);
	if (len)
		out.Put(~value, len);
}

static void WriteChunkSources(Stream& out, const Vector<SourceRef>& sources) {
	out.Put32le((uint32)sources.GetCount());
	for (const SourceRef& src : sources) {
		out.Put64le(src.pkg_hash);
		out.Put64le(src.file_hash);
		out.Put32le(src.priority);
		out.Put32le((uint32)src.flags);
		WriteChunkString(out, src.local_path);
	}
}

static void WriteChunkMetadata(Stream& out, const ValueMap& metadata) {
	String payload;
	if (!metadata.IsEmpty()) {
		Value stored = metadata;
		payload = StoreAsString(stored);
	}
	WriteChunkString(out, payload);
}

OverlayIndexChunkWriter::OverlayIndexChunkWriter() {}

OverlayIndexChunkWriter::~OverlayIndexChunkWriter() {
	CloseStream();
}

void OverlayIndexChunkWriter::CloseStream() {
	if (stream) {
		stream->Close();
		stream.Clear();
	}
}

bool OverlayIndexChunkWriter::Begin(const String& path) {
	CloseStream();
	error = false;
	if (!RealizePath(GetFileFolder(path)))
		return false;
	stream.Create<FileOut>(path);
	if (!stream || !stream->IsOpen())
		return false;
	stream->Put32le(kOverlayChunkMagic);
	stream->Put16le((uint16)kVfsOverlayIndexVersion);
	stream->Put16le(0);
	return stream->IsOK();
}

bool OverlayIndexChunkWriter::Finish() {
	if (!stream)
		return false;
	stream->Close();
	bool ok = stream->IsOK() && !error;
	stream.Clear();
	return ok;
}

bool OverlayIndexChunkWriter::IsOpen() const {
	return stream && stream->IsOpen();
}

void OverlayIndexChunkWriter::AddRecord(const OverlayNodeRecord& record) {
	if (!stream || error)
		return;
	int64 chunk_start = stream->GetPos();
	stream->Put32le(kOverlayChunkNodeTag);
	stream->Put32le(0); // placeholder for size
	WriteChunkString(*stream, record.path);
	WriteChunkSources(*stream, record.sources);
	WriteChunkMetadata(*stream, record.metadata);
	int64 chunk_end = stream->GetPos();
	dword chunk_size = (dword)(chunk_end - chunk_start - 8);
	stream->Seek(chunk_start + 4);
	stream->Put32le(chunk_size);
	stream->Seek(chunk_end);
	if (!stream->IsOK())
		error = true;
}

static bool ReadChunkString(Stream& in, String& out) {
	uint32 len = in.Get32le();
	if (len > kOverlayChunkStringLimit)
		return false;
	String data = in.Get((int)len);
	if (data.GetLength() != (int)len)
		return false;
	out = pick(data);
	return true;
}

static bool ReadChunkSources(Stream& in, Vector<SourceRef>& out_sources) {
	uint32 count = in.Get32le();
	if (count > 4096)
		return false;
	out_sources.SetCount((int)count);
	for (uint32 i = 0; i < count; i++) {
		SourceRef& ref = out_sources[(int)i];
		ref.pkg_hash = in.Get64le();
		ref.file_hash = in.Get64le();
		ref.priority = in.Get32le();
		ref.flags = (dword)in.Get32le();
		String local_path;
		if (!ReadChunkString(in, local_path))
			return false;
		ref.local_path = pick(local_path);
	}
	return true;
}

static bool ReadChunkMetadata(Stream& in, ValueMap& metadata) {
	String payload;
	if (!ReadChunkString(in, payload))
		return false;
	if (payload.IsEmpty()) {
		metadata.Clear();
		return true;
	}
	try {
		Value restored;
		if (!LoadFromString(restored, payload))
			return false;
		if (!restored.Is<ValueMap>())
			return false;
		metadata = restored;
	}
	catch (...) {
		return false;
	}
	return true;
}

void BuildRouterOverlayIndex(const VfsValue& fragment, OverlayIndexSink& sink) {
	CollectRouterOverlayEntries(fragment, String(), sink);
}

void BuildRouterOverlayIndex(const VfsValue& fragment, VfsOverlayIndex& out_index) {
	OverlayIndexCollectorSink collector(out_index);
	BuildRouterOverlayIndex(fragment, collector);
}

bool VfsSaveFragment(const String& path, const VfsValue& fragment) {
	try {
		if (!RealizePath(GetFileFolder(path)))
			return false;
		FragmentDocument doc;
		FillDocumentFromFragment(fragment, doc);
		String json = StoreAsJson(doc, true);
		return SaveFile(path, json);
	}
	catch (const Exc& e) {
		RLOG("VfsSaveFragment: failed to save '" << path << "': " << e);
	}
	catch (...) {
		RLOG("VfsSaveFragment: unknown error while saving '" << path << "'");
	}
	return false;
}

bool VfsLoadFragment(const String& path, VfsValue& out_fragment) {
	try {
		String data = LoadFile(path);
		FragmentDocument doc;
		if (!LoadFromJson(doc, ~data))
			return false;
		if (doc.version != 0 && doc.version != kVfsFragmentVersion) {
			RLOG("VfsLoadFragment: unsupported fragment version " << doc.version << " for '" << path << "'");
			return false;
		}
		// Allow payload to override header hashes if they are missing.
		if (!doc.pkg_hash)
			doc.pkg_hash = doc.root.pkg_hash;
		if (!doc.file_hash)
			doc.file_hash = doc.root.file_hash;
		out_fragment.Assign(nullptr, doc.root);
		PropagateFragmentHashes(out_fragment, doc.pkg_hash, doc.file_hash);
		return true;
	}
	catch (const Exc& e) {
		RLOG("VfsLoadFragment: failed to load '" << path << "': " << e);
	}
	catch (...) {
		RLOG("VfsLoadFragment: unknown error while loading '" << path << "'");
	}
	return false;
}

bool VfsSaveFragmentBinary(const String& path, const VfsValue& fragment) {
	FragmentDocument doc;
	FillDocumentFromFragment(fragment, doc);
	String payload = StoreAsJson(doc, false);
	return StoreBinaryEnvelope(path, kFragmentBinaryMagic, kVfsFragmentVersion, payload);
}

bool VfsLoadFragmentBinary(const String& path, VfsValue& out_fragment) {
	String payload;
	if (!LoadBinaryEnvelope(path, kFragmentBinaryMagic, kVfsFragmentVersion, payload))
		return false;
	FragmentDocument doc;
	if (!LoadFromJson(doc, ~payload))
		return false;
	if (!doc.pkg_hash)
		doc.pkg_hash = doc.root.pkg_hash;
	if (!doc.file_hash)
		doc.file_hash = doc.root.file_hash;
	out_fragment.Assign(nullptr, doc.root);
	PropagateFragmentHashes(out_fragment, doc.pkg_hash, doc.file_hash);
	return true;
}

bool VfsLoadLegacy(const String& path, VfsValue& out_fragment) {
	// Placeholder: once the legacy binary/JSON reader migrates we can attempt it here.
	(void)path;
	(void)out_fragment;
	return false;
}

bool VfsSaveOverlayIndex(const String& path, const VfsOverlayIndex& index) {
	try {
		if (!RealizePath(GetFileFolder(path)))
			return false;
		OverlayIndexDocument doc;
		FillDocumentFromIndex(index, doc);
		String json = StoreAsJson(doc, true);
		return SaveFile(path, json);
	}
	catch (const Exc& e) {
		RLOG("VfsSaveOverlayIndex: failed to save '" << path << "': " << e);
	}
	catch (...) {
		RLOG("VfsSaveOverlayIndex: unknown error while saving '" << path << "'");
	}
	return false;
}

bool VfsLoadOverlayIndex(const String& path, VfsOverlayIndex& out_index) {
	out_index.Clear();
	try {
		String data = LoadFile(path);
		OverlayIndexDocument doc;
		if (!LoadFromJson(doc, ~data))
			return false;
		if (doc.version != 0 && doc.version != kVfsOverlayIndexVersion) {
			RLOG("VfsLoadOverlayIndex: unsupported overlay version " << doc.version << " for '" << path << "'");
			return false;
		}
		AssignIndexFromDocument(doc, out_index);
		return true;
	}
	catch (const Exc& e) {
		RLOG("VfsLoadOverlayIndex: failed to load '" << path << "': " << e);
	}
	catch (...) {
		RLOG("VfsLoadOverlayIndex: unknown error while loading '" << path << "'");
	}
	return false;
}

bool VfsSaveOverlayIndexBinary(const String& path, const VfsOverlayIndex& index) {
	OverlayIndexDocument doc;
	FillDocumentFromIndex(index, doc);
	String payload = StoreAsJson(doc, false);
	return StoreBinaryEnvelope(path, kOverlayBinaryMagic, kVfsOverlayIndexVersion, payload);
}

bool VfsLoadOverlayIndexBinary(const String& path, VfsOverlayIndex& out_index) {
	String payload;
	if (!LoadBinaryEnvelope(path, kOverlayBinaryMagic, kVfsOverlayIndexVersion, payload))
		return false;
	OverlayIndexDocument doc;
	if (!LoadFromJson(doc, ~payload))
		return false;
	AssignIndexFromDocument(doc, out_index);
	return true;
}

bool VfsSaveOverlayIndexChunked(const String& path, const VfsValue& fragment) {
	OverlayIndexChunkWriter writer;
	if (!writer.Begin(path))
		return false;
	BuildRouterOverlayIndex(fragment, writer);
	return writer.Finish();
}

bool VfsLoadOverlayIndexChunked(const String& path, VfsOverlayIndex& out_index) {
	out_index.Clear();
	FileIn in(path);
	if (!in.IsOpen()) {
		RLOG("VfsLoadOverlayIndexChunked: unable to open '" << path << "'");
		return false;
	}
	dword magic = in.Get32le();
	if (magic != kOverlayChunkMagic) {
		RLOG("VfsLoadOverlayIndexChunked: unexpected magic in '" << path << "'");
		return false;
	}
	int version = in.Get16le();
	in.Get16le(); // reserved
	if (version != 0 && version != kVfsOverlayIndexVersion) {
		RLOG("VfsLoadOverlayIndexChunked: unsupported version " << version << " for '" << path << "'");
		return false;
	}
	while (!in.IsEof() && in.IsOpen()) {
		int c = in.Peek();
		if (c < 0)
			break;
		dword chunk_type = in.Get32le();
		dword chunk_size = in.Get32le();
		int64 chunk_start = in.GetPos();
		if (chunk_type == kOverlayChunkNodeTag) {
			OverlayNodeRecord rec;
			if (!ReadChunkString(in, rec.path)) {
				RLOG("VfsLoadOverlayIndexChunked: failed to read path chunk in '" << path << "'");
				return false;
			}
			if (!ReadChunkSources(in, rec.sources)) {
				RLOG("VfsLoadOverlayIndexChunked: failed to read sources for '" << path << "'");
				return false;
			}
			if (!ReadChunkMetadata(in, rec.metadata)) {
				RLOG("VfsLoadOverlayIndexChunked: failed to read metadata for '" << path << "'");
				return false;
			}
			out_index.nodes.Add(pick(rec));
		}
		else {
			if (chunk_size > 0x2000000) {
				RLOG("VfsLoadOverlayIndexChunked: chunk too large (" << chunk_size << ") in '" << path << "'");
				return false;
			}
			in.Seek(chunk_start + chunk_size);
		}
		int64 chunk_end = in.GetPos();
		int64 consumed = chunk_end - chunk_start;
		if (consumed < (int64)chunk_size)
			in.Seek(chunk_start + chunk_size);
	}
	return true;
}

END_UPP_NAMESPACE
