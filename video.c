/* -*- mode: c; tab-width: 4; c-basic-offset: 4; c-file-style: "linux" -*- */
//
// Copyright (c) 2009-2011, Wei Mingzhi <whistler_wmz@users.sf.net>.
// Copyright (c) 2011-2017, SDLPAL development team.
// All rights reserved.
//
// This file is part of SDLPAL.
//
// SDLPAL is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program.  If not, see <http://www.gnu.org/licenses/>.
//
#include <float.h>
#include "main.h"

// Screen buffer
SDL_Surface              *gpScreen           = NULL;

// Backup screen buffer
SDL_Surface              *gpScreenBak        = NULL;

// The global palette
static SDL_Palette       *gpPalette          = NULL;

#if SDL_VERSION_ATLEAST(2,0,0)
SDL_Window               *gpWindow           = NULL;
static SDL_Renderer      *gpRenderer         = NULL;
static SDL_Texture       *gpTexture          = NULL;
static SDL_Texture       *gpTouchOverlay     = NULL;
static SDL_Rect           gOverlayRect;
static SDL_Rect           gTextureRect;
#endif

// The real screen surface
static SDL_Surface       *gpScreenReal       = NULL;

volatile BOOL g_bRenderPaused = FALSE;

static BOOL bScaleScreen = PAL_SCALE_SCREEN;

// Shake times and level
static WORD               g_wShakeTime       = 0;
static WORD               g_wShakeLevel      = 0;

#define FORCE_OPENGL_CORE_PROFILE 1

#if PAL_HAS_GLSL
static uint32_t gProgramId;
static uint32_t gVAOId; // 0 for whole screen; 1 for touch overlay
//static uint32_t gVBOIds[2];
//static uint32_t gIBOId;
static int gMVPSlot, gTextureSizeSlot;
static int glversion_major, glversion_minor;
static SDL_Texture *gpBackBuffer;
static int is_SHITEL = 0;

#if __IOS__
#include <SDL_opengles.h>
#include <SDL_opengles2.h>
#include <OpenGLES/ES3/gl.h>
#include <OpenGLES/ES3/glext.h>
#define glGenVertexArrays glGenVertexArraysOES
#define glBindVertexArray glBindVertexArrayOES
#else
#ifdef __APPLE__
#define GL_GLEXT_PROTOTYPES
#endif
#include <SDL_opengl.h>
#endif

#if __IOS__ || __ANDROID__ || __EMSCRIPTEN__
#define GLES 1
#undef FORCE_OPENGL_CORE_PROFILE
#endif

#if !defined(__APPLE__)

// I'm avoiding the use of GLEW or some extensions handler, but that 
// doesn't mean you should...
PFNGLCREATESHADERPROC glCreateShader;
PFNGLSHADERSOURCEPROC glShaderSource;
PFNGLCOMPILESHADERPROC glCompileShader;
PFNGLGETSHADERIVPROC glGetShaderiv;
PFNGLGETSHADERINFOLOGPROC glGetShaderInfoLog;
PFNGLDELETESHADERPROC glDeleteShader;
PFNGLATTACHSHADERPROC glAttachShader;
PFNGLCREATEPROGRAMPROC glCreateProgram;
PFNGLLINKPROGRAMPROC glLinkProgram;
PFNGLVALIDATEPROGRAMPROC glValidateProgram;
PFNGLGETPROGRAMIVPROC glGetProgramiv;
PFNGLGETPROGRAMINFOLOGPROC glGetProgramInfoLog;
PFNGLUSEPROGRAMPROC glUseProgram;
PFNGLGENVERTEXARRAYSPROC glGenVertexArrays;
PFNGLBINDVERTEXARRAYPROC glBindVertexArray;
PFNGLGENBUFFERSPROC glGenBuffers;
PFNGLBINDBUFFERPROC glBindBuffer;
PFNGLBUFFERDATAPROC glBufferData;
PFNGLBUFFERSUBDATAPROC glBufferSubData;
PFNGLGETATTRIBLOCATIONPROC glGetAttribLocation;
PFNGLENABLEVERTEXATTRIBARRAYPROC glEnableVertexAttribArray;
PFNGLVERTEXATTRIBPOINTERPROC glVertexAttribPointer;
PFNGLUNIFORMMATRIX4FVPROC glUniformMatrix4fv;
PFNGLUNIFORM2FVPROC glUniform2fv;
PFNGLGETUNIFORMLOCATIONPROC glGetUniformLocation;
PFNGLBINDFRAGDATALOCATIONPROC glBindFragDataLocation;
PFNGLGETSTRINGIPROC glGetStringi;

int initGLExtensions() {
	glCreateShader = (PFNGLCREATESHADERPROC)SDL_GL_GetProcAddress("glCreateShader");
	glShaderSource = (PFNGLSHADERSOURCEPROC)SDL_GL_GetProcAddress("glShaderSource");
	glCompileShader = (PFNGLCOMPILESHADERPROC)SDL_GL_GetProcAddress("glCompileShader");
	glGetShaderiv = (PFNGLGETSHADERIVPROC)SDL_GL_GetProcAddress("glGetShaderiv");
	glGetShaderInfoLog = (PFNGLGETSHADERINFOLOGPROC)SDL_GL_GetProcAddress("glGetShaderInfoLog");
	glDeleteShader = (PFNGLDELETESHADERPROC)SDL_GL_GetProcAddress("glDeleteShader");
	glAttachShader = (PFNGLATTACHSHADERPROC)SDL_GL_GetProcAddress("glAttachShader");
	glCreateProgram = (PFNGLCREATEPROGRAMPROC)SDL_GL_GetProcAddress("glCreateProgram");
	glLinkProgram = (PFNGLLINKPROGRAMPROC)SDL_GL_GetProcAddress("glLinkProgram");
	glValidateProgram = (PFNGLVALIDATEPROGRAMPROC)SDL_GL_GetProcAddress("glValidateProgram");
	glGetProgramiv = (PFNGLGETPROGRAMIVPROC)SDL_GL_GetProcAddress("glGetProgramiv");
	glGetProgramInfoLog = (PFNGLGETPROGRAMINFOLOGPROC)SDL_GL_GetProcAddress("glGetProgramInfoLog");
	glUseProgram = (PFNGLUSEPROGRAMPROC)SDL_GL_GetProcAddress("glUseProgram");
	glGenVertexArrays = (PFNGLGENVERTEXARRAYSPROC)SDL_GL_GetProcAddress("glGenVertexArrays");
	glBindVertexArray = (PFNGLBINDVERTEXARRAYPROC)SDL_GL_GetProcAddress("glBindVertexArray");
	glGenBuffers = (PFNGLGENBUFFERSPROC)SDL_GL_GetProcAddress("glGenBuffers");
	glBindBuffer = (PFNGLBINDBUFFERPROC)SDL_GL_GetProcAddress("glBindBuffer");
	glBufferData = (PFNGLBUFFERDATAPROC)SDL_GL_GetProcAddress("glBufferData");
	glBufferSubData = (PFNGLBUFFERSUBDATAPROC)SDL_GL_GetProcAddress("glBufferSubData");
	glGetAttribLocation = (PFNGLGETATTRIBLOCATIONPROC)SDL_GL_GetProcAddress("glGetAttribLocation");
	glEnableVertexAttribArray = (PFNGLENABLEVERTEXATTRIBARRAYPROC)SDL_GL_GetProcAddress("glEnableVertexAttribArray");
	glVertexAttribPointer = (PFNGLVERTEXATTRIBPOINTERPROC)SDL_GL_GetProcAddress("glVertexAttribPointer");
    glUniformMatrix4fv = (PFNGLUNIFORMMATRIX4FVPROC)SDL_GL_GetProcAddress("glUniformMatrix4fv");
    glUniform2fv = (PFNGLUNIFORM2FVPROC)SDL_GL_GetProcAddress("glUniform2fv");
    glGetUniformLocation = (PFNGLGETUNIFORMLOCATIONPROC)SDL_GL_GetProcAddress("glGetUniformLocation");
#if !GLES
    glBindFragDataLocation = (PFNGLBINDFRAGDATALOCATIONPROC)SDL_GL_GetProcAddress("glBindFragDataLocation") ;
    glGetStringi = (PFNGLGETSTRINGIPROC)SDL_GL_GetProcAddress("glGetStringi") ;
#endif

	return glCreateShader && glShaderSource && glCompileShader && glGetShaderiv && 
		glGetShaderInfoLog && glDeleteShader && glAttachShader && glCreateProgram &&
		glLinkProgram && glValidateProgram && glGetProgramiv && glGetProgramInfoLog &&
		glUseProgram && glGenVertexArrays && glBindVertexArray && glGenBuffers &&
        glBindBuffer && glBufferData && glBufferSubData && glGetAttribLocation &&
        glEnableVertexAttribArray && glVertexAttribPointer && glUniformMatrix4fv;
}

