#include <assert.h>
#include "extension.h"
#include "log.h"
#include "theta_a_not_b.hpp"
#include "theta_intersection.hpp"
#include "theta_sketch.hpp"
#include "theta_union.hpp"

using namespace datasketches;

typedef compact_theta_sketch (*merge_compacts_callback)(
    const compact_theta_sketch& a,
    const compact_theta_sketch& b);

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

  void merge(SketchState &other, merge_compacts_callback merge_compacts) {
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

  void copymerge(
    SketchState &left,
    SketchState &right,
    merge_compacts_callback merge_compacts)
  {
    assert(!m_compact);
    assert(!m_update);

    // Compact all sketches
    left.compact_sketch();
    right.compact_sketch();

    if (!left.m_compact)
    {
      if (right.m_compact)
      {
        m_compact = new compact_theta_sketch(*right.m_compact);
      }
    }
    else if (!right.m_compact)
    {
      if (left.m_compact)
      {
        m_compact = new compact_theta_sketch(*left.m_compact);
      }
    }
    else
    {
      assert(left.m_compact && right.m_compact);
      m_compact = new compact_theta_sketch(
        merge_compacts(*left.m_compact, *right.m_compact));
    }
  }

  SketchState* clone() const
  {
    auto *res = new SketchState{
      .m_update = m_update 
          ? new update_theta_sketch(*m_update)
          : nullptr,
      .m_compact = m_compact
          ? new compact_theta_sketch(*m_compact)
          : nullptr,
    };
    return res;
  }

  ~SketchState() {
    delete m_update;
    delete m_compact;
  }
};

//////////////////////////////////////////////////
// Aux functions
//////////////////////////////////////////////////

typedef void (*merge_callback)(SketchState& left, SketchState& right);
typedef void (*copymerge_callback)(SketchState& res, SketchState& left, SketchState& right);

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

extension_state_t join_sketches(
    extension_state_t left,
    extension_state_t right,
    merge_callback f)
{
  auto joined = with_sketch(left, [&](SketchState &left_sketch) {
    with_sketch(right, [&](SketchState &right_sketch) {
      // Updates the left sketch with the state of other sketch
      f(left_sketch, right_sketch);
    });
  });
  sketch_destroy(right);
  return joined;
}

extension_state_t join_sketches_copy(
    extension_state_t left,
    extension_state_t right,
    copymerge_callback f)
{
  SketchState* res = new SketchState{
      .m_update = nullptr,
      .m_compact = nullptr
  };
  with_sketch(left, [&](SketchState &left_sketch) {
    with_sketch(right, [&](SketchState &right_sketch) {
      // Updates the first sketch with a merge of the other two sketches.
      f(*res, left_sketch, right_sketch);
    });
  });
  return reinterpret_cast<extension_state_t>(res);
}

//////////////////////////////////////////////////
// Extension functions
//////////////////////////////////////////////////

extension_state_t extension_theta_sketch_handle_init()
{
  auto *sketch = new SketchState{
      .m_update =
          new update_theta_sketch(update_theta_sketch::builder().build()),
      .m_compact = nullptr,
  };
  auto handle = reinterpret_cast<extension_state_t>(sketch);
  DEBUG_LOG("[INIT] handle=%d\n", handle);
  return handle;
}

extension_state_t extension_theta_sketch_handle_clone(
    extension_state_t handle_src)
{
  DEBUG_LOG("[CLONE] Cloning handle=%d\n", handle_src);

  auto *sketch_src = reinterpret_cast<SketchState*>(handle_src);
  auto *sketch_dst = sketch_src->clone();

  const auto handle_dst = reinterpret_cast<extension_state_t>(sketch_dst);
  DEBUG_LOG("[CLONE] Cloning new handle=%d\n", handle_dst);

  return handle_dst;
}

int32_t extension_theta_sketch_handle_destroy(
    extension_state_t handle)
{
  DEBUG_LOG("[DESTROY] Destroying=%d\n", handle);
  sketch_destroy(handle);
  return 0;
}

