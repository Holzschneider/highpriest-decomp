// Entropy padding: a steering marker. Its definitions exist only to move the
// unit's declaration count, on which some functions' codegen depends modulo
// 64 (docs/compiler-notes.md, Codegen entropy). They stand in for header
// content the original had.
//
// RAG_ENTROPY_SHIFT(name, n), n a decimal literal from 0 to 63, declares n
// unused externs (extern int rag_entropy_pad_<name>_<tag>;), each worth 1, so
// it shifts the count of every later function by n. The #include of this
// macro-only header is worth 1 where it stands: it is included just before
// the first pad, so the functions before it keep their count. A pad pair puts
// the shift a count-sensitive function needs before it (the include's 1
// counted in) and a second RAG_ENTROPY_SHIFT that brings the total to a
// multiple of 64, so every later function compiles as without the pads. The
// restore goes after the last place whose count the function's code depends
// on, which can lie past its own body.
// RAG_ENTROPY_FILL_256(name) declares 256 such externs: a unit whose code
// depends on the count modulo 65,536 (the 16-bit counter in the keys of
// calls) takes several, each with its own name (src/resource/Action.cpp).
// RAG_ENTROPY_PAD(name) adds 2 (an empty inline function) and
// RAG_ENTROPY_PAD_ENUM(name) a one-enumerator enum.









