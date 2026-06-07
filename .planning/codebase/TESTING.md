# Testing Patterns

**Analysis Date:** 2026-06-07

## Test Frameworks Overview

FreeCAD uses two primary testing frameworks depending on language and layer:

| Framework | Language | Purpose | Location |
|-----------|----------|---------|----------|
| **Google Test (GTest)** | C++ | Unit tests for core libraries (Base, App, Gui, Modules) | `tests/src/` |
| **Python unittest** | Python | Functional and integration tests for workbenches | `src/Mod/*/Test*.py` |

## C++ Unit Tests (Google Test / GTest)

### Framework Configuration

**Runner:** Google Test (GTest)
- Configuration: CMake-based integration
- Config files: `tests/src/*/CMakeLists.txt`, `tests/CMakeLists.txt`
- Main build flag: `ENABLE_DEVELOPER_TESTS` (CMake option)
- Test execution: `ctest` command

**Assertion Library:** Google Test (EXPECT_*, ASSERT_* macros)

### Test Execution

**Run C++ Tests:**
```bash
# Configure with developer tests enabled
cmake -DENABLE_DEVELOPER_TESTS=ON [other options] ..

# Build tests
cmake --build . --target tests

# Run all tests
ctest

# Run tests with verbose output
ctest --verbose

# Run specific test suite
ctest -R "PropertyLink|Property" --verbose

# Run specific test
ctest -R "PropertyLink::TestSetValues" --verbose
```

**Test Executable Naming:**
- Format: `<Component>_tests_run`
- Examples: `App_tests_run`, `Base_tests_run`, `Gui_tests_run`, `Mesh_tests_run`, `Assembly_tests_run`
- Location: `tests/src/<Component>/`

### Test Directory Structure

```
tests/
├── CMakeLists.txt                    # Root test config
├── src/
│   ├── CMakeLists.txt                # Test source configuration
│   ├── App/
│   │   ├── CMakeLists.txt            # App tests config (GTest)
│   │   ├── ApplicationDirectories.cpp
│   │   ├── Property.cpp
│   │   ├── Document.cpp
│   │   ├── Expression.cpp
│   │   ├── ...
│   │   └── [30+ test files]
│   ├── Base/
│   │   ├── CMakeLists.txt
│   │   └── [Base component tests]
│   ├── Gui/
│   │   └── CMakeLists.txt
│   ├── Mod/
│   │   ├── Mesh/
│   │   ├── Sketcher/
│   │   ├── Assembly/
│   │   ├── Part/
│   │   └── [Module-specific GTest tests]
│   └── zipios++/
└── [GTest binary output]
```

### Test File Structure (GTest)

**Basic Test Structure:**
```cpp
#include <gtest/gtest.h>
#include <relevant/headers.h>

TEST(TestSuite, TestName)
{
    // Arrange
    Type instance;
    
    // Act
    instance.doSomething();
    
    // Assert
    EXPECT_EQ(result, expected);
}
```

**Fixture-Based Tests:**
```cpp
class PropertyFloatTest: public ::testing::Test
{
protected:
    static void SetUpTestSuite()
    {
        // One-time setup for all tests in suite
        XERCES_CPP_NAMESPACE::XMLPlatformUtils::Initialize();
    }
    
    void setUp()
    {
        // Per-test setup
    }
    
    void tearDown()
    {
        // Per-test cleanup
    }
};

TEST_F(PropertyFloatTest, testWriteRead)
{
    // Access SetUpTestSuite() resources
    // Test body
}
```

**Example from `Property.cpp`:**
```cpp
TEST(PropertyLink, TestSetValues)
{
    App::PropertyLinkSubList prop;
    std::vector<App::DocumentObject*> objs {nullptr, nullptr};
    std::vector<const char*> subs {"Sub1", "Sub2"};
    prop.setValues(objs, subs);
    const auto& sub = prop.getSubValues();
    EXPECT_EQ(sub.size(), 2);
    EXPECT_EQ(sub[0], "Sub1");
    EXPECT_EQ(sub[1], "Sub2");
}
```

### GTest Assertion Patterns

