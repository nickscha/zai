#version 330 core

uniform vec3  iResolution;
uniform sampler2D tex_font;

out vec4 FragColor;

#define C(c) U.x-=.5; outColor+= fontChar(U,64+c)

vec4 fontChar(vec2 p, int c) 
{
    if (p.x<.0|| p.x>1. || p.y<0.|| p.y>1.) return vec4(0,0,0,1e5);
	return textureGrad(tex_font, p/16. + fract( vec2(c, 15-c/16) / 16. ), dFdx(p/16.),dFdy(p/16.) );
}

void mainImage(out vec4 outColor, in vec2 fragCoord)
{
    outColor = vec4(0.0);
    fragCoord /= iResolution.y;

    vec2 position = vec2(.0);
    float FontSize = 3.;
    vec2 U = ( fragCoord - position)*64.0/FontSize;

    // "Zai "
    C(26);C(33);C(41);C(-32);

    outColor = outColor.xxxx;
}

void main()
{
  vec2 fragCoord = gl_FragCoord.xy;
  mainImage(FragColor, fragCoord);
}