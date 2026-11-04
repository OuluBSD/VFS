#ifndef _Vfs_Factory_VfsFactory_h_
#define _Vfs_Factory_VfsFactory_h_

struct VfsValue;
struct VfsValueExt;
struct DatasetPtrs;
struct VfsValueExtCtrl;

typedef enum {
	VFSEXT_DEFAULT,
	VFSEXT_SYSTEM_ECS,
	VFSEXT_SYSTEM_ATOM,
	VFSEXT_COMPONENT,
	VFSEXT_ATOM,
	VFSEXT_LINK,
	VFSEXT_EXCHANGE,
} VfsExtType;

template <bool b, class T>
struct EntityDataCreator {static EntityData* CreateEntityDataFn();};
template <class T> struct EntityDataCreator<false,T> {static EntityData* New() {return 0;}};
template <class T> struct EntityDataCreator<true, T> {static EntityData* New() {return new T;}};

struct VfsValueExtFactory {
	typedef VfsValueExt* (*NewFn)(VfsValue&);
	typedef VfsValueExtCtrl* (*NewCtrl)();
	typedef bool (*IsFn)(const VfsValueExt& e);
	typedef void (*SetDatasetPtr)(DatasetPtrs&, VfsValueExt&);
	typedef EntityData* (*CreateEntityData)();
	typedef LinkTypeCls (*GetLinkTypeFn)();
	
	struct Factory {
		VfsExtType			type;
		hash_t				type_hash = 0; // NOT same as TypeCls::GetHashValue()
		TypeCls				type_cls;
		String				category;
		String				name;
		String				eon_name;
		String				ctrl_name;
		NewFn				new_fn = 0;
		NewCtrl				new_ctrl_fn = 0;
		CreateEntityData	create_ed_fn = 0;
		SetDatasetPtr		set_ptr_fn = 0;
		IsFn				is_fn = 0;
	};
	
	struct AtomData : Moveable<AtomData> {
		NewFn				new_fn;
		String				name;
		AtomTypeCls			cls;
		TypeCls				rtti_cls;
		LinkTypeCls			link_type;
		Vector<String>		actions;
	};
	
	struct LinkData : Moveable<LinkData> {
		NewFn				new_fn;
		String				name;
		LinkTypeCls			cls;
		TypeCls				rtti_cls;
	};
	
	struct IfaceData : Moveable<IfaceData> {
		hash_t				type_hash = 0;
		TypeCls				cls;
		ValDevCls			vd;
		String				name;
	};
	
	/*struct ExptData : Moveable<ExptData> {
		NewExpt new_fn;
	};*/
	
	static VectorMap<String, TypeCls>& EonToType();
    
	template<class T> struct Functions {
		static VfsValueExt* Create(VfsValue& owner) {VfsValueExt* c = new T(owner); return c;}
		static bool IsNodeExt(const VfsValueExt& e) {return dynamic_cast<const T*>(&e);}
	};
	template<class T> struct CtrlFunctions {
		static VfsValueExtCtrl* CreateCtrl() {return new T;}
	};
	
	template<class T> static void SetDatasetData(DatasetPtrs& p, VfsValueExt& ext) {}
	static void SetDatasetPtrs(DatasetPtrs& p, VfsValueExt& ext);
	/*template <class T> static void DatasetEntityData(DatasetPtrs& p, EntityData& ed) {
		T* o = dynamic_cast<T*>(&ed);
		ASSERT(o);
		DatasetAssigner<T,0>::Set(p,o);
	}*/
	static int FindTypeHashFactory(hash_t h);
	static int FindTypeClsFactory(TypeCls t);
	static Array<Factory>& List() {static Array<Factory> f; return f;}
	/*static void SetEntityData(DatasetPtrs& p, hash_t type_hash, EntityData& data) {
		for (const auto& f : List()) {
			if (type_hash == f.type_hash) {
				f.set_data_ed_fn(p, data);
				return;
			}
		}
		ASSERT_(0, "Type not registered with given hash_type");
	}*/
	static Index<String>& Categories() {static Index<String> v; return v;}
	static VectorMap<AtomTypeCls,AtomData>& AtomDataMap() {static VectorMap<AtomTypeCls,AtomData> m; return m;}
	static VectorMap<LinkTypeCls,LinkData>& LinkDataMap() {static VectorMap<LinkTypeCls,LinkData> m; return m;}
	static VectorMap<ValDevTuple,IfaceData>& IfaceLinkDataMap() {static VectorMap<ValDevTuple,IfaceData> m; return m;}
	//static VectorMap<TypeCls,ExptData>& ExptDataMap() {static VectorMap<TypeCls,ExptData> m; return m;}
	static bool IsType(hash_t type_hash, VfsExtType t);
	template <class T> inline static void RegisterExchange(String name, DevCls dev, ValCls val) {
		if (!GetFactory<T>().type_hash)
			Register<T>(name, VFSEXT_EXCHANGE, "exchange." + ToVarName(name,'.'), "Sys|Exchange");
		ValDevCls vd(dev,val);
		IfaceData& d = IfaceLinkDataMap().GetAdd(vd);
		d.type_hash  = AsTypeHash<T>();
		d.cls        = AsTypeCls<T>();
		d.name       = AsTypeName<T>();
		d.vd.dev     = dev;
		d.vd.val     = val;
		//ExptData& d = ExptDataMap().GetAdd(AsTypeCls<T>()); // todo remove?
		//d.new_fn = &New<T>;
	}
	
