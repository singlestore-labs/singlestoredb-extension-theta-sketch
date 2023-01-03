#include <stdlib.h>
#include <extension.h>

__attribute__((weak, export_name("canonical_abi_realloc")))
void *canonical_abi_realloc(
void *ptr,
size_t orig_size,
size_t org_align,
size_t new_size
) {
  void *ret = realloc(ptr, new_size);
  if (!ret)
  abort();
  return ret;
}

__attribute__((weak, export_name("canonical_abi_free")))
void canonical_abi_free(
void *ptr,
size_t size,
size_t align
) {
  free(ptr);
}
#include <string.h>

void extension_string_set(extension_string_t *ret, const char *s) {
  ret->ptr = (char*) s;
  ret->len = strlen(s);
}

void extension_string_dup(extension_string_t *ret, const char *s) {
  ret->len = strlen(s);
  ret->ptr = (char *)canonical_abi_realloc(NULL, 0, 1, ret->len);
  memcpy(ret->ptr, s, ret->len);
}

void extension_string_free(extension_string_t *ret) {
  canonical_abi_free(ret->ptr, ret->len, 1);
  ret->ptr = NULL;
  ret->len = 0;
}
void extension_list_u8_free(extension_list_u8_t *ptr) {
  canonical_abi_free(ptr->ptr, ptr->len * 1, 1);
}
void extension_state_free(extension_state_t *ptr) {
  extension_list_u8_free(&ptr->buffer);
}

__attribute__((aligned(4)))
static uint8_t RET_AREA[12];
__attribute__((export_name("greet")))
int32_t __wasm_export_extension_greet(int32_t arg, int32_t arg0) {
  extension_string_t arg1 = (extension_string_t) { (char*)(arg), (size_t)(arg0) };
  extension_string_t ret;
  extension_greet(&arg1, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("answer-to-life")))
int32_t __wasm_export_extension_answer_to_life(void) {
  int32_t ret = extension_answer_to_life();
  return ret;
}
__attribute__((export_name("set-state")))
int32_t __wasm_export_extension_set_state(int32_t arg, int32_t arg0, int32_t arg1) {
  extension_state_t arg2 = (extension_state_t) {
    (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) },
    arg1,
  };
  int32_t ret = extension_set_state(&arg2);
  return ret;
}
__attribute__((export_name("get-state")))
int32_t __wasm_export_extension_get_state(void) {
  extension_state_t ret;
  extension_get_state(&ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) ((ret).buffer).len;
  *((int32_t*)(ptr + 0)) = (int32_t) ((ret).buffer).ptr;
  *((int32_t*)(ptr + 8)) = (ret).idx;
  return ptr;
}
