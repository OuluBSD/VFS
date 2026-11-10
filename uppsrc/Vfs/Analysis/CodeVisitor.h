#ifndef _Vfs_Analysis_CodeVisitor_h_
#define _Vfs_Analysis_CodeVisitor_h_


struct CodeVisitorProfile
{
	typedef enum : int {
		VISIT_VARIABLE_USAGE,
		VISIT_VARIABLE_TYPE_REQ_DEF,
		VISIT_VARIABLE_TYPE_DECL,
		
		RULE_COUNT,
	} Rule;
	bool cfg[RULE_COUNT];
	
	CodeVisitorProfile();
	CodeVisitorProfile(const CodeVisitorProfile& p) {*this = p;}
	void operator=(const CodeVisitorProfile& p);
	bool Is(Rule r) const;
	CodeVisitorProfile& Set(Rule r, bool b);
	CodeVisitorProfile& SetAllTrue();
};

struct CodeVisitor
{
	struct Item {
		Ptr<VfsValue> node;
		Ptr<VfsValue> link_node;
		String error;
		Point pos;
		String file;
		String link_file;
		
		Item() {}
		Item(const Item& it) {*this = it;}
		void operator=(const Item& it);
		bool operator()(const Item& a, const Item& b) const;
		String ToString() const;
	};
	int limit = 0;
	CodeVisitorProfile prof;
	Array<Item> export_items;
	VectorMap<String,String> files;
	Index<VfsValue*> visited;
	Vector<VfsValue*> macro_exps, macro_defs;
	
	CodeVisitor() {}
	void SetLimit(int i) {limit = i;}
	void SetNoLimit() {limit = 0;}
	void SetProfile(CodeVisitorProfile& p) {prof = p;}
	void Begin();
	void Visit(const String& filepath, VfsValue& n);
	void VisitSub(const String& filepath, VfsValue& n);
	void VisitRef(const String& filepath, VfsValue& it);
	void VisitId(const String& filepath, VfsValue& n, Item& it);
	const String& LoadPath(String path);
	int FindItem(VfsValue* n) const;
};


#endif