#endif

#define GetError( )\
{\
for ( GLenum Error = glGetError( ); ( GL_NO_ERROR != Error ); Error = glGetError( ) )\
{\
switch ( Error )\
{\
case GL_INVALID_ENUM:      UTIL_LogOutput(LOGLEVEL_DEBUG,  "\n%s\n\n", "GL_INVALID_ENUM"      ); assert( 0 ); break;\
case GL_INVALID_VALUE:     UTIL_LogOutput(LOGLEVEL_DEBUG,  "\n%s\n\n", "GL_INVALID_VALUE"     ); assert( 0 ); break;\
case GL_INVALID_OPERATION: UTIL_LogOutput(LOGLEVEL_DEBUG,  "\n%s\n\n", "GL_INVALID_OPERATION" ); assert( 0 ); break;\
case GL_OUT_OF_MEMORY:     UTIL_LogOutput(LOGLEVEL_DEBUG,  "\n%s\n\n", "GL_OUT_OF_MEMORY"     ); assert( 0 ); break;\
default:                                                                              break;\
}\
}\
}
struct AttrTexCoord
{
    GLfloat s;
    GLfloat t;
    GLfloat u;
    GLfloat v;
};
struct AttrVertexPos
{
    GLfloat x;
    GLfloat y;
    GLfloat z;
    GLfloat w;
};
struct VertexDataFormat
{
    struct AttrVertexPos position;
    struct AttrTexCoord texCoord;
};
#pragma pack(push,16)
union _GLKMatrix4
{
    struct
    {
        float m00, m01, m02, m03;
        float m10, m11, m12, m13;
        float m20, m21, m22, m23;
        float m30, m31, m32, m33;
    };
    float m[16];
};
#pragma pack(pop)
typedef union _GLKMatrix4 GLKMatrix4;

static GLKMatrix4 gOrthoMat;

GLKMatrix4 GLKMatrix4MakeOrtho(float left, float right,
                                          float bottom, float top,
                                          float nearZ, float farZ)
{
    float ral = right + left;
    float rsl = right - left;
    float tab = top + bottom;
    float tsb = top - bottom;
    float fan = farZ + nearZ;
    float fsn = farZ - nearZ;
    
    GLKMatrix4 m = { 2.0f / rsl, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f / tsb, 0.0f, 0.0f,
        0.0f, 0.0f, -2.0f / fsn, 0.0f,
        -ral / rsl, -tab / tsb, -fan / fsn, 1.0f };
    
    return m;
}
//DONT CHANGE
#define SHADER_TYPE(shaderType) (shaderType == GL_VERTEX_SHADER ? "VERTEX" : "FRAGMENT")
char *readShaderFile(const char *fileName, GLuint type);
GLuint compileShader(const char* fileName, GLuint shaderType) {
   UTIL_LogOutput(LOGLEVEL_DEBUG, "compiling %s shader %s", SHADER_TYPE(shaderType), fileName );
    char *source = readShaderFile(fileName, shaderType);
    // Create ID for shader
    GLuint result = glCreateShader(shaderType);
    // Define shader text
    glShaderSource(result, 1, &source, NULL);
    // Compile shader
    glCompileShader(result);
    
    //Check vertex shader for errors
    GLint shaderCompiled = GL_FALSE;
    glGetShaderiv( result, GL_COMPILE_STATUS, &shaderCompiled );
    if( shaderCompiled != GL_TRUE ) {
        GLint logLength;
        glGetShaderiv(result, GL_INFO_LOG_LENGTH, &logLength);
        if (logLength > 0)
        {
            GLchar *log = (GLchar*)malloc(logLength);
            glGetShaderInfoLog(result, logLength, &logLength, log);
            UTIL_LogOutput(LOGLEVEL_DEBUG, "shader compilation error:%s\r\n",log);
            free(log);
        }
        glDeleteShader(result);
        result = 0;
    }else
       UTIL_LogOutput(LOGLEVEL_DEBUG, "%s shader compilation finished success!", shaderType == GL_VERTEX_SHADER ? "Vertex" : "Fragment" );
    return result;
}

char g_ShaderBuffer[65536];
char *readShaderFile(const char *fileName, GLuint type) {
    memset(g_ShaderBuffer,0,sizeof(g_ShaderBuffer));
    FILE *fp = UTIL_OpenRequiredFile(fileName);
#if !GLES
    sprintf(g_ShaderBuffer,"%s\r\n",glversion_major>3 ? "#version 330" : "#version 110");
#endif
    sprintf(g_ShaderBuffer,"%s#define %s\r\n",g_ShaderBuffer,SHADER_TYPE(type));
    fread(g_ShaderBuffer+strlen(g_ShaderBuffer),1,sizeof(g_ShaderBuffer),fp);
    return g_ShaderBuffer;
}

GLuint compileProgram(const char* vtxFile, const char* fragFile) {
    GLuint programId = 0;
    GLuint vtxShaderId, fragShaderId;
    programId = glCreateProgram();
    
    vtxShaderId = compileShader(vtxFile, GL_VERTEX_SHADER);
    
    fragShaderId = compileShader(fragFile, GL_FRAGMENT_SHADER);
    
    if(vtxShaderId && fragShaderId) {
        // Associate shader with program
        glAttachShader(programId, vtxShaderId);
        glAttachShader(programId, fragShaderId);
        glLinkProgram(programId);
        glValidateProgram(programId);
        
        // Check the status of the compile/link
        GLint logLen;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &logLen);
        if(logLen > 0) {
            char* log = (char*) malloc(logLen * sizeof(char));
            // Show any errors as appropriate
            glGetProgramInfoLog(programId, logLen, &logLen, log);
            UTIL_LogOutput(LOGLEVEL_DEBUG, "shader linkage error:%s\r\n",log);
            free(log);
        }else
           UTIL_LogOutput(LOGLEVEL_DEBUG, "shaders linkage finished success!");
    }
    if(vtxShaderId) {
        glDeleteShader(vtxShaderId);
    }
    if(fragShaderId) {
        glDeleteShader(fragShaderId);
    }
    return programId;
}

