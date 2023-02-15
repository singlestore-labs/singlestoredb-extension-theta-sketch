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
  extension_state_t extension_sketch_init(void);
  extension_state_t extension_sketch_update(extension_state_t state, int32_t input);
  extension_state_t extension_sketch_union(extension_state_t left, extension_state_t right);
  extension_state_t extension_sketch_intersect(extension_state_t left, extension_state_t right);
  extension_state_t extension_sketch_anotb(extension_state_t left, extension_state_t right);
  void extension_sketch_serialize(extension_state_t s, extension_list_u8_t *ret0);
  extension_state_t extension_sketch_deserialize(extension_list_u8_t *data);
  int32_t extension_sketch_destroy(extension_state_t s);
  double extension_sketch_estimate(extension_list_u8_t *data);
  #ifdef __cplusplus
}
#endif
#endif
