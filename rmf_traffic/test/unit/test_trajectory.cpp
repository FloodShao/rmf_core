/*
 * Copyright (C) 2019 Open Source Robotics Foundation
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
*/

#include "utils_Trajectory.hpp"
#include <src/rmf_traffic/debug_Trajectory.hpp>
#include <rmf_utils/catch.hpp>
#include <iostream>

using namespace rmf_traffic;
using namespace Eigen;
using namespace std::chrono;

using AgencyType = Trajectory::Profile::Agency;

SCENARIO("Profile unit tests")
{
  // Profile Construction and Getters
  GIVEN("Construction values for Profile")
  {
    std::shared_ptr<geometry::Box> unitBox_shape = std::make_shared<geometry::Box>(1.0, 1.0);
    std::shared_ptr<geometry::Circle> unitCircle_shape = std::make_shared<geometry::Circle>(1.0);
    std::string queue_number = "5";

    WHEN("Constructing a Profile given shape and agency")
    {
      Trajectory::ProfilePtr strict_profile = Trajectory::Profile::make_strict(unitBox_shape);
      Trajectory::ProfilePtr queue_profile = Trajectory::Profile::make_queued(unitCircle_shape, queue_number);

      THEN("Profile is constructed according to specifications.")
      {
        CHECK(strict_profile->get_shape() == unitBox_shape);
        CHECK(strict_profile->get_agency() == AgencyType::Strict);
        CHECK(strict_profile->get_queue_info() == nullptr);

        CHECK(queue_profile->get_shape() == unitCircle_shape);
        CHECK(queue_profile->get_agency() == AgencyType::Queued);
        CHECK(queue_profile->get_queue_info()->get_queue_id() == queue_number);
      }
    }

    WHEN("Shape object used for profile construction is changed")
    {
      Trajectory::ProfilePtr strict_profile = Trajectory::Profile::make_strict(unitBox_shape);
      *unitBox_shape = geometry::Box(2.0, 2.0);

      THEN("Profile is still valid")
      {
        CHECK(strict_profile->get_shape() == unitBox_shape);
        // TODO: I assume that the profile shape is updated accordingly, but I do not know how to check
      }
    }

    WHEN("Pointer for shape used for profile construction is changed")
    {
      Trajectory::ProfilePtr strict_profile = Trajectory::Profile::make_strict(unitBox_shape);
      geometry::Box *ptr_address = unitBox_shape.get();
      unitBox_shape = std::make_shared<geometry::Box>(2.0, 2.0);

      THEN("Profile shape is unaffected")
      {
        CHECK(strict_profile->get_shape() != unitBox_shape);
        CHECK(strict_profile->get_shape().get() == ptr_address);
      }
    }

    WHEN("Shape object used for profile construction is moved")
    {
      // Move constructor
      Trajectory::ProfilePtr strict_profile = Trajectory::Profile::make_strict(unitBox_shape);
      std::shared_ptr<geometry::Box> new_unitBox_shape = std::move(unitBox_shape);

      THEN("Profile shape is unaffected")
      {
        CHECK(strict_profile->get_shape() == new_unitBox_shape);
      }
    }

    WHEN("Queue number used for profile construction is changed")
    {
      THEN("Queue number is unaffected")
      {
        //Should be true since queue_id is passed as const&
      }
    }
  }

  // Profile Function Tests
  GIVEN("Sample Profiles and Shapes")
  {
    Trajectory::ProfilePtr strict_unitbox_profile = create_test_profile(UnitBox, AgencyType::Strict);
    Trajectory::ProfilePtr queued_unitCircle_profile = create_test_profile(UnitCircle, AgencyType::Queued, "3");
    std::shared_ptr<geometry::Box> new_Box_shape = std::make_shared<geometry::Box>(2.0, 2.0);

    WHEN("Profile agency is changed using API set_to_* function")
    {
      THEN("Profile agency is successfully changed")
      {
        CHECK(strict_unitbox_profile->get_agency() == AgencyType::Strict);
        CHECK(strict_unitbox_profile->get_queue_info() == nullptr);

        strict_unitbox_profile->set_to_autonomous();
        CHECK(strict_unitbox_profile->get_agency() == AgencyType::Autonomous);
        CHECK(strict_unitbox_profile->get_queue_info() == nullptr);

        strict_unitbox_profile->set_to_queued("2");
        CHECK(strict_unitbox_profile->get_agency() == AgencyType::Queued);
        CHECK(strict_unitbox_profile->get_queue_info()->get_queue_id() == "2");

        strict_unitbox_profile->set_to_strict();
        CHECK(strict_unitbox_profile->get_agency() == AgencyType::Strict);
        CHECK(strict_unitbox_profile->get_queue_info() == nullptr);
      }
    }

    WHEN("Changing profile shapes using API set_shape function")
    {
      CHECK(strict_unitbox_profile->get_shape() != new_Box_shape);
      strict_unitbox_profile->set_shape(new_Box_shape);

      THEN("ProfilePtr is updated accordingly.")
      {
        CHECK(strict_unitbox_profile->get_shape() == new_Box_shape);
      }
    }
  }
}

