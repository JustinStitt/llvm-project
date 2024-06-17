// RUN: %clang %s -O2 -fsanitize=signed-integer-overflow,unsigned-integer-overflow -fno-sanitize-overflow-idioms -S -emit-llvm -o - | FileCheck %s
// RUN: %clang %s -O2 -fsanitize=signed-integer-overflow,unsigned-integer-overflow -fno-sanitize-overflow-idioms -fwrapv -S -emit-llvm -o - | FileCheck --check-prefix=WRAPV %s
// CHECK-NOT: br{{.*}}overflow
// WRAPV-NOT: br{{.*}}overflow

extern int a, b, c;
extern unsigned u, v, w;

extern int some(void);

void basic_commutativity(void) {
  if (a + b < a)
    c = 9;
  if (a + b < b)
    c = 9;
  if (b + a < b)
    c = 9;
  if (b + a < a)
    c = 9;
  if (a > a + b)
    c = 9;
  if (a > b + a)
    c = 9;
  if (b > a + b)
    c = 9;
  if (b > b + a)
    c = 9;
}

void arguments_and_commutativity(unsigned V1, unsigned V2) {
  if (V1 + V2 < V1)
    c = 9;
  if (V1 + V2 < V2)
    c = 9;
  if (V2 + V1 < V2)
    c = 9;
  if (V2 + V1 < V1)
    c = 9;
  if (V1 > V1 + V2)
    c = 9;
  if (V1 > V2 + V1)
    c = 9;
  if (V2 > V1 + V2)
    c = 9;
  if (V2 > V2 + V1)
    c = 9;
}

void pointers(unsigned *P1, unsigned *P2, unsigned V1) {
  if (*P1 + *P2 < *P1)
    c = 9;
  if (*P1 + V1 < V1)
    c = 9;
  if (V1 + *P2 < *P2)
    c = 9;
}

struct OtherStruct {
  int foo, bar;
};

struct MyStruct {
  int base, offset;
  struct OtherStruct os;
};

void structs(void) {
  struct MyStruct ms;
  if (ms.base + ms.offset > ms.base)
    c = 9;;
}

void nestedstructs(void) {
  struct MyStruct ms;
  if (ms.os.foo + ms.os.bar < ms.os.foo)
    c = 9;;
}
