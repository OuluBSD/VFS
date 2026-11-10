#ifndef _Vfs_Overlay_VfsOverlay_h_
#define _Vfs_Overlay_VfsOverlay_h_

struct SourceRef : Moveable<SourceRef> {
    hash_t pkg_hash = 0;
    hash_t file_hash = 0;
    String local_path;   // path or id inside the source fragment
    int    priority = 0; // higher wins
    dword  flags = 0;    // e.g., disabled
    
    SourceRef();
    SourceRef(hash_t pkg_hash, hash_t file_hash, const String& local_path, int priority = 0);
};

// Overlay view interface for virtual merge of per-file trees with provenance
struct OverlayView {
    // Resolve an effective child set for a logical node identified by path
    virtual Vector<String> List(String logical_path) const = 0;
    // Get merged json (for debugging/inspection)
    virtual Value GetMerged(String logical_path) const = 0;
    virtual ~OverlayView() {}
};

// Concrete implementation of an overlay representing a single source fragment
class VfsOverlay : public OverlayView, public Pte<VfsOverlay> {
    SourceRef source;
    VectorMap<String, One<VfsValue>> fragments;  // path -> VfsValue fragments (using One instead of direct storage)
    
public:
    VfsOverlay();
    VfsOverlay(const SourceRef& source_ref);
    virtual ~VfsOverlay();
    
    // OverlayView implementation
    virtual Vector<String> List(String logical_path) const override;
    virtual Value GetMerged(String logical_path) const override;
    
    // Additional methods for managing fragments
    void AddFragment(const String& path, const VfsValue& fragment);
    const VfsValue* GetFragment(const String& path) const;
    
    const SourceRef& GetSource() const { return source; }
};

// Manager for combining multiple overlays with precedence
class OverlayManager : public OverlayView {
    Vector<Ptr<VfsOverlay>> overlays;
    
    OverlayManager();  // Singleton
    
public:
    static OverlayManager& GetInstance();
    
    void AddOverlay(Ptr<VfsOverlay> overlay);
    void RemoveOverlay(Ptr<VfsOverlay> overlay);
    
    // OverlayView implementation
    virtual Vector<String> List(String logical_path) const override;
    virtual Value GetMerged(String logical_path) const override;
    
    // Get overlays in precedence order
    const Vector<Ptr<VfsOverlay>>& GetOverlays() const { return overlays; }
};

#endif