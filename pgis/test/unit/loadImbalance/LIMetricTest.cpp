/**
 * File: LIMetricTest.cpp
 * License: Part of the metacg project. Licensed under BSD 3 clause license. See LICENSE.txt file at https://github.com/tudasc/metacg/LICENSE.txt
 */

#include "gtest/gtest.h"
#include <loadImbalance/metric/EfficiencyMetric.h>
#include <loadImbalance/metric/ImbalancePercentageMetric.h>
#include <loadImbalance/metric/VariationCoeffMetric.h>

using namespace metacg;

TEST(LIMetricTest, EfficiencyMetric) {
  LoadImbalance::EfficiencyMetric metric;

  CgNodePtr func1 = std::make_shared<CgNode>("func1"), func2 = std::make_shared<CgNode>("func2"),
            func3 = std::make_shared<CgNode>("func3"), func4 = std::make_shared<CgNode>("func4"),
            func5 = std::make_shared<CgNode>("func5"), func6 = std::make_shared<CgNode>("func6"),
            main = std::make_shared<CgNode>("main");

  {
    // func1: no profiling data
    metric.setNode(func1);
    ASSERT_EQ(metric.calc(), 0.0);
  }
  {
    // func2: single call

    func2->get<pira::BaseProfileData>()->setCallData(main, 1, 1.0, 1.0, 1, 1);
    metric.setNode(func2);
    ASSERT_EQ(metric.calc(), 1.0);
  }
  {
    // func3: 2 identical calls

    func3->get<pira::BaseProfileData>()->setCallData(main, 1, 1.0, 1.0, 1, 1);
    func3->get<pira::BaseProfileData>()->setCallData(main, 1, 1.0, 1.0, 1, 2);
    metric.setNode(func3);
    ASSERT_EQ(metric.calc(), 1.0);
  }
  {
    // func4: moderate speedup
    func4->get<pira::BaseProfileData>()->setCallData(main, 1, 4.0, 4.0, 1, 1);
    func4->get<pira::BaseProfileData>()->setCallData(main, 1, 6.0, 6.0, 1, 2);
    metric.setNode(func4);
    ASSERT_EQ(metric.calc(), 1.2);
  }
  {
    // func5: extreme speedup
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 1);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 2);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 3);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 4);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 5);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 45.0, 45.0, 1, 6);
    metric.setNode(func5);
    EXPECT_NEAR(metric.calc(), 4.909090909090909, 0.001);
  }
  {
    // func6: "empty" calls (to be ignored)
    func6->get<pira::BaseProfileData>()->setCallData(main, 1, 10.0, 10.0, 1, 1);
    func6->get<pira::BaseProfileData>()->setCallData(main, 1, 10.0, 10.0, 1, 2);
    func6->get<pira::BaseProfileData>()->setCallData(main, 1, 0.0, 0.0, 1, 3);
    metric.setNode(func6);
    ASSERT_EQ(metric.calc(), 1.0);
  }
}

