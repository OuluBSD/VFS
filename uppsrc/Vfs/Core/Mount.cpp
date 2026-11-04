#include "VfsBase.h"

NAMESPACE_UPP

const char *MountCapabilities::RoleName(MountCapabilities::Role r)
{
	switch(r) {
	case MountCapabilities::FILESYSTEM:     return "filesystem";
	case MountCapabilities::ASSET_DEBUGGER: return "asset-debugger";
	case MountCapabilities::GUI_HIERARCHY:  return "gui-hierarchy";
	default:                                return "unknown";
	}
}

String MountCapabilities::ToString() const
{
	String s = RoleName(role);
	s << " [";
	bool first = true;
	static const struct { int flag; const char *name; } names[] = {
		{ MUTABLE,          "mutable"     },
		{ CREATE_DIR,       "create-dir"  },
		{ CREATE_FILE,      "create-file" },
		{ CREATE_SYMLINK,   "create-symlink" },
		{ CASE_SENSITIVE,   "case"        },
		{ HAS_METADATA,     "metadata"    },
		{ SUPPORTS_OVERLAY, "overlay"     },
	};
	for(const auto& n : names)
		if(Has(n.flag)) {
			if(!first) s << ',';
			s << n.name;
			first = false;
		}
	s << ']';
	if(!description.IsEmpty()) s << ' ' << description;
	return s;
}

const char *MountCapabilities::FlagName(int f)
{
	static const struct { int flag; const char *name; } names[] = {
		{ MUTABLE,          "mutable"     },
		{ CREATE_DIR,       "create-dir"  },
		{ CREATE_FILE,      "create-file" },
		{ CREATE_SYMLINK,   "create-symlink" },
		{ CASE_SENSITIVE,   "case"        },
		{ HAS_METADATA,     "metadata"    },
		{ SUPPORTS_OVERLAY, "overlay"     },
	};
	for(const auto& n : names)
		if(n.flag == f)
			return n.name;
	return nullptr;
}

const MountCapabilities& MountManager::MountPoint::Caps() const
{
	static const MountCapabilities none;
	return vfs ? vfs->Caps() : none;
}

MountManager::MountManager()
{
}

bool MountManager::HasRoot() const {
	for (const auto& mm : mounts)
		if (mm.path.IsEmpty())
			return true;
	return false;
}

void MountManager::SortMounts()
{
	// Deepest first, so that the first match found by a linear scan is already the
	// winner. Insertion sort keeps insertion order among equal depths, which keeps
	// listings deterministic. Mount counts are small, so this is not a hot path.
	for(int i = 1; i < mounts.GetCount(); i++) {
		int j = i;
		while(j > 0 && mounts[j - 1].path.GetPartCount() < mounts[j].path.GetPartCount()) {
			Swap(mounts[j - 1], mounts[j]);
			j--;
		}
	}
}

MountPoint* MountManager::Find(const String& path) {
	for (auto& mm : mounts)
		if (mm.path == path)
			return &mm;
	return 0;
}

MountPoint* MountManager::Find(const VfsPath& path, VfsPath* rel_path) {
	return FindDeepest(path, rel_path);
}

MountPoint* MountManager::FindDeepest(const VfsPath& path, VfsPath* rel_path) {
	MountPoint* best = 0;
	int best_len = -1;
	for (auto& mm : mounts) {
		if (!path.IsLeft(mm.path)) continue;
		int len = mm.path.GetPartCount();
		if (len > best_len) {
			best = &mm;
			best_len = len;
		}
	}
	if (best && rel_path)
		rel_path->Set(path, best->path.GetPartCount(), path.GetPartCount());
	return best;
}

