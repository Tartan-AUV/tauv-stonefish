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

    color = texture(texCube, dir);
}
