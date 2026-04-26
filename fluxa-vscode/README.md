# Fluxa Language Extension

VS Code extension for the `Fluxa` programming language.

## Features

- `.flx` file association
- syntax highlighting
- bracket and indentation rules
- comment support with `#`
- starter snippets for common Fluxa patterns
- validation diagnostics powered by `fluxa.exe build`
- file icon theme for `.flx`

## Installation

### Local development

1. Open the `fluxa-vscode` folder in VS Code.
2. Press `F5`.
3. In the Extension Development Host window, open any `.flx` file.

### Package as `.vsix`

```powershell
npm install
npx vsce package
```

Then install the generated `.vsix`:

```powershell
code --install-extension fluxa-language-0.1.0.vsix
```

## Included snippets

- `main`
- `func`
- `asyncfunc`
- `if`
- `for`
- `struct`

## Notes

This extension focuses on language support. Running Fluxa programs is still handled by your local `fluxa.exe` CLI.

## Diagnostics

The extension validates Fluxa files on open and save. It calls `fluxa.exe check <file>` and parses diagnostics in the format:

```text
file.flx:12: error FLX1000: message
hint: extra help
```

If `fluxa.exe` is not in the workspace root, set:

```json
"fluxa.executablePath": "C:\\path\\to\\fluxa.exe"
```

## File Icons

After installing the extension, switch the file icon theme to `Fluxa File Icons` if VS Code does not pick it automatically.
