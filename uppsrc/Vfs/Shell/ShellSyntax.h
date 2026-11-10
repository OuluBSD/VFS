#ifndef _VfsShell_ShellSyntax_h_
#define _VfsShell_ShellSyntax_h_

#include <Core/Core.h>

NAMESPACE_UPP

// AST Node types for shell commands
enum class CmdNodeType {
    COMMAND,           // A simple command
    SEQUENCE,          // Command sequence (a; b)
    PIPELINE,          // Pipe command (a | b)
    AND_IF,            // Logical AND (a && b)
    OR_IF,             // Logical OR (a || b)
    BACKGROUND,        // Background execution (a &)
    SUBSHELL,          // Subshell (a && b | c, etc.)
    REDIRECTION        // I/O redirection
};

// Base AST node class
class CmdNode {
public:
    virtual ~CmdNode() {}
    virtual CmdNodeType GetType() const = 0;
    virtual String ToString() const = 0;
};

// Command AST node - represents a single executable command
class CommandNode : public CmdNode {
public:
    Vector<String> args;  // Command name and arguments
    ValueMap vars;        // Variables to set for this command
    
    CmdNodeType GetType() const override { return CmdNodeType::COMMAND; }
    
    String ToString() const override {
        String result;
        for (int i = 0; i < args.GetCount(); i++) {
            if (i > 0) result << " ";
            result << args[i];
        }
        return result;
    }
};

// Pipeline AST node - represents a | b | c
class PipelineNode : public CmdNode {
public:
    Vector<CmdNode*> commands;  // Commands connected by pipes
    
    virtual ~PipelineNode() {
        for (CmdNode* cmd : commands) delete cmd;
    }

    CmdNodeType GetType() const override { return CmdNodeType::PIPELINE; }
    
    String ToString() const override {
        String result;
        for (int i = 0; i < commands.GetCount(); i++) {
            if (i > 0) result << " | ";
            result << commands[i]->ToString();
        }
        return result;
    }
};

// Logical AND AST node - represents a && b
class AndIfNode : public CmdNode {
public:
    CmdNode* left;
    CmdNode* right;
    
    AndIfNode(CmdNode* l = nullptr, CmdNode* r = nullptr) : left(l), right(r) {}
    virtual ~AndIfNode() { delete left; delete right; }

    CmdNodeType GetType() const override { return CmdNodeType::AND_IF; }
    
    String ToString() const override {
        return (left ? left->ToString() : "") + " && " + (right ? right->ToString() : "");
    }
};

// Logical OR AST node - represents a || b
class OrIfNode : public CmdNode {
public:
    CmdNode* left;
    CmdNode* right;
    
    OrIfNode(CmdNode* l = nullptr, CmdNode* r = nullptr) : left(l), right(r) {}
    virtual ~OrIfNode() { delete left; delete right; }

    CmdNodeType GetType() const override { return CmdNodeType::OR_IF; }
    
    String ToString() const override {
        return (left ? left->ToString() : "") + " || " + (right ? right->ToString() : "");
    }
};

// Sequence AST node - represents command sequence with ;
class SequenceNode : public CmdNode {
public:
    Vector<CmdNode*> commands;  // Commands separated by ;
    
    virtual ~SequenceNode() {
        for (CmdNode* cmd : commands) delete cmd;
    }

    CmdNodeType GetType() const override { return CmdNodeType::SEQUENCE; }
    
    String ToString() const override {
        String result;
        for (int i = 0; i < commands.GetCount(); i++) {
            if (i > 0) result << "; ";
            result << commands[i]->ToString();
        }
        return result;
    }
};

// Shell command parser class
class ShellSyntaxParser {
private:
    String input;
    int pos;
    
    // Helper functions for parsing
    void SkipWhitespace();
    bool IsAtEnd() const;
    char CurrentChar() const;
    char Peek(int offset = 1) const;
    void Advance();
    String ConsumeToken();
    String ParseWord();
    CmdNode* ParseCommand();
    CmdNode* ParsePipeline();
    CmdNode* ParseLogicalExpr();
    CmdNode* ParseSimpleCommand();
    
public:
    ShellSyntaxParser(const String& input);
    ~ShellSyntaxParser();
    
    // Parse the entire input string into an AST
    CmdNode* Parse();
    
    // Utility function to parse a command string and return its AST representation
    static CmdNode* ParseString(const String& input);
};

END_UPP_NAMESPACE

#endif