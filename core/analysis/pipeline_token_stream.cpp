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

#include "pipeline_token_stream.hpp"

#include <rapidjson/rapidjson/document.h> // for rapidjson::Document
#include <rapidjson/rapidjson/writer.h> // for rapidjson::Writer
#include <rapidjson/rapidjson/stringbuffer.h> // for rapidjson::StringBuffer

NS_LOCAL

constexpr irs::string_ref PIPELINE_PARAM_NAME   = "pipeline";
constexpr irs::string_ref TYPE_PARAM_NAME       = "type";
constexpr irs::string_ref PROPERTIES_PARAM_NAME = "properties";

bool parse_json_config(const irs::string_ref& args,
	irs::analysis::pipeline_token_stream::options_t& options) {
	rapidjson::Document json;
	if (json.Parse(args.c_str(), args.size()).HasParseError()) {
		IR_FRMT_ERROR(
			"Invalid jSON arguments passed while constructing pipeline_token_stream, "
			"arguments: %s",
			args.c_str());

		return false;
	}

	if (rapidjson::kObjectType != json.GetType()) {
		IR_FRMT_ERROR(
			"Not a jSON object passed while constructing pipeline_token_stream, "
			"arguments: %s",
			args.c_str());

		return false;
	}

	if (json.HasMember(PIPELINE_PARAM_NAME.c_str())) {
		auto& pipeline = json[PIPELINE_PARAM_NAME.c_str()];
		if (pipeline.IsArray()) {
			for (auto pipe = pipeline.Begin(), end = pipeline.End(); pipe != end; ++pipe) {
				if (pipe->IsObject()) {
					irs::string_ref type;
					if (pipe->HasMember(TYPE_PARAM_NAME.c_str())) {
						auto& type_atr = (*pipe)[TYPE_PARAM_NAME.c_str()];
						if (type_atr.IsString()) {
							type = type_atr.GetString();
						}	else {
							IR_FRMT_ERROR(
								"Failed to read '%s' attribute of  '%s' member as string while constructing "
								"pipeline_token_stream from jSON arguments: %s",
								TYPE_PARAM_NAME.c_str(), PIPELINE_PARAM_NAME.c_str(), args.c_str());
							return false;
						}
					} else {
						IR_FRMT_ERROR(
							"Failed to get '%s' attribute of  '%s' member while constructing "
							"pipeline_token_stream from jSON arguments: %s",
							TYPE_PARAM_NAME.c_str(), PIPELINE_PARAM_NAME.c_str(), args.c_str());
						return false;
					}
					if (pipe->HasMember(PROPERTIES_PARAM_NAME.c_str())) {
						auto& properties_atr = (*pipe)[PROPERTIES_PARAM_NAME.c_str()];
						rapidjson::StringBuffer properties_buffer;
						rapidjson::Writer< rapidjson::StringBuffer> writer(properties_buffer);
						properties_atr.Accept(writer);
						auto analyzer = irs::analysis::analyzers::get(
							                type.c_str(), 
							                irs::type<irs::text_format::json>::get(),
							                properties_buffer.GetString());
						if (analyzer) {
							options.pipeline.push_back(std::move(analyzer));
						} else {
							IR_FRMT_ERROR(
								"Failed to create pipeline member of type '%s' with properties '%s' while constructing "
								"pipeline_token_stream from jSON arguments: %s",
								type.c_str(), properties_buffer.GetString(), args.c_str());
							return false;
						}
					} else {
						IR_FRMT_ERROR(
							"Failed to get '%s' attribute of  '%s' member while constructing "
							"pipeline_token_stream from jSON arguments: %s",
							PROPERTIES_PARAM_NAME.c_str(), PIPELINE_PARAM_NAME.c_str(), args.c_str());
						return false;
					}
				}	else {
					IR_FRMT_ERROR(
						"Failed to read '%s' member as object while constructing "
						"pipeline_token_stream from jSON arguments: %s",
						PIPELINE_PARAM_NAME.c_str(), args.c_str());
					return false;
				}
			}
		} else {
			IR_FRMT_ERROR(
				"Failed to read '%s' attribute as array while constructing "
				"pipeline_token_stream from jSON arguments: %s",
				PIPELINE_PARAM_NAME.c_str(), args.c_str());
			return false;
		}
	} else {
		IR_FRMT_ERROR(
			"Not found parameter '%s' while constructing pipeline_token_stream, "
			"arguments: %s",
			PIPELINE_PARAM_NAME.c_str(),
			args.c_str());
		return false;
	}
	return true;
}



bool normalize_json_config(const irs::string_ref& args, std::string& definition) {
	irs::analysis::pipeline_token_stream::options_t options;
	if (parse_json_config(args, options)) {
		return true;
	} else {
		return false;
	}
}

