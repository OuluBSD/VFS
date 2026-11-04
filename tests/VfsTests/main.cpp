#include "VfsTests.h"

using namespace Upp;

void TestBasicTree() {
	Cout() << "TestBasicTree: Starting...\n";

	VfsValue root;
	root.id = "workspace";

	VfsValue& src = root.sub.Add();
	src.id = "src";
	src.owner = &root;

	VfsValue& main_cpp = src.sub.Add();
	main_cpp.id = "main.cpp";
	main_cpp.owner = &src;

	ASSERT(root.sub.GetCount() == 1);
	ASSERT(root.sub[0].id == "src");
	ASSERT(src.sub.GetCount() == 1);
	ASSERT(src.sub[0].id == "main.cpp");
	ASSERT(main_cpp.owner == &src);
	ASSERT(src.owner == &root);

	Cout() << "TestBasicTree: Passed!\n";
}

void TestFactoryAndExt() {
	Cout() << "TestFactoryAndExt: Starting...\n";

	VfsValueExtFactory::Register<SampleDocExt>("SampleDocExt", VFSEXT_COMPONENT, "sample_doc", "Documents");

	hash_t doc_hash = TypedStringHasher<SampleDocExt>("SampleDocExt");
	ASSERT(doc_hash != 0);

	int fact_idx = VfsValueExtFactory::FindTypeHashFactory(doc_hash);
	ASSERT(fact_idx >= 0);

	VfsValue root;
	root.id = "root";

	VfsValue& node = root.sub.Add();
	node.id = "readme.md";
	node.owner = &root;
	node.type_hash = doc_hash;

	VfsValueExt* ext = VfsValueExtFactory::Create(doc_hash, node);
	ASSERT(ext != nullptr);
	node.ext = ext;

	SampleDocExt* doc = dynamic_cast<SampleDocExt*>(ext);
	ASSERT(doc != nullptr);
	doc->title = "Test Readme";
	doc->content = "This is sample markdown content.";

	ASSERT(doc->title == "Test Readme");
	ASSERT(doc->content == "This is sample markdown content.");
	ASSERT(doc->GetTypeHash() == doc_hash);

	Cout() << "TestFactoryAndExt: Passed!\n";
}

void TestJsonSerialization() {
	Cout() << "TestJsonSerialization: Starting...\n";

	VfsValue root;
	root.id = "proj";

	VfsValue& item1 = root.sub.Add();
	item1.id = "config";
	item1.owner = &root;

	VfsValue& item2 = root.sub.Add();
	item2.id = "data";
	item2.owner = &root;

	String json = StoreAsJson(root);
	ASSERT(!json.IsEmpty());

	VfsValue loaded;
	bool ok = LoadFromJson(loaded, json);
	ASSERT(ok);
	ASSERT(loaded.id == "proj");
	ASSERT(loaded.sub.GetCount() == 2);
	ASSERT(loaded.sub[0].id == "config");
	ASSERT(loaded.sub[1].id == "data");
	ASSERT(loaded.sub[0].owner == &loaded);
	ASSERT(loaded.sub[1].owner == &loaded);

	Cout() << "TestJsonSerialization: Passed!\n";
}

void TestAstValues() {
	Cout() << "TestAstValues: Starting...\n";

	VfsValue node;
	node.id = "code_parent";

	VfsValue& child = node.AstAdd(10, "int x");
	AstValue& ast = (AstValue&)child;
	ast.begin = Point(1, 1);
	ast.end = Point(1, 5);

	ASSERT(child.IsAstValue());
	const AstValue* found = (const AstValue*)child;
	ASSERT(found != nullptr);
	ASSERT(found->kind == 10);
	ASSERT(found->begin.x == 1 && found->begin.y == 1);
	ASSERT(found->end.x == 1 && found->end.y == 5);

	Cout() << "TestAstValues: Passed!\n";
}

CONSOLE_APP_MAIN {
	StdLogSetup(LOG_COUT | LOG_FILE);
	Cout() << "======================================\n";
	Cout() << "Running VFS Core Tests\n";
	Cout() << "======================================\n";

	TestBasicTree();
	TestFactoryAndExt();
	TestJsonSerialization();
	TestAstValues();

	Cout() << "======================================\n";
	Cout() << "ALL VFS TESTS PASSED SUCCESSFULLY!\n";
	Cout() << "======================================\n";
}
