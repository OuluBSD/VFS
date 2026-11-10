#include <Core/Core.h>
#include <Vfs/Shell/VfsShell.h>

using namespace Upp;

class TestShellHost : public VfsShellHostBase {
public:
	String output;
	int last_exit_code = 0;
	
	bool Command(VfsShellConsole& shell, const ValueArray& args) override {
		if (args.GetCount() == 0) return false;
		String cmd = args[0];
		if (cmd == "echo") {
			shell.CmdEcho(args);
			last_exit_code = 0;
			return true;
		}
		if (cmd == "pwd") {
			shell.CmdPwd();
			last_exit_code = 0;
			return true;
		}
		if (cmd == "true") {
			last_exit_code = 0;
			return true;
		}
		if (cmd == "false") {
			last_exit_code = 1;
			return true;
		}
		if (cmd == "touch") {
			shell.CmdTouch(args);
			return true;
		}
		if (cmd == "cat") {
			shell.CmdCat(args);
			return true;
		}
		if (cmd == "rm") {
			shell.CmdRm(args);
			return true;
		}
		if (cmd == "help") {
			shell.CmdHelp(args);
			return true;
		}
		return false;
	}
	
	String GetOutput() const override { return output; }
	void ClearOutput() override { output.Clear(); }
	void AddOutput(const String& s) override { output << s; }
	void AddOutputLine(const String& s = String()) override { output << s << "\n"; }
	int GetLastExitCode() const override { return last_exit_code; }
	void SetExitCode(int code) override { last_exit_code = code; }
};

void TestSyntaxParser() {
	Cout() << "TestSyntaxParser: Starting...\n";

	// Simple command
	CmdNode* node1 = ShellSyntaxParser::ParseString("echo hello world");
	ASSERT(node1 != nullptr);
	ASSERT(node1->GetType() == CmdNodeType::COMMAND);
	CommandNode* cmd1 = static_cast<CommandNode*>(node1);
	ASSERT(cmd1->args.GetCount() == 3);
	ASSERT(cmd1->args[0] == "echo");
	ASSERT(cmd1->args[1] == "hello");
	ASSERT(cmd1->args[2] == "world");
	delete node1;

	// Sequence
	CmdNode* node2 = ShellSyntaxParser::ParseString("echo one ; echo two");
	ASSERT(node2 != nullptr);
	ASSERT(node2->GetType() == CmdNodeType::SEQUENCE);
	SequenceNode* seq = static_cast<SequenceNode*>(node2);
	ASSERT(seq->commands.GetCount() == 2);
	delete node2;

	// Pipeline
	CmdNode* node3 = ShellSyntaxParser::ParseString("echo test | grep t");
	ASSERT(node3 != nullptr);
	ASSERT(node3->GetType() == CmdNodeType::PIPELINE);
	PipelineNode* pipe = static_cast<PipelineNode*>(node3);
	ASSERT(pipe->commands.GetCount() == 2);
	delete node3;

	// Logical AND
	CmdNode* node4 = ShellSyntaxParser::ParseString("true && echo ok");
	ASSERT(node4 != nullptr);
	ASSERT(node4->GetType() == CmdNodeType::AND_IF);
	AndIfNode* and_node = static_cast<AndIfNode*>(node4);
	ASSERT(and_node->left != nullptr && and_node->right != nullptr);
	delete node4;

	// Logical OR
	CmdNode* node5 = ShellSyntaxParser::ParseString("false || echo fallback");
	ASSERT(node5 != nullptr);
	ASSERT(node5->GetType() == CmdNodeType::OR_IF);
	OrIfNode* or_node = static_cast<OrIfNode*>(node5);
	ASSERT(or_node->left != nullptr && or_node->right != nullptr);
	delete node5;

	Cout() << "TestSyntaxParser: Passed!\n";
}

void TestPathHelpers() {
	Cout() << "TestPathHelpers: Starting...\n";

	ASSERT(VfsShellConsole::IsVfsPath("/vfs/test"));
	ASSERT(VfsShellConsole::IsVfsPath("/vfs/dir/file.txt"));
	ASSERT(!VfsShellConsole::IsVfsPath("/home/user/file.txt"));
	ASSERT(!VfsShellConsole::IsVfsPath("relative/path"));

	String internal1 = VfsShellConsole::ConvertToInternalPath("/vfs/file", "/vfs");
	ASSERT(internal1 == "/vfs/file");

	String internal2 = VfsShellConsole::ConvertToInternalPath("sub", "/vfs/root");
	ASSERT(internal2 == "/vfs/root/sub");

	Cout() << "TestPathHelpers: Passed!\n";
}

void TestConsoleExecution() {
	Cout() << "TestConsoleExecution: Starting...\n";

	TestShellHost host;
	VfsShellConsole console(host);

	// Test echo
	console.output = "echo Alpha Beta";
	console.Execute();
	ASSERT(host.GetOutput().Find("Alpha Beta") >= 0);
	host.ClearOutput();

	// Test logical AND success
	console.output = "true && echo BothExecuted";
	console.Execute();
	ASSERT(host.GetOutput().Find("BothExecuted") >= 0);
	host.ClearOutput();

	// Test logical AND short-circuit
	console.output = "false && echo ShouldNotRun";
	console.Execute();
	ASSERT(host.GetOutput().Find("ShouldNotRun") < 0);
	host.ClearOutput();

	// Test logical OR success
	console.output = "false || echo FallbackExecuted";
	console.Execute();
	ASSERT(host.GetOutput().Find("FallbackExecuted") >= 0);
	host.ClearOutput();

	// Test logical OR short-circuit
	console.output = "true || echo ShouldNotRun";
	console.Execute();
	ASSERT(host.GetOutput().Find("ShouldNotRun") < 0);
	host.ClearOutput();

	// Test sequence
	console.output = "echo First ; echo Second";
	console.Execute();
	ASSERT(host.GetOutput().Find("First") >= 0);
	ASSERT(host.GetOutput().Find("Second") >= 0);
	host.ClearOutput();

	Cout() << "TestConsoleExecution: Passed!\n";
}

CONSOLE_APP_MAIN {
	StdLogSetup(LOG_COUT | LOG_FILE);
	Cout() << "======================================\n";
	Cout() << "Running VFS Shell Unit Tests\n";
	Cout() << "======================================\n";

	TestSyntaxParser();
	TestPathHelpers();
	TestConsoleExecution();

	Cout() << "======================================\n";
	Cout() << "ALL VFS SHELL TESTS PASSED SUCCESSFULLY!\n";
	Cout() << "======================================\n";
}
