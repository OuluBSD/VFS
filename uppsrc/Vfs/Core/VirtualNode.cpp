#include "VfsBase.h"
#include <Vfs/Core/Core.h>

NAMESPACE_UPP

VirtualNode::VirtualNode() {}

VirtualNode::VirtualNode(const VirtualNode& vn) : data(vn.data) {
	if (data) data->Inc();
}

VirtualNode::VirtualNode(VirtualNode&& vn) : data(vn.data) {
	vn.data = 0;
}

VirtualNode::~VirtualNode() {
	Clear();
}

VirtualNode& VirtualNode::operator=(const VirtualNode& vn) {
	Clear();
	data = vn.data;
	if (data) data->Inc();
	return *this;
}

VirtualNode VirtualNode::Find(Value name) {
	VirtualNode n;
	if (data) {
		if (data->root_poly_value) {
			VfsPath p = data->path;
			p.Add(name);
			Value v = Get(*data->root_poly_value, p);
			if (!v.IsError())
				n.CreateValue(p, data->root_poly_value);
		}
		else if (data->vfs_value) {
			VfsValue& mn = *data->vfs_value;
			int i = mn.Find(name);
			if (i >= 0) {
				VfsValue& sub = mn.sub[i];
				n.Create(data->path + name, &sub);
			}
		}
		else TODO;
	}
	return n;
}

void VirtualNode::RemoveSubNodes() {
	ASSERT(data);
	if (!data) return;
	if (data->root_poly_value) {
		RemoveSubNodes(*data->root_poly_value, data->path);
	}
	else if (data->vfs_value) {
		VfsValue& mn = *data->vfs_value;
		mn.sub.Clear();
	}
	else TODO;
}

void VirtualNode::Remove(const Value& name) {
	ASSERT(data);
	if (!data) return;
	if (data->root_poly_value) {
		VfsPath p(data->path);
		p.Add(name);
		Remove(*data->root_poly_value, p);
	}
	else if (data->vfs_value) {
		VfsValue& mn = *data->vfs_value;
		int i = mn.Find(name);
		ASSERT(i >= 0);
		if (i >= 0)
			mn.Remove(i);
	}
	else TODO;
}

Vector<VirtualNode> VirtualNode::GetAll() {
	Vector<VirtualNode> v;
	ASSERT(data);
	if (data) {
		int mode = data->mode;
		if (mode == VirtualNode::VFS_VALUE) {
			if (data->root_poly_value) {
				Value poly_value = Get(*data->root_poly_value, data->path);
				
				if (poly_value.IsNull())
					poly_value = ValueMap();
				if (poly_value.Is<ValueMap>()) {
					ValueMap map = poly_value;
					v.Reserve(map.GetCount());
					for(int i = 0; i < map.GetCount(); i++) {
						Value& val = const_cast<Value&>(map.GetValue(i));
						if (val.Is<ValueMap>()) {
							Value key = map.GetKey(i);
							auto& o = v.Add().CreateValue(data->path + key, data->root_poly_value);
							ASSERT(o.path.IsEmpty() || o.root_poly_value->Is<ValueMap>());
						}
					}
				}
				else if (poly_value.Is<ValueArray>()) {
					ValueArray arr = poly_value;
					v.Reserve(arr.GetCount());
					for(int i = 0; i < arr.GetCount(); i++) {
						Value& val = arr.At(i);
						if (val.Is<ValueMap>()) {
							v.Add().CreateValue(data->path + i, data->root_poly_value);
						}
					}
				}
				else if (poly_value.Is<AstValue>()) {
					TODO
				}
				else {
					LOG(poly_value.GetTypeName());
					LOG(poly_value.ToString());
					TODO;
				}
			}
			else if (data->vfs_value) {
				ASSERT_(0, "only entity pointer in value based vfs");
			}
			else TODO;
		}
		else if (mode == VirtualNode::VFS_ENTITY) {
			if (data->vfs_value) {
				VfsValue& n = *data->vfs_value;
				if (!n.ext || !n.ext->GetAll(v)) {
					v.Reserve(n.sub.GetCount());
					for(int i = 0; i < n.sub.GetCount(); i++) {
						auto& sub = n.sub[i];
						if (sub.id.IsEmpty())
							v.Add().Create(data->path + i, &sub);
						else
							v.Add().Create(data->path + (Value)sub.id, &sub);
					}
				}
			}
			else if (data->root_poly_value) {
				ASSERT_(0, "only value pointer in entity based vfs");
			}
			else TODO;
		}
		else TODO;
	}
	return v;
}

