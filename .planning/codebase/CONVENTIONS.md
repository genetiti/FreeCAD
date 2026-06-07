# Coding Conventions

**Analysis Date:** 2026-06-07

## Overview

FreeCAD enforces code style conventions through automated tooling (clang-format for C++, Black for Python) and pre-commit hooks. All contributions must follow these conventions before merging.

## Naming Patterns

**C++ Files:**
- Headers: `.h` (no `.hpp`)
- Implementation: `.cpp`
- Pattern: `PascalCase` (e.g., `Axis.h`, `Type.h`, `BaseClass.h`)

**C++ Classes and Types:**
- Class names: `PascalCase` (e.g., `Axis`, `Placement`, `PropertyFloat`, `ApplicationDirectories`)
- Namespace names: `PascalCase` (e.g., `Base`, `App`, `Gui`)
- Enum names: `PascalCase` (e.g., `Base64ErrorHandling`)
- Template parameters: `UPPERCASE` or `PascalCase`

**C++ Member Variables:**
- Private members: `_lowerCamelCase` with leading underscore (e.g., `_base`, `_dir` in `Axis` class)
- Protected members: same as private
- Public accessors: `getXXX()` / `setXXX()` pattern (e.g., `getBase()`, `setBase()`, `getDirection()`, `setDirection()`)

**C++ Functions:**
- Free functions: `camelCase` (e.g., `appendVersionIfPossible`, `isDerivedFrom`)
- Method names: `camelCase` (e.g., `reverse()`, `reversed()`, `move()`)

**Python Files:**
- Modules: `snake_case.py` (e.g., `test_sync_version.py`, `test_creation.py`)
- Classes: `PascalCase` (e.g., `DraftCreation`, `ConsoleTestCase`, `UnitBasicCases`)
- Functions: `snake_case` (e.g., `compare`, `tu`, `ts`)
- Test classes: `PascalCase` with `Test` prefix or suffix (e.g., `UnitBasicCases`, `PropertyFloatTest`)

**Test Files:**
- C++ test files: Named after the component (e.g., `ApplicationDirectories.cpp`, `Property.cpp`)
- Python test files: `Test*.py` or `*Test.py` pattern (e.g., `TestDraft.py`, `TestDraftGui.py`, `UnitTests.py`)
- Test classes: `PascalCase` inheriting from `unittest.TestCase` (Python) or `::testing::Test` (C++)
- Test methods: `test_*` or `test*` (e.g., `testPrint()`, `testConversions()`)

## Code Style

**Formatting Tool: clang-format**
- Configuration file: `.clang-format`
- Auto-formatting on pre-commit (see `.pre-commit-config.yaml`)
- Run formatting: pre-commit hooks apply clang-format automatically

**Clang-Format Key Settings:**
- Based on LLVM style
- Indentation: 4 spaces (no tabs)
- Line length: 100 columns
- Pointer/Reference alignment: Left-aligned (e.g., `const Vector3d& getBase()`)
- Brace wrapping:
  - After class/struct/enum/function: new line
  - After namespace: new line
  - After control statements: same line
  - Before catch/else: new line
- No bin-packing of parameters (each parameter on new line if needed)
- Space before parens: control statements only
- Constructor initializer: break before comma

**Python Formatting: Black**
- Line length: 100 characters (configured in `.pre-commit-config.yaml`)
- Applied via pre-commit hooks

## Import Organization

**C++ Includes:**
- Order: Not enforced (SortIncludes: Never in `.clang-format`)
- Group structure: typically local includes, then third-party, then system
- Use: `#include "relative/path.h"` for local, `#include <system.h>` for system

**Python Imports:**
- Standard library imports first
- Then third-party (e.g., `FreeCAD`, `unittest`)
- Then local imports
- Separated by blank lines

Example from `BaseTests.py`:
```python
import math
import os
import sys
import tempfile
import unittest
import FreeCAD
from FreeCAD import Base
```

## Error Handling

**C++ Error Handling:**
- Exceptions: Uses C++ standard exceptions
- Example pattern: Throwing `std::runtime_error` for exceptional conditions (seen in `ApplicationDirectories.cpp`)
- NOLINT pragmas: Used to suppress clang-tidy warnings for specific sections
  - Example: `/* NOLINTBEGIN(readability-magic-numbers, ...) */` at top of test files

**Python Error Handling:**
- Try-except blocks: Standard Python exception handling
- Pattern: `try:` / `except Exception:` for general exception catching
- Example from `BaseTests.py`:
```python
try:
    import _thread as thread, time
except Exception:
    import thread, time
```

## Logging and Console Output

**C++ Logging:**
- No centralized logging framework observed
- Console messages: Not found in base library patterns

**Python Logging:**
- Uses FreeCAD Console methods for output
- Methods: `FreeCAD.Console.PrintMessage()`, `FreeCAD.Console.PrintError()`, `FreeCAD.Console.PrintWarning()`, `FreeCAD.Console.PrintLog()`
- Example from `BaseTests.py`:
```python
FreeCAD.Console.PrintMessage("   Printing message\n")
FreeCAD.Console.PrintError("   Printing error\n")
```

