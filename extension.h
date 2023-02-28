#ifndef __BINDINGS_EXTENSION_H
#define __BINDINGS_EXTENSION_H
#ifdef __cplusplus
extern "C"
{
  #endif
  
  #include <stdint.h>
  #include <stdbool.h>
  typedef int32_t extension_state_t;
  typedef struct {
    uint8_t *ptr;
    size_t len;
  } extension_list_u8_t;
  void extension_list_u8_free(extension_list_u8_t *ptr);
  extension_state_t extension_sketch_init_handle(void);
  extension_state_t extension_sketch_update_handle(extension_state_t state, int32_t input);
  extension_state_t extension_sketch_union_handle(extension_state_t left, extension_state_t right);
  extension_state_t extension_sketch_intersect_handle(extension_state_t left, extension_state_t right);
  extension_state_t extension_sketch_anotb_handle(extension_state_t left, extension_state_t right);
  void extension_sketch_serialize_handle(extension_state_t s, extension_list_u8_t *ret0);
  extension_state_t extension_sketch_deserialize_handle(extension_list_u8_t *data);
  double extension_sketch_estimate_handle(extension_list_u8_t *data);
  void extension_sketch_union(extension_list_u8_t *left, extension_list_u8_t *right, extension_list_u8_t *ret0);
  void extension_sketch_intersect(extension_list_u8_t *left, extension_list_u8_t *right, extension_list_u8_t *ret0);
  void extension_sketch_anotb(extension_list_u8_t *left, extension_list_u8_t *right, extension_list_u8_t *ret0);
  double extension_sketch_estimate(extension_list_u8_t *data);
  #ifdef __cplusplus
}
#endif
#endif
