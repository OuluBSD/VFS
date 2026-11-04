#include "Core.h"

#if 0
#ifdef flagAI
#include <AI/Core/Core.h>
#endif
#include <Eon/Eon.h>
#endif

NAMESPACE_UPP

String (*GetCursorKindNamePtr)(int);
String (*VfsValue_GetBasesString)(const VfsValue& v);


#define DO_TEMP_CHECK 0

int CreateTempCheck(int src) {
	static int counter = 0;
	#if DO_TEMP_CHECK
	LOG("CreateTemp " << src << ", id=" << counter);
	if (counter == 777) {
		LOG("__BREAK__");
	}
	#endif
	return counter++;
}

void ClearTempCheck(int src, int id) {
	#if DO_TEMP_CHECK
	LOG("ClearTemp " << src << " id=" << id);
	#endif
}

#undef DO_TEMP_CHECK


Vector<String> FindParentUppDirectories(const String& sub_dir)
{
	Vector<String> results;
	Vector<String> parts = Split(sub_dir, DIR_SEPS);
	for(int i = 0; i < parts.GetCount(); i++) {
		int c = parts.GetCount() - i;
		if(!c)
			continue;
		String parent_dir;
		for(int j = 0; j < c; j++) {
// a posix path always begins with the root /
#ifndef flagPOSIX
			if(!parent_dir.IsEmpty())
#endif
				parent_dir << DIR_SEPS;
			parent_dir << parts[j];
		}
		String topname = parts[c - 1];
		String upp_path = parent_dir + DIR_SEPS + topname + ".upp";
		if(!FileExists(upp_path))
			continue;
		ASSERT(parent_dir.Find(";") < 0);
		results << parent_dir;
	}
	return results;
}













bool AstValue::IsNullInstance() const {
	return	kind < 0 &&
			type.IsEmpty() &&
			begin == Null &&
			end == Null &&
			filepos_hash == 0 &&
			is_ref == false &&
			is_definition == false &&
			is_disabled == false;
}

void AstValue::Serialize(Stream& s) {
	s / kind / type % begin % end % filepos_hash;
	s % is_ref % is_definition % is_disabled;
}

void AstValue::Xmlize(XmlIO& xml) {
	xml ("kind", kind)
		("type", type)
		("begin", begin)
		("end", end)
		("filepos_hash", (int64&)filepos_hash)
		("is_ref", is_ref)
		("is_definition", is_definition)
		("is_disabled", is_disabled)
		;
}

void AstValue::Jsonize(JsonIO& io) {
	io  ("kind", kind)
		("type", type)
		("begin", begin)
		("end", end)
		("filepos_hash", (int64&)filepos_hash)
		("is_ref", is_ref)
		("is_definition", is_definition)
		("is_disabled", is_disabled)
		;
}

hash_t AstValue::GetHashValue() const {
	CombineHash c;
	c	.Do(kind)
		.Do(type)
		.Do(begin)
		.Do(end)
		.Do(filepos_hash)
		.Do(is_ref)
		.Do(is_definition)
		.Do(is_disabled)
		;
	return c;
}

String AstValue::ToString() const {
	return StoreAsJson(*this);
}

bool AstValue::operator==(const AstValue& v) const {
	return	kind == v.kind &&
			type == v.type &&
			begin == v.begin &&
			end == v.end &&
			filepos_hash == v.filepos_hash &&
			is_ref == v.is_ref &&
			is_definition == v.is_definition &&
			is_disabled == v.is_disabled;
}

int AstValue::Compare(const AstValue& v) const {
	return	kind - v.kind;
}

int AstValue::PolyCompare(const Value& v) const {
	return	kind - (int)v;
}













#if 0
VfsSrcFile& MetaEnvironment::ResolveFileInfo(const String& includes, String path)
{
	return ResolveFile(includes, path).RealizePath(includes, path);
}
#endif






MetaEnvironment& MetaEnv() { return Single<MetaEnvironment>(); }

MetaEnvironment::MetaEnvironment() {
	AstValue& ast = root;
	ast.kind = 22; //CXCursor_Namespace;
	root.serial = NewSerial();
	
	MountManager& mm = MountManager::System();
	mm.Mount(INTERNAL_ROOT_FILE("prj"), this, "MetaEnvironment");
}

hash_t MetaEnvironment::NewSerial() {
	serial_lock.Enter();
	hash_t new_hash = ++serial_counter;
	serial_lock.Leave();
	return new_hash;
}

bool MetaEnvironment::MergeVisit(Vector<VfsValue*>& scope, const VfsValue& n1, MergeMode mode)
{
	VfsValue& n0 = *scope.Top();
	
	if(IsSpaceMergeable(n0, n1)) {
		for(const VfsValue& s1 : n1.sub) {
			const AstValue* a1 = s1;
			int i = -1;
			if (a1)
				i = n0.AstFind(a1->kind, s1.id);
			else
				i = n0.Find(s1.id, s1.type_hash);
			
			if(i < 0) {
				auto& n = n0.Add(s1);
				MergeVisitPost(n);
			}
			else {
				VfsValue& sub0 = n0.sub[i];
				scope << &sub0;
				bool succ = MergeVisit(scope, s1, mode);
				scope.Pop();
				if(!succ)
					return false;
			}
		}
		n0.Chk();
	}
	else {
		return MergeVisitPartMatching(scope, n1, mode);
	}
	
	return true;
}

