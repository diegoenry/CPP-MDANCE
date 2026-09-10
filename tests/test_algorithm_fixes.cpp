#include <gtest/gtest.h>

#include <cmath>
#include <set>

#include "../src/tools/bts.h"
#include "../src/tools/cluster.h"
#include "../src/tools/scores.h"
#include "../src/cluster/KMeansRex/KMeans.h"

namespace {

// `nBlobs` tight groups spread `spacing` apart, `perBlob` rows each.
ArrayXXd makeBlobs(int nBlobs, int perBlob, int ncols, double spacing,
                   double offset = 0.0) {
    ArrayXXd data(nBlobs * perBlob, ncols);
    for (int b = 0; b < nBlobs; ++b) {
        for (int i = 0; i < perBlob; ++i) {
            for (int j = 0; j < ncols; ++j) {
                data(b * perBlob + i, j) = offset + b * spacing + (i % 3) * 0.01;
            }
        }
    }
    return data;
}

}  // namespace

// repSample's fill loop tested `sampledMols.size() < nSamples` against a VectorXi
// that was already sized to nSamples -- a constant-false condition, so the loop
// never ran and the function returned an UNINITIALIZED vector. Every caller got
// garbage frame indices.
TEST(AlgorithmFixes, RepSampleReturnsRealFrameIndices) {
    ArrayXXd data = makeBlobs(4, 10, 6, 25.0);
    const int N = (int)data.rows();

    ArrayXi picked = repSample(data, MD::Metric::MSD, 2, 5, 12, true);

    ASSERT_EQ(picked.size(), 12);
    std::set<int> distinct;
    for (int i = 0; i < picked.size(); ++i) {
        EXPECT_GE(picked(i), 0) << "index " << i;
        EXPECT_LT(picked(i), N) << "index " << i;
        distinct.insert(picked(i));
    }
    EXPECT_EQ((int)distinct.size(), 12) << "sampled the same frame twice";
}

// The bin width was std::floor((max - min) / nBins), which truncates to 0
// whenever the complementary-similarity range is narrower than nBins. Every row
// then advanced the bin cursor and it ran off the end of the bin vector.
TEST(AlgorithmFixes, RepSampleHandlesDegenerateRange) {
    ArrayXXd data = ArrayXXd::Constant(30, 4, 7.0);   // identical rows -> zero range

    ArrayXi picked = repSample(data, MD::Metric::MSD, 1, 10, 8, true);

    ASSERT_LE(picked.size(), 8);
    for (int i = 0; i < picked.size(); ++i) {
        EXPECT_GE(picked(i), 0);
        EXPECT_LT(picked(i), 30);
    }
}

TEST(AlgorithmFixes, RepSampleCapsAtDatasetSize) {
    ArrayXXd data = makeBlobs(2, 5, 3, 40.0);         // 10 frames
    ArrayXi picked = repSample(data, MD::Metric::MSD, 1, 4, 500, true);
    EXPECT_LE(picked.size(), 10);
    EXPECT_GT(picked.size(), 0);
}

// The default constructor's body was four statements with no effect, leaving n
// with whatever happened to be on the stack.
TEST(AlgorithmFixes, ClusterDefaultConstructorInitializesCount) {
    Cluster c;
    EXPECT_EQ(c.getN(), 0);
}

// A single cluster makes the Calinski-Harabasz ratio 0/0. The NaN propagated
// into every serialized result.
TEST(AlgorithmFixes, ScoresAreFiniteForSingleCluster) {
    ArrayXXd data = makeBlobs(1, 9, 4, 0.0);
    VectorXi labels = VectorXi::Zero(9);

    double ch = calinskiHarabaszScore(data, labels);
    double db = daviesBouldinScore(data, labels);
    EXPECT_TRUE(std::isfinite(ch)) << "calinskiHarabasz = " << ch;
    EXPECT_TRUE(std::isfinite(db)) << "daviesBouldin = " << db;
}

// init_Mu truncated when the seeder returned MORE centers than requested but not
// when it returned fewer. assignClosest then handed out labels in [0, k) against
// a shorter `centers`, and calcMu indexed past the end -- a segfault in a release
// build, on any trajectory short enough that percentage% of it is under k.
TEST(AlgorithmFixes, KmeansRejectsUnderproducedSeeds) {
    ArrayXXd data = makeBlobs(3, 3, 4, 50.0);        // 9 frames; 10% of 9 -> 0 seeds
    EXPECT_THROW(KmeansNANI(data, 3, MD::Metric::MSD, MD::KinitType::StratAll, 1, 10),
                 std::runtime_error);
    // Sampling the whole trajectory gives the seeder enough to work with.
    EXPECT_NO_THROW(KmeansNANI(data, 3, MD::Metric::MSD, MD::KinitType::StratAll, 1, 100));
}

TEST(AlgorithmFixes, KmeansRejectsMoreClustersThanFrames) {
    ArrayXXd data = makeBlobs(3, 3, 4, 50.0);        // 9 frames
    EXPECT_THROW(KmeansNANI(data, 20, MD::Metric::MSD, MD::KinitType::StratAll, 1, 100),
                 std::runtime_error);
}

// KinitType::KmeansPP was absent from init_Mu's switch, so `centers` stayed at
// its Mat::Zero initialization. Every frame is then equidistant from every
// center, assignClosest puts them all in cluster 0, and the run collapses to a
// single cluster whatever k was asked for.
TEST(AlgorithmFixes, KmeansPPSeedsFromTheData) {
    // Offset well away from the origin so a zeroed center is never the nearest
    // point for anything -- otherwise Lloyd drifts back to a sane partition and
    // masks the uninitialized centers.
    ArrayXXd data = makeBlobs(3, 8, 5, 1000.0, 10000.0);

    KmeansNANI km(data, 3, MD::Metric::MSD, MD::KinitType::KmeansPP, 1, 100);
    Veci labels = km.getLabels();

    std::set<int> distinct;
    for (int i = 0; i < labels.size(); ++i) distinct.insert(labels(i));
    EXPECT_EQ((int)distinct.size(), 3) << "collapsed to " << distinct.size() << " cluster(s)";
}

// Both constructors initialized `seed` from itself, so the Mersenne Twister was
// seeded with an indeterminate value and the randomized initializers were not
// reproducible between runs of the same input.
TEST(AlgorithmFixes, RandomizedInitializationIsReproducible) {
    ArrayXXd data = makeBlobs(3, 8, 5, 1000.0, 10000.0);

    Veci first  = KmeansNANI(data, 3, MD::Metric::MSD, MD::KinitType::KmeansPP, 1, 100).getLabels();
    Veci second = KmeansNANI(data, 3, MD::Metric::MSD, MD::KinitType::KmeansPP, 1, 100).getLabels();

    ASSERT_EQ(first.size(), second.size());
    EXPECT_TRUE((first.array() == second.array()).all()) << "same input, different labels";
}