**Common Assertions:**
- `EXPECT_EQ(actual, expected)` - Check equality (test continues on failure)
- `EXPECT_NE(a, b)` - Not equal
- `EXPECT_TRUE(condition)` - Boolean true
- `EXPECT_FALSE(condition)` - Boolean false
- `EXPECT_STREQ(str1, str2)` - String equality
- `EXPECT_DOUBLE_EQ(a, b)` - Double equality
- `EXPECT_THROW(statement, exception_type)` - Exception checking
- `ASSERT_*` variants - Same as EXPECT_* but halt test on failure

**Examples from codebase:**
```cpp
EXPECT_EQ(sub.size(), 2);
EXPECT_EQ(sub[0], "Sub1");
EXPECT_DOUBLE_EQ(prop2.getValue(), value);
EXPECT_TRUE(isRenamed);
EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
EXPECT_EQ(varSet->getDynamicPropertyByName("Variable"), nullptr);
```

### Test Setup and Teardown

**CMakeLists.txt Pattern:**
```cmake
add_executable(App_tests_run
    AsyncRecompute.cpp
    Application.cpp
    ApplicationDirectories.cpp
    # ... more test files
)

target_compile_definitions(App_tests_run PRIVATE DATADIR="${CMAKE_SOURCE_DIR}/data")

target_link_libraries(App_tests_run PRIVATE
    GTest::gtest_main
    GTest::gmock_main
    ${Python3_LIBRARIES}
    FreeCADApp
)
```

**Key Points:**
- `GTest::gtest_main` provides main() function
- Tests link against the component they're testing (e.g., `FreeCADApp` for App tests)
- Python libraries linked for Python bindings testing
- Preprocessor defines: `DATADIR` for test data location

### Mocking

**Framework:** Google Mock (gmock) - included with GTest
- Linked via `GTest::gmock_main`
- Not heavily used in observed test files

**Pattern:** Not extensively documented in visible tests; mainly using real objects

### Test Data and Fixtures

**Shared Test Data:**
- Location: `data/` directory (referenced by `DATADIR` CMake define)
- Test files access via `DATADIR` macro
- Example: Setting locale and using XML fixtures in `Property.cpp`

**Fixture Pattern (Example from Property.cpp):**
```cpp
class RenameProperty: public ::testing::Test
{
    // Shared setup/teardown
protected:
    static std::string _docName;
    static App::Document* _doc;
    App::PropertyFloat* prop;
    App::VarSet* varSet;
    
    static void SetUpTestSuite()
    {
        // Initialize once for all tests
    }
    
    void setUp()
    {
        // Per-test setup
    }
};

std::string RenameProperty::_docName;
App::Document* RenameProperty::_doc {nullptr};

TEST_F(RenameProperty, renameProperty)
{
    // Test uses fixture members
}
```

## Python Unit Tests (unittest)

### Framework Configuration

**Runner:** Python unittest
- Invoked by: FreeCAD command line with `-t` option or from FreeCAD console
- Discovery: Modules named `Test*.py` or `*Test.py` in `src/Mod/` directories

**Module Structure:**
- Test orchestrator files import submodule test classes
- Submodule classes inherit from `unittest.TestCase`
- Execution via FreeCAD's Test framework or direct `unittest` runner

### Test Execution

**Run Python Tests:**
```bash
# From command line
FreeCAD -t TestDraft                          # Run all Draft tests
FreeCAD -t drafttests.test_creation           # Run specific module
FreeCAD -t drafttests.test_creation.DraftCreation  # Run test class
FreeCAD -t drafttests.test_creation.DraftCreation.test_line  # Run specific test

# From Python console within FreeCAD
import Test, TestDraft
Test.runTestsFromModule(TestDraft)

# From unittest directly
import unittest
one_test = "drafttests.test_creation.DraftCreation.test_line"
all_tests = unittest.TestLoader().loadTestsFromName(one_test)
unittest.TextTestRunner().run(all_tests)
```

### Test Directory Structure

**Python Test Organization:**
```
src/Mod/Draft/
├── TestDraft.py               # Orchestrator - imports submodule tests
├── TestDraftGui.py            # GUI tests
└── drafttests/
    ├── __init__.py
    ├── test_creation.py       # DraftCreation test class
    ├── test_modification.py   # DraftModification test class
    ├── test_draftgeomutils.py # TestDraftGeomUtils test class
    ├── test_svg.py
    ├── test_dxf.py
    └── test_array.py

src/Mod/Test/
├── BaseTests.py              # Basic test class (ConsoleTestCase, etc.)
├── UnitTests.py              # Unit tests (UnitBasicCases, etc.)
├── TestGui.py                # GUI tests
├── TestPerf.py               # Performance tests
└── [other test modules]
```

