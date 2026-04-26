# Fluxa

Fluxa is a modern statically typed scripting language with a clean Python-like syntax, JavaScript-inspired ergonomics, built-in `async/await`, and safe memory management via a managed runtime.

Files use the `.flx` extension and run through:

```powershell
fluxa hello.flx
```

If PowerShell says `fluxa` is not recognized, install the command once from the project root:

```powershell
.\install-fluxa.ps1
```

Then open a new terminal.

On Windows the installer builds a native standalone `fluxa.exe`, so `fluxa` works like a normal command and does not require Python to execute `.flx` programs.

Or with explicit commands:

```powershell
fluxa run hello.flx
fluxa check hello.flx
fluxa build hello.flx
fluxa version
```

## Language overview

- Simple syntax based on indentation and colons.
- Static typing with optional annotations and local inference for `let`.
- Async-first runtime through `async func`, `await`, and an automatic `main` entry point.
- Safe memory management through the Fluxa managed runtime.
- Standalone CLI executable on Windows.

## Syntax

### Variables

Fluxa has two kinds of variables: immutable `let` and mutable `var`.

Use `let` for values that don't change:

```fluxa
func main() -> Void:
    let name: String = "Fluxa"
    let year = 2026  # type inferred
```

Use `var` for values that can be modified:

```fluxa
func main() -> Void:
    var age: Int = 20
    age = 21  # ok
```

Attempting to reassign `let` causes a compile error:

```fluxa
let name: String = "Mira"
name = "Anna"  # error: cannot assign to immutable variable
```

### Functions

```fluxa
func main() -> Void:
    print(add(2, 3))

func add(a: Int, b: Int) -> Int:
    return a + b
```

### Conditions

```fluxa
func main() -> Void:
    let score: Int = 91

    if score >= 90:
        print("excellent")
    else:
        print("keep going")
```

### Loops

```fluxa
func main() -> Void:
    for i in range(0, 3):
        print(i)

    let counter: Int = 3
    while counter > 0:
        print(counter)
        counter = counter - 1
```

### Collections

Fluxa supports generic collections: `List<T>` and `Map<K, V>`.

Lists:

```fluxa
func main() -> Void:
    let nums: List<Int> = [1, 2, 3]
    nums.append(4)
    print(nums.length)  # 4
    nums.remove(2)  # removes element at index 2
```

Maps:

```fluxa
func main() -> Void:
    let user: Map<String, Int> = {
        "age": 20,
        "score": 85
    }
    print(user["age"])  # 20
    print(user.keys())  # ["age", "score"]
    print(user.values())  # [20, 85]
```

### Strings

Strings support interpolation and methods:

```fluxa
func main() -> Void:
    let name: String = "Mira"
    print("Hello {name}")  # Hello Mira
    
    let s: String = "123"
    let num: Int = s.to_int()  # 123
    let back: String = num.to_string()  # "123"
    print(s.length)  # 3
```

### Structures

Structures can have methods and constructors:

```fluxa
struct User:
    name: String
    age: Int = 0

    func greet(self) -> String:
        return "Hi " + self.name

    func birthday(self) -> Void:
        self.age = self.age + 1

func main() -> Void:
    let user = User("Mira", 28)
    print(user.greet())  # Hi Mira
    user.birthday()
    print(user.age)  # 29
```

### Error Handling

Fluxa uses try/catch for error handling:

```fluxa
func risky() -> Int:
    if true:
        throw "Something went wrong"

func main() -> Void:
    try:
        let result = risky()
    catch e:
        print("Error: " + e)
```

Runtime errors are automatically caught. Custom errors can be thrown with `throw "message"`.

### Modules and imports

Imports are top-level:

```fluxa
import math
import io
from helpers import square

func main() -> Void:
    print(square(9))
```

Local modules are imported by relative path.

### Optional Types

Use `?` for nullable types:

```fluxa
func main() -> Void:
    let name: String? = nil
    if name != nil:
        print(name)  # safe
```

### Async/Await

