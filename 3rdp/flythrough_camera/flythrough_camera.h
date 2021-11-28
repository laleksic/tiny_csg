#ifndef FLYTHROUGH_CAMERA_H
#define FLYTHROUGH_CAMERA_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

// Flags for tweaking the view matrix
#define FLYTHROUGH_CAMERA_LEFT_HANDED_BIT 1

// * eye:
//     * Current eye position. Will be updated to new eye position.
// * look:
//     * Current look direction. Will be updated to new look direction.
// * up:
//     * Camera's "up" direction. Likely (0,1,0). Likely constant throughout application.
// * view (optional):
//     * The matrix that will be updated with the new view transform. Previous contents don't matter.
// * delta_time_seconds:
//     * Amount of seconds passed since last update.
// * eye_speed:
//     * How much the eye should move in world units per second.
// * degrees_per_cursor_move:
//     * How many degrees the camera rotates when the mouse moves by that many units.
// * max_pitch_rotation_degrees:
//     * How far up or down you're allowed to look.
//     * This prevents you from looking straight up or straight down,
//     * since being in alignment with the "up" direction leads to discontinuities.
//     * 0 degrees means you can't look up or down at all
//     * 80 degrees means you can almost look straight up, but not quite. (a good choice)
// * delta_cursor_x, delta_cursor_y:
//     * Update these every frame based on horizontal and vertical mouse movement.
// * forward_held, left_held, backward_held, right_held, jump_held, crouch_held:
//     * Update these every frame based on whether their associated keyboard keys are pressed.
//     * Example layout: W, A, S, D, space, ctrl
// * flags:
//     * For producing a different view matrix depending on your conventions.
void flythrough_camera_update(
    float eye[3],
    float look[3],
    const float up[3],
    float view[16],
    float delta_time_seconds,
    float eye_speed,
    float degrees_per_cursor_move,
    float max_pitch_rotation_degrees,
    int delta_cursor_x, int delta_cursor_y,
    int forward_held, int left_held, int backward_held, int right_held,
    int jump_held, int crouch_held,
    unsigned int flags);

// Utility for producing a look-to matrix without having to update a camera.
void flythrough_camera_look_to(
    const float eye[3],
    const float look[3],
    const float up[3],
    float view[16],
    unsigned int flags);

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FLYTHROUGH_CAMERA_H

#ifdef FLYTHROUGH_CAMERA_IMPLEMENTATION

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <math.h>
#include <assert.h>

