#ifndef WOW_SOURCE_EXTENSION_SCRIPTDEFINES_MODELSCRIPT_H
#define WOW_SOURCE_EXTENSION_SCRIPTDEFINES_MODELSCRIPT_H

#include <cstdint>
#include "WoW/Source/Extension/ScriptObject.h"

class CM2Model;
class CM2Shared;

// The M2 model pipeline (Engine/Source/Model2). One CM2Shared holds a model file: the relocated
// M2Data (m_data), the chosen .skin profile (m_skinData), textures, GPU buffers and effects; it is
// shared by every CM2Model instance of that file through CM2Cache. Loading is asynchronous: the .m2
// read completes in CM2Shared::FinishLoading, which issues the .skin read, which completes in
// CM2Shared::FinishLoadingSkinProfile; a CM2Model created before both are done waits on the
// shared's callback list and is initialised when the skin arrives. Every hook passes the real
// object by reference so a script can read extra data from it or attach its own state keyed by
// the object's address (released in the matching Destroy hook).
class ModelScript : public ScriptObject {
    protected:
    explicit ModelScript(const char* name);

    public:
    // End of CM2Shared::FinishLoading: the .m2 file is in memory and relocated (every M2Array of
    // shared.m_data points into the buffer, shared.m_fileSize bytes), the materials are flagged,
    // the textures created, the skin profile chosen and its read issued. shared.m_skinData is not
    // usable yet. shared.m_fileName is the model path.
    virtual void OnModelSharedLoaded(CM2Shared& shared) {}

    // CM2Shared::FinishLoadingSkinProfile after the profile is relocated and initialised (vertex
    // and index buffers, batches, shader effects) and before the models waiting on this shared
    // are initialised. shared.m_skinData is the M2SkinProfile of the profile the device's bone
    // budget selected (the NN in <model>NN.skin).
    virtual void OnModelSkinLoaded(CM2Shared& shared) {}

    // CM2Shared::Initialize after the texture loop: shared.m_textures holds one CTexture per
    // M2Texture of the model (a file texture for a named one, a white solid for a runtime
    // replaceable one). Fires before OnModelSharedLoaded, while the skin read is being issued.
    virtual void OnModelTexturesCreated(CM2Shared& shared) {}

    // CM2Shared::FinishLoadingLowPrioritySequence once a .anim file's data is relocated into
    // the model's sequence (M2Init with the sequence's buffer) and the sequence is flagged
    // loaded. Fires once per sequence index the file satisfies, aliases included, from the
    // async-read completion; sequenceIndex indexes shared.m_data->m_sequences.
    virtual void OnModelSequenceLoaded(CM2Shared& shared, uint32_t sequenceIndex) {}

    // Start of ~CM2Shared: the file data, skin, textures and buffers are still allocated. Fires
    // when the cache evicts the shared or the last reference goes with no cache.
    virtual void OnModelSharedDestroy(CM2Shared& shared) {}

    // End of CM2Model::Initialize, the one constructor path of CM2Scene::CreateModel and
    // DuplicateModel: the instance is attached to its scene, holds its shared and its flags, and
    // the attachments of a duplicated source are copied. Not loaded yet; the shared may still be
    // reading. Always fires before OnModelLoaded for that instance.
    virtual void OnModelCreated(CM2Model& model) {}

    // End of CM2Model::InitializeLoaded: the shared has both its data and skin, the instance's
    // bones, bone matrices, loops, textures, particle and ribbon emitters, lights, colors,
    // texture transforms and cameras are allocated, attachments resolved and the model calls
    // queued while it was loading (sequences, textures, lights...) replayed. The model can be
    // rendered from here on.
    virtual void OnModelLoaded(CM2Model& model) {}

    // CM2Scene::Animate, in the single-threaded pass, right after a root model's AnimateST: the
    // bone matrices, particle and ribbon emitters, lights and texture transforms of this model
    // and of its attached children are computed for the frame (the multithreaded AnimateMT pass
    // ran before), lighting not yet set up, nothing drawn. Only models that were queued for
    // animation this frame (visible ones) come here, and only roots: children animate with
    // their parent. Every visible model every frame, on the main thread: keep it cheap. The
    // dispatcher skips the loop entirely when no ModelScript is registered.
    virtual void OnModelAnimated(CM2Model& model) {}

    // Start of ~CM2Model: the instance is about to release its shared and free its arrays;
    // everything is still valid for reading.
    virtual void OnModelDestroy(CM2Model& model) {}
};

#endif