SCENARIO("Segment Unit Tests")
{
  // Segment Construction and Getters
  GIVEN("Construction values for Segments")
  {
    Trajectory::ProfilePtr strict_unitbox_profile = create_test_profile(UnitBox, AgencyType::Strict);
    Trajectory::ProfilePtr queued_unitCircle_profile = create_test_profile(UnitCircle, AgencyType::Queued, "3");
    const auto time = std::chrono::steady_clock::now();
    const Eigen::Vector3d pos = Eigen::Vector3d(0, 0, 0);
    const Eigen::Vector3d vel = Eigen::Vector3d(0, 0, 0);

    WHEN("Attemping to construct Segment using Trajectory::add_segment()")
    {
      rmf_traffic::Trajectory trajectory{"test_map"};
      auto result = trajectory.insert(time, strict_unitbox_profile, pos, vel);

      Trajectory::Segment segment = *(result.it);

      THEN("Segment is constructed according to specifications.")
      {
        // From IteratorResult
        CHECK(result.inserted);
        CHECK(segment.get_finish_time() == time);
        CHECK(segment.get_finish_position() == pos);
        CHECK(segment.get_finish_velocity() == vel);
        CHECK(segment.get_profile() == strict_unitbox_profile);
      }
    }

    WHEN("Profile used for construction is changed")
    {
      rmf_traffic::Trajectory trajectory{"test_map"};
      auto result = trajectory.insert(time, strict_unitbox_profile, pos, vel);
      Trajectory::Segment segment = *(result.it);

      *strict_unitbox_profile = *queued_unitCircle_profile;

      THEN("Segment profile is still valid")
      {
        CHECK(segment.get_profile() == strict_unitbox_profile);
        // TODO: Again, we can only assume segment is updated
      }
    }

    WHEN("Pointer for profile used for construction is changed")
    {
      rmf_traffic::Trajectory trajectory{"test_map"};
      auto result = trajectory.insert(time, strict_unitbox_profile, pos, vel);
      Trajectory::Segment segment = *(result.it);

      Trajectory::ProfilePtr new_profile = std::move(strict_unitbox_profile);

      THEN("Segment profile is updated")
      {
        CHECK(segment.get_profile() != strict_unitbox_profile);
        CHECK(segment.get_profile() == new_profile);
      }
    }

    WHEN("Profile used for construction is moved")
    {
      rmf_traffic::Trajectory trajectory{"test_map"};
      auto result = trajectory.insert(time, strict_unitbox_profile, pos, vel);
      Trajectory::Segment segment = *(result.it);

      Trajectory::ProfilePtr new_profile = std::move(strict_unitbox_profile);

      THEN("Segment profile is updated")
      {
        CHECK(segment.get_profile() != strict_unitbox_profile);
        CHECK(segment.get_profile() == new_profile);
      }
    }

    WHEN("time, pos and vel parameters are changed")
    {
      THEN("Segment is unaffected")
      {
        // This should be true since all of them are pass by value
      }
    }
  }

  // Segment Functions
  GIVEN("Sample Segment")
  {
    std::vector<TrajectoryInsertInput> inputs;
    Time time = steady_clock::now();
    inputs.push_back({time, UnitBox, Eigen::Vector3d(0, 0, 0), Eigen::Vector3d(0, 0, 0)});
    inputs.push_back({time + 10s, UnitBox, Eigen::Vector3d(1, 1, 1), Eigen::Vector3d(1, 1, 1)});
    inputs.push_back({time + 20s, UnitBox, Eigen::Vector3d(2, 2, 2), Eigen::Vector3d(0, 0, 0)});
    Trajectory trajectory = create_test_trajectory(inputs);
    Trajectory::iterator trajectory_it = trajectory.begin();
    Trajectory::Segment segment = *trajectory_it;
    Trajectory::Segment segment_10s = *(trajectory_it++);

    WHEN("Setting a new profile using set_profile function")
    {
      Trajectory::ProfilePtr new_profile = create_test_profile(UnitCircle, AgencyType::Autonomous);
      segment.set_profile(new_profile);

      THEN("Profile is updated successfully.")
      {
        CHECK(segment.get_profile() == new_profile);
      }
    }

    WHEN("Setting a new finish position using set_finish function")
    {
      Eigen::Vector3d new_position = Eigen::Vector3d(1, 1, 1);
      segment.set_finish_position(new_position);

      THEN("Finish position is updated successfully.")
      {
        CHECK(segment.get_finish_position() == new_position);
      }
    }

    WHEN("Setting a new finish velocity using set_velocity function")
    {
      Eigen::Vector3d new_velocity = Eigen::Vector3d(1, 1, 1);
      segment.set_finish_velocity(new_velocity);

      THEN("Finish position is updated successfully.")
      {
        CHECK(segment.get_finish_velocity() == new_velocity);
      }
    }

    WHEN("Setting a new finish time using set_finish_time function")
    {
      Time new_time = time + 5s;
      segment.set_finish_time(new_time);

      THEN("Finish time is updated successfully.")
      {
        CHECK(segment.get_finish_time() == new_time);
      }
    }

    WHEN("Setting a new finish time that conflicts with another segment")
    {
      Time new_time = time + 10s;

      THEN("Error is thrown.")
      {
        CHECK_THROWS(segment.set_finish_time(new_time));
      }
    }

    WHEN("Setting a new finish time that causes a rearrangement of adjacent segments")
    {
      Time new_time = time + 12s;
      segment.set_finish_time(new_time);

      THEN("The appropriate segments are rearranged")
      {
        int new_order[3] = {1, 0, 2};
        int i = 0;
        for (Trajectory::iterator it = trajectory.begin(); it != trajectory.end(); it++, i++)
          CHECK(it->get_finish_position() == Eigen::Vector3d(new_order[i],
                                                             new_order[i],
                                                             new_order[i]));
      }
    }

    WHEN("Setting a new finish time that causes a rearrangement of non-adjacent segments")
    {
      Time new_time = time + 22s;
      segment.set_finish_time(new_time);

      THEN("The appropriate segments are rearranged")
      {
        int new_order[3] = {1, 2, 0};
        int i = 0;
        for (Trajectory::iterator it = trajectory.begin(); it != trajectory.end(); it++, i++)
        {
          CHECK(it->get_finish_position() == Eigen::Vector3d(new_order[i],
                                                             new_order[i],
                                                             new_order[i]));
        }
      }
    }

    WHEN("Positively adjusting all finish times using adjust_finish_times function, using first segment")
    {
      seconds delta_t = seconds(5);
      segment.adjust_finish_times(delta_t);
      int i = 0;
      Time new_order[3] = {time + 5s, time + 15s, time + 25s};

      THEN("All finish times are adjusted correctly.")
      {
        for (Trajectory::iterator it = trajectory.begin(); it != trajectory.end(); it++, i++)
        {
          CHECK(it->get_finish_time() == new_order[i]);
        }
      }
    }

    WHEN("Negatively adjusting all finish times using adjust_finish_times function, using first segment")
    {
      seconds delta_t = seconds(-5);
      segment.adjust_finish_times(delta_t);
      int i = 0;
      Time new_order[3] = {time - 5s, time + 5s, time + 15s};

      THEN("All finish times are adjusted correctly.")
      {
        for (Trajectory::iterator it = trajectory.begin(); it != trajectory.end(); it++, i++)
        {
          CHECK(it->get_finish_time() == new_order[i]);
        }
      }
    }

    WHEN("Large negative adjustment all finish times using adjust_finish_times function, using first segment")
    {
      seconds delta_t = seconds(-50);
      segment.adjust_finish_times(delta_t);
      int i = 0;
      Time new_order[3] = {time - 50s, time - 40s, time - 30s};

      THEN("All finish times are adjusted correctly, as there is no segment preceding first segment")
      {
        for (Trajectory::iterator it = trajectory.begin(); it != trajectory.end(); it++, i++)
        {
          CHECK(it->get_finish_time() == new_order[i]);
        }
      }
    }

    WHEN("Positively adjusting all finish times using adjust_finish_times function, using second segment")
    {
      seconds delta_t = seconds(5);
      segment_10s.adjust_finish_times(delta_t);
      int i = 0;

      THEN("All finish times are adjusted correctly.")
      {
        Time new_order[3] = {time + 5s, time + 15s, time + 25s};
        for (Trajectory::iterator it = trajectory.begin(); it != trajectory.end(); it++, i++)
        {
          CHECK(it->get_finish_time() == new_order[i]);
        }
      }
    }

    WHEN("Negatively adjusting all finish times using adjust_finish_times function, using second segment")
    {
      seconds delta_t = seconds(-5);
      segment_10s.adjust_finish_times(delta_t);
      int i = 0;

      THEN("All finish times are adjusted correctly.")
      {
        Time new_order[3] = {time - 5s, time + 5s, time + 15s};
        for (Trajectory::iterator it = trajectory.begin(); it != trajectory.end(); it++, i++)
        {
          CHECK(it->get_finish_time() == new_order[i]);
        }
      }
    }

    WHEN("Large negative adjustment all finish times using adjust_finish_times function, using second segment")
    {
      seconds delta_t = seconds(-50);

      THEN("std::invalid_argument exception thrown due to violation of previous segment time boundary")
      {
        // FLAG: No exception was thrown here
        // CHECK_THROWS(segment_10s.adjust_finish_times(delta_t));
      }
    }
  }
}

