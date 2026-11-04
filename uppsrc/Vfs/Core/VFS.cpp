#include "VfsBase.h"

#ifdef PLATFORM_POSIX
#include <sys/types.h>
#include <sys/statvfs.h>
#include <utime.h>
#endif//PLATFORM_POSIX

NAMESPACE_UPP


VfsPath ValVfs(Value v) {
	VfsPath vfs;
	vfs.Add(v);
	return vfs;
}

VfsPath StrVfs(String s) {
	VfsPath vfs;
	vfs.Set(s);
	return vfs;
}




VfsPath::VfsPath() {
	
}

/* Too ambiguous: use ValVfs & StrVfs
VfsPath::VfsPath(const String& s) {
	Set(s);
}*/

VfsPath::VfsPath(const VfsPath& path) {
	str = path.str;
	parts <<= path.parts;
}

VfsPath::VfsPath(const Vector<Value>& path) {
	parts <<= path;
	StrFromParts();
}

VfsPath::VfsPath(VfsPath&& path) {
	str = pick(path.str);
	parts = pick(path.parts);
}

void VfsPath::Visit(Vis& v) {
	v.Ver(1)
	(1)	("str", str)
		("parts", parts);
}

VfsPath& VfsPath::operator=(const VfsPath& path) {
	str = path.str;
	if (path.parts.IsEmpty())
		parts.Clear();
	else
		parts <<= path.parts;
	return *this;
}

VfsPath& VfsPath::Add(Value part) {
	parts.Add(part);
	StrFromParts();
	return *this;
}

void VfsPath::Set(String path) {
	this->str = path;
	
	#if INTERNAL_POSIX
	if (this->str.IsEmpty())
		str = "/";
	#endif
	
	Vector<String> str_parts = Split(path , INTERNAL_SEPS);
	
	#if INTERNAL_COMB || INTERNAL_CPM
	if (str_parts.GetCount()) {
		auto& p = str_parts[0];
		int c = p.GetCount();
		if (c && p[c-1] == ':')
			p = p.Left(c-1);
	}
	#endif
	
	parts.Clear();
	for(auto& p : str_parts)
		parts.Add(p);
}

void VfsPath::Set(const Vector<Value>& parts) {
	this->parts <<= parts;
	StrFromParts();
}

void VfsPath::StrFromParts() {
	String s;
	#if INTERNAL_POSIX
	if (parts.IsEmpty())
		s = "/";
	else
		for (const String& p: parts)
			s << "/" << p;
	#elif INTERNAL_COMB
	int c = parts.GetCount();
	if (c > 0) {
		s << parts[0] << ":";
		for(int i = 1; i < c; i++)
			s << "/" << parts[i];
	}
	#else
	int c = parts.GetCount();
	if (c > 0) {
		s << parts[0] << ":";
		for(int i = 1; i < c; i++)
			s << "\\" << parts[i];
	}
	#endif
	str = s;
}

void VfsPath::Set(const VfsPath& path, int begin, int end) {
	parts.Clear();
	for(int i = begin; i < path.parts.GetCount() && i < end; i++) {
		parts << path.parts[i];
	}
	StrFromParts();
}

bool VfsPath::IsLeft(const VfsPath& path) const {
	if (path.parts.GetCount() > parts.GetCount())
		return false;
	int c = path.parts.GetCount();
	for(int i = 0; i < c; i++)
		if (parts[i] != path.parts[i])
			return false;
	return true;
}

bool VfsPath::IsSame(const VfsPath& path, int this_begin, int other_begin, int len) const {
	ASSERT(this->parts.GetCount() >= this_begin + len);
	if (path.parts.GetCount() < other_begin + len)
		return false;
	for(int i = 0; i < len; i++) {
		int a = this_begin + i;
		int b = other_begin + i;
		if (this->parts[a] != path.parts[b])
			return false;
	}
	return true;
}

String VfsPath::AsSysPath(const char *root) const {
	if (!parts.GetCount())
		return root ? String(root) : String();
	String s;
	#ifdef flagWIN32
	if (parts.GetCount())
		s << parts[0] << ":";
	for(int i = 1; i < parts.GetCount(); i++)
		s << "\\" << parts[i];
	#else
	for(int i = 0; i < parts.GetCount(); i++)
		s << "/" << parts[i];
	#endif
	return s;
}

