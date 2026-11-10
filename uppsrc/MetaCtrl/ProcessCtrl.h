#ifndef _MetaCtrl_ProcessCtrl_h_
#define _MetaCtrl_ProcessCtrl_h_

struct AITask : Moveable<AITask>
{
	typedef enum : int {
		NO_REASON,
		USAGE_REF,
		USAGE_TYPE,
		USAGE_ADDR,
		USAGE_CALL,
		TYPE_INHERITANCE_DEPENDENCY,
		TYPE_INHERITANCE_DEPENDING,
		TYPE_USAGE,
		TYPE_PARENT,
		MACRO_EXPANSION,
		MACRO_DEFINITION,
		RETURN_VALUE,
		METHOD,
		FIELD,
		
		REASON_COUNT
	} Reason;
	static String GetReasonString(int i) {
		switch (i){
			case NO_REASON:						return "No reason";
			case USAGE_REF:						return "Usage: Reference";
			case USAGE_TYPE:					return "Usage: Type";
			case USAGE_ADDR:					return "Usage: Addr";
			case USAGE_CALL:					return "Usage: call addr";
			case TYPE_INHERITANCE_DEPENDENCY:	return "Inherited type";
			case TYPE_INHERITANCE_DEPENDING:	return "Inheriting this type";
			case TYPE_USAGE:					return "Type in use";
			case TYPE_PARENT:					return "Type as parent";
			case MACRO_EXPANSION:				return "Macro expansion";
			case MACRO_DEFINITION:				return "Macro definition";
			case RETURN_VALUE:					return "Return value";
			case METHOD:						return "Method";
			case FIELD:							return "Field";
			default:							return "<error>";
		}
	}
	struct Relation : Moveable<Relation> {
		Reason reason = NO_REASON;
		bool is_dependency = false;
		String file;
		Ptr<VfsValue> node, link_node;
		hash_t type_hash = 0;
	};
	
	CodeVisitor::Item vis;
	Vector<Relation> relations;
	String filepath;
	int order = -1;
	
	AITask() {}
	bool IsLinked(const AITask& t, const Relation& rel) const;
	bool HasInput(const VfsValue& n) const;
	bool HasInputLink(const VfsValue& n, bool is_dep) const;
	bool HasDepType(hash_t type_hash) const;
	bool HasReason(Reason r, Point begin) const;
	int GetDependencyCount() const;
	bool operator()(const AITask& a, const AITask& b) const {return a.order < b.order;}
};

CodeVisitorProfile& BaseAnalysisProfile();

struct MetaProcess
{
	typedef enum : int {
		FN_BASE_ANALYSIS,
		FN_COUNT
	} FnType;
	struct Error : Moveable<Error> {
		String filepath;
		Point pos;
		String msg;
	};
	struct SortItem {
		AITask& task;
		int task_i;
		bool ready = false;
		Vector<const SortItem*> deps;
		
		SortItem(AITask& t, int task_i) : task(t), task_i(task_i) {}
	};
	
	Array<AITask> tasks;
	FnType cur_fn;
	bool running = false, stopped = true;
	bool waiting = false;
	Ptr<VfsValue> node;
	Vector<String> code;
	Vector<Error> errors;
	String filepath;
	int file_idx = -1;
	int task_i = 0;
	CodeVisitor vis;
	
	typedef MetaProcess CLASSNAME;
	MetaProcess();
	~MetaProcess();
	void SetSource(String filepath, VfsValue& n, Vector<String> code);
	void Start(FnType fn);
	void Stop();
	void Run();
	void MakeBaseAnalysis();
	void SortTasks();
	bool MakeTask(AITask& t);
	bool ProcessTask(AITask& t);
	void AddError(String msg);
	void AddError(String filepath, Point pos, String msg);
	void FindDependencies(Array<SortItem>& sort_items, SortItem& it);
	void OnResult(String s);
	void Chk();
};

struct MetaProcessCtrl : ParentCtrl
{
	Splitter vsplit;
	ArrayCtrl tasks, info, errors;
	MetaProcess process;
	
	typedef MetaProcessCtrl CLASSNAME;
	MetaProcessCtrl();
	void Data();
	void DataTask();
	void RunTask(String filepath, VfsValue& n, Vector<String> code, MetaProcess::FnType fn);
};

typedef MetaProcessCtrl ProcessCtrl;


bool IsTypeKind(int kind);
bool IsVarKind(int kind);
bool IsFunctionAny(int kind);
bool IsMethodAny(int kind);
bool IsCallAny(int kind);
bool IsTypeKindBuiltIn(const String& s);

#endif