extension_state_t extension_theta_sketch_handle_update(
    extension_state_t handle,
    int32_t input)
{
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

static compact_theta_sketch union_merge(
    const compact_theta_sketch& a,
    const compact_theta_sketch& b)
{
    auto combined = theta_union::builder().build();
    combined.update(a);
    combined.update(b);
    return combined.get_result();
}

extension_state_t extension_theta_sketch_handle_union(
    extension_state_t left,
    extension_state_t right)
{
  DEBUG_LOG("[UNION] left=%d right=%d\n", left, right);
  return join_sketches(
    left,
    right,
    [](SketchState &left_sketch, SketchState &right_sketch) {
      left_sketch.merge(right_sketch, union_merge);
    });
}

extension_state_t extension_theta_sketch_handle_union_copy(
    extension_state_t left,
    extension_state_t right)
{
  DEBUG_LOG("[UNION COPY] left=%d right=%d\n", left, right);
  return join_sketches_copy(
    left,
    right,
    [](SketchState &new_sketch, SketchState &left_sketch, SketchState &right_sketch)
    {
      new_sketch.copymerge(left_sketch, right_sketch, union_merge);
    });
}

static compact_theta_sketch intersect_merge(
    const compact_theta_sketch& a,
    const compact_theta_sketch& b)
{
    auto combined = theta_union::builder().build();
    combined.update(a);
    combined.update(b);
    return combined.get_result();
}

extension_state_t extension_theta_sketch_handle_intersect(
    extension_state_t left,
    extension_state_t right)
{
  DEBUG_LOG("[INTERSECT] left=%d right=%d\n", left, right);
  return join_sketches(
    left,
    right,
    [](SketchState &left_sketch, SketchState &right_sketch)
    {
      left_sketch.merge(right_sketch, intersect_merge);
    });
}

extension_state_t extension_theta_sketch_handle_intersect_copy(
    extension_state_t left,
    extension_state_t right)
{
  DEBUG_LOG("[INTERSECT COPY] left=%d right=%d\n", left, right);
  return join_sketches_copy(
    left,
    right,
    [](SketchState &new_sketch, SketchState &left_sketch, SketchState &right_sketch)
    {
      new_sketch.copymerge(left_sketch, right_sketch, intersect_merge);
    });
}

static compact_theta_sketch anotb_merge(
    const compact_theta_sketch& a,
    const compact_theta_sketch& b)
{
    return theta_a_not_b().compute(a, b);
}

extension_state_t extension_theta_sketch_handle_anotb(
    extension_state_t left,
    extension_state_t right)
{
  DEBUG_LOG("[ANOTB] left=%d right=%d\n", left, right);
  return join_sketches(
    left,
    right,
    [](SketchState &left_sketch, SketchState &right_sketch)
    {
      left_sketch.merge(right_sketch, anotb_merge);
    });
}

extension_state_t extension_theta_sketch_handle_anotb_copy(
    extension_state_t left,
    extension_state_t right)
{
  DEBUG_LOG("[ANOTB COPY] left=%d right=%d\n", left, right);
  return join_sketches_copy(
    left,
    right,
    [](SketchState &new_sketch, SketchState &left_sketch, SketchState &right_sketch)
    {
      new_sketch.copymerge(left_sketch, right_sketch, anotb_merge);
    });
}

void extension_theta_sketch_handle_serialize(
    extension_state_t handle,
    extension_list_u8_t *data)
{
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
extension_theta_sketch_handle_deserialize(
    extension_list_u8_t *data)
{
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

double extension_theta_sketch_handle_estimate(
    extension_state_t handle) {
  double estimate;
  with_sketch(handle, [&](SketchState &sketch) {
    sketch.compact_sketch();
    estimate = sketch.m_compact->get_estimate();
  });
  DEBUG_LOG("[ESTIMATE] estimate=%f\n", estimate);
  sketch_destroy(handle);
  return estimate;
}

