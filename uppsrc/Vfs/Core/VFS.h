#ifndef _Core_VfsBase_VFS_h_
#define _Core_VfsBase_VFS_h_

NAMESPACE_UPP

/*
	Internal path notation (default is INTERNAL_COMB):
		INTERNAL_POSIX:		e.g. /usr/local/bin/bash
		INTERNAL_CPM:		e.g. C:\GAMES\ACROSS
		INTERNAL_COMB:		e.g. C:/Apps/Upp
*/
#if defined flagINTERNAL_POSIX
	#define INTERNAL_POSIX 1
	#define INTERNAL_CPM   0
	#define INTERNAL_COMB  0
	#define INTERNAL_ROOT_PATH "/"
	#define INTERNAL_SEPS "/"
	#define INTERNAL_SEP '/'
	#define INTERNAL_ROOT_FILE(x) "/" x
#elif defined flagINTERNAL_CPM
	#define INTERNAL_POSIX 0
	#define INTERNAL_CPM   1
	#define INTERNAL_COMB  0
	#define INTERNAL_ROOT_PATH ""
	#define INTERNAL_SEPS "\\"
	#define INTERNAL_SEP '\\'
	#define INTERNAL_ROOT_FILE(x) x ":"
#else
	#define INTERNAL_POSIX 0
	#define INTERNAL_CPM   0
	#define INTERNAL_COMB  1
	#define INTERNAL_ROOT_PATH ""
	#define INTERNAL_SEPS "/" // the default is "/" and not "\" because it's easier to type for the original maintainer (who uses the Finnish/Swedish keyboard layout)
	#define INTERNAL_SEP '/'
	#define INTERNAL_ROOT_FILE(x) x ":"
#endif

struct Visitor;

struct VfsPath : Moveable<VfsPath> {
	VfsPath();
	//VfsPath(const Value& s);
	VfsPath(const VfsPath& path);
	VfsPath(const Vector<Value>& path);
	VfsPath(VfsPath&& path);
	VfsPath& operator=(const VfsPath& path);
	VfsPath& Add(Value part);
	void	Set(String path);
	void	Set(Value s, const VfsPath& vfs);
	void	Set(const Vector<Value>& parts);
	void	Set(const VfsPath& path, int begin, int end);
	void	SetDotPath(String path);
	void	SetPosixPath(String path);
	void    Remove(int i);
	bool	IsLeft(const VfsPath& path) const;
	bool	IsSame(const VfsPath& path, int this_begin, int other_begin, int len) const;
	String	AsSysPath(const char *root = nullptr) const;
	int		GetPartCount() const;
	bool	IsEmpty() const;
	Value	TopPart() const;
	String  GetFilename() const;
	String	ToString() const;
	VfsPath GetCanonical() const;
	VfsPath Left(int i) const;
	bool	Normalize();
	bool	IsValid() const;
	bool	IsValidFullPath() const;
	void	Append(const VfsPath& p);
	const String& Get() const;
	const Vector<Value>& Parts() const;
	operator String() const;
	void    RemoveLast();
	hash_t  GetHashValue() const;
	bool operator==(const VfsPath& p) const;
	bool operator!=(const VfsPath& p) const;
	bool operator==(const String& p) const;
	void    Visit(Visitor& v);
	bool    IsSysDirectory() const;
	bool    IsSysFile() const;
private:
	String			str;
	Vector<Value>	parts;
	void	StrFromParts();
};

VfsPath ValVfs(Value v);
VfsPath StrVfs(String s);

VfsPath operator+(const char* s, const VfsPath& vfs);
VfsPath operator+(Value s, const VfsPath& vfs);
VfsPath operator+(const VfsPath& vfs, const char* s);
VfsPath operator+(const VfsPath& vfs, Value s);
VfsPath operator/(const VfsPath& a, const VfsPath& b);
bool IsFullInternalDirectory(const String& path);
String AppendInternalFileName(const String& a, const String& b);
String NormalizeInternalPath(const String& path);
Value FindValuePath(Value v, const VfsPath& p);

typedef enum : byte {
	VFS_NULL,
	VFS_DIRECTORY,
	VFS_FILE,
	VFS_SYMLINK
} VfsItemType;

struct VfsItem : Moveable<VfsItem> {
	String name;
	String type_str;
	VfsItemType type = VFS_NULL;
	virtual ~VfsItem();
	void operator=(Value v);
	Shared<VfsItem> operator()(Value key);
	virtual Value Get();
	virtual Value Get(Value key);
	virtual Value GetKey(int i);
	virtual void Set(Value v);
	virtual void MapSet(Value key, Value v);
	virtual void RealizeMap();
	virtual Value At(int i);
	virtual int GetCount() const;
	virtual Shared<VfsItem> Open(Value key);
};

