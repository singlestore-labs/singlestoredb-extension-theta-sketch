#include "extension.h"
#include "log.h"
#include "theta_a_not_b.hpp"
#include "theta_intersection.hpp"
#include "theta_sketch.hpp"
#include "theta_union.hpp"

using namespace datasketches;

struct SketchState {
  // The in-progress sketch
  update_theta_sketch *m_update;

  // The previously accumulated sketch state
  compact_theta_sketch *m_compact;

  // join the `m_update` sketch into m_compact by `UNION`.
  void compact_sketch() {
    if (m_update != nullptr) {
      if (m_compact == nullptr) {
        m_compact = new compact_theta_sketch(m_update->compact());
      } else {
        auto combined = theta_union::builder().build();
        combined.update(*m_compact);
        delete m_compact;
        combined.update(*m_update);
        m_compact = new compact_theta_sketch(combined.get_result());
      }
      delete m_update;
      m_update = nullptr;
    }
  }

  template <typename F> void merge(SketchState &other, F &&merge_compacts) {
    // Compact both sketches
    this->compact_sketch();
    other.compact_sketch();

    if (m_compact == nullptr) {
      m_compact = other.m_compact;
      other.m_compact = nullptr;
    } else if (other.m_compact != nullptr) {

      auto combined = merge_compacts(*m_compact, *other.m_compact);
      delete m_compact;
      delete other.m_compact;
      other.m_compact = nullptr;

      m_compact = new compact_theta_sketch(combined);
    }
  }

  ~SketchState() {
    delete m_update;
    delete m_compact;
  }
};

//////////////////////////////////////////////////
// Aux functions
//////////////////////////////////////////////////

void sketch_destroy(extension_state_t handle) {
  DEBUG_LOG("[DESTROY] handle=%d\n", handle);
  auto *sketch = reinterpret_cast<SketchState *>(handle);
  delete sketch;
}

template <typename F>
extension_state_t with_sketch(extension_state_t handle, F &&f) {
  auto *sketch = reinterpret_cast<SketchState *>(handle);
  f(*sketch);
  return reinterpret_cast<extension_state_t>(sketch);
}

template <typename F>
extension_state_t join_sketches(extension_state_t left, extension_state_t right,
                                F &&f) {
  auto joined = with_sketch(left, [&](SketchState &left_sketch) {
    with_sketch(right, [&](SketchState &right_sketch) {
      // Updates the left sketch with the state of other sketch
      f(left_sketch, right_sketch);
    });
  });
  sketch_destroy(right);
  return joined;
}

//////////////////////////////////////////////////
// Extension functions
//////////////////////////////////////////////////

extension_state_t extension_sketch_init_handle() {
  auto *sketch = new SketchState{
      .m_update =
          new update_theta_sketch(update_theta_sketch::builder().build()),
      .m_compact = nullptr,
  };
  auto handle = reinterpret_cast<extension_state_t>(sketch);
  DEBUG_LOG("[INIT] handle=%d\n", handle);
  return handle;
}

extension_state_t extension_sketch_update_handle(extension_state_t handle,
                                                 int32_t input) {
  DEBUG_LOG("[UPDATE] Updating handle=%d val=%d\n", handle, input);
  return with_sketch(
      handle, [input](SketchState &sketch) {
        if (sketch.m_update == nullptr) {
          sketch.m_update =
              new update_theta_sketch(update_theta_sketch::builder().build());
        }
        sketch.m_update->update(input);
      });
}

extension_state_t extension_sketch_union_handle(extension_state_t left,
                                                extension_state_t right) {
  DEBUG_LOG("[UNION] left=%d right=%d\n", left, right);
  return join_sketches(left, right,
                       [](SketchState &left_sketch, SketchState &right_sketch) {
                         left_sketch.merge(right_sketch, [](auto &a, auto &b) {
                           auto combined = theta_union::builder().build();
                           combined.update(a);
                           combined.update(b);
                           return combined.get_result();
                         });
                       });
}

extension_state_t extension_sketch_intersect_handle(extension_state_t left,
                                                    extension_state_t right) {
  DEBUG_LOG("[INTERSECT] left=%d right=%d\n", left, right);
  return join_sketches(left, right,
                       [](SketchState &left_sketch, SketchState &right_sketch) {
                         left_sketch.merge(right_sketch, [](auto &a, auto &b) {
                           auto combined = theta_intersection();
                           combined.update(a);
                           combined.update(b);
                           return combined.get_result();
                         });
                       });
}

extension_state_t extension_sketch_anotb_handle(extension_state_t left,
                                                extension_state_t right) {
  DEBUG_LOG("[ANOTB] left=%d right=%d\n", left, right);
  return join_sketches(left, right,
                       [](SketchState &left_sketch, SketchState &right_sketch) {
                         left_sketch.merge(right_sketch, [](auto &a, auto &b) {
                           return theta_a_not_b().compute(a, b);
                         });
                       });
}

void extension_sketch_serialize_handle(extension_state_t handle,
                                       extension_list_u8_t *data) {
  DEBUG_LOG("[SERIALIZE] handle=%d\n", handle);
  with_sketch(handle, [&](SketchState &sketch) {
    sketch.compact_sketch();
    auto *sk = new std::vector<uint8_t>(sketch.m_compact->serialize());
    data->ptr = reinterpret_cast<unsigned char *>(sk->data());
    data->len = sk->size();
    DEBUG_LOG("[SERIALIZE] data=%p len=%zu\n", data->ptr, data->len);
  });
  sketch_destroy(handle);
}

extension_state_t
extension_sketch_deserialize_handle(extension_list_u8_t *data) {
  DEBUG_LOG("[DESERIALIZE] ptr=%p len=%zu\n", data->ptr, data->len);
  auto compact_sketch = compact_theta_sketch::deserialize(data->ptr, data->len);
  extension_list_u8_free(data);
  auto *sketch = new SketchState{
      .m_update = nullptr,
      .m_compact = new compact_theta_sketch(compact_sketch),
  };
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