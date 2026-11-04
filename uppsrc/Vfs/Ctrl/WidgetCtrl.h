#ifndef _Vfs_Ctrl_WidgetCtrl_h_
#define _Vfs_Ctrl_WidgetCtrl_h_

#include <CtrlLib/CtrlLib.h>
#include <Vfs/Core/Core.h>

NAMESPACE_UPP

class WidgetCtrl : public Ctrl {
public:
	typedef WidgetCtrl CLASSNAME;
	WidgetCtrl();
	virtual ~WidgetCtrl();

	virtual void Data() = 0;
	virtual void DataTree(TreeCtrl& tree);
	virtual void ToolMenu(Bar& bar);
	virtual void EditPos(JsonIO& json) {}
	virtual void Initialize(Value args) {}
	virtual String GetTitle() const { return String(); }
	virtual Value* GetPolyValue() { return nullptr; }
	virtual VfsPath GetCursorPath() const = 0;

	Event<> WhenTitle;
};

END_UPP_NAMESPACE

#endif
