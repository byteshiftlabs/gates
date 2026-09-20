AST (Abstract Syntax Tree) Implementation
==========================================

Overview
--------

The Abstract Syntax Tree (AST) is the internal representation of the parsed C source code. It is implemented in ``src/core/astnode.c`` and defined in ``include/astnode.h``.

**Key characteristics:**

* **Tree structure**: Parent-child relationships with dynamic child arrays
* **Heterogeneous nodes**: Single ``ASTNode`` struct type with ``NodeType`` enum for differentiation
* **Minimal data**: Stores only type, value, token, and tree structure
* **Manual memory management**: Explicit allocation and deallocation functions

Location
--------

- Source file: ``src/core/astnode.c``
- Header file: ``include/astnode.h``

ASTNode Structure
-----------------

The core AST node structure is defined in ``include/astnode.h``:

.. code-block:: c

   typedef struct ASTNode {
       NodeType type;              // Node type (function, expression, statement, etc.)
       Token token;                // Associated token (for type info, line numbers)
       char *value;                // String value (identifier name, operator, etc.)
       struct ASTNode *parent;     // Parent node (NULL for root)
       struct ASTNode **children;  // Dynamic array of child nodes
       int num_children;           // Current number of children
       int capacity;               // Allocated capacity of children array
   } ASTNode;

**Fields:**

* ``type``: Distinguishes between different node types (see :ref:`node-types`)
* ``token``: Stores the token that created this node (useful for type information, line numbers)
* ``value``: String value for identifiers, operators, literals (dynamically allocated)
* ``parent``: Pointer to parent node for tree traversal (enables bottom-up traversal)
* ``children``: Dynamic array of child pointers (grows as needed)
* ``num_children``: Current number of children in the array
* ``capacity``: Allocated size of children array (doubled when full)

**Memory layout:**

.. code-block:: text

   ASTNode (stack or heap)
    ├─ type: NodeType
    ├─ token: Token struct
     ├─ value: char* → heap-allocated string
     ├─ parent: ASTNode*
     ├─ children: ASTNode** → heap-allocated array
     ├─ num_children: int
     └─ capacity: int

.. _node-types:

Node Types
----------

The ``NodeType`` enum, verbatim from ``include/astnode.h`` (which carries no
per-value comments — the notes below are this page's, not the source's):

.. code-block:: c

   typedef enum {
       NODE_PROGRAM,
       NODE_FUNCTION_DECL,
       NODE_STRUCT_DECL,
       NODE_VAR_DECL,
       NODE_STATEMENT,
       NODE_EXPRESSION,
       NODE_BINARY_EXPR,
       NODE_LITERAL,
       NODE_IDENTIFIER,
       NODE_ASSIGNMENT,
       NODE_BINARY_OP,
       NODE_IF_STATEMENT,
       NODE_ELSE_IF_STATEMENT,
       NODE_ELSE_STATEMENT,
       NODE_WHILE_STATEMENT,
       NODE_FOR_STATEMENT,
       NODE_BREAK_STATEMENT,
       NODE_CONTINUE_STATEMENT,
       NODE_FUNC_CALL
   } NodeType;

**Usage notes:**

* ``NODE_LITERAL`` and ``NODE_IDENTIFIER`` are defined but unused; the parser uses ``NODE_EXPRESSION`` for both
* ``NODE_STRUCT_DECL`` is defined but struct declarations use custom logic in ``parse_struct.c``
* ``NODE_BINARY_OP`` is a misnomer; it actually represents **unary** operators (``!``, ``~``, ``-``)
* ``NODE_BINARY_EXPR`` represents true binary expressions (``a + b``, ``x == y``)

Core AST Functions
------------------

create_node()
~~~~~~~~~~~~~

.. code-block:: c

   ASTNode* create_node(NodeType type);

Creates and initializes a new AST node:

.. code-block:: c

   ASTNode* create_node(NodeType type)
   {
       ASTNode *node = (ASTNode*)malloc(sizeof(ASTNode));

       if (!node) {
           fprintf(stderr, "Fatal: Failed to allocate memory for AST node\n");
           exit(EXIT_FAILURE);
       }

       node->type = type;
       node->value = NULL;
       node->parent = NULL;
       node->children = NULL;
       node->num_children = 0;
       node->capacity = 0;

       return node;
   }

