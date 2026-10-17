#include "Tools/AppleBootstrap/MetalProbe.h"
#include "Tools/AppleBootstrap/MetalView.h"
#include "WoW/Source/WowServices/ClientData.h"
#import <AppKit/AppKit.h>
#include <cstdio>
#include <cstring>

@interface MetalPreviewDelegate : NSObject <NSApplicationDelegate>
@property(nonatomic, strong) NSWindow* window;
@property(nonatomic, strong) WowTextureRenderer* renderer;
@end
@implementation MetalPreviewDelegate
- (BOOL)applicationShouldTerminateAfterLastWindowClosed:(NSApplication*)sender { return YES; }
- (void)applicationDidResignActive:(NSNotification*)notification { [self.renderer suspend]; }
- (void)applicationDidBecomeActive:(NSNotification*)notification { [self.renderer resume]; }
@end

int main(int argc, char** argv) {
    if ((argc != 5 && argc != 6) || std::strcmp(argv[1], "--data-root") || std::strcmp(argv[3], "--texture") ||
        (argc == 6 && std::strcmp(argv[5], "--preview"))) {
        std::fprintf(stderr, "Usage: AppleMetalProbe --data-root /path/to/Data --texture 'Interface\\Buttons\\UI-Quickslot2.blp' [--preview]\n");
        return 2;
    }
    @autoreleasepool {
        try {
            ClientData client(argv[2], "");
            const auto asset = client.Read(argv[4], 64 * 1024 * 1024);
            const auto image = BLP::Image::Load(asset.bytes.data(), asset.bytes.size(), 256 * 1024 * 1024);
            const auto ui = GxMetal::UIShader::FromBls(client.Read("Shaders\\Vertex\\vs_3_0\\UI.bls", 1024 * 1024).bytes,
                client.Read("Shaders\\Pixel\\ps_3_0\\UI.bls", 1024 * 1024).bytes);
            std::printf("%s from %s: %ux%u %s, %zu mips\n", argv[4], asset.archive.c_str(), image.mips[0].width, image.mips[0].height,
                BLP::FormatName(image.format), image.mips.size());
            std::string report;
            const bool success = AppleBootstrap::CheckMetal(image, ui, report);
            std::fputs(report.c_str(), stdout);
            if (success && argc == 6) {
                [NSApplication sharedApplication];
                [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
                __attribute__((objc_precise_lifetime)) MetalPreviewDelegate* delegate = [MetalPreviewDelegate new];
                NSApp.delegate = delegate;
                delegate.window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 900, 600)
                    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskResizable
                    backing:NSBackingStoreBuffered defer:NO];
                delegate.window.title = [NSString stringWithFormat:@"WoW Metal — %s", argv[4]];
                delegate.window.releasedWhenClosed = NO;
                MTKView* view = [[MTKView alloc] initWithFrame:delegate.window.contentView.bounds];
                delegate.window.contentView = view;
                delegate.renderer = [[WowTextureRenderer alloc] initWithView:view image:image uiShader:ui];
                [delegate.window center];
                [delegate.window makeKeyAndOrderFront:nil];
                [NSApp activateIgnoringOtherApps:YES];
                [delegate.renderer resume];
                [NSApp run];
            }
            return success ? 0 : 1;
        } catch (const std::exception& error) {
            std::fprintf(stderr, "%s\n", error.what());
            return 1;
        }
    }
}
