#ifndef ZAI_SDF_SCENE_H
#define ZAI_SDF_SCENE_H

#include "zai_types.h"
#include "zai_color.h"
#include "zai_math_sdf.h"
#include "zai_sparse_grid.h"

/* #############################################################################
 * # [SECTION] SDF Scene
 * #############################################################################
 */
typedef enum zai_sdf_primitive_id
{
    ZAI_SDF_PRIMITIVE_SPHERE = 0,
    ZAI_SDF_PRIMITIVE_BOX,
    ZAI_SDF_PRIMITIVE_BOX_FRAME,
    ZAI_SDF_PRIMITIVE_ELLIPSOID,
    ZAI_SDF_PRIMITIVE_OCTAHEDRON,
    ZAI_SDF_PRIMITIVE_CAPSULE_VERTICAL,
    ZAI_SDF_PRIMITIVE_COUNT

} zai_sdf_primitive_id;

typedef enum zai_sdf_operation_id
{
    ZAI_SDF_OPERATION_UNION_SMOOTH = 0,
    ZAI_SDF_OPERATION_SUBTRACT_SMOOTH,
    ZAI_SDF_OPERATION_INTERSECT_SMOOTH,
    ZAI_SDF_OPERATION_UNION,
    ZAI_SDF_OPERATION_SUBTRACT,
    ZAI_SDF_OPERATION_INTERSECT,
    ZAI_SDF_OPERATION_XOR,
    ZAI_SDF_OPERATION_COUNT

} zai_sdf_operation_id;

typedef struct zai_sdf_transform
{
    zai_vec3 position;
    zai_vec3 rotation;
    f32 scale;

} zai_sdf_transform;

typedef struct zai_sdf_primitive
{
    u8 primitive_id; /* zai_sdf_primitive */
    u8 material_id;
    u8 operation_id;
    zai_sdf_transform transform;

    union
    {
        struct
        {
            f32 radius;
        } sphere;

        struct
        {
            zai_vec3 base;
        } box;

        struct
        {
            zai_vec3 base;
            f32 edge_thickness;
        } box_frame;

        struct
        {
            zai_vec3 radius;
        } ellipsoid;

        struct
        {
            f32 scale;
        } octahedron;

        struct
        {
            f32 height;
            f32 radius;
        } capsule_vertical;

    } attributes;

} zai_sdf_primitive;

#define ZAI_SDF_PRIMITIVE_COUNT 6
static zai_sdf_primitive primitives[ZAI_SDF_PRIMITIVE_COUNT];
static zai_sdf_aabb sdf_scene_aabb;

#define ZAI_SDF_MATERIAL_COUNT 256
static u8 zai_sdf_scene_materials[ZAI_SDF_MATERIAL_COUNT * 3];

