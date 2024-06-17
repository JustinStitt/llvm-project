; RUN: opt -S %s --passes='idiomexclusions' | FileCheck %s
; None of these tests should have any branches to overflow handlers
; CHECK-NOT: br{{.*}}overflow

%struct.MyStruct = type { i32, i32, %struct.OtherStruct }
%struct.OtherStruct = type { i32, i32 }

@a = external global i32, align 4
@b = external global i32, align 4
@.src = private unnamed_addr constant [15 x i8] c"common-tests.c\00", align 1
@0 = private unnamed_addr constant { i16, i16, [6 x i8] } { i16 0, i16 11, [6 x i8] c"'int'\00" }
@1 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 7, i32 9 }, ptr @0 }
@c = external global i32, align 4
@2 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 9, i32 9 }, ptr @0 }
@3 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 11, i32 9 }, ptr @0 }
@4 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 13, i32 9 }, ptr @0 }
@5 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 15, i32 13 }, ptr @0 }
@6 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 17, i32 13 }, ptr @0 }
@7 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 19, i32 13 }, ptr @0 }
@8 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 21, i32 13 }, ptr @0 }
@9 = private unnamed_addr constant { i16, i16, [15 x i8] } { i16 0, i16 10, [15 x i8] c"'unsigned int'\00" }
@10 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 26, i32 10 }, ptr @9 }
@11 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 28, i32 10 }, ptr @9 }
@12 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 30, i32 10 }, ptr @9 }
@13 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 32, i32 10 }, ptr @9 }
@14 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 34, i32 15 }, ptr @9 }
@15 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 36, i32 15 }, ptr @9 }
@16 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 38, i32 15 }, ptr @9 }
@17 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 40, i32 15 }, ptr @9 }
@18 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 46, i32 11 }, ptr @9 }
@19 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 48, i32 11 }, ptr @9 }
@20 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 51, i32 10 }, ptr @9 }
@21 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 66, i32 15 }, ptr @0 }
@22 = private unnamed_addr global { { ptr, i32, i32 }, ptr } { { ptr, i32, i32 } { ptr @.src, i32 72, i32 17 }, ptr @0 }

; Function Attrs: nounwind uwtable
define dso_local void @basic_commutativity() #0 {
entry:
  %0 = load i32, ptr @a, align 4, !tbaa !5
  %1 = load i32, ptr @b, align 4, !tbaa !5
  %2 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %0, i32 %1), !nosanitize !9
  %3 = extractvalue { i32, i1 } %2, 0, !nosanitize !9
  %4 = extractvalue { i32, i1 } %2, 1, !nosanitize !9
  %5 = xor i1 %4, true, !nosanitize !9
  br i1 %5, label %cont, label %handler.add_overflow, !prof !10, !nosanitize !9

handler.add_overflow:                             ; preds = %entry
  %6 = zext i32 %0 to i64, !nosanitize !9
  %7 = zext i32 %1 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @1, i64 %6, i64 %7) #4, !nosanitize !9
  br label %cont, !nosanitize !9

cont:                                             ; preds = %handler.add_overflow, %entry
  %8 = load i32, ptr @a, align 4, !tbaa !5
  %cmp = icmp slt i32 %3, %8
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %cont
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end

if.end:                                           ; preds = %if.then, %cont
  %9 = load i32, ptr @a, align 4, !tbaa !5
  %10 = load i32, ptr @b, align 4, !tbaa !5
  %11 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %9, i32 %10), !nosanitize !9
  %12 = extractvalue { i32, i1 } %11, 0, !nosanitize !9
  %13 = extractvalue { i32, i1 } %11, 1, !nosanitize !9
  %14 = xor i1 %13, true, !nosanitize !9
  br i1 %14, label %cont2, label %handler.add_overflow1, !prof !10, !nosanitize !9

handler.add_overflow1:                            ; preds = %if.end
  %15 = zext i32 %9 to i64, !nosanitize !9
  %16 = zext i32 %10 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @2, i64 %15, i64 %16) #4, !nosanitize !9
  br label %cont2, !nosanitize !9

cont2:                                            ; preds = %handler.add_overflow1, %if.end
  %17 = load i32, ptr @b, align 4, !tbaa !5
  %cmp3 = icmp slt i32 %12, %17
  br i1 %cmp3, label %if.then4, label %if.end5

