#ifndef _Core_VfsBase_Mount_h_
#define _Core_VfsBase_Mount_h_

NAMESPACE_UPP

// Process-wide registry of mounted virtual filesystems.
//
// One singleton serves every kind of consumer: a plain file manager, a standalone
// virtual filesystem, an attachable asset debugger, or a GUI widget hierarchy
// debugger. Mounts are therefore not assumed to be browsable filesystems --
// MountPoint::Caps() reports the role, and Enumerate() selects by role.
//
// Overlaying is a first-class feature: a mount may be placed at any path, and the
// deepest matching mount wins. Resolution is longest-prefix, so mounting at
// /home/user/archive shadows the root mount for that subtree only.
struct MountManager {
	struct MountPoint {
		VfsPath path;
		String type;
		Ptr<VFS> vfs;
		One<VFS> owned_vfs;

		//< Capabilities of the mounted VFS, or an empty descriptor if no VFS is
		//< attached. Never null.
		const MountCapabilities& Caps() const;

		bool   IsBrowsable() const  { return Caps().IsBrowsable(); }
		bool   CanMutate() const    { return Caps().CanMutate(); }
	};

	enum MountFlags {
		MOUNT_DEFAULT     = 0,
		MOUNT_REJECT_SHADOW = 1 << 0, //< fail if a deeper mount already shadows this path
	};

	typedef MountManager CLASSNAME;
	MountManager();

	bool HasRoot() const;

	//< Mounts a **borrowed** vfs at path: the caller keeps ownership and must keep
	//< the object alive for as long as the mount exists. Use this for stack or
	//< otherwise externally owned filesystems, and MountOwned() for heap VFSes.
	//< An already mounted exact path is rejected. Shadowing a deeper mount is
	//< allowed by default (that is how overlays are built); pass
	//< MOUNT_REJECT_SHADOW to refuse it instead. On failure last_error explains why.
	bool Mount(String path, VFS* vfs, String type = String(), int flags = MOUNT_DEFAULT);

	//< Mounts vfs at path and **takes ownership**: the VFS is released by
	//< Unmount() or when the MountManager itself goes away.
	//<
	//< Ownership is never inferred. Core/Ptr.h defines Ptr<T> as a weak reference
	//< that shares the reference block but never deletes the pointee, so a raw
	//< VFS* handed to Mount() would leak. One<T> is the owning half.
	bool MountOwned(String path, One<VFS> vfs, String type = String(), int flags = MOUNT_DEFAULT);

	//< Detaches the mount at path and releases the owned VFS. Fails when no such
	//< mount exists.
	bool Unmount(String path);

	//< Exact mount point lookup, by string or path.
	MountPoint* Find(const String& path);
	MountPoint* Find(const VfsPath& path, VfsPath* rel_path = nullptr);

	//< Deepest mount whose path is a prefix of path. This is what makes overlays
	//< resolve correctly; when rel_path is given it receives the path relative to
	//< the mount.
	MountPoint* FindDeepest(const VfsPath& path, VfsPath* rel_path = nullptr);

	//< Role-based enumerator. Pass UNKNOWN_ROLE to get every mount. Result order is
	//< stable (deepest path first) so callers get deterministic listings.
	Vector<MountPoint*> Enumerate(MountCapabilities::Role role = MountCapabilities::UNKNOWN_ROLE);

	//< True when the deepest mount owning path is browsable as a filesystem.
	bool IsBrowsable(const VfsPath& path);

	//< Mounts that shadow other mounts, deepest first. Diagnostics for the log view.
	Vector<MountPoint*> GetShadowed();

	int  GetMountCount() const                        { return mounts.GetCount(); }

	bool GetFiles(const VfsPath& path, Vector<Shared<VfsItem>>& items);
	bool DirectoryExists(const VfsPath& dir);
	bool FileExists(const VfsPath& dir);
	bool Exists(const VfsPath& dir, VfsItemType type);

	//< Mutation, addressed by the same full paths the readers above take, so a
	//< caller never has to resolve a mount by hand to change something.
	//<
	//< Each one resolves the deepest owning mount and delegates to it, refusing
	//< when that mount lacks the matching capability flag. Nothing here
	//< *authorizes* anything: the Vfs layer has no opinion about permissions, and
	//< the caller's policy is expected to have run before it gets here.
	//<
	//< Rename, Copy and Move need both endpoints in the *same* mount. They refuse
	//< across mounts because VFS can list a tree but exposes no way to read an
	//< entry's bytes, so moving data between two filesystems is not expressible at
	//< this layer -- saying so beats half-copying.
	bool CreateDir(const VfsPath& path);
	bool CreateFile(const VfsPath& path);
	bool CreateSymlink(const VfsPath& link_path, const VfsPath& target);
	bool Remove(const VfsPath& path, bool recursive = false);
	bool Rename(const VfsPath& from, const VfsPath& to);
	bool Copy(const VfsPath& from, const VfsPath& to);
	bool Move(const VfsPath& from, const VfsPath& to);

	//< Total and free bytes of the device holding `path`, as reported by the mount
	//< that owns it. False when that mount cannot answer -- the normal case for an
	//< AST-backed tree -- so a caller must render "unknown" rather than 0. A
	//< read-only filesystem still has a perfectly good answer here, so this needs no
	//< capability flag.
	bool GetDiskSpace(const VfsPath& path, int64& total, int64& free_space);

	Array<MountPoint> mounts;
	String last_error;

	static MountManager& System() {static MountManager m; return m;}

private:
int  Depth(const MountPoint& mp) const                 { return mp.path.GetPartCount(); }
	void SortMounts();
	mutable MountCapabilities empty_caps;
};

// Convenient unqualified spelling of the nested type.
typedef MountManager::MountPoint MountPoint;

END_UPP_NAMESPACE

#endif