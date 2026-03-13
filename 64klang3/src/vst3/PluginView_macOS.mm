///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// PluginView_macOS.mm — macOS NSView + Metal + ImGui rendering for 64klang3
// Compiled as Objective-C++ (.mm) only on Apple targets.
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

#ifdef __APPLE__

#include "PluginView.h"
#include "gui/ImGuiPlugin.h"

#import <Cocoa/Cocoa.h>
#import <Metal/Metal.h>
#import <QuartzCore/CAMetalLayer.h>
#include "imgui.h"
#include "imgui_impl_osx.h"
#include "imgui_impl_metal.h"

using Steinberg::Vst::K64PluginView;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File-scope Metal state
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

static id<MTLDevice>        g_device       = nil;
static id<MTLCommandQueue>  g_cmdQueue     = nil;
static NSView*              g_childView    = nil;
static CAMetalLayer*        g_metalLayer   = nil;
static NSTimer*             g_timer        = nil;
static K64PluginView*       g_activeView   = nullptr;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Custom NSView that hosts the Metal layer and forwards events to ImGui
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

@interface K64MetalView : NSView
@end

@implementation K64MetalView

- (BOOL)acceptsFirstResponder { return YES; }

// Y-axis: Cocoa is flipped vs ImGui (Y=0 is top in ImGui).  Marking the view
// as flipped makes NSEvent coordinates match ImGui expectations.
- (BOOL)isFlipped { return YES; }

- (void)viewDidMoveToWindow
{
    [super viewDidMoveToWindow];
    if (self.window)
        [self.window makeFirstResponder:self];
}

@end

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Timer callback target
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

@interface K64TimerTarget : NSObject
@end

@implementation K64TimerTarget

- (void)onTimer:(NSTimer*)t
{
    (void)t;
    if (g_activeView)
        g_activeView->renderFrameMacOS();
}

@end

static K64TimerTarget* g_timerTarget = nil;

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// C++ bridge functions called from PluginView.cpp
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool k64_macOS_createView(void* parentNSView, int width, int height, K64PluginView* view)
{
    g_activeView = view;

    NSView* parent = (__bridge NSView*)parentNSView;

    // Metal device
    g_device = MTLCreateSystemDefaultDevice();
    if (!g_device)
        return false;
    g_cmdQueue = [g_device newCommandQueue];

    // Child NSView with CAMetalLayer
    NSRect frame = NSMakeRect(0, 0, (CGFloat)width, (CGFloat)height);
    g_childView = [[K64MetalView alloc] initWithFrame:frame];
    g_childView.wantsLayer = YES;

    g_metalLayer = [CAMetalLayer layer];
    g_metalLayer.device          = g_device;
    g_metalLayer.pixelFormat     = MTLPixelFormatBGRA8Unorm;
    g_metalLayer.framebufferOnly = YES;
    g_metalLayer.frame           = g_childView.bounds;
    g_childView.layer            = g_metalLayer;

    [parent addSubview:g_childView];

    // ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.IniFilename = nullptr;
    ImGui::StyleColorsDark();
    ImGuiStyle& style = ImGui::GetStyle();
    style.WindowRounding = 0.0f;
    style.FrameRounding  = 2.0f;

    // Fonts — prefer system fonts; fall back to the ImGui default bitmap font
    const char* fontPaths[] = {
        "/System/Library/Fonts/SFNS.ttf",
        "/System/Library/Fonts/Helvetica.ttc",
        "/Library/Fonts/Arial.ttf",
        nullptr
    };
    bool loadedSmall = false, loadedLarge = false;
    for (int i = 0; fontPaths[i] && (!loadedSmall || !loadedLarge); ++i)
    {
        if (!loadedSmall && io.Fonts->AddFontFromFileTTF(fontPaths[i], 14.0f))
            loadedSmall = true;
        if (!loadedLarge && io.Fonts->AddFontFromFileTTF(fontPaths[i], 32.0f))
            loadedLarge = true;
    }
    if (!loadedSmall) io.Fonts->AddFontDefault();
    if (!loadedLarge) io.Fonts->AddFontDefault();
    io.Fonts->Build();

    ImGui_ImplMetal_Init(g_device);
    ImGui_ImplOSX_Init(g_childView);

    K64GUI::init();
    K64GUI::setWindowHandle((__bridge void*)g_childView);

    // Render timer at ~60 fps on the main run loop
    g_timerTarget = [[K64TimerTarget alloc] init];
    g_timer = [NSTimer scheduledTimerWithTimeInterval:(1.0 / 60.0)
                                               target:g_timerTarget
                                             selector:@selector(onTimer:)
                                             userInfo:nil
                                              repeats:YES];
    [[NSRunLoop currentRunLoop] addTimer:g_timer forMode:NSRunLoopCommonModes];

    return true;
}

