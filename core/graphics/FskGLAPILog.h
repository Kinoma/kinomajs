/*
 *     Copyright (C) 2002-2015 Kinoma, Inc.
 *
 *     All rights reserved.
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 *
 */

#ifndef __FSKGLAPILOG__
#define __FSKGLAPILOG__

static const char *_GLstr(int code) {
	struct _str_tab { int code; const char *str; };
	static const struct _str_tab tab[] = {
		{	GL_ALPHA,						"ALPHA"					},
		{	GL_BLEND,						"BLEND"					},
		{	GL_CLAMP_TO_EDGE,				"CLAMP_TO_EDGE"			},
		{	GL_COLOR_ATTACHMENT0,			"COLOR_ATTACHMENT0"		},
		{	GL_CONSTANT_ALPHA,				"CONST_ALPHA"			},
		{	GL_CONSTANT_COLOR,				"CONST_COLOR"			},
		{	GL_DST_ALPHA,					"DST_ALPHA"				},
		{	GL_DST_COLOR,					"DST_COLOR"				},
		{	GL_FLOAT,						"FLOAT"					},
		{	GL_FRAGMENT_SHADER,				"FRAGMENT_SHADER",		},
		{	GL_FRAMEBUFFER,					"FRAMEBUFFER"			},
		{	GL_LINEAR,						"LINEAR"				},
		{	GL_LUMINANCE_ALPHA,				"LUMINANCE_ALPHA"		},
		{	GL_MIRRORED_REPEAT,				"MIRRORED_REPEAT"		},
		{	GL_NEAREST,						"NEAREST"				},
		{	GL_ONE,							"1"						},
		{	GL_ONE_MINUS_CONSTANT_ALPHA,	"1-CONST_ALPHA"			},
		{	GL_ONE_MINUS_CONSTANT_COLOR,	"1-CONST_COLOR"			},
		{	GL_ONE_MINUS_DST_ALPHA,			"1-DST_ALPHA"			},
		{	GL_ONE_MINUS_DST_COLOR,			"1-DST_COLOR"			},
		{	GL_ONE_MINUS_SRC_ALPHA,			"1-SRC_ALPHA"			},
		{	GL_ONE_MINUS_SRC_COLOR,			"1-SRC_COLOR"			},
		{	GL_PACK_ALIGNMENT,				"PACK_ALIGNMENT"		},
		{	GL_REPEAT,						"REPEAT"				},
		{	GL_RGB,							"RGB"					},
		{	GL_RGBA,						"RGBA"					},
		{	GL_SCISSOR_TEST,				"SCISSOR_TEST"			},
		{	GL_SRC_ALPHA,					"SRC_ALPHA"				},
		{	GL_SRC_ALPHA_SATURATE,			"SRC_ALPHA_SATURATE"	},
		{	GL_SRC_COLOR,					"SRC_COLOR"				},
		{	GL_TEXTURE0,					"TEXTURE0"				},
		{	GL_TEXTURE1,					"TEXTURE1"				},
		{	GL_TEXTURE2,					"TEXTURE2"				},
		{	GL_TEXTURE_2D,					"TEXTURE_2D"			},
		{	GL_TEXTURE_MAG_FILTER,			"TEXTURE_MAG_FILTER"	},
		{	GL_TEXTURE_MIN_FILTER,			"TEXTURE_MIN_FILTER"	},
		{	GL_TEXTURE_WRAP_S,				"TEXTURE_WRAP_S"		},
		{	GL_TEXTURE_WRAP_T,				"TEXTURE_WRAP_T"		},
		{	GL_UNPACK_ALIGNMENT,			"UNPACK_ALIGNMENT"		},
		{	GL_UNSIGNED_BYTE,				"UNSIGNED_BYTE"			},
		{	GL_VERTEX_SHADER,				"VERTEX_SHADER"			},
		{	GL_ZERO,						"0"						},
		{	GL_TRIANGLES,					"TRIANGLES"				},
		{	GL_TRIANGLE_STRIP,				"TRIANGLE_STRIP"		},
		{	GL_TRIANGLE_FAN,				"TRIANGLE_FAN"			},

#ifdef GL_BACK
		{	GL_BACK,						"GL_BACK"				},
#endif
#ifdef GL_BGR
		{	GL_BGR,							"BGR"					},
#endif
#ifdef GL_BGRA
		{	GL_BGRA,						"BGRA"					},
#endif
#ifdef GL_FLAT
		{	GL_FLAT,						"FLAT"					},
#endif
#ifdef GL_FRONT
		{	GL_FRONT,						"GL_FRONT"				},
#endif
#ifdef GL_LIGHTING
		{	GL_LIGHTING,					"LIGHTING"				},
#endif
#ifdef GL_PACK_ROW_LENGTH
		{	GL_PACK_ROW_LENGTH,				"PACK_ROW_LENGTH"		},
#endif
#ifdef GL_RGB_422_APPLE
		{	GL_RGB_422_APPLE,				"RGB_422_APPLE"			},
#endif
#ifdef GL_UNPACK_ROW_LENGTH
		{	GL_UNPACK_ROW_LENGTH,			"UNPACK_ROW_LENGTH"		},
#endif
#ifdef GL_UNPACK_SKIP_PIXELS
		{	GL_UNPACK_SKIP_PIXELS,			"UNPACK_SKIP_PIXELS"	},
#endif
#ifdef GL_UNPACK_SKIP_ROWS
		{	GL_UNPACK_SKIP_ROWS,			"UNPACK_SKIP_ROWS"		},
#endif
#ifdef GL_UNSIGNED_SHORT_8_8_APPLE
		{	GL_UNSIGNED_SHORT_8_8_APPLE,	"UNSIGNED_SHORT_8_8_APPLE"	},
#endif
#ifdef GL_UNSIGNED_INT_8_8_8_8
		{	GL_UNSIGNED_INT_8_8_8_8,		"UNSIGNED_INT_8_8_8_8"		},
#endif
#ifdef GL_UNSIGNED_INT_8_8_8_8_REV
		{	GL_UNSIGNED_INT_8_8_8_8_REV,	"UNSIGNED_INT_8_8_8_8_REV"	},
#endif
#ifdef GL_UNSIGNED_SHORT_4_4_4_4
		{	GL_UNSIGNED_SHORT_4_4_4_4,		"UNSIGNED_SHORT_4_4_4_4"	},
#endif
#ifdef GL_UNSIGNED_SHORT_5_6_5
		{	GL_UNSIGNED_SHORT_5_6_5,		"UNSIGNED_SHORT_5_6_5"		},
#endif
#ifdef GL_UNSIGNED_SHORT_5_6_5_REV
		{	GL_UNSIGNED_SHORT_5_6_5_REV,	"UNSIGNED_SHORT_5_6_5_REV"	},
#endif
#ifdef GL_UNSIGNED_SHORT_8_8_REV_APPLE
		{	GL_UNSIGNED_SHORT_8_8_REV_APPLE,"UNSIGNED_SHORT_8_8_REV_APPLE"	},
#endif
#ifdef GL_YCBCR_422_APPLE
		{	GL_YCBCR_422_APPLE,				"YCBCR_422_APPLE"			},
#endif

	};
	const struct _str_tab *p;
	static char buf[10];
	for (p = tab; p < tab + sizeof(tab)/sizeof(tab[0]); ++p)
		if (code == p->code)
			return p->str;
	snprintf(buf, sizeof(buf), "%#x", code);
	return buf;
}


