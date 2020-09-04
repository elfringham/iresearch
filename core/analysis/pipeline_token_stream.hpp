////////////////////////////////////////////////////////////////////////////////
/// DISCLAIMER
///
/// Copyright 2014-2020 ArangoDB GmbH, Cologne, Germany
/// Copyright 2004-2014 triAGENS GmbH, Cologne, Germany
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
/// @author Andrei Lobov
////////////////////////////////////////////////////////////////////////////////

#ifndef IRESEARCH_IQL_PIPELINE_TOKEN_STREAM_H
#define IRESEARCH_IQL_PIPELINE_TOKEN_STREAM_H

#include "shared.hpp"
#include "analyzers.hpp"
#include "token_stream.hpp"
#include "token_attributes.hpp"
#include "utils/frozen_attributes.hpp"

NS_ROOT
NS_BEGIN(analysis)

class pipeline_token_stream final
	: public frozen_attributes<3, analyzer>,  private util::noncopyable {
public:
	struct options_t {
		std::vector<irs::analysis::analyzer::ptr> pipeline;
	};

	static constexpr string_ref type_name() noexcept { return "pipeline"; }
	static void init(); // for triggering registration in a static build

	pipeline_token_stream(const options_t& options);
	virtual bool next() override;
	virtual bool reset(const string_ref& data) override;

private:
	struct sub_analyzer_t {
		explicit sub_analyzer_t(const irs::analysis::analyzer::ptr& a)
			: analyzer(a),
			term(irs::get<irs::term_attribute>(*analyzer)),
			inc(irs::get<irs::increment>(*analyzer)),
			offs(irs::get<irs::offset>(*analyzer)){}

		irs::analysis::analyzer::ptr analyzer;
		const term_attribute* term;
		const increment* inc;
		const offset* offs;
	};
	using pipeline_t = std::vector<sub_analyzer_t>;
	pipeline_t pipeline_;
	pipeline_t::iterator current_;
	pipeline_t::iterator top_;
	pipeline_t::iterator bottom_;
	offset offs_;
	increment inc_;
	// FIXME: find way to wire attribute directly from last pipeline member
	term_attribute term_;
	uint32_t upstream_inc_{ 0 };
};

NS_END
NS_END

#endif // IRESEARCH_IQL_PIPELINE_TOKEN_STREAM_H