### Test File Structure (unittest)

**Basic Test Structure:**
```python
import unittest
import FreeCAD
from FreeCAD import Base

class TestClassName(unittest.TestCase):
    def setUp(self):
        """Per-test setup"""
        self.count = 0
        self.some_value = 100
    
    def tearDown(self):
        """Per-test cleanup"""
        pass
    
    def test_something(self):
        """Test description"""
        # Arrange
        result = some_function()
        
        # Assert
        self.assertEqual(result, expected)
        self.assertTrue(condition)
```

**Example from BaseTests.py (ConsoleTestCase):**
```python
import unittest
import FreeCAD

class ConsoleTestCase(unittest.TestCase):
    def setUp(self):
        self.count = 0

    def testPrint(self):
        FreeCAD.Console.PrintMessage("   Printing message\n")
        FreeCAD.Console.PrintError("   Printing error\n")
        FreeCAD.Console.PrintWarning("   Printing warning\n")
        FreeCAD.Console.PrintLog("   Printing Log\n")

    def testSynchronPrintFromThread(self):
        try:
            import _thread as thread, time
        except Exception:
            import thread, time
        
        def adder():
            lock.acquire()
            self.count = self.count + 1
            FreeCAD.Console.PrintMessage(f"Call from thread: count={self.count}\n")
            lock.release()
        
        lock = thread.allocate_lock()
        for i in range(10):
            thread.start_new(adder, ())
```

**Example from UnitTests.py (UnitBasicCases):**
```python
import unittest
import FreeCAD
import math

def tu(str):
    """FreeCAD unit translator"""
    return FreeCAD.Units.Quantity(str).Value

def compare(x, y):
    """Compare floating point with tolerance"""
    return math.fabs(x - y) < 0.00001

class UnitBasicCases(unittest.TestCase):
    def setUp(self):
        par = FreeCAD.ParamGet("User parameter:BaseApp/Preferences/Units")
        dec = par.GetInt("Decimals")
        self.delta = math.pow(10, -dec)

    def testConversions(self):
        self.assertTrue(compare(tu("10 m"), 10000.0))
        self.assertTrue(compare(tu("3/8 in"), 9.525))

    def testImperial(self):
        self.assertTrue(compare(tu("3/8in"), 9.525))
        
        psi = FreeCAD.Units.parseQuantity("1psi")
        mpa = psi.getValueAs("MPa").Value
        self.assertAlmostEqual(0.0068947572932, mpa, delta=self.delta)
```

### Assertion Methods (unittest)

**Common Assertions:**
- `assertEqual(a, b)` - Check equality
- `assertNotEqual(a, b)` - Not equal
- `assertTrue(expr)` - Boolean true
- `assertFalse(expr)` - Boolean false
- `assertAlmostEqual(a, b, places=7, delta=None)` - Float comparison with tolerance
- `assertIn(a, b)` - Check containment
- `assertRaises(exception_type, callable, *args)` - Exception checking

### Test Orchestrator Pattern

**From TestDraft.py:**
```python
# Import tests from submodules with ordered names to control execution order
from drafttests.test_import import DraftImport as DraftTest01
from drafttests.test_creation import DraftCreation as DraftTest02
from drafttests.test_modification import DraftModification as DraftTest03
from drafttests.test_draftgeomutils import TestDraftGeomUtils as DraftTest04
from drafttests.test_svg import DraftSVGExportRegression as DraftTest05
from drafttests.test_dxf import DraftDXF as DraftTest06
from drafttests.test_array import DraftArray as DraftTest10

# Use to prevent linters from complaining
True if DraftTest01 else False
True if DraftTest02 else False
# ... etc
```

**Pattern Explanation:**
1. Each test class imported with numbered prefix (Test01, Test02, etc.) to control execution order
2. Classes run in alphabetical order, so numbering enforces a specific test sequence
3. Tests can be imported conditionally (e.g., `from drafttests.test_dwg import ... # commented out`)
4. Dummy usage prevents linter warnings about unused imports

