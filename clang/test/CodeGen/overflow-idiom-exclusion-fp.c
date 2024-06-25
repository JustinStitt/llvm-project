// Check for potential false positives from patterns that _almost_ match classic overflow idioms
// RUN: %clang %s -O2 -fsanitize=signed-integer-overflow,unsigned-integer-overflow -fno-sanitize-overflow-idioms -S -emit-llvm -o - | FileCheck %s
// RUN: %clang %s -O2 -fsanitize=signed-integer-overflow,unsigned-integer-overflow -fno-sanitize-overflow-idioms -fwrapv -S -emit-llvm -o - | FileCheck %s

extern unsigned a, b, c;
extern int u, v, w;

extern void some(void);

// Make sure all these still have handler paths, we shouldn't be excluding
// instrumentation of any "near" idioms.
void close_but_not_quite(void) {
  // CHECK: br i1{{.*}}handler.
  if (a + b > a)
    c = 9;

  // CHECK: br i1{{.*}}handler.
  if (a - b < a)
    c = 9;

  // CHECK: br i1{{.*}}handler.
  if (a + b < a)
    c = 9;

  // CHECK: br i1{{.*}}handler.
  if (a + b + 1 < a)
    c = 9;

  // CHECK: br i1{{.*}}handler.
  if (a + b < a + 1)
    c = 9;

  // CHECK: br i1{{.*}}handler.
  if (b >= a + b)
    c = 9;

  // CHECK: br i1{{.*}}handler.
  if (a + a < a)
    c = 9;

  // CHECK: br i1{{.*}}handler.
  if (a + b == a)
    c = 9;

  // CHECK: br i1{{.*}}handler
  if (u + v < u) /* matches overflow idiom, but is signed */
    c = 9;

  // CHECK: br i1{{.*}}handler
  // Although this can never actually overflow we are still checking that the
  // sanitizer instruments it.
  while (--a)
    some();
}
