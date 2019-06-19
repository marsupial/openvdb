
#import <Cocoa/Cocoa.h>
#import <Foundation/NSTimer.h>
#import <mach/mach_time.h>
#include "pez.h"


@interface GLView : NSOpenGLView <NSWindowDelegate> {
  uint64_t mPreviousTime;
  NSTimer *mTimer;
}
@end

@implementation GLView

- (id) initWithFrame: (NSRect) frame
{
    NSOpenGLPixelFormatAttribute attribs[] = {
//		NSOpenGLPFAOpenGLProfile, NSOpenGLProfileVersion3_2Core,
        NSOpenGLPFAAccelerated,
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, 24,
        NSOpenGLPFAAlphaSize, 8,
        NSOpenGLPFAColorSize, 32,
        NSOpenGLPFANoRecovery,
#if PEZ_ENABLE_MULTISAMPLING
        kCGLPFASampleBuffers, 1,
        kCGLPFASamples, 4,
#endif
        0
    };

    NSOpenGLPixelFormat *fmt = [[NSOpenGLPixelFormat alloc]
                            initWithAttributes:(NSOpenGLPixelFormatAttribute*) attribs];

    self = [self initWithFrame:frame pixelFormat:fmt];
    [fmt release];

    return self;
}

- (NSSize) size
{
  return [ self frame ].size;
}

- (void) reshape
{
    NSOpenGLContext *ctx = [self openGLContext];
    [ ctx makeCurrentContext];
    CGSize size = [self size];

    // Set the viewport to be the entire window
    // glViewport(0, 0, size.width, size.height);

	PezResize(size.width, size.height);

    [ctx update];
}

- (void)prepareOpenGL
{
    int TransparentWindow = 0;
    if (TransparentWindow) {
        int opaque = NO;
        [[self openGLContext]
            setValues:&opaque
            forParameter:NSOpenGLCPSurfaceOpacity];

        [[self window] setOpaque:NO];
        [[self window] setAlphaValue:0.5];
    }

    const NSSize size = [self size];
	PezInitialize(size.width, size.height);

    mPreviousTime = mach_absolute_time();
    mTimer = [[NSTimer
                       scheduledTimerWithTimeInterval:1.0f/120.0f
                       target:self 
                       selector:@selector(animate:)
                       userInfo:nil
                       repeats:YES] retain];
}

- (void) drawRect:(NSRect) theRect
{
    //[[self openGLContext] makeCurrentContext];
	PezRender();
    [[self openGLContext] flushBuffer];
}


static void toPezModifiers( NSEvent *theEvent, GLView *self, unsigned base )
{
  const NSUInteger flags = [ theEvent modifierFlags ];

  if ( flags & NSAlternateKeyMask ) base |= PEZ_ALT_KEY;
  if ( flags & NSControlKeyMask )   base |= PEZ_CTL_KEY;
  if ( flags & NSShiftKeyMask )     base |= PEZ_SFT_KEY;
  if ( flags & NSCommandKeyMask )   base |= PEZ_CMD_KEY;

  const NSSize size = [self size];
  const NSPoint curPoint = [theEvent locationInWindow];
  PezHandleMouse(curPoint.x, size.height - curPoint.y, base);
  [self display];
}

- (void) mouseDragged: (NSEvent*) theEvent
{
	toPezModifiers(theEvent, self, PEZ_MOVE);
}

- (void) mouseUp: (NSEvent*) theEvent
{
	toPezModifiers(theEvent, self, PEZ_UP);
}

- (void) mouseDown: (NSEvent*) theEvent
{
	toPezModifiers(theEvent, self,  PEZ_DOWN);
}

- (void)rightMouseDown:(NSEvent *)theEvent
{
	toPezModifiers(theEvent, self,  PEZ_DOWN|PEZ_R_MOUSE);
}
- (void)otherMouseDown:(NSEvent *)theEvent
{
	toPezModifiers(theEvent, self,  PEZ_DOWN|PEZ_M_MOUSE);
}
- (void)rightMouseDragged:(NSEvent *)theEvent
{
	toPezModifiers(theEvent, self,  PEZ_MOVE|PEZ_R_MOUSE);
}
- (void)otherMouseDragged:(NSEvent *)theEvent
{
	toPezModifiers(theEvent, self,  PEZ_MOVE|PEZ_M_MOUSE);
}
- (void)rightMouseUp:(NSEvent *)theEvent
{
	toPezModifiers(theEvent, self,  PEZ_UP|PEZ_R_MOUSE);
}
- (void)otherMouseUp:(NSEvent *)theEvent
{
	toPezModifiers(theEvent, self,  PEZ_UP|PEZ_M_MOUSE);
}
- (void)scrollWheel:(NSEvent *)theEvent
{
  const NSPoint curPoint = [theEvent locationInWindow];
  //if ( [ theEvent scrollingDeltaY] > 0.0 )
  if ( [ theEvent deltaY] > 0.0 )
  	PezHandleMouse(curPoint.x, [self size].height - curPoint.y, PEZ_SCROLL|PEZ_M_MOUSE);
  else
  	PezHandleMouse(curPoint.x, [self size].height - curPoint.y, PEZ_SCROLL|PEZ_R_MOUSE);

  [self display];
}

