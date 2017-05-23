//
// IResearch search engine 
// 
// Copyright (c) 2016 by EMC Corporation, All Rights Reserved
// 
// This software contains the intellectual property of EMC Corporation or is licensed to
// EMC Corporation from third parties. Use of this software and the intellectual property
// contained therein is expressly limited to the terms and conditions of the License
// Agreement under which it is provided by or on behalf of EMC.
// 

#include "shared.hpp"
#include "score_doc_iterators.hpp"

NS_LOCAL

// placeholder for empty score
irs::score EMPTY_SCORE;
irs::attribute_ref<irs::score> EMPTY_SCORE_REF(&EMPTY_SCORE);

NS_END

NS_ROOT
  
score_doc_iterator_base::score_doc_iterator_base(const order::prepared& ord)
  : ord_(&ord) {
  auto* score = iresearch::score::apply(attrs_, ord);
  scr_ = score ? score : &EMPTY_SCORE_REF;
}

#if defined(_MSC_VER)
  #pragma warning( disable : 4706 )
#elif defined (__GNUC__)
  #pragma GCC diagnostic ignored "-Wparentheses"
#endif

basic_score_iterator::basic_score_iterator(
    const sub_reader& segment,
    const term_reader& field,
    const iresearch::attributes& stats, 
    doc_iterator::ptr&& it,
    const order::prepared& ord,
    cost::cost_t estimation) NOEXCEPT
  : score_doc_iterator_base(ord),
    it_(std::move(it)), 
    stats_(&stats) {
  assert( it_ );
  // set estimation value
  attrs_.add<cost>()->value(estimation);

  // set scorers
  scorers_ = ord_->prepare_scorers(
    segment, field, *stats_, it_->attributes()
  );
}

#if defined(_MSC_VER)
  #pragma warning( default : 4706 )
#elif defined (__GNUC__)
  #pragma GCC diagnostic pop
#endif

NS_END // ROOT