void VAO_Draw(SDL_Renderer * renderer, SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect, int vaoId)
{
   GLfloat minx, miny, maxx, maxy;
   GLfloat minu, maxu, minv, maxv;
   
//   glEnable(GL_TEXTURE_2D);
   SDL_GL_BindTexture(texture, NULL, NULL);
//   glColor4f(1.0,1.0,1.0,1.0);
//   glDisable(GL_BLEND);
   
   SDL_Rect _srcrect,_dstrect;
   
   int w, h;
   SDL_QueryTexture(texture, NULL, NULL, &w, &h);
   
   int render_w,render_h;
   SDL_GetRendererOutputSize(gpRenderer, &render_w, &render_h);
   
   if(!srcrect) {
      srcrect = &_srcrect;
      _srcrect.x = 0;
      _srcrect.y = 0;
      _srcrect.w = w;
      _srcrect.h = h;
   }
   if(!dstrect) {
      dstrect = &_dstrect;
      _dstrect.x = 0;
      _dstrect.y = 0;
      _dstrect.w = render_w;
      _dstrect.h = render_h;
   }
   
   minx = dstrect->x;
   miny = dstrect->y;
   maxx = dstrect->x + dstrect->w;
   maxy = dstrect->y + dstrect->h;
   
   minu = (GLfloat) srcrect->x / w;
   //   minu *= texturedata->texw;
   maxu = (GLfloat) (srcrect->x + w) / w / (is_SHITEL ? 1.6 : 1);
   //   maxu *= texturedata->texw;
   minv = (GLfloat) srcrect->y / h;
   //   minv *= texturedata->texh;
   maxv = (GLfloat) (srcrect->y + h) / h / (is_SHITEL ? 1.28 : 1);
   //   maxv *= texturedata->texh;
   
   struct VertexDataFormat vData[ 4 ];
   
   //Texture coordinates
   vData[ 0 ].texCoord.s =  minu; vData[ 0 ].texCoord.t = minv; vData[ 0 ].texCoord.u = 0.0; vData[ 0 ].texCoord.v = 0.0;
   vData[ 1 ].texCoord.s =  maxu; vData[ 1 ].texCoord.t = minv; vData[ 1 ].texCoord.u = 0.0; vData[ 1 ].texCoord.v = 0.0;
   vData[ 2 ].texCoord.s =  maxu; vData[ 2 ].texCoord.t = maxv; vData[ 2 ].texCoord.u = 0.0; vData[ 2 ].texCoord.v = 0.0;
   vData[ 3 ].texCoord.s =  minu; vData[ 3 ].texCoord.t = maxv; vData[ 3 ].texCoord.u = 0.0; vData[ 3 ].texCoord.v = 0.0;
   
   //Vertex positions
   vData[ 0 ].position.x = minx; vData[ 0 ].position.y = miny; vData[ 0 ].position.z = 0.0; vData[ 0 ].position.w = 1.0;
   vData[ 1 ].position.x = maxx; vData[ 1 ].position.y = miny; vData[ 1 ].position.z = 0.0; vData[ 1 ].position.w = 1.0;
   vData[ 2 ].position.x = maxx; vData[ 2 ].position.y = maxy; vData[ 2 ].position.z = 0.0; vData[ 2 ].position.w = 1.0;
   vData[ 3 ].position.x = minx; vData[ 3 ].position.y = maxy; vData[ 3 ].position.z = 0.0; vData[ 3 ].position.w = 1.0;
   
   glBindVertexArray(gVAOId);
   
   //Update vertex buffer data
//   glBindBuffer( GL_ARRAY_BUFFER, gVBOIds[vaoId] );
   glBufferSubData( GL_ARRAY_BUFFER, 0, 4 * sizeof(struct VertexDataFormat), vData );
   
   //Draw quad using vertex data and index data
   glDrawElements( GL_TRIANGLE_STRIP, 4, GL_UNSIGNED_INT, NULL );
   //    glDrawArrays(GL_QUADS, 0,4);
   
   glBindVertexArray(0);
}

void presentBackBuffer(SDL_Renderer * renderer, SDL_Texture* backBuffer) {
   GLint oldProgramId;

   //Detach the texture
   SDL_SetRenderTarget(renderer, NULL);
   SDL_RenderClear(renderer);
   
   SDL_GL_BindTexture(backBuffer, NULL, NULL);
   if(gProgramId != 0) {
      glGetIntegerv(GL_CURRENT_PROGRAM,&oldProgramId);
      glUseProgram(gProgramId);
   }

   // Setup MVP projection matrix uniform
   glUniformMatrix4fv(gMVPSlot, 1, GL_FALSE, gOrthoMat.m);
   
   GLfloat textureSize[2];
   textureSize[0] = 320.0;
   textureSize[1] = 200.0;
   glUniform2fv(gTextureSizeSlot, 1, textureSize);

   VAO_Draw(gpRenderer, backBuffer, NULL,NULL,0);
   
   if(gProgramId != 0) {
      glUseProgram(oldProgramId);
   }
}

#if FORCE_OPENGL_CORE_PROFILE
//remove all fixed pipeline call in RenderCopy

int CORE_SetupCopy(SDL_Renderer * renderer, SDL_Texture * texture)
{
   //setup shader
   return 0;
}

int orig_SDL_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                    const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
   return SDL_RenderCopy(renderer, texture, srcrect, dstrect);
}
#define SDL_RenderCopy CORE_RenderCopy
int CORE_RenderCopy(SDL_Renderer * renderer, SDL_Texture * texture,
                    const SDL_Rect * srcrect, const SDL_Rect * dstrect)
{
#if FORCE_OPENGL_CORE_PROFILE
   if(!gConfig.fEnableGLSL)
#endif
      return orig_SDL_RenderCopy(renderer, texture, srcrect, dstrect);

   VAO_Draw(renderer, texture, srcrect, dstrect, 1);
   
   return 0;//GL_CheckError("", renderer);
}
#endif //FORCE_OPENGL_CORE_PROFILE

#endif //PAL_HAS_GLSL

#if SDL_VERSION_ATLEAST(2, 0, 0)
#define SDL_SoftStretch SDL_UpperBlit
static SDL_Texture *VIDEO_CreateTexture(int width, int height)
{
	int texture_width, texture_height;
	float ratio = (float)width / (float)height;
	ratio *= 1.6f * (float)gConfig.dwAspectY / (float)gConfig.dwAspectX;
   
#if PAL_HAS_GLSL
    gOrthoMat = GLKMatrix4MakeOrtho(0, width,height, 0,  -1, 1);
#endif
	//
	// Check whether to keep the aspect ratio
	//
	if (gConfig.fKeepAspectRatio && fabs(ratio - 1.6f) > DBL_EPSILON)
	{
		if (ratio > 1.6f)
		{
			texture_height = 200;
			texture_width = (int)(200 * ratio) & ~0x3;
			ratio = (float)height / 200.0f;
		}
		else
		{
			texture_width = 320;
			texture_height = (int)(320 / ratio) & ~0x3;
			ratio = (float)width / 320.0f;
		}

		WORD w = (WORD)(ratio * 320.0f) & ~0x3;
		WORD h = (WORD)(ratio * 200.0f) & ~0x3;
		gOverlayRect.x = (width - w) / 2;
		gOverlayRect.y = (height - h) / 2;
		gOverlayRect.w = w;
		gOverlayRect.h = h;
		gTextureRect.x = (texture_width - 320) / 2;
		gTextureRect.y = (texture_height - 200) / 2;
		gTextureRect.w = 320; gTextureRect.h = 200;
#if PAL_HAS_TOUCH
		PAL_SetTouchBounds(width, height, gOverlayRect);
#endif
	}
	else
	{
		texture_width = 320;
		texture_height = 200;
		gOverlayRect.x = gOverlayRect.y = 0;
		gOverlayRect.w = width;
		gOverlayRect.h = height;
		gTextureRect.x = gTextureRect.y = 0;
		gTextureRect.w = 320; gTextureRect.h = 200;
	}

	//
	// Create texture for screen.
	//
	return SDL_CreateTexture(gpRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_STREAMING, texture_width, texture_height);
}
#endif

