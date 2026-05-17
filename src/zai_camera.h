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
  f32 roll;

} zai_camera;

ZAI_API void zai_camera_update(zai_camera *cam)
{
  f32 yawRad = ZAI_DEG_TO_RAD(cam->yaw);
  f32 pitchRad = ZAI_DEG_TO_RAD(cam->pitch);
  f32 rollRad = ZAI_DEG_TO_RAD(cam->roll);

  f32 cy = zai_cosf(yawRad);
  f32 sy = zai_sinf(yawRad);

  f32 cp = zai_cosf(pitchRad);
  f32 sp = zai_sinf(pitchRad);

  f32 cr = zai_cosf(rollRad);
  f32 sr = zai_sinf(rollRad);

  zai_vec3 right;
  zai_vec3 up;

  cam->front.x = cy * cp;
  cam->front.y = sp;
  cam->front.z = sy * cp;

  right.x = -sy;
  right.y = 0.0f;
  right.z = cy;

  up.x = -cy * sp;
  up.y = cp;
  up.z = -sy * sp;

  /* Apply roll */
  cam->right.x = right.x * cr - up.x * sr;
  cam->right.y = right.y * cr - up.y * sr;
  cam->right.z = right.z * cr - up.z * sr;

  cam->up.x = right.x * sr + up.x * cr;
  cam->up.y = right.y * sr + up.y * cr;
  cam->up.z = right.z * sr + up.z * cr;
}

ZAI_API ZAI_INLINE zai_camera zai_camera_init(void)
{
  zai_camera cam = {0};

  cam.position = zai_vec3_zero;
  cam.front = zai_vec3_init(0.0f, 0.0f, -1.0f);
  cam.up = zai_vec3_init(0.0f, 1.0f, 0.0f);
  cam.worldUp = cam.up;
  cam.fov = 90.0f;
  cam.yaw = -90.0f;

  zai_camera_update(&cam);

  return cam;
}

#endif /* ZAI_CAMERA_H */