const String& VfsPath::Get() const {return str;}
const Vector<Value>& VfsPath::Parts() const {return parts;}
VfsPath::operator String() const {return str;}

int VfsPath::GetPartCount() const {
	return parts.GetCount();
}

bool VfsPath::IsEmpty() const {
	return parts.IsEmpty();
}

String VfsPath::ToString() const {
	return str;
}

Value VfsPath::TopPart() const {
	if (parts.IsEmpty())
		return Value();
	else
		return parts.Top();
}

String VfsPath::GetFilename() const {
	if (parts.IsEmpty())
		return String();
	else
		return parts.Top();
}

VfsPath VfsPath::GetCanonical() const {
	VfsPath p(*this);
	p.Normalize();
	return p;
}

VfsPath VfsPath::Left(int i) const {
	VfsPath p(*this);
	int c = max(0, min(i, p.GetPartCount()));
	p.parts.SetCount(c);
	p.StrFromParts();
	return p;
}

bool VfsPath::Normalize() {
	bool any_changes = false;
	while (1) {
		bool changes = false;
		for(int i = 0; i < parts.GetCount(); i++) {
			Value& p = parts[i];
			if (p.Is<String>()) {
				String s = p;
				if (s == "..") {
					if (!i) {
						str.Clear();
						parts.Clear();
						return false;
					}
					parts.Remove(i-1,2);
					changes = true;
				}
			}
		}
		if (!changes) break;
		any_changes = true;
	}
	if (any_changes)
		StrFromParts();
	return true;
}

bool VfsPath::IsValid() const {
	if (parts.IsEmpty()) return false;
	for (const auto& p : parts)
		if (p.ToString().IsEmpty())
			return false;
	return true;
}

bool VfsPath::IsValidFullPath() const {
	return IsFullInternalDirectory(str);
}

void VfsPath::Append(const VfsPath& p) {
	parts.Append(p.parts);
	StrFromParts();
}

void VfsPath::RemoveLast() {
	parts.SetCount(parts.GetCount()-1);
}

bool VfsPath::operator==(const VfsPath& p) const {
	return this->str == p.str;
}

bool VfsPath::operator!=(const VfsPath& p) const {
	return this->str != p.str;
}

bool VfsPath::operator==(const String& s) const {
	return this->str == s;
}

hash_t VfsPath::GetHashValue() const {
	CombineHash ch;
	for (const auto& p : parts)
		ch.Do(p);
	return ch;
}

bool VfsPath::IsSysDirectory() const {
	return ::UPP::DirectoryExists(str);
}

bool VfsPath::IsSysFile() const {
	return ::UPP::FileExists(str);
}


VfsPath operator+(const char* s, const VfsPath& vfs) {
	VfsPath path;
	path.Add(String(s));
	path.Append(vfs);
	return path;
}

VfsPath operator+(const VfsPath& vfs, const char* s) {
	VfsPath path(vfs);
	path.Add(String(s));
	return path;
}

void VfsPath::Set(Value s, const VfsPath& vfs) {
	parts.Clear();
	parts.Add(s);
	parts.Append(vfs.parts);
	StrFromParts();
}

void VfsPath::SetDotPath(String s) {
	Vector<String> v = Split(s, ".");
	parts.SetCount(v.GetCount());
	for(int i = 0; i < v.GetCount(); i++)
		parts[i] = v[i];
	StrFromParts();
}

void VfsPath::SetPosixPath(String s) {
	for(int i = 0; i < s.GetCount(); i++) {
		char c = s[i];
		if (IsSpace(c)) continue;
		if (c == '/') {s = s.Mid(i+1); break;}
		else break;
	}
	Vector<String> v = Split(s, "/", false);
	parts.SetCount(v.GetCount());
	for(int i = 0; i < v.GetCount(); i++)
		parts[i] = v[i];
	StrFromParts();
}

