// Copyright (c) 2014 The Chromium OS Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <gtest/gtest.h>
#include <list>
#include <map>
#include <set>
#include <string>

#include <base/strings/stringprintf.h>
#include <base/time/time.h>

#include "update_engine/policy_manager/boxed_value.h"
#include "update_engine/policy_manager/pmtest_utils.h"
#include "update_engine/policy_manager/shill_provider.h"
#include "update_engine/policy_manager/updater_provider.h"

using base::Time;
using base::TimeDelta;
using std::set;
using std::list;
using std::map;
using std::string;

namespace chromeos_policy_manager {

// The DeleterMarker flags a bool variable when the class is destroyed.
class DeleterMarker {
 public:
  DeleterMarker(bool* marker) : marker_(marker) { *marker_ = false; }

  ~DeleterMarker() { *marker_ = true; }

 private:
  friend string BoxedValue::ValuePrinter<DeleterMarker>(const void *);

  // Pointer to the bool marker.
  bool* marker_;
};

template<>
string BoxedValue::ValuePrinter<DeleterMarker>(const void *value) {
  const DeleterMarker* val = reinterpret_cast<const DeleterMarker*>(value);
  return base::StringPrintf("DeleterMarker:%s",
                            *val->marker_ ? "true" : "false");
}

TEST(PmBoxedValueTest, Deleted) {
  bool marker = true;
  const DeleterMarker* deleter_marker = new DeleterMarker(&marker);

  EXPECT_FALSE(marker);
  BoxedValue* box = new BoxedValue(deleter_marker);
  EXPECT_FALSE(marker);
  delete box;
  EXPECT_TRUE(marker);
}

TEST(PmBoxedValueTest, MoveConstructor) {
  bool marker = true;
  const DeleterMarker* deleter_marker = new DeleterMarker(&marker);

  BoxedValue* box = new BoxedValue(deleter_marker);
  BoxedValue* new_box = new BoxedValue(std::move(*box));
  // box is now undefined but valid.
  delete box;
  EXPECT_FALSE(marker);
  // The deleter_marker gets deleted at this point.
  delete new_box;
  EXPECT_TRUE(marker);
}

TEST(PmBoxedValueTest, MixedList) {
  list<BoxedValue> lst;
  // This is mostly a compile test.
  lst.emplace_back(new const int(42));
  lst.emplace_back(new const string("Hello world!"));
  bool marker;
  lst.emplace_back(new const DeleterMarker(&marker));
  EXPECT_FALSE(marker);
  lst.clear();
  EXPECT_TRUE(marker);
}

TEST(PmBoxedValueTest, MixedMap) {
  map<int, BoxedValue> m;
  m.emplace(42, std::move(BoxedValue(new const string("Hola mundo!"))));

  auto it = m.find(42);
  ASSERT_NE(it, m.end());
  PMTEST_EXPECT_NOT_NULL(it->second.value());
  PMTEST_EXPECT_NULL(m[33].value());
}

TEST(PmBoxedValueTest, StringToString) {
  EXPECT_EQ("Hej Verden!",
            BoxedValue(new string("Hej Verden!")).ToString());
}

TEST(PmBoxedValueTest, IntToString) {
  EXPECT_EQ("42", BoxedValue(new int(42)).ToString());
}

TEST(PmBoxedValueTest, UnsignedIntToString) {
  // 4294967295 is the biggest possible 32-bit unsigned integer.
  EXPECT_EQ("4294967295", BoxedValue(new unsigned int(4294967295)).ToString());
}

TEST(PmBoxedValueTest, UnsignedLongToString) {
  EXPECT_EQ("4294967295", BoxedValue(new unsigned long(4294967295)).ToString());
}

TEST(PmBoxedValueTest, UnsignedLongLongToString) {
  // 18446744073709551615 is the biggest possible 64-bit unsigned integer.
  EXPECT_EQ("18446744073709551615", BoxedValue(
      new unsigned long long(18446744073709551615ULL)).ToString());
}

TEST(PmBoxedValueTest, BoolToString) {
  EXPECT_EQ("false", BoxedValue(new bool(false)).ToString());
  EXPECT_EQ("true", BoxedValue(new bool(true)).ToString());
}

TEST(PmBoxedValueTest, DoubleToString) {
  EXPECT_EQ("1.501", BoxedValue(new double(1.501)).ToString());
}

TEST(PmBoxedValueTest, TimeToString) {
  // Tue Apr 29 22:30:55 UTC 2014 is 1398810655 seconds since the Unix Epoch.
  EXPECT_EQ("4/29/2014 22:30:55 GMT",
            BoxedValue(new Time(Time::FromTimeT(1398810655))).ToString());
}

TEST(PmBoxedValueTest, TimeDeltaToString) {
  // 12345 seconds is 3 hours, 25 minutes and 45 seconds.
  EXPECT_EQ("3h25m45s",
            BoxedValue(new TimeDelta(TimeDelta::FromSeconds(12345)))
            .ToString());
}

TEST(PmBoxedValueTest, ConnectionTypeToString) {
  EXPECT_EQ("Ethernet",
            BoxedValue(new ConnectionType(ConnectionType::kEthernet))
            .ToString());
  EXPECT_EQ("Wifi",
            BoxedValue(new ConnectionType(ConnectionType::kWifi)).ToString());
  EXPECT_EQ("Wimax",
            BoxedValue(new ConnectionType(ConnectionType::kWimax)).ToString());
  EXPECT_EQ("Bluetooth",
            BoxedValue(new ConnectionType(ConnectionType::kBluetooth))
            .ToString());
  EXPECT_EQ("Cellular",
            BoxedValue(new ConnectionType(ConnectionType::kCellular))
            .ToString());
  EXPECT_EQ("Unknown",
            BoxedValue(new ConnectionType(ConnectionType::kUnknown))
            .ToString());
}

TEST(PmBoxedValueTest, ConnectionTetheringToString) {
  EXPECT_EQ("Not Detected",
            BoxedValue(new ConnectionTethering(
                ConnectionTethering::kNotDetected)).ToString());
  EXPECT_EQ("Suspected",
            BoxedValue(new ConnectionTethering(ConnectionTethering::kSuspected))
            .ToString());
  EXPECT_EQ("Confirmed",
            BoxedValue(new ConnectionTethering(ConnectionTethering::kConfirmed))
            .ToString());
  EXPECT_EQ("Unknown",
            BoxedValue(new ConnectionTethering(ConnectionTethering::kUnknown))
            .ToString());
}

TEST(PmBoxedValueTest, SetConnectionTypeToString) {
  set<ConnectionType>* set1 = new set<ConnectionType>;
  set1->insert(ConnectionType::kWimax);
  set1->insert(ConnectionType::kEthernet);
  EXPECT_EQ("Ethernet,Wimax", BoxedValue(set1).ToString());

  set<ConnectionType>* set2 = new set<ConnectionType>;
  set2->insert(ConnectionType::kWifi);
  EXPECT_EQ("Wifi", BoxedValue(set2).ToString());
}

TEST(PmBoxedValueTest, StageToString) {
  EXPECT_EQ("Idle",
            BoxedValue(new Stage(Stage::kIdle)).ToString());
  EXPECT_EQ("Checking For Update",
            BoxedValue(new Stage(Stage::kCheckingForUpdate)).ToString());
  EXPECT_EQ("Update Available",
            BoxedValue(new Stage(Stage::kUpdateAvailable)).ToString());
  EXPECT_EQ("Downloading",
            BoxedValue(new Stage(Stage::kDownloading)).ToString());
  EXPECT_EQ("Verifying",
            BoxedValue(new Stage(Stage::kVerifying)).ToString());
  EXPECT_EQ("Finalizing",
            BoxedValue(new Stage(Stage::kFinalizing)).ToString());
  EXPECT_EQ("Updated, Need Reboot",
            BoxedValue(new Stage(Stage::kUpdatedNeedReboot)).ToString());
  EXPECT_EQ("Reporting Error Event",
            BoxedValue(new Stage(Stage::kReportingErrorEvent)).ToString());
  EXPECT_EQ("Attempting Rollback",
            BoxedValue(new Stage(Stage::kAttemptingRollback)).ToString());
}

TEST(PmBoxedValueTest, DeleterMarkerToString) {
  bool marker = false;
  BoxedValue value = BoxedValue(new DeleterMarker(&marker));
  EXPECT_EQ("DeleterMarker:false", value.ToString());
  marker = true;
  EXPECT_EQ("DeleterMarker:true", value.ToString());
}

}  // namespace chromeos_policy_manager
