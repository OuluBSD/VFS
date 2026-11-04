#ifndef _Core_VfsBase_Automation_h_
#define _Core_VfsBase_Automation_h_

#include <Core/Core.h>

NAMESPACE_UPP

struct AutomationElement : Moveable<AutomationElement> {
	String  path;
	String  text;
	String  semantic_type;
	String  semantic_name;
	String  semantic_path;
	::Upp::Value   value;
	bool    checked = false;
	bool    enabled = true;
	bool    visible = true;
	bool    is_menu = false;
	// The keyboard accelerator bound to the item, 0 when it has none. Kept as the
	// key code and not as text, because only GetKeyDesc() knows the platform's
	// spelling of it.
	dword   accel = 0;
	Rect    rect = Null;
	String  visible_text;
	double  visible_text_ratio = 1.0;
	
	String ToString() const { return String().Cat() << path << " = " << value << (visible ? "" : " (hidden)"); }
};

class AutomationVisitor : virtual public Visitor {
protected:
	AutomationElement& AddElement(const char *text, const char *semantic_type = "Element");

public:
	typedef AutomationVisitor CLASSNAME;

	virtual Visitor& AccessAction(const char *text, Event<> cb) override;
	virtual Visitor& AccessOption(bool check, const char *text, Event<> cb) override;
	virtual Visitor& AccessMenu(const char *text, Event<Visitor&> proc) override;
	virtual Visitor& AccessLabel(const char *text) override;
	virtual Visitor& AccessValue(const ::Upp::Value& v) override;
	virtual void SetAccessRect(int left, int top, int right, int bottom) override;

	Array<AutomationElement> elements;
	String                   current_path;
	String                   current_semantic_path;
	
	// Config
	bool                     include_hidden = false;
	
	// Write mode
	bool                     write_mode = false;
	String                   target_path;
	::Upp::Value              target_value;
	bool                     target_action = true;
	bool                     found = false;

	String BuildSemanticNode(const String& type, const String& name) const;
	void   SetElementSemantic(AutomationElement& el, const String& type, const String& name);

	AutomationVisitor();
	virtual ~AutomationVisitor() {}
};

END_UPP_NAMESPACE

#endif
