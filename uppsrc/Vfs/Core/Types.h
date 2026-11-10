// Small common typedefs/enums extracted from legacy VfsValue API (no behavior)
#ifndef _Vfs_Core_Types_h_
#define _Vfs_Core_Types_h_

inline TypeCls AsTypeClsStatic(const std::type_info& ti) { return TypeCls(ti); }

inline bool RangeContains(const Point& pos, const Point& begin, const Point& end)
{
	if (pos.y < begin.y ||
		(pos.y == begin.y && pos.x < begin.x) ||
		pos.y > end.y ||
		(pos.y == end.y && pos.x > end.x)) {
		return false;
	}
	return true;
}


#endif