- (void) animate: (NSTimer*) timer
{
    uint64_t currentTime = mach_absolute_time();
    uint64_t elapsedTime = currentTime - mPreviousTime;
    mPreviousTime = currentTime;
    
    mach_timebase_info_data_t info;
    mach_timebase_info(&info);
    
    elapsedTime *= info.numer;
    elapsedTime /= info.denom;
    
    float timeStep = elapsedTime / 1000000.0f;

    if ( PezUpdate(timeStep) )
      [self display];
}

- (void) onKey: (unichar) character downEvent: (BOOL) flag
{
    switch(character) {
        case 27:
        case 'q':
        	[ [ self window] close ];
            //[[NSApplication sharedApplication] terminate:self];
            break;
    }
    PezHandleKey(character);
}

- (void) keyDown:(NSEvent *)theEvent
{
    NSString *characters;
    unsigned int characterIndex, characterCount;
    
    characters = [theEvent charactersIgnoringModifiers];
    characterCount = [characters length];

    for (characterIndex = 0; characterIndex < characterCount; characterIndex++)
      [self onKey:[characters characterAtIndex:characterIndex] downEvent:YES];

  	[self display];
}

@end

@interface ApplicationDelegate : NSObject <NSApplicationDelegate, NSWindowDelegate> {
}
@end

@implementation ApplicationDelegate : NSObject

// - (void)applicationWillFinishLaunching:(NSNotification *)notification;

- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication *)sender
{
  return YES;
}

@end

int main(int argc, char * argv[]) {

    NSAutoreleasePool * pool = [[NSAutoreleasePool alloc] init];

    NSApplication * application = [NSApplication sharedApplication];
    [application setActivationPolicy:NSApplicationActivationPolicyRegular];
  
    [application setDelegate: [[ApplicationDelegate alloc] init] ];
    //[appDelegate release];

	{
      PezConfig cfg = PezGetConfig();
      NSRect screenBounds = [[NSScreen mainScreen] frame];
      NSRect viewBounds = NSMakeRect(0, 0, cfg.Width, cfg.Height);
      NSRect centered = NSMakeRect(NSMidX(screenBounds) - NSMidX(viewBounds),
                                   NSMidY(screenBounds) - NSMidY(viewBounds),
                                   viewBounds.size.width, viewBounds.size.height);

      GLView* view = [[GLView alloc] initWithFrame:viewBounds];

      NSWindow*
      window = [[NSWindow alloc]
          initWithContentRect:centered
          styleMask:NSTitledWindowMask | NSClosableWindowMask | NSMiniaturizableWindowMask
          backing:NSBackingStoreBuffered
          defer:NO];

      [window setStyleMask:[window styleMask] | NSResizableWindowMask];
      [window setContentView:view];
      [window setDelegate:view];
      [window makeFirstResponder:view];
      [window setTitle: [NSString stringWithUTF8String: cfg.Title]];

      //[window setLevel: NSFloatingWindowLevel];
      //[window makeKeyAndOrderFront: view];
    
      [view release];

      [window makeKeyAndOrderFront:nil];
	}

    [application run];

    [pool drain];

    return EXIT_SUCCESS;
}

void PezDebugStringW(const wchar_t* pStr, ...)
{
    va_list a;
    va_start(a, pStr);

    wchar_t msg[1024] = {0};
    vswprintf(msg, countof(msg), pStr, a);
    fputws(msg, stderr);
}

void PezDebugString(const char* pStr, ...)
{
    va_list a;
    va_start(a, pStr);

    char msg[1024] = {0};
    vsnprintf(msg, countof(msg), pStr, a);
    fputs(msg, stderr);
}

void PezFatalErrorW(const wchar_t* pStr, ...)
{
    fwide(stderr, 1);

    va_list a;
    va_start(a, pStr);

    wchar_t msg[1024] = {0};
    vswprintf(msg, countof(msg), pStr, a);
    fputws(msg, stderr);
    exit(1);
}

void PezFatalError(const char* pStr, ...)
{
    va_list a;
    va_start(a, pStr);

    char msg[1024] = {0};
    vsnprintf(msg, countof(msg), pStr, a);
    fputs(msg, stderr);
    exit(1);
}

const char* PezResourcePath()
{
    return "../../";
}
