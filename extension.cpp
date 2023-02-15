#include <extension.h>
#include <stdlib.h>

__attribute__((weak, export_name("canonical_abi_realloc"))) void *
canonical_abi_realloc(void *ptr, size_t orig_size, size_t org_align,
                      size_t new_size) {
  void *ret = realloc(ptr, new_size);
  if (!ret)
    abort();
  return ret;
}

__attribute__((weak, export_name("canonical_abi_free"))) void
canonical_abi_free(void *ptr, size_t size, size_t align) {
  free(ptr);
}
void extension_list_u8_free(extension_list_u8_t *ptr) {
  canonical_abi_free(ptr->ptr, ptr->len * 1, 1);
}

__attribute__((aligned(4))) static uint8_t RET_AREA[8];
__attribute__((export_name("sketch-init"))) int32_t
__wasm_export_extension_sketch_init(void) {
  extension_state_t ret = extension_sketch_init();
  return ret;
}
__attribute__((export_name("sketch-update"))) int32_t
__wasm_export_extension_sketch_update(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_sketch_update(arg, arg0);
  return ret;
}
__attribute__((export_name("sketch-union"))) int32_t
__wasm_export_extension_sketch_union(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_sketch_union(arg, arg0);
  return ret;
}
__attribute__((export_name("sketch-intersect"))) int32_t
__wasm_export_extension_sketch_intersect(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_sketch_intersect(arg, arg0);
  return ret;
}
__attribute__((export_name("sketch-anotb"))) int32_t
__wasm_export_extension_sketch_anotb(int32_t arg, int32_t arg0) {
  extension_state_t ret = extension_sketch_anotb(arg, arg0);
  return ret;
}
__attribute__((export_name("sketch-serialize"))) int32_t
__wasm_export_extension_sketch_serialize(int32_t arg) {
  extension_list_u8_t ret;
  extension_sketch_serialize(arg, &ret);
  int32_t ptr = (int32_t)&RET_AREA;
  *((int32_t *)(ptr + 4)) = (int32_t)(ret).len;
  *((int32_t *)(ptr + 0)) = (int32_t)(ret).ptr;
  return ptr;
}
__attribute__((export_name("sketch-deserialize"))) int32_t
__wasm_export_extension_sketch_deserialize(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 =
      (extension_list_u8_t){(uint8_t *)(arg), (size_t)(arg0)};
  extension_state_t ret = extension_sketch_deserialize(&arg1);
  return ret;
}
__attribute__((export_name("sketch-destroy"))) int32_t
__wasm_export_extension_sketch_destroy(int32_t arg) {
  int32_t ret = extension_sketch_destroy(arg);
  return ret;
}
__attribute__((export_name("sketch-estimate"))) double
__wasm_export_extension_sketch_estimate(int32_t arg, int32_t arg0) {
  extension_list_u8_t arg1 =
      (extension_list_u8_t){(uint8_t *)(arg), (size_t)(arg0)};
  double ret = extension_sketch_estimate(&arg1);
  return ret;
}
