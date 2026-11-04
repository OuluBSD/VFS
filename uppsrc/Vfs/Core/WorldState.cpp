#include "VfsBase.h"

NAMESPACE_UPP


#define DBG_PRINT 1



WorldState::WorldState() {
	Clear();
}

void WorldState::Clear() {
	values.Clear();
	//using_act.Clear();
	//cur_atom = Null;
	//type = INVALID;
}

int WorldState::GetValueCount() const {
	// this is a bit ankward counting
	int c = 0;
	for (const String& v : values.GetValues())
		c += (v.IsEmpty() ? 0 : 1);
	return c;
}

void WorldState::FindKeys(String key_left, Index<String>& keys) const {
	for(int i = 0; i < values.GetCount(); i++) {
		auto& v = values.GetValue(i);
		if (v.IsNull())
			continue;
		String key = values.GetKey(i);
		if (key.Left(key_left.GetCount()) == key_left)
			keys.FindAdd(key);
	}
}

void WorldState::SetTrue(const String& key) {Set(key, true);}

void WorldState::SetFalse(const String& key) {Set(key, false);}

bool WorldState::IsEmpty() const {return values.GetCount() == 0;}

WorldState& WorldState::operator=(const WorldState& src) {
	values = src.values;
	return *this;
}


hash_t WorldState::GetHashValue() const {
	CombineHash c;
	for(int i = values.GetCount()-1; i >= 0; i--) {
		const Value& o = values.GetValue(i);
		c.Put(o.GetHashValue());
	}
	return c;
}

bool WorldState::Set(const String& key, bool value) {
	values.GetAdd(key) = value;
	return true;
}

bool WorldState::Set(const String& key, String value) {
	values.GetAdd(key) = value;
	return true;
}

bool WorldState::IsTrue(const String& key, bool def) const {
	int i = values.Find(key);
	if (i >= 0)
		return values.GetValue(i);
	return def;
}

bool WorldState::IsFalse(const String& key, bool def) const {
	int i = values.Find(key);
	if (i >= 0)
		return !values.GetValue(i);
	return def;
}

bool WorldState::IsFalse(int idx) const {
	return !values.GetValue(idx);
}

bool WorldState::IsUndefined(const String& key) const {
	return values.Find(key) < 0;
}

bool WorldState::IsUndefined(int idx) const {
	return idx < 0 || idx >= values.GetCount();
}

String WorldState::Get(const String& key, String def) const {
	int i = values.Find(key);
	if (i < 0)
		return def;
	const Value& o = values.GetValue(i);
	return o.ToString();
}

String WorldState::Get(int idx) const {
	const Value& o = values.GetValue(idx);
	return o.ToString();
}

Size WorldState::GetSize(const String& cx, const String& cy, Size def) const {
	String cx_str = Get(cx);
	String cy_str = Get(cy);
	if (cx_str.IsEmpty() || cy_str.IsEmpty())
		return def;
	return Size(StrInt(cx_str), StrInt(cy_str));
}

int WorldState::GetInt(const String& key, int def) const {
	int i = values.Find(key);
	if (i >= 0) {
		const Value& o = values.GetValue(i);
		return o;
	}
	return def;
}

double WorldState::GetDouble(const String& key, double def) const {
	int i = values.Find(key);
	if (i >= 0) {
		const Value& o = values.GetValue(i);
		return o;
	}
	return def;
}

bool WorldState::GetBool(const String& key, bool def) const {
	int i = values.Find(key);
	if (i >= 0) {
		const Value& o = values.GetValue(i);
		return o;
	}
	return def;
}

String WorldState::GetString(const String& key, String def) const {
	int i = values.Find(key);
	if (i >= 0) {
		const Value& o = values.GetValue(i);
		if (!o.Is<String>()) {LOG(AsJSON(o));}
		ASSERT(o.Is<String>());
		return o
		;
	}
	return def;
}

String WorldState::ToString() const {
	String s;
	for(int i = 0; i < values.GetCount(); i++) {
		const Value& vo = values.GetValue(i);
		String v = vo.ToString();
		if (v.IsEmpty()) v = "false";
		String k = values.GetKey(i).ToString();
		if (!s.IsEmpty())
			s << ",";
		s << k << "=" << v;
	}
	return s;
}

String WorldState::GetFullString() const {
	TODO
	return String();
}

bool WorldState::Contains(const WorldState& ws) const {
	TODO
	return false;
}

