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

#ifndef IRESEARCH_LIMITED_SAMPLE_COLLECTOR_H
#define IRESEARCH_LIMITED_SAMPLE_COLLECTOR_H

#include "shared.hpp"
#include "cost.hpp"
#include "sort.hpp"
#include "index/iterators.hpp"
#include "utils/string.hpp"
#include "utils/bitset.hpp"

NS_ROOT

struct sub_reader;
struct index_reader;

//////////////////////////////////////////////////////////////////////////////
/// @struct limited_sample_state
//////////////////////////////////////////////////////////////////////////////
struct limited_sample_state {
  limited_sample_state() = default;
  limited_sample_state(limited_sample_state&& rhs) noexcept
    : reader(rhs.reader),
      scored_states(std::move(rhs.scored_states)),
      unscored_docs(std::move(rhs.unscored_docs)) {
    rhs.reader = nullptr;
  }
  limited_sample_state& operator=(limited_sample_state&& rhs) noexcept {
    if (this != &rhs) {
      scored_states = std::move(rhs.scored_states);
      unscored_docs = std::move(rhs.unscored_docs);
      reader = rhs.reader;
      rhs.reader = nullptr;
    }
    return *this;
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @return total cost of execution
  //////////////////////////////////////////////////////////////////////////////
  cost::cost_t estimation() const noexcept {
    return scored_states_estimation + unscored_docs.count();
  }

  //////////////////////////////////////////////////////////////////////////////
  /// @brief reader using for iterate over the terms
  //////////////////////////////////////////////////////////////////////////////
  const term_reader* reader{};

  //////////////////////////////////////////////////////////////////////////////
  /// @brief scored term states (cookie + stat offset)
  //////////////////////////////////////////////////////////////////////////////
  std::vector<std::pair<seek_term_iterator::seek_cookie::ptr, size_t>> scored_states;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief matching doc_ids that may have been skipped
  ///        while collecting statistics and should not be
  ///        scored by the disjunction
  //////////////////////////////////////////////////////////////////////////////
  bitset unscored_docs;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief estimated cost of scored states
  //////////////////////////////////////////////////////////////////////////////
  cost::cost_t scored_states_estimation{};
}; // limited_sample_state

//////////////////////////////////////////////////////////////////////////////
/// @class limited_sample_collector
/// @brief object to collect and track a limited number of scorers,
///        terms with longer postings are treated as more important
//////////////////////////////////////////////////////////////////////////////
class limited_sample_collector : util::noncopyable {
 public:
  explicit limited_sample_collector(size_t scored_terms_limit);

  //////////////////////////////////////////////////////////////////////////////
  /// @brief prepare scorer for terms collecting
  /// @param segment segment reader for the current term
  /// @param state state containing this scored term
  /// @param terms segment term-iterator positioned at the current term
  //////////////////////////////////////////////////////////////////////////////
  void prepare(const sub_reader& segment,
               const seek_term_iterator& terms,
               limited_sample_state& scored_state) noexcept;

  //////////////////////////////////////////////////////////////////////////////
  /// @brief collect current term
  //////////////////////////////////////////////////////////////////////////////
  void collect();

  //////////////////////////////////////////////////////////////////////////////
  /// @brief finish collecting and evaluate stats
  //////////////////////////////////////////////////////////////////////////////
  void score(const index_reader& index,
             const order::prepared& order,
             std::vector<bstring>& stats);

 private:
  //////////////////////////////////////////////////////////////////////////////
  /// @brief a representation of state of the collector
  //////////////////////////////////////////////////////////////////////////////
  struct collector_state {
    const sub_reader* segment{};
    const seek_term_iterator* terms{};
    limited_sample_state* state{};
    const uint32_t* docs_count{};
    size_t state_id{};
  };

  //////////////////////////////////////////////////////////////////////////////
  /// @brief a representation of a term cookie with its associated range_state
  //////////////////////////////////////////////////////////////////////////////
  struct scored_term_state {
    scored_term_state(const collector_state& state)
      : cookie(state.terms->cookie()),
        state(state.state),
        segment(state.segment),
        term(state.terms->value()),
        docs_count(*state.docs_count),
        state_offset(state.state_id) {
      assert(this->cookie);
    }

    seek_term_iterator::cookie_ptr cookie; // term offset cache
    limited_sample_state* state; // state containing this scored term
    const irs::sub_reader* segment; // segment reader for the current term
    bstring term; // actual term value this state is for
    uint32_t docs_count;
    size_t state_offset;
  };

  collector_state state_;
  std::vector<scored_term_state> scored_states_;
  std::vector<size_t> scored_states_heap_; // use external heap as states are big
  size_t scored_terms_limit_;
}; // limited_sample_collector

NS_END

#endif // IRESEARCH_LIMITED_SAMPLE_COLLECTOR_H