**Behavior:**

* Allocates memory for the node structure
* Initializes all fields to default values
* **Aborts the process** (``fprintf`` to stderr, then ``exit(EXIT_FAILURE)``) if
  allocation fails — unlike every other allocation failure in the parser,
  which reports via ``log_error()`` and returns ``NULL``/``void`` for the
  caller to handle. ``create_node()`` itself never returns ``NULL``: a few
  call sites (e.g. ``parse_struct_field()`` in ``parse_struct.c``) still have
  a defensive ``if (!field_node)`` check right after calling it — that check
  is unreachable in practice, since the OOM path never gets that far back to
  the caller

**Memory allocation:**

* Node structure is allocated on the heap (``malloc``)
* ``value`` field is NULL (must be set separately, via ``safe_strdup()`` — see :doc:`parser`)
* ``children`` array is NULL (allocated on first ``add_child()`` call)

free_node()
~~~~~~~~~~~

.. code-block:: c

   void free_node(ASTNode *node);

Recursively frees an AST node and all its descendants:

.. code-block:: c

   void free_node(ASTNode *node) {
       if (!node) return;
       
       // Recursively free all children
       for (int i = 0; i < node->num_children; i++) {
           free_node(node->children[i]);
       }
       
       // Free children array
       free(node->children);
       
       // Free value string
       free(node->value);
       
       // Free node itself
       free(node);
   }

**Behavior:**

* Post-order traversal: frees children before parent
* Handles NULL nodes gracefully (early return)
* Frees all dynamically allocated memory:
  
  - Children nodes (recursive)
  - Children array pointer
  - Value string
  - Node structure itself

**Usage:**

Typically called only on the root node (``NODE_PROGRAM``), which recursively frees the entire AST:

.. code-block:: c

   ASTNode *root = parse_program(input);
   // ... use AST
   free_node(root);  // Frees entire tree

add_child()
~~~~~~~~~~~

.. code-block:: c

   void add_child(ASTNode *parent, ASTNode *child);

Adds a child node to a parent node, growing the children array if necessary:

.. code-block:: c

   void add_child(ASTNode *parent, ASTNode *child) {
       if (!parent || !child) {
           log_error(ERROR_CATEGORY_GENERAL, 0,
                     "add_child called with NULL %s", !parent ? "parent" : "child");
           return;
       }

       // Allocate initial children array if needed
       if (!parent->children) {
           parent->capacity = INITIAL_CHILDREN_CAPACITY;
           parent->children = (ASTNode**)malloc(parent->capacity * sizeof(ASTNode*));
           if (!parent->children) {
               log_error(ERROR_CATEGORY_GENERAL, 0, "Failed to allocate memory for child nodes");
               return;
           }
       }
       
       // Grow the array if it's full. Capacity is committed only after the
       // reallocation succeeds — raising it first left the node claiming
       // space it didn't own on failure, so the *next* add_child call would
       // skip this branch and write past the still-undersized old block.
       if (parent->num_children >= parent->capacity) {
           int new_capacity = parent->capacity * CHILDREN_GROWTH_FACTOR;
           ASTNode **new_children = (ASTNode**)realloc(parent->children,
                                                (size_t)new_capacity * sizeof(ASTNode*));
           if (!new_children) {
               log_error(ERROR_CATEGORY_GENERAL, 0, "Failed to reallocate memory for child nodes");
               return;
           }
           parent->children = new_children;
           parent->capacity = new_capacity;
       }

       // Add child and set parent pointer
       parent->children[parent->num_children++] = child;
       child->parent = parent;
   }

**Behavior:**

