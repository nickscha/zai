#ifndef ZAI_RIGID_BODY_H
#define ZAI_RIGID_BODY_H

#include "zai_math_linear_algebra.h"

/* #############################################################################
 * # [SECTION] Rigid Body
 * #############################################################################
 */
typedef struct
{
    zai_vec3 velocity;
    zai_vec3 angular_velocity;

    zai_vec3 force_accumulator;
    zai_vec3 torque_accumulator;

    f32 mass;

} zai_rigid_body;

ZAI_API ZAI_INLINE zai_vec3 zai_rigid_body_get_point_velocity(zai_rigid_body *rb, zai_vec3 center_of_mass, zai_vec3 point)
{
    zai_vec3 r = zai_vec3_sub(point, center_of_mass);
    zai_vec3 angular_vel_part = zai_vec3_cross(rb->angular_velocity, r);
    return zai_vec3_add(rb->velocity, angular_vel_part);
}

ZAI_API ZAI_INLINE void zai_rigid_body_add_force_at_position(zai_rigid_body *rb, zai_vec3 center_of_mass, zai_vec3 force, zai_vec3 position)
{
    zai_vec3 r = zai_vec3_sub(position, center_of_mass);
    zai_vec3 torque = zai_vec3_cross(r, force);

    rb->force_accumulator = zai_vec3_add(rb->force_accumulator, force);
    rb->torque_accumulator = zai_vec3_add(rb->torque_accumulator, torque);
}

#endif /* ZAI_RIGID_BODY_H */