#include "extension.h"
#include "log.h"
#include "theta_a_not_b.hpp"
#include "theta_intersection.hpp"
#include "theta_sketch.hpp"
#include "theta_union.hpp"

using namespace datasketches;

//////////////////////////////////////////////////
// Aux functions
//////////////////////////////////////////////////

wrapped_compact_theta_sketch as_sketch(extension_list_u8_t *data) {
  return wrapped_compact_theta_sketch::wrap(data->ptr, data->len);
}

template <typename F>
void join_sketches(extension_list_u8_t *left, extension_list_u8_t *right, F &&f,
                   extension_list_u8_t &data) {
  auto left_sketch = as_sketch(left);
  auto right_sketch = as_sketch(right);
  auto *combined =
      new std::vector<uint8_t>(f(left_sketch, right_sketch).serialize());
  data.ptr = combined->data();
  data.len = combined->size();
  extension_list_u8_free(left);
  extension_list_u8_free(right);
}

//////////////////////////////////////////////////
// Value functions
//////////////////////////////////////////////////

void extension_sketch_union(extension_list_u8_t *left,
                            extension_list_u8_t *right,
                            extension_list_u8_t *data) {
  DEBUG_LOG("[UNION] left=%p right=%p\n", left->ptr, right->ptr);
  join_sketches(
      left, right,
      [](auto &left_sketch, auto &right_sketch) {
        auto sketch = theta_union::builder().build();
        sketch.update(left_sketch);
        sketch.update(right_sketch);
        return sketch.get_result();
      },
      *data);
}

void extension_sketch_intersect(extension_list_u8_t *left,
                                extension_list_u8_t *right,
                                extension_list_u8_t *data) {
  DEBUG_LOG("[INTERSECT] left=%p right=%p\n", left->ptr, right->ptr);
  join_sketches(
      left, right,
      [](auto &left_sketch, auto &right_sketch) {
        auto sketch = theta_intersection();
        sketch.update(left_sketch);
        sketch.update(right_sketch);
        return sketch.get_result();
      },
      *data);
}

void extension_sketch_anotb(extension_list_u8_t *left,
                            extension_list_u8_t *right,
                            extension_list_u8_t *data) {
  DEBUG_LOG("[ANOTB] left=%p right=%p\n", left->ptr, right->ptr);
  join_sketches(
      left, right,
      [](auto &left_sketch, auto &right_sketch) {
        theta_a_not_b sketch;
        return sketch.compute(left_sketch, right_sketch);
      },
      *data);
}

double extension_sketch_estimate(extension_list_u8_t *data) {
  const auto estimate = as_sketch(data).get_estimate();
  extension_list_u8_free(data);
  DEBUG_LOG("[ESTIMATE] estimate=%f\n", estimate);
  return estimate;
}