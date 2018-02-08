#if defined(VERTEX)
#if __VERSION__ >= 130
#define COMPAT_VARYING out
#define COMPAT_ATTRIBUTE in
#define COMPAT_TEXTURE texture
#else
#define COMPAT_VARYING varying
#define COMPAT_ATTRIBUTE attribute
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif
uniform mat4 MVPMatrix;

COMPAT_ATTRIBUTE vec2 position;
COMPAT_ATTRIBUTE vec2 texcoord;

COMPAT_VARYING vec2 v_texCoord;

void main()
{
    gl_Position = MVPMatrix * vec4(position,0.0,1.0);
    v_texCoord = texcoord;
}
#elif defined(FRAGMENT)
#if __VERSION__ >= 130
#define COMPAT_VARYING in
#define COMPAT_TEXTURE texture
out vec4 FragColor;
#else
#define COMPAT_VARYING varying
#define FragColor gl_FragColor
#define COMPAT_TEXTURE texture2D
#endif

#ifdef GL_ES
#ifdef GL_FRAGMENT_PRECISION_HIGH
precision highp float;
#else
precision mediump float;
#endif
#define COMPAT_PRECISION mediump
#else
#define COMPAT_PRECISION
#endif

COMPAT_VARYING vec2 v_texCoord;

uniform sampler2D tex0;

void main()
{
	FragColor = vec4(COMPAT_TEXTURE(tex0 , v_texCoord.xy).rgb, 1.0);
}
#endif