Async supports parallelism:

```fluxa
async func fetch_data() -> String:
    import asyncs
    await asyncs.sleep(1.0)
    return "data"

async func main() -> Void:
    let task1 = fetch_data()
    let task2 = fetch_data()
    let result1 = await task1
    let result2 = await task2
    print(result1 + " " + result2)
```

## Entry Point

Every runnable `.flx` file must have `main` as the first non-import top-level statement.

```fluxa
import math
import asyncs

func main() -> Void:
    let b: Int = 12
    print(b)
```

Imports can be placed at the top. Fluxa calls `main` automatically. Top-level executable code such as `print(...)`, `let ...`, loops, and `run(main())` should be placed inside `main`. Library modules that only declare functions or structs can omit `main`.

## Standard library

## Standard library

Fluxa includes a comprehensive standard library with modules for common tasks:

### Core modules

- `math`: Mathematical functions and constants
  - `sqrt(x)`, `pow(x, y)`, `sin(x)`, `cos(x)`, `tan(x)`
  - `log(x)`, `exp(x)`, `pi`, `e`
- `random`: Random number generation
  - `int(min, max)`, `float()`, `choice(list)`
- `strings`: String manipulation (extended)
  - `upper()`, `lower()`, `split(sep)`, `join(list)`
- `collections`: Data structures
  - `List<T>`, `Map<K, V>`, `Set<T>`
  - Methods: `append()`, `pop()`, `get()`, `add()`, etc.
- `io`: Input/output operations
  - `print()`, `input()`, `read(file)`, `write(file, text)`
  - `append(file, text)`, `exists(file)`, `delete(file)`
- `asyncs`: Asynchronous operations
  - `sleep(seconds)`, `all(tasks)`, `race(tasks)`

### Data and serialization

- `json`: JSON parsing and serialization
  - `parse(text)`, `stringify(data)`
  - `load(file)`, `dump(file, data)`
- `csv`: CSV file handling
  - `read(file)`, `write(file, rows)`

### Scientific computing

- `arrays`: Multi-dimensional arrays (numpy-like)
  - `array(data)`, vector operations `+`, `-`, `*`, `/`
  - `reshape(shape)`, `sum()`, `mean()`

### Types

- `Any`: Universal type for dynamic data
  - Use for JSON parsing, mixed data
  - Type checking disabled for `Any` variables

### Error handling

- Built-in error types with `try/catch`
- Custom error classes

### Functional programming

- Higher-order functions on collections
- `map(func)`, `filter(func)`, `reduce(func, initial)`

### Package management

- `fluxa install package`
- `fluxa remove package`
- Dependencies in `fluxa.json`

## CLI

```powershell
fluxa app.flx              # run app.flx
fluxa run app.flx          # explicit run
fluxa check app.flx        # validate syntax
fluxa build app.flx        # build to JSON
fluxa install package      # install package
fluxa remove package       # remove package
fluxa version              # show version
```

Commands output errors to stderr, success to stdout.

## Examples

The `examples/` directory contains sample programs:

- `hello.flx` - Basic "Hello World"
- `calculator.flx` - Calculator demo with arithmetic operations
- `async_demo.flx` - Async/await usage
- `full_example.flx` - Complete application with async processing
- `ui_demo.flx` - Console UI library demonstration
- `helpers.flx` - Utility functions
- `interface_demo.flx` - Interface usage
- `models.flx` - Data structures
- `modules.flx` - Module system

Run any example with `fluxa examples/filename.flx`.

## Package Manager

Packages are stored in `fluxa_modules/`. Dependencies are listed in `fluxa.json`:

```json
{
  "name": "myapp",
  "dependencies": {
    "web": "1.0.0"
  }
}
```

## Memory and Behavior

- `let` variables are immutable, passed by value
- `var` variables are mutable, passed by reference for collections
- Structures are passed by value, but methods modify the instance

## Full Example