////////////////////////////////////////////////////////////////////////////////
/// @brief args is a jSON encoded object with the following attributes:
/// pipeline: Array of objects containing analyzers definition inside pipeline.
/// Each definition is an object with the following attributes:
/// type: analyzer type name (one of registered analyzers type)
/// properties: object with properties for corresponding analyzer
////////////////////////////////////////////////////////////////////////////////
irs::analysis::analyzer::ptr make_json(const irs::string_ref& args) {
	irs::analysis::pipeline_token_stream::options_t options;
	if (parse_json_config(args, options)) {
		return std::make_shared<irs::analysis::pipeline_token_stream>(std::move(options));
	} else {
		return nullptr;
	}
}


REGISTER_ANALYZER_JSON(irs::analysis::pipeline_token_stream, make_json, 
	normalize_json_config);

NS_END

NS_ROOT
NS_BEGIN(analysis)

pipeline_token_stream::pipeline_token_stream(const pipeline_token_stream::options_t& options) 
	: attributes{ {
		{ irs::type<increment>::id(), &inc_ },
		{ irs::type<offset>::id(), &offs_ },
		{ irs::type<term_attribute>::id(), &term_ }}, // TODO: use get_mutable
		irs::type<pipeline_token_stream>::get()} {
	pipeline_.reserve(options.pipeline.size());
	for (const auto& p : options.pipeline) {
		pipeline_.emplace_back(p);
	}
	top_ = pipeline_.begin();
	bottom_ = --pipeline_.end();
}


/// Pipeline position change rules:
///  - If none of pipeline members changes position - whole pipeline holds position
///  - If one or more pipeline member moves - pipeline moves( change from max->0 is not move, see rules below!).
///    All position gaps are accumulated (e.g. if one member has inc 2(1 pos gap) and other has inc 3(2 pos gap)  - pipeline has inc 4 (1+2 pos gap))
///  - All position changes caused by parent analyzer move next (e.g. transfer from max->0 by first next after reset) are collapsed.
///    e.g if parent moves after next, all its children are resetted to new token and also moves step froward - this whole operation
///    is just one step for pipeline (if any of children has moved more than 1 step - gaps are preserved!)
///  - If parent after next is NOT moved (inc == 0) than pipeline makes one step forward if at least one child changes
///    position from any positive value back to 0 due to reset (additional gaps also preserved!) as this is
///    not change max->0 and position is indeed changed.
inline bool pipeline_token_stream::next() {
	uint32_t upstream_inc = 0;
	while (!current_->next()) {
		if (current_ == top_) { // reached pipeline top and next has failed - we are done
			return false;
		}
		--current_;
	}
	upstream_inc += current_->inc->value;

	const auto top_holds_position = current_->inc->value == 0;

	// go down to lowest pipe to get actual tokens
	bool step_for_rollback{ false };
	while (current_ != bottom_) {
		const auto prev_term = current_->term->value;
		++current_;
		// check do we need to do step forward due to rollback to 0.
		step_for_rollback |= top_holds_position && current_->last_pos !=0 &&
			                   current_->last_pos != irs::integer_traits<uint32_t>::const_max;
		if (!current_->reset(irs::ref_cast<char>(prev_term))) {
			return false;
		}
		while (!current_->next()) { // empty one found. Move upstream.
			if (current_ == top_) { // reached pipeline top and next has failed - we are done
				return false;
			}
			--current_;
		}
		upstream_inc += current_->inc->value;
		assert(current_->inc->value > 0); // first increment after reset should be positive to give 0 or next pos!
		assert(upstream_inc > 0);
		upstream_inc--; // compensate placing sub_analyzer from max to 0 due to reset
										// as this step actually does not move whole pipeline
										// sub analyzer just stays same pos as it`s parent (rollback step will be done below if necessary!)
	}
	if (step_for_rollback) {
		upstream_inc++;
	}
	term_.value = current_->term->value;

	// FIXME: get rid of full recalc. Use incremental approach
	uint32_t start{ 0 };
	uint32_t upstream_end{ static_cast<uint32_t>(pipeline_.front().data_size) };
	for (const auto& p : pipeline_) {
		start += p.offs->start;
		if (p.offs->end != p.data_size && p.analyzer != bottom_->analyzer) {
			// this analyzer is not last and eaten not all its data.
			// so it will mark new pipeline offset end.
			upstream_end = start +  (p.offs->end - p.offs->start);
		}
	}
	inc_.value = upstream_inc;
	offs_.start = start;
	offs_.end = current_->offs->end == current_->data_size ? 
		          upstream_end : // all data eaten - actual end is defined by upstream
              (offs_.start + (current_->offs->end - current_->offs->start));
	return true;
}

inline bool pipeline_token_stream::reset(const string_ref& data) {
	current_ = top_;
	return pipeline_.front().reset(data);
}

/*static*/ void pipeline_token_stream::init() {
	REGISTER_ANALYZER_JSON(pipeline_token_stream, make_json,
		normalize_json_config);  // match registration above
}

NS_END
NS_END