ZAI_API ZAI_INLINE void zai_sdf_scene_build(void)
{
    zai_sdf_primitive sphere = {0};
    zai_sdf_primitive box = {0};
    zai_sdf_primitive ellipsoid = {0};
    zai_sdf_primitive octahedron = {0};
    zai_sdf_primitive box_frame = {0};
    zai_sdf_primitive capsule_vertical = {0};

    sphere.primitive_id = ZAI_SDF_PRIMITIVE_SPHERE;
    sphere.material_id = 1;
    sphere.transform.position = zai_vec3_zero;
    sphere.attributes.sphere.radius = 0.5f;

    box.primitive_id = ZAI_SDF_PRIMITIVE_BOX;
    box.material_id = 2;
    box.transform.position = zai_vec3_init(-0.5f, 0.5f, -0.5f);
    box.attributes.box.base = zai_vec3_initf(0.25f);

    ellipsoid.primitive_id = ZAI_SDF_PRIMITIVE_ELLIPSOID;
    ellipsoid.material_id = 4;
    ellipsoid.transform.position = zai_vec3_init(1.0f, 0.5f, -0.5f);
    ellipsoid.attributes.ellipsoid.radius = zai_vec3_init(0.5f, 0.25f, 0.125f);

    octahedron.primitive_id = ZAI_SDF_PRIMITIVE_OCTAHEDRON;
    octahedron.material_id = 3;
    octahedron.operation_id = ZAI_SDF_OPERATION_UNION;
    octahedron.transform.position = zai_vec3_init(0.0f, 0.5f, -1.0f);
    octahedron.attributes.octahedron.scale = 0.25f;

    box_frame.primitive_id = ZAI_SDF_PRIMITIVE_BOX_FRAME;
    box_frame.transform.position = zai_vec3_init(-1.0f, 0.75f, -0.5f);
    box_frame.attributes.box_frame.base = zai_vec3_init(0.25f, 0.25f, 0.25f);
    box_frame.attributes.box_frame.edge_thickness = 0.025f;

    capsule_vertical.primitive_id = ZAI_SDF_PRIMITIVE_CAPSULE_VERTICAL;
    capsule_vertical.transform.position = zai_vec3_init(0.5f, 0.5f, -1.5f);
    capsule_vertical.attributes.capsule_vertical.height = 0.5f;
    capsule_vertical.attributes.capsule_vertical.radius = 0.075f;

    primitives[0] = sphere;
    primitives[1] = box;
    primitives[2] = ellipsoid;
    primitives[3] = octahedron;
    primitives[4] = box_frame;
    primitives[5] = capsule_vertical;

    /* Set simple bounding box */
    sdf_scene_aabb.min = zai_vec3_init(-2.0f, -1.0f, -2.0f);
    sdf_scene_aabb.max = zai_vec3_init(2.0f, 2.0f, 2.0f);

    /* Set Materials */
    /* ID 0: Default/Background */
    zai_sdf_scene_materials[0] = (u8)zai_color_f32_to_u8_unorm(1.0f);
    zai_sdf_scene_materials[1] = (u8)zai_color_f32_to_u8_unorm(1.0f);
    zai_sdf_scene_materials[2] = (u8)zai_color_f32_to_u8_unorm(1.0f);

    /* ID 1: Sphere Material */
    zai_sdf_scene_materials[3] = 60;
    zai_sdf_scene_materials[4] = 255;
    zai_sdf_scene_materials[5] = 60;

    /* ID 2: Box Material */
    zai_sdf_scene_materials[6] = 255;
    zai_sdf_scene_materials[7] = 60;
    zai_sdf_scene_materials[8] = 60;

    /* ID 3: Octahedron Material */
    zai_sdf_scene_materials[9] = 7;
    zai_sdf_scene_materials[10] = 27;
    zai_sdf_scene_materials[11] = 38;

    /* ID 4: Ellipsoid Material */
    zai_sdf_scene_materials[12] = 46;
    zai_sdf_scene_materials[13] = 191;
    zai_sdf_scene_materials[14] = 199;
}

ZAI_API ZAI_INLINE zai_grid_data zai_sdf_scene(zai_vec3 position, void *user_data)
{
    f32 ground = position.y - (-0.25f);

    zai_grid_data d;
    d.distance = ground;
    d.material = 0;

    (void)user_data;

    if (zai_sdf_aabb_distance(position, &sdf_scene_aabb) <= d.distance)
    {
        f32 primitive_distance_total = 1e30f; /* very large start distance */
        u8 primitive_material_total = 0;

        u32 i;

        for (i = 0; i < ZAI_SDF_PRIMITIVE_COUNT; ++i)
        {
            zai_sdf_primitive *primitive = &primitives[i];
            zai_vec3 primitive_pos = zai_vec3_sub(position, primitive->transform.position);
            f32 primitive_distance = ground;

            switch (primitive->primitive_id)
            {
            case ZAI_SDF_PRIMITIVE_SPHERE:
            {
                primitive_distance = zai_sdf_sphere(primitive_pos, primitive->attributes.sphere.radius);
            }
            break;
            case ZAI_SDF_PRIMITIVE_BOX:
            {
                primitive_distance = zai_sdf_box(primitive_pos, primitive->attributes.box.base);
            }
            break;
            case ZAI_SDF_PRIMITIVE_BOX_FRAME:
            {
                primitive_distance = zai_sdf_box_frame(primitive_pos, primitive->attributes.box_frame.base, primitive->attributes.box_frame.edge_thickness);
            }
            break;
            case ZAI_SDF_PRIMITIVE_ELLIPSOID:
            {
                primitive_distance = zai_sdf_ellipsoid(primitive_pos, primitive->attributes.ellipsoid.radius);
            }
            break;
            case ZAI_SDF_PRIMITIVE_OCTAHEDRON:
            {
                primitive_distance = zai_sdf_octahedron(primitive_pos, primitive->attributes.octahedron.scale);
            }
            break;
            case ZAI_SDF_PRIMITIVE_CAPSULE_VERTICAL:
            {
                primitive_distance = zai_sdf_capsule_vertical(primitive_pos, primitive->attributes.capsule_vertical.height, primitive->attributes.capsule_vertical.radius);
            }
            break;
            default:
                break;
            }

            /* material: choose the primitive that is closer (before smooth offset) */
            if (primitive_distance < primitive_distance_total)
            {
                primitive_material_total = primitive->material_id;
            }

            /* smooth union distance */
            switch (primitive->operation_id)
            {
            case ZAI_SDF_OPERATION_UNION_SMOOTH:
                primitive_distance_total = zai_sdf_op_union_smooth(primitive_distance_total, primitive_distance, 0.4f);
                break;
            case ZAI_SDF_OPERATION_SUBTRACT_SMOOTH:
                primitive_distance_total = zai_sdf_op_subtract_smooth(primitive_distance_total, primitive_distance, 0.4f);
                break;
            case ZAI_SDF_OPERATION_INTERSECT_SMOOTH:
                primitive_distance_total = zai_sdf_op_intersect_smooth(primitive_distance_total, primitive_distance, 0.4f);
                break;
            case ZAI_SDF_OPERATION_UNION:
                primitive_distance_total = zai_sdf_op_union(primitive_distance_total, primitive_distance);
                break;
            case ZAI_SDF_OPERATION_SUBTRACT:
                primitive_distance_total = zai_sdf_op_subtract(primitive_distance_total, primitive_distance);
                break;
            case ZAI_SDF_OPERATION_INTERSECT:
                primitive_distance_total = zai_sdf_op_intersect(primitive_distance_total, primitive_distance);
                break;
            case ZAI_SDF_OPERATION_XOR:
                primitive_distance_total = zai_sdf_op_xor(primitive_distance_total, primitive_distance);
                break;
            default:
                break;
            }
        }

        if (ground < primitive_distance_total)
        {
            primitive_material_total = 0; /* ground material id */
        }

        d.distance = zai_sdf_op_union_smooth(ground, primitive_distance_total, 0.6f);
        d.material = primitive_material_total;
    }

    return d;
}

