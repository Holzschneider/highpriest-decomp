// Symbol shift: a steering marker. It stands in for symbols the original
// function held (locals, inline expansions) whose source nothing shows.
//
// c2 numbers the optimizer's temporaries after the function's symbols, and
// the hash that orders commutative operands contains those numbers
// (docs/compiler-notes.md, Symbol-number shift), so the count of symbols
// decides operand orders anywhere in the function.
//
// RAG_SYMBOL_SHIFT(n) is n blocks, each one local and one dead store to it:
//   { int rag_symbol_shift; rag_symbol_shift = 0; }
// The optimizer removes store and local: no machine code, no frame slot, no
// line record. Each block adds one symbol; it also adds a little own size, so
// past about 14 blocks inlining can change. n is a decimal literal 1 to 16.
//
// Codegen entropy (docs/compiler-notes.md, Codegen entropy): each block's
// local counts 1 for every function the unit compiles after it, and the
// #include of this macro-only header counts 1 where it stands; it is included
// just before the first function that holds a shift.
//
// A self-assignment statement E = E; is the same kind of marker: it adds
// temporaries where it stands instead of symbols, and needs no include. So is
// a call of an inline function named rag_symbol_shift whose body assigns its
// parameters: each assigned parameter keeps one inline temporary, which moves
// the 32-symbol chunks of that pool (UIMailListWnd::OnCreate defines one
// beside it).
// RAG_SYMBOL_CHAIN(n) declares n locals and stores 0 to all of them in one
// chained assignment: n symbols, like n RAG_SYMBOL_SHIFT blocks, for much
// less own size, which matters where the function is itself inlined and its
// callers' budgets must stay as they are. n is a decimal literal 2 to 16; its
// locals are named rag_symbol_shift... like the blocks'.
// Both macros open a block: they go after the function's static locals, or
// each static's decorated name takes the next scope number (?1 -> ?2) and the
// unit's .bss layout moves (C3dNode::UpdateVertexColor in src/3dActor.cpp).