if.then4:                                         ; preds = %cont2
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end5

if.end5:                                          ; preds = %if.then4, %cont2
  %18 = load i32, ptr @b, align 4, !tbaa !5
  %19 = load i32, ptr @a, align 4, !tbaa !5
  %20 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %18, i32 %19), !nosanitize !9
  %21 = extractvalue { i32, i1 } %20, 0, !nosanitize !9
  %22 = extractvalue { i32, i1 } %20, 1, !nosanitize !9
  %23 = xor i1 %22, true, !nosanitize !9
  br i1 %23, label %cont7, label %handler.add_overflow6, !prof !10, !nosanitize !9

handler.add_overflow6:                            ; preds = %if.end5
  %24 = zext i32 %18 to i64, !nosanitize !9
  %25 = zext i32 %19 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @3, i64 %24, i64 %25) #4, !nosanitize !9
  br label %cont7, !nosanitize !9

cont7:                                            ; preds = %handler.add_overflow6, %if.end5
  %26 = load i32, ptr @b, align 4, !tbaa !5
  %cmp8 = icmp slt i32 %21, %26
  br i1 %cmp8, label %if.then9, label %if.end10

if.then9:                                         ; preds = %cont7
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end10

if.end10:                                         ; preds = %if.then9, %cont7
  %27 = load i32, ptr @b, align 4, !tbaa !5
  %28 = load i32, ptr @a, align 4, !tbaa !5
  %29 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %27, i32 %28), !nosanitize !9
  %30 = extractvalue { i32, i1 } %29, 0, !nosanitize !9
  %31 = extractvalue { i32, i1 } %29, 1, !nosanitize !9
  %32 = xor i1 %31, true, !nosanitize !9
  br i1 %32, label %cont12, label %handler.add_overflow11, !prof !10, !nosanitize !9

handler.add_overflow11:                           ; preds = %if.end10
  %33 = zext i32 %27 to i64, !nosanitize !9
  %34 = zext i32 %28 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @4, i64 %33, i64 %34) #4, !nosanitize !9
  br label %cont12, !nosanitize !9

cont12:                                           ; preds = %handler.add_overflow11, %if.end10
  %35 = load i32, ptr @a, align 4, !tbaa !5
  %cmp13 = icmp slt i32 %30, %35
  br i1 %cmp13, label %if.then14, label %if.end15

if.then14:                                        ; preds = %cont12
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end15

if.end15:                                         ; preds = %if.then14, %cont12
  %36 = load i32, ptr @a, align 4, !tbaa !5
  %37 = load i32, ptr @a, align 4, !tbaa !5
  %38 = load i32, ptr @b, align 4, !tbaa !5
  %39 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %37, i32 %38), !nosanitize !9
  %40 = extractvalue { i32, i1 } %39, 0, !nosanitize !9
  %41 = extractvalue { i32, i1 } %39, 1, !nosanitize !9
  %42 = xor i1 %41, true, !nosanitize !9
  br i1 %42, label %cont17, label %handler.add_overflow16, !prof !10, !nosanitize !9

handler.add_overflow16:                           ; preds = %if.end15
  %43 = zext i32 %37 to i64, !nosanitize !9
  %44 = zext i32 %38 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @5, i64 %43, i64 %44) #4, !nosanitize !9
  br label %cont17, !nosanitize !9

cont17:                                           ; preds = %handler.add_overflow16, %if.end15
  %cmp18 = icmp sgt i32 %36, %40
  br i1 %cmp18, label %if.then19, label %if.end20

if.then19:                                        ; preds = %cont17
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end20

if.end20:                                         ; preds = %if.then19, %cont17
  %45 = load i32, ptr @a, align 4, !tbaa !5
  %46 = load i32, ptr @b, align 4, !tbaa !5
  %47 = load i32, ptr @a, align 4, !tbaa !5
  %48 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %46, i32 %47), !nosanitize !9
  %49 = extractvalue { i32, i1 } %48, 0, !nosanitize !9
  %50 = extractvalue { i32, i1 } %48, 1, !nosanitize !9
  %51 = xor i1 %50, true, !nosanitize !9
  br i1 %51, label %cont22, label %handler.add_overflow21, !prof !10, !nosanitize !9

