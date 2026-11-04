#ifndef _Core_VfsBase_WorldState_h_
#define _Core_VfsBase_WorldState_h_

NAMESPACE_UPP



namespace Eon {
class ScriptLoader;
class ScriptLoopLoader;
class ScriptDriverLoader;
class LoopContext;
class NetContext;
}
class ActionPlanner;
class ActionNode;
struct BinaryWorldStateSession;

static const int WSKEY_MAX_PARAMS = 4;
struct WorldStateKey : Moveable<WorldStateKey> {
	static const int max_len = WSKEY_MAX_PARAMS;
	
	struct Param {
		int cls  = -1;
		int name = -1;
		int val  = -1;
		bool shared = false;
		
		void Clear() {cls = name = val = -1; shared = false;}
	};
	int name = -1;
	Param params[WSKEY_MAX_PARAMS];
	
	WorldStateKey();
	WorldStateKey(const WorldStateKey& key);
	bool operator==(const WorldStateKey& k) const;
	operator hash_t() const;
	hash_t GetHashValue() const;
	bool IsEmpty() const;
	int GetLength() const;
	int GetSharedCount() const;
};

struct BinaryWorldStateMask : Pte<BinaryWorldStateMask> {
	using Key = WorldStateKey;
	struct Item : Moveable<Item> {
		Key key;
		int atom_idx = -1;
	};
	BinaryWorldStateSession* session = 0;
	Vector<Item> keys;
	
	int Find(const Key& key) const;
	int FindAdd(const Key& key, bool add_atom=false);
	String ToString(const WorldStateKey& key) const;
};

struct BinaryWorldStateSession : Pte<BinaryWorldStateSession> {
	using Key = WorldStateKey;
	
	struct Item : Moveable<Item> {
		Key key;
		int key_len = 0;
		bool initial = 0, goal = 0;
	};
	VectorMap<hash_t,Item> atoms;
	ArrayMap<hash_t, BinaryWorldStateMask> masks;
	Index<Value> key_values;
	mutable RWMutex lock;
	
	BinaryWorldStateSession();
	int FindAtom(const Key& k) const;
	int FindAddAtom(const Key& k);
	Item& GetAddAtom(const Key& k);
	String GetKeyString(int idx) const;
	String GetKeyString(const Key& k) const;
	WorldStateKey GetAtomKey(int atom_idx) const;
	bool ParseRaw(const String& atom_name, Key& atom_key);
	bool ParseDecl(const String& atom_name, Key& atom_key);
	bool ParseCall(const String& atom_name, Key& atom_key);
	bool ParseCondParam(const Key& action, const String& atom_name, Key& atom_key);
};

struct BinaryWorldState {
	friend class ActionPlanner;
	friend class ActionPlannerWrapper;
	friend class ActionNode;
	
	struct Item : Moveable<Item> {
		bool value = false;
		bool in_use = false;
	};
	
	Ptr<BinaryWorldStateMask> mask;
	Vector<Item> atoms;
	
	BinaryWorldState();
	BinaryWorldState(const BinaryWorldState& ws);
	BinaryWorldState(BinaryWorldState&& ws);
	void Clear();
	bool SetMasked(int index, bool value);
	bool SetKey(const WorldStateKey& key, bool value, bool add_atom=false);
	bool SetAtomIndex(int atom_idx, bool value);
	BinaryWorldState& operator=(const BinaryWorldState& src);
	bool operator==(const BinaryWorldState& src) const;
	bool IsPartialMatch(const BinaryWorldState& src) const;
	hash_t GetHashValue() const;
	Value ToValue() const;
	bool FromValue(bool use_params, Value v, Event<String> WhenError=Null);
	String ToString(const WorldStateKey& key) const;
	String ToString(int indent=0) const;
	String ToInlineString() const;
	String ToShortInlineString() const;
	void SetIntersection(BinaryWorldState& a, BinaryWorldState& b);
	void SetDifference(BinaryWorldState& a, BinaryWorldState& b);
	bool IsEmpty() const;
	int GetSharedCount() const;
	
	static BinaryWorldState GetDifference(BinaryWorldState& a, BinaryWorldState& b);
};

class WorldState : public Moveable<WorldState> {

public:
	typedef enum : byte {
		INVALID,
		ADD_COMP,
	} Type;
	
protected:
	friend class ActionPlanner;
	friend class ActionPlannerWrapper;
	friend class ActionNode;
	friend class Eon::ScriptLoader;
	friend class Eon::ScriptLoopLoader;
	friend class Eon::ScriptDriverLoader;
	friend class Eon::LoopContext;
	friend class Eon::NetContext;

	ValueMap values;
	
public:
	
	WorldState();
	void Clear();
	
	bool Set(int index, bool value);
	bool Set(int index, String value);
	bool Set(const String& key, bool value);
	bool Set(const String& key, String value);
	void SetTrue(const String& key);
	void SetFalse(const String& key);
	bool IsTrue(const String& key, bool def=false) const;
	bool IsFalse(const String& key, bool def=true) const;
	bool IsFalse(int idx) const;
	bool IsUndefined(const String& key) const;
	bool IsUndefined(int idx) const;
	bool IsEmpty() const;
	int GetValueCount() const;
	void FindKeys(String key_left, Index<String>& keys) const;
	String Get(const String& key, String def="") const;
	String Get(int idx) const;
	Size GetSize(const String& cx, const String& cy, Size def=Size(0,0)) const;
	int GetInt(const String& key, int def=0) const;
	double GetDouble(const String& key, double def=0) const;
	bool GetBool(const String& key, bool def=false) const;
	String GetString(const String& key, String def="") const;
	hash_t GetHashValue() const;
	String ToString() const;
	String GetFullString() const;
	bool Contains(const WorldState& ws) const;
	bool Conflicts(const WorldState& ws) const;
	int Compare(int idx, const WorldState& ws) const;
	
	WorldState& operator=(const WorldState& src);
	
	Value GetValues() const {return values;}
	
	bool operator==(const WorldState& ws) const {return GetHashValue() == ws.GetHashValue();}
	bool operator!=(const WorldState& ws) const {return GetHashValue() != ws.GetHashValue();}
	
};


namespace Eon {
using WorldState = ::Upp::WorldState;
}



END_UPP_NAMESPACE

#endif
