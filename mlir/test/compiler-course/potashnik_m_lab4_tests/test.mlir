// RUN: mlir-opt -load-pass-plugin=%mlir_lib_dir/potashnik_m_lab4_MLIR%shlibext --pass-pipeline="builtin.module(trip-count)" %s | FileCheck %s

func.func @test_all_loops(%arg0: index, %arg1: index) {
  // simple loop
  // CHECK: affine.for %{{.*}} = 0 to 10 {
  // CHECK: } {trip_count = 10 : i64}
  affine.for %i = 0 to 10 { arith.constant 1 : i32 }

  // offset loop
  // CHECK: affine.for %{{.*}} = 5 to 15 {
  // CHECK: } {trip_count = 10 : i64}
  affine.for %i = 5 to 15 { arith.constant 1 : i32 }

  // step loop
  // CHECK: affine.for %{{.*}} = 0 to 10 step 2 {
  // CHECK: } {trip_count = 5 : i64}
  affine.for %i = 0 to 10 step 2 { arith.constant 1 : i32 }

  // complex math loop
  // CHECK: affine.for %{{.*}} = 2 to 20 step 3 {
  // CHECK: } {trip_count = 6 : i64}
  affine.for %i = 2 to 20 step 3 { arith.constant 1 : i32 }

  // negative bounds
  // CHECK: affine.for %{{.*}} = -5 to 5 {
  // CHECK: } {trip_count = 10 : i64}
  affine.for %i = -5 to 5 { arith.constant 1 : i32 }

  // nested loops
  // CHECK: affine.for %{{.*}} = 0 to 4 {
  // CHECK:   affine.for %{{.*}} = 0 to 2 {
  // CHECK:   } {trip_count = 2 : i64}
  // CHECK: } {trip_count = 4 : i64}
  affine.for %i = 0 to 4 {
    affine.for %j = 0 to 2 { arith.constant 1 : i32 }
  }

  // zero trip (lower > upper)
  // CHECK: affine.for %{{.*}} = 10 to 5 {
  // CHECK: } {trip_count = 0 : i64}
  affine.for %i = 10 to 5 { arith.constant 1 : i32 }

  // zero trip (equal bounds)
  // CHECK: affine.for %{{.*}} = 5 to 5 {
  // CHECK: } {trip_count = 0 : i64}
  affine.for %i = 5 to 5 { arith.constant 1 : i32 }

  // step larger than range -> 1 iteration
  // CHECK: affine.for %{{.*}} = 0 to 3 step 10 {
  // CHECK: } {trip_count = 1 : i64}
  affine.for %i = 0 to 3 step 10 { arith.constant 1 : i32 }

  // dynamic loops – no trip_count attribute
  // CHECK: affine.for %{{.*}} = 0 to %{{.*}} {
  // CHECK-NOT: trip_count
  affine.for %i = 0 to %arg0 { arith.constant 1 : i32 }
  // CHECK: affine.for %{{.*}} = %{{.*}} to 100 {
  // CHECK-NOT: trip_count
  affine.for %i = %arg1 to 100 { arith.constant 1 : i32 }

  return
}