* **Lazy initialization**: allocates the children array on the first child (initial capacity = 4)
* **Dynamic growth**: doubles capacity when the current array is full
* **Bidirectional links**: sets the child's ``parent`` pointer for bottom-up traversal
* **Null safety**: checks for NULL ``parent`` or ``child`` and reports via ``log_error()`` instead of crashing
* **Error handling**: reports allocation/reallocation failure via ``log_error()`` and returns early (no ``exit`` here, unlike ``create_node()``)
* **Capacity committed only on success**: ``parent->capacity`` is only updated to ``new_capacity`` after ``realloc()`` returns non-NULL, so a failed grow leaves the struct's bookkeeping consistent with the array it actually still owns

**Capacity growth sequence:**

.. code-block:: text

   Initial: capacity = 0, children = NULL
   1st child: capacity = 4
   5th child: capacity = 8
   9th child: capacity = 16
   17th child: capacity = 32
   ...

AST Examples
------------

Simple Expression
~~~~~~~~~~~~~~~~~

C code:

.. code-block:: c

   3 + 4 * 5

AST structure:

.. code-block:: text

   NODE_BINARY_EXPR (value = "+")
     ├─ NODE_EXPRESSION (value = "3")
     └─ NODE_BINARY_EXPR (value = "*")
          ├─ NODE_EXPRESSION (value = "4")
          └─ NODE_EXPRESSION (value = "5")

Note: Precedence is reflected in tree structure (multiplication deeper than addition).

Variable Declaration with Initialization
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

C code:

.. code-block:: c

   int x = 10;

AST structure:

.. code-block:: text

   NODE_STATEMENT
     └─ NODE_VAR_DECL (token.value = "int", value = "x")
          └─ NODE_EXPRESSION (value = "10")

If-Else Statement
~~~~~~~~~~~~~~~~~

C code:

.. code-block:: c

   if (x > 0) {
       y = 1;
   } else {
       y = -1;
   }

AST structure:

.. code-block:: text

   NODE_IF_STATEMENT
     ├─ NODE_BINARY_EXPR (value = ">")
     │    ├─ NODE_EXPRESSION (value = "x")
     │    └─ NODE_EXPRESSION (value = "0")
     ├─ NODE_STATEMENT
     │    └─ NODE_ASSIGNMENT
     │         ├─ NODE_EXPRESSION (value = "y")
     │         └─ NODE_EXPRESSION (value = "1")
     └─ NODE_ELSE_STATEMENT
          └─ NODE_STATEMENT
               └─ NODE_ASSIGNMENT
                    ├─ NODE_EXPRESSION (value = "y")
                    └─ NODE_EXPRESSION (value = "-1")

For Loop
~~~~~~~~

C code:

.. code-block:: c

   for (int i = 0; i < 10; i++) {
       sum = sum + i;
   }

AST structure:

.. code-block:: text

   NODE_FOR_STATEMENT
     ├─ NODE_VAR_DECL (token.value = "int", value = "i")
     │    └─ NODE_EXPRESSION (value = "0")
     ├─ NODE_BINARY_EXPR (value = "<")
     │    ├─ NODE_EXPRESSION (value = "i")
     │    └─ NODE_EXPRESSION (value = "10")
     ├─ NODE_STATEMENT
     │    └─ NODE_ASSIGNMENT
     │         ├─ NODE_EXPRESSION (value = "sum")
     │         └─ NODE_BINARY_EXPR (value = "+")
     │              ├─ NODE_EXPRESSION (value = "sum")
     │              └─ NODE_EXPRESSION (value = "i")
     └─ NODE_ASSIGNMENT (increment, last child)
          ├─ NODE_EXPRESSION (value = "i")
          └─ NODE_BINARY_EXPR (value = "+")
               ├─ NODE_EXPRESSION (value = "i")
               └─ NODE_EXPRESSION (value = "1")

Function Declaration
~~~~~~~~~~~~~~~~~~~~

C code:

.. code-block:: c

   int add(int a, int b) {
       return a + b;
   }

AST structure:

.. code-block:: text

   NODE_FUNCTION_DECL (token.value = "int", value = "add")
     ├─ NODE_VAR_DECL (token.value = "int", value = "a")
     ├─ NODE_VAR_DECL (token.value = "int", value = "b")
     └─ NODE_STATEMENT (token.value = "return")
          └─ NODE_BINARY_EXPR (value = "+")
               ├─ NODE_EXPRESSION (value = "a")
               └─ NODE_EXPRESSION (value = "b")