bool MetaEnvironment::MergeVisitPartMatching(Vector<VfsValue*>& scope, const VfsValue& n1,
                                             MergeMode mode)
{
	VfsValue& n0 = *scope.Top();
	struct Hashes {
		const VfsValue* n = 0;
		const VfsValue* match = 0;
		bool source_kind;
		hash_t hash;
		bool ready = false;
		dword serial;
		void Set(const VfsValue* mn, bool is_source_kind, hash_t c)
		{
			n = mn;
			hash = c;
			source_kind = is_source_kind;
		}
	};
	if(n0.serial == n1.serial) {
		n0.Chk();
		return true;
	}
	
	#if 0
	AstValue* p0 = n0;
	const AstValue* p1 = n1;
	ASSERT(p0 && p1); // unverified (part of cleanup)
	if (!p0 || !p1)
		return false;
	AstValue& a0 = *p0;
	const AstValue& a1 = *p1;
	#endif
	
	ASSERT(n0.serial && n1.serial);
	const VfsValue& pri = mode == MERGEMODE_OVERWRITE_OLD ? (n0.serial > n1.serial ? n0 : n1)
	                                                      : (n0.serial < n1.serial ? n0 : n1);
	const VfsValue& sec = &pri == &n0 ? n1 : n0;
	
	if (pri.sub.IsEmpty() && sec.sub.IsEmpty()) {
		if (mode == MERGEMODE_OVERWRITE_OLD || mode == MERGEMODE_UPDATE_SUBSET) {
			if (n0.serial < n1.serial)
				n0.CopyFieldsFrom(n1);
			return true;
		}
		else {
			if (n0.serial > n1.serial)
				n0.CopyFieldsFrom(n1, true);
			return true;
		}
	}
	
	Array<Hashes> pri_subs;
	Array<Hashes> sec_subs;
	pri_subs.Reserve(pri.sub.GetCount());
	sec_subs.Reserve(sec.sub.GetCount());
	for(const VfsValue& pri1 : pri.sub) {
		if (!pri1.serial)
			const_cast<VfsValue&>(pri1).serial = pri.serial; // TODO not checked to be correct
		if(pri1.IsAstValue())
			pri_subs.Add().Set(&pri1, true, pri1.AstGetSourceHash());
		else
			pri_subs.Add().Set(&pri1, false, pri1.GetTotalHash());
	}
	for(const VfsValue& sec1 : sec.sub) {
		if(sec1.IsAstValue())
			sec_subs.Add().Set(&sec1, true, sec1.AstGetSourceHash());
		else
			sec_subs.Add().Set(&sec1, false, sec1.GetTotalHash());
	}

	// Match hashes starting from the beginning
	int pri_c = pri_subs.GetCount();
	int sec_c = sec_subs.GetCount();
	for(int node_kind_mode = 0; node_kind_mode < 2; node_kind_mode++) {
		bool filter_source_kind = !node_kind_mode;

		while(1) {
			bool found_matches = false;
			for(int i = 0, j = 0; i < pri_c && j < sec_c; i++, j++) {
				auto& pri = pri_subs[i];
				auto& sec = sec_subs[j];
				if(pri.source_kind != filter_source_kind || pri.ready) {
					j--;
					continue;
				}
				else if(sec.source_kind != filter_source_kind || sec.ready) {
					i--;
					continue;
				}
				if(pri.source_kind != filter_source_kind &&
				   sec.source_kind != filter_source_kind)
					continue;
				if(pri.ready && sec.ready)
					continue;

				if(pri.hash == sec.hash) {
					pri.match = sec.n;
					sec.match = pri.n;
					pri.ready = true;
					sec.ready = true;
					found_matches = true;
				}
				else
					break;
			}
			// Match common hashes starting from the end
			for(int i = 0, j = 0; i < pri_c && j < sec_c; i++, j++) {
				int i0 = pri_c - 1 - i;
				int j0 = sec_c - 1 - j;
				auto& pri = pri_subs[i0];
				auto& sec = sec_subs[j0];
				if(pri.source_kind != filter_source_kind || pri.ready) {
					j--;
					continue;
				}
				else if(sec.source_kind != filter_source_kind || sec.ready) {
					i--;
					continue;
				}
				if(pri.source_kind != filter_source_kind &&
				   sec.source_kind != filter_source_kind)
					continue;
				if(pri.ready && sec.ready)
					continue;

				if(pri.hash == sec.hash) {
					pri.match = sec.n;
					sec.match = pri.n;
					pri.ready = true;
					sec.ready = true;
					found_matches = true;
				}
				else
					break;
			}
			if(!found_matches)
				break;
		}
	}

	bool all_pri_match = true;
	for(auto& pri : pri_subs) if(!pri.ready) {
		all_pri_match = false;
		break;
	}
	bool all_sec_match = true;
	bool all_sec_source_match = true;
	for(auto& sec : sec_subs) if(!sec.ready) {
		all_sec_match = false;
		if (sec.source_kind) {
			all_sec_source_match = false;
			break;
		}
	}

	// Copy secondary extras, if all primary matches and all secondary sources matches
	if(all_pri_match && all_sec_source_match && !all_sec_match) {
		while(true) {
			int added_count = 0;
			for(int i = 1; i < sec_subs.GetCount(); i++) {
				auto& sec = sec_subs[i];
				if(sec.ready)
					continue;
				ASSERT(!sec.source_kind);
				auto& seca = sec_subs[i - 1];
				if(!seca.ready)
					continue;
				bool added = false;
				for(int j = 0; j < pri_subs.GetCount(); j++) {
					if(pri_subs[j].match == seca.n) {
						auto& pri = pri_subs.Insert(j + 1);
						pri.match = sec.n;
						pri.ready = true;
						sec.ready = true;
						added = true;
						break;
					}
				}
				if(added)
					added_count++;
			}
			for(int i = sec_subs.GetCount() - 2; i >= 0; i--) {
				auto& sec = sec_subs[i];
				if(sec.ready)
					continue;
				ASSERT(!sec.source_kind);
				auto& seca = sec_subs[i + 1];
				if(!seca.ready)
					continue;
				bool added = false;
				for(int j = 0; j < pri_subs.GetCount(); j++) {
					if(pri_subs[j].match == seca.n) {
						auto& pri = pri_subs.Insert(j);
						pri.match = sec.n;
						pri.ready = true;
						sec.ready = true;
						added = true;
						break;
					}
				}
				if(added)
					added_count++;
			}
			int unready_count = 0;
			for(int i = 0; i < sec_subs.GetCount(); i++)
				if(!sec_subs[i].ready)
					unready_count++;
			if(!unready_count)
				break;
			if(added_count)
				continue;
			for(int i = 0; i < sec_subs.GetCount(); i++) {
				auto& sec = sec_subs[i];
				if(sec.ready)
					continue;
				ASSERT(!sec.source_kind);
				bool added = false;
				if(i > 0) {
					auto& seca = sec_subs[i - 1];
					if(!seca.ready)
						continue;
					for(int j = 0; j < pri_subs.GetCount(); j++) {
						if(pri_subs[j].match == seca.n) {
							auto& pri = pri_subs.Insert(j + 1);
							pri.match = sec.n;
							pri.ready = true;
							sec.ready = true;
							added = true;
							break;
						}
					}
				}
				if(!added && i + 1 < sec_subs.GetCount()) {
					auto& seca = sec_subs[i + 1];
					if(!seca.ready)
						continue;
					for(int j = 0; j < pri_subs.GetCount(); j++) {
						if(pri_subs[j].match == seca.n) {
							auto& pri = pri_subs.Insert(j);
							pri.match = sec.n;
							pri.ready = true;
							sec.ready = true;
							added = true;
							break;
						}
					}
				}
				if(!added && i == 0) {
					auto& pri = pri_subs.Insert(0);
					pri.match = sec.n;
					pri.ready = true;
					sec.ready = true;
					added = true;
				}
				if(!added && i == sec_subs.GetCount() - 1) {
					auto& pri = pri_subs.Add();
					pri.match = sec.n;
					pri.ready = true;
					sec.ready = true;
					added = true;
				}
				if(added)
					added_count++;
			}
			if(!added_count) {
				ASSERT_(0, "unexpected internal error"); // this shouldn't ever happen
				return false;
			}
		}
	}

	hash_t old_serial = n0.serial;
	if(mode == MERGEMODE_KEEP_OLD) {
		if(n0.serial > n1.serial && !n0.IsFieldsSame(n1)) {
			n0.CopyFieldsFrom(n1);
			n0.serial = NewSerial(); // TODO this might be too early to create new serial, if all data is same
		}
	}
	else {
		if(n0.serial < n1.serial && !n0.IsFieldsSame(n1)) {
			n0.CopyFieldsFrom(n1);
			n0.serial = NewSerial(); // TODO this might be too early to create new serial, if all data is same
		}
	}

	if(&pri == &n0) {
		int pos = 0;
		for(int i = 0; i < pri_subs.GetCount(); i++) {
			auto& pri1 = pri_subs[i];
			if(!pri1.n) {
				pri1.n = &n0.sub.Insert(pos);
				if(n0.serial == old_serial)
					n0.serial = NewSerial();
			}
			else {
				ASSERT(pri1.n == &n0.sub[pos]);
			}
			pos++;
		}

		for(auto& pri1 : pri_subs) {
			if(pri1.ready && pri1.match) {
				VfsValue& s0 = const_cast<VfsValue&>(*pri1.n);
				if (!s0.serial)
					s0.serial = pri.serial; // TODO not checked to be correct
				if (s0.ext)
					s0.ext->Initialize(env_ws);
				hash_t old_sub_serial = s0.serial;
				scope.Add(&s0);
				bool succ = MergeVisitPartMatching(scope, *pri1.match, mode);
				scope.Remove(scope.GetCount() - 1);
				if(old_sub_serial != s0.serial && n0.serial == old_serial)
					n0.serial = NewSerial();
				if(!succ)
					return false;
			}
		}
	}
	else {
		for(auto& sec : sec_subs) {
			if(sec.ready && sec.match) {
				VfsValue& s0 = const_cast<VfsValue&>(*sec.n);
				if (s0.ext)
					s0.ext->Initialize(env_ws);
				hash_t old_sub_serial = s0.serial;
				scope.Add(&s0);
				bool succ = MergeVisitPartMatching(scope, *sec.match, mode);
				scope.Remove(scope.GetCount() - 1);
				if(old_sub_serial != n0.serial && n0.serial == old_serial)
					n0.serial = NewSerial();
				if(!succ)
					return false;
			}
		}
	}
	
	n0.Chk();
	return true;
}

