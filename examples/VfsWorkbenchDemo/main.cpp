#include <Vfs/Ctrl/Ctrl.h>

using namespace Upp;

// 1. Text Document Component & View
struct TextDocExt : VfsValueExt {
	CLASSTYPE(TextDocExt)

	String title;
	String body;

	TextDocExt(VfsValue& n) : VfsValueExt(n) {}

	void Visit(Vis& vis) override {
		vis
			("title", title)
			("body", body);
	}
};

struct TextDocCtrl : VfsValueExtCtrl {
	EditString title_edit;
	DocEdit    body_edit;
	Label      title_lbl;

	TextDocCtrl() {
		title_lbl.SetText("Document Title:");
		Add(title_lbl.TopPos(10, 20).LeftPos(10, 100));
		Add(title_edit.TopPos(10, 20).HSizePos(120, 10));
		Add(body_edit.VSizePos(40, 10).HSizePos(10, 10));

		title_edit.WhenAction = [this] {
			if (ext) {
				GetExt<TextDocExt>().title = title_edit.GetData();
				WhenEditorChange();
			}
		};
		body_edit.WhenAction = [this] {
			if (ext) {
				GetExt<TextDocExt>().body = body_edit.GetData();
				WhenEditorChange();
			}
		};
	}

	void Data() override {
		if (ext) {
			TextDocExt& doc = GetExt<TextDocExt>();
			title_edit.SetData(doc.title);
			body_edit.SetData(doc.body);
		}
	}
};

// 2. Counter Component & View
struct CounterExt : VfsValueExt {
	CLASSTYPE(CounterExt)

	int count = 0;

	CounterExt(VfsValue& n) : VfsValueExt(n) {}

	void Visit(Vis& vis) override {
		vis("count", count);
	}
};

struct CounterCtrl : VfsValueExtCtrl {
	Label  count_lbl;
	Button inc_btn;
	Button dec_btn;

	CounterCtrl() {
		count_lbl.SetAlign(ALIGN_CENTER);
		count_lbl.SetFont(Arial(24).Bold());
		Add(count_lbl.TopPos(30, 40).HSizePos(20, 20));

		inc_btn.SetLabel("Increment (+1)");
		dec_btn.SetLabel("Decrement (-1)");

		Add(inc_btn.TopPos(90, 30).LeftPos(40, 140));
		Add(dec_btn.TopPos(90, 30).LeftPos(200, 140));

		inc_btn.WhenAction = [this] {
			if (ext) {
				GetExt<CounterExt>().count++;
				Data();
				WhenEditorChange();
			}
		};
		dec_btn.WhenAction = [this] {
			if (ext) {
				GetExt<CounterExt>().count--;
				Data();
				WhenEditorChange();
			}
		};
	}

	void Data() override {
		if (ext) {
			count_lbl.SetText(Format("Counter Value: %d", GetExt<CounterExt>().count));
		}
	}
};

void RegisterWorkbenchComponents() {
	static bool registered = false;
	if (registered) return;
	registered = true;

	VfsValueExtFactory::Register<TextDocExt>("TextDocument", VFSEXT_COMPONENT, "text_doc", "Components");
	VfsValueExtFactory::RegisterCtrl<TextDocExt, TextDocCtrl>("TextDocCtrl");

	VfsValueExtFactory::Register<CounterExt>("Counter", VFSEXT_COMPONENT, "counter", "Components");
	VfsValueExtFactory::RegisterCtrl<CounterExt, CounterCtrl>("CounterCtrl");
}

INITIALIZER(WorkbenchComponents) {
	RegisterWorkbenchComponents();
}

GUI_APP_MAIN {
	RegisterWorkbenchComponents();

	VfsTreeAppWindow win;

	// Populate initial sample workspace
	VfsValue& root = win.app.GetRoot();
	root.id = "DemoWorkspace";

	VfsValue* folder = &root.sub.Add();
	folder->id = "Documents";
	folder->owner = &root;

	hash_t doc_th = TypedStringHasher<TextDocExt>("TextDocument");
	win.app.AddNode(folder, "Welcome.md", doc_th);
	if (folder->sub.GetCount() > 0 && folder->sub[0].ext) {
		TextDocExt* doc = dynamic_cast<TextDocExt*>(&*folder->sub[0].ext);
		if (doc) {
			doc->title = "Welcome to VFS Workbench";
			doc->body = "This is a demonstration of the reusable VFS Tree Workbench template.\n"
			            "The left pane displays the VfsValue tree hierarchy.\n"
			            "The right pane dynamically loads the registered VfsValueExtCtrl view.";
		}
	}

	hash_t counter_th = TypedStringHasher<CounterExt>("Counter");
	win.app.AddNode(&root, "MetricsCounter", counter_th);
	if (root.sub.GetCount() > 1 && root.sub[1].ext) {
		CounterExt* c = dynamic_cast<CounterExt*>(&*root.sub[1].ext);
		if (c) c->count = 42;
	}

	win.app.SyncTree();

	const Vector<String>& cmd = CommandLine();
	if (cmd.GetCount() > 0 && cmd[0] == "--test") {
		Cout() << "VfsWorkbenchDemo: Running automated self-test...\n";

		ASSERT(root.sub.GetCount() == 2);
		ASSERT(root.sub[0].id == "Documents");
		ASSERT(root.sub[0].sub.GetCount() == 1);
		ASSERT(root.sub[0].sub[0].id == "Welcome.md");

		// Test serialization round-trip
		String temp_file = ConfigFile("test_workbench_save.json");
		bool save_ok = win.app.SaveTree(temp_file);
		ASSERT(save_ok);

		bool load_ok = win.app.LoadTree(temp_file);
		ASSERT(load_ok);
		ASSERT(win.app.GetRoot().id == "DemoWorkspace");

		DeleteFile(temp_file);
		Cout() << "VfsWorkbenchDemo: Automated self-test passed successfully!\n";
		win.app.ClearExtensionCtrl();
		win.app.GetRoot().sub.Clear();
		MemoryIgnoreLeaksBegin();
		return;
	}

	win.Run();
}