Array Access
~~~~~~~~~~~~

C code:

.. code-block:: c

   arr[i + 1] = 42;

AST structure:

.. code-block:: text

   NODE_ASSIGNMENT
     ├─ NODE_EXPRESSION (value = "arr[i+1]")
     └─ NODE_EXPRESSION (value = "42")

Note: Array indexing is **not** represented as a tree structure; it's stored as a flat string in the expression node's ``value`` field (``"arr[i+1]"``). This simplifies code generation.

Struct Field Access
~~~~~~~~~~~~~~~~~~~

C code:

.. code-block:: c

   point.x = 10;

AST structure:

.. code-block:: text

   NODE_ASSIGNMENT
     ├─ NODE_EXPRESSION (value = "point__x")
     └─ NODE_EXPRESSION (value = "10")

Note: Field access is **flattened** at parse time using ``__`` separator (``point.x`` becomes ``point__x``). This avoids the need for a separate field access node type.

Tree Traversal Utilities
-------------------------

print_ast()
~~~~~~~~~~~

``src/core/utils.c`` (not ``astnode.c``) provides ``print_ast()``, declared in ``include/utils.h``:

.. code-block:: c

   void print_ast(ASTNode* node, int level);

.. important::
   ``print_ast()`` is only ever called from ``src/app/gates.c``, and only
   inside an ``#ifdef DEBUG`` block — it does not run in a normal build. To
   see it, configure with ``cmake -DDEBUG=ON`` (see :doc:`../usage`).

The traversal itself delegates the per-node-type formatting to a helper, ``print_node_type()``, so the tree-walking logic stays separate from the display format:

.. code-block:: c

   // Box-drawing characters (└── / ├──) give visual structure to the AST dump.
   static void print_tree_prefix(int level, int is_last)
   {
       for (int level_idx = 0; level_idx < level; level_idx++) {
           printf("%s", (level_idx == level - 1) ? (is_last ? "└── " : "├── ") : "    ");
       }
   }

   // Each AST node type has a distinct display format for the debug tree.
   static void print_node_type(const ASTNode *node)
   {
       switch (node->type) {
           case NODE_PROGRAM:
               printf("PROGRAM\n");
               break;
           case NODE_FUNCTION_DECL:
               printf("FUNCTION: %s (returns: %s)\n",
                      node->value ? node->value : "(null)", node->token.value);
               break;
           case NODE_VAR_DECL:
               printf("VAR: %s %s\n",
                      node->token.value, node->value ? node->value : "(null)");
               break;
           case NODE_EXPRESSION:
               printf("EXPR: %s\n", node->value ? node->value : "(null)");
               break;
           case NODE_BINARY_EXPR:
               printf("BINARY: %s\n", node->value ? node->value : "(op)");
               break;
           case NODE_ASSIGNMENT:
               printf("ASSIGN\n");
               break;
           // ...one case per remaining NodeType, including NODE_STRUCT_DECL,
           // NODE_LITERAL, and NODE_IDENTIFIER, plus a default that prints
           // "NODE_TYPE_%d" for anything unrecognized...
       }
   }

   void print_ast(ASTNode* node, int level)
   {
       if (!node) return;

       int is_last = 1;
       if (node->parent) {
           const ASTNode *parent = node->parent;
           for (int i = 0; i < parent->num_children; i++) {
               if (parent->children[i] == node) {
                   is_last = (i == parent->num_children - 1);
                   break;
               }
           }
       }

       print_tree_prefix(level, is_last);
       print_node_type(node);

       for (int i = 0; i < node->num_children; i++) {
           print_ast(node->children[i], level + 1);
       }
   }

**Example output:**

.. code-block:: text

   PROGRAM
   └── FUNCTION: main (returns: int)
       ├── VAR: int x
       │   └── EXPR: 10
       ├── STATEMENT
       │   └── ASSIGN
       │       ├── EXPR: x
       │       └── EXPR: 20
       └── STATEMENT (return)
           └── EXPR: x

