////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2020 ArangoDB GmbH, Cologne, Germany
///
/// Licensed under the Apache License, Version 2.0 (the "License");
/// you may not use this file except in compliance with the License.
/// You may obtain a copy of the License at
///
///     http://www.apache.org/licenses/LICENSE-2.0
///
/// Unless required by applicable law or agreed to in writing, software
/// distributed under the License is distributed on an "AS IS" BASIS,
/// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
/// See the License for the specific language governing permissions and
/// limitations under the License.
///
/// Copyright holder is ArangoDB GmbH, Cologne, Germany
///
/// @author Andrey Abramov
////////////////////////////////////////////////////////////////////////////////

#include "boost_sort.hpp"

NS_LOCAL

using namespace irs;

struct boost_score_ctx : score_ctx {
  explicit boost_score_ctx(boost_t boost)
    : boost(boost) {
  }

  boost_t boost;
};

struct prepared : prepared_sort_basic<boost_t> {
  const irs::flags& features() const override {
    return irs::flags::empty_instance();
  }

  std::pair<irs::score_ctx_ptr, irs::score_f> prepare_scorer(
      const irs::sub_reader&,
      const irs::term_reader&,
      const irs::byte_type*,
      const irs::attribute_view&,
      irs::boost_t boost) const override {
    return {
      irs::score_ctx_ptr(new boost_score_ctx(boost)), // FIXME can avoid allocation
      [](const irs::score_ctx* ctx, irs::byte_type* score) noexcept {
        auto& state = *reinterpret_cast<const boost_score_ctx*>(ctx);
        sort::score_cast<boost_t>(score) = state.boost;
      }
    };
  }
};

NS_END

NS_ROOT

DEFINE_SORT_TYPE(irs::boost_sort);
DEFINE_FACTORY_DEFAULT(irs::boost_sort);

boost_sort::boost_sort() noexcept
  : sort(boost_sort::type()) {
}

sort::prepared::ptr boost_sort::prepare() const {
  // FIXME can avoid allocation
  return memory::make_unique<::prepared>();
}

NS_END