SCENARIO("Trajectory and base_iterator unit tests")
{
  // Trajectory construction
  GIVEN("Parameters for insert function")
  {
    Time time = steady_clock::now();
    Eigen::Vector3d pos_0 = Eigen::Vector3d(0, 0, 0);
    Eigen::Vector3d vel_0 = Eigen::Vector3d(1, 1, 1);
    Eigen::Vector3d pos_1 = Eigen::Vector3d(2, 2, 2);
    Eigen::Vector3d vel_1 = Eigen::Vector3d(3, 3, 3);
    Eigen::Vector3d pos_2 = Eigen::Vector3d(4, 4, 4);
    Eigen::Vector3d vel_2 = Eigen::Vector3d(5, 5, 5);
    std::vector<TrajectoryInsertInput> param_inputs;
    param_inputs.push_back({time, UnitBox, pos_0, vel_0});
    param_inputs.push_back({time + 10s, UnitBox, pos_1, vel_1});
    param_inputs.push_back({time + 20s, UnitBox, pos_2, vel_2});

    WHEN("Construct empty trajectory")
    {
      Trajectory trajectory("test_map");

      THEN("Empty trajectory is created.")
      {
        CHECK(trajectory.begin() == trajectory.end());
        CHECK(trajectory.end() == trajectory.end());
      }
    }

    WHEN("Construct a length 1 trajectory")
    {
      Trajectory trajectory("test_map");
      auto result = trajectory.insert(time, create_test_profile(UnitBox, AgencyType::Strict),
                                      pos_0,
                                      vel_0);
      Trajectory::iterator zeroth_it = result.it;

      THEN("Length 1 trajectory is created.")
      {
        //base_iterator tests
        CHECK(result.inserted);
        CHECK(zeroth_it == trajectory.begin());
        CHECK(trajectory.begin() != trajectory.end());
        CHECK(zeroth_it != trajectory.end());
        CHECK(zeroth_it < trajectory.end());
        CHECK(zeroth_it <= trajectory.end());
        CHECK(trajectory.end() > zeroth_it);
        CHECK(trajectory.end() >= trajectory.end());

        CHECK(pos_0 == zeroth_it->get_finish_position());
        CHECK(vel_0 == zeroth_it->get_finish_velocity());
        CHECK(time == zeroth_it->get_finish_time());
      }
    }

    WHEN("Construct a length 2 trajectory")
    {
      Trajectory trajectory("test_map");
      auto result = trajectory.insert(time, create_test_profile(UnitBox, AgencyType::Strict),
                                      pos_0,
                                      vel_0);
      Trajectory::iterator zeroth_it = result.it;
      auto result_1 = trajectory.insert(time + 10s, create_test_profile(UnitBox, AgencyType::Strict),
                                        pos_1,
                                        vel_1);
      Trajectory::iterator first_it = result_1.it;

      THEN("Length 2 trajectory is created.")
      {
        //base_iterator tests
        CHECK(first_it == ++trajectory.begin());
        CHECK(first_it != trajectory.begin());
        CHECK(first_it > trajectory.begin());
        CHECK(first_it >= trajectory.begin());
        CHECK(trajectory.begin() < first_it);
        CHECK(trajectory.begin() <= first_it);

        CHECK(first_it != zeroth_it);
        CHECK(first_it > zeroth_it);
        CHECK(first_it >= zeroth_it);
        CHECK(zeroth_it < first_it);
        CHECK(zeroth_it <= first_it);

        CHECK(first_it != trajectory.end());
        CHECK(first_it < trajectory.end());
        CHECK(first_it <= trajectory.end());
        CHECK(trajectory.end() > first_it);
        CHECK(trajectory.end() >= first_it);

        CHECK(first_it->get_finish_position() == pos_1);
        CHECK(first_it->get_finish_velocity() == vel_1);
        CHECK(first_it->get_finish_time() == time + 10s);
      }
    }

    WHEN("Copy Construction from another base_iterator")
    {
      Trajectory trajectory("test_map");
      auto result = trajectory.insert(time, create_test_profile(UnitBox, AgencyType::Strict),
                                      pos_0,
                                      vel_0);
      Trajectory::iterator zeroth_it = result.it;
      auto result_1 = trajectory.insert(time + 10s, create_test_profile(UnitBox, AgencyType::Strict),
                                        pos_1,
                                        vel_1);
      Trajectory::iterator first_it = result_1.it;

      THEN("New iterator is created")
      {
        Trajectory::iterator copied_first_it(zeroth_it);
        CHECK(&zeroth_it != &copied_first_it);
        CHECK(copied_first_it->get_profile() == zeroth_it->get_profile());
      }
    }

    WHEN("Copy Construction from rvalue base_iterator")
    {
      Trajectory trajectory("test_map");
      auto result = trajectory.insert(time, create_test_profile(UnitBox, AgencyType::Strict),
                                      pos_0,
                                      vel_0);
      Trajectory::iterator zeroth_it = result.it;
      auto result_1 = trajectory.insert(time + 10s, create_test_profile(UnitBox, AgencyType::Strict),
                                        pos_1,
                                        vel_1);
      Trajectory::iterator first_it = result_1.it;

      THEN("New iterator is created")
      {
        Trajectory::iterator &&rvalue_it = std::move(zeroth_it);
        Trajectory::iterator copied_first_it(rvalue_it);
        CHECK(&zeroth_it != &copied_first_it);
        CHECK(copied_first_it->get_profile() == zeroth_it->get_profile());
      }
    }

    WHEN("Move Construction from another base_iterator")
    {
      Trajectory trajectory("test_map");
      auto result = trajectory.insert(time, create_test_profile(UnitBox, AgencyType::Strict),
                                      pos_0,
                                      vel_0);
      Trajectory::iterator zeroth_it = result.it;
      auto result_1 = trajectory.insert(time + 10s, create_test_profile(UnitBox, AgencyType::Strict),
                                        pos_1,
                                        vel_1);
      Trajectory::iterator first_it = result_1.it;

      THEN("New iterator is created")
      {
        Trajectory::iterator copied_first_it(zeroth_it);
        Trajectory::iterator moved_first_it(std::move(copied_first_it));
        CHECK(&zeroth_it != &moved_first_it);
        CHECK(moved_first_it->get_profile() == zeroth_it->get_profile());
      }
    }

    WHEN("Copy Construction of Trajectory from another trajectory")
    {
      Trajectory trajectory = create_test_trajectory(param_inputs);
      Trajectory trajectory_copy = trajectory;

      THEN("Elements of trajectories are consistent")
      {
        Trajectory::const_iterator ot = trajectory.begin();
        Trajectory::const_iterator ct = trajectory_copy.begin();
        for (; ot != trajectory.end() && ct != trajectory.end(); ++ot, ++ct)
        {
          CHECK(ot->get_profile() == ct->get_profile());
          CHECK(ot->get_finish_position() == ct->get_finish_position());
          CHECK(ot->get_finish_velocity() == ct->get_finish_velocity());
          CHECK(ot->get_finish_time() == ct->get_finish_time());
        }
        CHECK(ot == trajectory.end());
        CHECK(ct == trajectory_copy.end());
      }
    }

    WHEN("Copy Construction of Trajectory followed by move of source trajectory")
    {
      Trajectory trajectory = create_test_trajectory(param_inputs);
      Trajectory trajectory_copy = trajectory;
      Trajectory trajectory_moved = std::move(trajectory);

      THEN("Elements of trajectories are consistent")
      {
        // FLAG: I would expect the following to throw a segfault due to std::move, but it doesn't
        // As a result, the following test might not be much different from a copy constructor.
        trajectory.get_map_name();

        Trajectory::const_iterator ct = trajectory_copy.begin();
        Trajectory::const_iterator mt = trajectory_moved.begin();
        for (; ct != trajectory_copy.end() && mt != trajectory_moved.end(); ++ct, ++mt)
        {
          CHECK(ct->get_profile() == mt->get_profile());
          CHECK(ct->get_finish_position() == mt->get_finish_position());
          CHECK(ct->get_finish_velocity() == mt->get_finish_velocity());
          CHECK(ct->get_finish_time() == mt->get_finish_time());
        }
        CHECK(ct == trajectory_copy.end());
        CHECK(mt == trajectory_moved.end());
      }
    }
  }
}
// SCENARIO("Class Profile unit tests")
// {

