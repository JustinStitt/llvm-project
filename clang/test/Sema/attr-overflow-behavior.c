// RUN: %clang_cc1 %s -Winteger-overflow -Wno-unused-value -foverflow-behavior-types -verify -fsyntax-only

typedef int __attribute__((overflow_behavior)) bad_arg_count; // expected-error {{'overflow_behavior' attribute takes one argument}}
typedef int __attribute__((overflow_behavior(not_real))) bad_arg_spec; // expected-error {{'not_real' is not a valid argument to attribute 'overflow_behavior'}}
typedef int __attribute__((overflow_behavior("not_real"))) bad_arg_spec_str; // expected-error {{'not_real' is not a valid argument to attribute 'overflow_behavior'}}
typedef char* __attribute__((overflow_behavior("wrap"))) bad_type; // expected-warning {{'overflow_behavior' attribute cannot be applied to non-integer type char *}}

typedef int __attribute__((overflow_behavior(wrap))) ok_wrap; // OK
typedef long __attribute__((overflow_behavior(no_wrap))) ok_nowrap; // OK
typedef unsigned long __attribute__((overflow_behavior("wrap"))) str_ok_wrap; // OK
typedef char __attribute__((overflow_behavior("no_wrap"))) str_ok_nowrap; // OK

void foo() {
  (2147483647 + 100); // expected-warning {{overflow in expression; result is }}
  (ok_wrap)2147483647 + 100; // no warn
}