/* Open GL API */
#define glActiveTexture(t)									(LOGD("glActiveTexture(%s)",_GLstr(t)),									glActiveTexture(t))
#define glAttachShader(p,s)									(LOGD("glAttachShader(%u,%u)",p,s),										glAttachShader(p,s))
#define glBindAttribLocation(p,i,n)							(LOGD("glBindAttribLocation(%u,%u,%p)",p,i,n),							glBindAttribLocation(p,i,n))
#define glBindFramebuffer(t,f)								(LOGD("glBindFramebuffer(%s,%u)",_GLstr(t),f),							glBindFramebuffer(t,f))
#define glBindTexture(r,x)									(LOGD("glBindTexture(%s, #%u)",_GLstr(r),x),							glBindTexture(r,x))
#define glBlendColor(r,g,b,a)								(LOGD("glBlendColor(%g,%g,%g,%g)",r,g,b,a),								glBlendColor(r,g,b,a))
#define glBlendFunc(s,d)									(LOGD("glBlendFunc(%s,%s)",_GLstr(s),_GLstr(d)),						glBlendFunc(s,d))
#define glBlendFuncSeparate(s,d,r,e)						(LOGD("glBlendFuncSeparate(%s,%s,%s,%s)",_GLstr(s),_GLstr(d),_GLstr(r),_GLstr(e)),	glBlendFuncSeparate(s,d,r,e))
#define glCheckFramebufferStatus(t)							(LOGD("glCheckFramebufferStatus(%s)",_GLstr(t)),						glCheckFramebufferStatus(t))
#define glClear(m)											(LOGD("glClear(%#x)",m),												glClear(m))
#define glClearColor(r,g,b,a)								(LOGD("glClearColor(%g,%g,%g,%g)",r,g,b,a),								glClearColor(r,g,b,a))
#define glClearColorx(r,g,b,a)								(LOGD("glClearColorx(%d,%d,%d,%d)",r,g,b,a),							glClearColorx(r,g,b,a))
#define glColor4ub(r,g,b,a)									(LOGD("glColor4ub(%d,%d,%d,%d)",r,g,b,a),								glColor4ub(r,g,b,a))
#define glCompileShader(s)									(LOGD("glCompileShader(%u)",s),											glCompileShader(s))
#define glCopyTexImage2D(t,l,i,x,y,w,h,b)					(LOGD("glCopyTexImage2D(%d,%d,%d,%d,%d,%d,%d,%d)",t,l,i,x,y,w,h,b),		glCopyTexImage2D(t,l,i,x,y,w,h,b))
#define glCopyTexSubImage2D(t,l,p,q,x,y,w,h)				(LOGD("glCopyTexSubImage2D(%d,%d,%d,%d,%d,%d,%d,%d)",t,l,p,q,x,y,w,h),	glCopyTexSubImage2D(t,l,p,q,x,y,w,h))
#define glCreateProgram()									(LOGD("glCreateProgram()"),												glCreateProgram())
#define glCreateShader(t)									(LOGD("glCreateShader(%s)",_GLstr(t)),									glCreateShader(t))
#define glDeleteFramebuffers(n,f)							(LOGD("glDeleteFramebuffers(%d,%p)",n,f),								glDeleteFramebuffers(n,f))
#define glDeleteProgram(p)									(LOGD("glDeleteProgram(%u)",p),											glDeleteProgram(p))
#define glDeleteShader(s)									(LOGD("glDeleteShader(%u)",s),											glDeleteShader(s))
#define glDeleteTextures(n,t)								(LOGD("glDeleteTextures(%d,%p)",n,t),									glDeleteTextures(n,t))
#define glDisable(c)										(LOGD("glDisable(%s)",_GLstr(c)),										glDisable(c))
#define glDrawArrays(m,f,c)									(LOGD("glDrawArrays(%s,%d,%d)",_GLstr(m),f,c),							glDrawArrays(m,f,c))
#define glDrawElements(m,c,t,i)								(LOGD("glDrawElements(%s,%d,%s,%p)",_GLstr(m),c,_GLstr(t),i),			glDrawElements(m,c,t,i))
#define glEGLImageTargetTexture2DOES(t,i)					(LOGD("glEGLImageTargetTexture2DOES(%d,%p)",t,i),						glEGLImageTargetTexture2DOES(t,i))
#define glEnable(c)											(LOGD("glEnable(%s)",_GLstr(c)),										glEnable(c))
#define glEnableClientState(a)								(LOGD("glEnableClientState(%d)",a),										glEnableClientState(a))
#define glEnableVertexAttribArray(i)						(LOGD("glEnableVertexAttribArray(%u)",i),								glEnableVertexAttribArray(i))
#define glFinish()											(LOGD("glFinish()"),													glFinish())
#define glFlush()											(LOGD("glFlush()"),														glFlush())
#define glFramebufferTexture2D(t,a,x,u,l)					(LOGD("glFramebufferTexture2D(%s,%s,%s,#%u,%d)",_GLstr(t),_GLstr(a),_GLstr(x),u,l),	glFramebufferTexture2D(t,a,x,u,l))
#define glGenFramebuffers(n,f)								(LOGD("glGenFramebuffers(%d,%p)",n,f),									glGenFramebuffers(n,f))
#define glGenTextures(n,t)									(LOGD("glGenTextures(%d,%p)",n,t),										glGenTextures(n,t))
#define glGetActiveAttrib(p,i,b,l,s,t,n)					(LOGD("glGetActiveAttrib(%u,%d,%d,%p,%p,%p,%p)",p,i,b,l,s,t,n),			glGetActiveAttrib(p,i,b,l,s,t,n))
#define glGetActiveUniform(p,i,b,l,s,t,n)					(LOGD("glGetActiveUniform(%u,%u,%d,%p,%p,%p,%p)",p,i,b,l,s,t,n),		glGetActiveUniform(p,i,b,l,s,t,n))
#define glGetAttribLocation(p,n)							(LOGD("glGetAttribLocation(%u,%p)",p,n),								glGetAttribLocation(p,n))
#define glGetBooleanv(n,p)									(LOGD("glGetBooleanv(%d,%p)",n,p),										glGetBooleanv(n,p))
#define glGetError()										(LOGD("glGetError()"),													glGetError())
#define glGetFloatv(n,p)									(LOGD("glGetFloatv(%d,%p)",n,p),										glGetFloatv(n,p))
#define glGetIntegerv(n,p)									(LOGD("glGetIntegerv(%s,%p)",_GLstr(n),p),								glGetIntegerv(n,p))
#define glGetProgramInfoLog(p,b,l,i)						(LOGD("glGetProgramInfoLog(%u,%d,%p,%p)",p,b,l,i),						glGetProgramInfoLog(p,b,l,i))
#define glGetProgramiv(p,n,r)								(LOGD("glGetProgramiv(%u,%d,%p)",p,n,r),								glGetProgramiv(p,n,r))
#define glGetRenderbufferParameteriv(t,n,p)					(LOGD("glGetRenderbufferParameteriv(%d,%d,%p)",t,n,p),					glGetRenderbufferParameteriv(t,n,p))
#define glGetShaderInfoLog(s,b,l,i)							(LOGD("glGetShaderInfoLog(%u,%d,%p,%p)",s,b,l,i),						glGetShaderInfoLog(s,b,l,i))
#define glGetShaderPrecisionFormat(s,p,r,n)					(LOGD("glGetShaderPrecisionFormat(%d,%d,%p,%p)",s,p,r,n),				glGetShaderPrecisionFormat(s,p,r,n))
#define glGetShaderiv(s,n,p)								(LOGD("glGetShaderiv(%u,%d,%p)",s,n,p),									glGetShaderiv(s,n,p))
#define glGetString(n)										(LOGD("glGetString(%#x)",n),											glGetString(n))
#define glGetUniformLocation(p,n)							(LOGD("glGetUniformLocation(%u,\"%s\")",p,n),							glGetUniformLocation(p,n))
#define glLinkProgram(p)									(LOGD("glLinkProgram(%u)",p),											glLinkProgram(p))
#define glLoadIdentity()									(LOGD("glLoadIdentity()"),												glLoadIdentity())
#define glLoadMatrixf(mp)									(LOGD("glLoadMatrixf(%p)",mp),											glLoadMatrixf(mp))
#define glMatrixMode(m)										(LOGD("glMatrixMode(%d)",m),											glMatrixMode(m))
#define glOrtho(l,r,b,t,n,f)								(LOGD("glOrtho(%g,%g,%g,%g,%g,%g)",l,r,b,t,n,f),						glOrtho(l,r,b,t,n,f))
#define glPixelStorei(n,p)									(LOGD("glPixelStorei(%s,%d)",_GLstr(n),p),								glPixelStorei(n,p))
#define glReadBuffer(m)										(LOGD("glReadBuffer(%s)",_GLstr(m)),									glReadBuffer(m))
#define glReadPixels(x,y,w,h,f,t,p)							(LOGD("glReadPixels(%d,%d,%d,%d,%s,%s,%p)",x,y,w,h,_GLstr(f),_GLstr(t),p),	glReadPixels(x,y,w,h,f,t,p))
#define glScissor(x,y,w,h)									(LOGD("glScissor(%d,%d,%d,%d)",x,y,w,h),								glScissor(x,y,w,h))
#define glShadeModel(m)										(LOGD("glShadeModel(%s)",_GLstr(m)),									glShadeModel(m))
#define glShaderSource(s,c,t,l)								(LOGD("glShaderSource(%u,%d,%p,%p)",s,c,t,l),							glShaderSource(s,c,t,l))
#define glTexCoordPointer(z,t,s,p)							(LOGD("glTexCoordPointer(%d,%d,%d,%p)",z,t,s,p),						glTexCoordPointer(z,t,s,p))
#define glTexEnvi(t,n,p)									(LOGD("glTexEnvi(%d,%d,%p)",t,n,p),										glTexEnvi(t,n,p))
#define glTexImage2D(g,l,i,w,h,b,f,t,p)						(LOGD("glTexImage2D(%s,%d,%s,%d,%d,%d,%s,%s,%p)",_GLstr(g),l,_GLstr(i),w,h,b,_GLstr(f),_GLstr(t),p),	\
																																	glTexImage2D(g,l,i,w,h,b,f,t,p))
