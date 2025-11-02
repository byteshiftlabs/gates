Algorithms
==========

Key algorithms used in the compiler implementation.

Precedence Climbing
-------------------

Used for expression parsing with correct operator precedence and associativity.

See :doc:`parser` for detailed implementation.

Recursive Descent
-----------------

Core parsing strategy - each grammar rule maps to a function.

See :doc:`parser` for detailed implementation.

Future Algorithms
-----------------

- **Symbol table lookup optimization**: Hash tables instead of linear search
- **Dead code elimination**: Remove unreachable code
- **Constant folding**: Evaluate constant expressions at compile time
- **Loop unrolling**: Optimize small loops
