// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -foverflow-behavior-types -verify -fsyntax-only -std=c++11

#define __wrap __attribute__((overflow_behavior(wrap)))
#define __nowrap __attribute__((overflow_behavior(no_wrap)))

struct S {
  int i;
} __attribute__((overflow_behavior(wrap))); // expected-warning {{'overflow_behavior' attribute only applies to variables, typedefs, and non-static data members}}

void f(void) __attribute__((overflow_behavior(wrap))); // expected-warning {{'overflow_behavior' attribute cannot be applied to non-integer type 'void (void)'; attribute ignored}}

typedef float __attribute__((overflow_behavior(wrap))) wrap_float; // expected-warning {{'overflow_behavior' attribute cannot be applied to non-integer type 'float'; attribute ignored}}

void pointer_compatibility_test(int* i_ptr) {
  __nowrap int* nowrap_ptr;

  // static_cast should fail.
  nowrap_ptr = static_cast<__nowrap int*>(i_ptr); // expected-error {{static_cast from 'int *' to '__no_wrap int *' (aka 'int *') is not allowed}}

  // reinterpret_cast should succeed.
  nowrap_ptr = reinterpret_cast<__nowrap int*>(i_ptr);
  (void)nowrap_ptr;
}