void MetaEnvironment::MergeVisitPost(VfsValue& n)
{
	RefreshNodePtrs(n);
	if (n.ext)
		n.ext->Initialize(env_ws);
	for(auto& s : n.sub)
		MergeVisitPost(s);
}

void MetaEnvironment::AddSeenPath(const String& path) {
	using H = typename decltype(seen_path_names)::Hash;
	H h = (H)path.GetHashValue();
	seen_lock.Enter();
	seen_path_names.Put(h, path);
	seen_lock.Leave();
}

void MetaEnvironment::AddSeenPaths(const Vector<String>& paths) {
	seen_lock.Enter();
	for (int i = 0; i < paths.GetCount(); ++i)
		AddSeenPath(paths[i]);
	seen_lock.Leave();
}

String MetaEnvironment::GetSeenPath(hash_t str_hash) const
{
	seen_lock.Enter();
	const String* known = seen_path_names.Find(str_hash);
	String ret = known ? *known : String();
	seen_lock.Leave();
	return ret;
}

bool MetaEnvironment::MergeValue(VfsValue& root, const VfsValue& other, MergeMode mode)
{
	Vector<VfsValue*> scope;
	scope << &root;
	return MergeVisit(scope, other, mode);
}

VfsValue::~VfsValue()
{
#if DEBUG_METANODE_DTOR
	if(trace_kill)
		Panic("trace-kill");
#endif
}

String VfsValue::ToString() const {
	if (ext) return ext->ToString();
	return id;
}

void VfsValue::ClearExtDeep() {
	for (auto& s : sub)
		s.ClearExtDeep();
	if (ext)
		ext.Clear();
}

void VfsValue::PointPkgHashTo(VfsValueSubset& other, hash_t pkg)
{
	other.n = this;
	for(auto& n0 : sub) {
		if(n0.HasPkgHashDeep(pkg)) {
			VfsValueSubset& n1 = other.sub.Add();
			n0.PointPkgHashTo(n1, pkg);
		}
	}
}

void VfsValue::PointPkgHashTo(VfsValueSubset& other, hash_t pkg, hash_t file)
{
	other.n = this;
	for(auto& n0 : sub) {
		if(n0.HasPkgFileHashDeep(pkg, file)) {
			VfsValueSubset& n1 = other.sub.Add();
			n0.PointPkgHashTo(n1, pkg, file);
		}
	}
}

void VfsValue::CopyPkgHashTo(VfsValue& other, hash_t pkg) const
{
	other.CopyFieldsFrom(*this, true);
	for(const auto& n0 : sub) {
		if(n0.HasPkgHashDeep(pkg)) {
			VfsValue& n1 = other.Add();
			n0.CopyPkgHashTo(n1, pkg);
		}
	}
}

void VfsValue::CopyPkgHashTo(VfsValue& other, hash_t pkg, hash_t file) const
{
	other.CopyFieldsFrom(*this, true);
	for(const auto& n0 : sub) {
		if(n0.HasPkgFileHashDeep(pkg, file)) {
			VfsValue& n1 = other.Add();
			n0.CopyPkgHashTo(n1, pkg, file);
		}
	}
}

bool VfsValue::HasPkgHashDeep(hash_t pkg) const
{
	if(this->pkg_hash == pkg)
		return true;
	for(const auto& n : sub)
		if(n.HasPkgHashDeep(pkg))
			return true;
	return false;
}

bool VfsValue::HasPkgFileHashDeep(hash_t pkg, hash_t file) const
{
	if(this->pkg_hash == pkg && this->file_hash == file)
		return true;
	for(const auto& n : sub)
		if(n.HasPkgFileHashDeep(pkg, file))
			return true;
	return false;
}

void VfsValue::SetPkgHashDeep(hash_t pkg)
{
	this->pkg_hash = pkg;
	for(auto& n : sub)
		n.SetPkgHashDeep(pkg);
}

void VfsValue::SetFileHashDeep(hash_t file)
{
	this->file_hash = file;
	for(auto& n : sub)
		n.SetFileHashDeep(file);
}

void VfsValue::SetPkgFileHashDeep(hash_t pkg, hash_t file)
{
	this->pkg_hash = pkg;
	this->file_hash = file;
	for(auto& n : sub)
		n.SetPkgFileHashDeep(pkg, file);
}

void VfsValue::SetTempDeep()
{
	#ifdef flagDEBUG
	only_temporary = true;
	#endif
	for(auto& n : sub)
		n.SetTempDeep();
}

void VfsValue::AstFix()
{
	AstValue* p = *this;
	if (!p && id.GetCount() && (
			 value.IsVoid() ||
			(value.Is<int>() && (int)value == INT_MIN)
		)
	) {
		AstValue& v = *this;
		v.kind = Cursor_Namespace;
	}
}

VfsValueExt& VfsValue::CreateExt(hash_t type_hash) {
	ASSERT(type_hash != 0);
	ext.Clear();
	VfsValueExt* o = VfsValueExtFactory::Create(type_hash, *this);
	ASSERT(o);
	ext = o;
	type_hash = o ? type_hash : 0;
	return *o;
}

void MetaEnvironment::RefreshNodePtrs(VfsValue& n)
{
	n.AstFix();
	
	AstValue* p = n;
	if (!p)
		return;
	AstValue& a = *p;
	
	ASSERT(!n.only_temporary);
	if(a.filepos_hash != 0) {
		auto& vec = this->filepos.GetAdd(a.filepos_hash).hash_nodes;
		bool found = false;
		bool found_zero = false;
		for(auto& p : vec) {
			VfsValue* ptr = &*p;
			found_zero = ptr == 0 || found_zero;
			if(ptr == &n) {
				found = true;
				break;
			}
		}
		if(!found)
			vec.Add(&n);
		if(found_zero) {
			Vector<int> rmlist;
			for(int i = 0; i < vec.GetCount(); i++)
				if(&*vec[i] == 0)
					rmlist << i;
			vec.Remove(rmlist);
		}
	}
	if(n.type_hash) {
		auto& vec = this->filepos.GetAdd(a.filepos_hash).hash_nodes;
		bool found = false;
		bool found_zero = false;
		for(auto& p : vec) {
			VfsValue* ptr = &*p;
			found_zero = ptr == 0 || found_zero;
			if(ptr == &n) {
				found = true;
				break;
			}
		}
		if(!found)
			vec.Add(&n);
		if(found_zero) {
			Vector<int> rmlist;
			for(int i = 0; i < vec.GetCount(); i++)
				if(&*vec[i] == 0)
					rmlist << i;
			vec.Remove(rmlist);
		}
	}
	if (!n.serial)
		n.serial = NewSerial();
}