#ifndef HIGHPRIEST_RAG_SYMBOL_SHIFT_H
#define HIGHPRIEST_RAG_SYMBOL_SHIFT_H
#define RAG_SYMBOL_SHIFT_B { int rag_symbol_shift; rag_symbol_shift = 0; }
#define RAG_SYMBOL_SHIFT_1 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_2 RAG_SYMBOL_SHIFT_1 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_3 RAG_SYMBOL_SHIFT_2 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_4 RAG_SYMBOL_SHIFT_3 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_5 RAG_SYMBOL_SHIFT_4 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_6 RAG_SYMBOL_SHIFT_5 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_7 RAG_SYMBOL_SHIFT_6 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_8 RAG_SYMBOL_SHIFT_7 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_9 RAG_SYMBOL_SHIFT_8 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_10 RAG_SYMBOL_SHIFT_9 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_11 RAG_SYMBOL_SHIFT_10 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_12 RAG_SYMBOL_SHIFT_11 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_13 RAG_SYMBOL_SHIFT_12 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_14 RAG_SYMBOL_SHIFT_13 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_15 RAG_SYMBOL_SHIFT_14 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT_16 RAG_SYMBOL_SHIFT_15 RAG_SYMBOL_SHIFT_B
#define RAG_SYMBOL_SHIFT(n) RAG_SYMBOL_SHIFT_##n
#define RAG_SYMBOL_CHAIN(n) { int rag_symbol_shift RAG_SYMBOL_CHAIN_D##n; rag_symbol_shift = RAG_SYMBOL_CHAIN_A##n 0; }
#define RAG_SYMBOL_CHAIN_D1
#define RAG_SYMBOL_CHAIN_A1
#define RAG_SYMBOL_CHAIN_D2 RAG_SYMBOL_CHAIN_D1, rag_symbol_shift_1
#define RAG_SYMBOL_CHAIN_A2 RAG_SYMBOL_CHAIN_A1 rag_symbol_shift_1 =
#define RAG_SYMBOL_CHAIN_D3 RAG_SYMBOL_CHAIN_D2, rag_symbol_shift_2
#define RAG_SYMBOL_CHAIN_A3 RAG_SYMBOL_CHAIN_A2 rag_symbol_shift_2 =
#define RAG_SYMBOL_CHAIN_D4 RAG_SYMBOL_CHAIN_D3, rag_symbol_shift_3
#define RAG_SYMBOL_CHAIN_A4 RAG_SYMBOL_CHAIN_A3 rag_symbol_shift_3 =
#define RAG_SYMBOL_CHAIN_D5 RAG_SYMBOL_CHAIN_D4, rag_symbol_shift_4
#define RAG_SYMBOL_CHAIN_A5 RAG_SYMBOL_CHAIN_A4 rag_symbol_shift_4 =
#define RAG_SYMBOL_CHAIN_D6 RAG_SYMBOL_CHAIN_D5, rag_symbol_shift_5
#define RAG_SYMBOL_CHAIN_A6 RAG_SYMBOL_CHAIN_A5 rag_symbol_shift_5 =
#define RAG_SYMBOL_CHAIN_D7 RAG_SYMBOL_CHAIN_D6, rag_symbol_shift_6
#define RAG_SYMBOL_CHAIN_A7 RAG_SYMBOL_CHAIN_A6 rag_symbol_shift_6 =
#define RAG_SYMBOL_CHAIN_D8 RAG_SYMBOL_CHAIN_D7, rag_symbol_shift_7
#define RAG_SYMBOL_CHAIN_A8 RAG_SYMBOL_CHAIN_A7 rag_symbol_shift_7 =
#define RAG_SYMBOL_CHAIN_D9 RAG_SYMBOL_CHAIN_D8, rag_symbol_shift_8
#define RAG_SYMBOL_CHAIN_A9 RAG_SYMBOL_CHAIN_A8 rag_symbol_shift_8 =
#define RAG_SYMBOL_CHAIN_D10 RAG_SYMBOL_CHAIN_D9, rag_symbol_shift_9
#define RAG_SYMBOL_CHAIN_A10 RAG_SYMBOL_CHAIN_A9 rag_symbol_shift_9 =
#define RAG_SYMBOL_CHAIN_D11 RAG_SYMBOL_CHAIN_D10, rag_symbol_shift_10
#define RAG_SYMBOL_CHAIN_A11 RAG_SYMBOL_CHAIN_A10 rag_symbol_shift_10 =
#define RAG_SYMBOL_CHAIN_D12 RAG_SYMBOL_CHAIN_D11, rag_symbol_shift_11
#define RAG_SYMBOL_CHAIN_A12 RAG_SYMBOL_CHAIN_A11 rag_symbol_shift_11 =
#define RAG_SYMBOL_CHAIN_D13 RAG_SYMBOL_CHAIN_D12, rag_symbol_shift_12
#define RAG_SYMBOL_CHAIN_A13 RAG_SYMBOL_CHAIN_A12 rag_symbol_shift_12 =
#define RAG_SYMBOL_CHAIN_D14 RAG_SYMBOL_CHAIN_D13, rag_symbol_shift_13
#define RAG_SYMBOL_CHAIN_A14 RAG_SYMBOL_CHAIN_A13 rag_symbol_shift_13 =
#define RAG_SYMBOL_CHAIN_D15 RAG_SYMBOL_CHAIN_D14, rag_symbol_shift_14
#define RAG_SYMBOL_CHAIN_A15 RAG_SYMBOL_CHAIN_A14 rag_symbol_shift_14 =
#define RAG_SYMBOL_CHAIN_D16 RAG_SYMBOL_CHAIN_D15, rag_symbol_shift_15
#define RAG_SYMBOL_CHAIN_A16 RAG_SYMBOL_CHAIN_A15 rag_symbol_shift_15 =
#endif
