//
//  RenderView.h
//  Descent
//
//  Created by Devin Tuchsen on 10/19/15.
//

#import <UIKit/UIKit.h>

@interface RenderView : UIView

#ifdef OGLES
- (void)drawView;
@property EAGLContext* context;
@property GLuint colorRenderBuffer;
#endif

@end