handler.add_overflow21:                           ; preds = %if.end20
  %52 = zext i32 %46 to i64, !nosanitize !9
  %53 = zext i32 %47 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @6, i64 %52, i64 %53) #4, !nosanitize !9
  br label %cont22, !nosanitize !9

cont22:                                           ; preds = %handler.add_overflow21, %if.end20
  %cmp23 = icmp sgt i32 %45, %49
  br i1 %cmp23, label %if.then24, label %if.end25

if.then24:                                        ; preds = %cont22
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end25

if.end25:                                         ; preds = %if.then24, %cont22
  %54 = load i32, ptr @b, align 4, !tbaa !5
  %55 = load i32, ptr @a, align 4, !tbaa !5
  %56 = load i32, ptr @b, align 4, !tbaa !5
  %57 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %55, i32 %56), !nosanitize !9
  %58 = extractvalue { i32, i1 } %57, 0, !nosanitize !9
  %59 = extractvalue { i32, i1 } %57, 1, !nosanitize !9
  %60 = xor i1 %59, true, !nosanitize !9
  br i1 %60, label %cont27, label %handler.add_overflow26, !prof !10, !nosanitize !9

handler.add_overflow26:                           ; preds = %if.end25
  %61 = zext i32 %55 to i64, !nosanitize !9
  %62 = zext i32 %56 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @7, i64 %61, i64 %62) #4, !nosanitize !9
  br label %cont27, !nosanitize !9

cont27:                                           ; preds = %handler.add_overflow26, %if.end25
  %cmp28 = icmp sgt i32 %54, %58
  br i1 %cmp28, label %if.then29, label %if.end30

if.then29:                                        ; preds = %cont27
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end30

if.end30:                                         ; preds = %if.then29, %cont27
  %63 = load i32, ptr @b, align 4, !tbaa !5
  %64 = load i32, ptr @b, align 4, !tbaa !5
  %65 = load i32, ptr @a, align 4, !tbaa !5
  %66 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %64, i32 %65), !nosanitize !9
  %67 = extractvalue { i32, i1 } %66, 0, !nosanitize !9
  %68 = extractvalue { i32, i1 } %66, 1, !nosanitize !9
  %69 = xor i1 %68, true, !nosanitize !9
  br i1 %69, label %cont32, label %handler.add_overflow31, !prof !10, !nosanitize !9

handler.add_overflow31:                           ; preds = %if.end30
  %70 = zext i32 %64 to i64, !nosanitize !9
  %71 = zext i32 %65 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @8, i64 %70, i64 %71) #4, !nosanitize !9
  br label %cont32, !nosanitize !9

cont32:                                           ; preds = %handler.add_overflow31, %if.end30
  %cmp33 = icmp sgt i32 %63, %67
  br i1 %cmp33, label %if.then34, label %if.end35

if.then34:                                        ; preds = %cont32
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end35

if.end35:                                         ; preds = %if.then34, %cont32
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i32, i1 } @llvm.sadd.with.overflow.i32(i32, i32) #1

; Function Attrs: uwtable
declare void @__ubsan_handle_add_overflow(ptr, i64, i64) #2

; Function Attrs: nounwind uwtable
define dso_local void @arguments_and_commutativity(i32 noundef %V1, i32 noundef %V2) #0 {
entry:
  %V1.addr = alloca i32, align 4
  %V2.addr = alloca i32, align 4
  store i32 %V1, ptr %V1.addr, align 4, !tbaa !5
  store i32 %V2, ptr %V2.addr, align 4, !tbaa !5
  %0 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %1 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %2 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %0, i32 %1), !nosanitize !9
  %3 = extractvalue { i32, i1 } %2, 0, !nosanitize !9
  %4 = extractvalue { i32, i1 } %2, 1, !nosanitize !9
  %5 = xor i1 %4, true, !nosanitize !9
  br i1 %5, label %cont, label %handler.add_overflow, !prof !10, !nosanitize !9

handler.add_overflow:                             ; preds = %entry
  %6 = zext i32 %0 to i64, !nosanitize !9
  %7 = zext i32 %1 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @10, i64 %6, i64 %7) #4, !nosanitize !9
  br label %cont, !nosanitize !9

