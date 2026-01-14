/*    
    Copyright (c) 2025 TartanAUV. All rights reserved.

    This file is a part of Stonefish.

    Stonefish is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    Stonefish is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <https://www.gnu.org/licenses/>.
*/

#version 330

uniform samplerCube texCube;
uniform float focal;    // normalized focal (1/maxTheta)
uniform float maxTheta; // half-FOV in radians (<= pi/2 for 180 deg total)
uniform int debugMode;  // 0: sample cubemap, 1: visualize directions, 3: visualize cubemap face selection
uniform float exposure; // scalar exposure applied before tonemap

in vec2 texcoord;
out vec4 color;

void main()
{
    vec2 xy = texcoord * 2.0 - 1.0;
    float r = length(xy);
    if(r > 1.0)
        discard;

    float theta = r / focal; // theta in radians; focal is normalized so this is r * maxTheta
    if(theta > maxTheta)
        discard;

    float phi = atan(xy.y, xy.x);
    float sinT = sin(theta);
    vec3 dir = vec3(sinT * cos(phi), sinT * sin(phi), cos(theta));

    if(debugMode == 1)
    {
        // Visualize direction mapping (useful for debugging warp correctness).
        color = vec4(dir * 0.5 + 0.5, 1.0);
        return;
    }

    if(debugMode == 3)
    {
        // Visualize which cubemap face would be selected for this direction, plus local face UVs.
        // Matches OpenGL cubemap selection conventions.
        vec3 ad = abs(dir);
        int face = 0;
        vec2 uv = vec2(0.0);
        float m = 1.0;

        if(ad.x >= ad.y && ad.x >= ad.z)
        {
            m = ad.x;
            if(dir.x > 0.0) { face = 0; uv = vec2(-dir.z, -dir.y) / m; } // +X
            else            { face = 1; uv = vec2( dir.z, -dir.y) / m; } // -X
        }
        else if(ad.y >= ad.x && ad.y >= ad.z)
        {
            m = ad.y;
            if(dir.y > 0.0) { face = 2; uv = vec2( dir.x,  dir.z) / m; } // +Y
            else            { face = 3; uv = vec2( dir.x, -dir.z) / m; } // -Y
        }
        else
        {
            m = ad.z;
            if(dir.z > 0.0) { face = 4; uv = vec2( dir.x, -dir.y) / m; } // +Z
            else            { face = 5; uv = vec2(-dir.x, -dir.y) / m; } // -Z
        }

        vec3 base;
        if(face == 0) base = vec3(1.0, 0.2, 0.2);
        else if(face == 1) base = vec3(0.6, 0.0, 0.0);
        else if(face == 2) base = vec3(0.2, 1.0, 0.2);
        else if(face == 3) base = vec3(0.0, 0.6, 0.0);
        else if(face == 4) base = vec3(0.2, 0.2, 1.0);
        else               base = vec3(0.0, 0.0, 0.6);

        vec3 uvCol = vec3(uv * 0.5 + 0.5, 0.0);
        vec3 outCol = mix(base, uvCol, 0.5);

        // Crosshair at face center.
        float centerLine = max(1.0 - smoothstep(0.0, 0.02, abs(uv.x)),
                               1.0 - smoothstep(0.0, 0.02, abs(uv.y)));
        outCol = mix(outCol, vec3(1.0), clamp(centerLine, 0.0, 1.0));

        color = vec4(outCol, 1.0);
        return;
    }

    // Cubemap is rendered in linear HDR; apply a lightweight tonemap + gamma to get usable LDR output.
    vec3 hdr = texture(texCube, dir).rgb;
    if(any(isnan(hdr)) || any(isinf(hdr)))
    {
        color = vec4(1.0, 0.0, 1.0, 1.0);
        return;
    }

    hdr = max(hdr, vec3(0.0)) * max(exposure, 0.0);
    vec3 ldr = hdr / (vec3(1.0) + hdr);             // Reinhard
    ldr = pow(ldr, vec3(1.0 / 2.2));                // approx gamma
    color = vec4(clamp(ldr, 0.0, 1.0), 1.0);
}