//   GIVEN("Checking Accessqor Functions")
//   {
//     std::shared_ptr<rmf_traffic::geometry::Box> profile_shape =
//         std::make_shared<rmf_traffic::geometry::Box>(1.0, 1.0);
//     rmf_traffic::Trajectory::ProfilePtr profile =
//         rmf_traffic::Trajectory::Profile::make_strict(profile_shape);

//     WHEN("Initial Configuration")
//     {
//       REQUIRE(profile->get_agency() ==
//               rmf_traffic::Trajectory::Profile::Agency::Strict);
//       REQUIRE(profile->get_shape() == profile_shape);
//     }

//     WHEN("Change Agency to Autonomous")
//     {
//       profile->set_to_autonomous();
//       CHECK(profile->get_agency() ==
//             rmf_traffic::Trajectory::Profile::Agency::Autonomous);
//     }

//     WHEN("Change Agency to Queued")
//     {
//       //TODO: Should QueueID be string?
//       const std::string queue_id = "1";
//       profile->set_to_queued(queue_id);
//       CHECK(profile->get_queue_info()->get_queue_id() == queue_id);
//     }

//     WHEN("Change Shape to Unit Circle")
//     {q
//       std::shared_ptr<rmf_traffic::geometry::Circle> new_profile_shape =
//           std::make_shared<rmf_traffic::geometry::Circle>(1.0);
//       profile->set_shape(new_profile_shape);

//       CHECK(profile->get_agency() ==
//             rmf_traffic::Trajectory::Profile::Agency::Strict);
//       CHECK(profile->get_shape() == new_profile_shape);
//     }
//   }q

//   GIVEN("Checking Dirty Input")

//   {
//     rmf_traffic::Trajectory::ProfilePtr profile = make_test_profile(UnitBox);

//     WHEN("Giving nullptr as queue_id is not allowed")
//     {
//       CHECK_THROWS(profile->set_to_queued(nullptr));
//     }
//   }
// }

// SCENARIO("base_iterator unit tests")
// {
//   GIVEN("Testing operators")
//   {
//     using namespace std::chrono_literals;

//     rmf_traffic::Trajectory trajectory{"test_map"};
//     REQUIRE(trajectory.begin() == trajectory.end());
//     REQUIRE(trajectory.end() == trajectory.end());

//     const auto finish_time = std::chrono::steady_clock::now();
//     const auto profile = make_test_profile(UnitBox);
//     const Eigen::Vector3d final_pos = Eigen::Vector3d(1, 1, 1);
//     const Eigen::Vector3d final_vel = Eigen::Vector3d(1, 1, 1);