INT
VIDEO_Startup(
   VOID
)
/*++
  Purpose:

    Initialze the video subsystem.

  Parameters:

    None.

  Return value:

    0 = success, -1 = fail to create the screen surface,
    -2 = fail to create screen buffer.

--*/
{
#if SDL_VERSION_ATLEAST(2,0,0)
   int render_w, render_h;
   
#if !GLES
#  if PAL_HAS_GLSL
   if( gConfig.fEnableGLSL) {
   SDL_SetHint( SDL_HINT_RENDER_DRIVER, "opengl");
#     if FORCE_OPENGL_CORE_PROFILE
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 2);
   SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
#     endif
   }
#  endif
#endif

    //
   // Before we can render anything, we need a window and a renderer.
   //
   gpWindow = SDL_CreateWindow("Pal", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                               gConfig.dwScreenWidth, gConfig.dwScreenHeight, PAL_VIDEO_INIT_FLAGS | (gConfig.fFullScreen ? SDL_WINDOW_BORDERLESS : 0) | SDL_WINDOW_OPENGL);
   SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, gConfig.pszScaleQuality);

   if (gpWindow == NULL)
   {
      return -1;
   }

   gpRenderer = SDL_CreateRenderer(gpWindow, -1, SDL_RENDERER_ACCELERATED | SDL_RENDERER_PRESENTVSYNC);
#  if PAL_HAS_GLSL
   if( gConfig.fEnableGLSL) {
      gpBackBuffer = SDL_CreateTexture(gpRenderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, gConfig.dwScreenWidth, gConfig.dwScreenHeight);
      SDL_RendererInfo rendererInfo;
      SDL_GetRendererInfo(gpRenderer, &rendererInfo);
      
      UTIL_LogOutput(LOGLEVEL_DEBUG, "render info:%s\r\n",rendererInfo.name);
      if(!strncmp(rendererInfo.name, "opengl", 6)) {
#     ifndef __APPLE__
         // If you want to use GLEW or some other GL extension handler, do it here!
         if (!initGLExtensions()) {
            UTIL_LogOutput(LOGLEVEL_DEBUG,  "Couldn't init GL extensions!\r\n" );
            SDL_Quit();
            exit(-1);
         }
#     endif
      }
      char *glversion = (char*)glGetString(GL_VERSION);
      UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_VERSION:%s\r\n",glversion);
      UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_SHADING_LANGUAGE_VERSION:%s\r\n",glGetString(GL_SHADING_LANGUAGE_VERSION));
      UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_RENDERER:%s\r\n",glGetString(GL_RENDERER));
      SDL_sscanf(glversion, "%d.%d", &glversion_major, &glversion_minor);
      if( glversion_major >= 3 ) {
         GLint n, i;
         glGetIntegerv(GL_NUM_EXTENSIONS, &n);
         for (i = 0; i < n; i++) {
            UTIL_LogOutput(LOGLEVEL_DEBUG, "extension %d:%s\n", i, glGetStringi(GL_EXTENSIONS, i));
         }
      }else
         UTIL_LogOutput(LOGLEVEL_DEBUG, "GL_EXTENSIONS:%s\r\n",glGetString(GL_EXTENSIONS));
#if FORCE_OPENGL_CORE_PROFILE
       is_SHITEL = (strstr(glversion, "INTEL")!=NULL && strstr(glversion, "WebGL")==NULL);
#endif

      struct VertexDataFormat vData[ 4 ];
      GLuint iData[ 4 ];
      GLuint vbo, ebo;
      //Set rendering indices
      iData[ 0 ] = 0;
      iData[ 1 ] = 1;
      iData[ 2 ] = 3;
      iData[ 3 ] = 2;
      // Initialize vertex array object
      glGenVertexArrays(1, &gVAOId);
      glBindVertexArray(gVAOId);
      gProgramId = compileProgram(gConfig.pszVertexShader,gConfig.pszFragmentShader);
      
      //Create VBO
      glGenBuffers( 1, &vbo );
      glBindBuffer( GL_ARRAY_BUFFER, vbo );
      glBufferData( GL_ARRAY_BUFFER, 4 * sizeof(struct VertexDataFormat), vData, GL_DYNAMIC_DRAW );
      
      //Create IBO
      glGenBuffers( 1, &ebo );
      glBindBuffer( GL_ELEMENT_ARRAY_BUFFER, ebo );
      glBufferData( GL_ELEMENT_ARRAY_BUFFER, 4 * sizeof(GLuint), iData, GL_DYNAMIC_DRAW );
      
      int slot = glGetAttribLocation(gProgramId, "VertexCoord");
      if(slot >= 0) {
         glEnableVertexAttribArray(slot);
         glVertexAttribPointer(slot, 4, GL_FLOAT, GL_FALSE, sizeof(struct VertexDataFormat), (GLvoid*)offsetof(struct VertexDataFormat, position));
      }else{
         UTIL_LogOutput(LOGLEVEL_DEBUG, "attrib VertexCoord not exist\r\n");
      }
      
      slot = glGetAttribLocation(gProgramId, "TexCoord");
      if(slot >= 0) {
         glEnableVertexAttribArray(slot);
         glVertexAttribPointer(slot, 4, GL_FLOAT, GL_FALSE, sizeof(struct VertexDataFormat), (GLvoid*)offsetof(struct VertexDataFormat, texCoord));
      }else{
         UTIL_LogOutput(LOGLEVEL_DEBUG, "attrib TexCoord not exist\r\n");
      }
      
      gMVPSlot = glGetUniformLocation(gProgramId, "MVPMatrix");
      if(gMVPSlot < 0)
         UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform MVPMatrix not exist\r\n");

      gTextureSizeSlot = glGetUniformLocation(gProgramId, "TextureSize");
      if(gTextureSizeSlot < 0)
         UTIL_LogOutput(LOGLEVEL_DEBUG, "uniform TextureSize not exist\r\n");

#if !GLES
      glBindFragDataLocation(gProgramId, 0, "FragColor");
#endif
      
      glBindVertexArray(0);
   }
#  endif //PAL_HAS_GLSL

    if (gpRenderer == NULL)
   {
      return -1;
   }

#  if defined (__IOS__)
   SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 0);
   SDL_GL_SetAttribute(SDL_GL_RETAINED_BACKING, 1);
#  endif

   //
   // Create the screen buffer and the backup screen buffer.
   //
   gpScreen = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
   gpScreenBak = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 8, 0, 0, 0, 0);
   gpScreenReal = SDL_CreateRGBSurface(SDL_SWSURFACE, 320, 200, 32,
                                       0x00FF0000, 0x0000FF00, 0x000000FF, 0xFF000000);

   //
   // Create texture for screen.
   //
   SDL_GetRendererOutputSize(gpRenderer, &render_w, &render_h);
   gpTexture = VIDEO_CreateTexture(render_w, render_h);

   //
   // Create palette object
   //
   gpPalette = SDL_AllocPalette(256);

   //
   // Failed?
   //
   if (gpScreen == NULL || gpScreenBak == NULL || gpScreenReal == NULL || gpTexture == NULL || gpPalette == NULL)
   {
      VIDEO_Shutdown();
      return -2;
   }

   //
   // Create texture for overlay.
   //
   if (gConfig.fUseTouchOverlay)
   {
      extern const void * PAL_LoadOverlayBMP(void);
      extern int PAL_OverlayBMPLength();

      SDL_Surface *overlay = SDL_LoadBMP_RW(SDL_RWFromConstMem(PAL_LoadOverlayBMP(), PAL_OverlayBMPLength()), 1);
      if (overlay != NULL)
      {
         SDL_SetColorKey(overlay, SDL_RLEACCEL, SDL_MapRGB(overlay->format, 255, 0, 255));
         gpTouchOverlay = SDL_CreateTextureFromSurface(gpRenderer, overlay);
         SDL_SetTextureAlphaMod(gpTouchOverlay, 120);
         SDL_FreeSurface(overlay);
      }
   }