void VfsPath::Remove(int i) {
	parts.Remove(i);
	StrFromParts();
}

VfsPath operator+(Value s, const VfsPath& vfs) {
	VfsPath p;
	p.Set(s, vfs);
	return p;
}

VfsPath operator+(const VfsPath& vfs, Value s) {
	VfsPath p(vfs);
	p.Add(s);
	return p;
}

VfsPath operator/(const VfsPath& a, const VfsPath& b) {
	VfsPath p(a);
	p.Append(b);
	return p;
}

bool IsFullInternalDirectory(const String& path) {
	#if defined flagINTERNAL_POSIX
	return path.GetCount() > 0 && path[0] == '/';
	#else
	if (path.IsEmpty())
		return true;
	for(int i = 0; i < path.GetCount(); i++) {
		int chr = path[i];
		if (chr == ':')
			return true;
		else if (chr == '/' || chr == '\\')
			return false;
	}
	return false;
	#endif
}

String AppendInternalFileName(const String& a, const String& b) {
	#if INTERNAL_POSIX || INTERNAL_COMB
	return AppendUnixFileName(a, b);
	#else
	return AppendCpmFileName(a, b);
	#endif
}

String NormalizeInternalPath(const String& path) {
	VfsPath vfs;
	vfs.Set(path);
	vfs.Normalize();
	return vfs;
}

Value FindValuePath(Value v, const VfsPath& p) {
	if (p.IsEmpty())
		return v;
	for(auto& part : p.Parts()) {
		if (!v.Is<ValueMap>())
			return Value();
		ValueMap m = v;
		int i = m.Find(part);
		if (i < 0)
			return Value();
		v = m.GetValue(i);
	}
	return v;
}







bool SystemFS::GetFiles(const VfsPath& rel_path, Vector<Shared<VfsItem>>& items) {
	// An empty relative path is the mount root itself, which has no sys path of its
	// own: the host root stands in for it.
	String sys_path = rel_path.AsSysPath("/");
	if (!DirectoryExists(sys_path)) {
		last_error = "no directory '" + sys_path + "'";
		return false;
	}
	FindFile ff;
	if (ff.Search(AppendFileName(sys_path, "*"))) do {
		String name = ff.GetName();
		if (name == "." || name == "..") continue;
		// The listing already paid for the stat() (FindFile::Stat caches it), so the
		// metadata below is a read of a struct in hand, not another syscall per
		// entry. The fields guarded away on a platform do not exist there: Win32's
		// FindFile has no mode word, uid or gid, and their defaults stay.
		SystemVfsItem& item = items.Add().Create<SystemVfsItem>();
		item.name = name;
		if (ff.IsDirectory())
			item.type = VFS_DIRECTORY;
		else if (ff.IsSymLink())
			item.type = VFS_SYMLINK;
		else
			item.type = VFS_FILE;
		item.type_str = GetFileExt(name);
		if (item.type_str.GetCount() && item.type_str[0] == '.')
			item.type_str = item.type_str.Mid(1);
		item.length    = ff.GetLength();
		item.modified  = ff.GetLastWriteTime();
		item.accessed  = ff.GetLastAccessTime();
		item.read_only = ff.IsReadOnly();
		#ifdef PLATFORM_POSIX
		item.created   = ff.GetLastChangeTime();
		item.unix_mode = ff.GetMode();
		item.uid       = ff.GetUid();
		item.gid       = ff.GetGid();
		#endif
		#ifdef PLATFORM_WIN32
		item.created   = ff.GetCreationTime();
		#endif
		if (ff.IsSymLink())
			item.link = GetSymLinkPath(AppendFileName(sys_path, name));
	}
	while (ff.Next());
	return true;
}

VfsItemType SystemFS::CheckItem(const VfsPath& rel_path) {
	String sys_path = rel_path.AsSysPath("/");
	if (DirectoryExists(sys_path))
		return VFS_DIRECTORY;
	else if (FileExists(sys_path))
		return VFS_FILE;
	else
		return VFS_NULL;
}