bool MountManager::Mount(String path, VFS* vfs, String type, int flags)
{
	if(!vfs) {
		last_error = "Mount: null vfs for '" + path + "'";
		return false;
	}
	if(Find(path)) {
		last_error = "Mount: path already mounted '" + path + "'";
		return false;
	}
	if(flags & MOUNT_REJECT_SHADOW) {
		VfsPath probe;
		probe.Set(path);
		// IsLeft() reads "argument is a prefix of me", so mm.path.IsLeft(probe) means
		// the new mount point is an ancestor of an existing one: mounting it would
		// hide that mount, which is what this flag refuses.
		for(auto& mm : mounts)
			if(mm.path.IsLeft(probe) && mm.path.GetPartCount() > probe.GetPartCount()) {
				last_error = "Mount: '" + path + "' would shadow the deeper mount '" + mm.path.AsSysPath() + "'";
				return false;
			}
	}

	MountPoint& mp = mounts.Add();
	mp.path.Set(path);
	mp.type = type;
	mp.vfs = vfs;
	SortMounts();
	return true;
}

bool MountManager::MountOwned(String path, One<VFS> vfs, String type, int flags)
{
	if(!Mount(path, vfs.operator~(), type, flags))
		return false;
	MountPoint& mp = *Find(path);
	mp.owned_vfs = pick(vfs);
	return true;
}

bool MountManager::Unmount(String path)
{
	for(int i = 0; i < mounts.GetCount(); i++) {
		if(mounts[i].path == path) {
			mounts.Remove(i);
			return true;
		}
	}
	last_error = "Unmount: no mount at '" + path + "'";
	return false;
}

Vector<MountPoint*> MountManager::Enumerate(MountCapabilities::Role role)
{
	Vector<MountPoint*> out;
	for(auto& mm : mounts)
		if(role == MountCapabilities::UNKNOWN_ROLE || mm.Caps().role == role)
			out.Add(&mm);
	return out;
}

bool MountManager::IsBrowsable(const VfsPath& path)
{
	MountPoint* mp = FindDeepest(path);
	return mp && mp->IsBrowsable();
}

Vector<MountPoint*> MountManager::GetShadowed()
{
	Vector<MountPoint*> out;
	for(auto& mm : mounts) {
		bool shadowed = false;
		for(auto& other : mounts) {
			if(&other == &mm) continue;
			// IsLeft() reads "argument is a prefix of me": other.path.IsLeft(mm.path)
			// means mm.path is an ancestor of other.path, i.e. other sits below mm and
			// hides it for its own subtree.
			if(other.path.IsLeft(mm.path) && other.path.GetPartCount() > mm.path.GetPartCount()) {
				shadowed = true;
				break;
			}
		}
		if(shadowed) out.Add(&mm);
	}
	return out;
}

bool MountManager::GetFiles(const VfsPath& path, Vector<Shared<VfsItem>>& items) {
	items.Clear();
	for (auto& mm : mounts) {
		// A mount shows up as a directory entry in its own parent. The root mount
		// has no parent, so it is not repeated as an entry.
		if (mm.path.GetPartCount() && mm.path.GetPartCount()-1 == path.GetPartCount() &&
			mm.path.IsSame(path, 0, 0, path.GetPartCount())) {
			VfsItem& item = items.Add().Create<VfsItem>();
			item.name = mm.path.TopPart();
			item.type_str = mm.type;
			#if INTERNAL_CPM || INTERNAL_COMB
			if (mm.path.GetPartCount() == 1) {
				item.name += ":";
			}
			#endif
		}
	}
	// An empty path is the mount root itself, so delegation must happen there too:
	// skipping it is what used to make the root directory look empty.
	VfsPath rel_path;
	MountPoint* mp = FindDeepest(path, &rel_path);
	if (!mp) {
		last_error = "Path is not contained in any mount: " + path;
		return false;
	}
	if (!mp->vfs) {
		last_error = "Null vfs in mountpoint";
		return false;
	}
	if (!mp->vfs->GetFiles(rel_path, items)) {
		last_error = mp->vfs->last_error.IsEmpty()
			? String("Mount '") + mp->path.AsSysPath("/") + "': failed to list '" + path.AsSysPath("/") + "'"
			: mp->vfs->last_error;
		return false;
	}
	return true;
}

bool MountManager::DirectoryExists(const VfsPath& dir) {
	return Exists(dir, VFS_DIRECTORY);
}

// ---------------------------------------------------------------------------
// Mutation
// ---------------------------------------------------------------------------

