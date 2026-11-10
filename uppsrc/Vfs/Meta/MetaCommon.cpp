#include "Meta.h"

NAMESPACE_UPP

String GetRelSrcPath(const String& rel_path) {
	String ext = GetFileExt(rel_path);
	if(ext == ".ecs" || ext == ".env" || ext == ".db-src")
		return rel_path;
	return META_FILENAME;
}

END_UPP_NAMESPACE