#else

   //
   // Create the screen surface.
   //
   gpScreenReal = SDL_SetVideoMode(gConfig.dwScreenWidth, gConfig.dwScreenHeight, 8, PAL_VIDEO_INIT_FLAGS);

   if (gpScreenReal == NULL)
   {
      //
      // Fall back to 640x480 software mode.
      //
      gpScreenReal = SDL_SetVideoMode(640, 480, 8,
         SDL_SWSURFACE | (gConfig.fFullScreen ? SDL_FULLSCREEN : 0));
   }

   //
   // Still fail?
   //
   if (gpScreenReal == NULL)
   {
      return -1;
   }

   gpPalette = gpScreenReal->format->palette;

   //
   // Create the screen buffer and the backup screen buffer.
   //
   gpScreen = SDL_CreateRGBSurface(gpScreenReal->flags & ~SDL_HWSURFACE, 320, 200, 8,
      gpScreenReal->format->Rmask, gpScreenReal->format->Gmask,
      gpScreenReal->format->Bmask, gpScreenReal->format->Amask);

   gpScreenBak = SDL_CreateRGBSurface(gpScreenReal->flags & ~SDL_HWSURFACE, 320, 200, 8,
      gpScreenReal->format->Rmask, gpScreenReal->format->Gmask,
      gpScreenReal->format->Bmask, gpScreenReal->format->Amask);

   //
   // Failed?
   //
   if (gpScreen == NULL || gpScreenBak == NULL)
   {
      VIDEO_Shutdown();
      return -2;
   }

   if (gConfig.fFullScreen)
   {
      SDL_ShowCursor(FALSE);
   }

#endif

   return 0;
}

VOID
VIDEO_Shutdown(
   VOID
)
/*++
  Purpose:

    Shutdown the video subsystem.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   if (gpScreen != NULL)
   {
      SDL_FreeSurface(gpScreen);
   }
   gpScreen = NULL;

   if (gpScreenBak != NULL)
   {
      SDL_FreeSurface(gpScreenBak);
   }
   gpScreenBak = NULL;

#if SDL_VERSION_ATLEAST(2,0,0)

   if (gpTouchOverlay)
   {
      SDL_DestroyTexture(gpTouchOverlay);
   }
   gpTouchOverlay = NULL;

   if (gpTexture)
   {
	  SDL_DestroyTexture(gpTexture);
   }
   gpTexture = NULL;

#if PAL_HAS_GLSL
   if (gpBackBuffer)
   {
      SDL_DestroyTexture(gpBackBuffer);
   }
   gpBackBuffer = NULL;
#endif

   if (gpRenderer)
   {
      SDL_DestroyRenderer(gpRenderer);
   }
   gpRenderer = NULL;

   if (gpWindow)
   {
      SDL_DestroyWindow(gpWindow);
   }
   gpWindow = NULL;

   if (gpPalette)
   {
      SDL_FreePalette(gpPalette);
   }
#endif
   gpPalette = NULL;

   if (gpScreenReal != NULL)
   {
      SDL_FreeSurface(gpScreenReal);
   }
   gpScreenReal = NULL;
}

#if SDL_VERSION_ATLEAST(2,0,0)

PAL_FORCE_INLINE
VOID
VIDEO_RenderCopy(
   VOID
)
{
	void *texture_pixels;
	int texture_pitch;

	SDL_LockTexture(gpTexture, NULL, &texture_pixels, &texture_pitch);
	memset(texture_pixels, 0, gTextureRect.y * texture_pitch);
	uint8_t *pixels = (uint8_t *)texture_pixels + gTextureRect.y * texture_pitch;
	uint8_t *src = (uint8_t *)gpScreenReal->pixels;
	int left_pitch = gTextureRect.x << 2;
	int right_pitch = texture_pitch - ((gTextureRect.x + gTextureRect.w) << 2);
	for (int y = 0; y < gTextureRect.h; y++, src += gpScreenReal->pitch)
	{
		memset(pixels, 0, left_pitch); pixels += left_pitch;
		memcpy(pixels, src, 320 << 2); pixels += 320 << 2;
		memset(pixels, 0, right_pitch); pixels += right_pitch;
	}
	memset(pixels, 0, gTextureRect.y * texture_pitch);
	SDL_UnlockTexture(gpTexture);

#if !GLES //INVALID_VALUE: bufferSubData: offset out of range
#if PAL_HAS_GLSL
    if( gConfig.fEnableGLSL) {
        SDL_SetRenderTarget(gpRenderer, gpBackBuffer);
        SDL_RenderClear(gpRenderer);
    }
#endif
   SDL_RenderCopy(gpRenderer, gpTexture, NULL, NULL);
   if (gpTouchOverlay)
   {
      SDL_RenderCopy(gpRenderer, gpTouchOverlay, NULL, &gOverlayRect);
   }
#endif
#if PAL_HAS_GLSL
    if( gConfig.fEnableGLSL) {
       presentBackBuffer(gpRenderer,
#if FORCE_OPENGL_CORE_PROFILE || GLES//need figure howto implement RTT in macOS Core Profile
                         gpTexture
#else
                         gpBackBuffer
#endif
                         );
       SDL_GL_SwapWindow(gpWindow);
    }else
#endif
    {
#if FORCE_OPENGL_CORE_PROFILE
       SDL_RenderCopy(gpRenderer, gpTexture, NULL, NULL);
       if (gpTouchOverlay)
       {
          SDL_RenderCopy(gpRenderer, gpTouchOverlay, NULL, &gOverlayRect);
       }
#endif
       SDL_RenderPresent(gpRenderer);
    }
}
#endif //SDL2

VOID
VIDEO_UpdateScreen(
   const SDL_Rect  *lpRect
)
/*++
  Purpose:

    Update the screen area specified by lpRect.

  Parameters:

    [IN]  lpRect - Screen area to update.

  Return value:

    None.

--*/
{
   SDL_Rect        srcrect, dstrect;
   short           offset = 240 - 200;
   short           screenRealHeight = gpScreenReal->h;
   short           screenRealY = 0;

#if SDL_VERSION_ATLEAST(2,0,0)
   if (g_bRenderPaused)
   {
	   return;
   }
#endif

   //
   // Lock surface if needed
   //
   if (SDL_MUSTLOCK(gpScreenReal))
   {
      if (SDL_LockSurface(gpScreenReal) < 0)
         return;
   }

   if (!bScaleScreen)
   {
      screenRealHeight -= offset;
      screenRealY = offset / 2;
   }

   if (lpRect != NULL)
   {
      dstrect.x = (SHORT)((INT)(lpRect->x) * gpScreenReal->w / gpScreen->w);
      dstrect.y = (SHORT)((INT)(screenRealY + lpRect->y) * screenRealHeight / gpScreen->h);
      dstrect.w = (WORD)((DWORD)(lpRect->w) * gpScreenReal->w / gpScreen->w);
      dstrect.h = (WORD)((DWORD)(lpRect->h) * screenRealHeight / gpScreen->h);

      SDL_SoftStretch(gpScreen, (SDL_Rect *)lpRect, gpScreenReal, &dstrect);
   }
   else if (g_wShakeTime != 0)
   {
      //
      // Shake the screen
      //
      srcrect.x = 0;
      srcrect.y = 0;
      srcrect.w = 320;
      srcrect.h = 200 - g_wShakeLevel;

      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = 320 * gpScreenReal->w / gpScreen->w;
      dstrect.h = (200 - g_wShakeLevel) * screenRealHeight / gpScreen->h;

      if (g_wShakeTime & 1)
      {
         srcrect.y = g_wShakeLevel;
      }
      else
      {
         dstrect.y = (screenRealY + g_wShakeLevel) * screenRealHeight / gpScreen->h;
      }

      SDL_SoftStretch(gpScreen, &srcrect, gpScreenReal, &dstrect);

      if (g_wShakeTime & 1)
      {
         dstrect.y = (screenRealY + screenRealHeight - g_wShakeLevel) * screenRealHeight / gpScreen->h;
      }
      else
      {
         dstrect.y = screenRealY;
      }

      dstrect.h = g_wShakeLevel * screenRealHeight / gpScreen->h;

      SDL_FillRect(gpScreenReal, &dstrect, 0);

#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION <= 2
      dstrect.x = dstrect.y = 0;
      dstrect.w = gpScreenReal->w;
      dstrect.h = gpScreenReal->h;
#endif
      g_wShakeTime--;
   }
   else
   {
      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = gpScreenReal->w;
      dstrect.h = screenRealHeight;

      SDL_SoftStretch(gpScreen, NULL, gpScreenReal, &dstrect);

#if SDL_MAJOR_VERSION == 1 && SDL_MINOR_VERSION <= 2
      dstrect.x = dstrect.y = 0;
      dstrect.w = gpScreenReal->w;
      dstrect.h = gpScreenReal->h;
#endif
   }

#if SDL_VERSION_ATLEAST(2,0,0)
   VIDEO_RenderCopy();
#else
   SDL_UpdateRect(gpScreenReal, dstrect.x, dstrect.y, dstrect.w, dstrect.h);
#endif

   if (SDL_MUSTLOCK(gpScreenReal))
   {
	   SDL_UnlockSurface(gpScreenReal);
   }
}

