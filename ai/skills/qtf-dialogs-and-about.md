# QTF in Dialogs: About Boxes, Prompts, and the `[` Problem

## Purpose

Rich text in U++ dialogs -- About boxes, prompts, error messages -- and the two
places it goes wrong: markup characters taken from user data, and prompts that
cannot collect input.

## QTF only

The internal rich text format is **QTF**. Never HTML or XML for rich text UI
(see `ai/rules/rich-text-qtf.md`). Display it with `SetQTF()` on the control, or
with a `Prompt*` function, which takes QTF directly.

## The `[` problem: text from the filesystem is not QTF

QTF uses `[...]` for markup. A message built from a **path** can therefore contain
markup, and the message renders wrong or throws:

```
mount /home/x/[backup] is read-only      ->  QTF sees an unterminated [tag]
```

A prompt is not a place to escape by hand, because whether the string is QTF is a
property of the *call site*, not of the text. Two rules, and they are not
interchangeable:

- **A string that is deliberately QTF** (a real About box, a real error template
  with bold or a link) goes to the prompt as it is.
- **A string assembled from data** -- a path, a file name, an entry's type, an
  error from the VFS -- is plain text and must be `DeQtf()`d first:

```cpp
// The path is the reason this is DeQtf: a mount can be named "[backup]".
PromptYesNo(DeQtf(t_("Cannot write to mount ") + mount + t_(".")));
```

Everything else in this codebase is prompted as
`PromptOK(DeQtf(...))` for the same reason. A `DeQtf` around a string with no
markup is a no-op, so applying it everywhere is safe; forgetting it where there is
markup is not.

## About boxes

There is **no edit-ok-cancel dialog** in this fork's CtrlLib, and `Prompt()`
renders rich text and returns a *button index*, so it cannot collect input. An
About box is therefore a small dialog class of its own:

- `SetQTF()` on the text control for the body: title, version, build, licence.
- Labels or an `EditField` for the parts the user may want to copy -- a version
  string and a path are two things people paste.
- Buttons are `Pusher`s. `Button` has **no label setter at all**; `SetLabel()` is
  on `Pusher` (and `ButtonOption`).
- `TopWindow::Close()` takes **no** argument here. Record the answer in a member
  and close plainly.

Worked example: `Help > About` in `uppsrc/Explorer/Explorer.cpp`. It is a
`PromptOK`, not a dialog class, and it needs `DeQtf` for a second reason that is
easy to miss: **QTF has meaning for characters that are ordinary in prose.** A
`"--"` is a table row separator inside a table, and the parser's complaint about it
outside one is "Not in table". A file manager's own About text is full of `--`
(from paths and command lines) and of `[` (from bracketed paths), and both of them
turn into a parse error. Its strings live in `Explorer.t`, so an About box in a
translated window is a translated About box.

## Dialog sizing without a human

`SetRect` before showing, and let the dialog's own `GetMinSize` decide the rest.
Two fork-specific traps when a dialog does not appear:

- **`ctrl <<= rect` is `SetData(Value)`, not `SetRect`.** `Ctrl` has no
  `operator<<=(const Rect&)`, so the `Value` overload wins and hands a rectangle
  to `EditField::SetData`, which asks for it as a `WString` and throws
  `ValueTypeError`. A compile-clean crash at start-up.
- **`String::Trim(int pos)` truncates the string at `pos`.** It is not whitespace
  trimming and there is no no-arg overload, so `Trim(0)` silently produces the
  empty string. Use the free functions `TrimLeft(s)` and `TrimRight(s)`. This is
  what made the new-folder and rename dialogs impossible to accept, and what made
  Enter in a path field always reset.

## Reporting

LOG, not dialogs, for anything the user did not ask to see (`ai/rules/
platform-tooling-notes.md`). A dialog is for a decision the user has to make; a log
line is for the fact they did not.
