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

#ifndef __GEO_HDA_FILEDATA_H__
#define __GEO_HDA_FILEDATA_H__

#include "GEO_SceneDescriptionData.h"
#include "GEO_FilePrim.h"
#include "GEO_FilePrimUtils.h"
#include "GEO_HAPIReader.h"
#include <HAPI/HAPI.h>
#include <UT/UT_Array.h>
#include <UT/UT_UniquePtr.h>
#include <deque>

PXR_NAMESPACE_OPEN_SCOPE

TF_DECLARE_WEAK_AND_REF_PTRS(GEO_HDAFileData);

/// \class GEO_HDAFileData
///
class GEO_HDAFileData : public GEO_SceneDescriptionData
{
public:
    static GEO_HDAFileDataRefPtr New(
        const SdfFileFormat::FileFormatArguments &args);

    bool Open(const std::string &filePath) override;

protected:
    GEO_HDAFileData();
    ~GEO_HDAFileData() override;

private:
    // Configures options based on file format arguments
    void configureOptions(GEO_ImportOptions &options,
                          GEO_HAPIMetadataInfo &timeInfo);

    GEO_FilePrim *myLayerInfoPrim;
    SdfFileFormat::FileFormatArguments myCookArgs;
    fpreal mySampleTime;
    bool mySaveSampleFrame;

    friend class GEO_FilePrim;
};

PXR_NAMESPACE_CLOSE_SCOPE

#endif // __GEO_HDA_FILEDATA_H__
