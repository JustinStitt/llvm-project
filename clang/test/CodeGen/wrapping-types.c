// Test the _NoWrap and _Wrap type specifiers

// RUN: %clang_cc1 %s -emit-llvm -o - | FileCheck %s -check-prefix=NOSAN

// NOSAN-LABEL: @test_01
void test_01(_NoWrap unsigned char A, int B) {
  // _NoWrap type should trap when no sanitizer is provided

  // NOSAN: [[I:%.*]] = call { i8, i1 } @llvm.uadd.with.overflow.i8
  // NOSAN-NEXT: extractvalue { i8, i1 } [[I]], 0
  // NOSAN-NEXT: [[OF:%.*]] = extractvalue { i8, i1 } [[I]], 1
  // NOSAN-NEXT: [[XOR:%.*]] = xor i1 [[OF]], true
  // NOSAN-NEXT: br i1 [[XOR]], label %cont, label %trap
  (A + B);
}

// NOSAN-LABEL @test_02
void test_02(_NoWrap unsigned int A, char B) {
  // _NoWrap type should trap when no sanitizer is provided

  // NOSAN: [[I:%.*]] = call { i32, i1 } @llvm.uadd.with.overflow.i32
  // NOSAN-NEXT: extractvalue { i32, i1 } %2, 0
  // NOSAN-NEXT: [[OF:%.*]] = extractvalue { i32, i1 } %2, 1
  // NOSAN-NEXT: [[XOR:%.*]] = xor i1 [[OF]], true
  // NOSAN-NEXT: br i1 [[XOR]], label %cont, label %trap
  (B + A);
}
