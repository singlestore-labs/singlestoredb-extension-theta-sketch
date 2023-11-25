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
#include <unistd.h>

using namespace datasketches;

#define NO_HANDLE ((extension_state_t) -1)
#define TO_HND(x_) (reinterpret_cast<extension_state_t>(x_))
#define TO_PTR(x_) (reinterpret_cast<void*>(x_))
#define VALID_HANDLE(s_) (TO_HND(s_) != NO_HANDLE)


static void* theta_sketch_new_default()
{
    return new update_theta_sketch(update_theta_sketch::builder().build());
}

static void* theta_union_new_default()
{
    return new theta_union(theta_union::builder().build());
}

static void* theta_intersection_new_default()
{
    return new theta_intersection;
}

static void update_theta_sketch_delete(void* sketchptr)
{
    if (sketchptr)
    {
        delete static_cast<update_theta_sketch*>(sketchptr);
    }
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
        delete static_cast<theta_union*>(unionptr);
    }
}

static void theta_intersection_delete(void* interptr)
{
    if (interptr)
    {
        delete static_cast<theta_intersection*>(interptr);
    }
}

static void theta_sketch_update(void* sketchptr, const void* data, unsigned length)
{
    static_cast<update_theta_sketch*>(sketchptr)->update(data, length);
}

static void theta_data_set_type(void* dataptr, agg_state_type t)
{
    static_cast<theta_data*>(dataptr)->set_type(t);
}

static agg_state_type theta_data_get_type(void* dataptr)
{
    return static_cast<theta_data*>(dataptr)->get_type();
}

static void* theta_union_get_result(void* unionptr)
{
    auto u = static_cast<theta_union*>(unionptr);
    auto sketchptr = new compact_theta_sketch(u->get_result());
    theta_union_delete(u);
    return sketchptr;
}

static void* theta_intersection_get_result(void* interptr)
{
    auto i = static_cast<theta_intersection*>(interptr);
    auto sketchptr = new compact_theta_sketch(i->get_result());
    theta_intersection_delete(i);
    return sketchptr;
}

static void theta_union_update_with_sketch(void* unionptr, const void* sketchptr)
{
    static_cast<theta_union*>(unionptr)->update(*static_cast<const theta_sketch*>(sketchptr));
}

static void theta_intersection_update_with_sketch(void* interptr, const void* sketchptr)
{
    static_cast<theta_intersection*>(interptr)->update(*static_cast<const theta_sketch*>(sketchptr));
}

static void* theta_sketch_compact(void* sketchptr)
{
    auto s = static_cast<update_theta_sketch*>(sketchptr);
    auto res = new compact_theta_sketch(s->compact());
    update_theta_sketch_delete(s);
    return res;
}

static void theta_sketch_serialize(const void* sketchptr, extension_list_u8_t* output)
{
    unsigned char* bytes;
    size_t size;
    auto s = static_cast<const compact_theta_sketch*>(sketchptr);
    if (!s->serialize2(&bytes, &size))
    {
        abort();
    }
    output->ptr = bytes;
    output->len = size;
}

static void* theta_sketch_deserialize(extension_list_u8_t* bytes)
{
    return new compact_theta_sketch(
        compact_theta_sketch::deserialize(bytes->ptr, bytes->len));
}

static double theta_sketch_get_estimate(const void* sketchptr)
{
    return static_cast<const theta_sketch*>(sketchptr)->get_estimate();
}

static void theta_sketch_to_string(void* sketchptr, extension_string_t *ret0)
{
    auto str = static_cast<const theta_sketch*>(sketchptr)->to_string();
    ret0->len = str.length() + 1;
    char* buffer = (char*) malloc(ret0->len);
    strncpy(buffer, str.c_str(), ret0->len);
    ret0->ptr = buffer;
}


extension_state_t extension_sketch_agg_init_handle()
{
    return NO_HANDLE;
}

extension_state_t extension_sketch_agg_update_handle(extension_state_t handle, extension_list_u8_t *input)
{
    void* state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_sketch_new_default();
        theta_data_set_type(state, MUTABLE_SKETCH);
    }
    theta_sketch_update(state, input->ptr, input->len);
    free(input->ptr);
    return TO_HND(state);
}

extension_state_t extension_sketch_agg_merge_handle(extension_state_t left, extension_state_t right)
{
    auto combined = theta_union_new_default();
    theta_data_set_type(combined, IMMUTABLE_SKETCH);
    if (VALID_HANDLE(left))
    {
        void* state = TO_PTR(left);
        if (theta_data_get_type(state) == UNION)
        {
            state = theta_union_get_result(state);
        }
        theta_union_update_with_sketch(combined, state);
        theta_sketch_delete(state);
    }
    if (VALID_HANDLE(right))
    {
        void* state = TO_PTR(right);
        if (theta_data_get_type(state) == UNION)
        {
            state = theta_union_get_result(state);
        }
        theta_union_update_with_sketch(combined, state);
        theta_sketch_delete(state);
    }
    combined = theta_union_get_result(combined);
    return TO_HND(combined);
}

void extension_sketch_agg_serialize_handle(extension_state_t handle, extension_list_u8_t *output)
{
    auto state = TO_PTR(handle);
    if (!VALID_HANDLE(handle))
    {
        state = theta_sketch_new_default();
        theta_data_set_type(state, MUTABLE_SKETCH);
    }
    switch (theta_data_get_type(state))
    {
        case MUTABLE_SKETCH:
            state = theta_sketch_compact(state);
            break;
        case UNION:
            state = theta_union_get_result(state);
            break;
        case INTERSECTION:
            state = theta_intersection_get_result(state);
            break;
        default:
            break;
    }
    theta_sketch_serialize(state, output);
    theta_sketch_delete(state);
}

extension_state_t extension_sketch_agg_deserialize_handle(extension_list_u8_t *data)
{
    auto stateptr = theta_sketch_deserialize(data);
    theta_data_set_type(stateptr, IMMUTABLE_SKETCH);
    free(data->ptr);
    return TO_HND(stateptr);
}

double extension_sketch_estimate(extension_list_u8_t* data)
{
    auto sketchptr = theta_sketch_deserialize(data);
    auto estimate = theta_sketch_get_estimate(sketchptr);
    theta_sketch_delete(sketchptr);
    free(data->ptr);
    return estimate;
}

void extension_sketch_to_string(extension_list_u8_t *data, extension_string_t *ret0)
{
    auto sketchptr = theta_sketch_deserialize(data);
    theta_sketch_to_string(sketchptr, ret0);
    theta_sketch_delete(sketchptr);
}