cont:                                             ; preds = %handler.add_overflow, %entry
  %8 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %cmp = icmp ult i32 %3, %8
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %cont
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end

if.end:                                           ; preds = %if.then, %cont
  %9 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %10 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %11 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %9, i32 %10), !nosanitize !9
  %12 = extractvalue { i32, i1 } %11, 0, !nosanitize !9
  %13 = extractvalue { i32, i1 } %11, 1, !nosanitize !9
  %14 = xor i1 %13, true, !nosanitize !9
  br i1 %14, label %cont2, label %handler.add_overflow1, !prof !10, !nosanitize !9

handler.add_overflow1:                            ; preds = %if.end
  %15 = zext i32 %9 to i64, !nosanitize !9
  %16 = zext i32 %10 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @11, i64 %15, i64 %16) #4, !nosanitize !9
  br label %cont2, !nosanitize !9

cont2:                                            ; preds = %handler.add_overflow1, %if.end
  %17 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %cmp3 = icmp ult i32 %12, %17
  br i1 %cmp3, label %if.then4, label %if.end5

if.then4:                                         ; preds = %cont2
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end5

if.end5:                                          ; preds = %if.then4, %cont2
  %18 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %19 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %20 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %18, i32 %19), !nosanitize !9
  %21 = extractvalue { i32, i1 } %20, 0, !nosanitize !9
  %22 = extractvalue { i32, i1 } %20, 1, !nosanitize !9
  %23 = xor i1 %22, true, !nosanitize !9
  br i1 %23, label %cont7, label %handler.add_overflow6, !prof !10, !nosanitize !9

handler.add_overflow6:                            ; preds = %if.end5
  %24 = zext i32 %18 to i64, !nosanitize !9
  %25 = zext i32 %19 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @12, i64 %24, i64 %25) #4, !nosanitize !9
  br label %cont7, !nosanitize !9

cont7:                                            ; preds = %handler.add_overflow6, %if.end5
  %26 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %cmp8 = icmp ult i32 %21, %26
  br i1 %cmp8, label %if.then9, label %if.end10

if.then9:                                         ; preds = %cont7
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end10

if.end10:                                         ; preds = %if.then9, %cont7
  %27 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %28 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %29 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %27, i32 %28), !nosanitize !9
  %30 = extractvalue { i32, i1 } %29, 0, !nosanitize !9
  %31 = extractvalue { i32, i1 } %29, 1, !nosanitize !9
  %32 = xor i1 %31, true, !nosanitize !9
  br i1 %32, label %cont12, label %handler.add_overflow11, !prof !10, !nosanitize !9

handler.add_overflow11:                           ; preds = %if.end10
  %33 = zext i32 %27 to i64, !nosanitize !9
  %34 = zext i32 %28 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @13, i64 %33, i64 %34) #4, !nosanitize !9
  br label %cont12, !nosanitize !9

cont12:                                           ; preds = %handler.add_overflow11, %if.end10
  %35 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %cmp13 = icmp ult i32 %30, %35
  br i1 %cmp13, label %if.then14, label %if.end15

if.then14:                                        ; preds = %cont12
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end15

if.end15:                                         ; preds = %if.then14, %cont12
  %36 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %37 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %38 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %39 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %37, i32 %38), !nosanitize !9
  %40 = extractvalue { i32, i1 } %39, 0, !nosanitize !9
  %41 = extractvalue { i32, i1 } %39, 1, !nosanitize !9
  %42 = xor i1 %41, true, !nosanitize !9
  br i1 %42, label %cont17, label %handler.add_overflow16, !prof !10, !nosanitize !9

handler.add_overflow16:                           ; preds = %if.end15
  %43 = zext i32 %37 to i64, !nosanitize !9
  %44 = zext i32 %38 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @14, i64 %43, i64 %44) #4, !nosanitize !9
  br label %cont17, !nosanitize !9

cont17:                                           ; preds = %handler.add_overflow16, %if.end15
  %cmp18 = icmp ugt i32 %36, %40
  br i1 %cmp18, label %if.then19, label %if.end20

if.then19:                                        ; preds = %cont17
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end20