//     auto result = trajectory.insert(finish_time, profile, final_pos, final_vel);
//     rmf_traffic::Trajectory::iterator zeroth_it = result.it;
//     REQUIRE(result.inserted);
//     REQUIRE(trajectory.begin() == zeroth_it);

//     const auto finish_time_2 = std::chrono::steady_clock::now() + 10s;
//     const auto profile_2 = make_test_profile(UnitBox);
//     const Eigen::Vector3d begin_pos_2 = Eigen::Vector3d(2, 0, 3);
//     const Eigen::Vector3d begin_vel_2 = Eigen::Vector3d(2, 0, 3);

//     auto result_1 = trajectory.insert(finish_time_2, profile_2, begin_pos_2, begin_vel_2);
//     rmf_traffic::Trajectory::iterator first_it = result_1.it;
//     REQUIRE(result_1.inserted);
//     REQUIRE(--trajectory.end() == first_it); // trajectory.end() is a placeholder "beyond" the last element

//     WHEN("Doing Comparisons")
//     {
//       CHECK((*zeroth_it).get_profile() == profile);
//       CHECK(zeroth_it->get_profile() == profile);
//       CHECK(zeroth_it == trajectory.begin());
//       CHECK(trajectory.begin() != trajectory.end());
//       CHECK(zeroth_it != trajectory.end());
//       CHECK(zeroth_it < trajectory.end());
//       CHECK(zeroth_it <= trajectory.end());
//       CHECK(trajectory.end() > zeroth_it);
//       CHECK(trajectory.end() >= trajectory.end());
//     }

//     WHEN("Doing Increment/Decrements")
//     {
//       zeroth_it++;
//       CHECK(zeroth_it == first_it);
//       zeroth_it--;
//       CHECK(zeroth_it != first_it);
//       CHECK(zeroth_it < first_it);
//     }

//     WHEN("Copy Constructing from another base_iterator")
//     {
//       const rmf_traffic::Trajectory::iterator copied_first_it(zeroth_it);
//       CHECK(&zeroth_it != &copied_first_it);
//       CHECK(copied_first_it->get_profile() == zeroth_it->get_profile());
//     }

//     WHEN("Copy Constructing from an rvalue base_iterator")
//     {
//       rmf_traffic::Trajectory::iterator &&rvalue_it = std::move(zeroth_it);
//       const rmf_traffic::Trajectory::iterator copied_first_it(rvalue_it);
//       CHECK(&zeroth_it != &copied_first_it);
//       CHECK(copied_first_it->get_profile() == zeroth_it->get_profile());
//     }

//     WHEN("Moving from another base_iterator")
//     {
//       rmf_traffic::Trajectory::iterator copied_first_it(zeroth_it);
//       const rmf_traffic::Trajectory::iterator moved_first_it(std::move(copied_first_it));
//       CHECK(&zeroth_it != &moved_first_it);
//       CHECK(moved_first_it->get_profile() == zeroth_it->get_profile());
//     }

//     WHEN("Moving from an rvalue base_iterator")
//     {
//       rmf_traffic::Trajectory::iterator copied_first_it(zeroth_it);
//       rmf_traffic::Trajectory::iterator &&moved_first_it(std::move(copied_first_it));
//       CHECK(&zeroth_it != &moved_first_it);
//       CHECK(moved_first_it->get_profile() == zeroth_it->get_profile());
//     }
//   }
// }

// SCENARIO("Class Segment unit tests")
// {

//   GIVEN("Testing accessor functions")
//   {
//     using namespace std::chrono_literals;

//     rmf_traffic::Trajectory trajectory{"test_map"};
//     REQUIRE(trajectory.begin() == trajectory.end());
//     REQUIRE(trajectory.end() == trajectory.end());

//     const auto finish_time = std::chrono::steady_clock::now();
//     const auto profile = make_test_profile(UnitBox);
//     const Eigen::Vector3d final_pos = Eigen::Vector3d(0, 0, 0);
//     const Eigen::Vector3d final_vel = Eigen::Vector3d(0, 0, 0);

//     auto result = trajectory.insert(finish_time, profile, final_pos, final_vel);
//     REQUIRE(result.inserted);

//     rmf_traffic::Trajectory::Segment segment = *trajectory.find(finish_time);

//     WHEN("Initial Configuration")
//     {
//       REQUIRE(segment.get_profile() == profile);
//       REQUIRE(segment.get_finish_position() == final_pos);
//       REQUIRE(segment.get_finish_velocity() == final_vel);
//       REQUIRE(segment.get_finish_time() == finish_time);
//     }

//     WHEN("Setting a new profile")
//     {
//       const auto new_profile = make_test_profile(UnitCircle);
//       segment.set_profile(new_profile);
//       CHECK(segment.get_profile() == new_profile);
//       CHECK(segment.get_profile() != profile);
//     }

//     WHEN("Mutating current profile")
//     {
//       profile->set_to_autonomous();
//       CHECK(segment.get_profile()->get_agency() ==
//             rmf_traffic::Trajectory::Profile::Agency::Autonomous);

//       const auto new_shape = std::make_shared<rmf_traffic::geometry::Circle>(1.0);
//       profile->set_shape(new_shape);
//       CHECK(segment.get_profile()->get_shape() == new_shape);
//     }

//     // TODO: The Docs record this as a 2D homogenous position, should be 3D
//     WHEN("Setting a new position")
//     {
//       const Eigen::Vector3d new_pos = Eigen::Vector3d(1, 1, 1);
//       segment.set_finish_position(new_pos);
//       CHECK(segment.get_finish_position() == new_pos);
//       CHECK(segment.get_finish_position() != final_pos);
//     }

//     // TODO: The Docs record this as a 2D homogenous position, should be 3D
//     WHEN("Setting a new velocity")
//     {
//       const Eigen::Vector3d new_vel = Eigen::Vector3d(1, 1, 1);
//       segment.set_finish_velocity(new_vel);
//       CHECK(segment.get_finish_velocity() == new_vel);
//       CHECK(segment.get_finish_velocity() != final_vel);
//     }

//     WHEN("Setting a finish time")
//     {
//       const auto new_finish_time = std::chrono::steady_clock::now() + 15s;
//       segment.set_finish_time(new_finish_time);
//       CHECK(segment.get_finish_time() == new_finish_time);
//       CHECK(segment.get_finish_time() != finish_time);
//     }
//   }

