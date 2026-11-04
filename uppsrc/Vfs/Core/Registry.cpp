#include "VfsValue.h"

NAMESPACE_UPP

INITBLOCK {
	VfsValueExtFactory::Register<VfsRegistry>("VfsRegistry", VFSEXT_NONE, "registry", "Vfs|Core");
}

void VfsRegistry::Register(String registry_path, String vfs_path) {
	Entry& e = entries.GetAdd(registry_path);
	e.path = vfs_path;
	e.cache = nullptr;  // Will resolve on first Find()
}

void VfsRegistry::Register(String registry_path, VfsValue* val) {
	Entry& e = entries.GetAdd(registry_path);
	e.path = "";  // No path, pointer-only registration
	e.cache = val;
}

VfsValue* VfsRegistry::Find(String registry_path) {
	int idx = entries.Find(registry_path);
	if (idx < 0)
		return nullptr;

	Entry& e = entries[idx];

	// Check cache first
	if (e.cache)
		return &*e.cache;

	// Resolve path if not cached
	if (!e.path.IsEmpty()) {
		// Get Engine root to search from
		Engine* eng = val.FindOwner<Engine>();
		if (!eng) {
			LOG("VfsRegistry::Find: error: no Engine found for path resolution");
			return nullptr;
		}

		// Search from Engine root
		VfsValue* found = eng->val.FindOwnerWithPath(e.path);
		if (found) {
			e.cache = found;
			return found;
		}

		LOG("VfsRegistry::Find: error: path '" << e.path << "' not found for registry entry '" << registry_path << "'");
		return nullptr;
	}

	return nullptr;
}

void VfsRegistry::InvalidateCache() {
	for (int i = 0; i < entries.GetCount(); i++) {
		entries[i].cache = nullptr;
	}
}

void VfsRegistry::Unregister(String registry_path) {
	entries.RemoveKey(registry_path);
}

String VfsRegistry::Dump() const {
	String s;
	s << "VfsRegistry entries (" << entries.GetCount() << "):\n";
	for (int i = 0; i < entries.GetCount(); i++) {
		const String& key = entries.GetKey(i);
		const Entry& e = entries[i];
		s << "  " << key << " -> ";
		if (e.cache)
			s << "cached(" << (void*)&*e.cache << ")";
		else if (!e.path.IsEmpty())
			s << "path(" << e.path << ")";
		else
			s << "INVALID";
		s << "\n";
	}
	return s;
}

void VfsRegistry::Visit(RuntimeVisitor& vis) {
	vis % entries;
}

// Helper: Get registry from Engine root
VfsRegistry* GetVfsRegistry(Engine& eng) {
	VfsRegistry* reg = eng.val.Find<VfsRegistry>();
	if (!reg) {
		// Create registry on first access
		reg = &eng.val.Create<VfsRegistry>();
		LOG("GetVfsRegistry: created new VfsRegistry at Engine root");
	}
	return reg;
}

VfsRegistry* GetVfsRegistry(VfsValue& val) {
	Engine* eng = val.FindOwner<Engine>();
	if (!eng) {
		LOG("GetVfsRegistry: error: no Engine found");
		return nullptr;
	}
	return GetVfsRegistry(*eng);
}

END_UPP_NAMESPACE