VOID
VIDEO_SetPalette(
   SDL_Color        rgPalette[256]
)
/*++
  Purpose:

    Set the palette of the screen.

  Parameters:

    [IN]  rgPalette - array of 256 colors.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2,0,0)
   SDL_Rect rect;

   SDL_SetPaletteColors(gpPalette, rgPalette, 0, 256);

   SDL_SetSurfacePalette(gpScreen, gpPalette);
   SDL_SetSurfacePalette(gpScreenBak, gpPalette);

   //
   // HACKHACK: need to invalidate gpScreen->map otherwise the palette
   // would not be effective during blit
   //
   SDL_SetSurfaceColorMod(gpScreen, 0, 0, 0);
   SDL_SetSurfaceColorMod(gpScreen, 0xFF, 0xFF, 0xFF);
   SDL_SetSurfaceColorMod(gpScreenBak, 0, 0, 0);
   SDL_SetSurfaceColorMod(gpScreenBak, 0xFF, 0xFF, 0xFF);

   rect.x = 0;
   rect.y = 0;
   rect.w = 320;
   rect.h = 200;

   VIDEO_UpdateScreen(&rect);
#else
   SDL_SetPalette(gpScreen, SDL_LOGPAL | SDL_PHYSPAL, rgPalette, 0, 256);
   SDL_SetPalette(gpScreenBak, SDL_LOGPAL | SDL_PHYSPAL, rgPalette, 0, 256);
   SDL_SetPalette(gpScreenReal, SDL_LOGPAL | SDL_PHYSPAL, rgPalette, 0, 256);
# if defined(PAL_FORCE_UPDATE_ON_PALETTE_SET)
   {
      static UINT32 time = 0;
      if (SDL_GetTicks() - time > 50)
      {
	      SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
	      time = SDL_GetTicks();
      }
   }
# endif
#endif
}

VOID
VIDEO_Resize(
   INT             w,
   INT             h
)
/*++
  Purpose:

    This function is called when user resized the window.

  Parameters:

    [IN]  w - width of the window after resizing.

    [IN]  h - height of the window after resizing.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2,0,0)
   SDL_Rect rect;

   if (gpTexture)
   {
      SDL_DestroyTexture(gpTexture);
   }

   gpTexture = VIDEO_CreateTexture(w, h);
   
#if PAL_HAS_GLSL
   if (gpBackBuffer)
   {
      SDL_DestroyTexture(gpBackBuffer);
   }
   gpBackBuffer = SDL_CreateTexture(gpRenderer, SDL_PIXELFORMAT_ARGB8888,                                                  SDL_TEXTUREACCESS_TARGET, w, h);
#endif

   if (gpTexture == NULL)
   {
      TerminateOnError("Re-creating texture failed on window resize!\n");
   }

   rect.x = 0;
   rect.y = 0;
   rect.w = 320;
   rect.h = 200;

   VIDEO_UpdateScreen(&rect);
#else
   DWORD                    flags;
   PAL_LARGE SDL_Color      palette[256];
   int                      i, bpp;

   //
   // Get the original palette.
   //
   if (gpScreenReal->format->palette != NULL)
   {
      for (i = 0; i < gpScreenReal->format->palette->ncolors; i++)
      {
         palette[i] = gpScreenReal->format->palette->colors[i];
      }
   }
   else i = 0;

   //
   // Create the screen surface.
   //
   flags = gpScreenReal->flags;
   bpp = gpScreenReal->format->BitsPerPixel;

   SDL_FreeSurface(gpScreenReal);
   gpScreenReal = SDL_SetVideoMode(w, h, bpp, flags);

   if (gpScreenReal == NULL)
   {
      //
      // Fall back to software windowed mode in default size.
      //
      gpScreenReal = SDL_SetVideoMode(PAL_DEFAULT_WINDOW_WIDTH, PAL_DEFAULT_WINDOW_HEIGHT, bpp, SDL_SWSURFACE);
   }

   SDL_SetPalette(gpScreenReal, SDL_PHYSPAL | SDL_LOGPAL, palette, 0, i);
   VIDEO_UpdateScreen(NULL);

   gpPalette = gpScreenReal->format->palette;
#endif
}

SDL_Color *
VIDEO_GetPalette(
   VOID
)
/*++
  Purpose:

    Get the current palette of the screen.

  Parameters:

    None.

  Return value:

    Pointer to the current palette.

--*/
{
   return gpPalette->colors;
}

VOID
VIDEO_ToggleScaleScreen(
   VOID
)
/*++
  Purpose:

    Toggle scalescreen mode, only used in some platforms.

  Parameters:

    None.

  Return value:

    None.

--*/
{
   bScaleScreen = !bScaleScreen;
   VIDEO_Resize(PAL_DEFAULT_WINDOW_WIDTH, PAL_DEFAULT_WINDOW_HEIGHT);
   VIDEO_UpdateScreen(NULL);
}