//   GIVEN("Test automatic reordering when setting finish times")
//   {
//     using namespace std::chrono_literals;

//     rmf_traffic::Trajectory trajectory{"test_map"};
//     REQUIRE(trajectory.begin() == trajectory.end());
//     REQUIRE(trajectory.end() == trajectory.end());

//     const auto finish_time = std::chrono::steady_clock::now();
//     const auto profile = make_test_profile(UnitBox);
//     const Eigen::Vector3d final_pos = Eigen::Vector3d(1, 1, 1);
//     const Eigen::Vector3d final_vel = Eigen::Vector3d(1, 1, 1);

//     auto result = trajectory.insert(finish_time, profile, final_pos, final_vel);
//     const rmf_traffic::Trajectory::iterator zeroth_it = result.it;
//     REQUIRE(result.inserted);

//     const auto finish_time_2 = std::chrono::steady_clock::now() + 10s;
//     const auto profile_2 = make_test_profile(UnitBox);
//     const Eigen::Vector3d begin_pos_2 = Eigen::Vector3d(2, 0, 3);
//     const Eigen::Vector3d begin_vel_2 = Eigen::Vector3d(2, 0, 3);

//     auto result_1 = trajectory.insert(finish_time_2, profile_2, begin_pos_2, begin_vel_2);
//     const rmf_traffic::Trajectory::iterator first_it = result_1.it;
//     REQUIRE(result_1.inserted);

//     const auto finish_time_3 = std::chrono::steady_clock::now() + 20s;
//     const auto profile_3 = make_test_profile(UnitBox);
//     const Eigen::Vector3d begin_pos_3 = Eigen::Vector3d(4, 2, 6);
//     const Eigen::Vector3d begin_vel_3 = Eigen::Vector3d(6, 2, 4);

//     auto result_3 = trajectory.insert(finish_time_3, profile_3, begin_pos_3, begin_vel_3);
//     const rmf_traffic::Trajectory::iterator third_it = result_3.it;
//     REQUIRE(result_3.inserted);

//     REQUIRE(trajectory.begin() == zeroth_it);
//     REQUIRE(zeroth_it < first_it);
//     REQUIRE(first_it < third_it);

//     WHEN("Single forward time shift for one positional swap")
//     {
//       const auto new_finish_time = finish_time + 15s;
//       zeroth_it->set_finish_time(new_finish_time);
//       CHECK(first_it < zeroth_it);
//       CHECK(zeroth_it < third_it);
//     }

//     WHEN("Single forward time shift for two positional swap")
//     {
//       const auto new_finish_time = finish_time + 25s;
//       zeroth_it->set_finish_time(new_finish_time);
//       CHECK(first_it < third_it);
//       CHECK(third_it < zeroth_it);
//     }

//     WHEN("Single backward time shift for one positional swap")
//     {
//       const auto new_finish_time = finish_time_3 - 15s;
//       third_it->set_finish_time(new_finish_time);
//       CHECK(zeroth_it < third_it);
//       CHECK(third_it < first_it);
//     }

//     WHEN("Single backward time shift for two positional swap")
//     {
//       const auto new_finish_time = finish_time_3 - 25s;
//       third_it->set_finish_time(new_finish_time);

//       CHECK(rmf_traffic::Trajectory::Debug::check_iterator_time_consistency(
//           trajectory, true));

//       CHECK(third_it < zeroth_it);
//       CHECK(zeroth_it < first_it);
//     }

//     WHEN("Forward time shift with time conflict")
//     {
//       CHECK_THROWS(zeroth_it->set_finish_time(finish_time_2));
//     }

//     WHEN("Backward time shift with time conflict")
//     {
//       CHECK_THROWS(third_it->set_finish_time(finish_time_2));
//     }

//     WHEN("Adding 0s across all segments")
//     {
//       zeroth_it->adjust_finish_times(0s);

//       CHECK(rmf_traffic::Trajectory::Debug::check_iterator_time_consistency(
//           trajectory, true));

//       CHECK(zeroth_it->get_finish_time() == finish_time);
//       CHECK(first_it->get_finish_time() == finish_time_2);
//       CHECK(third_it->get_finish_time() == finish_time_3);
//     }

//     WHEN("Adding +2s across all segments")
//     {
//       zeroth_it->adjust_finish_times(2s);

//       CHECK(rmf_traffic::Trajectory::Debug::check_iterator_time_consistency(
//           trajectory, true));

//       CHECK(zeroth_it->get_finish_time() == finish_time + 2s);
//       CHECK(first_it->get_finish_time() == finish_time_2 + 2s);
//       CHECK(third_it->get_finish_time() == finish_time_3 + 2s);
//     }

//     WHEN("Adding -2s across all segments")
//     {
//       zeroth_it->adjust_finish_times(-2s);

//       CHECK(rmf_traffic::Trajectory::Debug::check_iterator_time_consistency(
//           trajectory, true));

//       CHECK(zeroth_it->get_finish_time() == finish_time - 2s);
//       CHECK(first_it->get_finish_time() == finish_time_2 - 2s);
//       CHECK(third_it->get_finish_time() == finish_time_3 - 2s);
//     }

//     WHEN("Time Invariance when +2 followed by -2")
//     {
//       zeroth_it->adjust_finish_times(2s);
//       zeroth_it->adjust_finish_times(-2s);

//       CHECK(rmf_traffic::Trajectory::Debug::check_iterator_time_consistency(
//           trajectory, true));

//       CHECK(zeroth_it->get_finish_time() == finish_time);
//       CHECK(first_it->get_finish_time() == finish_time_2);
//       CHECK(third_it->get_finish_time() == finish_time_3);
//     }
//   }
// }

// SCENARIO("Class Trajectory unit tests")
// {
//   GIVEN("Checking Insertion, Deletion and Finding")
//   {
//     using namespace std::chrono_literals;
//     rmf_traffic::Trajectory trajectory("test_map");
//     REQUIRE(trajectory.begin() == trajectory.end());
//     REQUIRE(trajectory.end() == trajectory.end());
//     REQUIRE(trajectory.duration() == 0s);
//     REQUIRE(trajectory.start_time() == nullptr);
//     REQUIRE(trajectory.finish_time() == nullptr);

