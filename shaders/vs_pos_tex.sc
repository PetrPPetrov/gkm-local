// Copyright 2023 Petr Petrov. All rights reserved.
// License: https://github.com/PetrPPetrov/gkm-local/blob/main/LICENSE

$input a_position, a_texcoord0, a_texcoord1, i_data0
$output v_texcoord0

#include <bgfx_shader.sh>
#include "constants.sh"

void main()
{
    vec4 world_pos = vec4(a_position, 0.0) + i_data0;
    gl_Position = mul(u_modelViewProj, world_pos);
    ivec3 offsets = mod(i_data0, 64) * 8;
    ivec2 uv_modes = abs(a_texcoord1) - ivec2(1, 1);
    ivec2 uv_signs = sign(a_texcoord1);
    v_texcoord0 = (a_texcoord0 + ivec2(offsets[uv_modes.x] * uv_signs.x, offsets[uv_modes.y] * uv_signs.y)) / (float)TEXTURE_SIZE;
}
