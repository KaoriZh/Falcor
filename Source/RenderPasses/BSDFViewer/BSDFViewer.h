/***************************************************************************
 # Copyright (c) 2015-23, NVIDIA CORPORATION. All rights reserved.
 #
 # Redistribution and use in source and binary forms, with or without
 # modification, are permitted provided that the following conditions
 # are met:
 #  * Redistributions of source code must retain the above copyright
 #    notice, this list of conditions and the following disclaimer.
 #  * Redistributions in binary form must reproduce the above copyright
 #    notice, this list of conditions and the following disclaimer in the
 #    documentation and/or other materials provided with the distribution.
 #  * Neither the name of NVIDIA CORPORATION nor the names of its
 #    contributors may be used to endorse or promote products derived
 #    from this software without specific prior written permission.
 #
 # THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS "AS IS" AND ANY
 # EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 # IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 # PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 # CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 # EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 # PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 # PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 # OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 # (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 # OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 **************************************************************************/
#pragma once
#include "Falcor.h"
#include "RenderGraph/RenderPass.h"
#include "Utils/Sampling/SampleGenerator.h"
#include "Utils/Debug/PixelDebug.h"
#include "Scene/Lights/EnvMap.h"
#include "BSDFViewerParams.slang"

using namespace Falcor;

class BSDFViewer : public RenderPass
{
public:
    FALCOR_PLUGIN_CLASS(BSDFViewer, "BSDFViewer", "BSDF inspection utility.");

    static ref<BSDFViewer> create(ref<Device> pDevice, const Dictionary& dict) { return make_ref<BSDFViewer>(pDevice, dict); }

    BSDFViewer(ref<Device> pDevice, const Dictionary& dict);

    virtual Dictionary getScriptingDictionary() override;
    virtual RenderPassReflection reflect(const CompileData& compileData) override;
    virtual void compile(RenderContext* pRenderContext, const CompileData& compileData) override;
    virtual void execute(RenderContext* pRenderContext, const RenderData& renderData) override;
    virtual void renderUI(Gui::Widgets& widget) override;
    virtual void setScene(RenderContext* pRenderContext, const ref<Scene>& pScene) override;
    virtual bool onMouseEvent(const MouseEvent& mouseEvent) override;
    virtual bool onKeyEvent(const KeyboardEvent& keyEvent) override;

    static void registerBindings(pybind11::module& m);

private:
    void parseDictionary(const Dictionary& dict);
    bool loadEnvMap(const std::filesystem::path& path);
    void readPixelData();

    // Internal state
    ref<Scene>                      mpScene;                    ///< Loaded scene if any, nullptr otherwise.
    ref<EnvMap>                     mpEnvMap;                   ///< Environment map if loaded, nullptr otherwise.
    bool                            mUseEnvMap = true;          ///< Use environment map if available.

    BSDFViewerParams                mParams;                    ///< Parameters shared with the shaders.
    ref<SampleGenerator>            mpSampleGenerator;          ///< Random number generator for the integrator.
    bool                            mOptionsChanged = false;

    ref<GpuFence>                   mpFence;                    ///< GPU fence for synchronizing readback.
    ref<Buffer>                     mpPixelDataBuffer;          ///< Buffer for data for the selected pixel.
    ref<Buffer>                     mpPixelStagingBuffer;       ///< Staging buffer for readback of pixel data.
    PixelData                       mPixelData;                 ///< Pixel data for the selected pixel (if valid).
    bool                            mPixelDataValid = false;
    bool                            mPixelDataAvailable = false;

    std::unique_ptr<PixelDebug>     mpPixelDebug;               ///< Utility class for pixel debugging (print in shaders).

    ref<ComputePass>                mpViewerPass;

    // UI variables
    Gui::DropdownList               mMaterialList;
};
