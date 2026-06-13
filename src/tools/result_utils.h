#pragma once

#include <list>
#include <set>
#include <vector>

#include "types.h"
#include "bts.h"
#include "hc_utils.h"

std::vector<HCTree> buildClusterTree(const ArrayXXd& data, const Eigen::ArrayXi& labels);

std::vector<int> computeRepresentatives(const ArrayXXd& data, const std::vector<int>& labels,
                                         int nClusters, int nAtoms, MD::Metric mt);

std::vector<int> computeClusterSizes(const std::vector<int>& labels, int nClusters);

std::vector<double> computeClusterMSD(const ArrayXXd& data, const std::vector<int>& labels,
                                       int nClusters, int nAtoms);
