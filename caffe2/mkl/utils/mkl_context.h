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

#ifndef CAFFE2_UTILS_MKL_CONTEXT_H_
#define CAFFE2_UTILS_MKL_CONTEXT_H_

#include <cstdlib>
#include <ctime>
#include <random>

#include "caffe2/core/context.h"

namespace caffe2 {

/**
 * The MKL Context, which is largely the same as the CPUContext. We instantiate
 * this mainly in order to have a first-class MKL device.
 *
 * Note that although New() and Delete() are implemented, we expect MKLContext
 * operators to mainly perform input and output via MKLMemory. As a result,
 * most likely MKLContext::New and ::Delete won't be used as often.
 */
class MKLContext final {
 public:
  MKLContext() : random_seed_(math::randomNumberSeed()) {}
  explicit MKLContext(const DeviceOption& option)
      : random_seed_(
            option.has_random_seed() ? option.random_seed()
                                     : math::randomNumberSeed()) {
    CAFFE_ENFORCE_EQ(option.device_type(), MKLDNN);
  }

  ~MKLContext() {}

  inline void SwitchToDevice(int /*stream_id*/ = 0) {}
  inline void WaitEvent(const Event& ev) {
    ev.Wait(MKLDNN, this);
  }
  inline void Record(Event* ev) const {
    CAFFE_ENFORCE(ev, "Event must not be null.");
    ev->Record(MKLDNN, this);
  }
  inline void FinishDeviceComputation() {}

  inline std::mt19937& RandGenerator() {
    if (!random_generator_.get()) {
      random_generator_.reset(new std::mt19937(random_seed_));
    }
    return *random_generator_.get();
  }

  inline static std::pair<void*, MemoryDeleter> New(size_t nbytes) {
    return GetCPUAllocator()->New(nbytes);
  }

  // Two copy functions that deals with cross-device copies.
  template <class SrcContext, class DstContext>
  inline void CopyBytes(size_t nbytes, const void* src, void* dst);

  template <typename T, class SrcContext, class DstContext>
  inline void Copy(size_t n, const T* src, T* dst) {
    if (std::is_fundamental<T>::value) {
      CopyBytes<SrcContext, DstContext>(
          n * sizeof(T),
          static_cast<const void*>(src),
          static_cast<void*>(dst));
    } else {
      for (int i = 0; i < n; ++i) {
        dst[i] = src[i];
      }
    }
  }

  template <class SrcContext, class DstContext>
  inline void
  CopyItems(const TypeMeta& meta, size_t n, const void* src, void* dst) {
    if (meta.copy()) {
      meta.copy()(src, dst, n);
    } else {
      CopyBytes<SrcContext, DstContext>(n * meta.itemsize(), src, dst);
    }
  }

 protected:
  // TODO(jiayq): instead of hard-coding a generator, make it more flexible.
  int random_seed_{1701};
  std::unique_ptr<std::mt19937> random_generator_;
};

template <>
inline void MKLContext::CopyBytes<MKLContext, MKLContext>(
    size_t nbytes,
    const void* src,
    void* dst) {
  memcpy(dst, src, nbytes);
}

template <>
inline void MKLContext::CopyBytes<CPUContext, MKLContext>(
    size_t nbytes,
    const void* src,
    void* dst) {
  memcpy(dst, src, nbytes);
}

template <>
inline void MKLContext::CopyBytes<MKLContext, CPUContext>(
    size_t nbytes,
    const void* src,
    void* dst) {
  memcpy(dst, src, nbytes);
}
} // namespace caffe2

#endif // CAFFE2_UTILS_MKL_CONTEXT_H_
