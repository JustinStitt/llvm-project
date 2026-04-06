// RUN: %clang_cc1 %s -fexperimental-overflow-behavior-types -verify -fsyntax-only -std=c11

// Handler label with trap: accepted
void test_label_with_trap() {
  int __attribute__((overflow_behavior(trap, handler))) a = 0;
  a = a + 1;
  return;
handler:
  return;
}

// Handler label with wrap: rejected
void test_label_with_wrap() {
  int __attribute__((overflow_behavior(wrap, handler))) a = 0; // expected-error {{overflow handler label can only be specified with 'trap' behavior}}
}

// Handler label not defined in function: error (via forward-reference check)
void test_label_undefined() {
  int __attribute__((overflow_behavior(trap, missing_label))) a = 0; // expected-error {{use of undeclared label 'missing_label'}}
  a = a + 1;
}

// Multiple args but third arg: error
typedef int __attribute__((overflow_behavior(trap, lbl, extra))) bad_args; // expected-error {{'overflow_behavior' attribute takes one argument}}

// No label, default trap behavior: accepted
void test_no_label() {
  int __attribute__((overflow_behavior(trap))) a = 0;
  a = a + 1;
}

// Typedef with handler label
typedef int __attribute__((overflow_behavior(trap, my_handler))) trap_with_label;

// Use typedef inside function with label defined: OK
void test_typedef_with_label() {
  trap_with_label x = 0;
  x = x + 1;
  return;
my_handler:
  return;
}

// Multiple labels in one function
void test_multi_label(int n) {
  int __attribute__((overflow_behavior(trap, handler_a))) a = n;
  int __attribute__((overflow_behavior(trap, handler_b))) b = n;
  a = a + 1;
  b = b + 1;
  return;
handler_a:
  return;
handler_b:
  return;
}

// Label inside a loop
void test_loop(int n) {
  int __attribute__((overflow_behavior(trap, loop_err))) a = n;
  for (int i = 0; i < 10; i++) {
    a = a + 1;
  }
  return;
loop_err:
  return;
}

// Label with braced scope
void test_braced_scope(int n) {
  int __attribute__((overflow_behavior(trap, scope_err))) a = n;
  {
    a = a + 1;
  }
  return;
scope_err:
  return;
}

// Label with switch statement
void test_switch(int n, int sel) {
  int __attribute__((overflow_behavior(trap, sw_err))) a = n;
  switch (sel) {
  case 0:
    a = a + 1;
    break;
  case 1:
    a = a * 2;
    break;
  }
  return;
sw_err:
  return;
}

// Multiple labels, one per scope pattern
void test_multi_scope(int n) {
  int __attribute__((overflow_behavior(trap, outer_err))) a = n;
  a = a + 1;
  {
    int __attribute__((overflow_behavior(trap, inner_err))) b = n;
    b = b * 2;
  }
  return;
outer_err:
inner_err:
  return;
}
