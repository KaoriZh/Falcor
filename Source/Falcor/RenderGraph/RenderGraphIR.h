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
#include "Core/Macros.h"
#include "Core/API/Formats.h"
#include "Utils/Scripting/Dictionary.h"
#include <memory>
#include <string>

namespace Falcor
{
class Scene;

class FALCOR_API RenderGraphIR
{
public:
    RenderGraphIR(const std::string& name, bool newGraph = true);

    void createPass(const std::string& passClass, const std::string& passName, const Dictionary& = Dictionary());
    void updatePass(const std::string& passName, const Dictionary& dictionary);
    void removePass(const std::string& passName);
    void addEdge(const std::string& src, const std::string& dst);
    void removeEdge(const std::string& src, const std::string& dst);
    void markOutput(const std::string& name, const TextureChannelFlags mask = TextureChannelFlags::RGB);
    void unmarkOutput(const std::string& name);

    std::string getIR() { return mIR + mIndentation + (mIndentation.size() ? "return g\n" : "\n"); }

    static std::string getFuncName(const std::string& graphName);

private:
    std::string mName;
    std::string mIR;
    std::string mIndentation;
    std::string mGraphPrefix;
};
} // namespace Falcor
