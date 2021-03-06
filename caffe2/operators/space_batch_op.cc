/**
 * Copyright (c) 2016-present, Facebook, Inc.
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

#include "caffe2/operators/space_batch_op.h"

namespace caffe2 {

REGISTER_CPU_OPERATOR(SpaceToBatch, SpaceToBatchOp<CPUContext>);
OPERATOR_SCHEMA(SpaceToBatch).NumInputs(1).NumOutputs(1).SetDoc(R"DOC(

SpaceToBatch for 4-D tensors of type T.

Zero-pads and then rearranges (permutes) blocks of spatial data into
batch. More specifically, this op outputs a copy of the input tensor
where values from the height and width dimensions are moved to the
batch dimension. After the zero-padding, both height and width of the
input must be divisible by the block size.

)DOC");

REGISTER_CPU_OPERATOR(BatchToSpace, BatchToSpaceOp<CPUContext>);
OPERATOR_SCHEMA(BatchToSpace).NumInputs(1).NumOutputs(1).SetDoc(R"DOC(

BatchToSpace for 4-D tensors of type T.

Rearranges (permutes) data from batch into blocks of spatial data,
followed by cropping. This is the reverse transformation of
SpaceToBatch. More specifically, this op outputs a copy of the input
tensor where values from the batch dimension are moved in spatial
blocks to the height and width dimensions, followed by cropping along
the height and width dimensions.

)DOC");

class GetSpaceToBatchGradient : public GradientMakerBase {
  using GradientMakerBase::GradientMakerBase;
  vector<OperatorDef> GetGradientDefs() override {
    return SingleGradientDef(
        "BatchToSpace", "", vector<string>{GO(0)}, vector<string>{GI(0)});
  }
};

class GetBatchToSpaceGradient : public GradientMakerBase {
  using GradientMakerBase::GradientMakerBase;
  vector<OperatorDef> GetGradientDefs() override {
    return SingleGradientDef(
        "SpaceToBatch", "", vector<string>{GO(0)}, vector<string>{GI(0)});
  }
};
REGISTER_GRADIENT(SpaceToBatch, GetSpaceToBatchGradient);
REGISTER_GRADIENT(BatchToSpace, GetBatchToSpaceGradient);
}
