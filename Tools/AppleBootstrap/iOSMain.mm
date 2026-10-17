#import <UIKit/UIKit.h>
#import <os/log.h>
#include <memory>
#include "Tools/AppleBootstrap/Probe.h"
#include "Tools/AppleBootstrap/AudioProbe.h"
#include <sstream>
#include "Tools/AppleBootstrap/MetalProbe.h"
#include "Tools/AppleBootstrap/MetalView.h"
#include "WoW/Source/WowServices/ClientData.h"

@interface BootstrapSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow* window;
@property(nonatomic, strong) UITextView* output;
@property(nonatomic, strong) MTKView* metalView;
@property(nonatomic, strong) WowTextureRenderer* renderer;
@property(nonatomic) BOOL running;
@end

@implementation BootstrapSceneDelegate {
    std::shared_ptr<ClientData> _client; // Keep mounted archives alive for game shader/resource APIs.
    std::shared_ptr<BLP::Image> _pendingImage;
    std::shared_ptr<GxMetal::UIShader> _pendingUI;
    NSString* _pendingTextureName;
}
- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options {
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene*)scene];
    UIViewController* controller = [UIViewController new];
    self.output = [UITextView new];
    self.metalView = [[MTKView alloc] initWithFrame:CGRectZero];
    self.output.editable = NO;
    self.output.font = [UIFont monospacedSystemFontOfSize:13 weight:UIFontWeightRegular];
    self.output.text = @"WoW Apple foundation — startup checks\n";
    controller.view.backgroundColor = UIColor.systemBackgroundColor;
    self.output.translatesAutoresizingMaskIntoConstraints = NO;
    self.metalView.translatesAutoresizingMaskIntoConstraints = NO;
    [controller.view addSubview:self.metalView];
    [controller.view addSubview:self.output];
    UILayoutGuide* area = controller.view.safeAreaLayoutGuide;
    [NSLayoutConstraint activateConstraints:@[
        [self.metalView.topAnchor constraintEqualToAnchor:area.topAnchor],
        [self.metalView.leadingAnchor constraintEqualToAnchor:area.leadingAnchor],
        [self.metalView.trailingAnchor constraintEqualToAnchor:area.trailingAnchor],
        [self.metalView.heightAnchor constraintEqualToAnchor:area.heightAnchor multiplier:0.4],
        [self.output.topAnchor constraintEqualToAnchor:self.metalView.bottomAnchor],
        [self.output.leadingAnchor constraintEqualToAnchor:area.leadingAnchor],
        [self.output.trailingAnchor constraintEqualToAnchor:area.trailingAnchor],
        [self.output.bottomAnchor constraintEqualToAnchor:area.bottomAnchor]
    ]];
    self.window.rootViewController = controller;
    [self.window makeKeyAndVisible];
}

- (void)renderPendingIfActive {
    if (self.window.windowScene.activationState != UISceneActivationStateForegroundActive || !_pendingImage || !_pendingUI) { return; }
    [self.renderer suspend];
    self.renderer = nil;
    std::string report;
    if (AppleBootstrap::CheckMetal(*_pendingImage, *_pendingUI, report)) {
        try {
            if (NSProcessInfo.processInfo.environment[@"WOW_AUDIO_CHECK"].boolValue) {
                std::ostringstream audio;
                CheckAudioOutput(*_client, audio);
                report += audio.str();
                NSLog(@"%s", audio.str().c_str());
            }
            self.renderer = [[WowTextureRenderer alloc] initWithView:self.metalView textureName:_pendingTextureName.UTF8String];
            [self.renderer resume];
        } catch (const std::exception& error) {
            report += std::string("Metal view failed: ") + error.what() + "\n";
        }
    }
    self.output.text = [NSString stringWithFormat:@"%s\n%@", report.c_str(), self.output.text];
    os_log(OS_LOG_DEFAULT, "%{public}s", report.c_str());
    _pendingImage.reset();
    _pendingUI.reset();
    _pendingTextureName = nil;
}

