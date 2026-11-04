#include "Core.h"

NAMESPACE_UPP


bool IsSpaceMergeable(const VfsValue& n0, const VfsValue& n1)
{
	const AstValue* a0 = n1;
	const AstValue* a1 = n1;
	if (a0 && a1) {
		return	a0->kind == a1->kind && n0.id == n1.id && IsMergeable(a0->kind);
	}
	else if (a0 && !a1) {
		return	n0.id == n1.id && n0.type_hash == 0 && n1.type_hash == 0 &&
				IsMergeable(a0->kind);
	}
	else {
		return	n0.id == n1.id && n0.type_hash == 0 && n1.type_hash == 0;
	}
}

bool IsMergeable(int kind)
{
	switch(kind) {
	// case Cursor_StructDecl:
	// case Cursor_ClassDecl:
	case Cursor_Namespace:
	case Cursor_LinkageSpec:
	case 0:
		return true;
	default:
		return false;
	}
}


END_UPP_NAMESPACE