String VfsValue::GetTypeString() const
{
	if (ext)
		return ext->GetTypeCls().GetName();
	const AstValue* a = *this;
	if (a && a->type.GetCount())
		return a->type;
	else if (a && a->kind > 0)
		return AstGetKindString(a->kind);
	if (type_hash) {
		int i = VfsValueExtFactory::FindTypeHashFactory(type_hash);
		if (i >= 0)
			return VfsValueExtFactory::List()[i].name;
	}
	return String();
}

String VfsValue::GetTreeString(int depth) const
{
	String s;
	s.Cat('\t', depth);
	if(1)
		s << IntStr(pkg_hash) << ":" << IntStr(file_hash) << ": ";
	if (!value.IsNull()) {
		s << value.GetTypeName();
		dword type = value.GetType();
		if (type == INT_V || type == INT64_V || type == DOUBLE_V)
			s << "(" << value.ToString() << ")";
		if (type == STRING_V) {
			String vs = value.ToString();
			if (vs.GetCount() > 13)
				vs = vs.Left(10) + "...";
			s << "(" << vs << ")";
		}
	}
	if(!id.IsEmpty())
		s << ": " << id;
	s << "(" << HexStrPtr(this) << ")";
	s << "\n";
	if (ext) {
		ASSERT(type_hash != 0);
		s.Cat('\t', depth+1);
		int fac_i = VfsValueExtFactory::FindTypeHashFactory(type_hash);
		s << "EXT: ";
		String e = ext->ToString();
		if (!e.IsEmpty())
			s << e;
		else if (fac_i >= 0) {
			const auto& fac = VfsValueExtFactory::List()[fac_i];
			s << ClassPathTop(fac.name);
			if (!fac.ctrl_name.IsEmpty())
				s << " (" << fac.ctrl_name << ")";
		}
		else {
			TypeCls type = ext->GetTypeCls();
			s << type.GetName();
		}
		s << "\n";
	}
	for(auto& n : sub)
		s << n.GetTreeString(depth + 1);
	return s;
}

int VfsValue::AstFind(int kind, const String& id) const
{
	int i = 0;
	for(const VfsValue& n : sub) {
		const AstValue* a = n;
		if(a && a->kind == kind && n.id == id)
			return i;
		i++;
	}
	return -1;
}

int VfsValue::Find(const String& id) const
{
	int i = 0;
	for(const VfsValue& n : sub) {
		if (n.id == id)
			return i;
		i++;
	}
	return -1;
}

int VfsValue::Find(const String& id, hash_t type_hash) const
{
	int i = 0;
	for(const VfsValue& n : sub) {
		if (n.id == id && n.type_hash == type_hash)
			return i;
		i++;
	}
	return -1;
}

void VfsValue::Destroy()
{
	if(!owner)
		return;
	int i = 0;
	for(VfsValue& n : owner->sub) {
		if(&n == this) {
			owner->sub.Remove(i);
			break;
		}
		i++;
	}
}

VfsValue& VfsValue::AstGetAdd(String id, String type, int kind)
{
	for(VfsValue& s : sub) {
		if (s.id != id)
			continue;
		AstValue* a = s;
		if(a && a->kind == kind && a->type == type)
			return s;
	}
	VfsValue& s = sub.Add();
	s.owner = this;
	s.id = id;
	s.serial = MetaEnv().NewSerial();
	s.file_hash = this->file_hash;
	s.pkg_hash = this->pkg_hash;
	
	AstValue& a = s;
	a.type = type;
	a.kind = kind;
	ASSERT(kind > 0 && kind < 1000); // CXCursor range
	
	#if 0
	this->serial = MetaEnv().NewSerial();
	if (kind >= METAKIND_EXTENSION_BEGIN && kind <= METAKIND_EXTENSION_END) {
		int i = VfsValueExtFactory::FindKindFactory(kind);
		if (i >= 0) {
			s.ext = VfsValueExtFactory::List()[i].new_fn(s);
			s.type_hash = s.ext->GetTypeHash();
		}
	}
	#endif
	
	return s;
}

int VfsValue::GetCount(String id) const {
	int c = 0;
	for (auto& s : sub)
		if (s.id == id)
			c++;
	return c;
}

VfsValue& VfsValue::GetAdd(String id) {
	for (auto& s : sub) {
		if (s.id == id) {
			ASSERT(GetCount(id) == 1);
			return s;
		}
	}
	return Add(id, 0);
}

VfsValue& VfsValue::GetAdd(String id, hash_t type_hash) {
	for (auto& s : sub)
		if (s.id == id && s.type_hash == type_hash)
			return s;
	return Add(id, 0);
}

void VfsValue::UninitializeDeep()
{
	for (auto& s : sub)
		s.UninitializeDeep();
	if (ext) {
		ext->UninitializeDeep();
		/*ext->Uninitialize();
		ext->SetInitialized(false);*/
		ASSERT(!ext->IsInitialized());
	}
}

void VfsValue::FindFiles(VectorMap<hash_t,VectorMap<hash_t,int>>& pkgfiles) const
{
	VectorMap<hash_t,int>& file = pkgfiles.GetAdd(pkg_hash);
	file.GetAdd(file_hash,0)++;
	
	for (const auto& s : sub)
		s.FindFiles(pkgfiles);
}

void VfsValue::StopDeep()
{
	for (auto& s : sub)
		s.StopDeep();
	if (ext)
		ext->Stop();
}

void VfsValue::ClearDependenciesDeep()
{
	for (auto& s : sub)
		s.ClearDependenciesDeep();
	if (ext)
		ext->ClearDependencies();
}

VfsValue* VfsValue::Resolve() {
	VfsValue* p = this;
	const int limit = 100;
	for(int i = 0; p->symbolic_link && i < limit; i++) {
		ASSERT(i != limit-1);
		p = p->symbolic_link;
		if (p == this) {
			ASSERT_(0,"unexpected loop");
			return 0;
		}
	}
	return p;
}

VfsValue& VfsValue::Add(const VfsValue& n)
{
	VfsValue& s = sub.Add();
	s.owner = this;
	s.CopySubFrom(n);
	s.CopyFieldsFrom(n);
	s.serial = MetaEnv().NewSerial();
	if (n.pkg_hash < 0 && n.file_hash < 0) {
		s.file_hash = this->file_hash;
		s.pkg_hash = this->pkg_hash;
	}
	this->serial = MetaEnv().NewSerial();
	s.Chk();
	return s;
}

VfsValue& VfsValue::Add(VfsValue* n)
{
	VfsValue& s = sub.Add(n);
	s.owner = this;
	if (s.pkg_hash < 0 && s.file_hash < 0) {
		s.file_hash = this->file_hash;
		s.pkg_hash = this->pkg_hash;
	}
	s.serial = MetaEnv().NewSerial();
	this->serial = MetaEnv().NewSerial();
	s.Chk();
	return s;
}

VfsValue& VfsValue::Insert(int idx, String id, hash_t h)
{
	ASSERT(idx >= 0 && idx <= sub.GetCount());
	if (idx < 0 || idx > sub.GetCount())
		throw Exc("VfsValue::Insert: error: invalid index");
	return Init(sub.Insert(idx), id, h);
}

VfsValue& VfsValue::Add(String id, hash_t h)
{
	return Init(sub.Add(), id, h);
}

