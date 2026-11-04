#ifndef _Vfs_Ctrl_VfsTreeApp_h_
#define _Vfs_Ctrl_VfsTreeApp_h_

#include "VfsValueExtCtrl.h"

NAMESPACE_UPP

class VfsTreeApp : public Ctrl {
public:
	typedef VfsTreeApp CLASSNAME;

	Splitter             splitter;
	TreeCtrl             tree;
	ParentCtrl           right_pane;
	Label                placeholder;
	One<VfsValueExtCtrl> ext_ctrl;
	hash_t               ext_ctrl_type_hash = 0;
	VfsValue             root;
	Vector<VfsValue*>    tree_nodes;
	String               current_file_path;

	Event<VfsValue&>     WhenNodeSelected;
	Event<VfsValue&>     WhenNodeModified;
	Event<>              WhenTreeChanged;

	VfsTreeApp();
	virtual ~VfsTreeApp();

	void                 SetRoot(const String& name = "Workspace", hash_t type_hash = 0);
	VfsValue&            GetRoot() { return root; }
	TreeCtrl&            GetTree() { return tree; }
	ParentCtrl&          GetRightPane() { return right_pane; }
	VfsValueExtCtrl*     GetActiveCtrl() { return ~ext_ctrl; }
	VfsValue*            GetSelectedNode();

	void                 SyncTree();
	void                 OnTreeCursor();
	void                 SetExtensionCtrl(hash_t type_hash, VfsValueExtCtrl* ctrl);
	void                 ClearExtensionCtrl();

	void                 TreeBar(Bar& bar);
	void                 AddNode(VfsValue* parent, const String& id, hash_t type_hash);
	void                 RemoveNode(VfsValue* node);
	void                 RenameNode(VfsValue* node);
	void                 DuplicateNode(VfsValue* node);

	bool                 LoadTree(const String& path);
	bool                 SaveTree(const String& path);
	void                 NewTree(const String& name = "Workspace");

private:
	void                 SyncSubTree(int parent_id, VfsValue& node);
	bool                 RemoveNodeFromParent(VfsValue& parent, VfsValue* target);
};

END_UPP_NAMESPACE

#endif