Vector<VirtualNode> VirtualNode::FindAll(hash_t type_hash) {
	// TODO optimized solution (this is lazy)
	Vector<VirtualNode> n = GetAll();
	Vector<int> rmlist;
	for(int i = 0; i < n.GetCount(); i++) {
		if (n[i].GetTypeHash() != type_hash)
			rmlist << i;
	}
	if (!rmlist.IsEmpty()) n.Remove(rmlist);
	return n;
}

VirtualNode VirtualNode::Add(Value name, hash_t type_hash) {
	VirtualNode n;
	if (data) {
		if (data->mode == VirtualNode::VFS_VALUE) {
			if (data->root_poly_value) {
				Value poly_value = Get(*data->root_poly_value, data->path);
				
				if (!poly_value.Is<ValueMap>())
					poly_value = ValueMap();
				ValueMap map = poly_value;
				{
					Value v = ValueMapGet(map, name, Value());
					if (!v.Is<ValueMap>())
						v = ValueMap();
					ValueMap sub_map = v;
					sub_map.Set(".type_hash", (int64)type_hash);
					map.Set(name, sub_map);
				}
				n.CreateValue(data->path + name, data->root_poly_value);
				poly_value = map;
				
				Set(*data->root_poly_value, data->path, poly_value);
			}
		}
		else if (data->mode == VirtualNode::VFS_ENTITY) {
			if (data->vfs_value) {
				VfsValue& sub = data->vfs_value->Add(name, type_hash);
				n.Create(data->path + name, &sub);
			}
		}
		else TODO;
	}
	return n;
}

VirtualNode VirtualNode::GetAdd(Value name, hash_t type_hash) {
	VirtualNode n;
	if (data) {
		if (data->mode == VirtualNode::VFS_VALUE) {
			if (data->root_poly_value) {
				Value poly_value = Get(*data->root_poly_value, data->path);
				
				if (!poly_value.Is<ValueMap>())
					poly_value = ValueMap();
				{
					ValueMap map = poly_value;
					{
						Value v = ValueMapGet(map, name, Value());
						if (!v.Is<ValueMap>())
							v = ValueMap();
						ValueMap sub_map = v;
						Value old_type_hash = ValueMapGet(sub_map, ".type_hash", Value());
						if (old_type_hash.Is<int64>() && (int64)old_type_hash == (int64)type_hash) {
							n.CreateValue(data->path + name, data->root_poly_value);
							return n;
						}
						sub_map.Set(".type_hash", (int64)type_hash);
						map.Set(name, sub_map);
					}
					poly_value = map;
				}
				auto& val = poly_value.GetAdd(name);
				ASSERT(val.Is<ValueMap>());
				n.CreateValue(data->path + name, data->root_poly_value);
				
				Set(*data->root_poly_value, data->path, poly_value);
			}
		}
		else if (data->mode == VirtualNode::VFS_ENTITY) {
			if (data->vfs_value) {
				VfsValue& sub = data->vfs_value->GetAdd(name, type_hash);
				n.Create(data->path + name, &sub);
			}
		}
		else TODO;
	}
	return n;
}