// ---------------------------------------------------------------------------
// Mutation
//
// These run only after the caller's own authorization policy has cleared the
// operation. The host filesystem imposes no opinion of its own beyond what the
// OS already enforces, which is why each one reports failure through last_error
// instead of asserting: a read-only or permission-denied result is an ordinary
// outcome that the caller has to be able to show the user.
// ---------------------------------------------------------------------------

//< Refuses with a reason, so a read-only VFS says why rather than looking broken.
static bool Refuse(VFS& vfs, const char *op, const VfsPath& p) {
	vfs.last_error = "this filesystem does not support " + String(op) + " of '" +
	                 p.AsSysPath("/") + "'";
	return false;
}

//< "Is there anything at this host path", file or directory. CheckItem() speaks
//< VfsPath and mount-relative coordinates; these operations work in sys paths once
//< they have left the mount, so they need the host-level question directly.
static bool Exists2(const String& sys_path) {
	return DirectoryExists(sys_path) || FileExists(sys_path);
}


bool VFS::CreateDir(const VfsPath& rel_path)  { return Refuse(*this, "creating a directory", rel_path); }
	bool VFS::CreateFile(const VfsPath& rel_path) { return Refuse(*this, "creating a file", rel_path); }
	bool VFS::CreateSymlink(const VfsPath& rel_path, const VfsPath&) {
		return Refuse(*this, "creating a symbolic link", rel_path);
	}
bool VFS::Remove(const VfsPath& rel_path, bool)  { return Refuse(*this, "removing", rel_path); }
bool VFS::Rename(const VfsPath& rel_from, const VfsPath&) { return Refuse(*this, "renaming", rel_from); }
bool VFS::Transfer(const VfsPath& rel_from, const VfsPath&, bool) {
	return Refuse(*this, "copying or moving", rel_from);
}

bool VFS::GetDiskSpace(const VfsPath&, int64&, int64&) {
	last_error = "this filesystem does not report disk space";
	return false;
}

bool SystemFS::GetDiskSpace(const VfsPath& rel_path, int64& total, int64& free_space)
{
	// Asked for the *directory*, not the mount root: a mount can cover one branch
	// of a device, and the whole point is to report the device that actually holds
	// what the user is looking at.
	String sys_path = rel_path.AsSysPath("/");
	if(sys_path.IsEmpty()) sys_path = "/";
	total = free_space = 0;
#if defined(PLATFORM_POSIX)
	struct statvfs buf;
	if(statvfs(ToSystemCharset(sys_path), &buf) != 0) {
		last_error = "cannot read disk space for '" + sys_path + "'";
		return false;
	}
	// f_frsize is the fragment size the block count is expressed in; f_bsize is only
	// the preferred I/O size and using it overstates the filesystem on some mounts.
	int64 unit = buf.f_frsize ? (int64)buf.f_frsize : (int64)buf.f_bsize;
	if(unit <= 0) {
		last_error = "filesystem reports a zero block size for '" + sys_path + "'";
		return false;
	}
	total = (int64)buf.f_blocks * unit;
	free_space = (int64)buf.f_bavail * unit;
	// A filesystem with no blocks at all -- procfs, sysfs, some network mounts --
	// has answered, but the answer is not a number a user can act on. Reporting
	// "0 B free of 0 B" reads as a full disk, which is the opposite of the truth,
	// so this counts as not knowing.
	if(total <= 0) {
		last_error = "filesystem at '" + sys_path + "' has no measurable size";
		total = free_space = 0;
		return false;
	}
#elif defined(PLATFORM_WIN32)
	ULARGE_INTEGER avail, total_bytes, free_bytes;
	if(!GetDiskFreeSpaceExW(ToSystemCharsetW(sys_path), &avail, &total_bytes, &free_bytes)) {
		last_error = "cannot read disk space for '" + sys_path + "'";
		return false;
	}
	// Bytes not available to this user: on Windows the "free for me" figure is the
	// meaningful one, and quoting the unfiltered total would hide a quota.
	total = (int64)total_bytes.QuadPart;
	free_space = (int64)avail.QuadPart;
#else
	last_error = "disk space is not supported on this platform";
	return false;
#endif
	return true;
}

