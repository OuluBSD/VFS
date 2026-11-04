#include "Ctrl.h"

NAMESPACE_UPP

VfsTreeApp::VfsTreeApp() {
	Add(splitter.SizePos());
	splitter.Horz() << tree << right_pane;
	splitter.SetPos(2500);

	placeholder.SetText("Select an item in the tree to view or edit.");
	placeholder.SetAlign(ALIGN_CENTER);
	right_pane.Add(placeholder.SizePos());

	tree.WhenCursor = THISBACK(OnTreeCursor);
	tree.WhenBar = THISBACK(TreeBar);

	SetRoot("Workspace", 0);
}

VfsTreeApp::~VfsTreeApp() {
	ClearExtensionCtrl();
}

void VfsTreeApp::SetRoot(const String& name, hash_t type_hash) {
	root.Destroy();
	root.id = name;
	root.type_hash = type_hash;
	if (type_hash) {
		root.ext = VfsValueExtFactory::Create(type_hash, root);
	}
	SyncTree();
}

void VfsTreeApp::SyncTree() {
	int prev_cursor = tree.GetCursor();
	String prev_id;
	if (prev_cursor >= 0 && prev_cursor < tree_nodes.GetCount() && tree_nodes[prev_cursor])
		prev_id = tree_nodes[prev_cursor]->id;

	tree.Clear();
	tree_nodes.Clear();

	int root_id = tree.Add(0, Null, root.id.IsEmpty() ? "Workspace" : root.id);
	tree_nodes.Add(&root);

	for (int i = 0; i < root.sub.GetCount(); i++)
		SyncSubTree(root_id, root.sub[i]);

	tree.OpenDeep(root_id);

	if (!prev_id.IsEmpty()) {
		for (int i = 0; i < tree_nodes.GetCount(); i++) {
			if (tree_nodes[i] && tree_nodes[i]->id == prev_id) {
				tree.SetCursor(i);
				return;
			}
		}
	}
	if (tree.GetChildCount(root_id) > 0)
		tree.SetCursor(tree.GetChild(root_id, 0));
	else
		tree.SetCursor(root_id);
}

void VfsTreeApp::SyncSubTree(int parent_id, VfsValue& node) {
	String label = node.id;
	if (node.type_hash) {
		int fi = VfsValueExtFactory::FindTypeHashFactory(node.type_hash);
		if (fi >= 0) {
			const auto& f = VfsValueExtFactory::List()[fi];
			label << " [" << (f.name.IsEmpty() ? "Component" : f.name) << "]";
		}
	}

	int item_id = tree.Add(parent_id, Null, label);
	tree_nodes.Add(&node);

	for (int i = 0; i < node.sub.GetCount(); i++)
		SyncSubTree(item_id, node.sub[i]);
}

VfsValue* VfsTreeApp::GetSelectedNode() {
	int cur = tree.GetCursor();
	if (cur >= 0 && cur < tree_nodes.GetCount())
		return tree_nodes[cur];
	return nullptr;
}

void VfsTreeApp::ClearExtensionCtrl() {
	if (ext_ctrl) {
		right_pane.RemoveChild(&*ext_ctrl);
		ext_ctrl.Clear();
		ext_ctrl_type_hash = 0;
	}
	placeholder.Show();
}

void VfsTreeApp::SetExtensionCtrl(hash_t type_hash, VfsValueExtCtrl* ctrl) {
	ClearExtensionCtrl();
	if (ctrl) {
		ext_ctrl_type_hash = type_hash;
		ext_ctrl.Attach(ctrl);
		placeholder.Hide();
		right_pane.Add(ctrl->SizePos());
	}
}

void VfsTreeApp::OnTreeCursor() {
	VfsValue* node = GetSelectedNode();
	if (!node) {
		ClearExtensionCtrl();
		return;
	}

	if (node->type_hash != 0) {
		int fi = VfsValueExtFactory::FindTypeHashFactory(node->type_hash);
		if (fi >= 0) {
			const auto& fact = VfsValueExtFactory::List()[fi];
			if (fact.new_ctrl_fn) {
				if (!ext_ctrl || ext_ctrl_type_hash != node->type_hash) {
					VfsValueExtCtrl* ctrl = fact.new_ctrl_fn();
					SetExtensionCtrl(node->type_hash, ctrl);
				}
				if (ext_ctrl) {
					if (!node->ext && fact.new_fn)
						node->ext = fact.new_fn(*node);
					ext_ctrl->ext = ~node->ext;
					ext_ctrl->Data();
				}
				WhenNodeSelected(*node);
				return;
			}
		}
	}

	ClearExtensionCtrl();
	WhenNodeSelected(*node);
}

