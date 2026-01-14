/*    
    Copyright (c) 2019 Patryk Cieslak. All rights reserved.

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

in vec3 texcoord;
out vec4 fragcolor;
uniform samplerCube tex;
uniform float exposure;
    
void main(void)
{
    // Cubemaps in Stonefish are typically stored in linear HDR; apply a lightweight tonemap for display.
    vec3 hdr = texture(tex, texcoord).rgb;
    // If the cubemap contains NaNs/Infs (often due to invalid lighting/atmosphere state), make it obvious.
    if(any(isnan(hdr)) || any(isinf(hdr)))
    {
        fragcolor = vec4(1.0, 0.0, 1.0, 1.0);
        return;
    }

    hdr = max(hdr, vec3(0.0)) * max(exposure, 0.0);
    vec3 ldr = hdr / (vec3(1.0) + hdr); // Reinhard
    ldr = pow(ldr, vec3(1.0 / 2.2));    // approx gamma
    fragcolor = vec4(clamp(ldr, 0.0, 1.0), 1.0);
}
