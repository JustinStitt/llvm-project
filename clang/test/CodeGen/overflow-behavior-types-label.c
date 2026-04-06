// RUN: %clang_cc1 -triple x86_64-linux-gnu -fexperimental-overflow-behavior-types %s \
// RUN: -emit-llvm -o - | FileCheck %s

#define __trap_label(lbl) __attribute__((overflow_behavior(trap, lbl)))

// CHECK-LABEL: define {{.*}} @test_add_label
// Verify overflow check with branch to handler label
void test_add_label(int n) {
  int __trap_label(on_overflow) a = n;
  // CHECK: call { i32, i1 } @llvm.sadd.with.overflow.i32
  // CHECK: br i1 %{{.*}}, label %overflow.handler, label %nooverflow
  // CHECK: nooverflow:
  // CHECK: overflow.handler:
  // CHECK-NEXT: br label %on_overflow
  a = a + 1;
  return;
on_overflow:
  return;
}

// CHECK-LABEL: define {{.*}} @test_sub_label
void test_sub_label(int n) {
  int __trap_label(on_overflow) a = n;
  // CHECK: call { i32, i1 } @llvm.ssub.with.overflow.i32
  // CHECK: br i1 %{{.*}}, label %overflow.handler, label %nooverflow
  // CHECK: overflow.handler:
  // CHECK-NEXT: br label %on_overflow
  a = a - 1;
  return;
on_overflow:
  return;
}

// CHECK-LABEL: define {{.*}} @test_mul_label
void test_mul_label(int n) {
  int __trap_label(on_overflow) a = n;
  // CHECK: call { i32, i1 } @llvm.smul.with.overflow.i32
  // CHECK: br i1 %{{.*}}, label %overflow.handler, label %nooverflow
  // CHECK: overflow.handler:
  // CHECK-NEXT: br label %on_overflow
  a = a * 2;
  return;
on_overflow:
  return;
}

// CHECK-LABEL: define {{.*}} @test_no_label_still_traps
void test_no_label_still_traps(int n) {
  int __attribute__((overflow_behavior(trap))) a = n;
  // CHECK: call { i32, i1 } @llvm.sadd.with.overflow.i32
  // CHECK: call void @llvm.ubsantrap
  a = a + 1;
}

// Multiple labels: each variable branches to its own handler
// CHECK-LABEL: define {{.*}} @test_multi_label
void test_multi_label(int n) {
  int __trap_label(handler_a) a = n;
  int __trap_label(handler_b) b = n;
  // CHECK: call { i32, i1 } @llvm.sadd.with.overflow.i32
  // CHECK: br i1 %{{.*}}, label %overflow.handler, label %nooverflow
  a = a + 1;
  // Second add also uses sadd.with.overflow
  // CHECK: call { i32, i1 } @llvm.sadd.with.overflow.i32
  // CHECK: br i1 %{{.*}}, label %overflow.handler{{[0-9]+}}, label %nooverflow
  b = b + 1;
  // First handler branches to handler_a
  // CHECK: overflow.handler:
  // CHECK-NEXT: br label %handler_a
  // Second handler branches to handler_b
  // CHECK: overflow.handler{{[0-9]+}}:
  // CHECK-NEXT: br label %handler_b
  return;
handler_a:
  return;
handler_b:
  return;
}

// Loop: overflow inside loop body branches to handler
// CHECK-LABEL: define {{.*}} @test_loop_label
void test_loop_label(int n) {
  int __trap_label(loop_err) a = n;
  // CHECK: for.body:
  // CHECK: call { i32, i1 } @llvm.sadd.with.overflow.i32
  // CHECK: br i1 %{{.*}}, label %overflow.handler, label %nooverflow
  // CHECK: overflow.handler:
  // CHECK-NEXT: br label %loop_err
  for (int i = 0; i < 10; i++) {
    a = a + 1;
  }
  return;
loop_err:
  return;
}

// Switch: overflow inside switch branches to handler
// CHECK-LABEL: define {{.*}} @test_switch_label
void test_switch_label(int n, int sel) {
  int __trap_label(sw_err) a = n;
  switch (sel) {
  case 0:
    // CHECK: call { i32, i1 } @llvm.sadd.with.overflow.i32
    // CHECK: br i1 %{{.*}}, label %overflow.handler, label %nooverflow
    // CHECK: overflow.handler:
    // CHECK-NEXT: br label %sw_err
    a = a + 1;
    break;
  case 1:
    // CHECK: call { i32, i1 } @llvm.smul.with.overflow.i32
    // CHECK: br i1 %{{.*}}, label %overflow.handler{{[0-9]*}}, label %nooverflow
    // CHECK: overflow.handler{{[0-9]*}}:
    // CHECK-NEXT: br label %sw_err
    a = a * 2;
    break;
  }
  return;
sw_err:
  return;
}
