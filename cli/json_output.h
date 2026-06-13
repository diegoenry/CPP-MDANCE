#pragma once

#include <string>
#include <vector>

struct ClusterResult {
    std::string algorithm;
    int nFrames = 0;
    int nClusters = 0;
    std::vector<int> labels;
    std::vector<int> clusterSizes;
    std::vector<int> representatives;
    std::vector<double> clusterMSD;
    double chScore = 0.0;
    double dbScore = 0.0;
    std::vector<std::vector<double>> zMatrix; // HELM only
};

void writeResultJSON(const std::string& filename, const ClusterResult& result);
