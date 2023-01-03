#ifndef __BINDINGS_EXTENSION_H
#define __BINDINGS_EXTENSION_H
#ifdef __cplusplus
extern "C"
{
  #endif
  
  #include <stdint.h>
  #include <stdbool.h>
  
  typedef struct {
    char *ptr;
    size_t len;
  } extension_string_t;
  
  void extension_string_set(extension_string_t *ret, const char *s);
  void extension_string_dup(extension_string_t *ret, const char *s);
  void extension_string_free(extension_string_t *ret);
  typedef struct {
    uint8_t *ptr;
    size_t len;
  } extension_state_t;
  void extension_state_free(extension_state_t *ptr);
  void extension_sketch_init(extension_state_t *ret0);
  void extension_sketch_update(extension_state_t *s, int32_t input, extension_state_t *ret0);
  void extension_sketch_union(extension_state_t *left, extension_state_t *right, extension_state_t *ret0);
  void extension_sketch_intersect(extension_state_t *left, extension_state_t *right, extension_state_t *ret0);
  void extension_sketch_anotb(extension_state_t *left, extension_state_t *right, extension_state_t *ret0);
  void extension_sketch_finalize(extension_state_t *s, extension_state_t *ret0);
  double extension_sketch_estimate(extension_state_t *s);
  void extension_sketch_print(extension_state_t *s, extension_string_t *ret0);
  #ifdef __cplusplus
}
#endif
#endif
