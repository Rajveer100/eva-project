; ModuleID = 'Eva'
source_filename = "Eva"

@VERSION = global i32 42, align 4
@0 = private unnamed_addr constant [6 x i8] c"Hello\00", align 1
@1 = private unnamed_addr constant [8 x i8] c"X: %s\0A\0A\00", align 1
@2 = private unnamed_addr constant [8 x i8] c"X: %d\0A\0A\00", align 1
@3 = private unnamed_addr constant [8 x i8] c"X: %d\0A\0A\00", align 1

declare i32 @printf(ptr, ...)

define i32 @main() {
entry:
  %x = alloca i32, align 4
  store i32 42, ptr %x, align 4
  %x1 = alloca ptr, align 8
  store ptr @0, ptr %x1, align 8
  %x2 = load ptr, ptr %x1, align 8
  %0 = call i32 (ptr, ...) @printf(ptr @1, ptr %x2)
  %x3 = load ptr, ptr %x, align 8
  %1 = call i32 (ptr, ...) @printf(ptr @2, ptr %x3)
  store i32 100, ptr %x, align 4
  %x4 = load ptr, ptr %x, align 8
  %2 = call i32 (ptr, ...) @printf(ptr @3, ptr %x4)
  ret i32 0
}