Value VirtualNode::GetName() const {
	if (data) {
		if (data->mode == VirtualNode::VFS_VALUE) {
			if (data->root_poly_value)
				return data->key;
		}
		else if (data->mode == VirtualNode::VFS_ENTITY) {
			if (data->vfs_value)
				return data->vfs_value->id;
		}
		else TODO;
	}
	return Value();
}

String VirtualNode::GetTypeString() const {
	if (data) {
		if (data->mode == VirtualNode::VFS_VALUE) {
			if (data->root_poly_value) {
				Value poly_value = Get(*data->root_poly_value, data->path);
				
				if (poly_value.Is<ValueMap>()) {
					ValueMap map = poly_value;
					int i = map.Find(".type_hash");
					if (i >= 0) {
						Value type_hash_value = map.GetValue(i);
						ASSERT(type_hash_value.Is<int64>());
						hash_t type_hash = (hash_t)(int64)type_hash_value;
						return TypeStringHasherIndex::ToString(type_hash);
					}
				}
			}
		}
		else if (data->mode == VirtualNode::VFS_ENTITY) {
			if (data->vfs_value)
				return data->vfs_value->AstGetKindString();
		}
		else TODO;
	}
	return String();
}

hash_t VirtualNode::GetTypeHash() const {
	if (data) {
		if (data->mode == VirtualNode::VFS_VALUE) {
			if (data->root_poly_value) {
				Value poly_value = Get(*data->root_poly_value, data->path);
			
				if (poly_value.Is<ValueMap>()) {
					ValueMap map = poly_value;
					int i = map.Find(".type_hash");
					if (i >= 0)
						return (hash_t)(int64)map.GetValue(i);
				}
				return 0;
			}
		}
		else if (data->mode == VirtualNode::VFS_ENTITY) {
			if (data->vfs_value)
				return data->vfs_value->type_hash;
		}
		else TODO;
	}
	ASSERT_(0, "no pointer");
	return 0;
}

void VirtualNode::SetType(hash_t type_hash) {
	ASSERT(data);
	if (data) {
		if (data->mode == VirtualNode::VFS_VALUE) {
			Value poly_value = Get(*data->root_poly_value, data->path);
			
			if (poly_value.IsNull())
				poly_value = ValueMap();
			if (poly_value.Is<ValueMap>()) {
				ValueMap map = poly_value;
				Value old_type_hash = ValueMapGet(map, ".type_hash", Value());
				if (!old_type_hash.Is<int64>() || (int64)old_type_hash != (int64)type_hash) {
					map.Set(".type_hash", (int64)type_hash);
					poly_value = map;
					Set(*data->root_poly_value, data->path, poly_value);
				}
			}
			else TODO;
		}
		else if (data->mode == VirtualNode::VFS_ENTITY) {
			if (data->vfs_value) {
				if (data->vfs_value->type_hash != type_hash)
					data->vfs_value->CreateExt(type_hash);
			}
			else TODO;
		}
		else TODO;
	}
}

bool VirtualNode::IsValue() const {return data && data->mode == VFS_VALUE && data->root_poly_value;}
bool VirtualNode::IsEntity() const {return data && data->mode == VFS_ENTITY && data->vfs_value;}

Value VirtualNode::GetValue() const {
	if (data && data->mode == VFS_VALUE && data->root_poly_value)
		return Get(*data->root_poly_value, data->path);
	return Value();
}

VfsValue* VirtualNode::GetVfsValue() {
	if (data && data->mode == VFS_ENTITY)
		return data->vfs_value;
	return nullptr;
}

const VfsValue* VirtualNode::GetVfsValue() const {
	if (data && data->mode == VFS_ENTITY)
		return data->vfs_value;
	return nullptr;
}

void VirtualNode::WriteValue(Value val) {
	ASSERT(IsValue());
	if (data && data->mode == VFS_VALUE && data->root_poly_value)
		Set(*data->root_poly_value, data->path, val);
}