// Describes what a mounted VFS actually is and what may be done with it.
//
// MountManager is shared by several kinds of consumer (a plain file manager, a
// standalone virtual filesystem, an attachable asset debugger, a GUI widget
// hierarchy debugger), so a mount cannot be assumed to be a browsable
// filesystem. Callers select mounts by role through this metadata instead.
struct MountCapabilities {
	enum Role {
		FILESYSTEM,     //< browsable tree of files and directories
		ASSET_DEBUGGER, //< attachable asset store for inspection
		GUI_HIERARCHY,  //< inspectable GUI widget hierarchy
		UNKNOWN_ROLE
	};

	enum Flags {
		MUTABLE          = 1 << 0, //< mount accepts mutation requests
		CREATE_DIR       = 1 << 1, //< can create directories, system dirs included
		                          //< (always still subject to the caller's auth policy)
		CREATE_FILE      = 1 << 2, //< can create files
		CREATE_SYMLINK   = 1 << 3, //< can create symbolic links
		CASE_SENSITIVE   = 1 << 4, //< name comparison is case sensitive
		HAS_METADATA     = 1 << 5, //< items carry more than name and type
		SUPPORTS_OVERLAY = 1 << 6, //< may be shadowed by a deeper mount
	};

	Role   role = FILESYSTEM;
	int    flags = 0;
	String description;

	bool   Has(int f) const                        { return (flags & f) != 0; }
	void   Enable(int f)                           { flags |= f; }
	void   Disable(int f)                          { flags &= ~f; }
	bool   IsBrowsable() const                     { return role == FILESYSTEM; }
	bool   CanMutate() const                       { return Has(MUTABLE); }

	static const char *RoleName(Role r);
	//< Name of a single capability flag, for refusal messages. Unknown bits render
	//< as "flag <n>" rather than an empty string, so a typo is visible in a log.
	static const char *FlagName(int f);
	String ToString() const;
};

struct VFS : Pte<VFS> {
	VFS() {}
	virtual ~VFS() {}
	virtual bool GetFiles(const VfsPath& rel_path, Vector<Shared<VfsItem>>& items) = 0;
	virtual VfsItemType CheckItem(const VfsPath& rel_path) = 0;

	//< Mutation, in the same mount-relative coordinates as the two readers above.
	//<
	//< Every default implementation *refuses* and fills last_error, so a read-only
	//< VFS has to opt in deliberately: MountCapabilities::MUTABLE is the promise a
	//< VFS makes, and overriding these is how it keeps it. A VFS must also gate the
	//< individual operation behind the matching capability flag (CREATE_DIR,
	//< CREATE_FILE), because MUTABLE alone says "writable", not "writable anything".
	//<
	//< Authorization is deliberately absent here. The Vfs layer has no opinion about
	//< permissions and must not grow one; policy belongs to the caller, and it is
	//< supposed to have run *before* it gets this far. A mutation that reaches here
	//< unauthorized is a bug in the caller, not a gap in this interface.
	virtual bool CreateDir(const VfsPath& rel_path);
	virtual bool CreateFile(const VfsPath& rel_path);
	virtual bool CreateSymlink(const VfsPath& link_path, const VfsPath& target);
	//< Removes a file, or an *empty* directory. A non-empty directory is refused
	//< unless recursive is set, so a stray flag cannot silently eat a subtree.
	virtual bool Remove(const VfsPath& rel_path, bool recursive = false);
	virtual bool Rename(const VfsPath& rel_from, const VfsPath& rel_to);
	//< Copies when move is false, moves the entry when it is true. A move that has
	//< to cross a filesystem boundary falls back to copy-then-remove, since the
	//< rename(2) underneath will refuse that case.
	virtual bool Transfer(const VfsPath& rel_from, const VfsPath& rel_to, bool move);

	//< Total and free bytes of the filesystem holding rel_path. False when this VFS
	//< cannot answer, which is the ordinary case for an AST-backed tree: it has a
	//< shape but no notion of a device. Callers must show the unknown, never zero --
	//< "0 bytes free" and "no idea" are very different claims to a user looking at a
	//< full disk.
	virtual bool GetDiskSpace(const VfsPath& rel_path, int64& total, int64& free_space);

	//< Declares what this VFS is and what it allows. Defaults to a browsable,
	//< non-mutable filesystem, which is what every VFS has been so far.
	MountCapabilities&       Caps()             { return caps; }
	const MountCapabilities& Caps() const       { return caps; }
	void                    SetRole(MountCapabilities::Role r) { caps.role = r; }
	bool                    IsBrowsable() const { return caps.IsBrowsable(); }

