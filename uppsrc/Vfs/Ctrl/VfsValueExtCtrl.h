#ifndef _Vfs_Ctrl_VfsValueExtCtrl_h_
#define _Vfs_Ctrl_VfsValueExtCtrl_h_

#include "WidgetCtrl.h"

NAMESPACE_UPP

struct VfsValueExtCtrl : WidgetCtrl {
	Ptr<VfsValueExt> ext;
	Event<> WhenEditorChange;
	Event<> WhenTitle;
	Event<> WhenSaveEditPos;

	VfsValueExtCtrl();
	virtual ~VfsValueExtCtrl();

	VfsValue& GetValue();
	const VfsValue& GetValue() const;
	VfsValueExt& GetExt();
	VfsPath GetCursorPath() const override;
	void ToolMenu(Bar& bar) override;

	template <class T> T& GetExt() { return dynamic_cast<T&>(*ext); }
	template <class T> T* FindExt() { return dynamic_cast<T*>(&*ext); }
};

END_UPP_NAMESPACE

#endif
