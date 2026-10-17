#import <UIKit/UIKit.h>
#include "Tools/AppleBootstrap/Probe.h"

@interface BootstrapSceneDelegate : UIResponder <UIWindowSceneDelegate>
@property(nonatomic, strong) UIWindow* window;
@property(nonatomic, strong) UITextView* output;
@property(nonatomic) BOOL running;
@end

@implementation BootstrapSceneDelegate
- (void)scene:(UIScene*)scene willConnectToSession:(UISceneSession*)session options:(UISceneConnectionOptions*)options {
    self.window = [[UIWindow alloc] initWithWindowScene:(UIWindowScene*)scene];
    UIViewController* controller = [UIViewController new];
    self.output = [[UITextView alloc] initWithFrame:self.window.bounds];
    self.output.autoresizingMask = UIViewAutoresizingFlexibleWidth | UIViewAutoresizingFlexibleHeight;
    self.output.editable = NO;
    self.output.font = [UIFont monospacedSystemFontOfSize:13 weight:UIFontWeightRegular];
    self.output.text = @"WoW Apple foundation — startup checks\n";
    controller.view = self.output;
    self.window.rootViewController = controller;
    [self.window makeKeyAndVisible];
}

- (void)sceneDidBecomeActive:(UIScene*)scene {
    if (self.running) { return; }
    self.running = YES;
    // Reinspect on foreground, including after importing Data using Files/Finder.
    // Only bounded diagnostics run here; no background game loop is claimed.
    NSString* documents = NSSearchPathForDirectoriesInDomains(NSDocumentDirectory, NSUserDomainMask, YES).firstObject;
    NSString* dataRoot = [documents stringByAppendingPathComponent:@"Data"];
    NSString* override = NSProcessInfo.processInfo.environment[@"WOW_DATA_ROOT"];
    if (override.length) { dataRoot = override; }
    NSString* scratch = NSTemporaryDirectory();
    self.output.text = [NSString stringWithFormat:@"Checking core and Data at %@…\n", dataRoot];
    dispatch_async(dispatch_get_global_queue(QOS_CLASS_USER_INITIATED, 0), ^{
        std::string core, data;
        const bool coreOK = AppleBootstrap::CheckCore(scratch.fileSystemRepresentation, core);
        const bool dataOK = coreOK && AppleBootstrap::InspectData(dataRoot.fileSystemRepresentation, data);
        NSString* status = [NSString stringWithFormat:@"%@\n%s%s\nCopy the original Data folder into this app's Documents using Files or Finder.\nThis is a foundation probe; game and rendering are not linked.",
            coreOK && dataOK ? @"Foundation checks passed" : @"Checks incomplete — see details", core.c_str(), data.c_str()];
        NSLog(@"%@", status);
        dispatch_async(dispatch_get_main_queue(), ^{
            self.output.text = status;
            self.running = NO;
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