void flythrough_camera_update(
    float eye[3],
    float look[3],
    const float up[3],
    float view[16],
    float delta_time_seconds,
    float eye_speed,
    float degrees_per_cursor_move,
    float max_pitch_rotation_degrees,
    int delta_cursor_x, int delta_cursor_y,
    int forward_held, int left_held, int backward_held, int right_held,
    int jump_held, int crouch_held,
    unsigned int flags)
{
    float look_len = sqrtf(look[0] * look[0] + look[1] * look[1] + look[2] * look[2]);
    float up_len = sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);

    // unit length of look direction is expected and maintained throughout the algorithm
    // otherwise, the vector gets smaller and smaller as error accumulates, eventually becoming 0.
    assert(fabsf(look_len - 1.0f) < 0.000001f);
    assert(fabsf(up_len - 1.0f) < 0.000001f);

    // account for Y going down in cursor apis
    delta_cursor_y = -delta_cursor_y;

    // across = normalize(cross(normalize(look), normalize(up)))
    float across[3] = {
        look[1] / look_len * up[2] / up_len - look[2] / look_len * up[1] / up_len,
        look[2] / look_len * up[0] / up_len - look[0] / look_len * up[2] / up_len,
        look[0] / look_len * up[1] / up_len - look[1] / look_len * up[0] / up_len
    };
    float across_len = sqrtf(across[0] * across[0] + across[1] * across[1] + across[2] * across[2]);
    across[0] /= across_len;
    across[1] /= across_len;
    across[2] /= across_len;

    // forward = normalize(look)
    float forward[3] = { look[0] / look_len, look[1] / look_len, look[2] / look_len };

    // upward = normalize(cross(across, forward))
    float upward[3] = {
        across[1] * forward[2] - across[2] * forward[1],
        across[2] * forward[0] - across[0] * forward[2],
        across[0] * forward[1] - across[1] * forward[0]
    };
    float upward_len = sqrtf(upward[0] * upward[0] + upward[1] * upward[1] + upward[2] * upward[2]);
    upward[0] /= upward_len;
    upward[1] /= upward_len;
    upward[2] /= upward_len;

    // apply eye movement in the xz plane
    if ((right_held && !left_held) || (!right_held && left_held) ||
        (forward_held && !backward_held) || (!forward_held && backward_held))
    {
        float x_multiplier = (right_held ? 1.0f : 0.0f) - (left_held ? 1.0f : 0.0f);
        float z_multiplier = (forward_held ? 1.0f : 0.0f) - (backward_held ? 1.0f : 0.0f);

        float xz_movement[3] = {
            across[0] * x_multiplier + forward[0] * z_multiplier,
            across[1] * x_multiplier + forward[1] * z_multiplier,
            across[2] * x_multiplier + forward[2] * z_multiplier
        };

        float xz_movement_len = sqrtf(xz_movement[0] * xz_movement[0] + xz_movement[1] * xz_movement[1] + xz_movement[2] * xz_movement[2]);

        eye[0] += xz_movement[0] / xz_movement_len * eye_speed * delta_time_seconds;
        eye[1] += xz_movement[1] / xz_movement_len * eye_speed * delta_time_seconds;
        eye[2] += xz_movement[2] / xz_movement_len * eye_speed * delta_time_seconds;
    }

    // apply eye movement in the y direction
    if ((jump_held && !crouch_held) || (!jump_held && crouch_held))
    {
        float y_multiplier = (jump_held ? 1.0f : 0.0f) - (crouch_held ? 1.0f : 0.0f);
        float y_movement[3] = { up[0] * y_multiplier, up[1] * y_multiplier, up[2] * y_multiplier };
        float y_movement_len = sqrtf(y_movement[0] * y_movement[0] + y_movement[1] * y_movement[1] + y_movement[2] * y_movement[2]);

        eye[0] += y_movement[0] / y_movement_len * eye_speed * delta_time_seconds;
        eye[1] += y_movement[1] / y_movement_len * eye_speed * delta_time_seconds;
        eye[2] += y_movement[2] / y_movement_len * eye_speed * delta_time_seconds;
    }

    // apply yaw rotation (rotating left or right)
    if (delta_cursor_x != 0)
    {
        // rotation here is counter-clockwise because sin/cos are counter-clockwise
        float yaw_degrees = -delta_cursor_x * degrees_per_cursor_move;

        float yaw_radians = yaw_degrees * 3.14159265359f / 180.0f;
        float yaw_cos = cosf(yaw_radians);
        float yaw_sin = sinf(yaw_radians);

        float up_norm[3] = { up[0] / up_len, up[1] / up_len, up[2] / up_len };

        float yaw_rotation[9] = {
            yaw_cos + (1.0f - yaw_cos) * up_norm[0] * up_norm[0],
            (1.0f - yaw_cos)  * up_norm[0] * up_norm[1] + yaw_sin * up_norm[2],
            (1.0f - yaw_cos) * up_norm[0] * up_norm[2] - yaw_sin * up_norm[1],

            (1.0f - yaw_cos) * up_norm[0] * up_norm[1] - yaw_sin * up_norm[2],
            yaw_cos + (1.0f - yaw_cos) * up_norm[1] * up_norm[1],
            (1.0f - yaw_cos) * up_norm[1] * up_norm[2] + yaw_sin * up_norm[0],

            (1.0f - yaw_cos) * up_norm[0] * up_norm[2] + yaw_sin * up_norm[1],
            (1.0f - yaw_cos) * up_norm[1] * up_norm[2] - yaw_sin * up_norm[0],
            yaw_cos + (1.0f - yaw_cos) * up_norm[2] * up_norm[2]
        };

        float newlook[3] = {
            yaw_rotation[0] * look[0] + yaw_rotation[3] * look[1] + yaw_rotation[6] * look[2],
            yaw_rotation[1] * look[0] + yaw_rotation[4] * look[1] + yaw_rotation[7] * look[2],
            yaw_rotation[2] * look[0] + yaw_rotation[5] * look[1] + yaw_rotation[8] * look[2]
        };

        float newlook_len = sqrtf(newlook[0] * newlook[0] + newlook[1] * newlook[1] + newlook[2] * newlook[2]);

        look[0] = newlook[0] / newlook_len;
        look[1] = newlook[1] / newlook_len;
        look[2] = newlook[2] / newlook_len;
    }

    // apply pitch rotation (rotating up or down)
    if (delta_cursor_y != 0)
    {
        float rads_to_up = acosf(look[0] * up[0] + look[1] * up[1] + look[2] * up[2]);
        float rads_to_down = acosf(look[0] * -up[0] + look[1] * -up[1] + look[2] * -up[2]);

        float degs_to_up = rads_to_up / 3.14159265359f * 180.0f;
        float degs_to_down = rads_to_down / 3.14159265359f * 180.0f;

        float max_pitch_degrees = degs_to_up - (90.0f - max_pitch_rotation_degrees);
        if (max_pitch_degrees < 0.0f)
            max_pitch_degrees = 0.0f;

        float min_pitch_degrees = degs_to_down - (90.0f - max_pitch_rotation_degrees);
        if (min_pitch_degrees < 0.0f)
            min_pitch_degrees = 0.0f;

        // rotation here is counter-clockwise because sin/cos are counter-clockwise
        float pitch_degrees = delta_cursor_y * degrees_per_cursor_move;

        if (pitch_degrees > 0.0f && pitch_degrees > max_pitch_degrees)
            pitch_degrees = max_pitch_degrees;

        if (pitch_degrees < 0.0f && -pitch_degrees > min_pitch_degrees)
            pitch_degrees = -min_pitch_degrees;

        float pitch_rads = pitch_degrees * 3.14159265359f / 180.0f;
        float pitch_cos = cosf(pitch_rads);
        float pitch_sin = sinf(pitch_rads);

        float pitch_rotation[9] = {
            pitch_cos + (1.0f - pitch_cos) * across[0] * across[0],
            (1.0f - pitch_cos)  * across[0] * across[1] + pitch_sin * across[2],
            (1.0f - pitch_cos) * across[0] * across[2] - pitch_sin * across[1],

            (1.0f - pitch_cos) * across[0] * across[1] - pitch_sin * across[2],
            pitch_cos + (1.0f - pitch_cos) * across[1] * across[1],
            (1.0f - pitch_cos) * across[1] * across[2] + pitch_sin * across[0],

            (1.0f - pitch_cos) * across[0] * across[2] + pitch_sin * across[1],
            (1.0f - pitch_cos) * across[1] * across[2] - pitch_sin * across[0],
            pitch_cos + (1.0f - pitch_cos) * across[2] * across[2]
        };

        float newlook[3] = {
            pitch_rotation[0] * look[0] + pitch_rotation[3] * look[1] + pitch_rotation[6] * look[2],
            pitch_rotation[1] * look[0] + pitch_rotation[4] * look[1] + pitch_rotation[7] * look[2],
            pitch_rotation[2] * look[0] + pitch_rotation[5] * look[1] + pitch_rotation[8] * look[2]
        };

        float newlook_len = sqrtf(newlook[0] * newlook[0] + newlook[1] * newlook[1] + newlook[2] * newlook[2]);

        look[0] = newlook[0] / newlook_len;
        look[1] = newlook[1] / newlook_len;
        look[2] = newlook[2] / newlook_len;
    }

    flythrough_camera_look_to(eye, look, up, view, flags);
}

