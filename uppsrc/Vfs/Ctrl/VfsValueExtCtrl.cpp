#include "Ctrl.h"

NAMESPACE_UPP

VfsValueExtCtrl::VfsValueExtCtrl() {
}

VfsValueExtCtrl::~VfsValueExtCtrl() {
}

VfsValueExt& VfsValueExtCtrl::GetExt() {
	ASSERT(ext);
	return *ext;
}

VfsValue& VfsValueExtCtrl::GetValue() {
	ASSERT(ext);
	return ext->val;
}

const VfsValue& VfsValueExtCtrl::GetValue() const {
	ASSERT(ext);
	return ext->val;
}

VfsPath VfsValueExtCtrl::GetCursorPath() const {
	if (ext)
		return ext->val.GetPath();
	return VfsPath();
}

void VfsValueExtCtrl::ToolMenu(Bar& bar) {
}

END_UPP_NAMESPACE
