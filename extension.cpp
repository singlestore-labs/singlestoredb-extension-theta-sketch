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
void extension_state_free(extension_state_t *ptr) {
  canonical_abi_free(ptr->ptr, ptr->len * 1, 1);
}

__attribute__((aligned(4)))
static uint8_t RET_AREA[8];
__attribute__((export_name("sketch-init")))
int32_t __wasm_export_extension_sketch_init(void) {
  extension_state_t ret;
  extension_sketch_init(&ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("sketch-update")))
int32_t __wasm_export_extension_sketch_update(int32_t arg, int32_t arg0, int32_t arg1) {
  extension_state_t arg2 = (extension_state_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_state_t ret;
  extension_sketch_update(&arg2, arg1, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("sketch-union")))
int32_t __wasm_export_extension_sketch_union(int32_t arg, int32_t arg0, int32_t arg1, int32_t arg2) {
  extension_state_t arg3 = (extension_state_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_state_t arg4 = (extension_state_t) { (uint8_t*)(arg1), (size_t)(arg2) };
  extension_state_t ret;
  extension_sketch_union(&arg3, &arg4, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("sketch-intersect")))
int32_t __wasm_export_extension_sketch_intersect(int32_t arg, int32_t arg0, int32_t arg1, int32_t arg2) {
  extension_state_t arg3 = (extension_state_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_state_t arg4 = (extension_state_t) { (uint8_t*)(arg1), (size_t)(arg2) };
  extension_state_t ret;
  extension_sketch_intersect(&arg3, &arg4, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("sketch-anotb")))
int32_t __wasm_export_extension_sketch_anotb(int32_t arg, int32_t arg0, int32_t arg1, int32_t arg2) {
  extension_state_t arg3 = (extension_state_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_state_t arg4 = (extension_state_t) { (uint8_t*)(arg1), (size_t)(arg2) };
  extension_state_t ret;
  extension_sketch_anotb(&arg3, &arg4, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("sketch-finalize")))
int32_t __wasm_export_extension_sketch_finalize(int32_t arg, int32_t arg0) {
  extension_state_t arg1 = (extension_state_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_state_t ret;
  extension_sketch_finalize(&arg1, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("sketch-estimate")))
double __wasm_export_extension_sketch_estimate(int32_t arg, int32_t arg0) {
  extension_state_t arg1 = (extension_state_t) { (uint8_t*)(arg), (size_t)(arg0) };
  double ret = extension_sketch_estimate(&arg1);
  return ret;
}
__attribute__((export_name("sketch-print")))
int32_t __wasm_export_extension_sketch_print(int32_t arg, int32_t arg0) {
  extension_state_t arg1 = (extension_state_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_string_t ret;
  extension_sketch_print(&arg1, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
