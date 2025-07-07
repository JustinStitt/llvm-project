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

// C++ stuff expects no warns
typedef int __attribute__((overflow_behavior(wrap))) wrap_int;

template <typename T>
T bar(T a) {
  return 1UL;
}

void f() {
  wrap_int a = 4;
  bar(a);
}

class TestOverload {
  public:
    void operator<<(int other); // expected-note {{candidate function}}
    void operator<<(char other); // expected-note {{candidate function}}
};

void test_overload1() {
  wrap_int a = 4;
  TestOverload TO;
  TO << a; // expected-error {{use of overloaded operator '<<' is ambiguous}}
}

// expected-note@+1 {{candidate function}}
int add_one(long a) { // expected-note {{candidate function}}
  return (a + 1);
}

// expected-note@+1 {{candidate function}}
int add_one(char a) { // expected-note {{candidate function}}
  return (a + 1);
}

// expected-note@+1 {{candidate function}}
int add_one(int a) { // expected-note {{candidate function}}
  return (a + 1);
}

void test_overload2(wrap_int a) {
  // to be clear, this is the same ambiguity expected when using a non-OBT int type.
  add_one(a); // expected-error {{call to 'add_one' is ambiguous}}
  long __attribute__((overflow_behavior(no_wrap))) b; // don't consider underlying type an exact match.
  add_one(b); // expected-error {{call to 'add_one' is ambiguous}}
}

#define __no_wrap __attribute__((overflow_behavior(no_wrap)))
void func(__no_wrap int i);
void func(int i); // Overload, not invalid redeclaration

// TODO: make this diagnostic message more descriptive
template <typename Ty>
void func2(__no_wrap Ty i) {} // expected-warning {{'overflow_behavior' attribute cannot be applied to non-integer type Ty}}

template <typename Ty>
struct S {};

template <>
struct S<__no_wrap int> {};

template <>
struct S<int> {};

void ptr(int a) {
  int __no_wrap *p = &a; // expected-error {{cannot initialize a variable of type '__no_wrap int *'}} {{.*}} {{with an rvalue of type 'int *'}}
}

void ptr2(__no_wrap int a) {
  int *p = &a; // expected-error {{cannot initialize a variable of type 'int *'}} {{.*}} {{with an rvalue of type '__no_wrap int *'}}
}

void overloadme(__no_wrap int a); // expected-note {{candidate function}}
void overloadme(short a); // expected-note {{candidate function}}

void test_overload_ambiguity() {
  int a;
  overloadme(a); // expected-error {{call to 'overloadme' is ambiguous}}
}