void VfsTreeApp::AddNode(VfsValue* parent, const String& id, hash_t type_hash) {
	if (!parent)
		parent = &root;

	VfsValue& child = parent->sub.Add();
	child.id = id;
	child.owner = parent;
	child.type_hash = type_hash;

	if (type_hash) {
		int fi = VfsValueExtFactory::FindTypeHashFactory(type_hash);
		if (fi >= 0 && VfsValueExtFactory::List()[fi].new_fn)
			child.ext = VfsValueExtFactory::List()[fi].new_fn(child);
	}

	SyncTree();
	WhenTreeChanged();
}

bool VfsTreeApp::RemoveNodeFromParent(VfsValue& parent, VfsValue* target) {
	for (int i = 0; i < parent.sub.GetCount(); i++) {
		if (&parent.sub[i] == target) {
			parent.sub.Remove(i);
			return true;
		}
		if (RemoveNodeFromParent(parent.sub[i], target))
			return true;
	}
	return false;
}

void VfsTreeApp::RemoveNode(VfsValue* node) {
	if (!node || node == &root)
		return;

	if (RemoveNodeFromParent(root, node)) {
		ClearExtensionCtrl();
		SyncTree();
		WhenTreeChanged();
	}
}

void VfsTreeApp::RenameNode(VfsValue* node) {
	if (!node)
		return;

	String new_name = node->id;
	if (EditText(new_name, "Rename Item", "New Name:")) {
		if (!new_name.IsEmpty()) {
			node->id = new_name;
			SyncTree();
			WhenNodeModified(*node);
			WhenTreeChanged();
		}
	}
}

void VfsTreeApp::DuplicateNode(VfsValue* node) {
	if (!node || node == &root)
		return;

	VfsValue* parent = node->owner ? &*node->owner : &root;
	VfsValue& copy = parent->sub.Add();
	copy.CopyFrom(*node);
	copy.id = node->id + "_copy";
	copy.owner = parent;

	SyncTree();
	WhenTreeChanged();
}

void VfsTreeApp::TreeBar(Bar& bar) {
	VfsValue* node = GetSelectedNode();
	if (!node)
		node = &root;

	bar.Add("Add Folder", [this, node] {
		String name = "New Folder";
		if (EditText(name, "New Folder", "Folder Name:"))
			AddNode(node, name, 0);
	});

	bar.Sub("Add Component", [this, node](Bar& sub_bar) {
		const auto& list = VfsValueExtFactory::List();
		for (int i = 0; i < list.GetCount(); i++) {
			const auto& f = list[i];
			if (f.type == VFSEXT_COMPONENT || f.new_ctrl_fn != nullptr) {
				String item_title = f.name;
				hash_t th = f.type_hash;
				sub_bar.Add(item_title, [this, node, item_title, th] {
					String name = item_title;
					if (EditText(name, "New " + item_title, "Item Name:"))
						AddNode(node, name, th);
				});
			}
		}
	});

	bar.Separator();

	if (node != &root) {
		bar.Add("Rename", THISBACK1(RenameNode, node));
		bar.Add("Duplicate", THISBACK1(DuplicateNode, node));
		bar.Add("Remove", THISBACK1(RemoveNode, node));
	}
}

bool VfsTreeApp::LoadTree(const String& path) {
	if (!FileExists(path))
		return false;

	String json = LoadFile(path);
	if (json.IsEmpty())
		return false;

	VfsValue loaded;
	if (!LoadFromJson(loaded, json))
		return false;

	root.Destroy();
	root.Assign(nullptr, loaded);
	current_file_path = path;
	SyncTree();
	WhenTreeChanged();
	return true;
}

bool VfsTreeApp::SaveTree(const String& path) {
	RealizePath(path);
	String json = StoreAsJson(root, true);
	if (!SaveFile(path, json))
		return false;

	current_file_path = path;
	return true;
}

void VfsTreeApp::NewTree(const String& name) {
	SetRoot(name, 0);
	current_file_path.Clear();
	WhenTreeChanged();
}

END_UPP_NAMESPACE