	String last_error;

protected:
	MountCapabilities caps;
};

//< The item SystemFS produces: VfsItem's three fields, plus what the directory
//< walk already knows because the listing paid for its stat() anyway.
//<
//< The type itself is the "the mount knows" signal: a VFS with nothing to say
//< hands back plain VfsItem and the consumer's dynamic_cast yields null. No
//< sentinel per field, no validity flag, nothing to keep in sync. Unknown
//< *values* are still expressed per field, because the platforms differ about
//< what exists at all: uid/gid/mode are POSIX facts (Win32 has none, so they
//< keep their defaults there), while sizes, timestamps, read-only and the
//< symlink target are portable. "Created" on POSIX is st_ctime, the inode
//< change time -- FileSystemInfo::FileInfo (Core/Path.cpp) makes the same
//< substitution, and both are honest about the platform they run on rather
//< than synthesizing a creation date the OS does not keep.
//<
//< These are *facts*, not policy: whether a caller may write here is the
//< caller's authorization decision, so there is deliberately no CanWrite().
struct SystemVfsItem : VfsItem {
	int64  length = -1;
	Time   modified, accessed, created;
	bool   read_only = false;
	dword  unix_mode = 0;    //< st_mode; 0 when the platform has no mode word
	int    uid = -1;         //< -1, not 0: root is 0, and 0 must not read as unknown
	int    gid = -1;
	String link;             //< symlink target, empty when not a symlink
};

struct SystemFS : VFS {
	SystemFS() {
		// The host filesystem is the one VFS that is writable by definition.
		// Creating system directories is included; the caller's authorization
		// policy still gates it.
		caps.Enable(MountCapabilities::MUTABLE);
		caps.Enable(MountCapabilities::CREATE_DIR);
		caps.Enable(MountCapabilities::CREATE_FILE);
		caps.Enable(MountCapabilities::SUPPORTS_OVERLAY);
		caps.description = "host filesystem";
	}
	bool GetFiles(const VfsPath& rel_path, Vector<Shared<VfsItem>>& items) override;
	VfsItemType CheckItem(const VfsPath& rel_path) override;
	bool CreateDir(const VfsPath& rel_path) override;
	bool CreateFile(const VfsPath& rel_path) override;
	bool CreateSymlink(const VfsPath& link_path, const VfsPath& target) override;
	bool Remove(const VfsPath& rel_path, bool recursive = false) override;
	bool Rename(const VfsPath& rel_from, const VfsPath& rel_to) override;
	bool Transfer(const VfsPath& rel_from, const VfsPath& rel_to, bool move) override;
	bool GetDiskSpace(const VfsPath& rel_path, int64& total, int64& free_space) override;

	//< Recursive copy, used by Transfer when the source is a directory. Kept as a
	//< member so a subtree can be staged and reported on as one unit.
	static bool CopyTree(const String& src_sys, const String& dst_sys, String& error);
	//< Fails when dir_sys is not empty and rdonly is false, which is what keeps a
	//< mistyped recursive flag from deleting a whole subtree by accident.
	static bool DeleteTree(const String& dir_sys, bool rdonly, String& error);
};

struct ValueFS;

struct ValueVfsItem : VfsItem {
	VfsPath path;
	mutable Value value;
	mutable bool valid_read = false;
	ValueFS* fs = 0;
	Ptr<VFS> fs_smart_ptr = 0; // for checking if 'fs' is usable
	
	Value Get() override;
	Value Get(Value key) override;
	Value GetKey(int i) override;
	void Set(Value v) override;
	void MapSet(Value key, Value v) override;
	void RealizeMap() override;
	Value At(int i) override;
	int GetCount() const override;
	void RealizeValue() const;
	Shared<VfsItem> Open(Value key) override;
};

struct ValueFS : VFS {
	ValueFS();
	ValueFS(Value& v);
	
	Value GetValue(const VfsPath& p);
	bool Set(const VfsPath& p, const Value& val);
	Shared<VfsItem> Get(const VfsPath& p);
	Shared<VfsItem> operator()(String);
	Shared<VfsItem> RealizeMap(const VfsPath& p);
	bool GetFiles(const VfsPath& rel_path, Vector<Shared<VfsItem>>& items) override;
	VfsItemType CheckItem(const VfsPath& rel_path) override;
	
protected:
	friend struct ValueVfsItem;
	Value* v = 0;
	
	bool Set(Value& dir, int i, const VfsPath& p, const Value& val);
	bool Get(const Value& dir, int i, const VfsPath& p, Value& val);
	void RealizeMap(Value& dir, int i, const VfsPath& p);
};

END_UPP_NAMESPACE

#endif
