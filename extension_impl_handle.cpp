#ifdef NOTWASM
#  include "extension-nowasm.h"
#else
#  include "extension.h"
#endif
#include "log.h"
#include "theta_a_not_b.hpp"
#include "theta_intersection.hpp"
#include "theta_sketch.hpp"
#include "theta_union.hpp"
#include "theta_constants.hpp"
#include "MurmurHash3.h"
#include <unistd.h>
#include <assert.h>

using namespace datasketches;

#define NO_HANDLE ((extension_state_t) -1)
#define TO_HND(x_) (reinterpret_cast<extension_state_t>(x_))
#define TO_PTR(x_) (reinterpret_cast<void*>(x_))
#define VALID_HANDLE(s_) (TO_HND(s_) != NO_HANDLE)
#define BAIL(...) { fprintf(stderr, __VA_ARGS__); abort(); }
#define BAIL_IF(cond_, ...) { if (cond_) { BAIL(__VA_ARGS__); } }

using s2_theta_sketch = theta_sketch;
using s2_update_theta_sketch = update_theta_sketch;
using s2_compact_theta_sketch = compact_theta_sketch;
using s2_wrapped_compact_theta_sketch = wrapped_compact_theta_sketch;
using s2_theta_union = theta_union;
using s2_theta_intersection = theta_intersection;

#define theta_sketch                 s2_theta_sketch
#define update_theta_sketch          s2_update_theta_sketch
#define compact_theta_sketch         s2_compact_theta_sketch
#define wrapped_compact_theta_sketch s2_wrapped_compact_theta_sketch
#define theta_union                  s2_theta_union
#define theta_intersection           s2_theta_intersection

static void theta_data_set_type(void* dataptr, agg_state_type t)
{
    static_cast<theta_data*>(dataptr)->set_type(t);
}

static agg_state_type theta_data_get_type(const void* dataptr)
{
    return static_cast<const theta_data*>(dataptr)->get_type();
}

static void* theta_sketch_new_default()
{
    auto res = new update_theta_sketch(update_theta_sketch::builder().build());
    theta_data_set_type(res, MUTABLE_SKETCH);
    return res;
}

static void* theta_union_new_default()
{
    auto res = new theta_union(theta_union::builder().build());
    theta_data_set_type(res, UNION);
    return res;
}

static void* theta_intersection_new_default()
{
    auto res = new theta_intersection;
    theta_data_set_type(res, INTERSECTION);
    return res;
}

#define theta_compact_new(from_) \
    ({ \
        auto res_ = new compact_theta_sketch(from_); \
        theta_data_set_type(res_, IMMUTABLE_SKETCH); \
        res_; \
    })

static void update_theta_sketch_delete(void* sketchptr)
{
    if (sketchptr)
    {
        assert(theta_data_get_type(sketchptr) == MUTABLE_SKETCH);
        delete static_cast<update_theta_sketch*>(sketchptr);
    }
}

static void* theta_sketch_compact(void* sketchptr, bool destroyInput)
{
    assert(theta_data_get_type(sketchptr) == MUTABLE_SKETCH);
    auto s = static_cast<update_theta_sketch*>(sketchptr);
    auto res = theta_compact_new(s->compact());
    
    if (destroyInput)
    {
        update_theta_sketch_delete(sketchptr);
    }

    return res;
}

static void theta_sketch_delete(void* sketchptr)
{
    if (sketchptr)
    {
        delete static_cast<theta_sketch*>(sketchptr);
    }
}

static void theta_union_delete(void* unionptr)
{
    if (unionptr)
    {
        assert(theta_data_get_type(unionptr) == UNION);
        delete static_cast<theta_union*>(unionptr);
    }
}

static void theta_intersection_delete(void* interptr)
{
    if (interptr)
    {
        assert(theta_data_get_type(interptr) == INTERSECTION);
        delete static_cast<theta_intersection*>(interptr);
    }
}

static void theta_sketch_update(void* sketchptr, const void* data, unsigned length)
{
    assert(theta_data_get_type(sketchptr) == MUTABLE_SKETCH);
    static_cast<update_theta_sketch*>(sketchptr)->update(data, length);
}

static void theta_sketch_update_by_hash(void* sketchptr, const uint64_t hash)
{
    assert(theta_data_get_type(sketchptr) == MUTABLE_SKETCH);
    static_cast<update_theta_sketch*>(sketchptr)->update_by_hash(hash);
}

