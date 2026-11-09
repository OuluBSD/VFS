#ifndef _Core_VfsBase_VfsBase_h_
#define _Core_VfsBase_VfsBase_h_

#include <Core/Core.h>
#include <Core/MetaTraits/MetaTraits.h>

NAMESPACE_UPP

inline String AppendUnixFileName(const String& path, const char *fileName) {
	String result = path;
	if(result.GetLength() && *result.Last() != '/' && *fileName != '/')
		result += '/';
	result += fileName;
	return result;
}

inline String AppendUnixFileName(String a, String b) {
	if (a.GetCount() && a[a.GetCount()-1] == '/')
		a = a.Left(a.GetCount()-1);
	if (b.GetCount() && b[0] == '/')
		b = b.Mid(1);
	return a + "/" + b;
}

inline void Jsonize(JsonIO& io, uint64& var) {
	int64 v = (int64)var;
	Jsonize(io, v);
	var = (uint64)v;
}

#ifndef _Core_Ext_ExtUtil_h_
inline String HexStr64(int64 i) { return Format("0x%X", i); }
inline String HexStrPtr(const void* i) { return Format("0x%X", (int64)(uintptr_t)i); }
template <class T> inline String HexStrPtr(const T* o) { return HexStrPtr((const void*)o); }

template <class T> inline int VectorFind(const Vector<T>& v, const T& o) {
	int i = 0;
	for(const auto& local : v) {
		if (local == o)
			return i;
		i++;
	}
	return -1;
}

template <class T> inline T& VectorGetAdd(Vector<T>& v, const T& o, int* found=0) {
	if (found) {
		int i = 0;
		for(auto& local : v) {
			if (local == o) {
				*found = i;
				return local;
			}
			i++;
		}
		*found = -1;
	}
	else {
		for(auto& i : v)
			if (i == o)
				return i;
	}
	return v.Add(o);
}

template <class T> inline int VectorFindAdd(Vector<T>& v, const T& o, T** found=0) {
	if (found) {
		int i = 0;
		for(auto& local : v) {
			if (local == o) {
				*found = &local;
				return i;
			}
			i++;
		}
		*found = 0;
	}
	else {
		for(int i = 0; i < v.GetCount(); i++)
			if (v[i] == o)
				return i;
	}
	int i = v.GetCount();
	v.Add(o);
	if (found) *found = &v[i];
	return i;
}
#endif

template <class T>
inline T* FindRawValue(Value& v) {
	if (v.Is<T>())
		return const_cast<T*>(&v.To<T>());
	return nullptr;
}

template <class T>
inline const T* FindRawValue(const Value& v) {
	if (v.Is<T>())
		return &v.To<T>();
	return nullptr;
}

template <class T>
inline T& RealizeRawValue(Value& v) {
	T* p = FindRawValue<T>(v);
	if (p) return *p;
	return CreateRawValue<T>(v);
}

inline Value ValueMapGet(const ValueMap& map, const Value& key, const Value& def = Value()) {
	int q = map.Find(key);
	return q >= 0 ? map.GetValue(q) : def;
}

END_UPP_NAMESPACE

#include "VFS.h"
#include "Mount.h"
#include "WorldState.h"
#include "VirtualNode.h"
#include "Automation.h"

#endif