bool WorldState::Conflicts(const WorldState& ws) const {
	TODO
	return false;
}


int WorldState::Compare(int idx, const WorldState& ws) const {
	TODO
	return 0;
}










BinaryWorldState::BinaryWorldState() {
	
	Clear();
}

BinaryWorldState::BinaryWorldState(const BinaryWorldState& ws) {
	*this = ws;
}

BinaryWorldState::BinaryWorldState(BinaryWorldState&& ws) : mask(ws.mask), atoms(pick(ws.atoms)) {
	
}

void BinaryWorldState::Clear() {
	atoms.Clear();
}

bool BinaryWorldState::SetMasked(int index, bool value) {
	ASSERT(index >= 0 && index < mask->keys.GetCount());
	if (index < 0) return false;
	if (atoms.GetCount() <= index) {
		atoms.SetCount(index+1);
	}
	auto& atom = atoms[index];
	atom.in_use = true;
	atom.value = value;
	return true;
}

bool BinaryWorldState::SetKey(const WorldStateKey& key, bool value, bool add_atom) {
	ASSERT(mask);
	if (!mask)
		return false;
	int index = mask->FindAdd(key, add_atom);
	return SetMasked(index, value);
}

bool BinaryWorldState::SetAtomIndex(int atom_idx, bool value) {
	ASSERT(atom_idx >= 0 && atom_idx < mask->session->atoms.GetCount());
	WorldStateKey key = mask->session->GetAtomKey(atom_idx);
	return SetKey(key, value);
}

BinaryWorldState& BinaryWorldState::operator = (const BinaryWorldState& src) {
	atoms <<= src.atoms;
	mask = src.mask;
	return *this;
}

bool BinaryWorldState::operator==(const BinaryWorldState& src) const {
	if (mask != src.mask)
		return false;
	int count = min(src.atoms.GetCount(), atoms.GetCount());
	for(int i = count; i < src.atoms.GetCount(); i++)
		if (src.atoms[i].in_use)
			return false;
	for(int i = count; i < atoms.GetCount(); i++)
		if (atoms[i].in_use)
			return false;
	for(int i = 0; i < count; i++) {
		if (src.atoms[i].in_use != atoms[i].in_use ||
			src.atoms[i].value  != atoms[i].value)
			return false;
	}
	return true;
}

bool BinaryWorldState::IsPartialMatch(const BinaryWorldState& src) const {
	if (mask != src.mask)
		return false;
	int count = min(src.atoms.GetCount(), atoms.GetCount());
	for(int i = count; i < src.atoms.GetCount(); i++)
		if (src.atoms[i].in_use)
			return false;
	for(int i = 0; i < count; i++) {
		bool u0 = atoms[i].in_use;
		bool u1 = src.atoms[i].in_use;
		if (!u0 && u1)
			return false;
		if (!u1)
			continue;
		ASSERT(u0 && u1);
		bool v0 = atoms[i].value;
		bool v1 = src.atoms[i].value;
		if (v0 != v1)
			return false;
	}
	return true;
}

hash_t BinaryWorldState::GetHashValue() const {
	CombineHash c;
	int last_i = atoms.GetCount()-1;
	while (last_i >= 0) {
		if (atoms[last_i].in_use)
			break;
		last_i--;
	}
	for(int i = 0; i <= last_i; i++) {
		bool b = atoms[last_i].in_use;
		c.Put(b);
		if (b)
			c.Put(atoms[i].value);
	}
	return c;
}

Value BinaryWorldState::ToValue() const {
	ValueMap map;
	int i = 0;
	for (auto& at : this->atoms) {
		if (at.in_use) {
			map.Add(i, (int)at.value);
		}
		i++;
	}
	return map;
}