	template <class T> inline static void RegisterAtom(String name) {
		Register<T>(name, VFSEXT_ATOM, "", "Atom|Uncategorised");
		AtomTypeCls cls = T::GetAtomType();
		AtomData& d = AtomDataMap().GetAdd(cls);
		d.rtti_cls = AsTypeCls<T>();
		d.cls = cls;
		d.name = AsTypeName<T>();
		d.new_fn = &Functions<T>::Create;
		d.link_type = T::GetLinkType();
		d.actions.Add(T::GetAction());
	}
	
	template <class T> static void RegisterLink(String name) {
		Register<T>(name, VFSEXT_LINK, "", "Link|Uncategorised");
		LinkTypeCls cls = T::GetLinkTypeStatic();
		ASSERT(cls.IsValid());
		LinkData& d = LinkDataMap().GetAdd(cls);
		d.rtti_cls = AsTypeCls<T>();
		d.cls = cls;
		d.name = AsTypeName<T>();
		d.new_fn = &Functions<T>::Create;
	}
	
	template <class T> inline static Factory& GetFactory() {
		TypeCls type_cls = AsTypeCls<T>();
		for (auto& f : List())
			if (f.type_cls == type_cls)
				return f;
		auto& f = List().Add();
		f.type_cls = type_cls;
		ASSERT(f.name.Find("|") < 0);
		return f;
	}
	inline static Factory* FindFactory(String name) {
		for (auto& f : List())
			if (f.name == name)
				return &f;
		return 0;
	}
	inline static Factory* FindFactoryEon(String eon_name, VfsExtType type) {
		for (auto& f : List())
			if (f.eon_name == eon_name && f.type == type)
				return &f;
		return 0;
	}
	template <class T> inline static void Register(String name, VfsExtType type, String eon_name, String category) {
		Factory& f = GetFactory<T>();
		ASSERT(!f.type_hash);
		f.type_hash = TypedStringHasher<T>(name);
		f.type = type;
		f.category = category;
		f.name = name;
		f.eon_name = eon_name;
		f.new_fn = &Functions<T>::Create;
		f.is_fn = &Functions<T>::IsNodeExt;
		//f.set_data_ed_fn = &DatasetEntityData<T>;
		f.create_ed_fn = &EntityDataCreator<std::is_base_of<::UPP::EntityData,T>::value,T>::New;
		if (type == VFSEXT_SYSTEM_ECS) {
			LOG("VfsValueExtFactory::Register: adding system '" << eon_name << "' to EonToType");
			EonToType().GetAdd(eon_name) = f.type_cls;  // Use eon_name (Eon ID) instead of name (class name)
		}
		if (type == VFSEXT_SYSTEM_ECS)
			f.set_ptr_fn = &SetDatasetData<T>;
		ASSERT(f.name.Find("|") < 0);
	}
	
	template <class Comp, class Ctrl> static void RegisterCtrl(String ctrl_name) {
		TypedStringHasher<Ctrl>(ctrl_name); // realize ctrl hash
		Factory& f = GetFactory<Comp>();
		ASSERT_(!f.new_ctrl_fn, "Only one Ctrl per Extension is supported currently, and one is already registered");
		f.new_ctrl_fn = &CtrlFunctions<Ctrl>::CreateCtrl;
		f.ctrl_name = ctrl_name;
		ASSERT(f.name.Find("|") < 0);
	}
	static VfsValueExt* Create(hash_t type_hash, VfsValue& owner);
	//static VfsValueExt* CloneKind(int kind, const VfsValueExt& e, VfsValue& owner);
	static VfsValueExt* Clone(const VfsValueExt& e, VfsValue& owner);
	//static int FindKindFactory(int kind);
	static int AstFindKindCategory(int kind);
	
	static int FindComponent(hash_t type_hash) {
		int i = 0;
		for (auto& l : List()) {
			if (l.type_hash == type_hash && l.type == VFSEXT_COMPONENT)
				return i;
			i++;
		}
		return -1;
	}
	
	static const Vector<Vector<String>>& GetCategories();
};

#endif