//     WHEN("Inserting 1 new segment to an empty trajectory")
//     {
//       const auto finish_time = std::chrono::steady_clock::now();
//       const auto profile = make_test_profile(UnitBox);
//       const Eigen::Vector3d final_pos = Eigen::Vector3d(1, 1, 1);
//       const Eigen::Vector3d final_vel = Eigen::Vector3d(1, 1, 1);
//       auto result = trajectory.insert(finish_time, profile, final_pos, final_vel);

//       CHECK(trajectory.duration() == 0s);
//       CHECK(trajectory.size() == 1);
//       CHECK(*trajectory.start_time() == finish_time);
//       CHECK(*trajectory.finish_time() == finish_time);
//       CHECK(result.it->get_profile() == profile);
//       CHECK(result.it->get_finish_position() == final_pos);
//       CHECK(result.it->get_finish_velocity() == final_vel);
//     }

//     WHEN("Removing 1 segment from a length 1 trajectory")
//     {
//       trajectory = make_test_trajectory(std::chrono::steady_clock::now(), 1, 5);
//       auto it = trajectory.begin();
//       auto erased_it = trajectory.erase(it);
//       CHECK(trajectory.size() == 0);
//       CHECK(erased_it == trajectory.end());
//     }

//     // TODO
//     // WHEN("Removing 1 segment from a length 2 trajectory")
//     // {
//     //   trajectory = make_test_trajectory(std::chrono::steady_clock::now(), 2, 5);
//     //   auto it = trajectory.begin();
//     //   auto next_it = it++;
//     //   auto erased_it = trajectory.erase(it);
//     //   CHECK(trajectory.size() == 1);
//     //   CHECK(erased_it == next_it++);
//     // }

//     WHEN("Finding the first segment in a length 1 trajectory")
//     {
//       const auto finish_time = std::chrono::steady_clock::now();
//       trajectory = make_test_trajectory(finish_time, 1, 5);
//       rmf_traffic::Trajectory::iterator it = trajectory.find(finish_time);
//       CHECK(trajectory.begin() == it);
//     }

//     WHEN("Finding a segment after a length 1 trajectory finishes")
//     {
//       const auto finish_time = std::chrono::steady_clock::now();
//       trajectory = make_test_trajectory(finish_time, 1, 5);
//       rmf_traffic::Trajectory::iterator it = trajectory.find(finish_time + 10s);
//       CHECK(trajectory.end() == it);
//     }

//     // CHECK(trajectory.begin() == it) seems to pass
//     // WHEN("Find a segment before a length 1 trajectory begins")
//     // {
//     //   const auto finish_time = std::chrono::steady_clock::now();
//     //   trajectory = make_test_trajectory(finish_time, 1, 5);
//     //   rmf_traffic::Trajectory::iterator it = trajectory.find(finish_time - 10s);
//     //   CHECK(trajectory.end() == it);
//     // }

//     WHEN("Finding the first segment in a length 2 trajectory at the registered time")
//     {
//       const auto finish_time = std::chrono::steady_clock::now();
//       trajectory = make_test_trajectory(finish_time, 2, 5);
//       rmf_traffic::Trajectory::iterator it = trajectory.find(finish_time);
//       CHECK(trajectory.begin() == it);
//     }

//     WHEN("Finding the first segment in a length 2 trajectory at an offset time")
//     {
//       const auto finish_time = std::chrono::steady_clock::now();
//       trajectory = make_test_trajectory(finish_time, 2, 5);
//       rmf_traffic::Trajectory::iterator it = trajectory.find(finish_time - 2s);
//       CHECK(trajectory.begin() == it);
//     }

//     WHEN("Finding the second segment in a length 2 trajectory at an offset time")
//     {
//       const auto finish_time = std::chrono::steady_clock::now();
//       trajectory = make_test_trajectory(finish_time, 2, 5);
//       rmf_traffic::Trajectory::iterator it = trajectory.find(finish_time + 2s);
//       rmf_traffic::Trajectory::iterator trajectory_it = trajectory.begin();
//       trajectory_it++;
//       CHECK(trajectory_it == it);
//     }

//     WHEN("Finding a segment after a length 2 trajectory finishes")
//     {
//       const auto finish_time = std::chrono::steady_clock::now();
//       trajectory = make_test_trajectory(finish_time, 2, 5);
//       rmf_traffic::Trajectory::iterator it = trajectory.find(finish_time + 20s);
//       CHECK(trajectory.end() == it);
//     }

//     // CHECK(trajectory.begin() == it); seems to pass
//     // WHEN("Find a segment before a length 2 trajectory begins")
//     // {
//     //   const auto finish_time = std::chrono::steady_clock::now();
//     //   trajectory = make_test_trajectory(finish_time, 2, 5);
//     //   rmf_traffic::Trajectory::iterator it = trajectory.find(finish_time - 20s);
//     //   CHECK(trajectory.end() == it);
//     // }

//     WHEN("Find all segments in a length 10 trajectory at their registered times")
//     {
//       const auto finish_time = std::chrono::steady_clock::now();
//       trajectory = make_test_trajectory(finish_time, 10, 5);
//       rmf_traffic::Trajectory::iterator trajectory_it = trajectory.begin();
//       for (int i = 0; i < 10; i++, trajectory_it++)
//       {
//         rmf_traffic::Trajectory::iterator it = trajectory.find(finish_time + std::chrono::seconds(i * 5));
//         CHECK(trajectory_it == it);
//       }
//     }

//     WHEN("Find all segments in a length 10 trajectory at their registered times plus offset")
//     {
//       const auto finish_time = std::chrono::steady_clock::now();
//       trajectory = make_test_trajectory(finish_time, 10, 5);
//       rmf_traffic::Trajectory::iterator trajectory_it = trajectory.begin();
//       trajectory_it++;
//       for (int i = 1; trajectory_it != trajectory.end(); i++, trajectory_it++) // Skipping first offset which is out of bounds
//       {
//         rmf_traffic::Trajectory::iterator it = trajectory.find(finish_time + std::chrono::seconds((i * 5)-2));
//         CHECK(trajectory_it == it);
//       }
//     }

//     // WHEN("Find all segments in a length 10 trajectory after 1 segment is erased")
//     // {
//     //   const auto finish_time = std::chrono::steady_clock::now();
//     //   trajectory = make_test_trajectory(finish_time, 10, 5);
//     //   rmf_traffic::Trajectory::iterator trajectory_it = trajectory.begin();
//     //   trajectory_it++;