bool BinaryWorldState::FromValue(bool use_params, Value v, Event<String> WhenError) {
	atoms.Clear();
	ASSERT(mask);
	if (!mask) {return false;}
	ValueMap ws = v;
	if (v.Is<ValueMap>()) {
		ws = v;
	}
	else if (v.IsVoid()) {
		return true;
	}
	else {
		WhenError("unexpected value type " + v.GetTypeName());
		return false;
	}
	
	if (ws.IsEmpty()) {
		return true;
	}
	
	Value first_key = ws.GetKey(0);
	if (first_key.Is<int>()) {
		for(int i = 0; i < ws.GetCount(); i++) {
			int j = ws.GetKey(i);
			Value val = ws.GetValue(i);
			if (val.Is<bool>()) {
				bool v = val;
				SetMasked(j, v);
			}
			else if (val.Is<int>()) {
				int v = val;
				SetMasked(j, v);
			}
			else {
				WhenError("unexpected type: " + val.GetTypeName());
				return false;
			}
		}
	}
	#if 0
	else if (first_key.Is<String>()) {
		for(int i = 0; i < ws.GetCount(); i++) {
			String atom_name = ws.GetKey(i);
			WorldStateKey atom_key;
			if (!use_params && !mask->session->ParseRaw(atom_name, atom_key)) return false;
			if ( use_params && !mask->session->ParseDecl(atom_name, atom_key)) return false;
			auto& session = *mask->session;
			int j = session.atoms.Find(atom_key);
			if (j < 0) {WhenError("the goal atom can't be found: " + atom_name); return false;}
			auto& atom = session.atoms[j];
			Value val = ws.GetValue(i);
			if (val.Is<bool>()) {
				bool v = val;
				atom.goal = v;
				SetKey(atom_key, v);
			}
			else if (val.Is<int>()) {
				int v = val;
				atom.goal = v;
				SetKey(atom_key, v);
			}
			else {
				WhenError("unexpected type: " + val.GetTypeName());
				return false;
			}
		}
	}
	#endif
	else {
		LOG(AsJSON(v, true));
		ASSERT(0);
		return false;
	}
	
	return true;
}

String BinaryWorldState::ToString(const WorldStateKey& key) const {
	return mask->ToString(key);
}

String BinaryWorldState::ToString(int indent) const {
	String s;
	int i = 0;
	for(auto& a : atoms) {
		if (a.in_use) {
			if (!s.IsEmpty())
				s << "\n";
			if (indent > 0) s.Cat('\t', indent);
			if (mask && mask->session) {
				const auto& k = mask->keys[i];
				s << mask->session->GetKeyString(k.key);
			}
			else
				s << i;
			
			/*if (a.cls > 0)
				s << " (class " << mask->session->GetKeyString(a.cls) << ")";
			if (a.val > 0)
				s << " (init " << mask->session->GetKeyString(a.val) << ")";*/
			
			s << ": ";
			s << (a.value ? "true" : "false");
		}
		i++;
	}
	if (s.IsEmpty())
		s = "<null worldstate>";
	return s;
}

String BinaryWorldState::ToShortInlineString() const {
	String s;
	#if DBG_PRINT
	s << "(";
	for(int i = 0; i < mask->keys.GetCount(); i++) {
		if (i) s << ";";
		s << mask->ToString(mask->keys[i].key);
	}
	s << ")";
	#endif
	for(auto& a : atoms)
		s.Cat(a.in_use ? (a.value ? '|' : '_') : '.');
	return s;
}

String BinaryWorldState::ToInlineString() const {
	String s;
	ASSERT(mask && mask->session);
	if (!mask || !mask->session) return "error";
	int i = 0;
	for(auto& a : atoms) {
		if (a.in_use) {
			if (!s.IsEmpty())
				s.Cat(' ');
			const auto& key = mask->keys[i].key;
			bool put_end = false;
			for(int j = -1; j < key.max_len; j++) {
				int cls, name, val;
				if (j < 0) {
					cls = 0;
					name = key.name;
				}
				else {
					cls = key.params[j].cls;
					name = key.params[j].name;
					val = key.params[j].val;
				}
				if (cls < 0) break;
				if (j == 0) {s.Cat('('); put_end = true;}
				else if (j > 0) s.Cat(',');
				
				String str;
				if (name >= 0)
					str = mask->session->key_values[name].ToString();
				else if (val >= 0)
					str = mask->session->key_values[val].ToString();
				else
					str = "<>";
				str.Replace(" ", "_");
				if (a.value)
					str = ToUpper(str);
				else
					str = ToLower(str);
				s << str;
			}
			if (put_end) s.Cat(')');
		}
		i++;
	}
	return s;
}

void BinaryWorldState::SetIntersection(BinaryWorldState& a, BinaryWorldState& b) {
	int count = min(a.atoms.GetCount(), b.atoms.GetCount());
	atoms.SetCount(0);
	atoms.SetCount(count);
	auto A = a.atoms.Begin();
	auto B = b.atoms.Begin();
	for (auto& to : atoms) {
		if (A->in_use && B->in_use &&
			A->value != B->value) {
			to.in_use = true;
			to.value = B->value;
		}
		A++; B++;
	}
	mask = b.mask;
}

