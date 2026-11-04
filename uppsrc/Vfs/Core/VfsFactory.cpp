#include "Core.h"

NAMESPACE_UPP

bool VfsValueExtFactory::IsType(hash_t type_hash, VfsExtType t) {
	for (auto& l : List())
		if (l.type_hash == type_hash && l.type == t)
			return true;
	return false;
}

VectorMap<String, TypeCls>& VfsValueExtFactory::EonToType() {
	static VectorMap<String, TypeCls> m;
	return m;
}

VfsValueExt* VfsValueExtFactory::Create(hash_t type_hash, VfsValue& owner) {
	int i = FindTypeHashFactory(type_hash);
	if (i < 0)
		return 0;
	return List()[i].new_fn(owner);
}

VfsValueExt* VfsValueExtFactory::Clone(const VfsValueExt& e, VfsValue& owner) {
	hash_t type_hash = e.GetTypeHash();
	int i = FindTypeHashFactory(type_hash);
	if (i < 0)
		return 0;
	VfsValueExt* n = List()[i].new_fn(owner);
	if (n)
		n->CopyFrom(e);
	return n;
}

int VfsValueExtFactory::FindTypeHashFactory(hash_t type_hash) {
	int i = 0;
	for (Factory& f : List()) {
		if (f.type_hash == type_hash)
			return i;
		i++;
	}
	return -1;
}

int VfsValueExtFactory::FindTypeClsFactory(TypeCls t) {
	int i = 0;
	for (Factory& f : List()) {
		if (f.type_cls == t)
			return i;
		i++;
	}
	return -1;
}

int VfsValueExtFactory::AstFindKindCategory(int k) {
	TODO
	#if 0
	#define DATASET_ITEM(type, name, desc) \
		if (k == kind) \
			return group;
	DATASET_LIST
	#undef DATASET_ITEM
	#endif
	return -1;
}

void VfsValueExtFactory::SetDatasetPtrs(DatasetPtrs& p, VfsValueExt& ext) {
	hash_t type_hash = ext.GetTypeHash();
	for (const auto& f : List()) {
		if (type_hash == f.type_hash) {
			if (f.set_ptr_fn)
				f.set_ptr_fn(p, ext);
			return;
		}
	}
	ASSERT_(0, "Type not registered with given hash_type");
}

const Vector<Vector<String>>& VfsValueExtFactory::GetCategories() {
	static Vector<Vector<String>> v;
	if (v.IsEmpty()) {
		Vector<Vector<String>> o;
		Index<String> uniq_cats;
		for (auto& l : List()) {
			String& cat = l.category;
			uniq_cats.FindAdd(cat);
		}
		SortIndex(uniq_cats, StdLess<String>());
		for(int i = 0; i < uniq_cats.GetCount(); i++) {
			String cat = uniq_cats[i];
			auto sc = Split(cat, "|");
			if (sc.IsEmpty())
				continue;
			for (auto& s : sc)
				s = TrimBoth(s);
			Swap(o.Add(), sc);
		}
		Swap(v, o);
	}
	return v;
}

END_UPP_NAMESPACE
