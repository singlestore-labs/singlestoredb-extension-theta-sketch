#include <cstdio>
#include <string>
#include "log.h"
#include "extension.h"
#include "theta_a_not_b.hpp"
#include "theta_sketch.hpp"
#include "theta_union.hpp"
#include "theta_intersection.hpp"

using namespace datasketches;

/// Size of the header
/// Header contains
const size_t PREFIX_LEN = 4;

struct WasmState {
    uint32_t generation;
    update_theta_sketch* sketch;

    WasmState(): generation(0), sketch(nullptr) {}
};

/// Per-partition state retained between `update` calls
static WasmState WASM_STATE = WasmState();

inline uint32_t generation_of(extension_state_t* state) {
    return (state && state->ptr) ? *reinterpret_cast<uint32_t*>(state->ptr) : 0;
}

template<typename Sketch>
void serialize_state(extension_state_t *state, Sketch sketch) {
    auto* sk = new std::vector<uint8_t>(sketch.serialize(PREFIX_LEN));
    state->ptr = reinterpret_cast<unsigned char *>(sk->data());
    state->len = sk->size();
    *reinterpret_cast<uint32_t*>(state->ptr) = WASM_STATE.generation;
}

compact_theta_sketch deserialize_state(extension_state_t* state) {
    return compact_theta_sketch::deserialize(state->ptr + PREFIX_LEN, state->len - PREFIX_LEN);
}

update_theta_sketch* try_get_state(extension_state_t* state) {
    const auto generation = generation_of(state);
    if (generation != WASM_STATE.generation || WASM_STATE.sketch == nullptr) {
        // This is a non sequential continuation, so we'll need to refresh the state
        WASM_STATE.sketch = new update_theta_sketch(update_theta_sketch::builder().build());
        auto compact = deserialize_state(state);
        for(const uint64_t item : compact) {
            WASM_STATE.sketch->update_hash(item);
        }
        WASM_STATE.generation = generation;
    }

    return WASM_STATE.sketch;
}

void extension_sketch_init(extension_state_t *state) {
    WASM_STATE.sketch = new update_theta_sketch(update_theta_sketch::builder().build());
    WASM_STATE.generation = 0;
    serialize_state(state, WASM_STATE.sketch->compact());
    DEBUG_LOG("[INIT] Ptr=%s Size=%zu\n", state->ptr, state->len);
}

void extension_sketch_update(extension_state_t *s, int32_t input, extension_state_t *state) {
    auto* sketch = try_get_state(s);
    sketch->update(input);
    // TODO: Optimize for single entry extension
    serialize_state(state, sketch->compact());
    extension_state_free(s);
    DEBUG_LOG("[UPDATE] Updating state as ptr=%s size=%zu\n", state->ptr, state->len);
}

void extension_sketch_union(extension_state_t *left, extension_state_t *right, extension_state_t *state) {
    const auto left_compact_sketch = deserialize_state(left);
    const auto right_compact_sketch = deserialize_state(right);
    auto sketch = theta_union::builder().build();
    sketch.update(left_compact_sketch);
    sketch.update(right_compact_sketch);
    WASM_STATE.generation = std::max(generation_of(left), generation_of(right)) + 1;
    serialize_state(state, sketch.get_result());

    extension_state_free(left);
    extension_state_free(right);
    DEBUG_LOG("[UNION] Ptr=%s Size=%zu\n", state->ptr, state->len);
}

void extension_sketch_intersect(extension_state_t *left, extension_state_t *right, extension_state_t *state) {
    const auto left_compact_sketch = deserialize_state(left);
    const auto right_compact_sketch = deserialize_state(right);
    auto sketch = theta_intersection();
    sketch.update(left_compact_sketch);
    sketch.update(right_compact_sketch);
    WASM_STATE.generation = std::max(generation_of(left), generation_of(right)) + 1;
    serialize_state(state, sketch.get_result());

    extension_state_free(left);
    extension_state_free(right);
    DEBUG_LOG("[INTERSECT] Ptr=%s Size=%zu\n", state->ptr, state->len);
}

void extension_sketch_anotb(extension_state_t *left, extension_state_t *right, extension_state_t *state) {
    const auto left_compact_sketch = deserialize_state(left);
    const auto right_compact_sketch = deserialize_state(right);
    theta_a_not_b sketch;
    auto result = sketch.compute(left_compact_sketch, right_compact_sketch);
    WASM_STATE.generation = std::max(generation_of(left), generation_of(right)) + 1;
    serialize_state(state, result);

    extension_state_free(left);
    extension_state_free(right);
    DEBUG_LOG("[ANOTB] Ptr=%s Size=%zu\n", state->ptr, state->len);
}

void extension_sketch_finalize(extension_state_t *s, extension_state_t *state) {
    // Note: No need to copy & free input buffer. Just swap fat ptrs;
    state->ptr = s->ptr;
    state->len = s->len;
}

double extension_sketch_estimate(extension_state_t *s) {
    const auto estimate = wrapped_compact_theta_sketch::wrap(s->ptr + PREFIX_LEN, s->len - PREFIX_LEN).get_estimate();
    extension_state_free(s);
    return estimate;
}

void extension_sketch_print(extension_state_t *s, extension_string_t *ret0) {
    const auto sketch = wrapped_compact_theta_sketch::wrap(s->ptr + PREFIX_LEN, s->len - PREFIX_LEN);
    std::ostringstream os;
    os << "### Theta sketch summary:" << std::endl;
    os << "   num retained entries : " << sketch.get_num_retained() << std::endl;
    os << "   seed hash            : " << sketch.get_seed_hash() << std::endl;
    os << "   empty?               : " << (sketch.is_empty() ? "true" : "false") << std::endl;
    os << "   ordered?             : " << (sketch.is_ordered() ? "true" : "false") << std::endl;
    os << "   theta (fraction)     : " << sketch.get_theta() << std::endl;
    os << "   estimate             : " << sketch.get_estimate() << std::endl;
    extension_string_dup(ret0, os.str().c_str());
    extension_state_free(s);
}
