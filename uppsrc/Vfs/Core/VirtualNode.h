#ifndef _Core_VfsBase_VirtualNode_h_
#define _Core_VfsBase_VirtualNode_h_

NAMESPACE_UPP


struct VfsValue;
struct EntityData;

struct VirtualNode : Moveable<VirtualNode> {
	enum {
		VFS_INVALID = -1,
		VFS_VALUE,
		VFS_ENTITY
	};
	struct Data {
		Atomic refs = 0;
		VfsValue* vfs_value = 0;
		EntityData* edata = 0;
		Value* root_poly_value = 0;
		Value key;
		VfsPath path;
		int mode = -1;
		void Inc() {refs++;}
		void Dec() {refs--; if (refs <= 0) delete this;}
	};
	Data* data = 0;
	VirtualNode();
	VirtualNode(const VirtualNode& vn);
	VirtualNode(VirtualNode&& vn);
	VirtualNode& operator=(const VirtualNode& vn);
	~VirtualNode();
	VirtualNode Find(Value name);
	Vector<VirtualNode> GetAll();
	Vector<VirtualNode> FindAll(hash_t type_hash);
	VirtualNode Add(Value name, hash_t type_hash);
	VirtualNode GetAdd(Value name, hash_t type_hash);
	Value GetName() const;
	String GetTypeString() const;
	hash_t GetTypeHash() const;
	void SetType(hash_t type_hash);
	bool IsValue() const;
	bool IsEntity() const;
	Value GetValue() const;
	VfsValue* GetVfsValue();
	const VfsValue* GetVfsValue() const;
	void WriteValue(Value val);
	operator bool() const;
	void Clear();
	void RemoveSubNodes();
	void Remove(const Value& name);
	//Data& Create();
	Data& Create(const VfsPath& p, VfsValue* n);
	Data& CreateValue(const VfsPath& p, Value* root_value);
	VfsPath GetPath() const;
	
	template <class T>
	T& GetAddExt(String name);
	
	template <class T>
	T* As();
	
	static Value Get(const Value& root_value, const VfsPath& path);
	static void  Set(Value& root_value, const VfsPath& path, const Value& value);
	static void  RemoveSubNodes(Value& root_value, const VfsPath& path);
	static void  Remove(Value& root_value, const VfsPath& path);
	static void  SetKey(Value& root_value, const VfsPath& path, int path_i, const Value& value);
};


END_UPP_NAMESPACE

#endif