VirtualNode::operator bool() const {return data;}
void VirtualNode::Clear() {if (data) {data->Dec(); data = 0;}}
//VirtualNode::Data& VirtualNode::Create() {Clear(); data = new Data(); data->Inc(); return *data;}
VirtualNode::Data& VirtualNode::Create(const VfsPath& p, VfsValue* n)
{
	Clear();
	data = new Data();
	data->path = p;
	data->vfs_value = n;
	data->mode = VFS_ENTITY;
	data->Inc();
	return *data;
}

VirtualNode::Data& VirtualNode::CreateValue(const VfsPath& p, Value* root)
{
	Clear();
	data = new Data();
	data->path = p;
	data->key = p.TopPart();
	data->root_poly_value = root;
	data->mode = VFS_VALUE;
	data->Inc();
	return *data;
}

VfsPath VirtualNode::GetPath() const {
	return data ? data->path : VfsPath();
}


Value VirtualNode::Get(const Value& root_value, const VfsPath& path) {
	if (path.IsEmpty())
		return root_value;
	ValueMap map;
	if (!root_value.Is<ValueMap>())
		return ErrorValue("root is not map");
	map = root_value;
	for(int i = 0; i < path.Parts().GetCount()-1; i++) {
		const Value& part = path.Parts()[i];
		int j = map.Find(part);
		if (j < 0)
			return ErrorValue("path not found: " + path.ToString());
		Value v = map.GetValue(j);
		if (!v.Is<ValueMap>())
			return ErrorValue("value is not map");
		map = v;
	}
	const Value& part = path.Parts().Top();
	int j = map.Find(part);
	if (j < 0)
		return ErrorValue("path not found: " + path.ToString());
	Value v = map.GetValue(j);
	return v;
}

void VirtualNode::Set(Value& root_value, const VfsPath& path, const Value& value) {
	if (path.IsEmpty()) {
		root_value = value;
		return;
	}
	struct Scope : Moveable<Scope> {
		ValueMap map;
		int idx = -1;
	};
	Vector<Scope> scopes;
	
	if (!root_value.Is<ValueMap>())
		root_value = ValueMap();
	
	scopes.Add().map = root_value;
	
	for(int i = 0; i < path.Parts().GetCount()-1; i++) {
		Scope& s0 = scopes.Add();
		Scope& s1 = scopes[i];
		ValueMap& map = s1.map;
		
		const Value& part = path.Parts()[i];
		int j = map.Find(part);
		if (j < 0) {
			j = map.GetCount();
			map.Add(part, Value());
		}
		Value v = map.GetValue(j);
		if (v.Is<ValueArray>()) {
			TODO
		}
		else if (!v.Is<ValueMap>()) {
			v = ValueMap();
		}
		s0.map = v;
		s0.idx = j;
	}
	
	{
		Scope& s1 = scopes.Top();
		ValueMap& map = s1.map;
		const Value& part = path.Parts().Top();
		int j = map.Find(part);
		if (j < 0) {
			j = map.GetCount();
			map.Add(part, value);
		}
		else {
			map.SetAt(j, value);
		}
	}
	
	for (int i = scopes.GetCount()-2; i >= 0; i--) {
		Scope& s0 = scopes[i];
		Scope& s1 = scopes[i+1];
		s0.map.SetAt(s1.idx, s1.map);
	}
	
	root_value = scopes[0].map;
}

