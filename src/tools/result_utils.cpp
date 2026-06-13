#include "result_utils.h"
#include "scores.h"

std::vector<HCTree> buildClusterTree(const ArrayXXd& data, const Eigen::ArrayXi& labels) {
    std::set<int> uniqueLabels;
    for (int i = 0; i < labels.size(); i++) {
        uniqueLabels.insert(labels(i));
    }

    std::vector<HCTree> clusters;
    int clusterIdx = 0;
    for (int label : uniqueLabels) {
        std::list<int> indices = {clusterIdx};
        Vec cSumi = Vec::Zero(data.cols());
        Vec sqSumi = Vec::Zero(data.cols());
        int ni = 0;
        for (int j = 0; j < labels.size(); j++) {
            if (labels(j) == label) {
                ni++;
                cSumi += data.row(j);
                sqSumi += data.row(j).square();
            }
        }
        HCTree tree = HCTree();
        tree.insertRoot(indices, cSumi, sqSumi, ni, clusterIdx);
        clusters.push_back(tree);
        clusterIdx++;
    }

    return clusters;
}

std::vector<int> computeRepresentatives(const ArrayXXd& data, const std::vector<int>& labels,
                                         int nClusters, int nAtoms, MD::Metric mt) {
    std::vector<int> reps;
    for (int c = 0; c < nClusters; ++c) {
        std::vector<int> memberIndices;
        for (size_t i = 0; i < labels.size(); ++i) {
            if (labels[i] == c) memberIndices.push_back(i);
        }
        if (memberIndices.empty()) {
            reps.push_back(-1);
            continue;
        }
        ArrayXXd subData(memberIndices.size(), data.cols());
        for (size_t i = 0; i < memberIndices.size(); ++i) {
            subData.row(i) = data.row(memberIndices[i]);
        }
        Index medoidLocal = calculateMedoid(subData, nAtoms, mt);
        reps.push_back(memberIndices[medoidLocal]);
    }
    return reps;
}

std::vector<int> computeClusterSizes(const std::vector<int>& labels, int nClusters) {
    std::vector<int> sizes(nClusters, 0);
    for (int l : labels) {
        if (l >= 0 && l < nClusters) sizes[l]++;
    }
    return sizes;
}

std::vector<double> computeClusterMSD(const ArrayXXd& data, const std::vector<int>& labels,
                                       int nClusters, int nAtoms) {
    std::vector<double> msds;
    for (int c = 0; c < nClusters; ++c) {
        std::vector<int> members;
        for (size_t i = 0; i < labels.size(); ++i) {
            if (labels[i] == c) members.push_back(i);
        }
        if (members.size() < 2) {
            msds.push_back(0.0);
            continue;
        }
        ArrayXXd subData(members.size(), data.cols());
        for (size_t i = 0; i < members.size(); ++i) {
            subData.row(i) = data.row(members[i]);
        }
        msds.push_back(meanSqDev(subData, nAtoms));
    }
    return msds;
}
