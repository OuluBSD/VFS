// AstValue struct extracted from legacy VfsValue header (API only)
#ifndef _Vfs_Core_AstValue_h_
#define _Vfs_Core_AstValue_h_


struct VfsValue;

struct AstValue {
	int             kind = -1;
	String          type;
	Point           begin = Null;
	Point           end = Null;
	hash_t          filepos_hash = 0;
	bool            is_ref = false;
	bool            is_definition = false;
	bool            is_disabled = false;

	// Temp
	Ptr<VfsValue>   type_ptr;

	bool IsNullInstance() const;
	void Serialize(Stream& s);
	void Xmlize(XmlIO& xml);
	void Jsonize(JsonIO& io);
	hash_t GetHashValue() const;
	String ToString() const;
	bool operator==(const AstValue& v) const;
	int Compare(const AstValue& v) const;
	int PolyCompare(const Value& v) const;
};

const dword ASTVALUE_V = 0x10001;
template<> inline dword ValueTypeNo(const AstValue*) { return ASTVALUE_V; }


#endif