### Test Discovery by FreeCAD

**File Pattern:**
- Modules ending in `Test.py` or starting with `Test_` are discovered
- Module must contain classes inheriting from `unittest.TestCase`
- Classes named `*Test*` or `*TestCase` are recognized

**Execution via FreeCAD Framework:**
- `Test.runTestsFromModule(module)` - Run all tests in a module
- `Test.runTestsFromClass(class)` - Run all tests in a class
- `-t` command line flag: `FreeCAD -t module.TestClass.test_method`

## Test Coverage

**C++ Coverage:**
- Not enforced by CMake (no coverage targets detected)
- Can be measured with tools like gcov/lcov if compiler flags set

**Python Coverage:**
- Not enforced by unittest framework configuration
- Can be measured with `coverage` package if installed

## Integration Between C++ and Python Tests

**App/Document Testing:**
- Python tests can instantiate FreeCAD documents and objects
- Access C++ objects through Python bindings
- Example from Property.cpp tests accessing Python API:
```cpp
TEST_F(RenameProperty, renamePropertyPython)
{
    // Call Python code from C++ test
    Base::Interpreter().runString(
        "App.ActiveDocument.getObject('VarSet').renameProperty('Variable', 'NewName')"
    );
    
    // Then verify C++ object state
    EXPECT_STREQ(varSet->getPropertyName(prop), "NewName");
}
```

## CI/CD Testing Integration

**Test Execution in Workflows:**
- Workflow: `.github/workflows/sub_lint.yml` - Code style/linting
- Workflow: `.github/workflows/sub_buildPixi.yml` - Build and test
- Tests run on all target platforms (Linux, macOS, Windows)

**Requirements for PR Merge (CONTRIBUTING.md #9):**
- Code MUST compile cleanly
- Code MUST pass project self-tests on all target platforms
- No test failures allowed

## Test Organization Best Practices

**When Adding New Tests:**

**For C++ (tests/src/):**
1. Place test file in `tests/src/<Component>/` directory (e.g., `tests/src/App/MyComponent.cpp`)
2. Inherit from `::testing::Test` for fixtures with setup/teardown
3. Use `TEST()` macro for simple tests or `TEST_F()` for fixture-based tests
4. Add test file to `CMakeLists.txt` in that directory
5. Link against the component under test
6. Use appropriate GTest assertions (EXPECT_*, ASSERT_*)

**For Python (src/Mod/<Module>/):**
1. Create test file in `src/Mod/<Module>/Test*.py` or submodule in `drafttests/test_*.py`
2. Inherit from `unittest.TestCase`
3. Implement `setUp()` and `tearDown()` methods
4. Name test methods `test_*` or `testXxx`
5. Use unittest assertions (assertEqual, assertTrue, etc.)
6. Import in orchestrator (e.g., `TestDraft.py`) with numbered prefix for ordering
7. Document test execution command in module docstring

## Special Testing Patterns

**Locale Testing:**
- Example from Property.cpp: Set locale for float serialization testing
```cpp
setlocale(LC_ALL, "");
setlocale(LC_NUMERIC, "C");  // avoid rounding of floating point numbers
```

**XML Fixture Testing:**
- Tests build XML strings and use `Base::XMLReader` to parse them
- Verify round-trip: serialize → deserialize → verify

**Thread-Safe Testing:**
- Example from BaseTests.py: Test thread-safe console printing
- Use thread locks to coordinate test assertions across threads

**Exception Testing (GTest):**
```cpp
EXPECT_THROW(expression, exception_type);
```

**Python Exception Testing:**
```python
with self.assertRaises(ExceptionType):
    function_that_raises()
```

## Running Tests from Command Line

**Full Test Suite:**
```bash
# Build tests
cmake --build . --target tests

# Run all tests
ctest --output-on-failure

# Run with verbose output
ctest -VV
```

**Specific Tests:**
```bash
# Run all Property tests
ctest -R Property -VV

# Run single test
ctest -R "PropertyLink::TestSetValues" -VV

# Run Python tests
FreeCAD -t drafttests.test_creation.DraftCreation
```

---

*Testing analysis: 2026-06-07*
