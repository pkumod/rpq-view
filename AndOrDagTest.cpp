#include <gtest/gtest.h>
#include "AndOrDag.h"
#include "Rpq2NFAConvertor.h"
using namespace std;

// AND-OR DAG file format: (expected output of CustomTest, input of buildAndOrDagFromFile)
// nodes
// #nodes
// each node: isEq opType #children child1 child2 … #startLabel lbl1 inv1 lbl2 inv2 ... #endLabel lbl1 inv1 lbl2 inv2 ...
// q2idx
// #q
// each line: q idx
// (Below is optional content)
// #eq nodes that need to specify targetChild
// each line: idx targetChild
void CustomTest(const std::string& testName) {
    // Generate input and expected output file names based on the test name
    string dataDir = "../test_data/AddQueryTestSuite/";
    std::string inputFileName = dataDir + testName + "_query.txt";
    std::string expectedOutputFileName = dataDir + testName + "_expected_output.txt";
    
    AndOrDag aod;

    // Read input data from the input file
    std::ifstream inputFile(inputFileName);
    cout << inputFileName << endl;
    ASSERT_EQ(inputFile.is_open(), true);
    string cur;
    vector<string> qVec;
    while (inputFile >> cur)
        qVec.emplace_back(cur);
    inputFile.close();

    // Calculate the result using the function being tested
    for (const auto &q : qVec)
        aod.addWorkloadQuery(q, 1);

    // Read the expected output from the expected output file
    std::ifstream expectedOutputFile(expectedOutputFileName);
    ASSERT_EQ(expectedOutputFile.is_open(), true);

    // Compare the actual result with the expected result
    size_t numNodes = 0;
    expectedOutputFile >> numNodes;
    ASSERT_EQ(aod.getNumNodes(), numNodes);
    bool isEq = false;
    int opType = 0;
    size_t numChildren = 0, curChild = 0, numLabel = 0, curLbl = 0;
    bool isInv = false;
    for (size_t i = 0; i < numNodes; i++) {
        expectedOutputFile >> isEq >> opType >> numChildren;
        EXPECT_EQ(aod.getNodes()[i].getIsEq(), isEq);
        EXPECT_EQ(int(aod.getNodes()[i].getOpType()), opType);
        EXPECT_EQ(aod.getNodes()[i].getChildIdx().size(), numChildren);
        for (size_t j = 0; j < numChildren; j++) {
            expectedOutputFile >> curChild;
            EXPECT_EQ(aod.getNodes()[i].getChildIdx()[j], curChild);
        }
        expectedOutputFile >> numLabel;
        for (size_t j = 0; j < numLabel; j++) {
            expectedOutputFile >> curLbl >> isInv;
            EXPECT_EQ(aod.getNodes()[i].getStartLabel()[j].lbl, curLbl);
            EXPECT_EQ(aod.getNodes()[i].getStartLabel()[j].inv, isInv);
        }
        expectedOutputFile >> numLabel;
        for (size_t j = 0; j < numLabel; j++) {
            expectedOutputFile >> curLbl >> isInv;
            EXPECT_EQ(aod.getNodes()[i].getEndLabel()[j].lbl, curLbl);
            EXPECT_EQ(aod.getNodes()[i].getEndLabel()[j].inv, isInv);
        }
    }

    size_t numQ = 0;
    expectedOutputFile >> numQ;
    ASSERT_EQ(aod.getQ2idx().size(), numQ);
    size_t qIdx = 0;
    for (size_t i = 0; i < numQ; i++) {
        expectedOutputFile >> cur;
        const auto &it = aod.getQ2idx().find(cur);
        EXPECT_EQ(it != aod.getQ2idx().end(), true);
        expectedOutputFile >> qIdx;
        EXPECT_EQ(it->second, qIdx);
    }
}