void VfsValue::SetCount(int i, hash_t type_hash)
{
	int c = sub.GetCount();
	sub.SetCount(i);
	for(int i = c; i < sub.GetCount(); i++) {
		Init(sub[i], String(), type_hash);
	}
}

VfsValue& VfsValue::Init(VfsValue& s, const String& id, hash_t h)
{
	s.id = id;
	s.owner = this;
	s.file_hash = this->file_hash;
	s.pkg_hash = this->pkg_hash;
	s.serial = MetaEnv().NewSerial();
	this->serial = MetaEnv().NewSerial();
	s.Chk();
	if (h) {
		s.ext = VfsValueExtFactory::Create(h, s);
		if (s.ext)
			s.type_hash = h;
	}
	return s;
}

VfsValue& VfsValue::AstAdd(int kind, String id)
{
	VfsValue& s = Add();
	s.id = id;
	
	AstValue& a = s;
	a.kind = kind;
	ASSERT(kind > 0 && kind < 1000);
	
	s.Chk();
	return s;
}

VfsValue* VfsValue::Detach(VfsValue* n) {
	for(int i = 0; i < sub.GetCount(); i++) {
		if (&sub[i] == n)
			return sub.Detach(i);
	}
	return 0;
}

VfsValue* VfsValue::Detach(int i) {
	if (i >= 0 && i < sub.GetCount())
		return sub.Detach(i);
	return 0;
}

VfsValue& VfsValue::GetRoot() {
	VfsValue* v = this;
	while (v->owner)
		v = v->owner;
	return *v;
}

void VfsValue::Remove(VfsValue* n) {
	int i = 0;
	for (auto& s : sub) {
		if (&s == n) {
			sub.Remove(i);
			break;
		}
		i++;
	}
}

void VfsValue::Remove(int i) {
	if (i >= 0 && i < sub.GetCount())
		sub.Remove(i);
}

VfsValue* VfsValue::FindPath(const VfsPath& path) {
	VfsValue* n = this;
	for(const Value& part : path.Parts()) {
		int i = n->Find(part);
		if (i < 0)
			return 0;
		n = &n->sub[i];
	}
	return n;
}

void VfsValue::Assign(VfsValue* owner, const VfsValue& n)
{
	this->owner = owner;
	CopyFieldsFrom(n);
	CopySubFrom(n);
}

void VfsValue::CopyFrom(const VfsValue& n)
{
	CopyFieldsFrom(n);
	CopySubFrom(n);
}

void VfsValue::CopySubFrom(const VfsValue& n)
{
	int c = n.sub.GetCount();
	sub.SetCount(c);
	for(int i = 0; i < c; i++)
		sub[i].Assign(this, n.sub[i]);
}

String VfsValue::AstGetKindString() const {
	const AstValue* ast = *this;
	ASSERT(ast);
	if (!ast) return "not a AstValue";
	if (GetCursorKindNamePtr)
		return GetCursorKindNamePtr(ast->kind);
	else
		return IntStr(ast->kind);
}

String VfsValue::AstGetKindString(int kind)
{
	if (GetCursorKindNamePtr)
		return GetCursorKindNamePtr(kind);
	else
		return IntStr(kind);
}

bool VfsValue::FindDifferences(const VfsValue& n, Vector<String>& diffs, int max_diffs) const
{
	bool had_diff = false;
	#define CHK_FIELD(x)                                                                           \
	if(x != n.x) {                                                                             \
		had_diff = true;                                                                       \
		diffs.Add("Different " #x ": " + AsString(x) + " vs " + AsString(n.x) + " at " +       \
		          ", " + id + " (" + HexStr64(n.type_hash) + ")");                               \
	}
	CHK_FIELD(id);
	CHK_FIELD(type_hash);
	CHK_FIELD(serial);
	CHK_FIELD(file_hash);
	CHK_FIELD(sub.GetCount());
	CHK_FIELD(pkg_hash);
	const AstValue* a0 = *this;
	const AstValue* a1 = n;
	if (a0 && a1) {
	#define CHK_AST_FIELD(x)                                                                           \
		if(a0->x != a1->x) {                                                                             \
			had_diff = true;                                                                       \
			diffs.Add("Different " #x ": " + AsString(a0->x) + " vs " + AsString(a1->x) + " at " +       \
			          AstGetKindString(a0->kind) + ", " + id + " (" + a0->type + ")");                            \
		}
		CHK_AST_FIELD(kind);
		CHK_AST_FIELD(type);
		CHK_AST_FIELD(begin);
		CHK_AST_FIELD(end);
		CHK_AST_FIELD(filepos_hash);
		CHK_AST_FIELD(is_ref);
		CHK_AST_FIELD(is_definition);
		CHK_AST_FIELD(is_disabled);
	}
	else if (!a0 && !a1) {
		CHK_FIELD(value);
	}
	else {
		had_diff = true;
		diffs.Add("Different value type: " + value.GetTypeName() + " vs " + n.value.GetTypeName());
	}
	
	if(!had_diff) {
		for(int i = 0; i < sub.GetCount(); i++) {
			CHK_FIELD(sub[i].id);
			if(had_diff)
				break;
			had_diff = sub[i].FindDifferences(n.sub[i], diffs, max_diffs);
			if(had_diff || diffs.GetCount() >= max_diffs)
				break;
		}
	}
	#undef CHK_FIELD
	#undef CHK_AST_FIELD
	return had_diff;
}

bool VfsValue::IsFieldsSame(const VfsValue& n) const
{
	if ((bool)ext != (bool)n.ext)
		return false;
	
	if (!( id == n.id && type_hash == n.type_hash &&
	       file_hash == n.file_hash && pkg_hash == n.pkg_hash &&
	       serial == n.serial && value.GetType() == n.value.GetType()))
		return false;
	
	const AstValue* a0 = *this;
	const AstValue* a1 = n;
	if (a0 && a1) {
		auto& a = *a0;
		auto& b = *a1;
	    if(!(	a.kind == b.kind && a.type == b.type &&
				a.begin == b.begin && a.end == b.end && a.filepos_hash == b.filepos_hash &&
				a.is_ref == b.is_ref &&
				a.is_definition == b.is_definition && a.is_disabled == b.is_disabled))
			return false;
	}
	
	if (ext && n.ext && !(*ext == *n.ext)) // kind is already checked to be the same, and both has ext
		return false;
	
	return true;
}

void VfsValue::CopyFieldsFrom(const VfsValue& n, bool forced_downgrade)
{
	id = n.id;
	type_hash = n.type_hash;
	file_hash = n.file_hash;
	pkg_hash = n.pkg_hash;
	
	const AstValue* a1 = n;
	if (a1) {
		AstValue& a = *this;
		const AstValue& b = *a1;
		a.kind = b.kind;
		a.type = b.type;
		a.begin = b.begin;
		a.end = b.end;
		a.filepos_hash = b.filepos_hash;
		a.is_ref = b.is_ref;
		a.is_definition = b.is_definition;
		a.is_disabled = b.is_disabled;
	}
	else
		value = n.value;
	
	if (n.ext) {
		if (ext && ext->GetTypeHash() == n.ext->GetTypeHash()) {
			ext->CopyFrom(*n.ext);
		}
		else {
			ext = VfsValueExtFactory::Clone(*n.ext, *this);
			type_hash = ext->GetTypeHash();
		}
	}
	else
		ext.Clear();
	
	ASSERT(serial <= n.serial || forced_downgrade);
	serial = n.serial;
	Chk();
}

