//
//  ViewControllerC.h
//  Descent
//
//  Created by Devin Tuchsen on 10/20/15.
//

#ifndef ViewControllerC_h
#define ViewControllerC_h

#ifdef OGLES
#ifdef ANDROID_NDK
#include <GLES/gl.h>
#else
#include <OpenGLES/ES1/gl.h>
#endif
#endif

int textIsActive();
void activateText();
void deactivateText();
#ifdef OGLES
void getRenderBufferSize(GLint *width, GLint *height);
#endif
void showRenderBuffer();

#endif /* ViewControllerC_h */