void buildAndOrDagFromFile(AndOrDag &aod, const string &inputFileName, bool getTargetChild=false) {
    std::ifstream inputFile(inputFileName);
    ASSERT_EQ(inputFile.is_open(), true);
    size_t numNodes = 0;
    inputFile >> numNodes;
    bool isEq = false;
    int opType = 0;
    size_t numChildren = 0, curChild = 0, numLabel = 0, curLabel = 0;
    bool isInv = false;
    aod.getNodes().resize(numNodes);
    aod.getWorkloadFreq().resize(numNodes);
    for (size_t i = 0; i < numNodes; i++) {
        inputFile >> isEq >> opType >> numChildren;
        aod.getNodes()[i].setIsEq(isEq);
        aod.getNodes()[i].setOpType(opType);
        for (size_t j = 0; j < numChildren; j++) {
            inputFile >> curChild;
            aod.addParentChild(i, curChild);
        }
        inputFile >> numLabel;
        for (size_t j = 0; j < numLabel; j++) {
            inputFile >> curLabel >> isInv;
            aod.getNodes()[i].addStartLabel(curLabel, isInv);
        }
        inputFile >> numLabel;
        for (size_t j = 0; j < numLabel; j++) {
            inputFile >> curLabel >> isInv;
            aod.getNodes()[i].addEndLabel(curLabel, isInv);
        }
    }
    size_t numQ = 0;
    inputFile >> numQ;
    size_t qIdx = 0;
    string cur;
    for (size_t i = 0; i < numQ; i++) {
        inputFile >> cur >> qIdx;
        aod.getQ2idx()[cur] = qIdx;
    }
    // Optionally set targetChild
    if (getTargetChild) {
        size_t numEq = 0;
        inputFile >> numEq;
        size_t curNodeIdx = 0, curTargetChild = 0;
        for (size_t i = 0; i < numEq; i++) {
            inputFile >> curNodeIdx >> curTargetChild;
            aod.getNodes()[curNodeIdx].setTargetChild(curTargetChild);
        }
    }
    inputFile.close();
}

class ExecuteTestSuite : public ::testing::TestWithParam<std::string> {
protected:
    std::shared_ptr<MultiLabelCSR> csrPtr;
    std::string dataDir;
    ExecuteTestSuite(): csrPtr(nullptr), dataDir("../test_data/ExecuteTestSuite/") {}
    void SetUp() override {
        string graphFilePath = dataDir + "graph.txt";
        csrPtr = make_shared<MultiLabelCSR>();
        csrPtr->loadGraph(graphFilePath);
    }
};

class ChooseMatViewsTestSuite : public ::testing::TestWithParam<vector<size_t>> {
protected:
    AndOrDag aod;
    std::shared_ptr<MultiLabelCSR> csrPtr;
    ChooseMatViewsTestSuite(): csrPtr(nullptr) {}
    void SetUp() override {
        string dataDir = "../test_data/ChooseMatViewsTestSuite/";
        string graphFilePath = dataDir + "KleeneIriConcatTest_graph.txt";
        csrPtr = make_shared<MultiLabelCSR>();
        csrPtr->loadGraph(graphFilePath);
        // Do not call fillStats, but directly assign stats
        size_t labelCnt = csrPtr->label2idx.size();
        csrPtr->stats.outCnt.resize(labelCnt);
        csrPtr->stats.inCnt.resize(labelCnt);
        csrPtr->stats.outCooccur.resize(labelCnt);
        csrPtr->stats.inCooccur.resize(labelCnt);
        for (size_t i = 0; i < labelCnt; i++) {
            csrPtr->stats.outCnt[i].assign(labelCnt, 0);
            csrPtr->stats.inCnt[i].assign(labelCnt, 0);
            csrPtr->stats.outCooccur[i].assign(labelCnt, 0);
            csrPtr->stats.inCooccur[i].assign(labelCnt, 0);
        }
        string statsFileName = dataDir + "KleeneIriConcatTest_stats.txt";
        std::ifstream statsFile(statsFileName);
        ASSERT_EQ(statsFile.is_open(), true);
        size_t outNum = 0, inNum = 0;
        size_t x = 0, y = 0, val = 0;
        statsFile >> outNum;
        for (size_t i = 0; i < outNum; i++) {
            statsFile >> x >> y >> val;
            csrPtr->stats.outCnt[csrPtr->label2idx[x]][csrPtr->label2idx[y]] = val;
        }
        statsFile >> inNum;
        for (size_t i = 0; i < inNum; i++) {
            statsFile >> x >> y >> val;
            csrPtr->stats.inCnt[csrPtr->label2idx[x]][csrPtr->label2idx[y]] = val;
        }
        statsFile.close();

        aod.setCsrPtr(csrPtr);
        string inputFileName = dataDir + "KleeneIriConcatTest_input.txt";
        buildAndOrDagFromFile(aod, inputFileName);
        aod.initAuxiliary();
        aod.getUseCnt().assign(aod.getNumNodes(), 0);
        string costFileName = dataDir + "KleeneIriConcatTest_cost.txt";
        std::ifstream costFile(costFileName);
        ASSERT_EQ(costFile.is_open(), true);
        size_t numLines = 0;
        costFile >> numLines;
        size_t curSrcCnt = 0, curDstCnt = 0;
        float curCost = 0, curPairProb = 0;
        for (size_t i = 0; i < numLines; i++) {
            costFile >> curCost >> curSrcCnt >> curDstCnt >> curPairProb;
            aod.setCost(i, curCost);
            aod.setSrcCnt(i, curSrcCnt);
            aod.setDstCnt(i, curDstCnt);
            aod.setPairProb(i, curPairProb);
        }
        costFile.close();
        string queryFileName = dataDir + "KleeneIriConcatTest_query2freq.txt";
        std::ifstream queryFile(queryFileName);
        ASSERT_EQ(queryFile.is_open(), true);
        string q;
        size_t useCnt_ = 0, workloadFreq_ = 0;
        while (queryFile >> q >> useCnt_ >> workloadFreq_)
            aod.setAsWorkloadQuery(q, useCnt_, workloadFreq_);
        queryFile.close();
    }
};

