#include "primitive.h"

#include <cassert>

namespace algos {

void Primitive::AddProgress(double const val) noexcept {
    assert(val >= 0);
    std::scoped_lock lock(progress_mutex_);
    cur_phase_progress_ += val;
    assert(cur_phase_progress_ < 101);
}

void Primitive::SetProgress(double const val) noexcept {
    assert(0 <= val && val < 101);
    std::scoped_lock lock(progress_mutex_);
    cur_phase_progress_ = val;
}

std::pair<uint8_t, double> Primitive::GetProgress() const noexcept {
    std::scoped_lock lock(progress_mutex_);
    return std::make_pair(cur_phase_id_, cur_phase_progress_);
}

void Primitive::ToNextProgressPhase() noexcept {
    /* Current phase done, ensure that this is displayed in the progress bar */
    SetProgress(kTotalProgressPercent);

    std::scoped_lock lock(progress_mutex_);
    ++cur_phase_id_;
    assert(cur_phase_id_ < phase_names_.size());
    cur_phase_progress_ = 0;
}

void Primitive::Fit(model::IDatasetStream& data_stream) {
    FitInternal(data_stream);
}

void Primitive::SetOption(const std::string& option_name) {
    auto it = possible_options_.find(option_name);
    if (it == possible_options_.end()
        || available_options.find(option_name) == available_options.end())
        throw std::invalid_argument("Invalid option");
    it->second->SetDefault();
}

void Primitive::SetOption(const std::string& option_name, const boost::any& value) {
    auto it = possible_options_.find(option_name);
    if (it == possible_options_.end()
        || available_options.find(it->first) == available_options.end())
        throw std::invalid_argument("Invalid option");
    it->second->SetAny(value);
}

void Primitive::UnsetOption(const std::string& option_name) noexcept {
    UnsetOption(std::string_view(option_name));
}

void Primitive::UnsetOption(std::string_view option_name) noexcept {
    auto it = possible_options_.find(option_name);
    if (it == possible_options_.end()
        || available_options.find(option_name) == available_options.end())
        return;
    it->second->Unset();
    ExcludeOptions(it->first);
}

std::unordered_set<std::string_view> Primitive::GetNeededOptions() const {
    std::unordered_set<std::string_view> needed{};
    for (std::string_view const& name : available_options) {
        auto it = possible_options_.find(name);
        assert(it != possible_options_.end());
        if (!it->second->IsSet()) {
            needed.insert(it->first);
        }
    }
    return needed;
}

void Primitive::ClearOptions() noexcept {
    available_options.clear();
    opt_parents_.clear();
}

void Primitive::MakeOptionsAvailable(const std::vector<std::string_view>& option_names) {
    for (std::string_view name : option_names) {
        auto it = possible_options_.find(name);
        assert(it != possible_options_.end());
        available_options.insert(it->first);
    }
}

void Primitive::MakeOptionsAvailable(std::string_view parent_name,
                                     std::vector<std::string_view> option_names) {
    MakeOptionsAvailable(option_names);
    auto it = possible_options_.find(parent_name);
    assert(it != possible_options_.end());
    for (const auto& option_name : option_names) {
        opt_parents_[it->first].emplace_back(option_name);
    }
}

void Primitive::ExcludeOptions(std::string_view parent_option) {
    auto it = opt_parents_.find(parent_option);
    if (it == opt_parents_.end()) return;

    for (auto const& option_name : it->second) {
        auto possible_opt_it = possible_options_.find(option_name);
        assert(possible_opt_it != possible_options_.end());
        available_options.erase(possible_opt_it->first);
        UnsetOption(possible_opt_it->first);
    }
    opt_parents_.erase(parent_option);
}

std::function<void(const std::string_view &, const std::vector<std::string_view>&)>
        Primitive::GetOptAvailFunc() {
    return [this](auto parent_opt, auto children) { MakeOptionsAvailable(parent_opt, children); };
}

}  // namespace algos