#define glTexParameteri(t,n,p)								(LOGD("glTexParameteri(%s,%s,%s)",_GLstr(t),_GLstr(n),_GLstr(p)),		glTexParameteri(t,n,p))
#define glTexParameteriv(t,n,p)								(LOGD("glTexParameteriv((%s,%s,%p)",_GLstr(t),_GLstr(n),p),				glTexParameteriv(t,n,p))
#define glTexSubImage2D(g,l,x,y,w,h,f,t,p)					(LOGD("glTexSubImage2D(%s,%d,%d,%d,%d,%d,%s,%s,%p)",_GLstr(g),l,x,y,w,h,_GLstr(f),_GLstr(t),p),	\
																																	glTexSubImage2D(g,l,x,y,w,h,f,t,p))
#define glUniform1f(l,x)									(LOGD("glUniform1f(%d,%g)",l,x),										glUniform1f(l,x))
#define glUniform1fv(l,c,v)									(LOGD("glUniform1fv(%d,%d,%p)",l,c,v),									glUniform1fv(l,c,v))
#define glUniform1i(l,x)									(LOGD("glUniform1i(%d,%d)",l,x),										glUniform1i(l,x))
#define glUniform1iv(l,c,v)									(LOGD("glUniform1iv(%d,%d,%p)",l,c,v),									glUniform1iv(l,c,v))
#define glUniform2f(l,x,y)									(LOGD("glUniform2f(%d,%g,%g)",l,x,y),									glUniform2f(l,x,y))
#define glUniform2fv(l,c,v)									(LOGD("glUniform2fv(%d,%d,%p)",l,c,v),									glUniform2fv(l,c,v))
#define glUniform2i(l,x,y)									(LOGD("glUniform2i(%d,%d,%d)",l,x,y),									glUniform2i(l,x,y))
#define glUniform2iv(l,c,v)									(LOGD("glUniform2iv(%d,%d,%p)",l,c,v),									glUniform2iv(l,c,v))
#define glUniform3f(l,x,y,z)								(LOGD("glUniform3f(%d,%g,%g,%g)",l,x,y,z),								glUniform3f(l,x,y,z))
#define glUniform3fv(l,c,v)									(LOGD("glUniform3fv(%d,%d,%p)",l,c,v),									glUniform3fv(l,c,v))
#define glUniform3i(l,x,y,z)								(LOGD("glUniform3i(%d,%d,%d,%d)",l,x,y,z),								glUniform3i(l,x,y,z))
#define glUniform3iv(l,c,v)									(LOGD("glUniform3iv(%d,%d,%p)",l,c,v),									glUniform3iv(l,c,v))
#define glUniform4f(l,x,y,z,w)								(LOGD("glUniform4f(%d,%g,%g,%g,%g)",l,x,y,z,w),							glUniform4f(l,x,y,z,w))
#define glUniform4fv(l,c,v)									(LOGD("glUniform4fv(%d,%d,%p)",l,c,v),									glUniform4fv(l,c,v))
#define glUniform4i(l,x,y,z,w)								(LOGD("glUniform4i(%d,%d,%d,%d,%d)",l,x,y,z,w),							glUniform4i(l,x,y,z,w))
#define glUniform4iv(l,c,v)									(LOGD("glUniform4iv(%d,%d,%p)",l,c,v),									glUniform4iv(l,c,v))
#define glUniformMatrix2fv(l,c,t,v)							(LOGD("glUniformMatrix2fv(%d,%d,%d,%p)",l,c,t,v),						glUniformMatrix2fv(l,c,t,v))
#define glUniformMatrix3fv(l,c,t,v)							(LOGD("glUniformMatrix3fv(%d,%d,%d,%p)",l,c,t,v),						glUniformMatrix3fv(l,c,t,v))
#define glUniformMatrix4fv(l,c,t,v)							(LOGD("glUniformMatrix4fv(%d,%d,%d,%p)",l,c,t,v),						glUniformMatrix4fv(l,c,t,v))
#define glUseProgram(p)										(LOGD("glUseProgram(%u)",p),											glUseProgram(p))
#define glValidateProgram(p)								(LOGD("glValidateProgram(%u)",p),										glValidateProgram(p))
#define glVertexAttribPointer(i,z,t,n,s,p)					(LOGD("glVertexAttribPointer(%u,%d,%s,%d,%d,%p)",i,z,_GLstr(t),n,s,p),	glVertexAttribPointer(i,z,t,n,s,p))
#define glViewport(x,y,w,h)									(LOGD("glViewport(%d,%d,%d,%d)",x,y,w,h),								glViewport(x,y,w,h))
#define glutSwapBuffers()									(LOGD("glutSwapBuffers()"),												glutSwapBuffers())