// Define test cases using the custom test case function
TEST(AddQueryTestSuite, SingleIriTest) {
    CustomTest("SingleIriTest");
}

TEST(AddQueryTestSuite, SingleInverseIriTest) {
    CustomTest("SingleInverseIriTest");
}

TEST(AddQueryTestSuite, SingleParenthesizedIriTest) {
    CustomTest("SingleParenthesizedIriTest");
}

TEST(AddQueryTestSuite, ConcatTest) {
    CustomTest("ConcatTest");
}

TEST(AddQueryTestSuite, AlternationTest) {
    CustomTest("AlternationTest");
}

TEST(AddQueryTestSuite, SingleIriKleeneTest) {
    CustomTest("SingleIriKleeneTest");
}

TEST(AddQueryTestSuite, ConcatKleeneTest) {
    CustomTest("ConcatKleeneTest");
}

TEST(AddQueryTestSuite, KleeneIriConcatTest) {
    CustomTest("KleeneIriConcatTest");
}

TEST(AddQueryTestSuite, QuerySubqueryTest) {
    CustomTest("QuerySubqueryTest");
}

TEST(AddQueryTestSuite, OverlapTest) {
    CustomTest("OverlapTest");
}

TEST(AnnotateLeafCostCardTestSuite, SimpleTest) {
    // Construct outCsr, inCsr for 0-[0]->1-[1]->2-[2]->3-[3]->4
    auto csrPtr = make_shared<MultiLabelCSR>();
    csrPtr->outCsr.resize(4);
    csrPtr->inCsr.resize(4);
    for (size_t i = 0; i < 4; i++) {
        csrPtr->label2idx[i] = i;
        csrPtr->outCsr[i].n = 1;
        csrPtr->outCsr[i].m = 1;
        csrPtr->outCsr[i].adj.emplace_back(i + 1);
        csrPtr->outCsr[i].offset = {0};
        csrPtr->outCsr[i].v2idx[i] = 0;
        csrPtr->inCsr[i].n = 1;
        csrPtr->inCsr[i].m = 1;
        csrPtr->inCsr[i].adj.emplace_back(i);
        csrPtr->inCsr[i].offset = {0};
        csrPtr->inCsr[i].v2idx[i + 1] = 0;
    }

    // Construct AndOrDag for (<1>/<2>)+/<3>
    AndOrDag aod(csrPtr);
    string dataDir = "../test_data/AnnotateLeafCostCardTestSuite/";
    string inputFileName = dataDir + "SimpleTest_input.txt";
    buildAndOrDagFromFile(aod, inputFileName);

    // Call annotateLeafCostCard
    aod.initAuxiliary();
    aod.annotateLeafCostCard();

    // Compare the actual result with the expected result
    // Output file format: #lines
    // each line nodeIdx srcCnt dstCnt pairProb cost (only leaf nodes)
    string expectedOutputFileName = dataDir + "SimpleTest_expected_output.txt";
    std::ifstream expectedOutputFile(expectedOutputFileName);
    ASSERT_EQ(expectedOutputFile.is_open(), true);
    size_t numLeafNodes = 0, nodeIdx = 0;
    expectedOutputFile >> numLeafNodes;
    size_t curSrcCnt = 0, curDstCnt = 0;
    float curPairProb = 0, curCost = 0;
    for (size_t i = 0; i < numLeafNodes; i++) {
        expectedOutputFile >> nodeIdx >> curSrcCnt >> curDstCnt >> curPairProb >> curCost;
        ASSERT_EQ(aod.getNumNodes() > nodeIdx, true);
        EXPECT_EQ(aod.getSrcCnt()[nodeIdx], curSrcCnt);
        EXPECT_EQ(aod.getDstCnt()[nodeIdx], curDstCnt);
        EXPECT_FLOAT_EQ(aod.getPairProb()[nodeIdx], curPairProb);
        EXPECT_FLOAT_EQ(aod.getCost()[nodeIdx], curCost);
    }
}