hash_t VfsValue::GetTotalHash() const
{
	CombineHash ch;
	ch	.Do(id)
		.Do(type_hash)
		.Do(serial)
		.Do(file_hash)
		.Do(pkg_hash) // TODO ensure that the hash is not used across sessions. This is not persistent hash because of this field.
	;
	const AstValue* a0 = *this;
	if (a0) {
		const AstValue& a = *a0;
		ch
			.Do(a.kind)
			.Do(a.type)
			.Do(a.begin)
			.Do(a.end)
			.Do(a.filepos_hash)
			.Do(a.is_ref)
			.Do(a.is_definition)
			.Do(a.is_disabled);
	}
	else
		ch.Do(value);
	
	if (ext)
		ch.Put(ext->GetHashValue());
	
	for(const auto& s : sub)
		ch.Put(s.GetTotalHash());
	
	return ch;
}

void VfsValue::Visit(Vis& v) {
	v.SetScope(this);
	#define Do(x) (#x,x)
	#define Do64(x) (#x,(int64&)x)
	v.Ver(4,true);
	if (v.file_ver >= 0 && v.file_ver <= 2) {
		int file = -1;
		AstValue& a = *this;
		v
		(1)	Do(a.kind)
			Do(id)
			Do(a.type)
			Do64(type_hash)
			Do(a.begin)
			Do(a.end)
			Do64(a.filepos_hash)
			Do(file)
			//Do(pkg)
			Do(a.is_ref)
			Do(a.is_definition)
			Do(a.is_disabled)
			Do64(serial)
		(2)	Do(value)
			;
	}
	else if (v.file_ver == 3) {
		int file = -1;
		v
		(3)	Do(id)
			Do64(type_hash)
			Do64(serial)
			Do(file)
			//Do(pkg)
			;
		
		bool is_ast = (const AstValue*)*this;
		v	Do(is_ast);
		if (is_ast) {
			AstValue& a = *this;
			v
				Do(a.kind)
				Do(a.type)
				Do(a.begin)
				Do(a.end)
				Do64(a.filepos_hash)
				Do(a.is_ref)
				Do(a.is_definition)
				Do(a.is_disabled);
		}
		else {
			v	Do(value);
		}
	}
	else if (v.file_ver == 4) {
		v
		(4)	Do(id)
			Do64(type_hash)
			Do64(serial)
			Do(file_hash)
			//Do(pkg)
			;
		
		bool is_ast = (const AstValue*)*this;
		v	Do(is_ast);
		if (is_ast) {
			AstValue& a = *this;
			v
				Do(a.kind)
				Do(a.type)
				Do(a.begin)
				Do(a.end)
				Do64(a.filepos_hash)
				Do(a.is_ref)
				Do(a.is_definition)
				Do(a.is_disabled);
		}
		else {
			v	Do(value);
		}
	}
	
	if (type_hash) {
		if (v.IsLoading()) {
			if (ext && ext->GetTypeHash() != type_hash)
				ext.Clear();
			if (ext.IsEmpty())
				ext = VfsValueExtFactory::Create(type_hash, *this);
			if (ext) {
				type_hash = ext->GetTypeHash();
				v("ext",*ext, VISIT_NODE);
			}
			else {
				//RLOG("VfsValue::Visit: error: could not load type_hash " + HexStr64(type_hash));
				type_hash = 0;
			}
		}
		else {
			if (type_hash) {
				if (ext)
					v("ext",*ext, VISIT_NODE);
			}
			else {
				ASSERT(!ext);
			}
		}
	}
	else {
		if (v.IsLoading())
			ext.Clear();
		else {
			ASSERT(!ext);
		}
	}
	v
		("sub", sub, VISIT_VECTOR)
		;
	#undef Do
	if (v.IsLoading())
		FixParent();
	
	#if 1
	if (v.IsLoading())
		Chk();
	#endif
}

void VfsValue::DeepChk() {
	#ifdef flagDEBUG
	Chk();
	for (auto& s : sub)
		s.DeepChk();
	#endif
}

void VfsValue::Chk() {
	#ifdef flagDEBUG
	if (owner && type_hash) {
		if (type_hash == AsTypeHash<Entity>()) {
			ASSERT(owner->type_hash == AsTypeHash<Engine>() ||
				   owner->type_hash == 0);
		}
		else if (VfsValueExtFactory::IsType(type_hash, VFSEXT_COMPONENT)) {
			if (owner->type_hash != AsTypeHash<Entity>()) {
				LOG(GetRoot().GetTreeString());
				LOG(owner->GetTreeString());
			}
			ASSERT(owner->type_hash == AsTypeHash<Entity>() ||
				   owner->type_hash == 0);
		}
	}
	#endif
}

bool VfsValue::IsOwnerDeep(VfsValueExt& n) const {
	const VfsValue* p = this;
	while (p) {
		if (p->ext && (&*p->ext) == &n)
			return true;
		p = p->owner;
	}
	return false;
}

int VfsValue::GetCount() const {
	return sub.GetCount();
}

int VfsValue::GetDepth() const {
	int depth = 0;
	VfsValue* n = owner;
	while (n) {
		depth++;
		n = n->owner;
	}
	return depth;
}

int VfsValue::GetDepth(const VfsValue* limit) const {
	int depth = 0;
	VfsValue* n = owner;
	while (n) {
		depth++;
		if (n == limit)
			break;
		n = n->owner;
	}
	return depth;
}

String VfsValue::GetSourceHashDump(int indent, bool* total_hash_diffs) const
{
	String s;
	const AstValue* a = *this;
	// Tree of AstValue is required
	if (!a || (a->kind < 0 || a->kind >= 1000)) {
		if(total_hash_diffs)
			*total_hash_diffs = true;
		return s;
	}
	s.Cat('\t', indent);
	s << id << ":" << a->kind << ", " << a->type << "\n";
	for(const auto& n : sub) {
		bool b = false;
		String o = n.GetSourceHashDump(indent+1, &b);
		if (b)
			continue;
		s << o;
	}
	return s;
}

hash_t VfsValue::AstGetSourceHash(bool* total_hash_diffs) const
{
	const AstValue* a = *this;
	// Tree of AstValue is required
	if (!a || (a->kind < 0 || a->kind >= 1000)) {
		if(total_hash_diffs)
			*total_hash_diffs = true;
		return 0;
	}
	CombineHash ch;
	ch.Do(a->kind).Do(id).Do(a->type);
	for(const auto& s : sub) {
		bool b = false;
		hash_t h = s.AstGetSourceHash(&b);
		if (b)
			continue;
		ch.Put(h);
	}
	return ch;
}

Vector<VfsValue*> VfsValue::FindAll(TypeCls type)
{
	Vector<VfsValue*> vec;
	for(auto& s : sub)
		if(s.ext && s.ext->GetTypeCls() == type)
			vec << &s;
	return vec;
}

Vector<VfsValue*> VfsValue::FindTypeAllShallow(hash_t type_hash)
{
	Vector<VfsValue*> vec;
	for(auto& s : sub) {
		if (s.type_hash == type_hash)
			vec << &s;
	}
	return vec;
}

Vector<Ptr<VfsValue>> VfsValue::FindTypeAllShallowPtr(hash_t type_hash)
{
	Vector<Ptr<VfsValue>> vec;
	for(auto& s : sub) {
		if (s.type_hash == type_hash)
			vec << &s;
	}
	return vec;
}

Vector<VfsValue*> VfsValue::FindAllShallow(hash_t type_hash)
{
	Vector<VfsValue*> vec;
	for(auto& s : sub) {
		if (s.type_hash == type_hash)
			vec << &s;
	}
	return vec;
}