void BinaryWorldState::SetDifference(BinaryWorldState& a, BinaryWorldState& b) {
	int count = b.atoms.GetCount();
	atoms.SetCount(0);
	atoms.SetCount(count);
	auto A = a.atoms.Begin();
	auto A_end = a.atoms.End();
	auto B = b.atoms.Begin();
	for (auto& to : atoms) {
		bool add = false;
		if (B->in_use) {
			if (A == A_end)
				add = B->value;
			else
				add = A->value != B->value;
		}
		if (add) {
			to.in_use = true;
			to.value = B->value;
		}
		A = A == A_end ? A : A+1;
		B++;
	}
	mask = b.mask;
}

bool BinaryWorldState::IsEmpty() const {
	for (auto& a : atoms)
		if (a.in_use)
			return false;
	return true;
}

BinaryWorldState BinaryWorldState::GetDifference(BinaryWorldState& a, BinaryWorldState& b) {
	BinaryWorldState ws;
	ws.SetDifference(a, b);
	return ws;
}

int BinaryWorldState::GetSharedCount() const {
	int c = 0;
	for(int i = 0; i < atoms.GetCount(); i++) {
		const auto& atom = atoms[i];
		if (atom.in_use) {
			const auto& key = mask->keys[i].key;
			c += key.GetSharedCount();
		}
	}
	return c;
}







BinaryWorldStateSession::BinaryWorldStateSession() {
	key_values.Add(Value()); // add empty to first
}

int BinaryWorldStateSession::FindAtom(const Key& k) const {
	hash_t h = k.GetHashValue();
	lock.EnterRead();
	int i = atoms.Find(h);
	lock.LeaveRead();
	return i;
}

int BinaryWorldStateSession::FindAddAtom(const Key& k) {
	ASSERT(k.name >= 0);
	if (k.name < 0) return -1;
	hash_t h = k.GetHashValue();
	lock.EnterRead();
	int i = atoms.Find(h);
	lock.LeaveRead();
	if (i >= 0)
		return i;
	lock.EnterWrite();
	i = atoms.GetCount();
	auto& atom = atoms.Add(h);
	atom.key = k;
	atom.key_len = k.GetLength();
	atom.goal = false;
	atom.initial = false;
	lock.LeaveWrite();
	#if DBG_PRINT
	LOG("add atom: " << GetKeyString(k));
	#endif
	return i;
}

BinaryWorldStateSession::Item& BinaryWorldStateSession::GetAddAtom(const Key& k) {
	hash_t h = k.GetHashValue();
	lock.EnterRead();
	int i = atoms.Find(h);
	lock.LeaveRead();
	if (i >= 0)
		return atoms[i];
	lock.EnterWrite();
	i = atoms.GetCount();
	auto& atom = atoms.Add(h);
	atom.key = k;
	atom.key_len = k.GetLength();
	atom.goal = false;
	atom.initial = false;
	lock.LeaveWrite();
	#if DBG_PRINT
	LOG("add atom: " << GetKeyString(k));
	#endif
	return atom;
}

String BinaryWorldStateSession::GetKeyString(int idx) const {
	String res;
	if (idx < 0 || idx >= key_values.GetCount())
		return res;
	lock.EnterRead();
	res = key_values[idx];
	lock.LeaveRead();
	return res;
}

String BinaryWorldStateSession::GetKeyString(const Key& k) const {
	lock.EnterRead();
	String out;
	out = (String)key_values[k.name];
	int len = 0;
	for(int i = 0; i < k.max_len; i++) {
		int cls_idx = k.params[i].cls;
		if (cls_idx < 0)
			break;
		if (i == 0) out.Cat('(');
		else        out << ", ";
		ASSERT(k.params[i].name >= 0 || k.params[i].val >= 0);
		if (k.params[i].shared)
			out.Cat('&');
		if (k.params[i].name >= 0) {
			int name_idx = k.params[i].name;
			out.Cat((String)key_values[name_idx]);
		}
		if (cls_idx > 0) {
			out.Cat(":");
			out.Cat((String)key_values[cls_idx]);
		}
		if (k.params[i].val >= 0) {
			int val_idx = k.params[i].val;
			out.Cat("=");
			out.Cat(key_values[val_idx].ToString());
		}
		len++;
	}
	lock.LeaveRead();
	if (len > 0)
		out.Cat(')');
	return out;
}