//< Resolves the mount owning `path` and hands back its mount-relative form.
//< Fills last_error and returns false when there is no mount, no VFS behind it,
//< or the mount declines to be written to. `cap` is the MountCapabilities flag
//< the operation needs on top of MUTABLE, or 0 when it needs none.
static bool ResolveForWrite(MountManager& mm, const VfsPath& path, int cap,
                            MountPoint*& mp, VfsPath& rel)
{
	VfsPath r;
	mp = mm.FindDeepest(path, &r);
	if(!mp) {
		mm.last_error = "Path is not contained in any mount: " + path;
		return false;
	}
	if(!mp->vfs) {
		mm.last_error = "Null vfs in mountpoint";
		return false;
	}
	if(!mp->vfs->Caps().CanMutate()) {
		mm.last_error = "Mount '" + mp->path.AsSysPath("/") + "' is read-only";
		return false;
	}
	if(cap && !mp->vfs->Caps().Has(cap)) {
		const char *name = MountCapabilities::FlagName(cap);
		mm.last_error = "Mount '" + mp->path.AsSysPath("/") + "' does not allow " +
		                (name ? String(name) : String("capability ") + Sprintf("%d", cap));
		return false;
	}
	rel = r;
	return true;
}

//< Both endpoints must land on the same mount. VFS lists a tree but cannot read an
//< entry's bytes, so a transfer between two filesystems has nothing to copy from.
static bool ResolveTransfer(MountManager& mm, const VfsPath& from, const VfsPath& to,
                            const char *op, MountPoint*& mp, VfsPath& rel_from,
                            VfsPath& rel_to)
{
	VfsPath rf, rt;
	MountPoint *mf = mm.FindDeepest(from, &rf), *mt = mm.FindDeepest(to, &rt);
	if(!mf) {
		mm.last_error = "Source is not contained in any mount: " + from;
		return false;
	}
	if(!mt) {
		mm.last_error = "Target is not contained in any mount: " + to;
		return false;
	}
	if(mf != mt) {
		mm.last_error = String(op) + " across mounts is not supported: '" +
		                from.AsSysPath("/") + "' is on '" +
		                mf->path.AsSysPath("/") + "', '" + to.AsSysPath("/") +
		                "' is on '" + mt->path.AsSysPath("/") + "'";
		return false;
	}
	if(!mf->vfs) {
		mm.last_error = "Null vfs in mountpoint";
		return false;
	}
	if(!mf->vfs->Caps().CanMutate()) {
		mm.last_error = "Mount '" + mf->path.AsSysPath("/") + "' is read-only";
		return false;
	}
	mp = mf;
	rel_from = rf;
	rel_to = rt;
	return true;
}

bool MountManager::CreateDir(const VfsPath& path) {
	MountPoint* mp;
	VfsPath rel;
	if(!ResolveForWrite(*this, path, MountCapabilities::CREATE_DIR, mp, rel))
		return false;
	if(!mp->vfs->CreateDir(rel)) {
		last_error = mp->vfs->last_error.IsEmpty()
			? "Mount '" + mp->path.AsSysPath("/") + "': cannot create directory '" +
			  path.AsSysPath("/") + "'"
			: mp->vfs->last_error;
		return false;
	}
	return true;
}

bool MountManager::CreateFile(const VfsPath& path) {
	MountPoint* mp;
	VfsPath rel;
	if(!ResolveForWrite(*this, path, MountCapabilities::CREATE_FILE, mp, rel))
		return false;
	if(!mp->vfs->CreateFile(rel)) {
		last_error = mp->vfs->last_error.IsEmpty()
			? "Mount '" + mp->path.AsSysPath("/") + "': cannot create file '" +
			  path.AsSysPath("/") + "'"
			: mp->vfs->last_error;
		return false;
	}
	return true;
}