static void* theta_union_get_result(void* unionptr, bool destroyInput)
{
    assert(theta_data_get_type(unionptr) == UNION);

    auto u = static_cast<theta_union*>(unionptr);
    auto sketchptr = new compact_theta_sketch(u->get_result());
    theta_data_set_type(sketchptr, IMMUTABLE_SKETCH);

    if (destroyInput)
    {
        theta_union_delete(u);
    }

    assert(theta_data_get_type(sketchptr) == IMMUTABLE_SKETCH);
    return sketchptr;
}

static void* theta_intersection_get_result(void* interptr, bool destroyInput)
{
    assert(theta_data_get_type(interptr) == INTERSECTION);

    auto i = static_cast<theta_intersection*>(interptr);
    compact_theta_sketch&& i_res = i->get_result();
    auto sketchptr = theta_compact_new(std::move(i_res));

    if (destroyInput)
    {
        theta_intersection_delete(i);
    }

    assert(theta_data_get_type(sketchptr) == IMMUTABLE_SKETCH);
    return sketchptr;
}

static void* theta_data_get_result(void* dataptr, bool destroyInput)
{
    switch (theta_data_get_type(dataptr))
    {
        case UNION:
            return theta_union_get_result(dataptr, destroyInput);

        case INTERSECTION:
            return theta_intersection_get_result(dataptr, destroyInput);

        case IMMUTABLE_SKETCH:
            return dataptr;

        case MUTABLE_SKETCH:
            return theta_sketch_compact(dataptr, destroyInput);

        default:
            assert(false);
            abort();
    }
}

static void theta_union_update_with_sketch(void* unionptr, const void* sketchptr)
{
    assert(theta_data_get_type(unionptr) == UNION);
    static_cast<theta_union*>(unionptr)->update(*static_cast<const theta_sketch*>(sketchptr));
}

static void theta_union_update_with_bytes(void* unionptr, extension_list_u8_t* bytes)
{
    assert(theta_data_get_type(unionptr) == UNION);
    static_cast<theta_union*>(unionptr)->update(
        wrapped_compact_theta_sketch::wrap(bytes->ptr, bytes->len));
}

static void theta_intersection_update_with_sketch(void* interptr, const void* sketchptr)
{
    assert(theta_data_get_type(interptr) == INTERSECTION);
    static_cast<theta_intersection*>(interptr)->update(*static_cast<const theta_sketch*>(sketchptr));
}

static void theta_intersection_update_with_bytes(void* interptr, extension_list_u8_t* bytes)
{
    assert(theta_data_get_type(interptr) == INTERSECTION);
    static_cast<theta_intersection*>(interptr)->update(
        wrapped_compact_theta_sketch::wrap(bytes->ptr, bytes->len));
}

static void theta_sketch_serialize(void* sketchptr, extension_list_u8_t* output)
{
    unsigned char* bytes;
    size_t size;

    void* compact_sketch = theta_data_get_result(sketchptr, true);
    assert(theta_data_get_type(compact_sketch) ==  IMMUTABLE_SKETCH);

    auto s = static_cast<const compact_theta_sketch*>(compact_sketch);
    if (!s->serialize2(&bytes, &size))
    {
        BAIL("Failed to serialize theta sketch");
    }
    output->ptr = bytes;
    output->len = size;
}

static void* theta_sketch_deserialize(extension_list_u8_t* bytes)
{
    auto res = theta_compact_new(
        compact_theta_sketch::deserialize(bytes->ptr, bytes->len));
    return res;
}

static double theta_sketch_get_estimate(const void* sketchptr)
{
    return static_cast<const theta_sketch*>(sketchptr)->get_estimate();
}

static void theta_sketch_to_string(void* sketchptr, extension_string_t *output)
{
    auto str = static_cast<const theta_sketch*>(sketchptr)->to_string();
    output->len = str.length() + 1;
    char* buffer = (char*) malloc(output->len);
    strncpy(buffer, str.c_str(), output->len);
    output->ptr = buffer;
}

static void*
theta_anotb(
    const extension_list_u8_t* buf1,
    const extension_list_u8_t* buf2)
{
    theta_a_not_b a_not_b;
    auto res = theta_compact_new(
        a_not_b.compute(
            wrapped_compact_theta_sketch::wrap(buf1->ptr, buf1->len),
            wrapped_compact_theta_sketch::wrap(buf2->ptr, buf2->len)));
    return res;
}

