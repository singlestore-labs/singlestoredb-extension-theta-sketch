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

void sketch_destroy(extension_state_t handle) {
  DEBUG_LOG("[DESTROY] handle=%d\n", handle);
  auto *sketch = reinterpret_cast<update_theta_sketch *>(handle);
  delete sketch;
}

template <typename F>
extension_state_t with_sketch(extension_state_t handle, F &&f) {
  update_theta_sketch *sketch = reinterpret_cast<update_theta_sketch *>(handle);
  f(*sketch);
  return reinterpret_cast<extension_state_t>(sketch);
}

template <typename F>
extension_state_t join_sketches(extension_state_t left, extension_state_t right,
                                F &&f) {
  auto joined = with_sketch(left, [&](update_theta_sketch &left_sketch) {
    with_sketch(right, [&](update_theta_sketch &right_sketch) {
      auto left_compact = left_sketch.compact();
      auto right_compact = right_sketch.compact();
      auto combined = f(left_compact, right_compact);
      left_sketch = update_theta_sketch::builder().build();
      for (const uint64_t val : combined) {
        left_sketch.update_hash(val);
      }
    });
  });
  sketch_destroy(right);
  return joined;
}

//////////////////////////////////////////////////
// Extension functions
//////////////////////////////////////////////////

extension_state_t extension_sketch_init_handle() {
  auto *sketch =
      new update_theta_sketch(update_theta_sketch::builder().build());
  extension_state_t handle = reinterpret_cast<extension_state_t>(sketch);
  DEBUG_LOG("[INIT] handle=%d\n", handle);
  return handle;
}

extension_state_t extension_sketch_update_handle(extension_state_t handle,
                                                 int32_t input) {
  DEBUG_LOG("[UPDATE] Updating handle=%d val=%d\n", handle, input);
  return with_sketch(
      handle, [input](update_theta_sketch &sketch) { sketch.update(input); });
}

extension_state_t extension_sketch_union_handle(extension_state_t left,
                                                extension_state_t right) {
  DEBUG_LOG("[UNION] left=%d right=%d\n", left, right);
  return join_sketches(left, right, [](auto &left_sketch, auto &right_sketch) {
    auto sketch = theta_union::builder().build();
    sketch.update(left_sketch);
    sketch.update(right_sketch);
    return sketch.get_result();
  });
}

extension_state_t extension_sketch_intersect_handle(extension_state_t left,
                                                    extension_state_t right) {
  DEBUG_LOG("[INTERSECT] left=%d right=%d\n", left, right);
  return join_sketches(left, right, [](auto &left_sketch, auto &right_sketch) {
    auto sketch = theta_intersection();
    sketch.update(left_sketch);
    sketch.update(right_sketch);
    return sketch.get_result();
  });
}

extension_state_t extension_sketch_anotb_handle(extension_state_t left,
                                                extension_state_t right) {
  DEBUG_LOG("[ANOTB] left=%d right=%d\n", left, right);
  return join_sketches(left, right, [](auto &left_sketch, auto &right_sketch) {
    theta_a_not_b sketch;
    return sketch.compute(left_sketch, right_sketch);
  });
}

void extension_sketch_serialize_handle(extension_state_t handle,
                                       extension_list_u8_t *data) {
  DEBUG_LOG("[SERIALIZE] handle=%d\n", handle);
  with_sketch(handle, [&](update_theta_sketch &sketch) {
    auto compact = sketch.compact();
    auto *sk = new std::vector<uint8_t>(compact.serialize());
    data->ptr = reinterpret_cast<unsigned char *>(sk->data());
    data->len = sk->size();
    DEBUG_LOG("[SERIALIZE] data=%p len=%zu\n", data->ptr, data->len);
  });
  sketch_destroy(handle);
}

extension_state_t
extension_sketch_deserialize_handle(extension_list_u8_t *data) {
  auto compact_sketch = compact_theta_sketch::deserialize(data->ptr, data->len);
  extension_list_u8_free(data);
  auto *sketch =
      new update_theta_sketch(update_theta_sketch::builder().build());
  for (const uint64_t val : compact_sketch) {
    sketch->update_hash(val);
  }
  const auto handle = reinterpret_cast<extension_state_t>(sketch);
  DEBUG_LOG("[DESERIALIZE] handle=%d\n", handle);
  return handle;
}

double extension_sketch_estimate_handle(extension_list_u8_t *data) {
  const auto estimate =
      wrapped_compact_theta_sketch::wrap(data->ptr, data->len).get_estimate();
  extension_list_u8_free(data);
  DEBUG_LOG("[ESTIMATE] estimate=%f\n", estimate);
  return estimate;
}