void k64_macOS_destroyView()
{
    [g_timer invalidate];
    g_timer = nil;
    g_timerTarget = nil;
    g_activeView = nullptr;

    K64GUI::setWindowHandle(nullptr);
    K64GUI::shutdown();

    ImGui_ImplOSX_Shutdown();
    ImGui_ImplMetal_Shutdown();
    ImGui::DestroyContext();

    [g_childView removeFromSuperview];
    g_childView  = nil;
    g_metalLayer = nil;
    g_cmdQueue   = nil;
    g_device     = nil;
}

void k64_macOS_resizeView(int width, int height)
{
    if (g_childView && g_metalLayer)
    {
        NSRect frame = NSMakeRect(0, 0, (CGFloat)width, (CGFloat)height);
        g_childView.frame  = frame;
        g_metalLayer.frame = g_childView.bounds;
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Called from K64PluginView::renderFrameMacOS() (which PluginView.cpp delegates here)
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

void k64_macOS_renderFrame()
{
    @autoreleasepool {
        id<CAMetalDrawable> drawable = [g_metalLayer nextDrawable];
        if (!drawable)
            return;

        MTLRenderPassDescriptor* rpd = [MTLRenderPassDescriptor renderPassDescriptor];
        rpd.colorAttachments[0].texture     = drawable.texture;
        rpd.colorAttachments[0].loadAction  = MTLLoadActionClear;
        rpd.colorAttachments[0].clearColor  = MTLClearColorMake(0.12, 0.12, 0.14, 1.0);
        rpd.colorAttachments[0].storeAction = MTLStoreActionStore;

        id<MTLCommandBuffer> cmdBuf = [g_cmdQueue commandBuffer];

        ImGui_ImplMetal_NewFrame(rpd);
        ImGui_ImplOSX_NewFrame(g_childView);
        ImGui::NewFrame();

        K64GUI::render();

        ImGui::Render();

        id<MTLRenderCommandEncoder> enc = [cmdBuf renderCommandEncoderWithDescriptor:rpd];
        ImGui_ImplMetal_RenderDrawData(ImGui::GetDrawData(), cmdBuf, enc);
        [enc endEncoding];

        [cmdBuf presentDrawable:drawable];
        [cmdBuf commit];
    }
}

///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// File dialog bridge — called from ImGuiPlugin.cpp openFileDialog()
///////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C"
bool k64_macOS_openFileDialog(char* outBuf, int bufSz,
                               const char* ext, bool forSave)
{
    @autoreleasepool {
        NSString* nsExt = [NSString stringWithUTF8String:ext ? ext : ""];

        if (forSave)
        {
            NSSavePanel* panel = [NSSavePanel savePanel];
            panel.allowedFileTypes = @[nsExt];
            panel.canCreateDirectories = YES;
            NSModalResponse resp = [panel runModal];
            if (resp == NSModalResponseOK && panel.URL)
            {
                strncpy(outBuf, panel.URL.fileSystemRepresentation, (size_t)bufSz - 1);
                outBuf[bufSz - 1] = '\0';
                return true;
            }
        }
        else
        {
            NSOpenPanel* panel = [NSOpenPanel openPanel];
            panel.allowedFileTypes = @[nsExt];
            panel.canChooseFiles = YES;
            panel.canChooseDirectories = NO;
            panel.allowsMultipleSelection = NO;
            NSModalResponse resp = [panel runModal];
            if (resp == NSModalResponseOK && panel.URLs.count > 0)
            {
                strncpy(outBuf, panel.URLs[0].fileSystemRepresentation, (size_t)bufSz - 1);
                outBuf[bufSz - 1] = '\0';
                return true;
            }
        }
        return false;
    }
}

#endif // __APPLE__