## Comments and Documentation

**Comment Style:**
- C++ comments: `//` for single line, `/* */` for blocks
- Header documentation: Doxygen-style comments above classes/functions
- Example from `Type.h`:
```cpp
/** Type system class
  Many of the classes in the FreeCAD must have their type
  information registered...
*/
class Type
```

**Inline Comments:**
- Use `//` with two spaces before comment start
- Keep comments brief and meaningful

**Function Documentation:**
- C++: Doxygen-style comments with `\code`, `\endcode` blocks
- Python: Module and class docstrings describing purpose and usage
- Example from `TestDraft.py`:
```python
"""Unit tests for the Draft workbench, non-GUI only.

From the terminal, run the following:
FreeCAD -t TestDraft
```

## File Headers and Licensing

**Required Header Format:**
All files must include SPDX identifier and license block:
```cpp
// SPDX-License-Identifier: LGPL-2.1-or-later

/***************************************************************************
 *   Copyright (c) YYYY Author Name <email>                              *
 *                                                                        *
 *   This file is part of the FreeCAD CAx development system.             *
 *                                                                        *
 *   [Full LGPL license text]                                            *
 ***************************************************************************/
```

Python equivalent:
```python
# SPDX-License-Identifier: LGPL-2.1-or-later

# ***************************************************************************
# *   Copyright (c) YYYY Author Name <email>                              *
# *   [license block]                                                     *
# ***************************************************************************
```

## Function Design

**Size Guidelines:**
- Functions should be reasonably sized
- Clang-format enforces: `AllowShortFunctionsOnASingleLine: None` (one-liners not allowed)
- Example: `reverse()` in `Axis.h` is a full function definition

**Parameters:**
- No bin-packing: each parameter on its own line if needed
- Use const references for non-trivial types: `const Vector3d&`
- Example from `Axis.h`:
```cpp
Axis(const Vector3d& Orig, const Vector3d& Dir);
```

**Return Values:**
- Use simple types or const references
- Getters typically return const references or values
- Example: `const Vector3d& getBase() const`

## Module and Namespace Design

**C++ Namespaces:**
- Hierarchical: `Base`, `App`, `Gui`, `Mod::<ModuleName>`
- No indentation inside namespace (NamespaceIndentation: None)
- Example from `Type.h`:
```cpp
namespace Base
{
class Axis { ... };
}  // namespace Base
```

**Python Modules:**
- Test modules in `src/Mod/<Module>/` directory
- Module organization: `drafttests.test_creation`, `drafttests.test_modification`, etc.
- Imports in test orchestrators import from submodules

**Barrel Files / __init__.py:**
- Not typically used; imports explicit

## Code Quality Checks

**Pre-Commit Hooks** (`.pre-commit-config.yaml`):
- Trailing whitespace removal
- End-of-file fixer
- YAML validation
- Large file detection
- Line ending normalization (mixed line ending check)
- clang-format (C++ code)
- Black (Python code, 100-character line length)
- Version sync checks

**When to Commit:**
- Files must pass pre-commit checks before committing
- Hooks are enforced in workflow via `sub_lint.yml`

**Linting Workflow** (`.github/workflows/sub_lint.yml`):
- Handles code style checks for C++ and Python
- Does NOT duplicate pre-commit checks (formatting handled by pre-commit)
- Additional checks: Qt connection verification, pylint (optional)

## Contribution Guidelines

**From CONTRIBUTING.md:**

**Code Requirements (#5-6):**
1. Contributions MUST adhere to code style guidelines
2. Code MUST compile cleanly
3. Code MUST pass project self-tests on all target platforms

**Commit Message Guidelines (#11-13):**
1. Each commit message MUST succinctly explain what it achieves
2. Follow git commit documentation suggestions
3. Include `Co-Authored-By` trailer if cherry-picking from others
4. Example format (not strict):
```
Brief description of change

Longer explanation if needed.

Fixes #issue-number
```

**Pull Request Requirements:**
- PR title: brief (one line)
- PR body: detailed description with screenshots for UI changes
- Each commit: must compile cleanly with previous commits
- Checkpoint commits: should be squashed

## Special Patterns

**Parameter Types in Functions:**
- Use leading uppercase for parameters that shadow member variables: `Axis(const Vector3d& Orig, const Vector3d& Dir)`
- This convention distinguishes parameters from member variables

**Test Subclass Pattern:**
- For testing protected members, create subclass that exposes them
- Example from `ApplicationDirectories.cpp`:
```cpp
class ApplicationDirectoriesTestClass: public App::ApplicationDirectories
{
    using App::ApplicationDirectories::ApplicationDirectories;
public:
    void wrapAppendVersionIfPossible(...) const { ... }
};
```

---

*Convention analysis: 2026-06-07*