/* EGL API */
#define eglBindAPI(a)										(LOGD("eglBindAPI(%d)",a),												eglBindAPI(a))
#define eglBindTexImage(d,s,b)								(LOGD("eglBindTexImage(%p,%p,%d)",d,s,b),								eglBindTexImage(d,s,b))
#define eglChooseConfig(d,a,c,z,n)							(LOGD("eglChooseConfig(%p,%p,%p,%d,%p)",d,a,c,z,n),						eglChooseConfig(d,a,c,z,n))
#define eglCopyBuffers(d,s,t)								(LOGD("eglCopyBuffers(%p,%p,%p)",d,s,t),								eglCopyBuffers(d,s,t))
#define eglCreateContext(d,c,s,a)							(LOGD("eglCreateContext(%p,%p,%p,%p)",d,c,s,a),							eglCreateContext(d,c,s,a))
#define eglCreatePbufferFromClientBuffer(d,t,b,c,a)			(LOGD("eglCreatePbufferFromClientBuffer(%p,%d,%p,%p,%p)",d,t,b,c,a),	eglCreatePbufferFromClientBuffer(d,t,b,c,a))
#define eglCreatePbufferSurface(d,c,a)						(LOGD("eglCreatePbufferSurface(%p,%p,%p)",d,c,a),						eglCreatePbufferSurface(d,c,a))
#define eglCreatePixmapSurface(d,c,p,a)						(LOGD("eglCreatePixmapSurface(%p,%d,%p,%p)",d,c,p,a),					eglCreatePixmapSurface(d,c,p,a))
#define eglCreateWindowSurface(d,c,w,a)						(LOGD("eglCreateWindowSurface(%p,%p,%p,%p)",d,c,w,a),					eglCreateWindowSurface(d,c,w,a))
#define eglDestroyContext(d,c)								(LOGD("eglDestroyContext(%p,%p)",d,c),									eglDestroyContext(d,c))
#define eglDestroySurface(d,s)								(LOGD("eglDestroySurface(%p,%p)",d,s),									eglDestroySurface(d,s))
#define eglGetConfigAttrib(d,c,a,v)							(LOGD("eglGetConfigAttrib(%p,%d, %d,%p)",d,c,a,v),						eglGetConfigAttrib(d,c,a,v))
#define eglGetConfigs(d,c,z,n)								(LOGD("eglGetConfigs(%p,%p,%d,%p)",d,c,z,n),							eglGetConfigs(d,c,z,n))
#define eglGetCurrentContext()								(LOGD("eglGetCurrentContext()"),										eglGetCurrentContext())
#define eglGetCurrentDisplay()								(LOGD("eglGetCurrentDisplay()"),										eglGetCurrentDisplay())
#define eglGetCurrentSurface(r)								(LOGD("eglGetCurrentSurface(%d)",r),									eglGetCurrentSurface(r))
#define eglGetDisplay(i)									(LOGD("eglGetDisplay(%p)",i),											eglGetDisplay(i))
#define eglGetError()										(LOGD("eglGetError()"),													eglGetError())
#define eglGetProcAddress(n)								(LOGD("eglGetProcAddress(\"%s\")",n),									eglGetProcAddress(n))
#define eglInitialize(d,M,m)								(LOGD("eglInitialize(%p,%p,%p)",d,M,m),									eglInitialize(d,M,m))
#define eglMakeCurrent(d,w,r,c)								(LOGD("eglMakeCurrent(%p,%p,%p,%p)",d,w,r,c),							eglMakeCurrent(d,w,r,c))
#define eglQueryAPI()										(LOGD("eglQueryAPI()"),													eglQueryAPI())
#define eglQueryContext(d,c,a,v)							(LOGD("eglQueryContext(%p,%p,%d,%p)",d,c,a,v),							eglQueryContext(d,c,a,v))
#define eglQueryString(d,n)									(LOGD("eglQueryString(%p,%d)",d,n),										eglQueryString(d,n))
#define eglQuerySurface(d,s,a,v)							(LOGD("eglQuerySurface(%p,%p,%d,%p)",d,s,a,v),							eglQuerySurface(d,s,a,v))
#define eglReleaseTexImage(d,s,b)							(LOGD("eglReleaseTexImage(%p,%p,%d)",d,s,b),							eglReleaseTexImage(d,s,b))
#define eglReleaseThread()									(LOGD("eglReleaseThread()"),											eglReleaseThread())
#define eglSurfaceAttrib(d,s,a,v)							(LOGD("eglSurfaceAttrib(%p,%p,%d,%d)",d,s,a,v),							eglSurfaceAttrib(d,s,a,v))
#define eglSwapBuffers(d,s)									(LOGD("eglSwapBuffers(%p,%p)",d,s),										eglSwapBuffers(d,s))
#define eglSwapInterval(d,i)								(LOGD("eglSwapInterval(%p,%d)",d,i),									eglSwapInterval(d,i))
#define eglTerminate(d)										(LOGD("eglTerminate(%p)",d),											eglTerminate(d))
#define eglWaitClient()										(LOGD("eglWaitClient()"),												eglWaitClient())
#define eglWaitGL()											(LOGD("eglWaitGL()"),													eglWaitGL())
#define eglWaitNative(e)									(LOGD("eglWaitNative(%d)",e),											eglWaitNative(e))

#endif /* __FSKGLAPILOG__ */