TEST(StatisticsTestSuite, SimpleTest) {
    // Assume CSR loadGraph is correct
    string graphFilePath = "../test_data/StatisticsTestSuite/SimpleTest_graph.txt";
    MultiLabelCSR mCsr;
    mCsr.loadGraph(graphFilePath);
    mCsr.fillStats();
    size_t numLabel = 0;
    string expectedOutputFileName = "../test_data/StatisticsTestSuite/SimpleTest_expected_output.txt";
    std::ifstream expectedOutputFile(expectedOutputFileName);
    ASSERT_EQ(expectedOutputFile.is_open(), true);
    expectedOutputFile >> numLabel;
    ASSERT_EQ(mCsr.stats.outCnt.size(), numLabel);
    ASSERT_EQ(mCsr.stats.inCnt.size(), numLabel);
    ASSERT_EQ(mCsr.stats.outCooccur.size(), numLabel);
    ASSERT_EQ(mCsr.stats.inCooccur.size(), numLabel);
    size_t curElem = 0;
    const std::vector<std::vector<size_t>> * ptr = nullptr;
    std::vector<std::vector<size_t>> *ptrArr[] = {&mCsr.stats.outCnt, &mCsr.stats.inCnt, &mCsr.stats.outCooccur, &mCsr.stats.inCooccur};
    for (size_t k = 0; k < 4; k++) {
        ptr = ptrArr[k];
        for (size_t i = 0; i < numLabel; i++) {
            ASSERT_EQ((*ptr)[mCsr.label2idx[i]].size(), numLabel);
            for (size_t j = 0; j < numLabel; j++) {
                expectedOutputFile >> curElem;
                EXPECT_EQ((*ptr)[mCsr.label2idx[i]][mCsr.label2idx[j]], curElem);
            }
        }
    }
}

