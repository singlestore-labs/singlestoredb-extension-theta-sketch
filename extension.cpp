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
void extension_list_u8_free(extension_list_u8_t *ptr) {
  canonical_abi_free(ptr->ptr, ptr->len * 1, 1);
}

__attribute__((aligned(4)))
static uint8_t RET_AREA[8];
__attribute__((export_name("sketch-init-handle")))
int32_t __wasm_export_extension_sketch_init_handle(void) {
  extension_state_t ret = extension_sketch_init_handle();
  return ret;
}
__attribute__((export_name("sketch-update-handle")))
int32_t __wasm_export_extension_sketch_update_handle(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_sketch_update_handle(arg, arg0);
  return ret;
}
__attribute__((export_name("sketch-union-handle")))
int32_t __wasm_export_extension_sketch_union_handle(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_sketch_union_handle(arg, arg0);
  return ret;
}
__attribute__((export_name("sketch-intersect-handle")))
int32_t __wasm_export_extension_sketch_intersect_handle(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_sketch_intersect_handle(arg, arg0);
  return ret;
}
__attribute__((export_name("sketch-anotb-handle")))
int32_t __wasm_export_extension_sketch_anotb_handle(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_sketch_anotb_handle(arg, arg0);
  return ret;
}
__attribute__((export_name("sketch-serialize-handle")))
int32_t __wasm_export_extension_sketch_serialize_handle(int32_t arg) {
  extension_list_u8_t ret;
  extension_sketch_serialize_handle(arg, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("sketch-deserialize-handle")))
int32_t __wasm_export_extension_sketch_deserialize_handle(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_state_t ret = extension_sketch_deserialize_handle(&arg1);
  return ret;
}
__attribute__((export_name("sketch-estimate-handle")))
double __wasm_export_extension_sketch_estimate_handle(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  double ret = extension_sketch_estimate_handle(&arg1);
  return ret;
}
__attribute__((export_name("sketch-union")))
int32_t __wasm_export_extension_sketch_union(int32_t arg, int32_t arg0, int32_t arg1, int32_t arg2) {
  extension_list_u8_t arg3 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_list_u8_t arg4 = (extension_list_u8_t) { (uint8_t*)(arg1), (size_t)(arg2) };
  extension_list_u8_t ret;
  extension_sketch_union(&arg3, &arg4, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("sketch-intersect")))
int32_t __wasm_export_extension_sketch_intersect(int32_t arg, int32_t arg0, int32_t arg1, int32_t arg2) {
  extension_list_u8_t arg3 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_list_u8_t arg4 = (extension_list_u8_t) { (uint8_t*)(arg1), (size_t)(arg2) };
  extension_list_u8_t ret;
  extension_sketch_intersect(&arg3, &arg4, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("sketch-anotb")))
int32_t __wasm_export_extension_sketch_anotb(int32_t arg, int32_t arg0, int32_t arg1, int32_t arg2) {
  extension_list_u8_t arg3 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_list_u8_t arg4 = (extension_list_u8_t) { (uint8_t*)(arg1), (size_t)(arg2) };
  extension_list_u8_t ret;
  extension_sketch_anotb(&arg3, &arg4, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("sketch-estimate")))
double __wasm_export_extension_sketch_estimate(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  double ret = extension_sketch_estimate(&arg1);
  return ret;
}
