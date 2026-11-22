// Site filler: a steering marker. It stands in for inline candidate call
// sites the original function held where nothing shows what they called.
//
// c2 gives a nested inline candidate the caller's remaining budget divided by
// the number of the caller's candidate sites not yet passed, free ones
// included (docs/compiler-notes.md, Inline budget), so a site that expands to
// nothing still shrinks the share of every expanded wrapper before it. It is
// the counterpart of RAG_BUDGET_FILL (src/rag_budget_fill.h), which adds own
// size without sites.
//
// RAG_BUDGET_SITES(n) is n statements rag_budget_site(); calling the empty
// inline function below (size 12, so below c2's free size of 40: always
// expanded, at no cost to the budget, and leaving no code). Each statement is
// one candidate site and adds 4 to the caller's own size (so 8 to its budget
// above the floor of 1000). n is a decimal literal from 1 to 32. It stands on
// the line of the statement it follows, so it adds no line.
//
// Codegen entropy (docs/compiler-notes.md, Codegen entropy): the #include of
// this header counts 2 where it stands (the include 1, the inline function's
// definition 1) for every function the unit compiles after it; the calls
// count nothing, whatever n. It is included just before the first function
// that holds a site filler, so functions before that keep their count.






#ifndef HIGHPRIEST_RAG_BUDGET_SITES_H
#define HIGHPRIEST_RAG_BUDGET_SITES_H
inline void rag_budget_site() {}
#define RAG_BUDGET_SITES(n) RAG_BUDGET_SITES_##n
#define RAG_BUDGET_SITES_1 rag_budget_site();
#define RAG_BUDGET_SITES_2 RAG_BUDGET_SITES_1 rag_budget_site();
#define RAG_BUDGET_SITES_3 RAG_BUDGET_SITES_2 rag_budget_site();
#define RAG_BUDGET_SITES_4 RAG_BUDGET_SITES_3 rag_budget_site();
#define RAG_BUDGET_SITES_5 RAG_BUDGET_SITES_4 rag_budget_site();
#define RAG_BUDGET_SITES_6 RAG_BUDGET_SITES_5 rag_budget_site();
#define RAG_BUDGET_SITES_7 RAG_BUDGET_SITES_6 rag_budget_site();
#define RAG_BUDGET_SITES_8 RAG_BUDGET_SITES_7 rag_budget_site();
#define RAG_BUDGET_SITES_9 RAG_BUDGET_SITES_8 rag_budget_site();
#define RAG_BUDGET_SITES_10 RAG_BUDGET_SITES_9 rag_budget_site();
#define RAG_BUDGET_SITES_11 RAG_BUDGET_SITES_10 rag_budget_site();
#define RAG_BUDGET_SITES_12 RAG_BUDGET_SITES_11 rag_budget_site();
#define RAG_BUDGET_SITES_13 RAG_BUDGET_SITES_12 rag_budget_site();
#define RAG_BUDGET_SITES_14 RAG_BUDGET_SITES_13 rag_budget_site();
#define RAG_BUDGET_SITES_15 RAG_BUDGET_SITES_14 rag_budget_site();
#define RAG_BUDGET_SITES_16 RAG_BUDGET_SITES_15 rag_budget_site();
#define RAG_BUDGET_SITES_17 RAG_BUDGET_SITES_16 rag_budget_site();
#define RAG_BUDGET_SITES_18 RAG_BUDGET_SITES_17 rag_budget_site();
#define RAG_BUDGET_SITES_19 RAG_BUDGET_SITES_18 rag_budget_site();
#define RAG_BUDGET_SITES_20 RAG_BUDGET_SITES_19 rag_budget_site();
#define RAG_BUDGET_SITES_21 RAG_BUDGET_SITES_20 rag_budget_site();
#define RAG_BUDGET_SITES_22 RAG_BUDGET_SITES_21 rag_budget_site();
#define RAG_BUDGET_SITES_23 RAG_BUDGET_SITES_22 rag_budget_site();
#define RAG_BUDGET_SITES_24 RAG_BUDGET_SITES_23 rag_budget_site();
#define RAG_BUDGET_SITES_25 RAG_BUDGET_SITES_24 rag_budget_site();
#define RAG_BUDGET_SITES_26 RAG_BUDGET_SITES_25 rag_budget_site();
#define RAG_BUDGET_SITES_27 RAG_BUDGET_SITES_26 rag_budget_site();
#define RAG_BUDGET_SITES_28 RAG_BUDGET_SITES_27 rag_budget_site();
#define RAG_BUDGET_SITES_29 RAG_BUDGET_SITES_28 rag_budget_site();
#define RAG_BUDGET_SITES_30 RAG_BUDGET_SITES_29 rag_budget_site();
#define RAG_BUDGET_SITES_31 RAG_BUDGET_SITES_30 rag_budget_site();
#define RAG_BUDGET_SITES_32 RAG_BUDGET_SITES_31 rag_budget_site();
#endif
