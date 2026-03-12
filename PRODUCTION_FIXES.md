# Production Fixes for v0.1.0 Release

**Status:** 72% ready (20/28 items pass)

---

## HIGH Priority — Must Fix Before Release

### 1. Split parse_statement.c (874 → under 500 lines each)
- **File:** [src/parser/parse_statement.c](src/parser/parse_statement.c)
- **Issue:** 874 lines exceeds 500-line limit
- **Action:** Split into control-flow statements and variable/expression statements

### 2. Fix bounds check in codegen_vhdl_expressions.c
- **File:** [src/codegen/codegen_vhdl_expressions.c](src/codegen/codegen_vhdl_expressions.c#L20)
- **Issue:** Accesses `node->children[1]` without verifying `num_children >= 2`
- **Risk:** Crash on malformed AST
- **Action:** Add bounds validation before array access

### 3. Fix memory leak in parse_statement.c
- **File:** [src/parser/parse_statement.c](src/parser/parse_statement.c#L557)
- **Issue:** `init_stmt` wrapper node not freed in `parse_for_init`
- **Action:** Free the wrapper node after extracting child

### 4. Remove duplicate declarations
- **Files:** [include/parse.h](include/parse.h), [include/astnode.h](include/astnode.h)
- **Issue:** `create_node`/`add_child`/`free_node` declared in both headers
- **Action:** Keep declarations in astnode.h only, remove from parse.h

### 5. Extract safe_append/safe_copy to shared utility
- **Files:** [src/parser/parse_statement.c](src/parser/parse_statement.c), [src/parser/parse_expression.c](src/parser/parse_expression.c)
- **Issue:** Duplicate implementation of buffer-safe string helpers
- **Action:** Move to src/core/utils.c with declarations in include/utils.h

### 6. Fix 4 compiler warnings (-Waddress)
- **Files:**
  - [src/parser/parse_statement.c](src/parser/parse_statement.c#L203)
  - [src/parser/parse_statement.c](src/parser/parse_statement.c#L237)
  - [src/parser/parse_expression.c](src/parser/parse_expression.c#L182)
  - [src/codegen/codegen_vhdl_main.c](src/codegen/codegen_vhdl_main.c#L173)
- **Issue:** Checking `node->value` (array) against NULL always evaluates to true
- **Action:** Remove impossible NULL checks on stack arrays

### 7. Create version tag
- **Action:** `git tag -a v0.1.0 -m "Initial production release"`

### 8. Add CONTRIBUTING.md
- **Action:** Create repo-root CONTRIBUTING.md based on docs/source/contributing.rst

---

## MEDIUM Priority — Should Fix

### 9. Add parser/codegen integration tests
- **Issue:** No end-to-end C → VHDL translation tests
- **Action:** Add integration tests in tests/integration_tests.cpp

### 10. Add edge case tests
- **Issue:** Missing tests for empty input, malformed syntax, deeply nested expressions
- **Action:** Extend test coverage in tests/

### 11. Fix double indentation bug
- **File:** [src/codegen/codegen_vhdl_statements.c](src/codegen/codegen_vhdl_statements.c#L293)
- **Issue:** Lines 293 and 325 both emit indentation for non-array assignments
- **Action:** Remove redundant indentation on line 325

### 12. Document tested compiler versions
- **File:** [README.md](README.md)
- **Action:** Add "Tested on GCC X.X and Clang X.X" to Requirements section

### 13. Add function-level doc comments
- **Files:** All public API headers in include/
- **Action:** Add Doxygen-style comments for parameters, return values, exceptions

---

## LOW Priority — Nice to Have

### 14. Fix is_numeric_literal accepting multiple dots
- **File:** [src/codegen/codegen_vhdl_helpers.c](src/codegen/codegen_vhdl_helpers.c#L126)
- **Issue:** Accepts "1.2.3" as numeric
- **Action:** Add check for at most one decimal point

### 15. Fix silent 255+ char identifier truncation
- **File:** [src/parser/token.c](src/parser/token.c#L118)
- **Issue:** No error/warning for identifiers longer than 255 characters
- **Action:** Report error when identifier exceeds buffer

### 16. Remove unused TOKEN_STRING enum
- **File:** [include/token.h](include/token.h)
- **Issue:** Enum value exists but lexer never produces it
- **Action:** Remove or implement string literal tokenization

### 17. Squash "Update README.md" commits
- **Action:** `git rebase -i` to clean up commit history before release

---

## Progress Tracking

- [x] HIGH #1: Split parse_statement.c (430 + 429 lines)
- [x] HIGH #2: Fix bounds check in codegen_vhdl_expressions.c
- [x] HIGH #3: Fix memory leak in parse_for_init
- [x] HIGH #4: Remove duplicate declarations
- [x] HIGH #5: Extract safe_append/safe_copy to utils
- [x] HIGH #6: Fix 4 compiler warnings
- [ ] HIGH #7: Create v0.1.0 tag
- [x] HIGH #8: Add CONTRIBUTING.md