- (void)sceneWillResignActive:(UIScene*)scene { [self.renderer suspend]; }

- (void)sceneDidBecomeActive:(UIScene*)scene {
    const bool hadPending = bool(_pendingImage);
    [self renderPendingIfActive];
    if (self.running || hadPending) { return; }
    [self.renderer suspend];
    self.renderer = nil;
    _client.reset();
    self.running = YES;
    // Reinspect on foreground, including after importing Data using Files/Finder.
    // File checks run off the main thread. All GPU work is gated on the scene
    // being active and submitted on the main thread before backgrounding.
    NSString* documents = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    NSString* dataRoot = [documents stringByAppendingPathComponent:@"Data"];
    NSString* override = NSProcessInfo.processInfo.environment[@"WOW_DATA_ROOT"];
    if (override.length) { dataRoot = override; }
    NSString* scratch = NSTemporaryDirectory();
    NSString* locale = NSProcessInfo.processInfo.environment[@"WOW_LOCALE"] ?: @"";
    NSString* textureName = NSProcessInfo.processInfo.environment[@"WOW_TEXTURE"] ?: @"Interface\\GLUES\\COMMON\\GLUES-WOW-BCLOGO.BLP";
    self.output.text = [NSString stringWithFormat:@"Checking core and Data at %@…\n", dataRoot];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        std::string core, data;
        const bool coreOK = AppleBootstrap::CheckCore(scratch.fileSystemRepresentation, core);
        const bool dataOK = coreOK && AppleBootstrap::InspectData(dataRoot.fileSystemRepresentation, data, locale.UTF8String);
        std::shared_ptr<ClientData> client;
        std::shared_ptr<BLP::Image> image;
        std::shared_ptr<GxMetal::UIShader> ui;
        if (dataOK) {
            try {
                client = std::make_shared<ClientData>(dataRoot.fileSystemRepresentation, locale.UTF8String);
                ui = std::make_shared<GxMetal::UIShader>(GxMetal::UIShader::FromBls(
                    client->Read("Shaders\\Vertex\\vs_3_0\\UI.bls", 1024 * 1024).bytes,
                    client->Read("Shaders\\Pixel\\ps_3_0\\UI.bls", 1024 * 1024).bytes));
                const auto asset = client->Read(textureName.UTF8String, 64 * 1024 * 1024);
                image = std::make_shared<BLP::Image>(BLP::Image::Load(asset.bytes.data(), asset.bytes.size(), 256 * 1024 * 1024));
                data += "Metal preview resource: " + std::string(textureName.UTF8String) + " from " + asset.archive + "\n";
            } catch (const std::exception& error) { data += std::string("Preview resource load failed: ") + error.what() + "\n"; }
        }
        NSString* status = [NSString stringWithFormat:@"%@\n%s%s\nCopy the original Data folder into this app's Documents using Files or Finder.\nThis is a foundation and texture probe; the game loop is not linked.",
            coreOK && dataOK ? @"Foundation checks passed" : @"Checks incomplete — see details", core.c_str(), data.c_str()];
        // A full report can fill simctl's console pipe and stall activation.
        // Keep details in the view; unified logging gets a bounded status.
        os_log(OS_LOG_DEFAULT, "Foundation checks %{public}s; texture loaded: %d", coreOK && dataOK ? "passed" : "incomplete", bool(image));
        dispatch_async(dispatch_get_main_queue(), ^{
            self.output.text = status;
            self.running = NO;
            self->_client = client;
            self->_pendingImage = image;
            self->_pendingUI = ui;
            self->_pendingTextureName = textureName;
            [self renderPendingIfActive];
        });
    });
}
@end

@interface BootstrapDelegate : UIResponder <UIApplicationDelegate>
@end
@implementation BootstrapDelegate
@end

int main(int argc, char** argv) {
    @autoreleasepool {
        return UIApplicationMain(argc, argv, nil, NSStringFromClass(BootstrapDelegate.class));
    }
}
