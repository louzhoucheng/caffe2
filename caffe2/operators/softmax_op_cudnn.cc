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

#include "caffe2/core/common_cudnn.h"
#include "caffe2/core/context_gpu.h"
#include "caffe2/core/types.h"
#include "caffe2/operators/softmax_op.h"

namespace caffe2 {

namespace {
constexpr int NUM_DESCRIPTORS = 2;
constexpr int GRADIENT_NUM_DESCRIPTORS = 3;
constexpr int BOTTOM_DESC_ID = 0;
constexpr int TOP_DESC_ID = 1;
constexpr int TOP_GRADIENT_DESC_ID = 2;
}  // namespace

class CuDNNSoftmaxOp final : public Operator<CUDAContext> {
 public:
  explicit CuDNNSoftmaxOp(const OperatorDef& def, Workspace* ws)
      : Operator<CUDAContext>(def, ws),
        cudnn_wrapper_(&context_),
        axis_(OperatorBase::GetSingleArgument<int>("axis", 1)) {
    CUDNN_ENFORCE(cudnnCreateTensorDescriptor(&desc_));
  }

  ~CuDNNSoftmaxOp() {
    CUDNN_ENFORCE(cudnnDestroyTensorDescriptor(desc_));
  }

  template <typename T>
  bool DoRunWithType() {
    auto& X = Input(0);
    auto* Y = Output(0);
    const auto canonical_axis = X.canonical_axis_index(axis_);
    const int N = X.size_to_dim(canonical_axis);
    const int D = X.size_from_dim(canonical_axis);

    Y->ResizeLike(X);
    if (dims_ != X.dims()) {
      CUDNN_ENFORCE(cudnnSetTensor4dDescriptor(
          desc_,
          GetCudnnTensorFormat(StorageOrder::NCHW),
          cudnnTypeWrapper<T>::type,
          N,
          D,
          1,
          1));
      dims_ = X.dims();
    }
    CUDNN_ENFORCE(cudnnSoftmaxForward(
        cudnn_wrapper_.inline_cudnn_handle(),
        CUDNN_SOFTMAX_ACCURATE,
        CUDNN_SOFTMAX_MODE_INSTANCE,
        cudnnTypeWrapper<T>::kOne(),
        desc_,
        X.template data<T>(),
        cudnnTypeWrapper<T>::kZero(),
        desc_,
        Y->template mutable_data<T>()));
    return true;
  }

  bool RunOnDevice() override {
    return DispatchHelper<TensorTypes<float, float16>>::call(this, Input(0));
  }

 protected:
  CuDNNWrapper cudnn_wrapper_;
  int axis_;
  cudnnTensorDescriptor_t desc_;
  vector<TIndex> dims_;
};


class CuDNNSoftmaxGradientOp final : public Operator<CUDAContext> {
 public:
  explicit CuDNNSoftmaxGradientOp(const OperatorDef& def, Workspace* ws)
      : Operator<CUDAContext>(def, ws),
        cudnn_wrapper_(&context_),
        axis_(OperatorBase::GetSingleArgument<int>("axis", 1)) {
    CUDNN_ENFORCE(cudnnCreateTensorDescriptor(&desc_));
  }

  ~CuDNNSoftmaxGradientOp() {
    CUDNN_ENFORCE(cudnnDestroyTensorDescriptor(desc_));
  }

  template <typename T>
  bool DoRunWithType() {
    auto& Y = Input(0);
    auto& dY = Input(1);
    auto* dX = Output(0);
    const auto canonical_axis = Y.canonical_axis_index(axis_);
    const int N = Y.size_to_dim(canonical_axis);
    const int D = Y.size_from_dim(canonical_axis);

    CHECK_EQ(Y.dims(), dY.dims());
    dX->ResizeLike(Y);
    if (dims_ != Y.dims()) {
      CUDNN_ENFORCE(cudnnSetTensor4dDescriptor(
          desc_,
          GetCudnnTensorFormat(StorageOrder::NCHW),
          cudnnTypeWrapper<T>::type,
          N,
          D,
          1,
          1));
      dims_ = Y.dims();
    }
    CUDNN_ENFORCE(cudnnSoftmaxBackward(
        cudnn_wrapper_.inline_cudnn_handle(),
        CUDNN_SOFTMAX_ACCURATE,
        CUDNN_SOFTMAX_MODE_INSTANCE,
        cudnnTypeWrapper<T>::kOne(),
        desc_,
        Y.template data<T>(),
        desc_,
        dY.template data<T>(),
        cudnnTypeWrapper<T>::kZero(),
        desc_,
        dX->template mutable_data<T>()));
    return true;
  }

  bool RunOnDevice() override {
    return DispatchHelper<TensorTypes<float, float16>>::call(this, Input(0));
  }

 protected:
  CuDNNWrapper cudnn_wrapper_;
  int axis_;
  cudnnTensorDescriptor_t desc_;
  vector<TIndex> dims_;
};

namespace {
REGISTER_CUDNN_OPERATOR(Softmax, CuDNNSoftmaxOp);
REGISTER_CUDNN_OPERATOR(SoftmaxGradient, CuDNNSoftmaxGradientOp);
}  // namespace
}  // namespace caffe2