TEST(PlanTestSuite, KleeneIriConcatTest) {
    // Assume CSR loadGraph is correct
    string dataDir = "../test_data/PlanTestSuite/";
    string graphFilePath = dataDir + "KleeneIriConcatTest_graph.txt";
    auto csrPtr = make_shared<MultiLabelCSR>();
    csrPtr->loadGraph(graphFilePath);
    // Do not call fillStats, but directly assign stats
    size_t labelCnt = csrPtr->label2idx.size();
    csrPtr->stats.outCnt.resize(labelCnt);
    csrPtr->stats.inCnt.resize(labelCnt);
    csrPtr->stats.outCooccur.resize(labelCnt);
    csrPtr->stats.inCooccur.resize(labelCnt);
    for (size_t i = 0; i < labelCnt; i++) {
        csrPtr->stats.outCnt[i].assign(labelCnt, 0);
        csrPtr->stats.inCnt[i].assign(labelCnt, 0);
        csrPtr->stats.outCooccur[i].assign(labelCnt, 0);
        csrPtr->stats.inCooccur[i].assign(labelCnt, 0);
    }
    string statsFileName = dataDir + "KleeneIriConcatTest_stats.txt";
    std::ifstream statsFile(statsFileName);
    ASSERT_EQ(statsFile.is_open(), true);
    size_t outNum = 0, inNum = 0;
    size_t x = 0, y = 0, val = 0;
    statsFile >> outNum;
    for (size_t i = 0; i < outNum; i++) {
        statsFile >> x >> y >> val;
        csrPtr->stats.outCnt[csrPtr->label2idx[x]][csrPtr->label2idx[y]] = val;
    }
    statsFile >> inNum;
    for (size_t i = 0; i < inNum; i++) {
        statsFile >> x >> y >> val;
        csrPtr->stats.inCnt[csrPtr->label2idx[x]][csrPtr->label2idx[y]] = val;
    }
    statsFile.close();

    AndOrDag aod(csrPtr);
    string inputFileName = dataDir + "KleeneIriConcatTest_input.txt";
    buildAndOrDagFromFile(aod, inputFileName);
    aod.initAuxiliary();
    aod.getUseCnt().assign(aod.getNumNodes(), 0);
    aod.annotateLeafCostCard();
    string queryFileName = dataDir + "KleeneIriConcatTest_query.txt";
    std::ifstream queryFile(queryFileName);
    ASSERT_EQ(queryFile.is_open(), true);
    string q;
    while (queryFile >> q)
        aod.setAsWorkloadQuery(q, 1);
    aod.plan();

    // Compare the actual result with the expected result
    string expectedOutputFileName = dataDir + "KleeneIriConcatTest_expected_output.txt";
    std::ifstream expectedOutputFile(expectedOutputFileName);
    ASSERT_EQ(expectedOutputFile.is_open(), true);
    size_t numLines = 0;
    expectedOutputFile >> numLines;
    const auto &cost = aod.getCost();
    const auto &srcCnt = aod.getSrcCnt(), &dstCnt = aod.getDstCnt();
    const auto &pairProb = aod.getPairProb();
    size_t curSrcCnt = 0, curDstCnt = 0;
    float curCost = 0, curPairProb = 0;
    for (size_t i = 0; i < numLines; i++) {
        expectedOutputFile >> curCost >> curSrcCnt >> curDstCnt >> curPairProb;
        EXPECT_FLOAT_EQ(cost[i], curCost);
        EXPECT_EQ(srcCnt[i], curSrcCnt);
        EXPECT_EQ(dstCnt[i], curDstCnt);
        EXPECT_FLOAT_EQ(pairProb[i], curPairProb);
    }
}

TEST(TopoSortTestSuite, KleeneIriConcatTest) {
    string dataDir = "../test_data/TopoSortTestSuite/";
    AndOrDag aod;
    string inputFileName = dataDir + "KleeneIriConcatTest_input.txt";
    buildAndOrDagFromFile(aod, inputFileName);
    aod.topoSort();
    string expectedOutputFileName = dataDir + "KleeneIriConcatTest_expected_output.txt";
    std::ifstream expectedOutputFile(expectedOutputFileName);
    ASSERT_EQ(expectedOutputFile.is_open(), true);
    size_t numNodes = 0, curOrder = 0;
    expectedOutputFile >> numNodes;
    for (size_t i = 0; i < numNodes; i++) {
        expectedOutputFile >> curOrder;
        EXPECT_EQ(aod.getNodes()[i].getTopoOrder(), curOrder);
    }
}