Memory Management Patterns
---------------------------

**Allocation:**

.. code-block:: c

   ASTNode *node = create_node(NODE_EXPRESSION);
   node->value = safe_strdup("foo");  // Allocate value string

**Child addition:**

.. code-block:: c

   ASTNode *parent = create_node(NODE_BINARY_EXPR);
   parent->value = safe_strdup("+");

   ASTNode *left = create_node(NODE_EXPRESSION);
   left->value = safe_strdup("3");

   ASTNode *right = create_node(NODE_EXPRESSION);
   right->value = safe_strdup("4");

   add_child(parent, left);   // Parent adopts left
   add_child(parent, right);  // Parent adopts right

**Deallocation:**

.. code-block:: c

   free_node(parent);  // Recursively frees parent, left, right, and all value strings

**Key invariants:**

* All nodes are heap-allocated via ``create_node()`` (which aborts the process on OOM rather than returning NULL — see above)
* All ``value`` fields are heap-allocated via ``safe_strdup()`` (``src/core/utils.c``) or NULL
* All ``children`` arrays are heap-allocated when non-NULL
* Parent nodes "own" their children (responsible for freeing them)
* Root node must be freed with ``free_node()`` to avoid leaks

Operational Characteristics
---------------------------

* Node allocation is explicit and paired with ``free_node()`` cleanup
* Child arrays are created lazily and expanded as the tree grows
* Recursive traversal utilities depend on tree depth and structure
* Memory use depends on node count, child fan-out, and stored string values

Design Decisions
----------------

**Single node type:**

* **Pro**: Simple, uniform interface
* **Pro**: Easy to extend with new node types
* **Con**: No compile-time type safety for node-specific data
* **Con**: Wastes memory (all nodes have unused ``token`` field)

**Dynamic children array:**

* **Pro**: Works well for nodes with many children (statements, function bodies)
* **Pro**: No fixed limits on tree width
* **Con**: Memory overhead for leaf nodes (NULL children array)
* **Con**: Reallocation overhead during tree construction

**Bidirectional links:**

* **Pro**: Enables bottom-up traversal (child to parent)
* **Pro**: Useful for semantic analysis (e.g., finding enclosing function)
* **Con**: Additional per-node storage for the ``parent`` pointer
* **Con**: Must maintain invariant during tree manipulation

**String values:**

* **Pro**: Simple and flexible (stores any string data)
* **Pro**: Easy to print for debugging
* **Con**: Wastes memory for numeric literals (stores "42" instead of 42)
* **Con**: Requires string parsing during code generation

Limitations and Future Improvements
------------------------------------

**Current limitations:**

* No source position information (only line numbers from tokens)
* No distinction between L-values and R-values in expressions
* Array indexing stored as flat strings (no AST representation of index expressions)
* Struct field access flattened to identifiers (loses semantic structure)
* No type annotations on nodes (type inference required for code generation)

**Potential improvements:**

1. **Typed node variants**: Use C11 anonymous unions for node-specific data
2. **Symbol table integration**: Store symbol pointers in identifier nodes
3. **Type annotations**: Add ``type`` field for semantic analysis results
4. **Source ranges**: Store start/end positions for better error messages
5. **Interned strings**: Use string interning for ``value`` fields to save memory
6. **Pooled allocation**: Use memory pool for nodes instead of individual ``malloc()`` calls
7. **Constant folding**: Evaluate constant expressions during tree construction
8. **Tree transformation API**: Provide functions for safe tree manipulation (not just construction)

Summary
-------

The AST implementation provides:

* **Simple, uniform node structure** with dynamic children arrays
* **Minimal memory management API**: ``create_node()``, ``add_child()``, ``free_node()``
* **Geometric child-array growth** as nodes are added
* **Bidirectional tree links** for flexible traversal
* **Recursive memory deallocation** for straightforward cleanup

The design prioritizes **simplicity and ease of use** over more specialized representations, making it straightforward to construct and traverse the AST during parsing and code generation.
