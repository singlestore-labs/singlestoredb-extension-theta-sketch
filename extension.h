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
  extension_state_t extension_theta_sketch_handle_init(void);
  extension_state_t extension_theta_sketch_handle_clone(extension_state_t state);
  int32_t extension_theta_sketch_handle_destroy(extension_state_t state);
  extension_state_t extension_theta_sketch_handle_update(extension_state_t state, int32_t input);
  extension_state_t extension_theta_sketch_handle_union(extension_state_t left, extension_state_t right);
  extension_state_t extension_theta_sketch_handle_union_copy(extension_state_t left, extension_state_t right);
  extension_state_t extension_theta_sketch_handle_intersect(extension_state_t left, extension_state_t right);
  extension_state_t extension_theta_sketch_handle_intersect_copy(extension_state_t left, extension_state_t right);
  extension_state_t extension_theta_sketch_handle_anotb(extension_state_t left, extension_state_t right);
  extension_state_t extension_theta_sketch_handle_anotb_copy(extension_state_t left, extension_state_t right);
  void extension_theta_sketch_handle_serialize(extension_state_t s, extension_list_u8_t *ret0);
  extension_state_t extension_theta_sketch_handle_deserialize(extension_list_u8_t *data);
  double extension_theta_sketch_handle_estimate(extension_state_t s);
  void extension_theta_sketch_union(extension_list_u8_t *left, extension_list_u8_t *right, extension_list_u8_t *ret0);
  void extension_theta_sketch_intersect(extension_list_u8_t *left, extension_list_u8_t *right, extension_list_u8_t *ret0);
  void extension_theta_sketch_anotb(extension_list_u8_t *left, extension_list_u8_t *right, extension_list_u8_t *ret0);
  double extension_theta_sketch_estimate(extension_list_u8_t *data);
  #ifdef __cplusplus
}
#endif
#endif
