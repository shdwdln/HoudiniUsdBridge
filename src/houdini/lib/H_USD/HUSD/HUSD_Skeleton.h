/*
 * Copyright 2019 Side Effects Software Inc.
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

#ifndef __HUSD_Skeleton_h__
#define __HUSD_Skeleton_h__

#include "HUSD_API.h"

#include <GU/GU_AgentClip.h>
#include <GU/GU_AgentRig.h>
#include <SYS/SYS_Types.h>

class GU_AgentClip;
class GU_AgentLayer;
class GU_AgentShapeLib;
class GU_Detail;
class HUSD_AutoReadLock;
class UT_StringHolder;
class UT_StringRef;

/// Returns the path to a SkelRoot prim in the stage, or the empty string.
HUSD_API UT_StringHolder
HUSDdefaultSkelRootPath(HUSD_AutoReadLock &readlock);

/// Imports all skinnable primitives underneath the provided SkelRoot prim.
HUSD_API bool
HUSDimportSkinnedGeometry(GU_Detail &gdp, const HUSD_AutoReadLock &readlock,
                          const UT_StringRef &skelrootpath,
                          const UT_StringHolder &shapeattrib);

enum class HUSD_SkeletonPoseType
{
    Animation,
    BindPose,
    RestPose
};

/// Imports all Skeleton primitives underneath the provided SkelRoot prim.
/// A point is created for each joint, and joints are connected to their
/// parents by polyline primitives.
/// Use HUSDimportSkeletonPose() to set the skeleton's transforms. The pose
/// type is only used in this method to initialize attributes that aren't
/// time-varying.
HUSD_API bool
HUSDimportSkeleton(GU_Detail &gdp, const HUSD_AutoReadLock &readlock,
                   const UT_StringRef &skelrootpath,
                   HUSD_SkeletonPoseType pose_type);

/// Updates the pose for the skeleton geometry created by HUSDimportSkeleton().
HUSD_API bool
HUSDimportSkeletonPose(GU_Detail &gdp, const HUSD_AutoReadLock &readlock,
                       const UT_StringRef &skelrootpath,
                       HUSD_SkeletonPoseType pose_type, fpreal time);

/// Builds an agent rig from the SkelRoot's first Skeleton prim.
HUSD_API GU_AgentRigPtr
HUSDimportAgentRig(const HUSD_AutoReadLock &readlock,
                   const UT_StringRef &skelrootpath,
                   const UT_StringHolder &rig_name,
                   bool create_locomotion_joint);

/// Imports all skinnable primitives underneath the provided SkelRoot prim
/// (which are associated with the skeleton used for HUSDimportRig()), and adds
/// the shape bindings to the provided layer.
HUSD_API bool
HUSDimportAgentShapes(GU_AgentShapeLib &shapelib,
                      GU_AgentLayer &layer,
                      const HUSD_AutoReadLock &readlock,
                      const UT_StringRef &skelrootpath,
                      fpreal layer_bounds_scale);

/// Initialize an agent clip from the animation associated with the skeleton
/// used for HUSDimportAgentRig().
/// The clip is assigned a name from the skeleton primitive's name.
HUSD_API GU_AgentClipPtr
HUSDimportAgentClip(const GU_AgentRigConstPtr &rig,
                    HUSD_AutoReadLock &readlock,
                    const UT_StringRef &skelrootpath);

/// Import clips from the provided primitive pattern, which can match against
/// either SkelRoot or Skeleton prims.
/// The clips are assigned names from the USD primitives' names.
HUSD_API UT_Array<GU_AgentClipPtr>
HUSDimportAgentClips(const GU_AgentRigConstPtr &rig,
                     HUSD_AutoReadLock &readlock,
                     const UT_StringRef &prim_pattern);

#endif
