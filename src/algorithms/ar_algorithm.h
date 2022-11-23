#pragma once

#include <list>
#include <set>
#include <stack>
#include <vector>

#include <boost/any.hpp>

#include "ar.h"
#include "ar_algorithm_enums.h"
#include "options/option_type.h"
#include "primitive.h"
#include "transactional_data.h"

namespace algos {

class ARAlgorithm : public algos::Primitive {
private:
    double minconf_;
    InputFormat input_format_ = InputFormat::_values()[0];
    unsigned tid_column_index_;
    unsigned item_column_index_;
    bool first_column_tid_;
    std::list<model::ArIDs> ar_collection_;
    std::shared_ptr<model::InputFormat> format_ptr_;

    static const config::OptionType<InputFormat> InputFormatOpt;
    static const config::OptionType<unsigned int> TidColumnIndexOpt;
    static const config::OptionType<unsigned int> ItemColumnIndexOpt;
    static const config::OptionType<bool> FirstColumnTidOpt;
    static const config::OptionType<double> MinSupportOpt;
    static const config::OptionType<double> MinConfidenceOpt;

    struct RuleNode {
        model::ArIDs rule;
        std::list<RuleNode> children;
        RuleNode() = default;

        /* Temporary fix. Now we allocate generated AR twice -- in ar_collection_
         * and also in a rule node by copying it.
         * */
        explicit RuleNode(model::ArIDs const& rule)
            : rule(rule) {}
    };

    RuleNode root_;

    bool GenerateRuleLevel(std::vector<unsigned> const& frequent_itemset,
                           double support, unsigned level_number);
    bool MergeRules(std::vector<unsigned> const& frequent_itemset, double support, RuleNode* node);
    static void UpdatePath(std::stack<RuleNode*>& path, std::list<RuleNode>& vertices);
    void RegisterOptions();

protected:
    std::unique_ptr<model::TransactionalData> transactional_data_;
    double minsup_;

    void GenerateRulesFrom(std::vector<unsigned> const& frequent_itemset, double support);

    virtual double GetSupport(std::vector<unsigned> const& frequent_itemset) const = 0;
    virtual unsigned long long GenerateAllRules() = 0;
    virtual unsigned long long FindFrequent() = 0;
    void FitInternal(model::IDatasetStream &data_stream) override;
    void MakeExecuteOptsAvailable() override;
    unsigned long long ExecuteInternal() override;

public:
    explicit ARAlgorithm(std::vector<std::string_view> phase_names);

    std::list<model::ArIDs> const& GetArIDsList() const noexcept { return ar_collection_; };
    std::vector<std::string> const& GetItemNamesVector() const noexcept {
        return transactional_data_->GetItemUniverse();
    }

    virtual std::list<std::set<std::string>> GetFrequentList() const = 0;  // for debugging and testing
    std::list<model::ARStrings> GetArStringsList() const;

    virtual ~ARAlgorithm() = default;
};

} // namespace algos
