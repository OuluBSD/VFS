// Vfs/Storage header (API scaffold)
#ifndef _Vfs_Storage_VfsStorage_h_
#define _Vfs_Storage_VfsStorage_h_

#include <Core/Core.h>
#include <Vfs/Core/Core.h>
#include <Vfs/Overlay/Overlay.h>

NAMESPACE_UPP

struct VfsValue;

// Save/load per-file fragments; overlay index handled separately.
// Planned JSON schema (draft):
// {
//   "version": 1,
//   "pkg_hash": uint64,
//   "file_hash": uint64,
//   "root": Node
// }
// Node -> {
//   "id": string,
//   "type_hash": uint64,
//   "value": {...},        // Upp::Value or Ast payload; serialized via helpers
//   "ext": {...},          // VfsValueExt payload keyed by ext type_hash
//   "flags": uint32,
//   "children": [Node]
// }
// Value payload encodes whether it's Upp::Value, AstValue, or extension-defined.

bool VfsSaveFragment(const String& path, const VfsValue& fragment);
bool VfsLoadFragment(const String& path, VfsValue& out_fragment);

bool VfsSaveFragmentBinary(const String& path, const VfsValue& fragment);
bool VfsLoadFragmentBinary(const String& path, VfsValue& out_fragment);

// Back-compat loader for legacy IDE dumps (placeholder).
bool VfsLoadLegacy(const String& path, VfsValue& out_fragment);

// Overlay index captures which on-disk fragments contribute to a logical node.
struct OverlayNodeRecord : Moveable<OverlayNodeRecord> {
	String            path;
	Vector<SourceRef> sources;
	ValueMap          metadata; // Allows router metadata (and future data) without new schema tweaks.
};

struct VfsOverlayIndex : Moveable<VfsOverlayIndex> {
	Vector<OverlayNodeRecord> nodes;

	bool IsEmpty() const { return nodes.IsEmpty(); }
	void Clear() { nodes.Clear(); }
};

class OverlayIndexSink {
public:
	virtual ~OverlayIndexSink() {}
	virtual void AddRecord(const OverlayNodeRecord& record) = 0;
};

class OverlayIndexCollectorSink : public OverlayIndexSink {
public:
	explicit OverlayIndexCollectorSink(VfsOverlayIndex& dst) : target(dst) { target.Clear(); }

	void AddRecord(const OverlayNodeRecord& record) override {
		OverlayNodeRecord& rec = target.nodes.Add();
		rec.path = record.path;
		rec.sources <<= record.sources;
		rec.metadata = record.metadata;
	}

private:
	VfsOverlayIndex& target;
};

class OverlayIndexSinkMultiplexer : public OverlayIndexSink {
public:
	void AddSink(OverlayIndexSink& sink) { sinks.Add(&sink); }

	void AddRecord(const OverlayNodeRecord& record) override {
		for (OverlayIndexSink* sink : sinks)
			sink->AddRecord(record);
	}

private:
	Vector<OverlayIndexSink*> sinks;
};

class OverlayIndexChunkWriter : public OverlayIndexSink {
public:
	OverlayIndexChunkWriter();
	~OverlayIndexChunkWriter();

	bool Begin(const String& path);
	bool Finish();
	bool IsOpen() const;
	bool HasError() const { return error; }

	void AddRecord(const OverlayNodeRecord& record) override;

private:
	void CloseStream();

	One<FileOut> stream;
	bool         error = false;
};

bool VfsSaveOverlayIndex(const String& path, const VfsOverlayIndex& index);
bool VfsLoadOverlayIndex(const String& path, VfsOverlayIndex& out_index);

bool VfsSaveOverlayIndexBinary(const String& path, const VfsOverlayIndex& index);
bool VfsLoadOverlayIndexBinary(const String& path, VfsOverlayIndex& out_index);

bool VfsSaveOverlayIndexChunked(const String& path, const VfsValue& fragment);
bool VfsLoadOverlayIndexChunked(const String& path, VfsOverlayIndex& out_index);

// Helper to extract router overlay metadata from a fragment tree.
void BuildRouterOverlayIndex(const VfsValue& fragment, VfsOverlayIndex& out_index);
void BuildRouterOverlayIndex(const VfsValue& fragment, OverlayIndexSink& sink);

inline Value RouterLookupValue(const ValueMap& map, const char* key) {
	int idx = map.Find(Value(String(key)));
	if (idx >= 0)
		return map.GetValue(idx);
	return Value();
}

#ifndef _RouterPortDesc_defined_
#define _RouterPortDesc_defined_
struct RouterPortDesc : Moveable<RouterPortDesc> {
	enum class Direction { Sink, Source };

	Direction   direction = Direction::Sink;
	int         index = -1;
	String      name;
	ValueMap    metadata;

	bool IsValid() const { return index >= 0; }
};

struct RouterConnectionDesc : Moveable<RouterConnectionDesc> {
	String   from_atom;
	int      from_port = -1;
	String   to_atom;
	int      to_port = -1;
	ValueMap metadata;

	bool IsValid() const { return from_port >= 0 && to_port >= 0; }
};

inline ValueMap StoreRouterPortDesc(const RouterPortDesc& desc) {
	ValueMap map;
	map.Add("direction", desc.direction == RouterPortDesc::Direction::Source ? "source" : "sink");
	map.Add("index", desc.index);
	if (!desc.name.IsEmpty())
		map.Add("name", desc.name);
	if (!desc.metadata.IsEmpty())
		map.Add("metadata", Value(desc.metadata));
	return map;
}

