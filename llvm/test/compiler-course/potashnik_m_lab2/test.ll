; RUN: opt -load-pass-plugin %llvmshlibdir/potashnik_m_lab2_LLVM_IR%pluginext\
; RUN: -passes=replace-icmp-to-opposite -S %s | FileCheck %s

define i1 @test_slt(i32 %x, i32 %y) {
; CHECK-LABEL: @test_slt
; CHECK: %cmp.inv = icmp sge i32 %x, %y
; CHECK-NEXT: %cmp.inv.not = xor i1 %cmp.inv, true
; CHECK-NEXT: ret i1 %cmp.inv.not
  %cmp = icmp slt i32 %x, %y
  ret i1 %cmp
}

define i1 @test_sgt(i32 %x, i32 %y) {
; CHECK-LABEL: @test_sgt
; CHECK: %cmp.inv = icmp sle i32 %x, %y
; CHECK-NEXT: %cmp.inv.not = xor i1 %cmp.inv, true
; CHECK-NEXT: ret i1 %cmp.inv.not
  %cmp = icmp sgt i32 %x, %y
  ret i1 %cmp
}

define i1 @test_sle(i32 %x, i32 %y) {
; CHECK-LABEL: @test_sle
; CHECK: %cmp.inv = icmp sgt i32 %x, %y
; CHECK-NEXT: %cmp.inv.not = xor i1 %cmp.inv, true
; CHECK-NEXT: ret i1 %cmp.inv.not
  %cmp = icmp sle i32 %x, %y
  ret i1 %cmp
}

define i1 @test_sge(i32 %x, i32 %y) {
; CHECK-LABEL: @test_sge
; CHECK: %cmp.inv = icmp slt i32 %x, %y
; CHECK-NEXT: %cmp.inv.not = xor i1 %cmp.inv, true
; CHECK-NEXT: ret i1 %cmp.inv.not
  %cmp = icmp sge i32 %x, %y
  ret i1 %cmp
}

define i1 @test_ult(i32 %x, i32 %y) {
; CHECK-LABEL: @test_ult
; CHECK: %cmp.inv = icmp uge i32 %x, %y
; CHECK-NEXT: %cmp.inv.not = xor i1 %cmp.inv, true
; CHECK-NEXT: ret i1 %cmp.inv.not
  %cmp = icmp ult i32 %x, %y
  ret i1 %cmp
}

define i1 @test_ugt(i32 %x, i32 %y) {
; CHECK-LABEL: @test_ugt
; CHECK: %cmp.inv = icmp ule i32 %x, %y
; CHECK-NEXT: %cmp.inv.not = xor i1 %cmp.inv, true
; CHECK-NEXT: ret i1 %cmp.inv.not
  %cmp = icmp ugt i32 %x, %y
  ret i1 %cmp
}

define i1 @test_ule(i32 %x, i32 %y) {
; CHECK-LABEL: @test_ule
; CHECK: %cmp.inv = icmp ugt i32 %x, %y
; CHECK-NEXT: %cmp.inv.not = xor i1 %cmp.inv, true
; CHECK-NEXT: ret i1 %cmp.inv.not
  %cmp = icmp ule i32 %x, %y
  ret i1 %cmp
}

define i1 @test_uge(i32 %x, i32 %y) {
; CHECK-LABEL: @test_uge
; CHECK: %cmp.inv = icmp ult i32 %x, %y
; CHECK-NEXT: %cmp.inv.not = xor i1 %cmp.inv, true
; CHECK-NEXT: ret i1 %cmp.inv.not
  %cmp = icmp uge i32 %x, %y
  ret i1 %cmp
}

define i1 @test_eq(i32 %x, i32 %y) {
; CHECK-LABEL: @test_eq
; CHECK: %cmp.inv = icmp ne i32 %x, %y
; CHECK-NEXT: %cmp.inv.not = xor i1 %cmp.inv, true
; CHECK-NEXT: ret i1 %cmp.inv.not
  %cmp = icmp eq i32 %x, %y
  ret i1 %cmp
}

define i1 @test_ne(i32 %x, i32 %y) {
; CHECK-LABEL: @test_ne
; CHECK: %cmp.inv = icmp eq i32 %x, %y
; CHECK-NEXT: %cmp.inv.not = xor i1 %cmp.inv, true
; CHECK-NEXT: ret i1 %cmp.inv.not
  %cmp = icmp ne i32 %x, %y
  ret i1 %cmp
}