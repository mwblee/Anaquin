#include <catch.hpp>
#include "data/standard.hpp"

using namespace Spike;

TEST_CASE("Standard_ID")
{
	REQUIRE("chrT" == Standard::instance().id);
}

TEST_CASE("Standard_DNA")
{
    //REQUIRE(Standard::instance().d_seqs.size() == 72);
}

//TEST_CASE("Standard_Introns")
//{
//    const auto &s = Standard::instance();
//
//    REQUIRE(s.r_introns.size() == 436);
//    REQUIRE(s.r_introns[0].id == "chrT");
//    REQUIRE(s.r_introns[0].type == Intron);
//    REQUIRE(s.r_introns[0].l == Locus(388627, 447004));
//}

//TEST_CASE("Standard_RNA_Sequins")
//{
//    const auto &s = Standard::instance();
//
//    REQUIRE(s.r_genes.size()  == 32);
//    REQUIRE(s.r_seqs_A.size() == 61);
//    REQUIRE(s.r_seqs_B.size() == 61);
//
//    const auto genes =
//    {
//        "R_1_1",  "R_1_2",  "R_1_3", "R_1_4",
//        "R_2_1",  "R_2_2",  "R_2_3", "R_2_4",
//        "R_3_1",  "R_3_2",  "R_3_3",
//        "R_4_1",  "R_4_2",  "R_4_3",
//        "R_5_1",  "R_5_2",  "R_5_3",
//        "R_6_1",  "R_6_2",  "R_6_3",
//        "R_7_1",  "R_7_2",  "R_7_3",
//        "R_8_1",  "R_8_2",  "R_8_3",
//        "R_9_1",  "R_9_2",  "R_9_3",
//        "R_10_1", "R_10_2", "R_10_3",
//    };
//    
//    const auto trans =
//    {
//        "R_1_1_R",  "R_1_1_V",  "R_1_2_R",  "R_1_2_V",  "R_1_3_R",  "R_1_3_V",
//        "R_1_4_R",  "R_10_1_R", "R_10_1_V", "R_10_2_R", "R_10_2_V", "R_10_3_R",
//        "R_10_3_V", "R_2_1_R",  "R_2_1_V",  "R_2_2_R",  "R_2_2_V",  "R_2_3_R",
//        "R_2_3_V",  "R_2_4_R",  "R_3_1_R",  "R_3_1_V",  "R_3_2_R",  "R_3_2_V",
//        "R_3_3_R",  "R_3_3_V",  "R_4_1_R",  "R_4_1_V",  "R_4_2_R",  "R_4_2_V",
//        "R_4_3_R",  "R_4_3_V",  "R_5_1_R",  "R_5_1_V",  "R_5_2_R",  "R_5_2_V",
//        "R_5_3_R",  "R_6_1_R",  "R_6_1_V",  "R_6_2_R",  "R_6_2_V",  "R_6_3_R",
//        "R_6_3_V",  "R_7_1_R",  "R_7_1_V",  "R_7_2_R",  "R_7_2_V",  "R_7_3_R",
//        "R_7_3_V",  "R_8_1_R",  "R_8_1_V",  "R_8_2_R",  "R_8_2_V",  "R_8_3_R",
//        "R_8_3_V",  "R_9_1_R",  "R_9_1_V",  "R_9_2_R",  "R_9_2_V",  "R_9_3_R",
//        "R_9_3_V"
//    };
//
//    for (auto t : trans)
//    {
//        REQUIRE(s.r_seqs_A.count(t));
//        REQUIRE(s.r_seqs_B.count(t));
//    }
//}