//     //   // Delete
//     //   trajectory.erase()
//     //   for (int i = 1; trajectory_it != trajectory.end(); i++, trajectory_it++) // Skipping first offset which is out of bounds
//     //   {
//     //     rmf_traffic::Trajectory::iterator it = trajectory.find(finish_time + std::chrono::seconds((i * 5)-2));
//     //     CHECK(trajectory_it == it);
//     //   }
//     // }
//   }
// }

// TEST_CASE("Construct a Trajectory")
// {
//   using namespace std::chrono_literals;

//   rmf_traffic::Trajectory trajectory{"test_map"};
//   CHECK(trajectory.begin() == trajectory.end());
//   CHECK(trajectory.end() == trajectory.end());

//   const auto profile = make_test_profile();

//   const auto finish_time = std::chrono::steady_clock::now();
//   const Eigen::Vector3d begin_p = Eigen::Vector3d(0, 0, 0);
//   const Eigen::Vector3d begin_v = Eigen::Vector3d(0, 0, 0);

//   auto result = trajectory.insert(
//         finish_time, profile, begin_p, begin_v);

//   const rmf_traffic::Trajectory::iterator zeroth_it = result.it;
//   CHECK(result.inserted);

//   CHECK(zeroth_it == trajectory.begin());
//   CHECK(trajectory.begin() != trajectory.end());
//   CHECK(zeroth_it != trajectory.end());
//   CHECK(zeroth_it < trajectory.end());
//   CHECK(zeroth_it <= trajectory.end());
//   CHECK(trajectory.end() > zeroth_it);
//   CHECK(trajectory.end() >= trajectory.end());

//   CHECK(begin_p == zeroth_it->get_finish_position());
//   CHECK(begin_v == zeroth_it->get_finish_velocity());
//   CHECK(finish_time == zeroth_it->get_finish_time());

//   const auto second_time = finish_time + 10s;
//   const Eigen::Vector3d second_p = Eigen::Vector3d(1, 2, 3);
//   const Eigen::Vector3d second_v = Eigen::Vector3d(3, 2, 1);

//   result = trajectory.insert(
//         second_time, profile, second_p, second_v);

//   const rmf_traffic::Trajectory::iterator first_it = result.it;
//   CHECK(result.inserted);

//   CHECK(first_it == ++trajectory.begin());
//   CHECK(first_it != trajectory.begin());
//   CHECK(first_it > trajectory.begin());
//   CHECK(first_it >= trajectory.begin());
//   CHECK(trajectory.begin() < first_it);
//   CHECK(trajectory.begin() <= first_it);

//   CHECK(first_it != zeroth_it);
//   CHECK(first_it > zeroth_it);
//   CHECK(first_it >= zeroth_it);
//   CHECK(zeroth_it < first_it);
//   CHECK(zeroth_it <= first_it);

//   CHECK(first_it != trajectory.end());
//   CHECK(first_it < trajectory.end());
//   CHECK(first_it <= trajectory.end());
//   CHECK(trajectory.end() > first_it);
//   CHECK(trajectory.end() >= first_it);

//   CHECK(first_it->get_finish_position() == second_p);
//   CHECK(first_it->get_finish_velocity() == second_v);
//   CHECK(first_it->get_finish_time() == second_time);
// }

// TEST_CASE("Copy and move a trajectory")
// {
//   using namespace std::chrono_literals;

//   rmf_traffic::Trajectory trajectory{"test_map"};

//   const auto finish_time = std::chrono::steady_clock::now();

//   trajectory.insert(
//         finish_time, make_test_profile(),
//         Eigen::Vector3d::UnitX(),
//         Eigen::Vector3d::UnitX());

//   trajectory.insert(
//         finish_time + 10s, make_test_profile(),
//         Eigen::Vector3d::UnitY(),
//         Eigen::Vector3d::UnitY());

//   trajectory.insert(
//         finish_time + 15s, make_test_profile(),
//         Eigen::Vector3d::UnitZ(),
//         Eigen::Vector3d::UnitZ());

//   rmf_traffic::Trajectory copy = trajectory;

//   rmf_traffic::Trajectory::const_iterator ot = trajectory.begin();
//   rmf_traffic::Trajectory::const_iterator ct = copy.begin();
//   for( ; ot != trajectory.end() && ct != trajectory.end(); ++ot, ++ct)
//   {
//     CHECK(ot->get_profile() == ct->get_profile());
//     CHECK(ot->get_finish_position() == ct->get_finish_position());
//     CHECK(ot->get_finish_velocity() == ct->get_finish_velocity());
//     CHECK(ot->get_finish_time() == ct->get_finish_time());
//   }
//   CHECK(ot == trajectory.end());
//   CHECK(ct == copy.end());

//   for(auto it = copy.begin(); it != copy.end(); ++it)
//   {
//     it->set_profile(make_test_profile());
//     it->set_finish_position(it->get_finish_position() + Eigen::Vector3d::UnitZ());
//     it->set_finish_velocity(it->get_finish_velocity() + Eigen::Vector3d::UnitZ());
//     it->set_finish_time(it->get_finish_time() + 2s);
//   }

//   ot = trajectory.begin();
//   ct = copy.begin();
//   for( ; ot != trajectory.end() && ct != trajectory.end(); ++ot, ++ct)
//   {
//     CHECK(ot->get_profile() != ct->get_profile());
//     CHECK(ot->get_finish_position() != ct->get_finish_position());
//     CHECK(ot->get_finish_velocity() != ct->get_finish_velocity());
//     CHECK(ot->get_finish_time() != ct->get_finish_time());
//   }
//   CHECK(ot == trajectory.end());
//   CHECK(ct == copy.end());

//   // Copy again
//   copy = trajectory;

//   // Now move the original
//   rmf_traffic::Trajectory moved = std::move(trajectory);

//   ct = copy.begin();
//   rmf_traffic::Trajectory::const_iterator mt = moved.begin();
//   for( ; ct != copy.end() && mt != moved.end(); ++ct, ++mt)
//   {
//     CHECK(ct->get_profile() == mt->get_profile());
//     CHECK(ct->get_finish_position() == mt->get_finish_position());
//     CHECK(ct->get_finish_velocity() == mt->get_finish_velocity());
//     CHECK(ct->get_finish_time() == mt->get_finish_time());
//   }
//   CHECK(ct == copy.end());
//   CHECK(mt == moved.end());
// }
