#include "Ctrl.h"

NAMESPACE_UPP

VfsTreeAppWindow::VfsTreeAppWindow() {
	Title("VFS Workbench Template");
	Sizeable().Zoomable();
	SetRect(0, 0, 960, 640);

	Add(menu.TopPos(0, Ctrl::STDSIZE));
	Add(status.BottomPos(0, Ctrl::STDSIZE));
	Add(app.VSizePos(Ctrl::STDSIZE, Ctrl::STDSIZE));

	menu.Set(THISBACK(MainMenu));

	app.WhenTreeChanged = THISBACK(UpdateTitle);
	app.WhenNodeSelected = [this](VfsValue& n) {
		status.Set(n.id + (n.type_hash ? " (" + Format("0x%X", (int64)n.type_hash) + ")" : ""));
	};

	UpdateTitle();
}

VfsTreeAppWindow::~VfsTreeAppWindow() {
}

void VfsTreeAppWindow::UpdateTitle() {
	String title = "VFS Workbench Template";
	if (!app.current_file_path.IsEmpty())
		title << " - " << GetFileName(app.current_file_path);
	else
		title << " - [Untitled]";
	Title(title);
}

void VfsTreeAppWindow::MainMenu(Bar& bar) {
	bar.Sub("File", THISBACK(FileMenu));
	bar.Sub("Edit", THISBACK(EditMenu));
	bar.Sub("View", THISBACK(ViewMenu));
	bar.Sub("Help", THISBACK(HelpMenu));
}

void VfsTreeAppWindow::FileMenu(Bar& bar) {
	bar.Add("New", CtrlImg::new_doc(), THISBACK(NewFile)).Key(K_CTRL_N);
	bar.Add("Open...", CtrlImg::open(), THISBACK(OpenFile)).Key(K_CTRL_O);
	bar.Add("Save", CtrlImg::save(), THISBACK(SaveFile)).Key(K_CTRL_S);
	bar.Add("Save As...", CtrlImg::save_as(), THISBACK(SaveFileAs));
	bar.Separator();
	bar.Add("Exit", THISBACK(Close)).Key(K_ALT_F4);
}

void VfsTreeAppWindow::EditMenu(Bar& bar) {
	bar.Add("Add Folder", [this] {
		VfsValue* node = app.GetSelectedNode();
		String name = "New Folder";
		if (EditText(name, "New Folder", "Folder Name:"))
			app.AddNode(node, name, 0);
	});
	bar.Separator();
	bar.Add("Refresh Tree", [this] { app.SyncTree(); }).Key(K_F5);
}

void VfsTreeAppWindow::ViewMenu(Bar& bar) {
	bar.Add("Refresh", [this] { app.SyncTree(); }).Key(K_F5);
}

void VfsTreeAppWindow::HelpMenu(Bar& bar) {
	bar.Add("About", THISBACK(About));
}

void VfsTreeAppWindow::NewFile() {
	app.NewTree("Workspace");
	UpdateTitle();
	status.Set("Created new workspace.");
}

void VfsTreeAppWindow::OpenFile() {
	FileSel fs;
	fs.Type("VFS Workspace (*.json)", "*.json");
	fs.AllFilesType();
	if (fs.ExecuteOpen("Open Workspace")) {
		if (app.LoadTree(fs.Get())) {
			UpdateTitle();
			status.Set("Loaded: " + fs.Get());
		}
		else {
			Exclamation("Failed to load workspace from: " + fs.Get());
		}
	}
}

void VfsTreeAppWindow::SaveFile() {
	if (app.current_file_path.IsEmpty()) {
		SaveFileAs();
		return;
	}
	if (app.SaveTree(app.current_file_path)) {
		status.Set("Saved: " + app.current_file_path);
	}
	else {
		Exclamation("Failed to save workspace.");
	}
}

void VfsTreeAppWindow::SaveFileAs() {
	FileSel fs;
	fs.Type("VFS Workspace (*.json)", "*.json");
	fs.AllFilesType();
	if (fs.ExecuteSaveAs("Save Workspace As")) {
		String path = fs.Get();
		if (GetFileExt(path).IsEmpty())
			path << ".json";
		if (app.SaveTree(path)) {
			UpdateTitle();
			status.Set("Saved: " + path);
		}
		else {
			Exclamation("Failed to save workspace.");
		}
	}
}

void VfsTreeAppWindow::About() {
	PromptOK("VFS Workbench Template\nPart of OuluBSD/VFS\nVirtual File System and dynamic component workbench.");
}

END_UPP_NAMESPACE
