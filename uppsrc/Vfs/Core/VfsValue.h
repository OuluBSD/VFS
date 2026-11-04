#ifndef _Vfs_Core_VfsValue_h_
#define _Vfs_Core_VfsValue_h_


struct VfsValue;
struct VfsValueExtCtrl;
struct Entity;
class Engine;
struct IdeShellHost;
struct IdeShell;
class ClangTypeResolver;
class ToolAppCtrl;
struct VfsValueSubset;


#define CLASSTYPE(x) \
	typedef x CLASSNAME; \
	static TypeCls AsTypeCls() {return typeid(x);} \
	TypeCls GetTypeCls() const override {return typeid(x);} \
	String GetTypeName() const override {return #x;} \
	hash_t GetTypeHash() const override {return TypedStringHasher<x>(#x);} \
	void* GetTypePtr() const override {return (void*)this;} \

#define DEFAULT_EXT(x) CLASSTYPE(x) x(VfsValue& n) : VfsValueExt(n) {}

#define INITIALIZER_VFSEXT(x, eon, cat) INITIALIZER(x) {VfsValueExtFactory::Register<x>(#x, VFSEXT_DEFAULT, eon, cat);}
#define INITIALIZER_COMPONENT(x, eon, cat) INITIALIZER(x) {VfsValueExtFactory::Register<x>(#x, VFSEXT_COMPONENT, eon, cat);}
#define INITIALIZER_COMPONENT_CTRL(comp,ctrl) INITIALIZER(ctrl) {VfsValueExtFactory::RegisterCtrl<comp,ctrl>(#ctrl);}

struct VfsValue : Pte<VfsValue> {
	String             id;
	hash_t             type_hash = 0;
	hash_t             serial = 0;
	hash_t             file_hash = 0;
	Array<VfsValue>    sub;
	Value              value;
	One<VfsValueExt>   ext;
	
	// Temp
	hash_t             pkg_hash = 0;
	Ptr<VfsValue>      owner;
	Ptr<VfsValue>      symbolic_link;
	
	#ifdef flagDEBUG
	bool               only_temporary = false;
	#endif
	
	#if DEBUG_METANODE_DTOR
	bool               trace_kill = false;
	#endif
	
	VfsValue() {}
	VfsValue(VfsValue* owner, const VfsValue& n) {Assign(owner, n);}
	~VfsValue();
	VfsValue& operator[](int i);
	void ClearExtDeep();
	void Destroy();
	void Assign(VfsValue* owner, const VfsValue& n);
	void CopyFrom(const VfsValue& n);
	void CopyFieldsFrom(const VfsValue& n, bool forced_downgrade=false);
	void CopySubFrom(const VfsValue& n);
	bool FindDifferences(const VfsValue& n, Vector<String>& diffs, int max_diffs=30) const;
	String ToString() const;
	
	// Functions to use when value is AstValue type.
	bool IsAstValue() const;
	int GetAstValueCount() const;
	String AstGetKindString() const;
	static String AstGetKindString(int i);
	VfsValue& AstGetAdd(String id, String type, int kind);
	VfsValue& AstAdd(int kind, String id=String());
	int AstFind(int kind, const String& id) const;
	Vector<VfsValue*> AstFindAllShallow(int kind);
	Vector<const VfsValue*> AstFindAllShallow(int kind) const;
	void AstFindAllDeep(int kind, Vector<VfsValue*>& out);
	void AstFindAllDeep(int kind, Vector<const VfsValue*>& out) const;
	void AstRemoveAllShallow(int kind);
	void AstRemoveAllDeep(int kind);
	hash_t AstGetSourceHash(bool* total_hash_diffs=0) const;
	String GetSourceHashDump(int indent=0, bool* total_hash_diffs=0) const;
	void AstFix();
	
	VfsValue* Resolve();
	VfsValue& Add(const VfsValue& n);
	VfsValue& Add(VfsValue* n);
	VfsValue& Add(String id=String(), hash_t h=0);
	VfsValue& Insert(int idx, String id=String(), hash_t h=0);
	VfsValue& Init(VfsValue& s, const String& id, hash_t h);
	VfsValue* Detach(VfsValue* n);
	VfsValue* Detach(int i);
	VfsValue& GetRoot();
	void SetCount(int i, hash_t type_hash=0);
	void Remove(VfsValue* n);
	void Remove(int i);
	String GetTreeString(int depth=0) const;
	String GetTypeString() const;
	int Find(const String& id) const;
	int Find(const String& id, hash_t type_hash) const;
	VfsValue* FindPath(const VfsPath& path);
	hash_t GetTotalHash() const;
	void Visit(Vis& v);
	void FixParent() {for (auto& s : sub) s.owner = this;}
	void PointPkgHashTo(VfsValueSubset& other, hash_t pkg);
	void PointPkgHashTo(VfsValueSubset& other, hash_t pkg, hash_t file);
	void CopyPkgHashTo(VfsValue& other, hash_t pkg) const;
	void CopyPkgHashTo(VfsValue& other, hash_t pkg, hash_t file) const;
	bool HasPkgHashDeep(hash_t pkg) const;
	bool HasPkgFileHashDeep(hash_t pkg, hash_t file) const;
	void SetPkgHashDeep(hash_t pkg);
	void SetFileHashDeep(hash_t pkg);
	void SetPkgFileHashDeep(hash_t pkg, hash_t file);
	void SetTempDeep();
	Vector<VfsValue*> FindAll(TypeCls type);
	Vector<VfsValue*> FindTypeAllShallow(hash_t type_hash);
	Vector<Ptr<VfsValue>> FindTypeAllShallowPtr(hash_t type_hash);
	VfsValue* FindDeep(TypeCls type);
	bool IsFieldsSame(const VfsValue& n) const;
	String GetBasesString() const;
	String GetNestString() const;
	bool OwnerRecursive(const VfsValue& n) const;
	bool ContainsDeep(const VfsValue& n) const;
	void GetTypeHashes(Index<hash_t>& type_hashes) const;
	void RealizeSerial();
	void FixSerialDeep();
	Vector<Ptr<VfsValueExt>> GetAllExtensions();
	VfsPath GetPath() const;
	void DeepChk();
	void Chk();
	bool IsOwnerDeep(VfsValueExt& n) const;
	void Jsonize(JsonIO& io);
	int GetCount() const;
	int GetDepth() const;
	int GetDepth(const VfsValue* limit) const;
	Vector<VfsValue*> FindAllShallow(hash_t type_hash);
	int GetCount(String id) const;
	VfsValue& GetAdd(String id);
	VfsValue& GetAdd(String id, hash_t type_hash);
	void StopDeep();
	void ClearDependenciesDeep();
	void UninitializeDeep();
	void FindFiles(VectorMap<hash_t,VectorMap<hash_t,int>>& pkgfiles) const;
	VirtualNode				RootPolyValue();
	VirtualNode				RootVfsValue(const VfsPath& path);
	
	operator AstValue&();
	operator AstValue*();
	operator const AstValue*() const;
	
	template <class T>
	bool IsTypeHash() const {
		return type_hash == AsTypeHash<T>();
	}
	
	template <class T>
	bool Is() const {
		hash_t h = AsTypeHash<T>();
		return type_hash == h && ext && ext->GetTypeHash() == h;
	}
	
	template <class T>
	T& CreateExt() {
		ext.Clear();
		T* o = new T(*this);
		ext = o;
		type_hash = AsTypeHash<T>();
		return *o;
	}
	
	VfsValueExt& CreateExt(hash_t type_hash);
	
	template <class T>
	T& GetAdd(String id="") {
		hash_t type_hash = AsTypeHash<T>();
		ASSERT(type_hash);
		for (auto& s : sub) {
			if (s.type_hash == type_hash && s.id == id) {
				ASSERT(s.ext);
				if (!s.ext)
					throw Exc("internal error: no ext");
				T* o = CastPtr<T>(&*s.ext);
				ASSERT(o);
				if (!o)
					throw Exc("internal error: empty pointer");
				ASSERT(s.owner == this); // additional check
				return *o;
			}
		}
		VfsValue& s = Add();
		s.owner = this;
		s.id = id;
		T* o = new T(s);
		s.ext = o;
		s.pkg_hash = pkg_hash;
		s.file_hash = file_hash;
		s.type_hash = type_hash;
		return *o;
	}
	
	template <class T>
	T& Add(String id="") {
		VfsValue& s = Add();
		s.owner = this;
		s.id = id;
		T* o = new T(s);
		s.ext = o;
		s.pkg_hash = pkg_hash;
		s.file_hash = file_hash;
		s.type_hash = AsTypeHash<T>();
		ASSERT(s.type_hash);
		return *o;
	}
	
	template <class T>
	Ptr<T> FindPath(const VfsPath& vfs) {
		VfsValue* n = this;
		for(const Value& key : vfs.Parts()) {
			String key_str = key.ToString();
			int i = n->Find(key_str);
			if (i >= 0)
				n = &n->sub[i];
			else
				break;
		}
		if (n && n->ext)
			return &*CastPtr<T>(&*n->ext);
		return 0;
	}
	
	template <class T>
	T* Find(String id) {
		hash_t type_hash = AsTypeHash<T>();
		for (auto& s : sub) {
			if (s.type_hash == type_hash && s.id == id) {
				T* o = dynamic_cast<T*>(&*s.ext);
				if (o) return o;
			}
		}
		return 0;
	}
	
	template <class T>
	int FindPos(String id) {
		hash_t type_hash = AsTypeHash<T>();
		int i = 0;
		for (auto& s : sub) {
			if (s.type_hash == type_hash && s.id == id)
				return i;
			i++;
		}
		return -1;
	}
	
	template <class T>
	T* Find() {
		hash_t type_hash = AsTypeHash<T>();
		for (auto& s : sub) {
			if (s.type_hash == type_hash) {
				T* o = dynamic_cast<T*>(&*s.ext);
				if (o) return o;
			}
		}
		return 0;
	}
	
	template <class T>
	T* FindCast() {
		for (auto& s : sub) {
			if (s.ext) {
				T* o = dynamic_cast<T*>(&*s.ext);
				if (o) return o;
			}
		}
		return 0;
	}
	
	template <class T>
	Vector<Ptr<T>> FindAll() {
		Vector<Ptr<T>> v;
		for (auto& s : sub) {
			if (s.ext) {
				T* o = dynamic_cast<T*>(&*s.ext);
				if (o) v.Add(o);
			}
		}
		return v;
	}
	
	template <class T>
	Vector<Ptr<VfsValue>> FindAllWith() {
		Vector<Ptr<VfsValue>> v;
		for (auto& s0 : sub) {
			for (auto& s1 : s0.sub) {
				if (s1.ext) {
					T* o = dynamic_cast<T*>(&*s1.ext);
					if (o) {
						v.Add(&s0);
						break;
					}
				}
			}
		}
		return v;
	}
	
	template <class T>
	Vector<Ptr<T>> FindAllWithT() {
		Vector<Ptr<T>> v;
		for (auto& s0 : sub) {
			for (auto& s1 : s0.sub) {
				if (s1.ext) {
					T* o = dynamic_cast<T*>(&*s1.ext);
					if (o) {
						v.Add(o);
						break;
					}
				}
			}
		}
		return v;
	}
	
	template <class T> T* FindExt() {
		if (!ext)
			return 0;
		return dynamic_cast<T*>(&*ext);
	}
	
	template <class T> T& GetExt() {
		return dynamic_cast<T&>(*ext);
	}
	
	template <class T> T* GetOwnerExt() const {
		return owner ? owner->ext ? CastPtr<T>(&*owner->ext) : 0 : 0;
	}
	
	VfsValue* FindOwnerNull(int max_depth=-1) const {
		VfsValue* n = owner;
		int d = 1;
		while (n && n->type_hash && (max_depth < 0 || d++ <= max_depth))
			n = n->owner;
		return n;
	}
	
	bool HasOwnerDeep(VfsValue& val, int max_depth=-1) const {
		VfsValue* n = owner;
		int d = 1;
		while (n && (max_depth < 0 || d++ <= max_depth)) {
			if (n == &val)
				return true;
			n = n->owner;
		}
		return n == &val;
	}
	
	template <class T> T* FindOwner(int max_depth=-1) const {
		TypeCls type = AsTypeCls<T>();
		VfsValue* n = owner;
		int d = 1;
		while (n && (max_depth < 0 || d++ <= max_depth)) {
			if (n->ext && n->ext->GetTypeCls() == type) {
				T* o = CastPtr<T>(&*n->ext);
				ASSERT(o);
				return o;
			}
			n = n->owner;
		}
		return 0;
	}
	
	template <class T> T* FindRoot(int max_depth=-1) {
		TypeCls type = AsTypeCls<T>();
		VfsValue* n = this;
		T* root = 0;
		int d = 1;
		while (n && (max_depth < 0 || d++ <= max_depth)) {
			if (n->ext && n->ext->GetTypeCls() == type) {
				T* o = CastPtr<T>(&*n->ext);
				ASSERT(o);
				root = o;
			}
			n = n->owner;
		}
		return root;
	}
	
	template <class T> T* FindOwnerRoot(int max_depth=-1) const {
		TypeCls type = AsTypeCls<T>();
		VfsValue* n = owner;
		T* root = 0;
		int d = 1;
		while (n && (max_depth < 0 || d++ <= max_depth)) {
			if (n->ext && n->ext->GetTypeCls() == type) {
				T* o = CastPtr<T>(&*n->ext);
				ASSERT(o);
				root = o;
			}
			n = n->owner;
		}
		return root;
	}
	
	template <class T> T* FindOwnerWith(String id, int max_depth=-1) const {
		hash_t type_hash = AsTypeHash<T>();
		VfsValue* n = owner;
		int d = 1;
		while (n && (max_depth < 0 || d++ <= max_depth)) {
			for (auto& s : n->sub) {
				if (s.id == id && s.type_hash == type_hash) {
					T* o = CastPtr<T>(&*s.ext);
					ASSERT(o);
					return o;
				}
			}
			n = n->owner;
		}
		return 0;
	}
	
	template <class T> T* FindOwnerWith(int max_depth=-1) const {
		hash_t type_hash = AsTypeHash<T>();
		VfsValue* n = owner;
		int d = 1;
		while (n && (max_depth < 0 || d++ <= max_depth)) {
			for (auto& s : n->sub) {
				if (s.type_hash == type_hash) {
					T* o = CastPtr<T>(&*s.ext);
					ASSERT(o);
					return o;
				}
			}
			n = n->owner;
		}
		return 0;
	}
	
	template <class T> T* FindOwnerWithPathAndCast(const String& path, int max_depth=-1) const {
		if (path.IsEmpty())
			return FindOwnerWith<T>(max_depth);
		Vector<String> raw_parts = Split(path, "/");
		Vector<String> parts;
		for (const String& part : raw_parts)
			if (!part.IsEmpty())
				parts.Add(part);
		if (parts.IsEmpty())
			return FindOwnerWith<T>(max_depth);
		VfsValue* n = owner;
		int d = 1;
		while (n && (max_depth < 0 || d++ <= max_depth)) {
			VfsValue* cur = n;
			bool matched = true;
			for (const String& part : parts) {
				int idx = cur->Find(part);
				if (idx < 0) {
					matched = false;
					break;
				}
				cur = &cur->sub[idx];
			}
			if (matched && cur->ext) {
				T* o = CastPtr<T>(&*cur->ext);
				if (o)
					return o;
			}
			n = n->owner;
		}
		return 0;
	}
	
	template <class T> T* FindOwnerWithCast(int max_depth=-1) const {
		TypeCls type = AsTypeCls<T>();
		VfsValue* n = owner;
		int d = 1;
		while (n && (max_depth < 0 || d++ <= max_depth)) {
			for (auto& s : n->sub) {
				if (s.ext) {
					T* o = CastPtr<T>(&*s.ext);
					if (o)
						return o;
				}
			}
			n = n->owner;
		}
		return 0;
	}

	// Helper to search deeper into sibling containers for new VFS tree structure
	template <class T> T* FindOwnerWithCastDeep(int max_depth=-1) const {
		TypeCls type = AsTypeCls<T>();
		VfsValue* n = owner;
		int d = 1;
		while (n && (max_depth < 0 || d++ <= max_depth)) {
			// First try direct children
			for (auto& s : n->sub) {
				if (s.ext) {
					T* o = CastPtr<T>(&*s.ext);
					if (o)
						return o;
				}
			}
			// Then try grandchildren (for sibling containers)
			for (auto& s : n->sub) {
				for (auto& ss : s.sub) {
					if (ss.ext) {
						T* o = CastPtr<T>(&*ss.ext);
						if (o)
							return o;
					}
				}
			}
			// Also search in sibling containers (owner's siblings and their children)
			if (n->owner) {
				for (auto& sibling : n->owner->sub) {
					// Skip if it's the same as current owner
					if (&sibling == n) continue;
					// Check sibling directly
					if (sibling.ext) {
						T* o = CastPtr<T>(&*sibling.ext);
						if (o) return o;
					}
					// Check sibling's children
					for (auto& sc : sibling.sub) {
						if (sc.ext) {
							T* o = CastPtr<T>(&*sc.ext);
							if (o) return o;
						}
					}
				}
			}
			n = n->owner;
		}
		return 0;
	}

	template <class T> T* FindOwnerRootWith(int max_depth=-1) const {
		TypeCls type = AsTypeCls<T>();
		VfsValue* n = owner;
		T* root = 0;
		int d = 1;
		while (n && (max_depth < 0 || d++ <= max_depth)) {
			for (auto& s : n->sub) {
				if (s.ext && s.ext->GetTypeCls() == type) {
					T* o = CastPtr<T>(&*s.ext);
					ASSERT(o);
					root = o;
				}
			}
			n = n->owner;
		}
		return root;
	}
	
	template <class T> Vector<Ptr<T>> FindAllDeep() {
		Vector<Ptr<T>> v;
		FindAllDeep0<T>(v);
		return v;
	}
	template <class T> void FindAllDeep0(Vector<Ptr<T>>& v) {
		for (auto& s : sub) {
			T* o = s.ext ? CastPtr<T>(&*s.ext) : 0;
			if (o)
				v.Add(o);
		}
		for (auto& s : sub) {
			s.FindAllDeep0<T>(v);
		}
	}
	template <class T> void RemoveAllDeep() {
		Vector<int> rmlist;
		int i = 0;
		for (auto& s : sub) {
			T* o = s.ext ? CastPtr<T>(&*s.ext) : 0;
			if (o)
				rmlist << i;
			else
				s.RemoveAllDeep<T>();
			i++;
		}
		if (!rmlist.IsEmpty())
			sub.Remove(rmlist);
	}
	template <class T> void RemoveAllShallow() {
		Vector<int> rmlist;
		int i = 0;
		for (auto& s : sub) {
			T* o = s.ext ? CastPtr<T>(&*s.ext) : 0;
			if (o)
				rmlist << i;
			i++;
		}
		if (!rmlist.IsEmpty())
			sub.Remove(rmlist);
	}
	
	
public:
	using Val = VfsValue;
	
	typedef typename Array<VfsValue>::Iterator Iterator;
	typedef typename Array<VfsValue>::ConstIterator ConstIterator;
	
	class IteratorDeep {
		Iterator begin;
		Vector<Val*> cur;
		Vector<int> pos;
	protected:
		friend struct VfsValue;
		IteratorDeep(Val* cur) {
			this->cur.Add(cur);
			pos.Add(0);
		}
		
		IteratorDeep(const Val* cur) {
			this->cur.Add((Val*)cur);
			pos.Add(0);
		}
		
	public:
		IteratorDeep(const IteratorDeep& it) {
			*this = it;
		}
		IteratorDeep& operator = (const IteratorDeep& it) {
			begin = it.begin;
			cur <<= it.cur;
			pos <<= it.pos;
			return *this;
		}
		int GetDepth() {return pos.GetCount();}
		int GetPos() {return pos[pos.GetCount()-1];}
		int64 GetCurrentAddr() {return (int64)cur[cur.GetCount() - 1];}
		
		bool IsEnd() const {return pos.GetCount() == 1 && pos[0] == 1;}
		bool operator == (IteratorDeep& it) {return GetCurrentAddr() == it.GetCurrentAddr();}
		bool operator != (IteratorDeep& it) {return !(*this == it);}
		void operator ++(int i) {
			#define LASTPOS pos[pos.GetCount()-1]
			#define LASTCUR cur[cur.GetCount()-1]
			#define SECLASTCUR cur[cur.GetCount()-2]
			#define ADDPOS pos[pos.GetCount()-1]++
			#define ADDCUR LASTCUR = &SECLASTCUR->sub[LASTPOS]
			#define REMLASTPOS pos.Remove(pos.GetCount()-1)
			#define REMLASTCUR cur.Remove(cur.GetCount()-1)
			//String s; for(int i = 0; i < pos.GetCount(); i++) s << " " << pos[i]; LOG("+++ " << s);
			
			if (pos.GetCount() == 1 && pos[0] < 0) {pos[0]++; return;}
			if (pos.GetCount() == 1 && pos[0] == 1) return; // at end
			
			if (LASTCUR->GetCount()) {
				pos.Add(0);
				cur.Add(&LASTCUR->sub[0]);
			}
			else if (pos.GetCount() == 1) {
				pos[0] = 1; // at end
			}
			else {
				while (1) {
					if (LASTPOS + 1 < SECLASTCUR->GetCount()) {
						ADDPOS;
						ADDCUR;
						break;
					} else {
						REMLASTPOS;
						REMLASTCUR;
						if (pos.GetCount() <= 1) {
							pos.SetCount(1);
							pos[0] = 1;
							break;
						}
					}
				}
			}
		}
		void IncNotDeep() {
			if (LASTCUR->GetCount()) {
				while (1) {
					if (cur.GetCount() >= 2 && LASTPOS + 1 < SECLASTCUR->GetCount()) {
						ADDPOS;
						ADDCUR;
						break;
					} else {
						REMLASTPOS;
						REMLASTCUR;
						if (pos.GetCount() <= 1) {
							pos.SetCount(1);
							pos[0] = 1;
							break;
						}
					}
				}
			}
			else operator++(1);
		}
		void DecToParent() {
			pos.Remove(pos.GetCount()-1);
			cur.Remove(cur.GetCount()-1);
		}
		
		operator Val*() {
			if (pos.GetCount() && pos[0] == 1) return 0;
			return LASTCUR;
		}
		
		Val* operator->() {
			if (pos.GetCount() && pos[0] == 1) return 0;
			return LASTCUR;
		}
		
		Val& operator*() {
			return *LASTCUR;
		}
		
		const Val& operator*() const {return *LASTCUR;}
		
		Val* Higher() {
			if (cur.GetCount() <= 1) return 0;
			return cur[cur.GetCount()-2];
		}
	};
	
	
	IteratorDeep		BeginDeep() { return IteratorDeep(this);}
	const IteratorDeep	BeginDeep() const { return IteratorDeep(this);}
	Iterator			Begin() { return sub.Begin(); }
	Iterator			End() { return sub.End(); }
	
public:
	template <class T>
	struct SubIterT {
		VfsValue& val;
		
		SubIterT(VfsValue& v) : val(v) {}
		SubIterT(SubIterT&& v) : val(v.val) {}
		int GetCount() const {
			hash_t type_hash = AsTypeHash<T>();
			int count = 0, i = -1;
			while (++i < val.sub.GetCount())
				if (val.sub[i].type_hash == type_hash)
					count++;
			return count;
		}
		bool IsEmpty() const {return GetCount() == 0;}
		
		struct Iterator {
			VfsValue& val;
			int pos = 0;
			Iterator(VfsValue& v, int i) : val(v), pos(i) {
				if (i < 0) {Inc();}
				else if (i == INT_MAX) {pos = val.sub.GetCount(); Dec();}
			}
			Iterator(Iterator&& v) : val(v.val), pos(v.pos) {}
			operator bool() const {return pos >= 0 && pos < val.sub.GetCount();}
			bool IsEnd() const {return pos < 0 && pos >= val.sub.GetCount();}
			void operator++() {Inc();}
			void operator++(int) {Inc();}
			void operator--() {Dec();}
			void operator--(int) {Dec();}
			void Inc() {
				hash_t type_hash = AsTypeHash<T>();
				while (++pos < val.sub.GetCount())
					if (val.sub[pos].type_hash == type_hash)
						break;
			}
			void Dec() {
				hash_t type_hash = AsTypeHash<T>();
				while (--pos >= 0)
					if (val.sub[pos].type_hash == type_hash)
						break;
			}
			operator T&() {return val.sub[pos].GetExt<T>();}
			T& operator*() {return val.sub[pos].GetExt <T>();}
		};
		Iterator begin() {return Iterator(val, -1);}
		Iterator end() {return Iterator(val, val.sub.GetCount());}
		Iterator rbegin() {return Iterator(val, INT_MAX);}
	};
	
	template <class T> SubIterT<T> Sub() {return SubIterT<T>(*this);}
};

using Val = VfsValue;
using ValPtr = Ptr<VfsValue>;
using VfsValuePtr = Ptr<VfsValue>;

template <class T>
inline T& VirtualNode::GetAddExt(String name) {
	ASSERT(data && data->mode == VFS_VALUE);
	ASSERT(data->vfs_value);
	return data->vfs_value->CreateExt<T>();
}

struct VfsValueSubset {
	Array<VfsValueSubset> sub;
	Ptr<VfsValue> n;
	
	VfsValueSubset() {}
	//VfsValueSubset(VfsValue& n) : n(&n) {}
	void Clear() {sub.Clear(); n = 0;}
};


typedef enum : byte {
	MERGEMODE_OVERWRITE_OLD,
	MERGEMODE_KEEP_OLD,
	MERGEMODE_UPDATE_SUBSET,
} MergeMode;

struct MetaEnvironment : VFS {
	struct FilePos : Moveable<FilePos> {
		Vector<Ptr<VfsValue>> hash_nodes;
	};
	struct Type : Moveable<Type> {
		Vector<Ptr<VfsValue>> hash_nodes;
		String seen_type;
	};
	VectorMap<hash_t,FilePos> filepos;
	VectorMap<hash_t,Type> types;
	hash_t serial_counter = 0;
	SpinLock serial_lock;
	WorldState env_ws;
	mutable Mutex seen_lock;
	
	// Track all distinct path names observed during workspace scanning
	// Use hash_t bit width (32 on 32-bit, 64 on 64-bit) for index keys
	static constexpr int PATH_HASH_BITS = (int)(sizeof(hash_t) * 8);

	struct StrHashAccessor {
		using Hash = hash_t;
		Hash operator()(const String& s) const { return (Hash)s.GetHashValue(); }
	};

	CritBitIndex<String, StrHashAccessor, PATH_HASH_BITS> seen_path_names;

	
	VfsValue root;
	RWMutex lock;
	
	typedef MetaEnvironment CLASSNAME;
	MetaEnvironment();
	hash_t NewSerial();
	hash_t CurrentSerial() const {return serial_counter;}
	//void Store(const String& includes, const String& path, FileAnnotation& fa);
	
	//static bool IsMergeable(CXCursorKind kind);
	//static bool IsMergeable(int kind);
	
	void AddSeenPath(const String& path);
	void AddSeenPaths(const Vector<String>& paths);
	String GetSeenPath(hash_t str_hash) const;
	bool MergeValue(VfsValue& root, const VfsValue& other, MergeMode mode);
	bool MergeVisit(Vector<VfsValue*>& scope, const VfsValue& n1, MergeMode mode);
	bool MergeVisitPartMatching(Vector<VfsValue*>& scope, const VfsValue& n1, MergeMode mode);
	void RefreshNodePtrs(VfsValue& n);
	void MergeVisitPost(VfsValue& n);
	VfsValue* FindTypeDeclaration(hash_t type_hash);
	bool MergeResolver(ClangTypeResolver& ctr);
	hash_t RealizeTypePath(const String& path);
	bool GetFiles(const VfsPath& rel_path, Vector<Shared<VfsItem>>& items) override;
	VfsItemType CheckItem(const VfsPath& rel_path) override;
	
	// Overlay support methods
	Vector<String> List(String logical_path) const;
	Value GetMerged(String logical_path) const;
	// void AddOverlay(Ptr<VfsOverlay> overlay);  // TODO: Fix namespace collision with VfsOverlay
};

MetaEnvironment& MetaEnv();




template<> inline void Visitor::VisitVectorSerialize<Array<VfsValue>>(Array<VfsValue>& o) {
	ChkSerializeMagic();
	Ver(1)(1);
	int count = max(0,o.GetCount());
	(*stream) / count;
	if (!storing) {
		o.SetCount(count);
		if (scope) {
			VfsValue* owner = reinterpret_cast<VfsValue*>(scope);
			for (auto& i : o)
				i.owner = owner;
		}
	}
	int dbg_i = 0;
	for (auto& v : o) {
		ChkSerializeMagic();
		v.Visit(*this);
		dbg_i++;
	}
}




template <class T>
inline T* VirtualNode::As() {
	if (!data) return 0;
	Data& d = *data;
	if (d.mode == VFS_VALUE) {
		if (d.root_poly_value) {
			Value val = Get(*d.root_poly_value, d.path);
			if (val.Is<T>())
				return &const_cast<T&>(val.To<T>());
		}
	}
	else if (d.mode == VFS_ENTITY) {
		if (d.vfs_value)
			return d.vfs_value->FindExt<T>();
	}
	return 0;
}



// MCP Env API stubs (to be implemented). These are non-breaking additions.
// Contract (draft):
// - Node identity (EnvNodeInfo.id): stable across sessions for the same symbol/config.
//   Suggested format: hash(namespace + qualified_name + kind + file + primary_range), or an internal USR.
// - Ranges are 1-based line/column, inclusive start, exclusive end.
// - EnvStatus.initialized = true when AST/xrefs for current config are populated.
// - References may be large: EnvReferences must support paging via page_token/limit.
// - Code extraction should prefer AST pretty-print; fallback to file slice by range.
struct EnvNodeInfo : Moveable<EnvNodeInfo> {
    String id;
    String kind;
    String name;
    String file;
    int    start_line = 0;
    int    start_col = 0;
    int    end_line = 0;
    int    end_col = 0;
};

struct EnvRefPage {
    Vector<EnvNodeInfo> items; // or locations if lighter desired
    String next_page_token;
};

struct EnvStatusInfo {
    bool initialized = false;
    int64 last_update_ts = 0; // unix seconds
    int   stale_files = 0;
};

// TODO: implement these in Core2/VfsValue*.cpp and/or adapters in ide/Vfs
EnvNodeInfo EnvLocate(const String& file, int line, int column);
EnvNodeInfo EnvGet(const String& id);
Vector<EnvNodeInfo> EnvDefinition(const String& id);
EnvRefPage EnvReferences(const String& id, const String& page_token = String(), int limit = 200);
String EnvCodeById(const String& id);
String EnvCodeByRange(const String& file, int sline, int scol, int eline, int ecol);
EnvStatusInfo EnvStatus();

#endif