if.end20:                                         ; preds = %if.then19, %cont17
  %45 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %46 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %47 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %48 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %46, i32 %47), !nosanitize !9
  %49 = extractvalue { i32, i1 } %48, 0, !nosanitize !9
  %50 = extractvalue { i32, i1 } %48, 1, !nosanitize !9
  %51 = xor i1 %50, true, !nosanitize !9
  br i1 %51, label %cont22, label %handler.add_overflow21, !prof !10, !nosanitize !9

handler.add_overflow21:                           ; preds = %if.end20
  %52 = zext i32 %46 to i64, !nosanitize !9
  %53 = zext i32 %47 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @15, i64 %52, i64 %53) #4, !nosanitize !9
  br label %cont22, !nosanitize !9

cont22:                                           ; preds = %handler.add_overflow21, %if.end20
  %cmp23 = icmp ugt i32 %45, %49
  br i1 %cmp23, label %if.then24, label %if.end25

if.then24:                                        ; preds = %cont22
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end25

if.end25:                                         ; preds = %if.then24, %cont22
  %54 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %55 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %56 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %57 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %55, i32 %56), !nosanitize !9
  %58 = extractvalue { i32, i1 } %57, 0, !nosanitize !9
  %59 = extractvalue { i32, i1 } %57, 1, !nosanitize !9
  %60 = xor i1 %59, true, !nosanitize !9
  br i1 %60, label %cont27, label %handler.add_overflow26, !prof !10, !nosanitize !9

handler.add_overflow26:                           ; preds = %if.end25
  %61 = zext i32 %55 to i64, !nosanitize !9
  %62 = zext i32 %56 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @16, i64 %61, i64 %62) #4, !nosanitize !9
  br label %cont27, !nosanitize !9

cont27:                                           ; preds = %handler.add_overflow26, %if.end25
  %cmp28 = icmp ugt i32 %54, %58
  br i1 %cmp28, label %if.then29, label %if.end30

if.then29:                                        ; preds = %cont27
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end30

if.end30:                                         ; preds = %if.then29, %cont27
  %63 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %64 = load i32, ptr %V2.addr, align 4, !tbaa !5
  %65 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %66 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %64, i32 %65), !nosanitize !9
  %67 = extractvalue { i32, i1 } %66, 0, !nosanitize !9
  %68 = extractvalue { i32, i1 } %66, 1, !nosanitize !9
  %69 = xor i1 %68, true, !nosanitize !9
  br i1 %69, label %cont32, label %handler.add_overflow31, !prof !10, !nosanitize !9

handler.add_overflow31:                           ; preds = %if.end30
  %70 = zext i32 %64 to i64, !nosanitize !9
  %71 = zext i32 %65 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @17, i64 %70, i64 %71) #4, !nosanitize !9
  br label %cont32, !nosanitize !9

cont32:                                           ; preds = %handler.add_overflow31, %if.end30
  %cmp33 = icmp ugt i32 %63, %67
  br i1 %cmp33, label %if.then34, label %if.end35

if.then34:                                        ; preds = %cont32
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end35

if.end35:                                         ; preds = %if.then34, %cont32
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind speculatable willreturn memory(none)
declare { i32, i1 } @llvm.uadd.with.overflow.i32(i32, i32) #1

; Function Attrs: nounwind uwtable
define dso_local void @pointers(ptr noundef %P1, ptr noundef %P2, i32 noundef %V1) #0 {
entry:
  %P1.addr = alloca ptr, align 8
  %P2.addr = alloca ptr, align 8
  %V1.addr = alloca i32, align 4
  store ptr %P1, ptr %P1.addr, align 8, !tbaa !11
  store ptr %P2, ptr %P2.addr, align 8, !tbaa !11
  store i32 %V1, ptr %V1.addr, align 4, !tbaa !5
  %0 = load ptr, ptr %P1.addr, align 8, !tbaa !11
  %1 = load i32, ptr %0, align 4, !tbaa !5
  %2 = load ptr, ptr %P2.addr, align 8, !tbaa !11
  %3 = load i32, ptr %2, align 4, !tbaa !5
  %4 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %1, i32 %3), !nosanitize !9
  %5 = extractvalue { i32, i1 } %4, 0, !nosanitize !9
  %6 = extractvalue { i32, i1 } %4, 1, !nosanitize !9
  %7 = xor i1 %6, true, !nosanitize !9
  br i1 %7, label %cont, label %handler.add_overflow, !prof !10, !nosanitize !9