```fluxa
import io

struct Task:
    title: String
    done: Bool = false

    func complete(self) -> Void:
        self.done = true

async func process_tasks(tasks: List<Task>) -> Void:
    for task in tasks:
        print("Processing {task.title}")
        await asyncs.sleep(0.5)
        task.complete()

func main() -> Void:
    var tasks: List<Task> = [
        Task("Learn Fluxa"),
        Task("Build app")
    ]
    
    print("Enter your name:")
    let name: String = io.input("")
    
    try:
        await process_tasks(tasks)
        print("Hello {name}, all tasks done!")
    catch e:
        print("Error: " + e)
```

## UI Library

Fluxa includes a console UI library for building interactive applications:

```fluxa
import fluxaui

func main() -> Void:
    let choice = menu("Main Menu", ["Data Entry", "Settings", "Exit"])
    
    if choice == 0:
        let data = form(["Name", "Age"])
        print("Name: " + data["Name"])
        
        progress_bar(50, 100, 20)
        
        if confirm("Show table?"):
            table(["Field", "Value"], [
                ["Name", data["Name"]],
                ["Age", data["Age"]]
            ])
    
    elif choice == 1:
        alert("Settings not implemented")
    
    else:
        print("Goodbye!")
```

UI functions:
- `menu(title, options)` - Interactive menu selection
- `form(fields)` - Data collection form
- `progress_bar(current, total, width)` - Progress display
- `table(headers, rows)` - Data table display
- `confirm(message)` - Yes/no confirmation
- `alert(message)` - Information display

See `examples/ui_demo.flx` for a complete example.

## Entry Point

Every runnable `.flx` file must start with `main`.

```fluxa
func main() -> Void:
    let b: Int = 12
    print(b)
```

Fluxa calls `main` automatically. Top-level executable code such as `print(...)`, `let ...`, loops, and `run(main())` should be placed inside `main`. Library modules that only declare functions or structs can omit `main`.

## Standard library

- `math`: `sqrt`, `pow`, `abs`, `max`, `min`, constants `pi`, `e`
- `strings`: `upper`, `lower`, `trim`, `split`, `join`, `length`, `to_int`, `to_string`
- `io`: `print`, `input`
- `asyncs`: `sleep`

## Implementation

`fluxa.exe` is a standalone interpreter built from `fluxa_launcher.c`.

1. `fluxa file.flx` is treated as `fluxa run file.flx`.
2. The interpreter tokenizes and parses Fluxa source into its own AST.
3. The runtime executes that AST directly, without delegating to Python.
4. A lightweight runtime type checker validates declarations and returns beginner-friendly diagnostics.
5. Standard modules like `math`, `strings`, `io`, and `asyncs` are provided by the native runtime.
6. `fluxa check file.flx` validates syntax and entry-point rules without writing files.
7. `fluxa build file.flx` emits a small JSON build artifact in `build/`.

## VS Code

The project includes:

- `.vscode/settings.json` for Code Runner support
- `.vscode/tasks.json` with a Fluxa problem matcher
- `.vscode/extensions.json` recommending Code Runner
- `.vscode/fluxa-language/` with local Fluxa syntax highlighting metadata

Run the current file with Code Runner or the `Fluxa: Run Current File` task.

If `.flx` still shows a Python icon or old highlighting, reload VS Code after opening the workspace.

## Full Example

A comprehensive example using multiple features:

```fluxa
import json
import arrays
import io
import asyncs

async func process_data(data: List<Any>) -> List<Float>:
    let nums = arrays.array(data)
    let processed = nums * 2 + 1
    await asyncs.sleep(0.1)  # Simulate async work
    return processed.to_list()

func main() -> Void:
    try:
        # Read JSON data
        let json_text = io.read("data.json")
        let data: Map<String, Any> = json.parse(json_text)
        let values: List<Any> = data["values"]
        
        # Process asynchronously
        let result = await process_data(values)
        
        # Save result
        let output = {"processed": result}
        io.write("result.json", json.stringify(output))
        
        print("Data processed and saved!")
    catch e:
        print("Error: " + e.message)
```