bool SystemFS::CreateDir(const VfsPath& rel_path) {
	if (!caps.Has(MountCapabilities::CREATE_DIR))
		return Refuse(*this, "creating a directory", rel_path);
	String sys_path = rel_path.AsSysPath("/");
	if (DirectoryExists(sys_path)) {
		last_error = "directory already exists: " + sys_path;
		return false;
	}
	// RealizeDirectory, not RealizePath: in this fork RealizePath() realizes the
	// *parent* of the path it is given (Path.cpp: RealizePath -> RealizeDirectory
	// (GetFileFolder(file))), so passing a directory to it creates only the parent
	// and then reports success while the directory itself is still absent.
	// RealizeDirectory also creates the whole missing chain, which is what a user
	// means by "make a folder here".
	if (!RealizeDirectory(sys_path)) {
		last_error = "cannot create directory: " + sys_path;
		return false;
	}
	return true;
}

bool SystemFS::CreateFile(const VfsPath& rel_path) {
	if (!caps.Has(MountCapabilities::CREATE_FILE))
		return Refuse(*this, "creating a file", rel_path);
	String sys_path = rel_path.AsSysPath("/");
	String dir = GetFileFolder(sys_path);
	if (!dir.IsEmpty() && !DirectoryExists(dir)) {
		last_error = "no such directory: " + dir;
		return false;
	}
	// FileOut truncates an existing file, which is exactly create-or-empties. An
	// existing file is not an error: "touch" style creation is idempotent.
	FileOut out;
	if (!out.Open(sys_path)) {
		last_error = "cannot create file: " + sys_path;
		return false;
	}
	return true;
}

bool SystemFS::CreateSymlink(const VfsPath& rel_path, const VfsPath& target) {
	if (!caps.Has(MountCapabilities::CREATE_SYMLINK))
		return Refuse(*this, "creating a symbolic link", rel_path);
	String sys_path = rel_path.AsSysPath("/");
	// Symlink creation uses symlink(2) on POSIX systems, or the equivalent on
	// Windows. The target path is relative to the mount point. For now, we
	// conservatively refuse symbolic link creation on the default filesystem.
	last_error = "symbolic links are not supported on this filesystem";
	return false;
}

bool SystemFS::Remove(const VfsPath& rel_path, bool recursive) {
	String sys_path = rel_path.AsSysPath("/");
	if (DirectoryExists(sys_path)) {
		if (!recursive) {
			// Refuse rather than recurse on a bare remove(): an accidental recursive
			// flag must never be able to delete a populated tree unattended.
			if (!DirectoryDelete(sys_path)) {
				last_error = "directory is not empty: " + sys_path;
				return false;
			}
			return true;
		}
		String error;
		if (!DeleteTree(sys_path, false, error)) {
			last_error = error;
			return false;
		}
		return true;
	}
	if (!FileExists(sys_path)) {
		last_error = "no such file: " + sys_path;
		return false;
	}
	if (!FileDelete(sys_path)) {
		last_error = "cannot remove: " + sys_path;
		return false;
	}
	return true;
}

bool SystemFS::Rename(const VfsPath& rel_from, const VfsPath& rel_to) {
	String from_sys = rel_from.AsSysPath("/");
	String to_sys = rel_to.AsSysPath("/");
	if (!Exists2(from_sys)) {
		last_error = "no such file or directory: " + from_sys;
		return false;
	}
	if (Exists2(to_sys)) {
		last_error = "target already exists: " + to_sys;
		return false;
	}
	String parent = GetFileFolder(to_sys);
	if (!parent.IsEmpty() && !DirectoryExists(parent)) {
		last_error = "no such directory: " + parent;
		return false;
	}
	if (!FileMove(from_sys, to_sys)) {
		last_error = "cannot rename '" + from_sys + "' to '" + to_sys + "'";
		return false;
	}
	return true;
}