Vector<VfsValue*> VfsValue::AstFindAllShallow(int kind)
{
	Vector<VfsValue*> vec;
	for(auto& s : sub) {
		const AstValue* a = s;
		if (a && a->kind == kind)
			vec << &s;
	}
	return vec;
}

VfsValue* VfsValue::FindDeep(TypeCls type)
{
	if (ext && ext->GetTypeCls() == type)
		return this;
	for (VfsValue& n : sub)
		if (auto r = n.FindDeep(type))
			return r;
	return 0;
}

void VfsValue::AstFindAllDeep(int kind, Vector<VfsValue*>& out)
{
	const AstValue* a = *this;
	if(a && a->kind == kind)
		out << this;
	for(auto& s : sub)
		s.AstFindAllDeep(kind, out);
}

void VfsValue::AstFindAllDeep(int kind, Vector<const VfsValue*>& out) const
{
	const AstValue* a = *this;
	if(a && a->kind == kind)
		out << this;
	for(const auto& s : sub)
		s.AstFindAllDeep(kind, out);
}

Vector<const VfsValue*> VfsValue::AstFindAllShallow(int kind) const
{
	Vector<const VfsValue*> vec;
	for(const auto& s : sub) {
		const AstValue* a = s;
		if(a && a->kind == kind)
			vec << &s;
	}
	return vec;
}

bool VfsValue::IsAstValue() const {
	const AstValue* v = *this;
	return v;
}

int VfsValue::GetAstValueCount() const
{
	int c = 0;
	for(const auto& s : sub) {
		const AstValue* a = s;
		if (a)
			c++;
	}
	return c;
}

String VfsValue::GetBasesString() const
{
	if (VfsValue_GetBasesString)
		return VfsValue_GetBasesString(*this);
	else
		return String();
}

String VfsValue::GetNestString() const
{
	if(owner)
		return owner->id;
	return String();
}

bool VfsValue::OwnerRecursive(const VfsValue& n) const
{
	VfsValue* o = this->owner;
	while(o) {
		if(o == &n)
			return true;
		o = o->owner;
	}
	return false;
}

bool VfsValue::ContainsDeep(const VfsValue& n) const
{
#if 1
	return n.OwnerRecursive(*this) || &n == this;
#else
	if(this == &n)
		return true;
	for(const auto& s : sub)
		if(s.ContainsDeep(n))
			return true;
	return false;
#endif
}

void VfsValue::AstRemoveAllShallow(int kind)
{
	Vector<int> rmlist;
	int i = 0;
	for(auto& s : sub) {
		const AstValue* a = s;
		if(a && a->kind == kind)
			rmlist << i;
		i++;
	}
	if(!rmlist.IsEmpty())
		sub.Remove(rmlist);
}

void VfsValue::AstRemoveAllDeep(int kind)
{
	AstRemoveAllShallow(kind);
	for(auto& s : sub)
		s.AstRemoveAllDeep(kind);
}

void VfsValue::GetTypeHashes(Index<hash_t>& type_hashes) const
{
	if(type_hash)
		type_hashes.FindAdd(type_hash);
	for(auto& s : sub)
		s.GetTypeHashes(type_hashes);
}

void VfsValue::RealizeSerial() {
	if (!serial)
		serial = MetaEnv().NewSerial();
	for (auto& s : sub)
		s.RealizeSerial();
}

Vector<Ptr<VfsValueExt>> VfsValue::GetAllExtensions() {
	Vector<Ptr<VfsValueExt>> v;
	for (auto& s : sub) {
		if (s.ext) {
			v.Add(&*s.ext);
		}
	}
	return v;
}

VfsPath VfsValue::GetPath() const {
	static const int LIMIT = 64;
	const VfsValue* ptrs[LIMIT];
	int i = 1;
	ptrs[0] = this;
	while (i < LIMIT-1) {
		if (ptrs[i-1]->owner) {
			const VfsValue* p = ptrs[i-1]->owner;
			ptrs[i++] = p;
		}
		else break;
	}
	if (i == 1) return VfsPath();
	ptrs[i] = 0;
	const VfsValue** iter = ptrs+i-2;
	const VfsValue** end = ptrs-1;
	Vector<Value> path;
	path.Reserve(i);
	while (iter != end) {
		path.Add((*iter)->id);
		iter--;
	}
	return path;
}

VfsValue& VfsValue::operator[](int i) {
	VfsValue& n = sub[i];
	if (n.symbolic_link) {
		Vector<hash_t> visited;
		visited << (hash_t)this;
		VfsValue* l = n.symbolic_link;
		while (l) {
			int i = VectorFind(visited, (hash_t)l);
			if (i >= 0) {
				l = n.symbolic_link; // reset
				break;
			}
			visited << (hash_t)l;
			if (!l->symbolic_link)
				break;
			l = l->symbolic_link;
		}
		return *l;
	}
	return n;
}

VirtualNode VfsValue::RootPolyValue() {
	VirtualNode root;
	VfsPath root_path; // empty
	if (value.Is<AstValue>()) {
		LOG("VfsValue::RootPolyValue: warning: resetting AstValue to Value");
		value = Value();
	}
	auto& data = root.CreateValue(root_path, &value);
	return root;
}

VirtualNode VfsValue::RootVfsValue(const VfsPath& root_path) {
	VirtualNode root;
	auto& data = root.Create(root_path, this);
	data.vfs_value = this;
	return root;
}

VfsValue::operator AstValue& () {
	return RealizeRawValue<AstValue>(value);
}

VfsValue::operator AstValue* () {
	return FindRawValue<AstValue>(value);
}

VfsValue::operator const AstValue* () const {
	return FindRawValue<AstValue>(value);
}



/*void MetaEnvironment::Store(const String& includes, const String& path, FileAnnotation& fa)
{
    VfsSrcPkg& af = ResolveFile(includes, path);
    af.Store(includes, path, fa);
}*/

/*void VfsSrcPkg::Store(const String& includes, const String& path, FileAnnotation& fa)
{
    VfsSrcFile& afi = RealizePath(includes, path);
    afi.UpdateLinks(fa);
    Save();
}*/

/*bool VfsValue::IsClassTemplateDefinition() const {
    if (kind == CXCursor_ClassTemplate)
        //for (const VfsValue& s : sub)
        //	if (s.kind == CXCursor_CompoundStmt)
                return true;
    return false;
}*/

VfsValue* MetaEnvironment::FindTypeDeclaration(hash_t type_hash)
{
	if(!type_hash)
		return 0;
	int i = types.Find(type_hash);
	if(i < 0)
		return 0;
	const auto& vec = types[i].hash_nodes;
	for(const auto& ptr : vec) {
		if(!ptr)
			continue;
		VfsValue& p = *ptr;
		const AstValue* a1 = p;
		if(a1 && a1->is_definition /* || p.IsClassTemplateDefinition()*/)
			return &p;
	}
	return 0;
}

// Overlay support methods
Vector<String> MetaEnvironment::List(String logical_path) const {
	// Delegate to the root VfsValue
	Vector<String> result;
	
	// Parse the logical path and navigate the VFS tree
	VfsPath path = StrVfs(logical_path);
	const VfsValue* current = &root;
	
	// Navigate to the specified path
	const auto& parts = path.Parts();
	for (int i = 0; i < parts.GetCount() && current; i++) {
		const String& part = parts[i].ToString();
		int idx = current->Find(part);
		if (idx >= 0) {
			current = &current->sub[idx];
		} else {
			current = nullptr;
		}
	}
	
	// List children if we found the node
	if (current) {
		for (int i = 0; i < current->sub.GetCount(); i++) {
			result.Add(current->sub[i].id);
		}
	}
	
	return result;
}

