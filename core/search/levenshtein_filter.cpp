////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2019 ArangoDB GmbH, Cologne, Germany
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

#include "levenshtein_filter.hpp"

#include "shared.hpp"
#include "index/index_reader.hpp"
#include "analysis/token_attributes.hpp"
#include "search/limited_sample_collector.hpp"
#include "search/multiterm_query.hpp"
#include "utils/automaton_utils.hpp"
#include "utils/fst_table_matcher.hpp"
#include "utils/levenshtein_utils.hpp"
#include "utils/levenshtein_default_pdp.hpp"
#include "utils/hash_utils.hpp"
#include "utils/noncopyable.hpp"
#include "utils/std.hpp"

NS_LOCAL

////////////////////////////////////////////////////////////////////////////////
/// @returns levenshtein similarity
////////////////////////////////////////////////////////////////////////////////
FORCE_INLINE float_t similarity(uint32_t distance, uint32_t size) noexcept {
  assert(size);
  return 1.f - float_t(distance) / size;
}

NS_END

NS_ROOT

DEFINE_FILTER_TYPE(by_edit_distance)
DEFINE_FACTORY_DEFAULT(by_edit_distance)

filter::prepared::ptr by_edit_distance::prepare(
    const index_reader& index,
    const order::prepared& order,
    boost_t boost,
    const attribute_view& ctx) const {
  if (0 == max_distance_) {
    return by_term::prepare(index, order, boost, ctx);
  }

  assert(provider_);
  const auto& d = (*provider_)(max_distance_, with_transpositions_);

  if (!d) {
    return prepared::empty();
  }

  boost *= this->boost();
  const string_ref field = this->field();

  const auto acceptor = make_levenshtein_automaton(d, term());
  automaton_table_matcher matcher(acceptor, fst::fsa::kRho);

  if (fst::kError == matcher.Properties(0)) {
    IR_FRMT_ERROR("Expected deterministic, epsilon-free acceptor, "
                  "got the following properties " IR_UINT64_T_SPECIFIER "",
                  acceptor.Properties(automaton_table_matcher::FST_PROPERTIES, false));

    return filter::prepared::empty();
  }

  const uint32_t utf8_term_size = static_cast<uint32_t>(utf8_utils::utf8_length(term()));
  limited_sample_collector<boost_t> collector(order.empty() ? 0 : scored_terms_limit()); // object for collecting order stats
  multiterm_query::states_t states(index.size());

  const byte_type NO_DISTANCE = 0;

  for (const auto& segment : index) {
    // get term dictionary for field
    const term_reader* reader = segment.field(field);

    if (!reader) {
      continue;
    }

    auto it = reader->iterator(matcher);
    auto& payload = it->attributes().get<irs::payload>();

    const byte_type* distance = &NO_DISTANCE;
    if (payload && !payload->value.empty()) {
      distance = &payload->value.front();
    }

    if (it->next()) {
      auto& state = states.insert(segment);
      state.reader = reader;

      collector.prepare(segment, *it, state);

      do {
        it->read(); // read term attributes

        const auto utf8_value_size = static_cast<uint32_t>(utf8_utils::utf8_length(it->value()));
        const auto key = ::similarity(*distance, std::min(utf8_value_size, utf8_term_size));

        collector.collect(key);
      } while (it->next());
    }
  }

  std::vector<bstring> stats;
  collector.score(index, order, stats);

  return memory::make_shared<multiterm_query>(
    std::move(states), std::move(stats),
    boost, sort::MergeType::MAX);
}

by_edit_distance::by_edit_distance() noexcept
  : by_prefix(by_edit_distance::type()),
    provider_(&default_pdp) {
}

by_edit_distance& by_edit_distance::provider(pdp_f provider) noexcept {
  if (!provider) {
    provider_ = &default_pdp;
  } else {
    provider_ = provider;
  }
  return *this;
}

size_t by_edit_distance::hash() const noexcept {
  size_t seed = 0;
  seed = hash_combine(0, by_prefix::hash());
  seed = hash_combine(seed, max_distance_);
  seed = hash_combine(seed, with_transpositions_);
  return seed;
}

bool by_edit_distance::equals(const filter& rhs) const noexcept {
  const auto& impl = static_cast<const by_edit_distance&>(rhs);

  return by_prefix::equals(rhs) &&
    max_distance_ == impl.max_distance_ &&
    with_transpositions_ == impl.with_transpositions_;
}

NS_END
