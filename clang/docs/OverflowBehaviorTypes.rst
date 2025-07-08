=====================
OverflowBehaviorTypes
=====================

.. contents::
   :local:

Introduction
============

Clang provides an attribute that allows developers to have fine-grained control
over the overflow behavior of integer types. The ``overflow_behavior``
attribute can be used to specify how arithmetic operations on a given integer
type should behave upon overflow. This is particularly useful for projects that
need to balance performance and safety, allowing developers to enable or
disable overflow checks for specific types.

The attribute can be enabled using the compiler option
``-foverflow-behavior-types``.

The attribute syntax is as follows:

.. code-block:: c++

  __attribute__((overflow_behavior(behavior)))

Where ``behavior`` can be one of the following:

* ``wrap``: Specifies that arithmetic operations on the integer type should
  wrap on overflow. This is equivalent to the behavior of ``-fwrapv``, but it
  applies only to the attributed type and may be used with both signed and
  unsigned types. When this is enabled, UBSan's integer overflow checks are
  suppressed for the attributed type.

* ``no_wrap``: Specifies that arithmetic operations on the integer type should
  be checked for overflow. When using the ``signed-integer-overflow`` sanitizer
  or when using ``-ftrapv`` alongside a signed type, this is the default
  behavior. Using this, one may enforce overflow checks for a type even when
  ``-fwrapv`` is enabled globally.

This attribute can be applied to ``typedef`` declarations and to integer types directly.

Examples
========

Here is an example of how to use the ``overflow_behavior`` attribute with a ``typedef``:

.. code-block:: c++

  typedef int __attribute__((overflow_behavior(wrap))) wrapping_int;

  wrapping_int add_one(wrapping_int a) {
    return a + 1; // No overflow check for this operation.
  }

Here is an example of how to use the ``overflow_behavior`` attribute with a type directly:

.. code-block:: c++

  int mul_alot(int n) {
    int __attribute__((overflow_behavior(wrap))) a = n;
    return a * 1337; // No overflow check for this operation.
  }

Overflow behavior types are implicitly convertible to and from built-in
integral types.

Note that C++ overload set formation rules treat promotions to and from
overflow behavior types the same as normal integral promotions and conversions.

Interaction with Command-Line Flags and Sanitizer Special Case Lists
====================================================================

The ``overflow_behavior`` attribute interacts with the ``-ftrapv``,
``-fwrapv``, and the Sanitizer Special Case List (SSCL) by wholly overriding
these global flags. The following table summarizes the interactions:

.. list-table:: Overflow Behavior Precedence
   :widths: 20 20 20 20
   :header-rows: 1

   * - Behavior
     - -ftrapv
     - -fwrapv
     - SSCL
   * - ``overflow_behavior(wrap)``
     - No trap
     - Wraps
     - Overrides SSCL
   * - ``overflow_behavior(no_wrap)``
     - Traps
     - No wrap
     - Overrides SSCL

The ``overflow_behavior`` attribute can be used to override the behavior of
entries from :doc:`SanitizerSpecialCaseList`. This is useful for allowlisting
specific types into overflow instrumentation.

Promotion Rules
===============

The promotion rules for overflow behavior types are designed to preserve the
specified overflow behavior throughout an arithmetic expression. They differ
from standard C/C++ integer promotions but in a predictable way.

* **OBT and Standard Integer Type**: In an operation involving an overflow
  behavior type (OBT) and a standard integer type, the result will have the
  type of the OBT, including its overflow behavior, sign, and bit-width. The
  standard integer type is implicitly converted to match the OBT.

  .. code-block:: c++

    typedef char __attribute__((overflow_behavior(no_wrap))) no_wrap_char;
    // The result of this expression is no_wrap_char.
    no_wrap_char c;
    unsigned long ul;
    auto result = c + ul;

* **Two OBTs of the Same Kind**: When an operation involves two OBTs of the
  same kind (e.g., both ``wrap``), the result will have the larger of the two
  bit-widths. If the bit-widths are the same, an unsigned type is favored over
  a signed one.

  .. code-block:: c++

    typedef unsigned char __attribute__((overflow_behavior(wrap))) u8_wrap;
    typedef unsigned short __attribute__((overflow_behavior(wrap))) u16_wrap;
    // The result of this expression is u16_wrap.
    u8_wrap a;
    u16_wrap b;
    auto result = a + b;

* **Two OBTs of Different Kinds**: In an operation between a ``wrap`` and a
  ``no_wrap`` type, the result is the ``no_wrap`` type. It is recommended to
  avoid such operations, as Clang may emit a warning for such cases in the
  future. Regardless, the resulting type matches the bit-width, sign and
  behavior of the ``no_wrap`` type.

