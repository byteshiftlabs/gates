Welcome to Gates's documentation!
=================================

.. toctree::
   :maxdepth: 2
   :caption: Contents:

   usage
   examples
   modules
   architecture
   internals
   code_quality
   testing
   contributing
   known_issues
   license

Introduction
------------

Gates is a minimal C-to-VHDL translator. It parses a subset of C and generates
clocked VHDL for its supported subset, with clock/reset ports, signals, and synchronous processes.

Features
--------

- Recursive-descent parser with full operator precedence (12 levels)
- Control flow: ``if/else``, ``while``, ``for``, ``break``, ``continue``
- Functions with return value propagation, structs with field access, arrays with indexing
- VHDL entity/architecture generation with clock/reset, signals, and synchronous processes
- Multi-level error diagnostics with colored output (error/warning/note across 5 categories)
- 75 unit, integration, and edge case tests (GoogleTest)

See :doc:`examples` for C-to-VHDL translation samples and :doc:`modules` for the
complete source code reference.

Debug Build
-----------

To enable verbose debug output:

.. code-block:: bash

   cmake -DDEBUG=ON ..
   make

Indices and tables
==================

* :ref:`genindex`
* :ref:`modindex`
* :ref:`search`
