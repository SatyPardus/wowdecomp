#include "fmod_settings.h"
#include "AppleOutput.h"
#include "fmod_outputi.h"
#include "fmod_pluginfactory.h"
#include "fmod_os_output.h"
#import <AudioToolbox/AudioToolbox.h>
#import <Foundation/Foundation.h>
#include <TargetConditionals.h>
#if TARGET_OS_IPHONE
#import <AVFAudio/AVFAudio.h>
#else
#import <CoreAudio/CoreAudio.h>
#endif
#include <atomic>
#include <cstdio>
#include <cstring>
#include <new>
#include <memory>
#include <vector>

namespace {
struct OutputState {
    AudioUnit unit = nullptr;
    FMOD_OUTPUT_STATE* mixer = nullptr;
    std::vector<uint8_t> block;
    unsigned blockFrames = 0, available = 0, offset = 0;
    int rate = 0, channels = 0;
    unsigned sampleBytes = 0;
    bool floatSamples = false;
    bool requested = false, active = true, running = false;
    std::atomic<bool> interrupted{false};
    std::atomic<FMOD_RESULT> error{FMOD_OK};
    std::atomic<uint64_t> frames{0}, callbacks{0}, nonzero{0};
#if TARGET_OS_IPHONE
    id interruptionObserver = nil;
    id resetObserver = nil;
#endif
};
OutputState* State(FMOD_OUTPUT_STATE* output) {
    auto* owner = static_cast<std::shared_ptr<OutputState>*>(output->plugindata);
    return owner ? owner->get() : nullptr;
}
FMOD_RESULT DriverError(OSStatus status) {
    if (!status) return FMOD_OK;
    fprintf(stderr, "Apple audio driver error: %d\n", int(status));
    return FMOD_ERR_OUTPUT_DRIVERCALL;
}
OSStatus Render(void* ref, AudioUnitRenderActionFlags* flags, const AudioTimeStamp*, UInt32, UInt32 frames, AudioBufferList* buffers) {
    auto* state = static_cast<OutputState*>(ref);
    if (buffers->mNumberBuffers != 1 || !buffers->mBuffers[0].mData ||
        buffers->mBuffers[0].mDataByteSize < frames * unsigned(state->channels) * state->sampleBytes) {
        state->error.store(FMOD_ERR_OUTPUT_FORMAT, std::memory_order_relaxed);
        return kAudio_ParamError;
    }
    auto* destination = static_cast<uint8_t*>(buffers->mBuffers[0].mData);
    const size_t frameBytes = state->channels * state->sampleBytes;
    unsigned remaining = frames;
    while (remaining) {
        if (!state->available) {
            FMOD_RESULT result = state->mixer->readfrommixer(state->mixer, state->block.data(), state->blockFrames);
            if (result != FMOD_OK) {
                state->error.store(result, std::memory_order_relaxed);
                memset(destination, 0, size_t(remaining) * frameBytes);
                *flags |= kAudioUnitRenderAction_OutputIsSilence;
                return noErr;
            }
            state->available = state->blockFrames;
            state->offset = 0;
        }
        unsigned count = remaining < state->available ? remaining : state->available;
        memcpy(destination, state->block.data() + size_t(state->offset) * frameBytes,
               size_t(count) * frameBytes);
        destination += size_t(count) * frameBytes;
        remaining -= count;
        state->offset += count;
        state->available -= count;
    }
    uint64_t nonzero = 0;
    if (state->floatSamples) {
        auto* samples = static_cast<const float*>(buffers->mBuffers[0].mData);
        for (size_t i = 0; i < size_t(frames) * state->channels; ++i) nonzero += samples[i] != 0.0f;
    } else {
        auto* samples = static_cast<const int16_t*>(buffers->mBuffers[0].mData);
        for (size_t i = 0; i < size_t(frames) * state->channels; ++i) nonzero += samples[i] != 0;
    }
    state->nonzero.fetch_add(nonzero, std::memory_order_relaxed);
    state->frames.fetch_add(frames, std::memory_order_relaxed);
    state->callbacks.fetch_add(1, std::memory_order_relaxed);
    return noErr;
}
FMOD_RESULT ApplyState(OutputState* state) {
    bool run = state->requested && state->active && !state->interrupted.load(std::memory_order_acquire);
    if (run == state->running) return state->error.load(std::memory_order_relaxed);
#if TARGET_OS_IPHONE
    if (run && ![[AVAudioSession sharedInstance] setActive:YES error:nil]) return FMOD_ERR_OUTPUT_INIT;
#endif
    FMOD_RESULT result = DriverError(run ? AudioOutputUnitStart(state->unit) : AudioOutputUnitStop(state->unit));
    if (result == FMOD_OK) state->running = run;
#if TARGET_OS_IPHONE
    if (!run && !state->interrupted.load(std::memory_order_acquire))
        [[AVAudioSession sharedInstance] setActive:NO withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation error:nil];
#endif
    return result;
}
FMOD_RESULT Close(FMOD_OUTPUT_STATE* output) {
    auto* state = State(output);
    if (!state) return FMOD_OK;
#if TARGET_OS_IPHONE
    if (state->interruptionObserver) [[NSNotificationCenter defaultCenter] removeObserver:state->interruptionObserver];
    if (state->resetObserver) [[NSNotificationCenter defaultCenter] removeObserver:state->resetObserver];
#endif
    if (state->unit) {
        AudioOutputUnitStop(state->unit);
        AudioUnitUninitialize(state->unit);
        AudioComponentInstanceDispose(state->unit);
    }
#if TARGET_OS_IPHONE
    [[AVAudioSession sharedInstance] setActive:NO withOptions:AVAudioSessionSetActiveOptionNotifyOthersOnDeactivation error:nil];
#endif
    delete static_cast<std::shared_ptr<OutputState>*>(output->plugindata);
    output->plugindata = nullptr;
    return FMOD_OK;
}
FMOD_RESULT Init(FMOD_OUTPUT_STATE* output, int driver, FMOD_INITFLAGS, int* rate, int channels,
                 FMOD_SOUND_FORMAT* format, int blockFrames, int, void*) {
    if (driver != 0 || !rate || !format || blockFrames <= 0) return FMOD_ERR_INVALID_PARAM;
    // First output path supports the game's mono/stereo configuration. Surround
    // needs an explicit FMOD-to-CoreAudio channel layout before it can be exposed.
    if (channels < 1 || channels > 2) return FMOD_ERR_OUTPUT_FORMAT;
    if (*format != FMOD_SOUND_FORMAT_PCM16 && *format != FMOD_SOUND_FORMAT_PCMFLOAT) return FMOD_ERR_OUTPUT_FORMAT;
    std::shared_ptr<OutputState>* owner = nullptr;
    try { owner = new std::shared_ptr<OutputState>(std::make_shared<OutputState>()); }
    catch (...) { return FMOD_ERR_MEMORY; }
    auto* state = owner->get();
    output->plugindata = owner;
    state->mixer = output;
    state->channels = channels;
    state->blockFrames = unsigned(blockFrames);
    state->floatSamples = *format == FMOD_SOUND_FORMAT_PCMFLOAT;
    state->sampleBytes = state->floatSamples ? sizeof(float) : sizeof(int16_t);
    try { state->block.resize(size_t(blockFrames) * channels * state->sampleBytes); }
    catch (...) { Close(output); return FMOD_ERR_MEMORY; }
    AudioComponentDescription description{};
    description.componentType = kAudioUnitType_Output;
    description.componentManufacturer = kAudioUnitManufacturer_Apple;
#if TARGET_OS_IPHONE
    description.componentSubType = kAudioUnitSubType_RemoteIO;
    AVAudioSession* session = [AVAudioSession sharedInstance];
    NSError* error = nil;
    if (![session setCategory:AVAudioSessionCategoryAmbient error:&error] ||
        ![session setPreferredSampleRate:*rate error:&error] ||
        ![session setPreferredIOBufferDuration:double(blockFrames) / *rate error:&error] ||
        ![session setActive:YES error:&error]) {
        NSLog(@"Apple audio session: %@", error);
        Close(output);
        return FMOD_ERR_OUTPUT_INIT;
    }
    // Preferences are hints; query the actual device rate after activation.
    *rate = int(session.sampleRate);
    std::weak_ptr<OutputState> observed = *owner;
    state->interruptionObserver = [[NSNotificationCenter defaultCenter]
        addObserverForName:AVAudioSessionInterruptionNotification object:session queue:[NSOperationQueue mainQueue]
        usingBlock:^(NSNotification* notification) {
            auto state = observed.lock();
            if (!state) return;
            bool begin = [notification.userInfo[AVAudioSessionInterruptionTypeKey] unsignedIntegerValue] == AVAudioSessionInterruptionTypeBegan;
            if (begin) state->interrupted.store(true, std::memory_order_release);
            else if ([notification.userInfo[AVAudioSessionInterruptionOptionKey] unsignedIntegerValue] & AVAudioSessionInterruptionOptionShouldResume)
                state->interrupted.store(false, std::memory_order_release);
            ApplyState(state.get());
        }];
    state->resetObserver = [[NSNotificationCenter defaultCenter]
        addObserverForName:AVAudioSessionMediaServicesWereResetNotification object:session queue:[NSOperationQueue mainQueue]
        usingBlock:^(NSNotification*) {
            if (auto state = observed.lock()) state->error.store(FMOD_ERR_OUTPUT_INIT, std::memory_order_relaxed);
        }];
#else
    description.componentSubType = kAudioUnitSubType_DefaultOutput;
    AudioDeviceID device = kAudioObjectUnknown;
    AudioObjectPropertyAddress address{kAudioHardwarePropertyDefaultOutputDevice, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
    UInt32 size = sizeof(device);
    OSStatus status = AudioObjectGetPropertyData(kAudioObjectSystemObject, &address, 0, nullptr, &size, &device);
    if (status || device == kAudioObjectUnknown) { Close(output); return FMOD_ERR_OUTPUT_INIT; }
    Float64 hardwareRate = 0;
    address = {kAudioDevicePropertyNominalSampleRate, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain};
    size = sizeof(hardwareRate);
    status = AudioObjectGetPropertyData(device, &address, 0, nullptr, &size, &hardwareRate);
    if (status) { Close(output); return DriverError(status); }
    *rate = int(hardwareRate);
#endif
    state->rate = *rate;
    AudioComponent component = AudioComponentFindNext(nullptr, &description);
    if (!component) { Close(output); return FMOD_ERR_OUTPUT_INIT; }
    OSStatus result = AudioComponentInstanceNew(component, &state->unit);
    AudioStreamBasicDescription stream{};
    stream.mSampleRate = *rate;
    stream.mFormatID = kAudioFormatLinearPCM;
    stream.mFormatFlags = state->floatSamples ? kAudioFormatFlagsNativeFloatPacked :
        kAudioFormatFlagIsSignedInteger | kAudioFormatFlagIsPacked | kAudioFormatFlagsNativeEndian;
    stream.mBytesPerPacket = stream.mBytesPerFrame = UInt32(channels) * state->sampleBytes;
    stream.mFramesPerPacket = 1;
    stream.mChannelsPerFrame = channels;
    stream.mBitsPerChannel = state->sampleBytes * 8;
    if (!result) result = AudioUnitSetProperty(state->unit, kAudioUnitProperty_StreamFormat, kAudioUnitScope_Input, 0, &stream, sizeof(stream));
    AURenderCallbackStruct callback{Render, state};
    if (!result) result = AudioUnitSetProperty(state->unit, kAudioUnitProperty_SetRenderCallback, kAudioUnitScope_Input, 0, &callback, sizeof(callback));
    if (!result) result = AudioUnitInitialize(state->unit);
    if (result) { Close(output); return DriverError(result); }
    return FMOD_OK;
}
FMOD_RESULT NumDrivers(FMOD_OUTPUT_STATE*, int* count) { *count = 1; return FMOD_OK; }
FMOD_RESULT DriverName(FMOD_OUTPUT_STATE*, int driver, char* name, int length) {
    if (driver || !name || length <= 0) return FMOD_ERR_INVALID_PARAM;
    snprintf(name, size_t(length), "Apple system output");
    return FMOD_OK;
}
FMOD_RESULT Caps(FMOD_OUTPUT_STATE*, int driver, FMOD_CAPS* caps) {
    if (driver || !caps) return FMOD_ERR_INVALID_PARAM;
    *caps = FMOD_CAPS_OUTPUT_FORMAT_PCM16 | FMOD_CAPS_OUTPUT_FORMAT_PCMFLOAT;
    return FMOD_OK;
}
FMOD_RESULT Start(FMOD_OUTPUT_STATE* output) {
    auto* state = State(output);
    state->requested = true;
    return ApplyState(state);
}
FMOD_RESULT Stop(FMOD_OUTPUT_STATE* output) {
    auto* state = State(output);
    if (!state) return FMOD_OK;
    state->requested = false;
    return ApplyState(state);
}
FMOD_RESULT Update(FMOD_OUTPUT_STATE* output) { return ApplyState(State(output)); }
FMOD_RESULT OutputHandle(FMOD_OUTPUT_STATE* output, void** handle) { *handle = State(output); return FMOD_OK; }
FMOD_RESULT GetState(FMOD::System* system, OutputState** state) {
    if (!system) return FMOD_ERR_INVALID_PARAM;
    FMOD_OUTPUTTYPE type;
    FMOD_RESULT result = system->getOutput(&type);
    if (result != FMOD_OK) return result;
    if (type != FMOD_OUTPUTTYPE_COREAUDIO) return FMOD_ERR_UNSUPPORTED;
    return system->getOutputHandle(reinterpret_cast<void**>(state));
}
}
FMOD_RESULT FMOD_OS_Output_Register(FMOD::PluginFactory* factory) {
    FMOD::FMOD_OUTPUT_DESCRIPTION_EX description{};
    description.name = "Apple Audio Unit output";
    description.version = 0x00010000;
    description.getnumdrivers = NumDrivers;
    description.getdrivername = DriverName;
    description.getdrivercaps = Caps;
    description.init = Init;
    description.close = Close;
    description.update = Update;
    description.start = Start;
    description.stop = Stop;
    description.gethandle = OutputHandle;
    description.mType = FMOD_OUTPUTTYPE_COREAUDIO;
    description.mSize = sizeof(FMOD::Output);
    return factory->registerOutput(&description);
}
FMOD_RESULT FMOD_OS_Output_GetDefault(FMOD_OUTPUTTYPE* type) {
    if (!type) return FMOD_ERR_INVALID_PARAM;
    *type = FMOD_OUTPUTTYPE_COREAUDIO;
    return FMOD_OK;
}
FMOD_RESULT FMOD_OS_CheckDriverList(bool* changed) {
    if (!changed) return FMOD_ERR_INVALID_PARAM;
    // One logical system-output route; AudioUnit follows physical route changes.
    *changed = false;
    return FMOD_OK;
}
FMOD_RESULT FMODAppleSetActive(FMOD::System* system, bool active) {
    OutputState* state = nullptr;
    FMOD_RESULT result = GetState(system, &state);
    if (result != FMOD_OK) return result;
    if (!state) return FMOD_ERR_UNINITIALIZED;
    state->active = active;
    return ApplyState(state);
}
FMOD_RESULT FMODAppleGetOutputStats(FMOD::System* system, FMODAppleOutputStats* stats) {
    if (!stats) return FMOD_ERR_INVALID_PARAM;
    OutputState* state = nullptr;
    FMOD_RESULT result = GetState(system, &state);
    if (result != FMOD_OK) return result;
    if (!state) return FMOD_ERR_UNINITIALIZED;
    *stats = {state->frames.load(), state->callbacks.load(), state->nonzero.load(), state->rate, state->channels, state->running};
    return state->error.load(std::memory_order_relaxed);
}