static extension_state_t theta_union_merge(
    extension_state_t left,
    extension_state_t right,
    bool destroyInput)
{
    if (!VALID_HANDLE(left) && !VALID_HANDLE(right))
    {
        return NO_HANDLE;
    }

    void* u = theta_union_new_default();
    if (VALID_HANDLE(left))
    {
        auto lptr = theta_data_get_result(TO_PTR(left), destroyInput);
        theta_union_update_with_sketch(u, lptr);
        if (lptr != TO_PTR(left))
        {
            theta_sketch_delete(lptr);
        }
    }
    if (VALID_HANDLE(right))
    {
        auto rptr = theta_data_get_result(TO_PTR(right), destroyInput);
        theta_union_update_with_sketch(u, rptr);
        if (rptr != TO_PTR(right))
        {
            theta_sketch_delete(rptr);
        }
    }

    assert(theta_data_get_type(u) == UNION);
    return TO_HND(u);
}

static extension_state_t theta_intersection_merge(
    extension_state_t left,
    extension_state_t right,
    bool destroyInput)
{
    if (!VALID_HANDLE(left) && !VALID_HANDLE(right))
    {
        return NO_HANDLE;
    }

    void* x = theta_intersection_new_default();
    if (VALID_HANDLE(left))
    {
        assert(theta_data_get_type(TO_PTR(left)) == INTERSECTION);
        auto lptr = theta_data_get_result(TO_PTR(left), destroyInput);
        theta_intersection_update_with_sketch(x, lptr);
        theta_sketch_delete(lptr);
    }
    if (VALID_HANDLE(right))
    {
        assert(theta_data_get_type(TO_PTR(right)) == INTERSECTION);
        auto rptr = theta_data_get_result(TO_PTR(right), destroyInput);
        theta_intersection_update_with_sketch(x, rptr);
        theta_sketch_delete(rptr);
    }

    assert(theta_data_get_type(x) == INTERSECTION);
    return TO_HND(x);
}

///////////////////////////////////////////////////////////////////////////


extension_state_t extension_sketch_handle_init()
{
    return NO_HANDLE;
}

int32_t extension_sketch_handle_destroy(extension_state_t handle)
{
    if (handle != NO_HANDLE)
    {
        theta_sketch_delete(TO_PTR(handle));
    }
    return 0;
}

extension_state_t extension_sketch_handle_clone(extension_state_t handle)
{
    if (handle == NO_HANDLE)
    {
        return NO_HANDLE;
    }
    auto state = TO_PTR(handle);
    void* clone = nullptr;
    switch (theta_data_get_type(state))
    {
        case MUTABLE_SKETCH:
        {
            clone = new update_theta_sketch(
                *reinterpret_cast<update_theta_sketch*>(state));
            theta_data_set_type(clone, MUTABLE_SKETCH);
            assert(theta_data_get_type(clone) == MUTABLE_SKETCH);
            assert(theta_data_get_type(state) == MUTABLE_SKETCH);
            break;
        }

        case IMMUTABLE_SKETCH:
        {
            clone = new compact_theta_sketch(
                *reinterpret_cast<compact_theta_sketch*>(state));
            theta_data_set_type(clone, IMMUTABLE_SKETCH);
            assert(theta_data_get_type(clone) == IMMUTABLE_SKETCH);
            assert(theta_data_get_type(state) == IMMUTABLE_SKETCH);
            break;
        }

        case UNION:
        {
            clone = TO_PTR(theta_union_merge(NO_HANDLE, handle, false));
            assert(theta_data_get_type(clone) == UNION);
            assert(theta_data_get_type(state) == UNION);
            break;
        }

        case INTERSECTION:
        {
            clone = TO_PTR(theta_intersection_merge(NO_HANDLE, handle, false));
            assert(theta_data_get_type(clone) == INTERSECTION);
            assert(theta_data_get_type(state) == INTERSECTION);
            break;
        }

        default:  // Shouldn't happen.
            assert(false);
            abort();
    }

    assert(theta_data_get_type(clone) == theta_data_get_type(state));

    return TO_HND(clone);
}

extension_state_t
extension_sketch_handle_build_accum(
    extension_state_t handle, 
    extension_list_u8_t *input)
{
    if (!input)
    {
        return handle;
    }

    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_sketch_new_default();
    }
    assert(theta_data_get_type(state) == MUTABLE_SKETCH);

    theta_sketch_update(state, input->ptr, input->len);
    extension_list_u8_free(input);

    assert(theta_data_get_type(state) == MUTABLE_SKETCH);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_build_accum_emptyisnull(
    extension_state_t handle, 
    extension_list_u8_t *input)
{
    // If input is empty string, no update needed.
    //
    if (!input->len)
    {
        extension_list_u8_free(input);
        input = nullptr;
    }
    return extension_sketch_handle_build_accum(handle, input);
}