Value MetaEnvironment::GetMerged(String logical_path) const {
	// Return merged JSON representation for debugging/inspection
	// This would typically merge values from multiple overlays
	
	VfsPath path = StrVfs(logical_path);
	const VfsValue* current = &root;
	
	// Navigate to the specified path
	const auto& parts = path.Parts();
	for (int i = 0; i < parts.GetCount() && current; i++) {
		const String& part = parts[i].ToString();
		int idx = current->Find(part);
		if (idx >= 0) {
			current = &current->sub[idx];
		} else {
			current = nullptr;
		}
	}
	
	// Convert the found node to a JSON value for inspection
	if (current) {
		return StoreAsJson(*current);
	}
	
	return Value();
}

// TODO: Fix namespace collision with VfsOverlay before uncommenting
// void MetaEnvironment::AddOverlay(Ptr<VfsOverlay> overlay) {
// 	// In a real implementation, this would register the overlay
// 	// For now, we'll just acknowledge it
// 	// The following conditional requires complete VfsOverlay type, so we avoid it for now
// 	// if (overlay) {
// 		// Store reference to overlay for future use
// 		// This is a placeholder implementation
// 	// }
// }

hash_t MetaEnvironment::RealizeTypePath(const String& path)
{
	hash_t h = path.GetHashValue();
	types.GetAdd(h).seen_type = path;
	return h;
}

bool MetaEnvironment::GetFiles(const VfsPath& rel_path, Vector<Shared<VfsItem>>& items) {
	// Note: appends to items
	VfsValue* n = &this->root;
	const auto& parts = rel_path.Parts();
	for(int i = 0; i < parts.GetCount(); i++) {
		const String& part = parts[i];
		int j = n->Find(part);
		if (j < 0)
			return false;
		n = &n->sub[j];
	}
	for(int i = 0; i < n->sub.GetCount(); i++) {
		VfsValue& n0 = n->sub[i];
		VfsItem& it = items.Add().Create<VfsItem>();
		it.name = n0.id;
		it.type = VFS_DIRECTORY;
		const AstValue* a0 = n0;
		if (a0 && !a0->type.IsEmpty())
			it.type_str = a0->type;
		else if (n0.ext)
			it.type_str = n0.ext->GetName();
	}
	return true;
}

VfsItemType MetaEnvironment::CheckItem(const VfsPath& rel_path) {
	VfsValue* n = &this->root;
	const auto& parts = rel_path.Parts();
	for(int i = 0; i < parts.GetCount(); i++) {
		const String& part = parts[i];
		int j = n->Find(part);
		if (j < 0)
			return VFS_NULL;
		n = &n->sub[j];
	}
	return VFS_DIRECTORY;
}



















VfsValueExt::VfsValueExt(VfsValue& n) : val(n) {
	
}

VfsValueExt::~VfsValueExt() {
	UninitializeDeep();
}

String VfsValueExt::GetName() const {return String();}
double VfsValueExt::GetUtility() {ASSERT_(0, "Not implemented"); return 0;}
double VfsValueExt::GetEstimate() {ASSERT_(0, "Not implemented"); return 0;}
double VfsValueExt::GetDistance(VfsValue& dest) {ASSERT_(0, "Not implemented"); return 0;}
bool VfsValueExt::GenerateSubValues(const Value& params, NodeRoute& prev) {return true;}
bool VfsValueExt::TerminalTest() {ASSERT_(0, "Not implemented"); return true;}
String VfsValueExt::ToString() const {return String();}
bool VfsValueExt::Initialize(const WorldState& ws) {SetInitialized(true); return true;}
bool VfsValueExt::PostInitialize() {return true;}
void VfsValueExt::Uninitialize() {}
bool VfsValueExt::IsInitialized() const {return is_initialized;}
void VfsValueExt::SetInitialized(bool b) {is_initialized = b;}
String VfsValueExt::GetCategory() {return String();}
void VfsValueExt::Update(double dt) {
	// Default implementation does nothing
	// Derived classes should override this if they need update functionality
}
bool VfsValueExt::Arg(String key, Value value) {ASSERT_(0,"unimplemented"); return false;}
bool VfsValueExt::Start() {return true;}
void VfsValueExt::Stop() {}
void VfsValueExt::UninitializeDeep() {
	if (IsInitialized()) {
		Uninitialize();
		SetInitialized(false);
		ClearDependencies();
	}
}
bool VfsValueExt::AddDependency(VfsValueExt& val) {
	for (auto& d : deps)
		if (d == &val)
			return true;
	deps.Add(&val);
	return true;
}
VfsValueExtPtr VfsValueExt::GetDependency(int i) const {
	return i >= 0 && i < deps.GetCount() ? deps[i] : VfsValueExtPtr();
}
int VfsValueExt::GetDependencyCount() const {
	return deps.GetCount();
}
void VfsValueExt::RemoveDependency(int i) {
	if (i >= 0 && i < deps.GetCount())
		deps.Remove(i);
}
void VfsValueExt::ClearDependencies() {
	deps.Clear();
}
void VfsValueExt::RemoveDependency(const VfsValueExt* e) {
	for(int i = 0; i < deps.GetCount(); i++)
		if (deps[i] == e)
			deps.Remove(i--);
}
String VfsValueExt::GetTreeString(int indent) const {
	String s;
	if (indent)
		s.Cat('\t', indent);
	s += val.id;
	s += ": " + ToString();
	return s;
}
bool VfsValueExt::GetAll(Vector<VirtualNode>&) {
	return false;
}
void VfsValueExt::CopyFrom(const VfsValueExt& e) {
	StringStream s;
	s.SetStoring();
	Vis read(s);
	const_cast<VfsValueExt&>(e).Visit(read); // reading
	s.SetLoading();
	s.Seek(0);
	Vis write(s);
	Visit(write);
}

bool VfsValueExt::operator==(const VfsValueExt& e) const {
	return GetHashValue() == e.GetHashValue();
}

hash_t VfsValueExt::GetHashValue() const {
	Vis vis(1);
	const_cast<VfsValueExt*>(this)->Visit(vis);
	return vis.hash;
}

int VfsValueExt::AstGetKind() const {
	const AstValue* a = val;
	return a ? a->kind : 0;
}

void VfsValueExt::Serialize(Stream& s){
	Vis vis(s);
	const_cast<VfsValueExt*>(this)->Visit(vis);
}

void VfsValueExt::Jsonize(JsonIO& json){
	Vis vis(json);
	const_cast<VfsValueExt*>(this)->Visit(vis);
}

void VfsValue::Jsonize(JsonIO& io) {
	io
		("id", id)
		("type_hash", type_hash)
		("serial", serial)
		("file_hash", file_hash)
		("sub", sub)
		("value", value)
		("pkg_hash", pkg_hash)
	#ifdef flagDEBUG
		("only_temporary", only_temporary)
	#endif
	#if DEBUG_METANODE_DTOR
		("trace_kill", trace_kill)
	#endif
	;
	
	// Don't serialize complex fields that may not have Jsonize methods
	Value null_val;
	io("ext", null_val)("symbolic_link", null_val)("owner", null_val);

	if(io.IsLoading()) {
		for(auto& a : sub)
			a.owner = this;
	}
}

END_UPP_NAMESPACE
