#include "substitution.h"

#include "mcsat/tracing.h"
#include "mcsat/bv/bv_utils.h"

#include <stdbool.h>

void substitution_construct(substitution_t* subst, term_manager_t* tm, tracer_t* tracer) {
  init_int_hmap(&subst->substitution_fwd, 0);
  init_int_hmap(&subst->substitution_bck, 0);
  subst->tm = tm;
  subst->tracer = tracer;
}

void substitution_destruct(substitution_t* subst) {
  delete_int_hmap(&subst->substitution_fwd);
  delete_int_hmap(&subst->substitution_bck);
}

static
term_t substitution_run_core(substitution_t* subst, term_t t, int_hmap_t* cache) {

  uint32_t i, n;
  int_hmap_pair_t* find = NULL;

  // Term stuff
  term_manager_t* tm = subst->tm;
  term_table_t* terms = tm->terms;

  // Check if already done
  find = int_hmap_find(cache, t);
  if (find != NULL) {
    return find->val;
  }

  // Start
  ivector_t substitution_stack;
  init_ivector(&substitution_stack, 0);
  ivector_push(&substitution_stack, t);

  // Run the substitutions
  while (substitution_stack.size) {
    // Current term
    term_t current = ivector_last(&substitution_stack);

    if (trace_enabled(subst->tracer, "mcsat::subst")) {
      FILE* out = trace_out(subst->tracer);
      fprintf(out, "processing ");
      trace_term_ln(subst->tracer, terms, current);
    }

    // Check if done already
    find = int_hmap_find(cache, current);
    if (find != NULL) {
      ivector_pop(&substitution_stack);
      continue;
    }

    // Deal with negation
    if (is_neg_term(current)) {
      term_t child = unsigned_term(current);
      find = int_hmap_find(cache, child);
      if (find == NULL) {
        // Not yet done
        ivector_push(&substitution_stack, child);
        continue;
      } else {
        // Done, just set it
        ivector_pop(&substitution_stack);
        term_t current_subst = opposite_term(find->val);
        int_hmap_add(cache, current, current_subst);
        continue;
      }
    }

    // For now
    assert(term_type_kind(terms, current) == BOOL_TYPE || term_type_kind(terms, current) == BITVECTOR_TYPE);

    // Current term kind
    term_kind_t current_kind = term_kind(terms, current);
    // The result substitution (will be NULL if not done yet)
    term_t current_subst = NULL_TERM;

    // Process each term kind
    switch(current_kind) {

    // Constants are noop
    case CONSTANT_TERM:    // constant of uninterpreted/scalar/boolean types
    case BV64_CONSTANT:    // compact bitvector constant (64 bits at most)
    case BV_CONSTANT:      // generic bitvector constant (more than 64 bits)
      current_subst = current;
      break;

    // If a variable hasn't been done already, it stays
    case UNINTERPRETED_TERM:  // variables not
      current_subst = current;
      break;

    // Composite terms
    case EQ_TERM:            // equality
    case OR_TERM:            // n-ary OR
    case XOR_TERM:           // n-ary XOR
    case BV_ARRAY:
    case BV_DIV:
    case BV_REM:
    case BV_SDIV:
    case BV_SREM:
    case BV_SMOD:
    case BV_SHL:
    case BV_LSHR:
    case BV_ASHR:
    case BV_EQ_ATOM:
    case BV_GE_ATOM:
    case BV_SGE_ATOM:
    {
      composite_term_t* desc = composite_term_desc(terms, current);
      n = desc->arity;

      bool children_done = true; // All children substituted
      bool children_same = true; // All children substitution same (no need to make new term)

      ivector_t children;
      init_ivector(&children, n);
      for (i = 0; i < n; ++ i) {
        term_t child = desc->arg[i];
        find = int_hmap_find(cache, child);
        if (find == NULL) {
          children_done = false;
          ivector_push(&substitution_stack, child);
        } else if (find->val != child) {
          children_same = false;
        }
        if (children_done) {
          ivector_push(&children, find->val);
        }
      }

      // Make the substitution (or not if noop)
      if (children_done) {
        if (children_same) {
          current_subst = current;
        } else {
          current_subst = mk_bv_composite(tm, current_kind, n, children.data);
        }
      }

      delete_ivector(&children);
      break;
    }

    case BIT_TERM: // bit-select current = child[i]
    {
      uint32_t index = bit_term_index(terms, current);
      term_t arg = bit_term_arg(terms, current);
      find = int_hmap_find(cache, arg);
      if (find == NULL) {
        ivector_push(&substitution_stack, arg);
      } else {
        if (find->val == arg) {
          current_subst = current;
        } else {
          current_subst = bit_term(terms, index, find->val);
        }
      }
      break;
    }
    case BV_POLY:  // polynomial with generic bitvector coefficients
    {
      bvpoly_t* p = bvpoly_term_desc(terms, current);
      n = p->nterms;

      bool children_done = true;
      bool children_same = true;

      ivector_t children;
      init_ivector(&children, n);
      for (i = 0; i < n; ++ i) {
        term_t x = p->mono[i].var;
        if (x != const_idx) {
          find = int_hmap_find(cache, x);
          if (find == NULL) {
            children_done = false;
            ivector_push(&substitution_stack, x);
          } else if (find->val != x) {
            children_same = false;
          }
          if (children_done) { ivector_push(&children, find->val); }
        } else {
          if (children_done) { ivector_push(&children, const_idx); }
        }
      }

      if (children_done) {
        if (children_same) {
          current_subst = current;
        } else {
          current_subst = mk_bvarith_poly(tm, p, n, children.data);
        }
      }

      delete_ivector(&children);

      break;
    }
    case BV64_POLY: // polynomial with 64bit coefficients
    {
      bvpoly64_t* p = bvpoly64_term_desc(terms, current);
      n = p->nterms;

      bool children_done = true;
      bool children_same = true;

      ivector_t children;
      init_ivector(&children, n);
      for (i = 0; i < n; ++ i) {
        term_t x = p->mono[i].var;
        if (x != const_idx) {
          find = int_hmap_find(cache, x);
          if (find == NULL) {
            children_done = false;
            ivector_push(&substitution_stack, x);
          } else if (find->val != x) {
            children_same = false;
          }
          if (children_done) { ivector_push(&children, find->val); }
        } else {
          if (children_done) { ivector_push(&children, const_idx); }
        }
      }

      if (children_done) {
        if (children_same) {
          current_subst = current;
        } else {
          current_subst = mk_bvarith64_poly(tm, p, n, children.data);
        }
      }

      delete_ivector(&children);

      break;
    }

    case POWER_PRODUCT:    // power products: (t1^d1 * ... * t_n^d_n)
    {
      pprod_t* pp = pprod_term_desc(terms, current);
      n = pp->len;

      bool children_done = true;
      bool children_same = true;

      ivector_t children;
      init_ivector(&children, n);
      for (i = 0; i < n; ++ i) {
        term_t x = pp->prod[i].var;
        find = int_hmap_find(cache, x);
        if (find == NULL) {
          children_done = false;
          ivector_push(&substitution_stack, x);
        } else if (find->val != x) {
          children_same = false;
        }
        if (children_done) { ivector_push(&children, find->val); }
      }

      if (children_done) {
        if (children_same) {
          current_subst = current;
        } else {
          // NOTE: it doens't change pp, it just uses it as a frame
          current_subst = mk_pprod(tm, pp, n, children.data);
        }
      }

      delete_ivector(&children);

      break;
    }

    default:
      // UNSUPPORTED TERM/THEORY
      assert(false);
      break;
    }

    // If done with substitution, record and pop
    if (current_subst != NULL_TERM) {
      int_hmap_add(cache, current, current_subst);
      ivector_pop(&substitution_stack);
    }
  }

  // Return the result
  find = int_hmap_find(cache, t);
  assert(find != NULL);

  // Delete the stack
  delete_ivector(&substitution_stack);

  if (trace_enabled(subst->tracer, "mcsat::subst")) {
    FILE* out = trace_out(subst->tracer);
    fprintf(out, "substitution result:\n");
    trace_term_ln(subst->tracer, terms, t);
    trace_term_ln(subst->tracer, terms, find->val);
  }

  return find->val;
}

term_t substitution_run_fwd(substitution_t* subst, term_t t) {
  // Run the substitution
  return substitution_run_core(subst, t, &subst->substitution_fwd);
}

term_t substitution_run_bck(substitution_t* subst, term_t t) {
  // Run the substitution backwards
  return substitution_run_core(subst, t, &subst->substitution_bck);
}

bool substitution_has_term(const substitution_t* subst, term_t term) {
  return int_hmap_find((int_hmap_t*) &subst->substitution_fwd, term) != NULL;
}

void substitution_add(substitution_t* subst, term_t t, term_t t_subst) {
  int_hmap_pair_t* find = int_hmap_get(&subst->substitution_fwd, t);
  assert(find->val == -1);
  find->val = t_subst;
  find = int_hmap_get(&subst->substitution_bck, t_subst);
  assert(find->val == -1);
  find->val = t;
}