#ifndef HIGHPRIEST_RAG_ENTROPY_PAD_H
#define HIGHPRIEST_RAG_ENTROPY_PAD_H
#define RAG_ENTROPY_PAD(name) inline void rag_entropy_pad_##name() {}
#define RAG_ENTROPY_PAD_ENUM(name) enum { rag_entropy_pad_##name };
#define RAG_ENTROPY_SHIFT(name, n) RAG_ENTROPY_SHIFT_##n(name)
#define RAG_ENTROPY_PAD_DECL(name, tag) extern int rag_entropy_pad_##name##_##tag;
#define RAG_ENTROPY_PAD_D16(name, h) RAG_ENTROPY_PAD_DECL(name, h##0) RAG_ENTROPY_PAD_DECL(name, h##1) RAG_ENTROPY_PAD_DECL(name, h##2) RAG_ENTROPY_PAD_DECL(name, h##3) RAG_ENTROPY_PAD_DECL(name, h##4) RAG_ENTROPY_PAD_DECL(name, h##5) RAG_ENTROPY_PAD_DECL(name, h##6) RAG_ENTROPY_PAD_DECL(name, h##7) RAG_ENTROPY_PAD_DECL(name, h##8) RAG_ENTROPY_PAD_DECL(name, h##9) RAG_ENTROPY_PAD_DECL(name, h##10) RAG_ENTROPY_PAD_DECL(name, h##11) RAG_ENTROPY_PAD_DECL(name, h##12) RAG_ENTROPY_PAD_DECL(name, h##13) RAG_ENTROPY_PAD_DECL(name, h##14) RAG_ENTROPY_PAD_DECL(name, h##15)
#define RAG_ENTROPY_FILL_256(name) RAG_ENTROPY_PAD_D16(name, f0) RAG_ENTROPY_PAD_D16(name, f1) RAG_ENTROPY_PAD_D16(name, f2) RAG_ENTROPY_PAD_D16(name, f3) RAG_ENTROPY_PAD_D16(name, f4) RAG_ENTROPY_PAD_D16(name, f5) RAG_ENTROPY_PAD_D16(name, f6) RAG_ENTROPY_PAD_D16(name, f7) RAG_ENTROPY_PAD_D16(name, f8) RAG_ENTROPY_PAD_D16(name, f9) RAG_ENTROPY_PAD_D16(name, fa) RAG_ENTROPY_PAD_D16(name, fb) RAG_ENTROPY_PAD_D16(name, fc) RAG_ENTROPY_PAD_D16(name, fd) RAG_ENTROPY_PAD_D16(name, fe) RAG_ENTROPY_PAD_D16(name, ff)
#define RAG_ENTROPY_PAD_H0(name)
#define RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_D16(name, a)
#define RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_D16(name, b)
#define RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_D16(name, c)
#define RAG_ENTROPY_PAD_L0(name)
#define RAG_ENTROPY_PAD_L1(name) RAG_ENTROPY_PAD_L0(name) RAG_ENTROPY_PAD_DECL(name, l1)
#define RAG_ENTROPY_PAD_L2(name) RAG_ENTROPY_PAD_L1(name) RAG_ENTROPY_PAD_DECL(name, l2)
#define RAG_ENTROPY_PAD_L3(name) RAG_ENTROPY_PAD_L2(name) RAG_ENTROPY_PAD_DECL(name, l3)
#define RAG_ENTROPY_PAD_L4(name) RAG_ENTROPY_PAD_L3(name) RAG_ENTROPY_PAD_DECL(name, l4)
#define RAG_ENTROPY_PAD_L5(name) RAG_ENTROPY_PAD_L4(name) RAG_ENTROPY_PAD_DECL(name, l5)
#define RAG_ENTROPY_PAD_L6(name) RAG_ENTROPY_PAD_L5(name) RAG_ENTROPY_PAD_DECL(name, l6)
#define RAG_ENTROPY_PAD_L7(name) RAG_ENTROPY_PAD_L6(name) RAG_ENTROPY_PAD_DECL(name, l7)
#define RAG_ENTROPY_PAD_L8(name) RAG_ENTROPY_PAD_L7(name) RAG_ENTROPY_PAD_DECL(name, l8)
#define RAG_ENTROPY_PAD_L9(name) RAG_ENTROPY_PAD_L8(name) RAG_ENTROPY_PAD_DECL(name, l9)
#define RAG_ENTROPY_PAD_L10(name) RAG_ENTROPY_PAD_L9(name) RAG_ENTROPY_PAD_DECL(name, l10)
#define RAG_ENTROPY_PAD_L11(name) RAG_ENTROPY_PAD_L10(name) RAG_ENTROPY_PAD_DECL(name, l11)
#define RAG_ENTROPY_PAD_L12(name) RAG_ENTROPY_PAD_L11(name) RAG_ENTROPY_PAD_DECL(name, l12)
#define RAG_ENTROPY_PAD_L13(name) RAG_ENTROPY_PAD_L12(name) RAG_ENTROPY_PAD_DECL(name, l13)
#define RAG_ENTROPY_PAD_L14(name) RAG_ENTROPY_PAD_L13(name) RAG_ENTROPY_PAD_DECL(name, l14)
#define RAG_ENTROPY_PAD_L15(name) RAG_ENTROPY_PAD_L14(name) RAG_ENTROPY_PAD_DECL(name, l15)
#define RAG_ENTROPY_SHIFT_0(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L0(name)
#define RAG_ENTROPY_SHIFT_1(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L1(name)
#define RAG_ENTROPY_SHIFT_2(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L2(name)
#define RAG_ENTROPY_SHIFT_3(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L3(name)
#define RAG_ENTROPY_SHIFT_4(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L4(name)
#define RAG_ENTROPY_SHIFT_5(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L5(name)
#define RAG_ENTROPY_SHIFT_6(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L6(name)
#define RAG_ENTROPY_SHIFT_7(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L7(name)
#define RAG_ENTROPY_SHIFT_8(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L8(name)
#define RAG_ENTROPY_SHIFT_9(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L9(name)
#define RAG_ENTROPY_SHIFT_10(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L10(name)
#define RAG_ENTROPY_SHIFT_11(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L11(name)
#define RAG_ENTROPY_SHIFT_12(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L12(name)
#define RAG_ENTROPY_SHIFT_13(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L13(name)
#define RAG_ENTROPY_SHIFT_14(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L14(name)
#define RAG_ENTROPY_SHIFT_15(name) RAG_ENTROPY_PAD_H0(name) RAG_ENTROPY_PAD_L15(name)
#define RAG_ENTROPY_SHIFT_16(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L0(name)
#define RAG_ENTROPY_SHIFT_17(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L1(name)
#define RAG_ENTROPY_SHIFT_18(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L2(name)
#define RAG_ENTROPY_SHIFT_19(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L3(name)
#define RAG_ENTROPY_SHIFT_20(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L4(name)
#define RAG_ENTROPY_SHIFT_21(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L5(name)
#define RAG_ENTROPY_SHIFT_22(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L6(name)
#define RAG_ENTROPY_SHIFT_23(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L7(name)
#define RAG_ENTROPY_SHIFT_24(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L8(name)
#define RAG_ENTROPY_SHIFT_25(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L9(name)
#define RAG_ENTROPY_SHIFT_26(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L10(name)
#define RAG_ENTROPY_SHIFT_27(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L11(name)
#define RAG_ENTROPY_SHIFT_28(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L12(name)
#define RAG_ENTROPY_SHIFT_29(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L13(name)
#define RAG_ENTROPY_SHIFT_30(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L14(name)
#define RAG_ENTROPY_SHIFT_31(name) RAG_ENTROPY_PAD_H1(name) RAG_ENTROPY_PAD_L15(name)
#define RAG_ENTROPY_SHIFT_32(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L0(name)
#define RAG_ENTROPY_SHIFT_33(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L1(name)
#define RAG_ENTROPY_SHIFT_34(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L2(name)
#define RAG_ENTROPY_SHIFT_35(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L3(name)
#define RAG_ENTROPY_SHIFT_36(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L4(name)
#define RAG_ENTROPY_SHIFT_37(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L5(name)
#define RAG_ENTROPY_SHIFT_38(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L6(name)
#define RAG_ENTROPY_SHIFT_39(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L7(name)
#define RAG_ENTROPY_SHIFT_40(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L8(name)
#define RAG_ENTROPY_SHIFT_41(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L9(name)
#define RAG_ENTROPY_SHIFT_42(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L10(name)
#define RAG_ENTROPY_SHIFT_43(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L11(name)
#define RAG_ENTROPY_SHIFT_44(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L12(name)
#define RAG_ENTROPY_SHIFT_45(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L13(name)
#define RAG_ENTROPY_SHIFT_46(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L14(name)
#define RAG_ENTROPY_SHIFT_47(name) RAG_ENTROPY_PAD_H2(name) RAG_ENTROPY_PAD_L15(name)
#define RAG_ENTROPY_SHIFT_48(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L0(name)
#define RAG_ENTROPY_SHIFT_49(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L1(name)
#define RAG_ENTROPY_SHIFT_50(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L2(name)
#define RAG_ENTROPY_SHIFT_51(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L3(name)
#define RAG_ENTROPY_SHIFT_52(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L4(name)
#define RAG_ENTROPY_SHIFT_53(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L5(name)
#define RAG_ENTROPY_SHIFT_54(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L6(name)
#define RAG_ENTROPY_SHIFT_55(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L7(name)
#define RAG_ENTROPY_SHIFT_56(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L8(name)
#define RAG_ENTROPY_SHIFT_57(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L9(name)
#define RAG_ENTROPY_SHIFT_58(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L10(name)
#define RAG_ENTROPY_SHIFT_59(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L11(name)
#define RAG_ENTROPY_SHIFT_60(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L12(name)
#define RAG_ENTROPY_SHIFT_61(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L13(name)
#define RAG_ENTROPY_SHIFT_62(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L14(name)
#define RAG_ENTROPY_SHIFT_63(name) RAG_ENTROPY_PAD_H3(name) RAG_ENTROPY_PAD_L15(name)
#endif