bool SystemFS::Transfer(const VfsPath& rel_from, const VfsPath& rel_to, bool move) {
	String from_sys = rel_from.AsSysPath("/");
	String to_sys = rel_to.AsSysPath("/");
	if (!Exists2(from_sys)) {
		last_error = "no such file or directory: " + from_sys;
		return false;
	}
	String to_parent = GetFileFolder(to_sys);
	if (!to_parent.IsEmpty() && !DirectoryExists(to_parent)) {
		last_error = "no such directory: " + to_parent;
		return false;
	}
	if (DirectoryExists(from_sys)) {
		if(Exists2(to_sys)) {
			last_error = "target already exists: " + to_sys;
			return false;
		}
		String error;
		if (!CopyTree(from_sys, to_sys, error)) {
			last_error = error;
			return false;
		}
		if (move && !DeleteTree(from_sys, false, error)) {
			last_error = error;
			return false;
		}
		return true;
	}
	if (Exists2(to_sys)) {
		last_error = "target already exists: " + to_sys;
		return false;
	}
// FileMove is a rename(2) underneath, so it fails across a filesystem boundary.
	// Falling back to copy-then-delete is what makes a cross-device move work at
	// all; the cost is that it is not atomic, which is why the log has to say so.
	if(move) {
		if(!FileMove(from_sys, to_sys)) {
			if(!FileCopy(from_sys, to_sys)) {
				last_error = "cannot move '" + from_sys + "' to '" + to_sys + "'";
				return false;
			}
			if(!FileDelete(from_sys)) {
				last_error = "copied to '" + to_sys +
				             "' but cannot remove the source '" + from_sys + "'";
				return false;
			}
		}
	}
	else if(!FileCopy(from_sys, to_sys)) {
		// A copy must never be attempted as a move first: FileMove succeeding would
		// consume the source and leave the caller believing it still had a copy.
		last_error = "cannot copy '" + from_sys + "' to '" + to_sys + "'";
		return false;
	}
	return true;
}

bool SystemFS::CopyTree(const String& src_sys, const String& dst_sys, String& error) {
	if (!DirectoryCreate(dst_sys)) {
		error = "cannot create directory: " + dst_sys;
		return false;
	}
	FindFile ff;
	if (!ff.Search(AppendFileName(src_sys, "*"))) {
		error = "cannot read directory: " + src_sys;
		return false;
	}
	do {
		String name = ff.GetName();
		if (name == "." || name == "..") continue;
		String src_child = AppendFileName(src_sys, name);
		String dst_child = AppendFileName(dst_sys, name);
		if (ff.IsDirectory()) {
			if (!CopyTree(src_child, dst_child, error)) return false;
		}
		else if (!FileCopy(src_child, dst_child)) {
			error = "cannot copy '" + src_child + "' to '" + dst_child + "'";
			return false;
		}
	}
	while (ff.Next());
	return true;
}

bool SystemFS::DeleteTree(const String& dir_sys, bool rdonly, String& error) {
	FindFile ff;
	if (!ff.Search(AppendFileName(dir_sys, "*"))) {
		error = "cannot read directory: " + dir_sys;
		return false;
	}
	do {
		String name = ff.GetName();
		if (name == "." || name == "..") continue;
		String child = AppendFileName(dir_sys, name);
		if (ff.IsDirectory()) {
			if (!DeleteTree(child, rdonly, error)) return false;
		}
		else if (rdonly || !FileDelete(child)) {
			error = "cannot remove: " + child;
			return false;
		}
	}
	while (ff.Next());
	if (!DirectoryDelete(dir_sys)) {
		error = "cannot remove directory: " + dir_sys;
		return false;
	}
	return true;
}






VfsItem::~VfsItem() {}
void VfsItem::operator=(Value v) {Set(v);}
Shared<VfsItem> VfsItem::operator()(Value key) {return Open(key);}
Value VfsItem::Get() {Panic("Not implemented"); return Value();}
Value VfsItem::Get(Value key) {Panic("Not implemented"); return Value();}
Value VfsItem::GetKey(int i) {Panic("Not implemented"); return Value();}
void VfsItem::Set(Value v) {Panic("Not implemented");}
void VfsItem::MapSet(Value k, Value v) {Panic("Not implemented");}
void VfsItem::RealizeMap() {Panic("Not implemented");}
Value VfsItem::At(int i) {Panic("Not implemented"); return Value();}
int VfsItem::GetCount() const {Panic("Not implemented"); return 0;}
Shared<VfsItem> VfsItem::Open(Value key) {Panic("Not implemented"); return Shared<VfsItem>();}