extension_state_t
extension_sketch_handle_build_accum_by_hash(
    extension_state_t handle,
    uint64_t input)
{
    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_sketch_new_default();
    }
    theta_sketch_update_by_hash(state, input);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_build_accum_by_hash_emptyisnull(
    extension_state_t handle,
    uint64_t input)
{
    if (!input)
    {
        return handle;
    }
    return extension_sketch_handle_build_accum_by_hash(handle, input);
}

extension_state_t
extension_sketch_handle_union_accum(
    extension_state_t handle,
    extension_list_u8_t *input)
{
    if (!input)
    {
        return handle;
    }

    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_union_new_default();
    }
    theta_union_update_with_bytes(state, input);
    extension_list_u8_free(input);

    assert(theta_data_get_type(state) == UNION);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_union_accum_emptyisnull(
    extension_state_t handle,
    extension_list_u8_t *input)
{
    // If input is empty string, no update needed.
    //
    if (!input->len)
    {
        extension_list_u8_free(input);
        input = nullptr;
    }
    return extension_sketch_handle_union_accum(handle, input);
}

extension_state_t
extension_sketch_handle_intersection_accum(
    extension_state_t handle,
    extension_list_u8_t *input)
{
    if (!input)
    {
        return handle;
    }

    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_intersection_new_default();
    }
    theta_intersection_update_with_bytes(state, input);
    extension_list_u8_free(input);

    assert(theta_data_get_type(state) == INTERSECTION);
    return TO_HND(state);
}

extension_state_t
extension_sketch_handle_intersection_accum_emptyisnull(
    extension_state_t handle,
    extension_list_u8_t *input)
{
    // If input is empty string, no update needed.
    //
    if (!input->len)
    {
        extension_list_u8_free(input);
        input = nullptr;
    }
    return extension_sketch_handle_intersection_accum(handle, input);
}

extension_state_t
extension_sketch_handle_union_merge(
    extension_state_t left,
    extension_state_t right)
{
    return theta_union_merge(left, right, true);
}

extension_state_t
extension_sketch_handle_union_copymerge(
    extension_state_t left,
    extension_state_t right)
{
    return theta_union_merge(left, right, false);
}

extension_state_t
extension_sketch_handle_intersection_merge(
    extension_state_t left,
    extension_state_t right)
{
    return theta_intersection_merge(left, right, true);
}

extension_state_t
extension_sketch_handle_intersection_copymerge(
    extension_state_t left,
    extension_state_t right)
{
    return theta_intersection_merge(left, right, false);
}

void
extension_sketch_handle_serialize(
    extension_state_t handle,
    extension_list_u8_t *output)
{
    if (!VALID_HANDLE(handle))
    {
        output->ptr = nullptr;
        output->len = 0;
        return;
    }
    auto state = TO_PTR(handle);
    state = theta_data_get_result(state, true);
    assert(theta_data_get_type(state) == IMMUTABLE_SKETCH);

    theta_sketch_serialize(state, output);
    theta_sketch_delete(state);
}

extension_state_t
extension_sketch_handle_deserialize(
    extension_list_u8_t *data)
{
    extension_state_t res;
    if (data->len == 0)
    {
        res = NO_HANDLE;
    }
    else
    {
        auto stateptr = theta_sketch_deserialize(data);
        assert(theta_data_get_type(stateptr) == IMMUTABLE_SKETCH);
        res = TO_HND(stateptr);
    }
    extension_list_u8_free(data);
    return res;
}

double extension_sketch_get_estimate(extension_list_u8_t* data)
{
    if (!data)
    {
        extension_list_u8_free(data);
        return 0.0;
    }
    BAIL_IF(data->len < 8, "at least 8 bytes expected, actual %zu", data->len);

    auto sketchptr = theta_sketch_deserialize(data);
    auto estimate = theta_sketch_get_estimate(sketchptr);
    theta_sketch_delete(sketchptr);
    extension_list_u8_free(data);
    return estimate;
}

double extension_sketch_get_estimate_emptyisnull(extension_list_u8_t* data)
{
    if (!data->len)
    {
        extension_list_u8_free(data);
        data = nullptr;
    }
    return extension_sketch_get_estimate(data);
}

