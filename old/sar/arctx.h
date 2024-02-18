#pragma once

#include "scom/dict.h"
#include "scom/check.h"
#include "sym_group.h"
#include "../../../sar/elf_member.h"
#include <sys/stat.h>
#include <ctype.h>

static void arctx_verify_member_by_name(struct arctx* ctx, const char* mem_name) {
  printf("Verify member %s\n", mem_name);
  struct elf_member* elfmem = arctx_get_elfmem_by_name(ctx, mem_name);
  elfmem_verify(elfmem, ctx->sglist);
}

/*
 * target_syms will be mutated.
 */
static void arctx_resolve_symbols(struct arctx* ctx, struct vec* target_syms, struct vec* predefined_syms) {
  assert(target_syms->len > 0);
  printf("Need resolve the following %d symbols:\n", target_syms->len);
  VEC_FOREACH(target_syms, char*, itemptr) {
    printf(" - %s\n", *itemptr);
  }

  struct dict added_file = dict_create_str_int(); // value is not used
	added_file.should_free_key = false; // TODO

  // resolve symbols in stack order
  while (target_syms->len > 0) {
    char* sym = *(char**) vec_pop_item(target_syms);
    // linear scan is fine since predefined_syms usually is quite a small list.
    if (vec_str_find(predefined_syms, sym) >= 0) {
      free(sym);
      continue;
    }
    struct dict_entry* entry = dict_find(&ctx->symname2memidx, (void*) sym);
    if (!entry->key) {
      FAIL("Symbol not found %s\n", sym);
    }
    int elfmem_idx = (int) entry->val;
    struct elf_member* elfmem = vec_get_item(&ctx->elf_mem_list, elfmem_idx);

    if (dict_put(&added_file, (void*) elfmem->name, (void*) 0) > 0) {
      printf("Add file %s\n", elfmem->name);
      // add the needed symbols
      VEC_FOREACH(&elfmem->need_syms, char*, pstr) {
        char *newsym = strdup(*pstr);
        vec_append(target_syms, &newsym);
        #if 0
        printf(" - need %s\n", newsym);
        #endif
      }
    }
    free(sym);
  }

	#if 0
  const char* path = "/tmp/dep_obj_list";
  printf("Write %d dependencies to %s\n", added_file.size, path);
  FILE* fp = fopen(path, "w");
  DICT_FOREACH(&added_file, entry_ptr) {
    fprintf(fp, "%s\n", entry_ptr->key);
  }
  fclose(fp);
	#else
	// assert(0 && "disabled temporarily");
	#endif
  
  dict_free(&added_file);
}