TEST(ReplanWithMaterializeTestSuite, KleeneIriConcatTest) {
    string dataDir = "../test_data/ReplanWithMaterializeTestSuite/";
    string graphFilePath = dataDir + "KleeneIriConcatTest_graph.txt";
    auto csrPtr = make_shared<MultiLabelCSR>();
    csrPtr->loadGraph(graphFilePath);
    // Do not call fillStats, but directly assign stats
    size_t labelCnt = csrPtr->label2idx.size();
    csrPtr->stats.outCnt.resize(labelCnt);
    csrPtr->stats.inCnt.resize(labelCnt);
    csrPtr->stats.outCooccur.resize(labelCnt);
    csrPtr->stats.inCooccur.resize(labelCnt);
    for (size_t i = 0; i < labelCnt; i++) {
        csrPtr->stats.outCnt[i].assign(labelCnt, 0);
        csrPtr->stats.inCnt[i].assign(labelCnt, 0);
        csrPtr->stats.outCooccur[i].assign(labelCnt, 0);
        csrPtr->stats.inCooccur[i].assign(labelCnt, 0);
    }
    string statsFileName = dataDir + "KleeneIriConcatTest_stats.txt";
    std::ifstream statsFile(statsFileName);
    ASSERT_EQ(statsFile.is_open(), true);
    size_t outNum = 0, inNum = 0;
    size_t x = 0, y = 0, val = 0;
    statsFile >> outNum;
    for (size_t i = 0; i < outNum; i++) {
        statsFile >> x >> y >> val;
        csrPtr->stats.outCnt[csrPtr->label2idx[x]][csrPtr->label2idx[y]] = val;
    }
    statsFile >> inNum;
    for (size_t i = 0; i < inNum; i++) {
        statsFile >> x >> y >> val;
        csrPtr->stats.inCnt[csrPtr->label2idx[x]][csrPtr->label2idx[y]] = val;
    }
    statsFile.close();

    AndOrDag aod(csrPtr);
    string inputFileName = dataDir + "KleeneIriConcatTest_input.txt";
    buildAndOrDagFromFile(aod, inputFileName);
    aod.initAuxiliary();
    aod.getUseCnt().assign(aod.getNumNodes(), 0);
    string costFileName = dataDir + "KleeneIriConcatTest_cost.txt";
    std::ifstream costFile(costFileName);
    ASSERT_EQ(costFile.is_open(), true);
    size_t numLines = 0;
    costFile >> numLines;
    size_t curSrcCnt = 0, curDstCnt = 0;
    float curCost = 0, curPairProb = 0;
    for (size_t i = 0; i < numLines; i++) {
        costFile >> curCost >> curSrcCnt >> curDstCnt >> curPairProb;
        aod.setCost(i, curCost);
        aod.setSrcCnt(i, curSrcCnt);
        aod.setDstCnt(i, curDstCnt);
        aod.setPairProb(i, curPairProb);
    }
    costFile.close();
    string queryFileName = dataDir + "KleeneIriConcatTest_query.txt";
    std::ifstream queryFile(queryFileName);
    ASSERT_EQ(queryFile.is_open(), true);
    string q;
    while (queryFile >> q)
        aod.setAsWorkloadQuery(q, 1);
    queryFile.close();

    string matFileName = dataDir + "KleeneIriConcatTest_mat.txt";
    std::ifstream matFile(matFileName);
    ASSERT_EQ(matFile.is_open(), true);
    std::vector<size_t> matIdx;
    std::unordered_map<size_t, float> node2cost;
    float reducedCost = 0;
    while (matFile >> q) {
        auto it = aod.getQ2idx().find(q);
        ASSERT_EQ(it != aod.getQ2idx().end(), true);
        matIdx.emplace_back(it->second);
    }
    aod.replanWithMaterialize(matIdx, node2cost, reducedCost);

    // Compare the actual result with the expected result
    // File format: len(node2cost)
    // each line: nodeIdx cost
    // last line: reducedCost
    string expectedOutputFileName = dataDir + "KleeneIriConcatTest_expected_output.txt";
    std::ifstream expectedOutputFile(expectedOutputFileName);
    ASSERT_EQ(expectedOutputFile.is_open(), true);
    size_t numNode2cost = 0, curNodeIdx = 0;
    expectedOutputFile >> numNode2cost;
    ASSERT_EQ(node2cost.size(), numNode2cost);
    for (size_t i = 0; i < numNode2cost; i++) {
        expectedOutputFile >> curNodeIdx >> curCost;
        auto it = node2cost.find(curNodeIdx);
        ASSERT_EQ(it != node2cost.end(), true);
        EXPECT_FLOAT_EQ(it->second, curCost);
    }
    expectedOutputFile >> curCost;
    EXPECT_FLOAT_EQ(reducedCost, curCost);
    expectedOutputFile.close();
}

