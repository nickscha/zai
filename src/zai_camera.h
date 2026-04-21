#ifndef ZAI_CAMERA_H
#define ZAI_CAMERA_H

#include "zai_types.h"
#include "zai_math_linear_algebra.h"

/* #############################################################################
 * # [SECTION] Basic Camera
 * #############################################################################
 */
typedef struct zai_camera
{
  zai_vec3 position;
  zai_vec3 front;
  zai_vec3 up;
  zai_vec3 right;
  zai_vec3 worldUp;

  f32 yaw;
  f32 pitch;
  f32 fov;

} zai_camera;

ZAI_API ZAI_INLINE void zai_camera_update(zai_camera *cam)
{
  f32 yawRadians = ZAI_DEG_TO_RAD(cam->yaw);
  f32 pitchRadians = ZAI_DEG_TO_RAD(cam->pitch);
  f32 pitchRadiansCos = zai_cosf(pitchRadians);

  cam->front.x = zai_cosf(yawRadians) * pitchRadiansCos;
  cam->front.y = zai_sinf(pitchRadians);
  cam->front.z = zai_sinf(yawRadians) * pitchRadiansCos;
  cam->front = zai_vec3_normalize(cam->front);
  cam->right = zai_vec3_normalize(zai_vec3_cross(cam->front, cam->worldUp));
  cam->up = zai_vec3_normalize(zai_vec3_cross(cam->right, cam->front));
}

ZAI_API zai_camera camera_init(void)
{
  zai_camera cam = {0};

  cam.position = zai_vec3_zero;
  cam.front = zai_vec3_init(0.0f, 0.0f, -1.0f);
  cam.up = zai_vec3_init(0.0f, 1.0f, 0.0f);
  cam.worldUp = cam.up;
  cam.fov = 90.0f;
  cam.yaw = -90.0f;

  zai_camera_update(&cam);

  return (cam);
}

#endif /* ZAI_CAMERA_H */