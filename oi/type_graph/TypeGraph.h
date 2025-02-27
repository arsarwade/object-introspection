/*
 * Copyright (c) Meta Platforms, Inc. and affiliates.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *    http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#pragma once

#include <functional>
#include <memory>
#include <vector>

#include "Types.h"

namespace type_graph {

/*
 * TypeGraph
 *
 * Holds the nodes and metadata which form a type graph.
 */
class TypeGraph {
 public:
  size_t size() const noexcept {
    return types_.size();
  }

  // TODO provide iterator instead of direct vector access?
  std::vector<std::reference_wrapper<Type>>& rootTypes() {
    return rootTypes_;
  }

  const std::vector<std::reference_wrapper<Type>>& rootTypes() const {
    return rootTypes_;
  }

  void addRoot(Type& type) {
    rootTypes_.push_back(type);
  }

  // Override of the generic makeType function that returns singleton Primitive
  // objects
  template <typename T>
  Primitive& makeType(Primitive::Kind kind);

  template <typename T, typename... Args>
  T& makeType(Args&&... args) {
    static_assert(!std::is_same<T, Primitive>::value,
                  "Primitive singleton override should be used");
    if constexpr (std::is_same<T, Class>::value ||
                  std::is_same<T, Container>::value ||
                  std::is_same<T, Array>::value ||
                  std::is_same<T, Typedef>::value ||
                  std::is_same<T, Pointer>::value) {
      // Node ID required
      auto type_unique_ptr =
          std::make_unique<T>(next_id_++, std::forward<Args>(args)...);
      auto type_raw_ptr = type_unique_ptr.get();
      types_.push_back(std::move(type_unique_ptr));
      return *type_raw_ptr;
    } else {
      // No Node ID
      auto type_unique_ptr = std::make_unique<T>(std::forward<Args>(args)...);
      auto type_raw_ptr = type_unique_ptr.get();
      types_.push_back(std::move(type_unique_ptr));
      return *type_raw_ptr;
    }
  }

  // TODO dodgy (use a getter instead to allow returning a const vector):
  std::vector<std::reference_wrapper<Type>> finalTypes;

 private:
  std::vector<std::reference_wrapper<Type>> rootTypes_;
  // Store all type objects in vectors for ownership. Order is not significant.
  std::vector<std::unique_ptr<Type>> types_;
  NodeId next_id_ = 0;
};

}  // namespace type_graph
