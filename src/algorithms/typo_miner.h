#pragma once

#include "column_layout_typed_relation_data.h"
#include "common_options.h"
#include "csv_parser.h"
#include "idataset_stream.h"
#include "option_names.h"
#include "option_type.h"
#include "primitive.h"
#include "primitive_enum.h"
#include "pyro.h"
#include "types.h"

namespace algos {

class TypoMiner : public Primitive {
protected:
    void FitInternal(model::IDatasetStream &data_stream) override;
    unsigned long long ExecuteInternal() override;

private:
    std::unique_ptr<FDAlgorithm> precise_algo_;
    std::unique_ptr<FDAlgorithm> approx_algo_;
    std::vector<FD> approx_fds_;
    std::shared_ptr<ColumnLayoutRelationData> relation_;
    std::unique_ptr<model::ColumnLayoutTypedRelationData> typed_relation_;
    /* Config members */
    double radius_;      /* Maximal distance between two values to consider one of them a typo */
    double ratio_;       /* Maximal fraction of deviations per cluster to flag the cluster as
                          * containing typos */
    config::EqNullsType is_null_equal_null_;

    static config::OptionType<decltype(radius_)> RadiusOpt;
    static config::OptionType<decltype(ratio_)> RatioOpt;

    static bool FDLess(FD const& l, FD const& r);
    static auto MakeTuplesByIndicesComparator(std::map<int, unsigned> const& frequency_map);
    static double VerifyRadius(double radius) {
        if (!(radius == -1 || radius >= 0)) {
            throw std::invalid_argument("Radius should be greater or equal to zero or equal to -1");
        }
        return radius;
    }
    static double VerifyRatio(double ratio) {
        if (!(ratio >= 0 && ratio <= 1)) {
            throw std::invalid_argument("Ratio should be between 0 and 1");
        }
        return ratio;
    }

    std::unordered_map<int, unsigned> CreateFrequencies(
            util::PLI::Cluster const& cluster, std::vector<int> const& probing_table) const;
    std::map<int, unsigned> CreateFrequencyMap(Column const& cluster_col,
                                               util::PLI::Cluster const& cluster) const;
    unsigned GetMostFrequentValueIndex(Column const& cluster_col,
                                       util::PLI::Cluster const& cluster) const;
    bool ValuesAreClose(std::byte const* l, std::byte const* r,
                        model::Type const& type) const {
        assert(type.IsMetrizable());
        return static_cast<model::IMetrizableType const&>(type).Dist(l, r) < radius_;
    }
    explicit TypoMiner(std::unique_ptr<FDAlgorithm> precise_algo,
                       std::unique_ptr<FDAlgorithm> approx_algo);
    void RegisterOptions();
    void MakeExecuteOptsAvailable() final;
    void AddMoreNeededOptions(std::unordered_set<std::string_view> &previous_options) const final;
    bool HandleUnknownOption(std::string_view const& option_name,
                             boost::optional<boost::any> const& value) final;
    int TrySetOption(std::string_view const& option_name,
                     boost::optional<boost::any> const& value_precise,
                     boost::optional<boost::any> const& value_approx);

public:
    using TyposVec = std::vector<util::PLI::Cluster::value_type>;
    using ClusterTyposPair = std::pair<util::PLI::Cluster, TyposVec>;

    struct SquashedElement {
        int tuple_index;  /* Tuple index */
        unsigned amount;  /* The number of tuples equal to the given and
                           * following immediately after the given */
    };

    TypoMiner(Primitives precise, Primitives approx = Primitives::pyro);

    std::vector<util::PLI::Cluster> FindClustersWithTypos(FD const& typos_fd,
                                                          bool const sort_clusters = true);
    /* Returns squashed representation of a cluster with respect to given fd.
     * Check description of SquashedElement. Two tuples are considered equal if they are
     * equal in rhs attribute of given fd (they are automatically equal in lhs too if clusters
     * were retrieved from FindClustersWithTypos()).
     */
    std::vector<SquashedElement> SquashCluster(FD const& squash_on,
                                               util::PLI::Cluster const& cluster);
    /* Sorts given cluster in ascending order by uniqueness of the values in rhs of sort_on fd.
     * More strictly:
     * t1 tuple is considered less than t2 tuple iff t1[sort_on.rhs] is less frequent value
     * in rhs column than t2[sort_on.rhs].
     */
    void SortCluster(FD const& sort_on, util::PLI::Cluster& cluster) const;
    /* Finds lines in a cluster that has typos on typos_fd.GetRhs() column values. A value is said
     * to contain a typo if it differs from the most frequent value in the cluster by less
     * than radius_ and the fraction of such values (values_num / cluster_size) is less than ratio_.
     * NOTE: The cluster argument is asumed to be consistent with the rhs of typos_fd, i.e.
     * indices in the cluster represent value indices in the typos_fd.GetRhs() column. Otherwise
     * the behavior of this method is undefined.
     * Most likely you want to pass to this method as arguments pure approximate FD some_fd and
     * a cluster retrieved from the FindClustersWithTypos(some_fd).
     */
    TyposVec FindLinesWithTypos(FD const& typos_fd, util::PLI::Cluster const& cluster) const;
    TyposVec FindLinesWithTypos(FD const& typos_fd, util::PLI::Cluster const& cluster,
                                double new_radius, double new_ratio);
    std::vector<ClusterTyposPair> FindClustersAndLinesWithTypos(FD const& typos_fd,
                                                                bool const sort_clusters = true);

    /* Returns vector of approximate fds only (there are no precise fds) */
    std::vector<FD> const& GetApproxFDs() const noexcept {
        return approx_fds_;
    }
    double GetRadius() const noexcept {
        return radius_;
    }
    double GetRatio() const noexcept {
        return ratio_;
    }
    double SetRadius(double radius) {
        SetOption(std::string_view{RadiusOpt.GetName()}, boost::optional<boost::any>{radius});
        return radius_;
    }
    double SetRatio(double ratio) {
        SetOption(std::string_view{RadiusOpt.GetName()}, boost::optional<boost::any>{ratio});
        return ratio_;
    }
    ColumnLayoutRelationData const& GetRelationData() const noexcept {
        assert(relation_ != nullptr);
        return *relation_;
    }
    std::string GetApproxFDsAsJson() const {
        return FDAlgorithm::FDsToJson(approx_fds_);
    }
};

}  // namespace algos