void VirtualNode::SetKey(Value& root_value, const VfsPath& path, int path_i, const Value& value) {
	ASSERT(path_i >= 0 && path_i < path.GetPartCount());
	
	struct Scope : Moveable<Scope> {
		ValueMap map;
		int idx = -1;
	};
	Vector<Scope> scopes;
	
	if (!root_value.Is<ValueMap>())
		root_value = ValueMap();
	
	scopes.Add().map = root_value;
	
	for(int i = 0; i < path_i; i++) {
		Scope& s0 = scopes.Add();
		Scope& s1 = scopes[i];
		ValueMap& map = s1.map;
		
		const Value& part = path.Parts()[i];
		int j = map.Find(part);
		if (j < 0) {
			j = map.GetCount();
			map.Add(part, Value());
		}
		Value v = map.GetValue(j);
		if (v.Is<ValueArray>()) {
			TODO
		}
		else if (!v.Is<ValueMap>()) {
			v = ValueMap();
		}
		s0.map = v;
		s0.idx = j;
	}
	
	{
		Scope& s1 = scopes.Top();
		ValueMap& map = s1.map;
		const Value& part = path.Parts()[path_i];
		int j = map.Find(part);
		ASSERT(j >= 0);
		if (j >= 0) {
			map.SetKey(j, value);
		}
	}
	
	for (int i = scopes.GetCount()-2; i >= 0; i--) {
		Scope& s0 = scopes[i];
		Scope& s1 = scopes[i+1];
		s0.map.SetAt(s1.idx, s1.map);
	}
	
	root_value = scopes[0].map;
}

void VirtualNode::RemoveSubNodes(Value& root_value, const VfsPath& path) {
	if (path.IsEmpty()) {
		root_value = ValueMap();
		return;
	}
	struct Scope : Moveable<Scope> {
		ValueMap map;
		int idx = -1;
	};
	Vector<Scope> scopes;
	
	if (!root_value.Is<ValueMap>())
		root_value = ValueMap();
	
	scopes.Add().map = root_value;
	
	for(int i = 0; i < path.Parts().GetCount(); i++) {
		Scope& s0 = scopes.Add();
		Scope& s1 = scopes[i];
		ValueMap& map = s1.map;
		
		const Value& part = path.Parts()[i];
		int j = map.Find(part);
		if (j < 0) {
			j = map.GetCount();
			map.Add(part, Value());
		}
		Value v = map.GetValue(j);
		if (v.Is<ValueArray>()) {
			TODO
		}
		else if (!v.Is<ValueMap>()) {
			v = ValueMap();
		}
		s0.map = v;
		s0.idx = j;
	}
	
	{
		Scope& s1 = scopes.Top();
		ValueMap& map = s1.map;
		map.Clear();
	}
	
	for (int i = scopes.GetCount()-2; i >= 0; i--) {
		Scope& s0 = scopes[i];
		Scope& s1 = scopes[i+1];
		s0.map.SetAt(s1.idx, s1.map);
	}
	
	root_value = scopes[0].map;
}

void VirtualNode::Remove(Value& root_value, const VfsPath& path) {
	if (path.IsEmpty()) {
		root_value = ValueMap();
		return;
	}
	struct Scope : Moveable<Scope> {
		ValueMap map;
		int idx = -1;
	};
	Vector<Scope> scopes;
	
	if (!root_value.Is<ValueMap>())
		root_value = ValueMap();
	
	scopes.Add().map = root_value;
	
	for(int i = 0; i < path.Parts().GetCount()-1; i++) {
		Scope& s0 = scopes.Add();
		Scope& s1 = scopes[i];
		ValueMap& map = s1.map;
		
		const Value& part = path.Parts()[i];
		int j = map.Find(part);
		if (j < 0) {
			j = map.GetCount();
			map.Add(part, Value());
		}
		Value v = map.GetValue(j);
		if (v.Is<ValueArray>()) {
			TODO
		}
		else if (!v.Is<ValueMap>()) {
			v = ValueMap();
		}
		s0.map = v;
		s0.idx = j;
	}
	
	{
		Scope& s1 = scopes.Top();
		ValueMap& map = s1.map;
		const Value& part = path.Parts().Top();
		int j = map.Find(part);
		if (j < 0)
			return;
		map.Remove(j);
	}
	
	for (int i = scopes.GetCount()-2; i >= 0; i--) {
		Scope& s0 = scopes[i];
		Scope& s1 = scopes[i+1];
		s0.map.SetAt(s1.idx, s1.map);
	}
	
	root_value = scopes[0].map;
}

END_UPP_NAMESPACE