void ValueVfsItem::RealizeValue() const {
	// Try reading again
	if (!valid_read) {
		ASSERT(fs_smart_ptr);
		if (fs_smart_ptr) {
			valid_read = fs->Get(*fs->v, 0, path, value);
		}
	}
}

Value ValueVfsItem::Get() {
	RealizeValue();
	return value;
}

Value ValueVfsItem::Get(Value key) {
	RealizeValue();
	ASSERT(type == VFS_DIRECTORY);
	ValueMap map = value;
	return map(key);
}

Value ValueVfsItem::GetKey(int i) {
	RealizeValue();
	ASSERT(type == VFS_DIRECTORY);
	ValueMap map = value;
	return map.GetKey(i);
}

void ValueVfsItem::Set(Value v) {
	ASSERT(fs_smart_ptr);
	if (fs_smart_ptr)
		fs->Set(path, v);
}

void ValueVfsItem::MapSet(Value key, Value v) {
	if (type != VFS_DIRECTORY) {
		ASSERT(fs_smart_ptr);
		if (fs_smart_ptr) {
			fs->RealizeMap(*fs->v, 0, path);
			type = VFS_DIRECTORY;
		}
		else return;
	}
	ASSERT(fs_smart_ptr);
	if (fs_smart_ptr) {
		VfsPath p(path);
		p.Add(key);
		fs->Set(p, v);
		valid_read = false; // invalidate after modification
	}
}

void ValueVfsItem::RealizeMap() {
	ASSERT(fs_smart_ptr);
	if (fs_smart_ptr)
		fs->RealizeMap(*fs->v, 0, path);
}

Shared<VfsItem> ValueVfsItem::Open(Value key) {
	ASSERT(fs_smart_ptr);
	if (fs_smart_ptr) {
		VfsPath p(path);
		p.Add(key);
		return fs->Get(p);
	}
	return Shared<VfsItem>();
}

Value ValueVfsItem::At(int i) {
	RealizeValue();
	if (value.Is<ValueArray>()) {
		ValueArray arr = value;
		return arr.At(i);
	}
	else if (value.Is<ValueMap>()) {
		ValueMap map = value;
		return map.GetValue(i);
	}
	else return Value();
}

int ValueVfsItem::GetCount() const {
	RealizeValue();
	Value v;
	ASSERT(fs_smart_ptr);
	if (fs_smart_ptr)
		v = fs->GetValue(path);
	return v.GetCount();
}