handler.add_overflow:                             ; preds = %entry
  %8 = zext i32 %1 to i64, !nosanitize !9
  %9 = zext i32 %3 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @18, i64 %8, i64 %9) #4, !nosanitize !9
  br label %cont, !nosanitize !9

cont:                                             ; preds = %handler.add_overflow, %entry
  %10 = load ptr, ptr %P1.addr, align 8, !tbaa !11
  %11 = load i32, ptr %10, align 4, !tbaa !5
  %cmp = icmp ult i32 %5, %11
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %cont
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end

if.end:                                           ; preds = %if.then, %cont
  %12 = load ptr, ptr %P1.addr, align 8, !tbaa !11
  %13 = load i32, ptr %12, align 4, !tbaa !5
  %14 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %15 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %13, i32 %14), !nosanitize !9
  %16 = extractvalue { i32, i1 } %15, 0, !nosanitize !9
  %17 = extractvalue { i32, i1 } %15, 1, !nosanitize !9
  %18 = xor i1 %17, true, !nosanitize !9
  br i1 %18, label %cont2, label %handler.add_overflow1, !prof !10, !nosanitize !9

handler.add_overflow1:                            ; preds = %if.end
  %19 = zext i32 %13 to i64, !nosanitize !9
  %20 = zext i32 %14 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @19, i64 %19, i64 %20) #4, !nosanitize !9
  br label %cont2, !nosanitize !9

cont2:                                            ; preds = %handler.add_overflow1, %if.end
  %21 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %cmp3 = icmp ult i32 %16, %21
  br i1 %cmp3, label %if.then4, label %if.end5

if.then4:                                         ; preds = %cont2
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end5

if.end5:                                          ; preds = %if.then4, %cont2
  %22 = load i32, ptr %V1.addr, align 4, !tbaa !5
  %23 = load ptr, ptr %P2.addr, align 8, !tbaa !11
  %24 = load i32, ptr %23, align 4, !tbaa !5
  %25 = call { i32, i1 } @llvm.uadd.with.overflow.i32(i32 %22, i32 %24), !nosanitize !9
  %26 = extractvalue { i32, i1 } %25, 0, !nosanitize !9
  %27 = extractvalue { i32, i1 } %25, 1, !nosanitize !9
  %28 = xor i1 %27, true, !nosanitize !9
  br i1 %28, label %cont7, label %handler.add_overflow6, !prof !10, !nosanitize !9

handler.add_overflow6:                            ; preds = %if.end5
  %29 = zext i32 %22 to i64, !nosanitize !9
  %30 = zext i32 %24 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @20, i64 %29, i64 %30) #4, !nosanitize !9
  br label %cont7, !nosanitize !9

cont7:                                            ; preds = %handler.add_overflow6, %if.end5
  %31 = load ptr, ptr %P2.addr, align 8, !tbaa !11
  %32 = load i32, ptr %31, align 4, !tbaa !5
  %cmp8 = icmp ult i32 %26, %32
  br i1 %cmp8, label %if.then9, label %if.end10

if.then9:                                         ; preds = %cont7
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end10

if.end10:                                         ; preds = %if.then9, %cont7
  ret void
}

; Function Attrs: nounwind uwtable
define dso_local void @structs() #0 {
entry:
  %ms = alloca %struct.MyStruct, align 4
  call void @llvm.lifetime.start.p0(i64 16, ptr %ms) #4
  %base = getelementptr inbounds %struct.MyStruct, ptr %ms, i32 0, i32 0
  %0 = load i32, ptr %base, align 4, !tbaa !13
  %offset = getelementptr inbounds %struct.MyStruct, ptr %ms, i32 0, i32 1
  %1 = load i32, ptr %offset, align 4, !tbaa !16
  %2 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %0, i32 %1), !nosanitize !9
  %3 = extractvalue { i32, i1 } %2, 0, !nosanitize !9
  %4 = extractvalue { i32, i1 } %2, 1, !nosanitize !9
  %5 = xor i1 %4, true, !nosanitize !9
  br i1 %5, label %cont, label %handler.add_overflow, !prof !10, !nosanitize !9

handler.add_overflow:                             ; preds = %entry
  %6 = zext i32 %0 to i64, !nosanitize !9
  %7 = zext i32 %1 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @21, i64 %6, i64 %7) #4, !nosanitize !9
  br label %cont, !nosanitize !9

