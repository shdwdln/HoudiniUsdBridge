/*
 * Copyright 2020 Side Effects Software Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "GEO_HDAFileFormat.h"
#include "GEO_HAPIUtils.h"
#include "pxr/base/tf/fileUtils.h"
#include "pxr/base/tf/pathUtils.h"
#include "pxr/base/tf/registryManager.h"
#include "pxr/base/tf/staticData.h"
#include "pxr/base/trace/trace.h"
#include "pxr/usd/pcp/dynamicFileFormatContext.h"
#include "pxr/usd/sdf/layer.h"
#include "pxr/usd/usd/usdaFileFormat.h"
#include <UT/UT_WorkBuffer.h>

PXR_NAMESPACE_OPEN_SCOPE

TF_DEFINE_PUBLIC_TOKENS(GEO_HDAFileFormatTokens, GEO_HDA_FILE_FORMAT_TOKENS);

// These must match the names of the metadata defined in the plugInfo.json file
static const TfToken theParamDictToken("HDAParms");
static const TfToken theOptionDictToken("HDAOptions");
static const TfToken theAssetNameToken("HDAAssetName");
static const TfToken theTimeCacheModeToken("HDATimeCacheMode");
static const TfToken theTimeCacheStartToken("HDATimeCacheStart");
static const TfToken theTimeCacheEndToken("HDATimeCacheEnd");
static const TfToken theTimeCacheIntervalToken("HDATimeCacheInterval");
static const TfToken theHAPISessionToken("HDAKeepEngineOpen");

TF_REGISTRY_FUNCTION_WITH_TAG(TfType, GEO_GEO_HDAFileFormat)
{
    SDF_DEFINE_FILE_FORMAT(GEO_HDAFileFormat, SdfFileFormat);
}

GEO_HDAFileFormat::GEO_HDAFileFormat()
    : SdfFileFormat(GEO_HDAFileFormatTokens->Id,      // id
                    GEO_HDAFileFormatTokens->Version, // version
                    GEO_HDAFileFormatTokens->Target,  // target
                    GEO_HDAFileFormatTokens->Id)      // extension
{
}

GEO_HDAFileFormat::~GEO_HDAFileFormat() {}

static const UT_ArrayStringSet theExtensions({"hda", "otl", "hdanc", "otlnc",
                                              "hdalc", "otllc"});

bool
GEO_HDAFileFormat::CanRead(const std::string &filePath) const
{
    return theExtensions.contains(TfGetExtension(filePath));
}

bool
GEO_HDAFileFormat::Read(SdfLayer *layer,
                        const std::string &resolvedPath,
                        bool metadataOnly) const
{
    SdfAbstractDataRefPtr data =
        GEO_HDAFileData::New(layer->GetFileFormatArguments());
    GEO_HDAFileDataRefPtr geoData = TfStatic_cast<GEO_HDAFileDataRefPtr>(data);

    if (!geoData->Open(resolvedPath))
    {
        return false;
    }

    _SetLayerData(layer, data);
    return true;
}

bool
GEO_HDAFileFormat::CanFieldChangeAffectFileFormatArguments(
    const TfToken &field,
    const VtValue &oldValue,
    const VtValue &newValue,
    const VtValue &dependencyContextData) const
{
    // If any of the time caching range settings are changed when the cache mode
    // is not set to range, the file format arguments will be unaffected
    if (field == theTimeCacheStartToken || field == theTimeCacheEndToken ||
        field == theTimeCacheIntervalToken)
    {
        if (dependencyContextData.IsHolding<GEO_HAPITimeCaching>())
        {
            GEO_HAPITimeCaching mode =
                dependencyContextData.UncheckedGet<GEO_HAPITimeCaching>();

            if (mode != GEO_HAPI_TIME_CACHING_RANGE)
                return false;
        }
    }
    return oldValue != newValue;
}

// Add a numerical entry to args based on the type and value of parmData.
// Nothing happens if the data is not numeric
//
// The added key will follow the form: "PREFIX TUPLE_INDEX NAME"
// Elements of GtVec will be space separated
//
// parmData is passed by value because VtValue.Cast() will mutate it.
static void
addNumericNodeParmToFormatArgs(SdfFileFormat::FileFormatArguments *args,
                                const std::string &parmName,
                                VtValue parmData,
                                UT_WorkBuffer &parmBuf,
                                UT_WorkBuffer &valBuf)
{
    auto addToArgs = [&](const auto *data, int count) {
        // Add the prefix to the parameter's name
        parmBuf.format("{0}{1}", GEO_HDA_PARM_NUMERIC_PREFIX, parmName);

        // Add each element to the buffer
        valBuf.format("{0}", data[0]);
        for (int i = 1; i < count; i++)
        {
            valBuf.appendFormat("{0}{1}", GEO_HDA_PARM_SEPARATOR, data[i]);
        }

        // Add the key/value pair to args
        (*args)[parmBuf.toStdString()] = valBuf.toStdString();
    };

    // Try casting to double to ensure the VtValue is numeric

    if (parmData.CanCast<double>())
    {
        const double &val = parmData.Cast<double>().UncheckedGet<double>();
        addToArgs(&val, 1);
    }
    else if (parmData.CanCast<GfVec2d>())
    {
        const GfVec2d &vec = parmData.Cast<GfVec2d>().UncheckedGet<GfVec2d>();
        const double *vals = vec.GetArray();
        addToArgs(vals, 2);
    }
    else if (parmData.CanCast<GfVec3d>())
    {
        const GfVec3d &vec = parmData.Cast<GfVec3d>().UncheckedGet<GfVec3d>();
        const double *vals = vec.GetArray();
        addToArgs(vals, 3);
    }
    else if (parmData.CanCast<GfVec4d>())
    {
        const GfVec4d &vec = parmData.Cast<GfVec4d>().UncheckedGet<GfVec4d>();
        const double *vals = vec.GetArray();
        addToArgs(vals, 4);
    }
    else if (parmData.CanCast<VtDoubleArray>())
    {
        auto &vals =
            parmData.Cast<VtDoubleArray>().UncheckedGet<VtDoubleArray>();
        addToArgs(vals.data(), vals.size());
    }
    else if (parmData.CanCast<VtInt64Array>())
    {
        auto &vals =
            parmData.Cast<VtInt64Array>().UncheckedGet<VtInt64Array>();
        addToArgs(vals.data(), vals.size());
    }
    else
    {
        TF_WARN("Unexpected data type '%s' for parameter.",
                parmData.GetTypeName().c_str());
    }
}

// Compose file format arguments based on predefined metedata fields
void
GEO_HDAFileFormat::ComposeFieldsForFileFormatArguments(
    const std::string &assetPath,
    const PcpDynamicFileFormatContext &context,
    FileFormatArguments *args,
    VtValue *dependencyContextData) const
{
    VtValue contextVal;

    // All of the metadata fields for this dynamic format must be defined in the
    // coressponding pluginfo.json file. Since HDAs can have arbitrary
    // parameters, we use a single dict to store the parameter names, and values.
    if (context.ComposeValue(theParamDictToken, &contextVal) &&
        contextVal.IsHolding<VtDictionary>())
    {
        // Add each parameter in this dict to args
        VtDictionary paramDict = contextVal.UncheckedGet<VtDictionary>();

        UT_WorkBuffer parmBuf;
        UT_WorkBuffer valBuf;
        VtDictionary::iterator end = paramDict.end();
        for (VtDictionary::iterator it = paramDict.begin(); it != end; it++)
        {
            const std::string &parmName = it->first;
            VtValue &data = it->second;

            // Check if we have a string arg
            if (data.IsHolding<std::string>())
            {
                const std::string &strData = data.UncheckedGet<std::string>();

                // Add string type to args
                parmBuf.sprintf(
                    "%s%s", GEO_HDA_PARM_STRING_PREFIX, parmName.c_str());
                (*args)[parmBuf.toStdString()] = strData;
            }
            else
            {
                addNumericNodeParmToFormatArgs(
                    args, parmName, data, parmBuf, valBuf);
            }
        }
    }

    // Options Dictionary
    if (context.ComposeValue(theOptionDictToken, &contextVal) &&
        contextVal.IsHolding<VtDictionary>())
    {
        VtDictionary optDict = contextVal.UncheckedGet<VtDictionary>();

        VtDictionary::iterator end = optDict.end();
        for (VtDictionary::iterator it = optDict.begin(); it != end; it++)
        {
            const std::string &optName = it->first;
            VtValue &data = it->second;

            if (data.IsHolding<std::string>())
            {
                const std::string &out = data.UncheckedGet<std::string>();
                (*args)[optName] = out;
            }
        }
    }

    // Asset name
    if (context.ComposeValue(theAssetNameToken, &contextVal) &&
        contextVal.IsHolding<std::string>())
    {
        const std::string &name = contextVal.UncheckedGet<std::string>();
        (*args)["assetname"] = name;
    }

    GEO_HAPITimeCaching cacheContext = GEO_HAPI_TIME_CACHING_NONE;

    // Time caching metadata
    if (context.ComposeValue(theTimeCacheModeToken, &contextVal) &&
        contextVal.IsHolding<std::string>())
    {
        const std::string &mode = contextVal.UncheckedGet<std::string>();
        
        if (mode == "none")
            cacheContext = GEO_HAPI_TIME_CACHING_NONE;
        else if (mode == "continuous")
            cacheContext = GEO_HAPI_TIME_CACHING_CONTINUOUS;
        else if (mode == "range")
        {
            cacheContext = GEO_HAPI_TIME_CACHING_RANGE;

            // Range time caching settings
            if (context.ComposeValue(theTimeCacheStartToken, &contextVal) &&
                contextVal.IsHolding<float>())
            {
                float out = contextVal.UncheckedGet<float>();
                (*args)["timecachestart"] = TfStringify(out);
            }
            if (context.ComposeValue(theTimeCacheEndToken, &contextVal) &&
                contextVal.IsHolding<float>())
            {
                float out = contextVal.UncheckedGet<float>();
                (*args)["timecacheend"] = TfStringify(out);
            }
            if (context.ComposeValue(theTimeCacheIntervalToken, &contextVal) &&
                contextVal.IsHolding<float>())
            {
                float out = contextVal.UncheckedGet<float>();
                (*args)["timecacheinterval"] = TfStringify(out);
            }
        }

        (*args)["timecachemethod"] = mode;
    }

    // Session Holding Metadata
    if (context.ComposeValue(theHAPISessionToken, &contextVal)
        && contextVal.IsHolding<bool>())
    {
        const bool &holdSession = contextVal.UncheckedGet<bool>();
        (*args)["keepengineopen"] = holdSession ? "1": "0";
    }

    // This will be the same data read in
    // CanFieldChangeAffectFileFormatArguments()
    *dependencyContextData = cacheContext;
}

PXR_NAMESPACE_CLOSE_SCOPE