ValueFS::ValueFS() {}
ValueFS::ValueFS(Value& v) : v(&v) {}
Value ValueFS::GetValue(const VfsPath& p) {
	Value out;
	Get(*v, 0, p, out);
	return out;
}
bool ValueFS::Set(const VfsPath& p, const Value& val) {
	return Set(*v, 0, p, val);
}
bool ValueFS::Set(Value& dir, int i, const VfsPath& p, const Value& val) {
	if (i < 0 || i > p.GetPartCount())
		return false;
	if (i == p.GetPartCount()) {
		dir = val;
	}
	else {
		const String& part = p.Parts()[i];
		if (!dir.Is<ValueMap>() && !dir.Is<ValueArray>()) {
			if (dir.IsVoid())
				dir = ValueMap();
			else
				return false;
		}
		if (dir.Is<ValueMap>()) {
			ValueMap map = dir;
			Value& subdir = map.GetAdd(part);
			if (!Set(subdir, i+1, p, val))
				return false;
			dir = map;
		}
		else {
			ASSERT(dir.Is<ValueArray>());
			ValueArray arr = dir;
			if (!IsAllDigit(part))
				return false;
			int i = ScanInt(part);
			if (i == arr.GetCount())
				arr.Add(Value());
			if (i < 0 || i >= arr.GetCount())
				return false;
			Value& subdir = arr.At(i);
			if (!Set(subdir, i+1, p, val))
				return false;
			dir = arr;
		}
	}
	return true;
}
bool ValueFS::Get(const Value& dir, int i, const VfsPath& p, Value& val) {
	if (i < 0 || i > p.GetPartCount())
		return false;
	if (i == p.GetPartCount()) {
		val = dir;
	}
	else {
		const String& part = p.Parts()[i];
		if (!dir.Is<ValueMap>() && !dir.Is<ValueArray>())
			return false;
		if (dir.Is<ValueMap>()) {
			ValueMap map;
			int i = map.Find(part);
			if (i < 0)
				return false;
			const Value& subdir = map.GetValue(i);
			if (!Get(subdir, i+1, p, val))
				return false;
		}
		else {
			ASSERT(dir.Is<ValueArray>());
			ValueArray arr;
			if (!IsAllDigit(part))
				return false;
			int i = ScanInt(part);
			if (i < 0 || i >= arr.GetCount())
				return false;
			const Value& subdir = arr.Get(i);
			if (!Get(subdir, i+1, p, val))
				return false;
		}
	}
	return true;
}
void ValueFS::RealizeMap(Value& dir, int i, const VfsPath& p) {
	if (i < 0 || i > p.GetPartCount())
		return;
	if (i == p.GetPartCount()) {
		if (!dir.Is<ValueMap>())
			dir = ValueMap();
	}
	else {
		const String& part = p.Parts()[i];
		if (!dir.Is<ValueMap>() && !dir.Is<ValueArray>())
			dir = ValueMap();
		if (dir.Is<ValueArray>() && !IsAllDigit(part))
			dir = ValueMap(dir);
		if (dir.Is<ValueMap>()) {
			ValueMap map = dir;
			Value& subdir = map.GetAdd(part);
			RealizeMap(subdir, i+1, p);
			dir = map;
		}
		else {
			ASSERT(dir.Is<ValueArray>());
			ValueArray arr = dir;
			int i = ScanInt(part);
			if (i == arr.GetCount())
				arr.Add(Value());
			if (i < 0 || i >= arr.GetCount()) {
				ValueMap map = dir;
				Value& subdir = map.GetAdd(part);
				RealizeMap(subdir, i+1, p);
				dir = map;
			}
			else {
				Value& subdir = arr.At(i);
				RealizeMap(subdir, i+1, p);
				dir = arr;
			}
		}
	}
}
Shared<VfsItem> ValueFS::Get(const VfsPath& p) {
	Shared<VfsItem> o;
	ValueVfsItem& it = o.Create<ValueVfsItem>();
	it.fs = this;
	it.fs_smart_ptr = this;
	it.type = v->Is<ValueMap>() || v->Is<ValueArray>() ? VFS_DIRECTORY : VFS_FILE;
	it.path = p;
	it.valid_read = Get(*v, 0, it.path, it.value);
	ASSERT(it.valid_read || it.value.IsNull());
	return o;
}
Shared<VfsItem> ValueFS::operator()(String s) {
	Shared<VfsItem> o;
	ValueVfsItem& it = o.Create<ValueVfsItem>();
	it.fs = this;
	it.fs_smart_ptr = this;
	it.type = v->Is<ValueMap>() || v->Is<ValueArray>() ? VFS_DIRECTORY : VFS_FILE;
	it.path.Set(s);
	it.valid_read = Get(*v, 0, it.path, it.value);
	ASSERT(it.valid_read || it.value.IsNull());
	return o;
}
Shared<VfsItem> ValueFS::RealizeMap(const VfsPath& p) {
	Shared<VfsItem> o;
	ValueVfsItem& it = o.Create<ValueVfsItem>();
	it.fs = this;
	it.fs_smart_ptr = this;
	it.type = VFS_DIRECTORY;
	it.path = p;
	RealizeMap(*v, 0, it.path);
	return o;
}
bool ValueFS::GetFiles(const VfsPath& rel_path, Vector<Shared<VfsItem>>& items)  {Panic("Not implemented"); return false;}
VfsItemType ValueFS::CheckItem(const VfsPath& rel_path)  {Panic("Not implemented"); return VFS_NULL;}


END_UPP_NAMESPACE
