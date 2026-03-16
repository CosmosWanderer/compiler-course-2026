// RUN: %clang_cc1 -load %llvmshlibdir/potashnik_m_lab1_ClangAST.so -plugin potashnik_m_lab1_plugin -fsyntax-only %s 2>&1 | FileCheck %s

// CHECK-NOT: int{{\*}} p1 = &var1;
// CHECK: const int{{\*}} const p1 = &var1;
void const_data_const_pointer() {
    int var1 = 10;
    int* p1 = &var1;    
}

// CHECK-NOT: const int{{\*}} const p2 = &var2;
// CHECK: int{{\*}} const p2 = &var2;
void nonconst_data_const_pointer() {
    int var2 = 10;
    int* p2 = &var2;   
    *p2 = 15;
}

// CHECK-NOT: const int{{\*}} const p3 = &var31;
// CHECK: const int{{\*}} p3 = &var31;
void const_data_nonconst_pointer() {
    int var31 = 10;
    int var32 = 15;
    int* p3 = &var31;   
    p3 = &var32;
}

// CHECK-NOT: const int{{\*}} const p4 = &var4;
// CHECK: const int{{\*}} p4 = &var4;
void const_data_nonconst_pointer_unary_op() {
    int var4 = 11;
    int* p4 = &var4;
    p4++;
}

// CHECK-NOT: const int{{\*}} const p5 = &var5;
// CHECK: int{{\*}} const p5 = &var5;
void nonconst_data_const_pointer_unary_op() {
    int var5 = 11;
    int* p5 = &var5;
    (*p5)++;
}

// CHECK-NOT: const int{{\*}} const p6 = &var6;
// CHECK: const int{{\*}} p6 = &var6;
void unary_op_const_data_nonconst_pointer() {
    int var6 = 11;
    int* p6 = &var6;
    ++p6;
}

// CHECK-NOT: const int{{\*}} const p7 = &var7;
// CHECK: int{{\*}} const p7 = &var7;
void unary_op_nonconst_data_const_pointer() {
    int var7 = 11;
    int* p7 = &var7;
    ++(*p7);
}

// CHECK-NOT: const const int{{\*}} const const p8 = &var8;
// CHECK: const int{{\*}} const p8 = &var8;
void already_const_pointer() {
    const int var8 = 15;
    const int* const p8 = &var8;
}

// CHECK-NOT: void const_reference(int &r1{{\)}} { 
// CHECK: void const_reference(const int &r1{{\)}} { 
void const_reference(int &r1) {
    int r2 = 1;  
}

// CHECK-NOT: void nonconst_reference(const int &r2{{\)}} { 
// CHECK: void nonconst_reference(int &r2{{\)}} { 
void nonconst_reference(int &r2) {
    r2 = 10;  
}

// CHECK-NOT: int{{\&}} r1 = var1;
// CHECK: const int{{\&}} r1 = var1;
void const_reference2() {
    int var1 = 10;
    int& r1 = var1;    
}

// CHECK-NOT: const int{{\&}} r2 = var2;
// CHECK: int{{\&}} r2 = var2;
void nonconst_reference2() {
    int var2 = 10;
    int& r2 = var2;
    r2 = 11;    
}