inline bool LoadRouterPortDesc(const Value& value, RouterPortDesc& out) {
	if (!value.Is<ValueMap>())
		return false;
	ValueMap map = value;
	Value dir_value = RouterLookupValue(map, "direction");
	if (dir_value.Is<String>()) {
		String d = dir_value;
		out.direction = (d == "source") ? RouterPortDesc::Direction::Source : RouterPortDesc::Direction::Sink;
	} else if (dir_value.Is<int>()) {
		out.direction = (int)dir_value ? RouterPortDesc::Direction::Source : RouterPortDesc::Direction::Sink;
	}
	Value idx_value = RouterLookupValue(map, "index");
	if (idx_value.Is<int>())
		out.index = (int)idx_value;
	Value name_value = RouterLookupValue(map, "name");
	if (name_value.Is<String>())
		out.name = name_value;
	Value meta_value = RouterLookupValue(map, "metadata");
	if (meta_value.Is<ValueMap>())
		out.metadata = meta_value;
	return true;
}

inline ValueMap StoreRouterConnectionDesc(const RouterConnectionDesc& desc) {
	ValueMap map;
	map.Add("from_atom", desc.from_atom);
	map.Add("from_port", desc.from_port);
	map.Add("to_atom", desc.to_atom);
	map.Add("to_port", desc.to_port);
	if (!desc.metadata.IsEmpty())
		map.Add("metadata", Value(desc.metadata));
	return map;
}

inline bool LoadRouterConnectionDesc(const Value& value, RouterConnectionDesc& out) {
	if (!value.Is<ValueMap>())
		return false;
	ValueMap map = value;
	Value from_atom = RouterLookupValue(map, "from_atom");
	Value to_atom = RouterLookupValue(map, "to_atom");
	Value from_port = RouterLookupValue(map, "from_port");
	Value to_port = RouterLookupValue(map, "to_port");
	if (!from_atom.Is<String>() || !to_atom.Is<String>())
		return false;
	out.from_atom = from_atom;
	out.to_atom = to_atom;
	out.from_port = (int)from_port;
	out.to_port = (int)to_port;
	Value meta_value = RouterLookupValue(map, "metadata");
	if (meta_value.Is<ValueMap>())
		out.metadata = meta_value;
	return true;
}
#endif

struct RouterPortEntry : Moveable<RouterPortEntry> {
	String          atom_id;
	RouterPortDesc  desc;
};

struct RouterSchema : Moveable<RouterSchema> {
	Vector<RouterPortEntry>  ports;
	Vector<RouterConnectionDesc> connections;
	ValueMap                 flow_control;

	bool IsEmpty() const { return ports.IsEmpty() && connections.IsEmpty() && flow_control.IsEmpty(); }
	void Clear() {
		ports.Clear();
		connections.Clear();
		flow_control.Clear();
	}
};

inline ValueMap StoreRouterSchema(const RouterSchema& schema) {
	ValueMap router;
	if (!schema.ports.IsEmpty()) {
		ValueArray ports_arr;
		for (const RouterPortEntry& entry : schema.ports) {
			ValueMap stored = StoreRouterPortDesc(entry.desc);
			if (!entry.atom_id.IsEmpty())
				stored.Add("atom", entry.atom_id);
			else
				stored.Add("atom", String());
			ports_arr.Add(stored);
		}
		router.Add("ports", Value(ports_arr));
	}
	if (!schema.connections.IsEmpty()) {
		ValueArray conn_arr;
		for (const RouterConnectionDesc& conn : schema.connections)
			conn_arr.Add(StoreRouterConnectionDesc(conn));
		router.Add("connections", Value(conn_arr));
	}
	if (!schema.flow_control.IsEmpty())
		router.Add("flow_control", Value(schema.flow_control));
	return router;
}

inline bool LoadRouterSchema(const Value& value, RouterSchema& out) {
	out.Clear();
	if (IsNull(value))
		return true;
	if (!value.Is<ValueMap>())
		return false;
	ValueMap map = value;
	Value ports_value = RouterLookupValue(map, "ports");
	if (!IsNull(ports_value)) {
		if (!ports_value.Is<ValueArray>())
			return false;
		ValueArray ports_arr = ports_value;
		for (int i = 0; i < ports_arr.GetCount(); i++) {
			const Value& entry_val = ports_arr[i];
			if (!entry_val.Is<ValueMap>())
				return false;
			ValueMap entry_map = entry_val;
			RouterPortEntry entry;
			if (!LoadRouterPortDesc(entry_val, entry.desc))
				return false;
			Value atom_value = RouterLookupValue(entry_map, "atom");
			if (atom_value.Is<String>())
				entry.atom_id = atom_value.Get<String>();
			out.ports.Add(pick(entry));
		}
	}
	Value conn_value = RouterLookupValue(map, "connections");
	if (!IsNull(conn_value)) {
		if (!conn_value.Is<ValueArray>())
			return false;
		ValueArray conn_arr = conn_value;
		for (int i = 0; i < conn_arr.GetCount(); i++) {
			const Value& entry_val = conn_arr[i];
			RouterConnectionDesc conn;
			if (!LoadRouterConnectionDesc(entry_val, conn))
				return false;
			out.connections.Add(pick(conn));
		}
	}
	Value flow_value = RouterLookupValue(map, "flow_control");
	if (flow_value.Is<ValueMap>())
		out.flow_control = flow_value;
	else
		out.flow_control.Clear();
	return true;
}

END_UPP_NAMESPACE

#endif
