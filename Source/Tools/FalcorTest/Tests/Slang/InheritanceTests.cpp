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
#include "Testing/UnitTest.h"
#include "Utils/HostDeviceShared.slangh"

#include <limits>
#include <random>

namespace Falcor
{

namespace
{
std::pair<int, float2> getCpuResult(int type, int2 value, float3 data)
{
    switch (type)
    {
    case 0:
        return std::make_pair(value[0] - value[1], float2(data[0] - data[1], -data[2]));
    case 1:
        return std::make_pair(value[0] - value[1] + 1, float2(data[0], data[2]));
    case 2:
        return std::make_pair(value[0] - value[1] + 2, float2(data[0], -data[2]));
    case 3:
        return std::make_pair(value[0] - value[1] + 3, float2(data[0] + data[1], data[2]));
    }
    return std::make_pair(-65537, float2(std::numeric_limits<float>::quiet_NaN()));
}

const uint32_t kNumTests = 16;
std::mt19937 r;
std::uniform_real_distribution uf;
std::uniform_int_distribution ui;

} // namespace

GPU_TEST(Inheritance_ManualCreate)
{
    ref<Device> pDevice = ctx.getDevice();

    Program::DefineList defines;
    defines.add("NUM_TESTS", std::to_string(kNumTests));
    ctx.createProgram("Tests/Slang/InheritanceTests.cs.slang", "testInheritanceManual", defines, Shader::CompilerFlags::None, "6_5");
    ctx.allocateStructuredBuffer("resultsInt", kNumTests);
    ctx.allocateStructuredBuffer("resultsFloat", kNumTests);

    std::vector<int> testType(kNumTests);
    // The first value is value0 in TestInterfaceBase, second is value1 in the inherited classes.
    // This tests that the memory order of base class and inherited class members has not changed with Slang updates.
    std::vector<int2> testValue(kNumTests);
    std::vector<float3> data(kNumTests);

    for (size_t i = 0; i < kNumTests; ++i)
    {
        testType[i] = i % 4;
        testValue[i][0] = ui(r);
        testValue[i][1] = ui(r);
        data[i][0] = float(uf(r));
        data[i][1] = float(uf(r));
        data[i][2] = float(uf(r));
    }

    auto var = ctx.vars().getRootVar();
    var["testType"] = Buffer::createStructured(
        pDevice, var["testType"], (uint32_t)testType.size(), ResourceBindFlags::ShaderResource, Buffer::CpuAccess::None, testType.data()
    );
    var["testValue"] = Buffer::createStructured(
        pDevice, var["testValue"], (uint32_t)testValue.size(), ResourceBindFlags::ShaderResource, Buffer::CpuAccess::None, testValue.data()
    );
    var["data"] = Buffer::createStructured(
        pDevice, var["data"], (uint32_t)testType.size(), ResourceBindFlags::ShaderResource, Buffer::CpuAccess::None, data.data()
    );

    ctx.runProgram(kNumTests, 1, 1);

    // Verify results.
    const int* resultsInt = ctx.mapBuffer<const int>("resultsInt");
    const float2* resultsFloat = ctx.mapBuffer<const float2>("resultsFloat");
    for (uint32_t i = 0; i < kNumTests; i++)
    {
        const auto expected = getCpuResult(testType[i], testValue[i], data[i]);
        EXPECT_EQ(resultsInt[i], expected.first) << "i = " << i;
        EXPECT_EQ(resultsFloat[i], expected.second) << "i = " << i;
    }
    ctx.unmapBuffer("resultsInt");
    ctx.unmapBuffer("resultsFloat");
}

GPU_TEST(Inheritance_ConformanceCreate)
{
    ref<Device> pDevice = ctx.getDevice();

    Program::DefineList defines;
    defines.add("NUM_TESTS", std::to_string(kNumTests));
    Program::Desc desc;
    desc.addShaderLibrary("Tests/Slang/InheritanceTests.cs.slang");
    desc.csEntry("testInheritanceConformance");
    desc.setShaderModel("6_5");

    Program::TypeConformanceList typeConformancess{
        {{"TestV0SubNeg", "ITestInterface"}, 0},
        {{"TestV1DefDef", "ITestInterface"}, 1},
        {{"TestV2DefNeg", "ITestInterface"}, 2},
        {{"TestV3SumDef", "ITestInterface"}, 3},
    };
    desc.addTypeConformances(typeConformancess);

    ctx.createProgram(desc, defines);
    ctx.allocateStructuredBuffer("resultsInt", kNumTests);
    ctx.allocateStructuredBuffer("resultsFloat", kNumTests);

    std::vector<int> testType(kNumTests);
    // The first value is value0 in TestInterfaceBase, second is value1 in the inherited classes.
    // This tests that the memory order of base class and inherited class members has not changed with Slang updates.
    std::vector<int2> testValue(kNumTests);
    std::vector<float3> data(kNumTests);

    for (size_t i = 0; i < kNumTests; ++i)
    {
        testType[i] = i % 4;
        testValue[i][0] = ui(r);
        testValue[i][1] = ui(r);
        data[i][0] = float(uf(r));
        data[i][1] = float(uf(r));
        data[i][2] = float(uf(r));
    }

    auto var = ctx.vars().getRootVar();
    var["testType"] = Buffer::createStructured(
        pDevice, var["testType"], (uint32_t)testType.size(), ResourceBindFlags::ShaderResource, Buffer::CpuAccess::None, testType.data()
    );
    var["testValue"] = Buffer::createStructured(
        pDevice, var["testValue"], (uint32_t)testValue.size(), ResourceBindFlags::ShaderResource, Buffer::CpuAccess::None, testValue.data()
    );
    var["data"] = Buffer::createStructured(
        pDevice, var["data"], (uint32_t)testType.size(), ResourceBindFlags::ShaderResource, Buffer::CpuAccess::None, data.data()
    );

    ctx.runProgram(kNumTests, 1, 1);

    // Verify results.
    const int* resultsInt = ctx.mapBuffer<const int>("resultsInt");
    const float2* resultsFloat = ctx.mapBuffer<const float2>("resultsFloat");
    for (uint32_t i = 0; i < kNumTests; i++)
    {
        const auto expected = getCpuResult(testType[i], testValue[i], data[i]);
        EXPECT_EQ(resultsInt[i], expected.first) << "i = " << i;
        EXPECT_EQ(resultsFloat[i], expected.second) << "i = " << i;
    }
    ctx.unmapBuffer("resultsInt");
    ctx.unmapBuffer("resultsFloat");
}
/// This correctly and reliably fails, but there is no way to automatically test it.
// GPU_TEST(Inheritance_CheckInvalid)
// {
//     Program::DefineList defines;
//     defines.add("NUM_TESTS", std::to_string(kNumTests));
//     defines.add("COMPILE_WITH_ERROR", "1");

//     ctx.createProgram("Tests/Slang/InheritanceTests.cs.slang", "testInheritance", defines, Shader::CompilerFlags::None, "6_5");
// }

} // namespace Falcor