WorldStateKey BinaryWorldStateSession::GetAtomKey(int atom_idx) const {
	WorldStateKey ret;
	lock.EnterRead();
	ret = atoms[atom_idx].key;
	lock.LeaveRead();
	return ret;
}

bool BinaryWorldStateSession::ParseRaw(const String& atom_name, Key& atom_key) {
	Key& k = atom_key;
	k.name = key_values.FindAdd(atom_name);
	if (k.name < 0) {return false;}
	for(int i = 0; i < WSKEY_MAX_PARAMS; i++)
		k.params[i].Clear();
	return true;
}

bool BinaryWorldStateSession::ParseDecl(const String& atom_name, Key& atom_key) {
	Key& k = atom_key;
	int i = atom_name.Find("(");
	if (i < 0) {
		String id = TrimBoth(atom_name);
		k.name = key_values.FindAdd(id);
	}
	else {
		String id = TrimBoth(atom_name.Left(i));
		k.name = key_values.FindAdd(id);
		String params = TrimBoth(atom_name.Mid(i));
		CParser p(params);
		try {
			int len = 0;
			if (p.Char('(')) {
				for(int i = 0; i < WSKEY_MAX_PARAMS; i++) {
					if (p.IsChar(')')) break;
					if (i) p.PassChar(',');
					String id = p.ReadId();
					auto& param = k.params[len++];
					param.name = key_values.FindAdd(id);
					param.cls = 0;
					if (p.Char(':')) {
						String cls = p.ReadId();
						param.cls = key_values.FindAdd(cls);
					}
					if (p.Char('=')) {
						String val = p.ReadId();
						param.val = key_values.FindAdd(val);
					}
					param.shared = true;
				}
				p.PassChar(')');
			}
			for(int i = len; i < WSKEY_MAX_PARAMS; i++)
				k.params[i].Clear();
		}
		catch (Exc e) {
			LOG("BinaryWorldStateSession::ParseDecl: error: " << e << " (input was: " + atom_name + ")");
			return false;
		}
	}
	return true;
}

bool BinaryWorldStateSession::ParseCall(const String& atom_name, Key& atom_key) {
	Key& k = atom_key;
	int i = atom_name.Find("(");
	if (i < 0) {
		String id = TrimBoth(atom_name);
		k.name = key_values.FindAdd(id);
	}
	else {
		String id = TrimBoth(atom_name.Left(i));
		k.name = key_values.FindAdd(id);
		String params = TrimBoth(atom_name.Mid(i));
		CParser p(atom_name);
		try {
			String id = p.ReadId();
			k.name = key_values.FindAdd(id);
			int len = 0;
			if (p.Char('(')) {
				for(int i = 0; i < WSKEY_MAX_PARAMS; i++) {
					if (p.IsChar(')')) break;
					if (i) p.PassChar(',');
					Value value;
					int name = -1, cls = -1;
					bool shared = false;
					if (p.IsString()) {
						value = p.ReadString();
						cls = key_values.FindAdd("string");
					}
					else if (p.IsId()) {
						name = key_values.FindAdd(p.ReadId());
						cls = 0;
						shared = true;
					}
					else if (p.IsInt()) {
						value = p.ReadInt64();
						cls = key_values.FindAdd("int");
					}
					else if (p.IsDouble()) {
						value = p.ReadDouble();
						cls = key_values.FindAdd("double");
					}
					
					auto& param = k.params[len++];
					param.name = name;
					param.val = key_values.FindAdd(value);
					param.cls = cls;
					param.shared = shared;
				}
				p.PassChar(')');
			}
			for(int i = len; i < WSKEY_MAX_PARAMS; i++)
				k.params[i].Clear();
		}
		catch (Exc e) {
			LOG("BinaryWorldStateSession::ParseCall: error: " << e);
			return false;
		}
	}
	return true;
}

