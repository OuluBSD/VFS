#include "Analysis.h"

NAMESPACE_UPP

VfsValue*         (*IdeMetaEnvironment_FindDeclaration)(const VfsValue& n);
Vector<VfsValue*> (*IdeMetaEnvironment_FindDeclarationsDeep)(const VfsValue& n);
bool (*IsStructPtr)(int kind);
bool (*IsFunctionPtr)(int kind);
bool (*IsStructKindPtr)(const VfsValue& n);

CodeVisitorProfile::CodeVisitorProfile() {
	memset(&cfg, 0, sizeof(cfg));
}

void CodeVisitorProfile::operator=(const CodeVisitorProfile& p) {
	memcpy(&cfg, &p.cfg, sizeof(cfg));
}

bool CodeVisitorProfile::Is(Rule r) const {
	ASSERT((int)r >= 0 && r < RULE_COUNT);
	return cfg[r];
}

CodeVisitorProfile& CodeVisitorProfile::Set(Rule r, bool b) {
	ASSERT((int)r >= 0 && r < RULE_COUNT);
	cfg[r] = b;
	return *this;
}

CodeVisitorProfile& CodeVisitorProfile::SetAllTrue() {
	memset(&cfg, 0xFF, sizeof(cfg));
	return *this;
}

void CodeVisitor::Item::operator=(const Item& it) {
	node = it.node;
	link_node = it.link_node;
	error = it.error;
	pos = it.pos;
	file = it.file;
	link_file = it.link_file;
}

bool CodeVisitor::Item::operator()(const Item& a, const Item& b) const {
	if(a.file != b.file)
		return a.file < b.file;
	if(a.pos.y != b.pos.y)
		return a.pos.y < b.pos.y;
	return a.pos.x < b.pos.x;
}

String CodeVisitor::Item::ToString() const {
	String s;
	if(node) {
		AstValue* ast = FindRawValue<AstValue>(node->value);
		if(ast) {
			if(node->id.GetCount() && ast->type.GetCount())
				s << node->id << ": " << ast->type;
			else if(node->id.GetCount())
				s << node->id;
			else if(ast->type.GetCount())
				s << ast->type;
		}
		else if(node->id.GetCount())
			s << node->id;
	}
	return s;
}

void CodeVisitor::Begin() {
	export_items.Clear();
	visited.Clear();
#ifdef flagASTCLANG
	macro_exps = MetaEnv().root.AstFindAllShallow(CXCursor_MacroExpansion);
	macro_defs = MetaEnv().root.AstFindAllShallow(CXCursor_MacroDefinition);
#endif
}

int CodeVisitor::FindItem(VfsValue* n) const {
	int i = 0;
	for(const auto& it : export_items) {
		if(&*it.node == n)
			return i;
		i++;
	}
	return -1;
}

void CodeVisitor::Visit(const String& filepath, VfsValue& n) {
	if(visited.Find(&n) >= 0)
		return;
	visited.Add(&n);
	AstValue& ast = RealizeRawValue<AstValue>(n.value);
	bool add_item = false;
#ifdef flagASTCLANG
	if((!n.id.IsEmpty() || !ast.type.IsEmpty()) && ast.kind != CXCursor_CXXBaseSpecifier)
		add_item = true;
	else if(ast.kind == CXCursor_ReturnStmt || ast.kind == CXCursor_MacroDefinition || ast.kind == CXCursor_MacroExpansion)
		add_item = true;
#else
	if(!n.id.IsEmpty() || !ast.type.IsEmpty())
		add_item = true;
#endif

	if(add_item) {
		Item& it = export_items.Add();
		it.pos = ast.begin;
		it.file = filepath;
		it.node = &n;
#ifdef flagASTCLANG
		if(ast.kind == CXCursor_MacroExpansion) {
			String id = n.id;
			for(VfsValue* md : macro_defs) {
				if(md->id == id) {
					it.link_node = md;
					Visit(filepath, *md);
				}
			}
		}
#endif
	}

	VisitSub(filepath, n);

#ifdef flagASTCLANG
	if(IsStructPtr && IsFunctionPtr && (IsStructPtr(ast.kind) || IsFunctionPtr(ast.kind))) {
		hash_t pkg_hash = n.pkg_hash;
		hash_t file_hash = n.file_hash;
		for(VfsValue* me : macro_exps) {
			if(me->pkg_hash == pkg_hash && me->file_hash == file_hash) {
				AstValue* ast1 = FindRawValue<AstValue>(me->value);
				if(ast1 && RangeContains(ast1->begin, ast.begin, ast.end))
					Visit(filepath, *me);
			}
		}
	}
#endif
}

void CodeVisitor::VisitSub(const String& filepath, VfsValue& n) {
	for(int i = 0; i < n.sub.GetCount(); i++) {
		VfsValue& s = n.sub[i];
		AstValue* s_ast = FindRawValue<AstValue>(s.value);
		if(s_ast && !s_ast->is_ref)
			Visit(filepath, s);
		else
			VisitRef(filepath, s);
		if(limit && export_items.GetCount() > limit)
			break;
	}
}

void CodeVisitor::VisitRef(const String& filepath, VfsValue& n) {
	AstValue* ast = FindRawValue<AstValue>(n.value);
	if(ast) {
		ASSERT(ast->is_ref);
		Item& it = export_items.Add();
		it.pos = ast->begin;
		it.file = filepath;
		it.node = &n;
		VisitId(filepath, n, it);
	}
	else
		TODO
}

void CodeVisitor::VisitId(const String& filepath, VfsValue& n, Item& link_it) {
	VisitSub(filepath, n);
	AstValue& ast = RealizeRawValue<AstValue>(n.value);
	ASSERT(ast.is_ref);
	auto& env = MetaEnv();
	VfsValue* decl = IdeMetaEnvironment_FindDeclaration ? IdeMetaEnvironment_FindDeclaration(n) : nullptr;
	if(decl) {
		link_it.link_node = decl;
		TODO
		Visit(filepath, *decl);
	}
	else {
		auto& it = export_items.Add();
		it.error = "id not found: " + n.id;
		it.pos = Point(0, 0);
		it.file = filepath;
		it.node = &n;
	}
}

const String& CodeVisitor::LoadPath(String path) {
	int i = files.Find(path);
	if(i >= 0)
		return files[i];
	String& s = files.Add(path);
	s = LoadFile(path);
	return s;
}

END_UPP_NAMESPACE