VOID
VIDEO_ToggleFullscreen(
   VOID
)
/*++
  Purpose:

    Toggle fullscreen mode.

  Parameters:

    None.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2,0,0)
	if (gConfig.fFullScreen)
	{
		SDL_SetWindowFullscreen(gpWindow, 0);
		gConfig.fFullScreen = FALSE;
	}
	else
	{
		SDL_SetWindowFullscreen(gpWindow, SDL_WINDOW_FULLSCREEN_DESKTOP);
		gConfig.fFullScreen = TRUE;
	}
#else
   DWORD                    flags;
   PAL_LARGE SDL_Color      palette[256];
   int                      i, bpp;

   //
   // Get the original palette.
   //
   if (gpScreenReal->format->palette != NULL)
   {
      for (i = 0; i < gpScreenReal->format->palette->ncolors; i++)
      {
         palette[i] = gpScreenReal->format->palette->colors[i];
      }
   }

   //
   // Get the flags and bpp of the original screen surface
   //
   flags = gpScreenReal->flags;
   bpp = gpScreenReal->format->BitsPerPixel;

   if (flags & SDL_FULLSCREEN)
   {
      //
      // Already in fullscreen mode. Remove the fullscreen flag.
      //
      flags &= ~SDL_FULLSCREEN;
      flags |= SDL_RESIZABLE;
      SDL_ShowCursor(TRUE);
   }
   else
   {
      //
      // Not in fullscreen mode. Set the fullscreen flag.
      //
      flags |= SDL_FULLSCREEN;
      SDL_ShowCursor(FALSE);
   }

   //
   // Free the original screen surface
   //
   SDL_FreeSurface(gpScreenReal);

   //
   // ... and create a new one
   //
   if (gConfig.dwScreenWidth == 640 && gConfig.dwScreenHeight == 400 && (flags & SDL_FULLSCREEN))
   {
      gpScreenReal = SDL_SetVideoMode(640, 480, bpp, flags);
   }
   else if (gConfig.dwScreenWidth == 640 && gConfig.dwScreenHeight == 480 && !(flags & SDL_FULLSCREEN))
   {
      gpScreenReal = SDL_SetVideoMode(640, 400, bpp, flags);
   }
   else
   {
      gpScreenReal = SDL_SetVideoMode(gConfig.dwScreenWidth, gConfig.dwScreenHeight, bpp, flags);
   }

   VIDEO_SetPalette(palette);

   //
   // Update the screen
   //
   VIDEO_UpdateScreen(NULL);
#endif
}

VOID
VIDEO_ChangeDepth(
   INT             bpp
)
/*++
  Purpose:

    Change screen color depth.

  Parameters:

    [IN]  bpp - bits per pixel (0 = default).

  Return value:

    None.

--*/
{
#if !SDL_VERSION_ATLEAST(2,0,0)
   DWORD                    flags;
   int                      w, h;

   //
   // Get the flags and resolution of the original screen surface
   //
   flags = gpScreenReal->flags;
   w = gpScreenReal->w;
   h = gpScreenReal->h;

   //
   // Clear the screen surface.
   //
   SDL_FillRect(gpScreenReal, NULL, 0);

   //
   // Create the screen surface.
   //
   SDL_FreeSurface(gpScreenReal);
   gpScreenReal = SDL_SetVideoMode(w, h, (bpp == 0)?8:bpp, flags);

   if (gpScreenReal == NULL)
   {
      //
      // Fall back to software windowed mode in default size.
      //
      gpScreenReal = SDL_SetVideoMode(PAL_DEFAULT_WINDOW_WIDTH, PAL_DEFAULT_WINDOW_HEIGHT, (bpp == 0)?8:bpp, SDL_SWSURFACE);
   }

   gpPalette = gpScreenReal->format->palette;
#endif
}

VOID
VIDEO_SaveScreenshot(
   VOID
)
/*++
  Purpose:

    Save the screenshot of current screen to a BMP file.

  Parameters:

    None.

  Return value:

    None.

--*/
{
	char filename[32];
#ifdef _WIN32
	SYSTEMTIME st;
	GetLocalTime(&st);
	sprintf(filename, "%04d%02d%02d%02d%02d%02d%03d.bmp", st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond, st.wMilliseconds);
#elif !defined( GEKKO )
	struct timeval tv;
	struct tm *ptm;
	gettimeofday(&tv, NULL);
	ptm = localtime(&tv.tv_sec);
	sprintf(filename, "%04d%02d%02d%02d%02d%02d%03d.bmp", ptm->tm_year + 1900, ptm->tm_mon, ptm->tm_mday, ptm->tm_hour, ptm->tm_min, ptm->tm_sec, (int)(tv.tv_usec / 1000));
#endif

	//
	// Save the screenshot.
	//
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SaveBMP(gpScreen, PAL_CombinePath(0, gConfig.pszSavePath, filename));
#else
	SDL_SaveBMP(gpScreenReal, PAL_CombinePath(0, gConfig.pszSavePath, filename));
#endif
}

VOID
VIDEO_ShakeScreen(
   WORD           wShakeTime,
   WORD           wShakeLevel
)
/*++
  Purpose:

    Set the screen shake time and level.

  Parameters:

    [IN]  wShakeTime - how many times should we shake the screen.

    [IN]  wShakeLevel - level of shaking.

  Return value:

    None.

--*/
{
   g_wShakeTime = wShakeTime;
   g_wShakeLevel = wShakeLevel;
}

VOID
VIDEO_SwitchScreen(
   WORD           wSpeed
)
/*++
  Purpose:

    Switch the screen from the backup screen buffer to the current screen buffer.
    NOTE: This will destroy the backup buffer.

  Parameters:

    [IN]  wSpeed - speed of fading (the larger value, the slower).

  Return value:

    None.

--*/
{
   int               i, j;
   const int         rgIndex[6] = {0, 3, 1, 5, 2, 4};
   SDL_Rect          dstrect;

   short             offset = 240 - 200;
   short             screenRealHeight = gpScreenReal->h;
   short             screenRealY = 0;

   if (!bScaleScreen)
   {
      screenRealHeight -= offset;
      screenRealY = offset / 2;
   }

   wSpeed++;
   wSpeed *= 10;

   for (i = 0; i < 6; i++)
   {
      for (j = rgIndex[i]; j < gpScreen->pitch * gpScreen->h; j += 6)
      {
         ((LPBYTE)(gpScreenBak->pixels))[j] = ((LPBYTE)(gpScreen->pixels))[j];
      }

      //
      // Draw the backup buffer to the screen
      //
      dstrect.x = 0;
      dstrect.y = screenRealY;
      dstrect.w = gpScreenReal->w;
      dstrect.h = screenRealHeight;

	  if (SDL_MUSTLOCK(gpScreenReal))
	  {
		  if (SDL_LockSurface(gpScreenReal) < 0)
			  return;
	  }

      SDL_SoftStretch(gpScreenBak, NULL, gpScreenReal, &dstrect);
#if SDL_VERSION_ATLEAST(2, 0, 0)
      VIDEO_RenderCopy();
#else
      SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
#endif

	  if (SDL_MUSTLOCK(gpScreenReal))
	  {
		  SDL_UnlockSurface(gpScreenReal);
	  }

      UTIL_Delay(wSpeed);
   }
}

