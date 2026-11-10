#include "Overlay.h"

NAMESPACE_UPP

// SourceRef implementation
SourceRef::SourceRef() {}

SourceRef::SourceRef(hash_t pkg_hash, hash_t file_hash, const String& local_path, int priority)
    : pkg_hash(pkg_hash), file_hash(file_hash), local_path(local_path), priority(priority) {}

// Basic OverlayView implementation
Vector<String> OverlayView::List(String logical_path) const {
    // Default implementation returns empty list
    return Vector<String>();
}

Value OverlayView::GetMerged(String logical_path) const {
    // Default implementation returns null value
    return Value();
}

// VfsOverlay implementation
VfsOverlay::VfsOverlay() {}

VfsOverlay::VfsOverlay(const SourceRef& source_ref)
    : source(source_ref) {}

VfsOverlay::~VfsOverlay() {}

Vector<String> VfsOverlay::List(String logical_path) const {
    // Return list of child names at the specified logical path
    // This is a simplified implementation - in reality, this would traverse the VFS tree
    Vector<String> result;
    
    // For now, return empty list
    return result;
}

Value VfsOverlay::GetMerged(String logical_path) const {
    // Return merged JSON representation for debugging/inspection
    // This is a simplified implementation
    return Value();
}

void VfsOverlay::AddFragment(const String& path, const VfsValue& fragment) {
    // Create a new VfsValue and assign from the fragment
    One<VfsValue>& frag = fragments.GetAdd(path);
    frag.Create();
    // Copy fields individually to avoid direct assignment which may not work with Pte classes
    frag->id = fragment.id;
    frag->type_hash = fragment.type_hash;
    frag->serial = fragment.serial;
    frag->file_hash = fragment.file_hash;
    frag->value = fragment.value;
    frag->pkg_hash = fragment.pkg_hash;
    // Copy other fields as needed
}

const VfsValue* VfsOverlay::GetFragment(const String& path) const {
    // Retrieve a VFS fragment by path
    int i = fragments.Find(path);
    return i >= 0 ? fragments[i].Get() : nullptr;
}

// OverlayManager implementation
OverlayManager::OverlayManager() {}

OverlayManager& OverlayManager::GetInstance() {
    static OverlayManager instance;
    return instance;
}

void OverlayManager::AddOverlay(Ptr<VfsOverlay> overlay) {
    if (overlay) {
        overlays.Add(overlay);
    }
}

void OverlayManager::RemoveOverlay(Ptr<VfsOverlay> overlay) {
    int i = FindIndex(overlays, overlay);
    if (i >= 0) {
        overlays.Remove(i);
    }
}

Vector<String> OverlayManager::List(String logical_path) const {
    // Union of all overlay lists, with precedence applied
    Vector<String> result;
    
    // Collect all items from all overlays
    for (const auto& overlay : overlays) {
        if (overlay) {
            Vector<String> items = overlay->List(logical_path);
            for (const String& item : items) {
                if (result.GetIndex(item) < 0) {
                    result.Add(item);
                }
            }
        }
    }
    
    return result;
}

Value OverlayManager::GetMerged(String logical_path) const {
    // Merge values from all overlays according to precedence
    // This is a simplified implementation
    ValueMap merged;
    
    // In a real implementation, this would merge values according to precedence rules
    // For now, we'll just return an empty map
    return merged;
}

END_UPP_NAMESPACE