bool MountManager::CreateSymlink(const VfsPath& link_path, const VfsPath& target) {
	MountPoint* mp;
	VfsPath rel;
	if(!ResolveForWrite(*this, link_path, MountCapabilities::CREATE_SYMLINK, mp, rel))
		return false;
	if(!mp->vfs->CreateSymlink(rel, target)) {
		last_error = mp->vfs->last_error.IsEmpty()
			? "Mount '" + mp->path.AsSysPath("/") + "': cannot create symlink '" +
			  link_path.AsSysPath("/") + "' to '" + target.AsSysPath("/") + "'"
			: mp->vfs->last_error;
		return false;
	}
	return true;
}

bool MountManager::Remove(const VfsPath& path, bool recursive) {
	MountPoint* mp;
	VfsPath rel;
	if(!ResolveForWrite(*this, path, 0, mp, rel))
		return false;
	if(!mp->vfs->Remove(rel, recursive)) {
		last_error = mp->vfs->last_error.IsEmpty()
			? "Mount '" + mp->path.AsSysPath("/") + "': cannot remove '" +
			  path.AsSysPath("/") + "'"
			: mp->vfs->last_error;
		return false;
	}
	return true;
}

bool MountManager::Rename(const VfsPath& from, const VfsPath& to) {
	MountPoint* mp;
	VfsPath rf, rt;
	if(!ResolveTransfer(*this, from, to, "rename", mp, rf, rt))
		return false;
	if(!mp->vfs->Rename(rf, rt)) {
		last_error = mp->vfs->last_error.IsEmpty()
			? "Mount '" + mp->path.AsSysPath("/") + "': cannot rename '" +
			  from.AsSysPath("/") + "'"
			: mp->vfs->last_error;
		return false;
	}
	return true;
}

bool MountManager::Copy(const VfsPath& from, const VfsPath& to) {
	MountPoint* mp;
	VfsPath rf, rt;
	if(!ResolveTransfer(*this, from, to, "copy", mp, rf, rt))
		return false;
	if(!mp->vfs->Transfer(rf, rt, false)) {
		last_error = mp->vfs->last_error.IsEmpty()
			? "Mount '" + mp->path.AsSysPath("/") + "': cannot copy '" +
			  from.AsSysPath("/") + "'"
			: mp->vfs->last_error;
		return false;
	}
	return true;
}

bool MountManager::GetDiskSpace(const VfsPath& path, int64& total, int64& free_space)
{
	VfsPath rel;
	MountPoint* mp = FindDeepest(path, &rel);
	if(!mp) {
		last_error = "Path is not contained in any mount: " + path;
		return false;
	}
	if(!mp->vfs) {
		last_error = "Null vfs in mountpoint";
		return false;
	}
	// Deliberately not gated on CanMutate: a read-only mount still sits on a device
	// with a known size, and refusing to say so would be a worse answer than the
	// truth.
	if(!mp->vfs->GetDiskSpace(rel, total, free_space)) {
		last_error = mp->vfs->last_error.IsEmpty()
			? String("Mount '") + mp->path.AsSysPath("/") +
			  "' does not report disk space for '" + path.AsSysPath("/") + "'"
			: mp->vfs->last_error;
		return false;
	}
	return true;
}

bool MountManager::Move(const VfsPath& from, const VfsPath& to) {
	MountPoint* mp;
	VfsPath rf, rt;
	if(!ResolveTransfer(*this, from, to, "move", mp, rf, rt))
		return false;
	if(!mp->vfs->Transfer(rf, rt, true)) {
		last_error = mp->vfs->last_error.IsEmpty()
			? "Mount '" + mp->path.AsSysPath("/") + "': cannot move '" +
			  from.AsSysPath("/") + "'"
			: mp->vfs->last_error;
		return false;
	}
	return true;
}

bool MountManager::FileExists(const VfsPath& dir) {
	return Exists(dir, VFS_FILE);
}

bool MountManager::Exists(const VfsPath& dir, VfsItemType type) {
	if (dir.GetPartCount() == 0)
		return true;
	VfsPath rel_path;
	MountPoint* mp = FindDeepest(dir, &rel_path);
	if (!mp || !mp->vfs)
		return false;
	VfsItemType path_type = mp->vfs->CheckItem(rel_path);
	return path_type == type;
}

END_UPP_NAMESPACE