VOID
VIDEO_FadeScreen(
   WORD           wSpeed
)
/*++
  Purpose:

    Fade from the backup screen buffer to the current screen buffer.
    NOTE: This will destroy the backup buffer.

  Parameters:

    [IN]  wSpeed - speed of fading (the larger value, the slower).

  Return value:

    None.

--*/
{
   int               i, j, k;
   DWORD             time;
   BYTE              a, b;
   const int         rgIndex[6] = {0, 3, 1, 5, 2, 4};
   SDL_Rect          dstrect;
   short             offset = 240 - 200;
   short             screenRealHeight = gpScreenReal->h;
   short             screenRealY = 0;

   //
   // Lock surface if needed
   //
   if (SDL_MUSTLOCK(gpScreenReal))
   {
      if (SDL_LockSurface(gpScreenReal) < 0)
         return;
   }

   if (!bScaleScreen)
   {
      screenRealHeight -= offset;
      screenRealY = offset / 2;
   }

   time = SDL_GetTicks();

   wSpeed++;
   wSpeed *= 10;

   for (i = 0; i < 12; i++)
   {
      for (j = 0; j < 6; j++)
      {
         PAL_ProcessEvent();
         while (!SDL_TICKS_PASSED(SDL_GetTicks(), time))
         {
            PAL_ProcessEvent();
            SDL_Delay(5);
         }
         time = SDL_GetTicks() + wSpeed;

         //
         // Blend the pixels in the 2 buffers, and put the result into the
         // backup buffer
         //
         for (k = rgIndex[j]; k < gpScreen->pitch * gpScreen->h; k += 6)
         {
            a = ((LPBYTE)(gpScreen->pixels))[k];
            b = ((LPBYTE)(gpScreenBak->pixels))[k];

            if (i > 0)
            {
               if ((a & 0x0F) > (b & 0x0F))
               {
                  b++;
               }
               else if ((a & 0x0F) < (b & 0x0F))
               {
                  b--;
               }
            }

            ((LPBYTE)(gpScreenBak->pixels))[k] = ((a & 0xF0) | (b & 0x0F));
         }

         //
         // Draw the backup buffer to the screen
         //
         if (g_wShakeTime != 0)
         {
            //
            // Shake the screen
            //
            SDL_Rect srcrect, dstrect;

            srcrect.x = 0;
            srcrect.y = 0;
            srcrect.w = 320;
            srcrect.h = 200 - g_wShakeLevel;

            dstrect.x = 0;
            dstrect.y = screenRealY;
            dstrect.w = 320 * gpScreenReal->w / gpScreen->w;
            dstrect.h = (200 - g_wShakeLevel) * screenRealHeight / gpScreen->h;

            if (g_wShakeTime & 1)
            {
               srcrect.y = g_wShakeLevel;
            }
            else
            {
               dstrect.y = (screenRealY + g_wShakeLevel) * screenRealHeight / gpScreen->h;
            }

            SDL_SoftStretch(gpScreenBak, &srcrect, gpScreenReal, &dstrect);

            if (g_wShakeTime & 1)
            {
               dstrect.y = (screenRealY + screenRealHeight - g_wShakeLevel) * screenRealHeight / gpScreen->h;
            }
            else
            {
               dstrect.y = screenRealY;
            }

            dstrect.h = g_wShakeLevel * screenRealHeight / gpScreen->h;

            SDL_FillRect(gpScreenReal, &dstrect, 0);
#if SDL_VERSION_ATLEAST(2, 0, 0)
            VIDEO_RenderCopy();
#else
			SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
#endif
            g_wShakeTime--;
         }
         else
         {
            dstrect.x = 0;
            dstrect.y = screenRealY;
            dstrect.w = gpScreenReal->w;
            dstrect.h = screenRealHeight;

            SDL_SoftStretch(gpScreenBak, NULL, gpScreenReal, &dstrect);
#if SDL_VERSION_ATLEAST(2, 0, 0)
            VIDEO_RenderCopy();
#else
            SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
#endif
         }
      }
   }

   if (SDL_MUSTLOCK(gpScreenReal))
   {
      SDL_UnlockSurface(gpScreenReal);
   }

   //
   // Draw the result buffer to the screen as the final step
   //
   VIDEO_UpdateScreen(NULL);
}

void
VIDEO_SetWindowTitle(
	const char*     pszTitle
)
/*++
  Purpose:

    Set the caption of the window.

  Parameters:

    [IN]  pszTitle - the new caption of the window.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2,0,0)
	SDL_SetWindowTitle(gpWindow, pszTitle);
#else
	SDL_WM_SetCaption(pszTitle, NULL);
#endif
}

SDL_Surface *
VIDEO_CreateCompatibleSurface(
	SDL_Surface    *pSource
)
{
	return VIDEO_CreateCompatibleSizedSurface(pSource, NULL);
}

SDL_Surface *
VIDEO_CreateCompatibleSizedSurface(
	SDL_Surface    *pSource,
	const SDL_Rect *pSize
)
/*++
  Purpose:

    Create a surface that compatible with the source surface.

  Parameters:

    [IN]  pSource   - the source surface from which attributes are taken.
    [IN]  pSize     - the size (width & height) of the created surface.

  Return value:

    None.

--*/
{
	//
	// Create the surface
	//
	SDL_Surface *dest = SDL_CreateRGBSurface(pSource->flags,
		pSize ? pSize->w : pSource->w,
		pSize ? pSize->h : pSource->h,
		pSource->format->BitsPerPixel,
		pSource->format->Rmask, pSource->format->Gmask,
		pSource->format->Bmask, pSource->format->Amask);

	if (dest)
	{
		VIDEO_UpdateSurfacePalette(dest);
	}

	return dest;
}

SDL_Surface *
VIDEO_DuplicateSurface(
	SDL_Surface    *pSource,
	const SDL_Rect *pRect
)
/*++
  Purpose:

    Duplicate the selected area from the source surface into new surface.

  Parameters:

    [IN]  pSource - the source surface.
	[IN]  pRect   - the area to be duplicated, NULL for entire surface.

  Return value:

    None.

--*/
{
	SDL_Surface* dest = VIDEO_CreateCompatibleSizedSurface(pSource, pRect);

	if (dest)
	{
		VIDEO_CopySurface(pSource, pRect, dest, NULL);
	}

	return dest;
}

void
VIDEO_UpdateSurfacePalette(
	SDL_Surface    *pSurface
)
/*++
  Purpose:

    Use the global palette to update the palette of pSurface.

  Parameters:

    [IN]  pSurface - the surface whose palette should be updated.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
	SDL_SetSurfacePalette(pSurface, gpPalette);
#else
	if (gpPalette != NULL)
	{
		SDL_SetPalette(pSurface, SDL_PHYSPAL | SDL_LOGPAL, gpPalette->colors, 0, 256);
	}
#endif
}

VOID
VIDEO_DrawSurfaceToScreen(
    SDL_Surface    *pSurface
)
/*++
  Purpose:

    Draw a surface directly to screen.

  Parameters:

    [IN]  pSurface - the surface which needs to be drawn to screen.

  Return value:

    None.

--*/
{
#if SDL_VERSION_ATLEAST(2, 0, 0)
   //
   // Draw the surface to screen.
   //
   if (g_bRenderPaused)
   {
      return;
   }
   SDL_BlitScaled(pSurface, NULL, gpScreenReal, NULL);
   VIDEO_RenderCopy();
#else
   SDL_Surface   *pCompatSurface;
   SDL_Rect       rect;

   rect.x = rect.y = 0;
   rect.w = pSurface->w;
   rect.h = pSurface->h;

   pCompatSurface = VIDEO_CreateCompatibleSizedSurface(gpScreenReal, &rect);

   //
   // First convert the surface to compatible format.
   //
   SDL_BlitSurface(pSurface, NULL, pCompatSurface, NULL);

   //
   // Draw the surface to screen.
   //
   SDL_SoftStretch(pCompatSurface, NULL, gpScreenReal, NULL);

   SDL_UpdateRect(gpScreenReal, 0, 0, gpScreenReal->w, gpScreenReal->h);
   SDL_FreeSurface(pCompatSurface);
#endif
}