TEST(LIMetricTest, VariationCoeffMetric) {
  LoadImbalance::VariationCoeffMetric metric;

  CgNodePtr func1 = std::make_shared<CgNode>("func1"), func2 = std::make_shared<CgNode>("func2"),
            func3 = std::make_shared<CgNode>("func3"), func4 = std::make_shared<CgNode>("func4"),
            func5 = std::make_shared<CgNode>("func5"), func6 = std::make_shared<CgNode>("func6"),
            main = std::make_shared<CgNode>("main");

  {
    // func1: no profiling data
    metric.setNode(func1);
    ASSERT_EQ(metric.calc(), 0.0);
  }
  {
    // func2: single call

    func2->get<pira::BaseProfileData>()->setCallData(main, 1, 1.0, 1.0, 1, 1);
    metric.setNode(func2);
    ASSERT_EQ(metric.calc(), 0.0);
  }
  {
    // func3: 2 identical calls

    func3->get<pira::BaseProfileData>()->setCallData(main, 1, 1.0, 1.0, 1, 1);
    func3->get<pira::BaseProfileData>()->setCallData(main, 1, 1.0, 1.0, 1, 2);
    metric.setNode(func3);
    ASSERT_EQ(metric.calc(), 0.0);
  }
  {
    // func4: moderate speedup
    func4->get<pira::BaseProfileData>()->setCallData(main, 1, 4.0, 4.0, 1, 1);
    func4->get<pira::BaseProfileData>()->setCallData(main, 1, 6.0, 6.0, 1, 2);
    metric.setNode(func4);
    EXPECT_NEAR(metric.calc(), 0.2, 0.001);
  }
  {
    // func5: extreme speedup
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 1);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 2);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 3);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 4);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 5);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 45.0, 45.0, 1, 6);
    metric.setNode(func5);
    EXPECT_NEAR(metric.calc(), 1.748198, 0.001);
    // ASSERT_TRUE(false);
  }
  {
    // func6: "empty" calls (to be ignored)
    func6->get<pira::BaseProfileData>()->setCallData(main, 1, 10.0, 10.0, 1, 1);
    func6->get<pira::BaseProfileData>()->setCallData(main, 1, 10.0, 10.0, 1, 2);
    func6->get<pira::BaseProfileData>()->setCallData(main, 1, 0.0, 0.0, 1, 3);
    metric.setNode(func6);
    ASSERT_EQ(metric.calc(), 0.0);
  }
}

TEST(LIMetricTest, ImbalancePercentageMetric) {
  LoadImbalance::ImbalancePercentageMetric metric;

  CgNodePtr func1 = std::make_shared<CgNode>("func1"), func2 = std::make_shared<CgNode>("func2"),
            func3 = std::make_shared<CgNode>("func3"), func4 = std::make_shared<CgNode>("func4"),
            func5 = std::make_shared<CgNode>("func5"), func6 = std::make_shared<CgNode>("func6"),
            main = std::make_shared<CgNode>("main");

  {
    // func1: no profiling data
    metric.setNode(func1);
    ASSERT_EQ(metric.calc(), 0.0);
  }
  {
    // func2: single call

    func2->get<pira::BaseProfileData>()->setCallData(main, 1, 1.0, 1.0, 1, 1);
    metric.setNode(func2);
    ASSERT_EQ(metric.calc(), 0.0);
  }
  {
    // func3: 2 identical calls

    func3->get<pira::BaseProfileData>()->setCallData(main, 1, 1.0, 1.0, 1, 1);
    func3->get<pira::BaseProfileData>()->setCallData(main, 1, 1.0, 1.0, 1, 2);
    metric.setNode(func3);
    ASSERT_EQ(metric.calc(), 0.0);
  }
  {
    // func4: moderate speedup
    func4->get<pira::BaseProfileData>()->setCallData(main, 1, 4.0, 4.0, 1, 1);
    func4->get<pira::BaseProfileData>()->setCallData(main, 1, 6.0, 6.0, 1, 2);
    metric.setNode(func4);
    EXPECT_NEAR(metric.calc(), 0.33333, 0.001);
  }
  {
    // func5: extreme speedup
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 1);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 2);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 3);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 4);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 2.0, 2.0, 1, 5);
    func5->get<pira::BaseProfileData>()->setCallData(main, 1, 45.0, 45.0, 1, 6);
    metric.setNode(func5);
    EXPECT_NEAR(metric.calc(), 0.9555, 0.001);
    // ASSERT_TRUE(false);
  }
  {
    // func6: "empty" calls (to be ignored)
    func6->get<pira::BaseProfileData>()->setCallData(main, 1, 10.0, 10.0, 1, 1);
    func6->get<pira::BaseProfileData>()->setCallData(main, 1, 10.0, 10.0, 1, 2);
    func6->get<pira::BaseProfileData>()->setCallData(main, 1, 0.0, 0.0, 1, 3);
    metric.setNode(func6);
    ASSERT_EQ(metric.calc(), 0.0);
  }
}
