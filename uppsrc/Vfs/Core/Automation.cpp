#include "VfsBase.h"

NAMESPACE_UPP

String AutomationVisitor::BuildSemanticNode(const String& type, const String& name) const
{
	String t = TrimBoth(type);
	if(t.IsEmpty())
		t = "Element";
	String n = TrimBoth(name);
	return n.IsEmpty() ? t : t + " : " + n;
}

void AutomationVisitor::SetElementSemantic(AutomationElement& el, const String& type, const String& name)
{
	el.semantic_type = TrimBoth(type);
	if(el.semantic_type.IsEmpty())
		el.semantic_type = "Element";
	el.semantic_name = TrimBoth(name);
	String node = BuildSemanticNode(el.semantic_type, el.semantic_name);
	el.semantic_path = current_semantic_path.IsEmpty() ? node : current_semantic_path + " -> " + node;
}

AutomationElement& AutomationVisitor::AddElement(const char *text, const char *semantic_type)
{
	AutomationElement& el = elements.Add();
	el.text = text;
	el.path = current_path;
	if(!el.text.IsEmpty()) {
		if(!el.path.IsEmpty()) el.path << "/";
		el.path << el.text;
	}
	SetElementSemantic(el, semantic_type, el.text);
	el.visible_text = el.text;
	el.visible_text_ratio = el.text.IsEmpty() ? 0.0 : 1.0;
	return el;
}

Visitor& AutomationVisitor::AccessAction(const char *text, Event<> cb)
{
	if(found && write_mode) return *this;
	String t = text;
	if(t.IsEmpty()) {
		// This is tricky as we don't have access to Ctrl here.
		// But usually Bar::Item or similar caller passes text.
	}
	String action_type = current_semantic_path.Find("MenuBar") >= 0 ? "Bar" : "Action";
	AutomationElement& el = AddElement(t, action_type);
	if(write_mode && (el.path == target_path || el.text == target_path || el.semantic_path == target_path)) {
		found = true;
		if(target_action && cb) cb();
	}
	return *this;
}

Visitor& AutomationVisitor::AccessOption(bool check, const char *text, Event<> cb)
{
	if(found && write_mode) return *this;
	AutomationElement& el = AddElement(text, "Option");
	el.checked = check;
	if(write_mode && (el.path == target_path || el.text == target_path || el.semantic_path == target_path)) {
		Cout() << "Automation Triggering Option: " << el.path << "\n";
		found = true;
		if(target_action && cb) cb();
	}
	return *this;
}

Visitor& AutomationVisitor::AccessMenu(const char *text, Event<Visitor&> proc)
{
	if(found && write_mode) return *this;
	AutomationElement& el = AddElement(text, "Bar");
	el.is_menu = true;
	if(write_mode && (el.path == target_path || el.text == target_path || el.semantic_path == target_path)) {
		found = true;
		return *this;
	}
	
	String old_path = current_path;
	String old_semantic_path = current_semantic_path;
	current_path = el.path;
	current_semantic_path = el.semantic_path;
	proc(*this);
	current_semantic_path = old_semantic_path;
	current_path = old_path;
	
	return *this;
}

Visitor& AutomationVisitor::AccessLabel(const char *text)
{
	if(found && write_mode) return *this;
	AutomationElement& el = AddElement(text, "Label");
	if(write_mode && (el.path == target_path || el.text == target_path || el.semantic_path == target_path)) {
		found = true;
	}
	return *this;
}

Visitor& AutomationVisitor::AccessValue(const ::Upp::Value& v)
{
	if(elements.GetCount())
		elements.Top().value = v;
	return *this;
}

void AutomationVisitor::SetAccessRect(int left, int top, int right, int bottom)
{
	if(elements.GetCount())
		elements.Top().rect = Rect(left, top, right, bottom);
}

AutomationVisitor::AutomationVisitor()
{
}

END_UPP_NAMESPACE