/* old keep for reference */
ZAI_API ZAI_INLINE zai_grid_data zai_sdf_scene_original(zai_vec3 position, void *user_data)
{
    static u8 sdf_scene_initialized;
    static zai_sdf_aabb sdf_scene_aabb;

    f32 ground = position.y - (-0.25f);

    zai_grid_data d;
    d.material = 0;

    if (!sdf_scene_initialized)
    {
        sdf_scene_aabb.min = zai_vec3_init(-2.0f, -1.0f, -2.0f);
        sdf_scene_aabb.max = zai_vec3_init(2.0f, 2.0f, 2.0f);

        sdf_scene_initialized = 1;
    }

    if (zai_sdf_aabb_distance(position, &sdf_scene_aabb) <= d.distance)
    {

        f32 sphere_radius = 0.5f;
        f32 sphere = zai_sdf_sphere(position, sphere_radius);

        zai_vec3 box_pos = zai_vec3_sub(position, zai_vec3_init(-0.5f, 0.5f, -0.5f));
        zai_mat2x2 box_rot = zai_mat2x2_rot2d(ZAI_DEG_TO_RAD(45.0f));
        f32 box_scale = 1.0f;

        (void)user_data;

        zai_vec2_mul_mat2x2(&box_pos.x, &box_pos.y, box_rot); /* rotate around z axis */
        zai_vec2_mul_mat2x2(&box_pos.y, &box_pos.z, box_rot); /* rotate around x axis */

        {
            f32 box = zai_sdf_box(zai_vec3_divf(box_pos, box_scale), zai_vec3_init(0.25f, 0.25f, 0.25f)) * box_scale;

            zai_vec3 ellipsoid_pos = zai_vec3_sub(position, zai_vec3_init(1.0f, 0.5f, -0.5f));
            f32 ellipsoid = zai_sdf_ellipsoid(ellipsoid_pos, zai_vec3_init(0.5f, 0.25f, 0.125f));

            zai_vec3 box_frame_pos = zai_vec3_sub(position, zai_vec3_init(0.25f, 0.5f, -1.0f));
            zai_vec3 box_frame_pos_repeat = zai_sdf_op_repeat_limited(box_frame_pos, 1.0f, zai_vec3_init(1.0f, 1.0f, 0.0f));
            f32 box_frame = zai_sdf_box_frame(box_frame_pos_repeat, zai_vec3_init(0.25f, 0.25f, 0.25f), 0.025f);

            f32 octahedron = zai_sdf_octahedron(zai_vec3_sub(position, zai_vec3_init(0.0f, 0.25f, 0.35f)), 0.25f);

            d.distance = zai_sdf_op_union_smooth(ground, zai_sdf_op_union_smooth(zai_sdf_op_union_smooth(zai_sdf_op_union_smooth(zai_sdf_op_subtract(octahedron, sphere), box, 0.4f), ellipsoid, 0.2f), box_frame, 0.1f), 0.6f);
            d.material = 0;

            if (sphere < box && sphere < ground)
            {
                d.material = 1;
            }
            else if (box < sphere && box < ground)
            {
                d.material = 2;
            }
        }
    }

    return d;
}

#endif /* ZAI_SDF_SCENE_H */