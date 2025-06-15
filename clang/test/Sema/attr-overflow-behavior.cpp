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
    int x;
    TestOverload() = delete;
    TestOverload(int x) : x(x) {}

    void operator<<(int other) {
      this->x += other;
    }

    void operator<<(char other) {
      this->x += (int)other;
    }
};

void test_overload1() {
  wrap_int a = 4;
  TestOverload TO(10);
  TO << a;
}

int add_one(long a) { // expected-note {{candidate function}}
  return (a + 1);
}

int add_one(char a) { // expected-note {{candidate function}}
  return (a + 1);
}

void test_overload2(wrap_int a) {
  // to be clear, this is the same ambiguity expected when using a non-OBT int type.
  add_one(a); // expected-error {{call to 'add_one' is ambiguous}}
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