bool BinaryWorldStateSession::ParseCondParam(const Key& action, const String& atom_name, Key& atom_key) {
	Key& k = atom_key;
	CParser p(atom_name);
	try {
		String act_name = p.ReadId();
		k.name = key_values.FindAdd(act_name);
		int len = 0;
		if (p.Char('(')) {
			for(int i = 0; i < WSKEY_MAX_PARAMS; i++) {
				if (p.IsChar(')')) break;
				if (i) p.PassChar(',');
				int val = -1;
				int name = -1;
				int cls = -1;
				bool shared = false;
				if (p.IsString()) {
					val = key_values.FindAdd(p.ReadString());
					cls = key_values.FindAdd("string");
				}
				else if (p.IsId()) {
					shared = true;
					String param_name = p.ReadId();
					int param_name_idx = key_values.FindAdd(param_name);
					int act_param_idx = -1;
					for(int i = 0; i < Key::max_len; i++) {
						if (action.params[i].name == param_name_idx) {
							act_param_idx = i;
							break;
						}
					}
					if (act_param_idx >= 0) {
						const Key::Param& act_param = action.params[act_param_idx];
						ASSERT(act_param.cls >= 0);
						cls = act_param.cls;
						name = act_param.name;
						val = act_param.val;
						ASSERT(param_name_idx == name);
					}
					else {
						cls = 0;
						name = param_name_idx;
					}
				}
				else if (p.IsInt()) {
					val = key_values.FindAdd(p.ReadInt64());
					cls = key_values.FindAdd("int");
				}
				else if (p.IsDouble()) {
					val = key_values.FindAdd(p.ReadDouble());
					cls = key_values.FindAdd("double");
				}
				
				auto& param = k.params[len++];
				param.cls = cls;
				param.name = name;
				param.val = val;
				param.shared = shared;
			}
			p.PassChar(')');
		}
		for(int i = len; i < WSKEY_MAX_PARAMS; i++)
			k.params[i].Clear();
	}
	catch (Exc e) {
		LOG("BinaryWorldStateSession::ParseCall: error: " << e);
		return false;
	}
	return true;
}




WorldStateKey::WorldStateKey() {
	
}

WorldStateKey::WorldStateKey(const WorldStateKey& key) {
	*this = key;
}

bool WorldStateKey::operator==(const WorldStateKey& k) const {
	bool same = name == k.name;
	for(int i = 0; i < max_len && same; i++)
		same =	same &&
				params[i].cls == k.params[i].cls &&
				params[i].name == k.params[i].name &&
				params[i].val == k.params[i].val &&
				params[i].shared == k.params[i].shared;
	return same;
}

WorldStateKey::operator hash_t() const {
	return GetHashValue();
}

hash_t WorldStateKey::GetHashValue() const {
	CombineHash ch;
	ch.Put(name);
	for(int i = 0; i < max_len; i++)
		ch.Do(params[i].cls)
		  .Do(params[i].name)
		  .Do(params[i].val)
		  .Do(params[i].shared);
	return ch;
}

bool WorldStateKey::IsEmpty() const {
	return name == -1;
}

int WorldStateKey::GetLength() const {
	int len = 0;
	for(int i = 0; i < max_len; i++) {
		if (params[i].cls < 0)
			break;
		len++;
	}
	return len;
}

int WorldStateKey::GetSharedCount() const {
	int c = 0;
	for(int i = 0; i < max_len && params[i].cls >= 0; i++)
		c = (params[i].shared ? c + 1 : c);
	return c;
}






int BinaryWorldStateMask::Find(const Key& key) const {
	int i = 0;
	for (const auto& it : keys) {
		if (it.key == key)
			return i;
		i++;
	}
	return -1;
}

int BinaryWorldStateMask::FindAdd(const Key& key, bool add_atom) {
	int i = 0;
	for (const auto& it : keys) {
		if (it.key == key)
			return i;
		i++;
	}
	ASSERT(session);
	if (!session) return -1;
	int atom_idx =
		add_atom ?
			session->FindAddAtom(key):
			session->atoms.Find(key);
#if DBG_PRINT
	if (atom_idx < 0) {LOG("failed to find atom: " << ToString(key));}
#endif
	ASSERT(atom_idx >= 0);
	if (atom_idx < 0) return -1;
	
	i = keys.GetCount();
	auto& it = keys.Add();
	it.key = key;
	it.atom_idx = atom_idx;
#if DBG_PRINT
	LOG("add mask: " << ToString(key));
#endif
	return i;
}

String BinaryWorldStateMask::ToString(const WorldStateKey& key) const {
	if (session)
		return session->GetKeyString(key);
	else
		return "<error no session>";
}


END_UPP_NAMESPACE