void flythrough_camera_look_to(
    const float eye[3],
    const float look[3],
    const float up[3],
    float view[16],
    unsigned int flags)
{
    if (!view)
        return;
        
    float look_len = sqrtf(look[0] * look[0] + look[1] * look[1] + look[2] * look[2]);
    float up_len = sqrtf(up[0] * up[0] + up[1] * up[1] + up[2] * up[2]);

    assert(fabsf(look_len - 1.0f) < 0.000001f);
    assert(fabsf(up_len - 1.0f) < 0.000001f);

    // up'' = normalize(up)
    float up_norm[3] = { up[0] / up_len, up[1] / up_len, up[2] / up_len };

    // f = normalize(look)
    float f[3] = { look[0] / look_len, look[1] / look_len, look[2] / look_len };

    // s = normalize(cross(f, up2))
    float s[3] = {
        f[1] * up_norm[2] - f[2] * up_norm[1],
        f[2] * up_norm[0] - f[0] * up_norm[2],
        f[0] * up_norm[1] - f[1] * up_norm[0]
    };
    float s_len = sqrtf(s[0] * s[0] + s[1] * s[1] + s[2] * s[2]);
    s[0] /= s_len;
    s[1] /= s_len;
    s[2] /= s_len;

    // u = normalize(cross(normalize(s), f))
    float u[3] = {
        s[1] * f[2] - s[2] * f[1],
        s[2] * f[0] - s[0] * f[2],
        s[0] * f[1] - s[1] * f[0]
    };
    float u_len = sqrtf(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    u[0] /= u_len;
    u[1] /= u_len;
    u[2] /= u_len;

    if (!(flags & FLYTHROUGH_CAMERA_LEFT_HANDED_BIT))
    {
        // in a right-handed coordinate system, the camera's z looks away from the look direction.
        // this gets flipped again later when you multiply by a right-handed projection matrix
        // (notice the last row of gluPerspective, which makes it back into a left-handed system after perspective division)
        f[0] = -f[0];
        f[1] = -f[1];
        f[2] = -f[2];
    }

    // t = [s;u;f] * -eye
    float t[3] = {
        s[0] * -eye[0] + s[1] * -eye[1] + s[2] * -eye[2],
        u[0] * -eye[0] + u[1] * -eye[1] + u[2] * -eye[2],
        f[0] * -eye[0] + f[1] * -eye[1] + f[2] * -eye[2]
    };

    // m = [s,t[0]; u,t[1]; -f,t[2]];
    view[0] = s[0];
    view[1] = u[0];
    view[2] = f[0];
    view[3] = 0.0f;
    view[4] = s[1];
    view[5] = u[1];
    view[6] = f[1];
    view[7] = 0.0f;
    view[8] = s[2];
    view[9] = u[2];
    view[10] = f[2];
    view[11] = 0.0f;
    view[12] = t[0];
    view[13] = t[1];
    view[14] = t[2];
    view[15] = 1.0f;
}

#ifdef __cplusplus
}
#endif // __cplusplus

#endif // FLYTHROUGH_CAMERA_IMPLEMENTATION