TEST_P(ChooseMatViewsTestSuite, KleeneIriConcatTest) {
    string testOutput;
    const auto &curParam = GetParam();
    size_t isCopy = curParam[0], curMode = curParam[1], curBudget = curParam[2];
    float retRealBenefit = 0;
    size_t usedSpace = 0;
    if (isCopy == 0)
        retRealBenefit = aod.chooseMatViews(curMode, usedSpace, curBudget, &testOutput);
    else {
        AndOrDag tmpAod(aod);
        retRealBenefit = tmpAod.chooseMatViews(curMode, usedSpace, curBudget, &testOutput);
    }

    // Compare the actual result with the expected result
    // File format: triples of (nodeIdx, satCond (1/0), realBenefit)
    // satCond: if true, enters the if branch; otherwise, enters the else branch
    string dataDir = "../test_data/ChooseMatViewsTestSuite/";
    string expectedOutputFileName = dataDir + "KleeneIriConcatTest_expected_output";
    expectedOutputFileName += "_" + std::to_string(curMode);
    if (curBudget == std::numeric_limits<size_t>::max())
        expectedOutputFileName += "_max";
    else
        expectedOutputFileName += "_" + std::to_string(curBudget);
    expectedOutputFileName += ".txt";
    std::ifstream expectedOutputFile(expectedOutputFileName);
    ASSERT_EQ(expectedOutputFile.is_open(), true);
    float totalRealBenefit = 0;
    if (curMode == 0) {
        vector<size_t> nodeIdxVec;
        size_t curNodeIdx = 0;
        vector<bool> satCondVec;
        bool curSatCond = 0;
        vector<float> realBenefitVec;
        float curRealBenefit = 0;
        stringstream ss(testOutput);
        while (ss >> curNodeIdx >> curSatCond >> curRealBenefit) {
            nodeIdxVec.emplace_back(curNodeIdx);
            satCondVec.emplace_back(curSatCond);
            realBenefitVec.emplace_back(curRealBenefit);
        }
        size_t i = 0;
        expectedOutputFile >> totalRealBenefit;
        EXPECT_FLOAT_EQ(retRealBenefit, totalRealBenefit);
        while (expectedOutputFile >> curNodeIdx >> curSatCond >> curRealBenefit) {
            EXPECT_EQ(nodeIdxVec[i], curNodeIdx);
            EXPECT_EQ(satCondVec[i], curSatCond);
            EXPECT_FLOAT_EQ(realBenefitVec[i], curRealBenefit);
            i++;
        }
    } else {
        vector<size_t> nodeIdxVec;
        vector<bool> decisionVec;
        size_t curNodeIdx = 0;
        bool curDecision = false;
        stringstream ss(testOutput);
        while (ss >> curNodeIdx >> curDecision) {
            nodeIdxVec.emplace_back(curNodeIdx);
            decisionVec.emplace_back(curDecision);
        }
        size_t i = 0;
        expectedOutputFile >> totalRealBenefit;
        EXPECT_FLOAT_EQ(retRealBenefit, totalRealBenefit);
        while (expectedOutputFile >> curNodeIdx >> curDecision) {
            EXPECT_EQ(nodeIdxVec[i], curNodeIdx);
            EXPECT_EQ(decisionVec[i], curDecision);
            i++;
        }
    }
}

INSTANTIATE_TEST_SUITE_P(ChooseMatViewsTestSuiteInstance, ChooseMatViewsTestSuite,
::testing::Values(vector<size_t>({0, 0, numeric_limits<size_t>::max()}), vector<size_t>({0, 0, 3}),
vector<size_t>({0, 1, numeric_limits<size_t>::max()}), vector<size_t>({0, 1, 3}),
vector<size_t>({0, 2, numeric_limits<size_t>::max()}), vector<size_t>({0, 2, 3}),
vector<size_t>({0, 3, numeric_limits<size_t>::max()}), vector<size_t>({0, 3, 3}),
vector<size_t>({0, 4, numeric_limits<size_t>::max()}), vector<size_t>({0, 4, 3}),
vector<size_t>({1, 0, numeric_limits<size_t>::max()}), vector<size_t>({1, 0, 3}),
vector<size_t>({1, 1, numeric_limits<size_t>::max()}), vector<size_t>({1, 1, 3}),
vector<size_t>({1, 2, numeric_limits<size_t>::max()}), vector<size_t>({1, 2, 3}),
vector<size_t>({1, 3, numeric_limits<size_t>::max()}), vector<size_t>({1, 3, 3}),
vector<size_t>({1, 4, numeric_limits<size_t>::max()}), vector<size_t>({1, 4, 3})
));

