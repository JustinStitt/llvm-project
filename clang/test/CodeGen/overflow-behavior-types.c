// RUN: %clang_cc1 -triple x86_64-linux-gnu %s -fsanitize=signed-integer-overflow,unsigned-integer-overflow -emit-llvm -o - | FileCheck %s --implicit-check-not='with.overflow'

#define __wrap __attribute__((overflow_behavior("wrap")))
#define __nowrap __attribute__((overflow_behavior("no_wrap")))

void test1(int __wrap a) {
  (a + 1);
}
