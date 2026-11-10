#include <Core/Core.h>
#include <Vfs/Core/Core.h>
#include <Vfs/Overlay/Overlay.h>
#include <Vfs/Storage/VfsStorage.h>
#include <Vfs/Analysis/Analysis.h>
#include <Vfs/Meta/Meta.h>
#include <Vfs/Dataset/Dataset.h>
#include <Vfs/Solver/Solver.h>
#include <Vfs/Runtime/Runtime.h>

using namespace Upp;

void TestOverlay() {
	Cout() << "TestOverlay: Starting..." << EOL;

	SourceRef ref(1001, 2002, "components/Atom.cpp", 5);
	ASSERT(ref.pkg_hash == 1001);
	ASSERT(ref.file_hash == 2002);
	ASSERT(ref.local_path == "components/Atom.cpp");
	ASSERT(ref.priority == 5);

	VfsOverlay overlay(ref);
	VfsValue node;
	node.id = "TestAtom";
	node.value = String("AtomPayload");
	overlay.AddFragment("components/Atom", node);

	const VfsValue* retrieved = overlay.GetFragment("components/Atom");
	ASSERT(retrieved != nullptr);
	ASSERT(retrieved->id == "TestAtom");

	Cout() << "TestOverlay: Passed!" << EOL;
}

void TestStorage() {
	Cout() << "TestStorage: Starting..." << EOL;

	String temp_dir = ConfigFile("temp_vfs_test");
	DirectoryCreate(temp_dir);
	String json_path = AppendFileName(temp_dir, "test_fragment.json");
	String bin_path = AppendFileName(temp_dir, "test_fragment.bin");
	String idx_path = AppendFileName(temp_dir, "test_overlay.idx");

	// 1. Fragment JSON save/load
	VfsValue root;
	root.id = "VirtualRoot";
	root.pkg_hash = 0xABCD1234;
	root.file_hash = 0x5678EF01;
	
	VfsValue& child = root.sub.Add();
	child.id = "LeafChild";
	child.value = 42;

	bool saved_json = VfsSaveFragment(json_path, root);
	ASSERT(saved_json);

	VfsValue loaded_json;
	bool loaded_ok = VfsLoadFragment(json_path, loaded_json);
	ASSERT(loaded_ok);
	ASSERT(loaded_json.id == "VirtualRoot");
	ASSERT(loaded_json.sub.GetCount() == 1);
	ASSERT(loaded_json.sub[0].id == "LeafChild");
	ASSERT((int)loaded_json.sub[0].value == 42);

	// 2. Fragment Binary save/load
	bool saved_bin = VfsSaveFragmentBinary(bin_path, root);
	ASSERT(saved_bin);

	VfsValue loaded_bin;
	bool loaded_bin_ok = VfsLoadFragmentBinary(bin_path, loaded_bin);
	ASSERT(loaded_bin_ok);
	ASSERT(loaded_bin.id == "VirtualRoot");
	ASSERT(loaded_bin.sub.GetCount() == 1);
	ASSERT(loaded_bin.sub[0].id == "LeafChild");

	// 3. Overlay Index save/load
	VfsOverlayIndex index;
	OverlayNodeRecord& rec = index.nodes.Add();
	rec.path = "/vfs/root/components/LeafChild";
	SourceRef src_ref(0xABCD1234, 0x5678EF01, "src/Leaf.cpp", 10);
	rec.sources.Add(src_ref);
	rec.metadata.Set("author", "OuluBSD");

	bool saved_idx = VfsSaveOverlayIndex(idx_path, index);
	ASSERT(saved_idx);

	VfsOverlayIndex loaded_idx;
	bool loaded_idx_ok = VfsLoadOverlayIndex(idx_path, loaded_idx);
	ASSERT(loaded_idx_ok);
	ASSERT(loaded_idx.nodes.GetCount() == 1);
	ASSERT(loaded_idx.nodes[0].path == "/vfs/root/components/LeafChild");
	ASSERT(loaded_idx.nodes[0].sources.GetCount() == 1);
	ASSERT(loaded_idx.nodes[0].sources[0].priority == 10);

	// Cleanup
	FileDelete(json_path);
	FileDelete(bin_path);
	FileDelete(idx_path);
	DirectoryDelete(temp_dir);

	Cout() << "TestStorage: Passed!" << EOL;
}

void TestAnalysis() {
	Cout() << "TestAnalysis: Starting..." << EOL;

	CodeVisitorProfile prof;
	prof.Set(CodeVisitorProfile::VISIT_VARIABLE_USAGE, true);
	prof.Set(CodeVisitorProfile::VISIT_VARIABLE_TYPE_REQ_DEF, false);

	ASSERT(prof.Is(CodeVisitorProfile::VISIT_VARIABLE_USAGE) == true);
	ASSERT(prof.Is(CodeVisitorProfile::VISIT_VARIABLE_TYPE_REQ_DEF) == false);

	prof.SetAllTrue();
	ASSERT(prof.Is(CodeVisitorProfile::VISIT_VARIABLE_USAGE) == true);
	ASSERT(prof.Is(CodeVisitorProfile::VISIT_VARIABLE_TYPE_REQ_DEF) == true);

	CodeVisitor visitor;
	visitor.SetProfile(prof);
	visitor.SetLimit(100);
	visitor.Begin();

	VfsValue ast_node;
	ast_node.id = "testVar";
	visitor.Visit("main.cpp", ast_node);

	Cout() << "TestAnalysis: Passed!" << EOL;
}

CONSOLE_APP_MAIN {
	Cout() << "======================================" << EOL;
	Cout() << "Running VFS Ext (Storage/Overlay/Analysis) Tests" << EOL;
	Cout() << "======================================" << EOL;

	TestOverlay();
	TestStorage();
	TestAnalysis();

	Cout() << "======================================" << EOL;
	Cout() << "ALL VFS EXT TESTS PASSED SUCCESSFULLY!" << EOL;
	Cout() << "======================================" << EOL;
}
