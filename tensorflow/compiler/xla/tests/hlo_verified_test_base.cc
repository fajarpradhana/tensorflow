/* Copyright 2017 The TensorFlow Authors. All Rights Reserved.

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

    http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.
==============================================================================*/

#include "tensorflow/compiler/xla/tests/hlo_verified_test_base.h"

#include "absl/memory/memory.h"
#include "tensorflow/compiler/xla/service/hlo_parser.h"
#include "tensorflow/compiler/xla/service/hlo_verifier.h"
#include "tensorflow/compiler/xla/shape_util.h"
#include "tensorflow/compiler/xla/status_macros.h"
#include "tensorflow/core/platform/logging.h"
#include "tensorflow/core/platform/test.h"

namespace xla {

HloVerifiedTestBase::HloVerifiedTestBase(bool layout_sensitive,
                                         bool allow_mixed_precision)
    : HloTestBase(
          /*verifier_layout_sensitive=*/layout_sensitive,
          /*allow_mixed_precision_in_hlo_verifier=*/allow_mixed_precision) {}

HloVerifiedTestBase::~HloVerifiedTestBase() {
  // We can't call the ASSERT or EXPECT test macros in destructors, so we
  // perform HLO verification in TearDown, and use the CHECK here to ensure
  // users don't accidentally override the verification.
  CHECK(tear_down_called_)
      << "TearDown was never called; subclasses of HloVerifiedTestBase that "
      << "override TearDown must call the superclass TearDown.";
}

void HloVerifiedTestBase::TearDown() {
  EXPECT_FALSE(tear_down_called_)
      << "TearDown called more than once; it should be called exactly once.";
  tear_down_called_ = true;
  if (module_) {
    VerifyModule(module_.get());
  }
  for (int i = 0; i < modules_.size(); ++i) {
    VerifyModule(modules_.at(i).get());
  }
  HloTestBase::TearDown();
}

void HloVerifiedTestBase::VerifyModule(HloModule* module) {
  xla::StatusOr<bool> mutated = verifier().Run(module);
  if (!mutated.ok()) {
    ADD_FAILURE() << "HloVerifier failed: " << mutated.status();
  } else {
    EXPECT_FALSE(mutated.ValueOrDie())
        << "HloVerifier should never mutate the HloModule";
  }
}

HloModule& HloVerifiedTestBase::module() {
  if (!module_) {
    module_ = HloTestBase::CreateNewModule();
  }
  return *module_;
}

HloModule* HloVerifiedTestBase::CreateNewModule(const string& name) {
  modules_.emplace_back(HloTestBase::CreateNewModule());
  return modules_.back().get();
}

void HloVerifiedTestBase::ParseAndVerifyModule(absl::string_view hlo_text,
                                               const HloModuleConfig& config) {
  CHECK(!module_) << "Called ParseModule when test already has a module.";
  TF_ASSERT_OK_AND_ASSIGN(module_, ParseHloString(hlo_text, config));
  VerifyModule(module_.get());
}
}  // namespace xla
