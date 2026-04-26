const vscode = require("vscode");
const cp = require("child_process");
const fs = require("fs");
const path = require("path");

function activate(context) {
  const diagnostics = vscode.languages.createDiagnosticCollection("fluxa");
  context.subscriptions.push(diagnostics);

  const validateDocument = (document) => {
    if (document.languageId !== "fluxa" || document.uri.scheme !== "file") {
      return;
    }

    const config = vscode.workspace.getConfiguration("fluxa");
    const executablePath = resolveFluxaExecutable(config);
    if (!executablePath) {
      diagnostics.set(document.uri, [
        new vscode.Diagnostic(
          new vscode.Range(0, 0, 0, 1),
          "Fluxa executable was not found. Set `fluxa.executablePath` or keep `fluxa.exe` in the workspace root.",
          vscode.DiagnosticSeverity.Warning
        )
      ]);
      return;
    }

    cp.execFile(
      executablePath,
      ["check", document.fileName],
      { cwd: workspaceRoot(document) || path.dirname(document.fileName) },
      (error, stdout, stderr) => {
        const output = `${stdout || ""}\n${stderr || ""}`;
        const parsed = parseDiagnostics(output, document);
        diagnostics.set(document.uri, parsed);

        if (error && parsed.length === 0) {
          diagnostics.set(document.uri, [
            new vscode.Diagnostic(
              new vscode.Range(0, 0, 0, 1),
              `Fluxa validation failed: ${error.message}`,
              vscode.DiagnosticSeverity.Error
            )
          ]);
        }
      }
    );
  };

  context.subscriptions.push(
    vscode.commands.registerCommand("fluxa.checkFile", () => {
      const editor = vscode.window.activeTextEditor;
      if (editor) {
        validateDocument(editor.document);
      }
    })
  );

  context.subscriptions.push(
    vscode.workspace.onDidSaveTextDocument((document) => {
      if (vscode.workspace.getConfiguration("fluxa").get("validateOnSave", true)) {
        validateDocument(document);
      }
    })
  );

  context.subscriptions.push(
    vscode.workspace.onDidOpenTextDocument((document) => {
      if (vscode.workspace.getConfiguration("fluxa").get("validateOnOpen", true)) {
        validateDocument(document);
      }
    })
  );

  context.subscriptions.push(
    vscode.workspace.onDidCloseTextDocument((document) => {
      diagnostics.delete(document.uri);
    })
  );

  if (vscode.window.activeTextEditor) {
    validateDocument(vscode.window.activeTextEditor.document);
  }
}

function deactivate() {}

function workspaceRoot(document) {
  const folder = vscode.workspace.getWorkspaceFolder(document.uri);
  return folder ? folder.uri.fsPath : "";
}

function resolveFluxaExecutable(config) {
  const configured = config.get("executablePath", "").trim();
  if (configured && fs.existsSync(configured)) {
    return configured;
  }

  const folders = vscode.workspace.workspaceFolders || [];
  for (const folder of folders) {
    const candidate = path.join(folder.uri.fsPath, "fluxa.exe");
    if (fs.existsSync(candidate)) {
      return candidate;
    }
  }

  return "";
}

function parseDiagnostics(output, document) {
  const diagnostics = [];
  const lines = output.split(/\r?\n/);
  const regex = /^(.*):(\d+): error (FLX\d+): (.*)$/;

  for (let i = 0; i < lines.length; i++) {
    const match = lines[i].match(regex);
    if (!match) {
      continue;
    }

    const [, fileName, lineText, code, message] = match;
    if (path.resolve(fileName) !== path.resolve(document.fileName)) {
      continue;
    }

    const zeroBasedLine = Math.max(0, parseInt(lineText, 10) - 1);
    const sourceLine = document.lineAt(Math.min(zeroBasedLine, document.lineCount - 1));
    const range = new vscode.Range(zeroBasedLine, 0, zeroBasedLine, Math.max(1, sourceLine.text.length));
    const diagnostic = new vscode.Diagnostic(range, `${message} (${code})`, vscode.DiagnosticSeverity.Error);

    if (i + 1 < lines.length && lines[i + 1].startsWith("hint: ")) {
      diagnostic.relatedInformation = [
        new vscode.DiagnosticRelatedInformation(
          new vscode.Location(document.uri, range),
          lines[i + 1]
        )
      ];
    }

    diagnostics.push(diagnostic);
  }

  return diagnostics;
}

module.exports = {
  activate,
  deactivate
};