void compareExecuteResult(const string &expectedOutputFileName, MultiLabelCSR *dataCsrPtr,
MappedCSR *resCsrPtr, bool explicitEpsilon=false) {
    std::ifstream expectedOutputFile(expectedOutputFileName);
    ASSERT_EQ(expectedOutputFile.is_open(), true);
    size_t numNodes = 0;
    expectedOutputFile >> numNodes;
    size_t curNodeIdx = 0, numNeighbors = 0, curNeighbor = 0;
    unordered_map<size_t, unordered_set<size_t>> realAdjList;
    AdjInterval aitv;
    for (size_t i = 0; i < numNodes; i++) {
        expectedOutputFile >> curNodeIdx >> numNeighbors;
        realAdjList[curNodeIdx] = unordered_set<size_t>();
        for (size_t j = 0; j < numNeighbors; j++) {
            expectedOutputFile >> curNeighbor;
            realAdjList[curNodeIdx].emplace(curNeighbor);
        }
    }
    bool realHasEpsilon = false;
    expectedOutputFile >> realHasEpsilon;
    expectedOutputFile.close();
    if (explicitEpsilon && realHasEpsilon) {
        // For DFA execution (explicitEpsilon == true), explicitly enhance the expected output if hasEpsilon == true
        for (size_t i = 0; i <= dataCsrPtr->maxNode; i++) {
            if (realAdjList.find(i) == realAdjList.end())
                realAdjList[i] = unordered_set<size_t>();
            realAdjList[i].emplace(i);
        }
    }
    // Compare the query result with the ground truth
    // Do not compare the number of nodes, since node idx may not be continuous
    // TODO: NFA execute didn't map the labels
    for (const auto &pr : resCsrPtr->v2idx) {
        size_t curNodeIdx = pr.first;
        ASSERT_EQ(realAdjList.find(curNodeIdx) != realAdjList.end(), true);
        resCsrPtr->getAdjIntervalByVert(curNodeIdx, aitv);
        ASSERT_EQ(aitv.len, realAdjList[curNodeIdx].size());
        for (size_t j = 0; j < aitv.len; j++)
            EXPECT_EQ(realAdjList[curNodeIdx].find((*aitv.start)[aitv.offset + j]) != realAdjList[curNodeIdx].end(), true);
    }
}

TEST_P(ExecuteTestSuite, ExecuteTest) {
    const string &testName = GetParam();
    AndOrDag aod;
    aod.setCsrPtr(csrPtr);
    string inputFileName = dataDir + testName + "_input.txt";
    if (testName == "ConcatTest")
        buildAndOrDagFromFile(aod, inputFileName, true);
    else
        buildAndOrDagFromFile(aod, inputFileName, false);
    aod.initAuxiliary();
    QueryResult qr(nullptr, false);
    string queryFileName = dataDir + testName + "_query.txt";
    std::ifstream queryFile(queryFileName);
    ASSERT_EQ(queryFile.is_open(), true);
    string q;
    queryFile >> q;
    queryFile.close();
    aod.execute(q, qr);

    // Compare the actual result with the expected result
    // Expected query result output file format: #nodes
    // each line: nodeIdx #neighbors neighbor1 neighbor2 ...
    string expectedOutputFileName = dataDir + testName + "_expected_output.txt";
    compareExecuteResult(expectedOutputFileName, csrPtr.get(), qr.csrPtr, false);
}

TEST_P(ExecuteTestSuite, NfaExecuteTest) {
    const string &testName = GetParam();
    string queryFileName = dataDir + testName + "_query.txt";
    std::ifstream queryFile(queryFileName);
    ASSERT_EQ(queryFile.is_open(), true);
    string q;
    queryFile >> q;
    queryFile.close();
    Rpq2NFAConvertor cvrt;
    shared_ptr<NFA> dfaPtr = cvrt.convert(q)->convert2Dfa();
    shared_ptr<MappedCSR> res = dfaPtr->execute(csrPtr);
    // Compare the actual result with the expected result
    string expectedOutputFileName = dataDir + testName + "_expected_output.txt";
    compareExecuteResult(expectedOutputFileName, csrPtr.get(), res.get(), true);
}

INSTANTIATE_TEST_SUITE_P(ExecuteTestSuiteInstance, ExecuteTestSuite, ::testing::Values("SingleIriTest", "SingleInverseIriTest",
"AlternationTest", "ConcatTest", "ConcatKleeneTest", "KleeneIriConcatTest", "KleeneStarIriConcatTest", "IriKleeneStarConcat"));