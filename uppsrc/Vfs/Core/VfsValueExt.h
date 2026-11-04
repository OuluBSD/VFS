#ifndef _Vfs_Core_VfsValueExt_h_
#define _Vfs_Core_VfsValueExt_h_

struct VfsValue;
struct VfsValueExtCtrl;
struct VfsSrcPkg;
struct Entity;
struct DatasetPtrs;
struct VirtualNode;
class WorldState;
class Value;
class JsonIO;
class Stream;
class ClangTypeResolver;
class ToolAppCtrl;

struct EntityData : Pte<EntityData> {
	virtual ~EntityData() {}
	virtual hash_t GetTypeHash() const = 0;
	virtual TypeCls GetTypeCls() const = 0;
	virtual String GetTypeName() const = 0;
	virtual void* GetTypePtr() const = 0;
	virtual void Visit(Vis& s) = 0;
};

struct NodeRoute {
	Vector<VfsValue*> route;
	bool from_owner_only = false;
};

struct VfsValueExt : Pte<VfsValueExt> {
	using VfsValueExtPtr = Ptr<VfsValueExt>;
	VfsValue& val;
	bool is_initialized = false;

	VfsValueExt(VfsValue& n);
	virtual ~VfsValueExt();
	virtual void Visit(Vis& s) = 0;
	virtual TypeCls GetTypeCls() const = 0;
	virtual String GetTypeName() const = 0;
	virtual hash_t GetTypeHash() const = 0;
	virtual void* GetTypePtr() const = 0;
	virtual String GetName() const;
	virtual double GetUtility();
	virtual double GetEstimate();
	virtual double GetDistance(VfsValue& dest);
	virtual bool GenerateSubValues(const Value& params, NodeRoute& prev);
	virtual bool TerminalTest();
	virtual String ToString() const;
	virtual bool Start();
	virtual void Stop();
	virtual bool Initialize(const WorldState& ws);
	virtual bool PostInitialize();
	virtual void Uninitialize();
	virtual void UninitializeDeep();
	virtual void Update(double dt);
	virtual bool Arg(String key, Value value);
	virtual bool AddDependency(VfsValueExt& val);
	virtual VfsValueExtPtr GetDependency(int i) const;
	virtual int GetDependencyCount() const;
	virtual void RemoveDependency(int i);
	virtual void ClearDependencies();
	virtual void RemoveDependency(const VfsValueExt* e);
	virtual String GetTreeString(int indent = 0) const;
	virtual bool GetAll(Vector<VirtualNode>&);
	hash_t GetHashValue() const;
	int AstGetKind() const;
	bool IsInitialized() const;
	void SetInitialized(bool b = true);
	void CopyFrom(const VfsValueExt& e);
	bool operator==(const VfsValueExt& e) const;
	void Serialize(Stream& s);
	void Jsonize(JsonIO& json);

	static String GetCategory();

protected:
	Vector<VfsValueExtPtr> deps;
};

using VfsValueExtPtr = Ptr<VfsValueExt>;

#endif