cont:                                             ; preds = %handler.add_overflow, %entry
  %base1 = getelementptr inbounds %struct.MyStruct, ptr %ms, i32 0, i32 0
  %8 = load i32, ptr %base1, align 4, !tbaa !13
  %cmp = icmp slt i32 %3, %8
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %cont
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end

if.end:                                           ; preds = %if.then, %cont
  call void @llvm.lifetime.end.p0(i64 16, ptr %ms) #4
  ret void
}

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.start.p0(i64 immarg, ptr nocapture) #3

; Function Attrs: nocallback nofree nosync nounwind willreturn memory(argmem: readwrite)
declare void @llvm.lifetime.end.p0(i64 immarg, ptr nocapture) #3

; Function Attrs: nounwind uwtable
define dso_local void @nestedstrucs() #0 {
entry:
  %ms = alloca %struct.MyStruct, align 4
  call void @llvm.lifetime.start.p0(i64 16, ptr %ms) #4
  %os = getelementptr inbounds %struct.MyStruct, ptr %ms, i32 0, i32 2
  %foo = getelementptr inbounds %struct.OtherStruct, ptr %os, i32 0, i32 0
  %0 = load i32, ptr %foo, align 4, !tbaa !17
  %os1 = getelementptr inbounds %struct.MyStruct, ptr %ms, i32 0, i32 2
  %bar = getelementptr inbounds %struct.OtherStruct, ptr %os1, i32 0, i32 1
  %1 = load i32, ptr %bar, align 4, !tbaa !18
  %2 = call { i32, i1 } @llvm.sadd.with.overflow.i32(i32 %0, i32 %1), !nosanitize !9
  %3 = extractvalue { i32, i1 } %2, 0, !nosanitize !9
  %4 = extractvalue { i32, i1 } %2, 1, !nosanitize !9
  %5 = xor i1 %4, true, !nosanitize !9
  br i1 %5, label %cont, label %handler.add_overflow, !prof !10, !nosanitize !9

handler.add_overflow:                             ; preds = %entry
  %6 = zext i32 %0 to i64, !nosanitize !9
  %7 = zext i32 %1 to i64, !nosanitize !9
  call void @__ubsan_handle_add_overflow(ptr @22, i64 %6, i64 %7) #4, !nosanitize !9
  br label %cont, !nosanitize !9

cont:                                             ; preds = %handler.add_overflow, %entry
  %os2 = getelementptr inbounds %struct.MyStruct, ptr %ms, i32 0, i32 2
  %foo3 = getelementptr inbounds %struct.OtherStruct, ptr %os2, i32 0, i32 0
  %8 = load i32, ptr %foo3, align 4, !tbaa !17
  %cmp = icmp slt i32 %3, %8
  br i1 %cmp, label %if.then, label %if.end

if.then:                                          ; preds = %cont
  store i32 9, ptr @c, align 4, !tbaa !5
  br label %if.end

if.end:                                           ; preds = %if.then, %cont
  call void @llvm.lifetime.end.p0(i64 16, ptr %ms) #4
  ret void
}

attributes #0 = { nounwind uwtable "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { nocallback nofree nosync nounwind speculatable willreturn memory(none) }
attributes #2 = { uwtable }
attributes #3 = { nocallback nofree nosync nounwind willreturn memory(argmem: readwrite) }
attributes #4 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3}
!llvm.ident = !{!4}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{!"clang version"}
!5 = !{!6, !6, i64 0}
!6 = !{!"int", !7, i64 0}
!7 = !{!"omnipotent char", !8, i64 0}
!8 = !{!"Simple C/C++ TBAA"}
!9 = !{}
!10 = !{!"branch_weights", i32 1048575, i32 1}
!11 = !{!12, !12, i64 0}
!12 = !{!"any pointer", !7, i64 0}
!13 = !{!14, !6, i64 0}
!14 = !{!"MyStruct", !6, i64 0, !6, i64 4, !15, i64 8}
!15 = !{!"OtherStruct", !6, i64 0, !6, i64 4}
!16 = !{!14, !6, i64 4}
!17 = !{!14, !6, i64 8}
!18 = !{!14, !6, i64 12}
