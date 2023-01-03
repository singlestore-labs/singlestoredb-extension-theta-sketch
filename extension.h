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
  } extension_list_u8_t;
  void extension_list_u8_free(extension_list_u8_t *ptr);
  typedef struct {
    extension_list_u8_t buffer;
    int32_t idx;
  } extension_state_t;
  void extension_state_free(extension_state_t *ptr);
  void extension_greet(extension_string_t *name, extension_string_t *ret0);
  int32_t extension_answer_to_life(void);
  int32_t extension_set_state(extension_state_t *s);
  void extension_get_state(extension_state_t *ret0);
  #ifdef __cplusplus
}
#endif
#endif
