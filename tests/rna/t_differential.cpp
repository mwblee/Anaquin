#include <catch.hpp>
#include "rna/r_differential.hpp"

using namespace Spike;

TEST_CASE("Differential_Simulations_Isoforms")
{
    /*
     * The experiment for mixutre B is identical but it has 100 times coverage.
     */

    RDifferential::Options o;
    o.level = RNALevel::Isoform;
    const auto r = RDifferential::analyze("tests/data/rna/isoform_exp.diff", o);

    REQUIRE(r.lm.m  == Approx(0.9771063293));
    REQUIRE(r.lm.r  == Approx(0.9637073186));
    REQUIRE(r.lm.r2 == Approx(0.9268056282));
}

TEST_CASE("Differential_Simulations_Genes")
{
    /*
     * The experiment for mixutre B is identical but it has 100 times coverage.
     */

    RDifferential::Options o;
    o.level = RNALevel::Gene;
    const auto r = RDifferential::analyze("tests/data/rna/gene_exp.diff", o);

    REQUIRE(r.lm.m  == Approx(1.0824361534));
    REQUIRE(r.lm.r  == Approx(0.8374341202));
    REQUIRE(r.lm.r2 == Approx(0.6863607009));
}