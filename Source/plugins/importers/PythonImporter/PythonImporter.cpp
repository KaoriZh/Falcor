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
#include "PythonImporter.h"
#include "Scene/Importer.h"
#include "GlobalState.h"
#include "Utils/Scripting/Scripting.h"
#include <filesystem>
#include <regex>

namespace Falcor
{

namespace
{
/**
 * Parse the legacy header on the first line of the script with the syntax:
 * # filename.extension
 */
static std::optional<std::string> parseLegacyHeader(const std::string& script)
{
    if (size_t endOfFirstLine = script.find_first_of("\n\r"); endOfFirstLine != std::string::npos)
    {
        const std::regex headerRegex(R"""(#\s+([\w-]+\.[\w]{1,10}))""");

        std::smatch match;
        if (std::regex_match(script.begin(), script.begin() + endOfFirstLine, match, headerRegex))
        {
            if (match.size() > 1)
                return match[1].str();
        }
    }

    return {};
}

/// Set of currently imported paths, used to avoid recursion. TODO: REMOVEGLOBAL
static std::set<std::filesystem::path> sImportPaths;
/// Stack of import directories to properly handle adding/removing data search paths. TODO: REMOVEGLOBAL
static std::vector<std::filesystem::path> sImportDirectories;

/**
 * This class is used to handle nested imports through RAII.
 * It keeps a set of import paths in sImportPaths to detect recursive imports.
 * It keeps a stack of import directories in sImportdirectories and updates the global data search directories.
 * It also keeps track of Settings, keeping them scoped to the individual scenes
 */
class ScopedImport
{
public:
    ScopedImport(const std::filesystem::path& path, SceneBuilder& builder)
        : mScopedSettings(builder.getSettings()), mPath(path), mDirectory(path.parent_path())
    {
        FALCOR_ASSERT(path.is_absolute());
        sImportPaths.emplace(mPath);
        sImportDirectories.push_back(mDirectory);

        // Add directory to search directories (add it to the front to make it highest priority).
        addDataDirectory(mDirectory, true);

        // Set global scene builder as workaround to support old Python API.
        setActivePythonSceneBuilder(&builder);
    }
    ~ScopedImport()
    {
        auto erased = sImportPaths.erase(mPath);
        FALCOR_ASSERT(erased == 1);

        FALCOR_ASSERT(sImportDirectories.size() > 0);
        sImportDirectories.pop_back();

        // Remove script directory from search path (only if not needed by the outer importer).
        if (std::find(sImportDirectories.begin(), sImportDirectories.end(), mDirectory) == sImportDirectories.end())
        {
            removeDataDirectory(mDirectory);
        }

        // Unset global scene builder.
        if (sImportDirectories.size() == 0)
        {
            setActivePythonSceneBuilder(nullptr);
        }
    }

private:
    ScopedSettings mScopedSettings;
    std::filesystem::path mPath;
    std::filesystem::path mDirectory;
};

static bool isRecursiveImport(const std::filesystem::path& path)
{
    return sImportPaths.find(path) != sImportPaths.end();
}

} // namespace

std::unique_ptr<Importer> PythonImporter::create()
{
    return std::make_unique<PythonImporter>();
}

void PythonImporter::importScene(const std::filesystem::path& path, SceneBuilder& builder, const pybind11::dict& dict)
{
    if (!path.is_absolute())
        throw ImporterError(path, "Expected absolute path.");

    if (isRecursiveImport(path))
        throw ImporterError(path, "Scene is imported recursively.");

    // Load the script file
    const std::string script = readFile(path);

    // Check for legacy .pyscene file format.
    if (auto sceneFile = parseLegacyHeader(script))
        throw ImporterError(path, "Python scene file is using old header comment syntax. Use the new 'sceneBuilder' object instead.");

    // Keep track of this import and add script directory to data search directories.
    // We use RAII here to make sure the scope is properly removed when throwing an exception.
    ScopedImport scopedImport(path, builder);

    // Execute script.
    try
    {
        Scripting::Context context;
        context.setObject("sceneBuilder", &builder);
        Scripting::runScript("from falcor import *", context);
        Scripting::runScriptFromFile(path, context);
    }
    catch (const std::exception& e)
    {
        throw ImporterError(path, fmt::format("Failed to run python scene script: {}", e.what()));
    }
}

extern "C" FALCOR_API_EXPORT void registerPlugin(Falcor::PluginRegistry& registry)
{
    registry.registerClass<Importer, PythonImporter>();
}

} // namespace Falcor