void
extension_sketch_union(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    void* unionptr = theta_union_new_default();
    if (left)
    {
        BAIL_IF(left->len < 8, "at least 8 bytes expected, actual %zu", left->len);
        assert(theta_data_get_type(left) == UNION);
        theta_union_update_with_bytes(unionptr, left);
    }
    if (right)
    {
        BAIL_IF(right->len < 8, "at least 8 bytes expected, actual %zu", right->len);
        assert(theta_data_get_type(right) == UNION);
        theta_union_update_with_bytes(unionptr, right);
    }

    void* sketchptr = theta_union_get_result(unionptr, false);
    theta_sketch_serialize(sketchptr, output);

    if (left)  extension_list_u8_free(left);
    if (right) extension_list_u8_free(right);
    theta_sketch_delete(sketchptr);
}

void
extension_sketch_union_emptyisnull(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    if (!left->len)
    {
        extension_list_u8_free(left);
        left = nullptr;
    }
    if (!right->len)
    {
        extension_list_u8_free(right);
        right = nullptr;
    }
    extension_sketch_union(left, right, output);
}

void
extension_sketch_intersection(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    void* interptr = theta_intersection_new_default();
    if (left)
    {
        BAIL_IF(left->len < 8, "at least 8 bytes expected, actual %zu", left->len);
        assert(theta_data_get_type(left) == INTERSECTION);
        theta_intersection_update_with_bytes(interptr, left);
    }
    if (right)
    {
        BAIL_IF(right->len < 8, "at least 8 bytes expected, actual %zu", right->len);
        assert(theta_data_get_type(right) == INTERSECTION);
        theta_intersection_update_with_bytes(interptr, right);
    }

    void* sketchptr = theta_intersection_get_result(interptr, true);
    theta_sketch_serialize(sketchptr, output);

    if (left)  extension_list_u8_free(left);
    if (right) extension_list_u8_free(right);
    theta_sketch_delete(sketchptr);
}

void
extension_sketch_intersection_emptyisnull(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    if (!left->len)
    {
        extension_list_u8_free(left);
        left = nullptr;
    }
    if (!right->len)
    {
        extension_list_u8_free(right);
        right = nullptr;
    }
    extension_sketch_intersection(left, right, output);
}

void
extension_sketch_a_not_b(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    BAIL_IF(!left || !right, "a_not_b must have two valid sketches");
    BAIL_IF(left->len < 8, "at least 8 bytes expected, actual %zu", left->len);
    BAIL_IF(right->len < 8, "at least 8 bytes expected, actual %zu", right->len);

    void* sketchptr = theta_anotb(left, right);
    theta_sketch_serialize(sketchptr, output);

    if (left) extension_list_u8_free(left);
    if (right) extension_list_u8_free(right);
    theta_sketch_delete(sketchptr);
}

void
extension_sketch_a_not_b_emptyisnull(
    extension_list_u8_t *left,
    extension_list_u8_t *right,
    extension_list_u8_t *output)
{
    if (!left->len)
    {
        extension_list_u8_free(left);
        left = nullptr;
    }
    if (!right->len)
    {
        extension_list_u8_free(right);
        right = nullptr;
    }
    extension_sketch_a_not_b(left, right, output);
}

void 
extension_sketch_to_string(
    extension_list_u8_t *data,
    extension_string_t *ret)
{
    if (!data)
    {
        ret->ptr = nullptr;
        ret->len = 0;
        return;
    }

    auto sketchptr = theta_sketch_deserialize(data);
    theta_sketch_to_string(sketchptr, ret);
    theta_sketch_delete(sketchptr);
    extension_list_u8_free(data);
}

void 
extension_sketch_to_string_emptyisnull(
    extension_list_u8_t *data,
    extension_string_t *ret)
{
    if (!data->len)
    {
        extension_list_u8_free(data);
        data = nullptr;
    }
    extension_sketch_to_string(data, ret);
}

uint64_t extension_sketch_hash(extension_list_u8_t *data)
{
    if (!data)
    {
        return 0;
    }
    HashState hashes;
    MurmurHash3_x64_128(data->ptr, data->len, DEFAULT_SEED, hashes);
    extension_list_u8_free(data);
    return (hashes.h1 >> 1);
}

uint64_t extension_sketch_hash_emptyisnull(extension_list_u8_t *data)
{
    if (!data->len)
    {
        extension_list_u8_free(data);
        data = nullptr;
    }
    return extension_sketch_hash(data);
}

