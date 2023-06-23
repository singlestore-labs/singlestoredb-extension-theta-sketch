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
__attribute__((export_name("theta-sketch-handle-init")))
int32_t __wasm_export_extension_theta_sketch_handle_init(void) {
  extension_state_t ret = extension_theta_sketch_handle_init();
  return ret;
}
__attribute__((export_name("theta-sketch-handle-clone")))
int32_t __wasm_export_extension_theta_sketch_handle_clone(int32_t arg) {
  extension_state_t ret = extension_theta_sketch_handle_clone(arg);
  return ret;
}
__attribute__((export_name("theta-sketch-handle-destroy")))
int32_t __wasm_export_extension_theta_sketch_handle_destroy(int32_t arg) {
  int32_t ret = extension_theta_sketch_handle_destroy(arg);
  return ret;
}
__attribute__((export_name("theta-sketch-handle-update")))
int32_t __wasm_export_extension_theta_sketch_handle_update(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_theta_sketch_handle_update(arg, arg0);
  return ret;
}
__attribute__((export_name("theta-sketch-handle-union")))
int32_t __wasm_export_extension_theta_sketch_handle_union(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_theta_sketch_handle_union(arg, arg0);
  return ret;
}
__attribute__((export_name("theta-sketch-handle-union-copy")))
int32_t __wasm_export_extension_theta_sketch_handle_union_copy(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_theta_sketch_handle_union_copy(arg, arg0);
  return ret;
}
__attribute__((export_name("theta-sketch-handle-intersect")))
int32_t __wasm_export_extension_theta_sketch_handle_intersect(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_theta_sketch_handle_intersect(arg, arg0);
  return ret;
}
__attribute__((export_name("theta-sketch-handle-intersect-copy")))
int32_t __wasm_export_extension_theta_sketch_handle_intersect_copy(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_theta_sketch_handle_intersect_copy(arg, arg0);
  return ret;
}
__attribute__((export_name("theta-sketch-handle-anotb")))
int32_t __wasm_export_extension_theta_sketch_handle_anotb(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_theta_sketch_handle_anotb(arg, arg0);
  return ret;
}
__attribute__((export_name("theta-sketch-handle-anotb-copy")))
int32_t __wasm_export_extension_theta_sketch_handle_anotb_copy(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_theta_sketch_handle_anotb_copy(arg, arg0);
  return ret;
}
__attribute__((export_name("theta-sketch-handle-serialize")))
int32_t __wasm_export_extension_theta_sketch_handle_serialize(int32_t arg) {
  extension_list_u8_t ret;
  extension_theta_sketch_handle_serialize(arg, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("theta-sketch-handle-deserialize")))
int32_t __wasm_export_extension_theta_sketch_handle_deserialize(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_state_t ret = extension_theta_sketch_handle_deserialize(&arg1);
  return ret;
}
__attribute__((export_name("theta-sketch-handle-estimate")))
double __wasm_export_extension_theta_sketch_handle_estimate(int32_t arg) {
  double ret = extension_theta_sketch_handle_estimate(arg);
  return ret;
}
__attribute__((export_name("theta-sketch-union")))
int32_t __wasm_export_extension_theta_sketch_union(int32_t arg, int32_t arg0, int32_t arg1, int32_t arg2) {
  extension_list_u8_t arg3 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_list_u8_t arg4 = (extension_list_u8_t) { (uint8_t*)(arg1), (size_t)(arg2) };
  extension_list_u8_t ret;
  extension_theta_sketch_union(&arg3, &arg4, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("theta-sketch-intersect")))
int32_t __wasm_export_extension_theta_sketch_intersect(int32_t arg, int32_t arg0, int32_t arg1, int32_t arg2) {
  extension_list_u8_t arg3 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_list_u8_t arg4 = (extension_list_u8_t) { (uint8_t*)(arg1), (size_t)(arg2) };
  extension_list_u8_t ret;
  extension_theta_sketch_intersect(&arg3, &arg4, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("theta-sketch-anotb")))
int32_t __wasm_export_extension_theta_sketch_anotb(int32_t arg, int32_t arg0, int32_t arg1, int32_t arg2) {
  extension_list_u8_t arg3 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  extension_list_u8_t arg4 = (extension_list_u8_t) { (uint8_t*)(arg1), (size_t)(arg2) };
  extension_list_u8_t ret;
  extension_theta_sketch_anotb(&arg3, &arg4, &ret);
  int32_t ptr = (int32_t) &RET_AREA;
  *((int32_t*)(ptr + 4)) = (int32_t) (ret).len;
  *((int32_t*)(ptr + 0)) = (int32_t) (ret).ptr;
  return ptr;
}
__attribute__((export_name("theta-sketch-estimate")))
double __wasm_export_extension_theta_sketch_estimate(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 = (extension_list_u8_t) { (uint8_t*)(arg), (size_t)(arg0) };
  double ret = extension_theta_sketch_estimate(&arg1);
  return ret;
}
