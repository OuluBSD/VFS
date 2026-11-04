#ifndef _Vfs_Core_Registry_h_
#define _Vfs_Core_Registry_h_


// VfsValue tree registry - Windows Registry style service locator
// Provides reliable path-based lookups for services, states, and resources
// without fragile VfsValue path traversal.
//
// Registry roots (like Windows Registry):
//   MACHINE     - Machine-wide resources (chains, loops, drivers)
//   SERVICES    - Running services (systems, managers)
//   STATES      - Runtime state containers (EnvState, etc.)
//   ENTITIES    - Entity pools and hierarchies
//   CONFIG      - Configuration values
//
// Path format: "ROOT/Category/Subcategory/Name"
// Examples:
//   "MACHINE/Chains/Program"
//   "SERVICES/Interaction/Manager"
//   "STATES/Input/Keyboard"
//   "ENTITIES/World/Player/Body"

class VfsRegistry : public VfsValueExt {
public:
	RTTI_DECL1(VfsRegistry, VfsValueExt)

	// Registry entry - stores both path and cached pointer
	struct Entry {
		String path;           // VfsValue tree path: "loop/event/register"
		Ptr<VfsValue> cache;   // Cached pointer (auto-nulls when destroyed)

		Entry() {}
		Entry(String p) : path(p) {}
		Entry(String p, VfsValue* v) : path(p), cache(v) {}
	};

	// Registry storage: "SERVICES/Interaction/Manager" -> Entry{path, cache}
	VectorMap<String, Entry> entries;

	// Register a service/state/resource with registry path
	void Register(String registry_path, String vfs_path);
	void Register(String registry_path, VfsValue* val);

	// Lookup by registry path - returns VfsValue pointer
	VfsValue* Find(String registry_path);

	// Lookup with type casting
	template <class T>
	Ptr<T> Find(String registry_path) {
		VfsValue* v = Find(registry_path);
		return v ? v->FindExt<T>() : nullptr;
	}

	// Clear all cached pointers (call when tree structure changes)
	void InvalidateCache();

	// Remove entry
	void Unregister(String registry_path);

	// Debug: dump all entries
	String Dump() const;

	// Serialize
	void Visit(RuntimeVisitor& vis) override;
};

// Helper: Get registry from Engine root
VfsRegistry* GetVfsRegistry(Engine& eng);
VfsRegistry* GetVfsRegistry(VfsValue& val);  // Search up tree to find Engine

// Helper macros for common patterns
#define REGISTER_SERVICE(eng, name, val) \
	GetVfsRegistry(eng)->Register("SERVICES/" name, val)

#define FIND_SERVICE(eng, name, type) \
	GetVfsRegistry(eng)->Find<type>("SERVICES/" name)

#define REGISTER_STATE(eng, name, val) \
	GetVfsRegistry(eng)->Register("STATES/" name, val)

#define FIND_STATE(eng, name, type) \
	GetVfsRegistry(eng)->Find<type>("STATES/" name)

#endif
