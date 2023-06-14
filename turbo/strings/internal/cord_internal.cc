// Copyright 2020 The Turbo Authors.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//      https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
#include "turbo/strings/internal/cord_internal.h"

#include <atomic>
#include <cassert>
#include <memory>

#include "turbo/base/internal/raw_logging.h"
#include "turbo/container/inlined_vector.h"
#include "turbo/strings/internal/cord_rep_btree.h"
#include "turbo/strings/internal/cord_rep_crc.h"
#include "turbo/strings/internal/cord_rep_flat.h"
#include "turbo/strings/internal/cord_rep_ring.h"
#include "turbo/format/str_format.h"

namespace turbo {
    TURBO_NAMESPACE_BEGIN
    namespace cord_internal {

        TURBO_CONST_INIT std::atomic<bool> cord_ring_buffer_enabled(
                kCordEnableRingBufferDefault);
        TURBO_CONST_INIT std::atomic<bool> shallow_subcords_enabled(
                kCordShallowSubcordsDefault);
        TURBO_CONST_INIT std::atomic<bool> cord_btree_exhaustive_validation(false);

        void LogFatalNodeType(CordRep *rep) {
            TURBO_INTERNAL_LOG(FATAL, turbo::Format("Unexpected node type: {}",
                                                    static_cast<int>(rep->tag)));
        }

        void CordRep::Destroy(CordRep *rep) {
            assert(rep != nullptr);

            while (true) {
                assert(!rep->refcount.IsImmortal());
                if (rep->tag == BTREE) {
                    CordRepBtree::Destroy(rep->btree());
                    return;
                } else if (rep->tag == RING) {
                    CordRepRing::Destroy(rep->ring());
                    return;
                } else if (rep->tag == EXTERNAL) {
                    CordRepExternal::Delete(rep);
                    return;
                } else if (rep->tag == SUBSTRING) {
                    CordRepSubstring *rep_substring = rep->substring();
                    rep = rep_substring->child;
                    delete rep_substring;
                    if (rep->refcount.Decrement()) {
                        return;
                    }
                } else if (rep->tag == CRC) {
                    CordRepCrc::Destroy(rep->crc());
                    return;
                } else {
                    assert(rep->IsFlat());
                    CordRepFlat::Delete(rep);
                    return;
                }
            }
        }

    }  // namespace cord_internal
    TURBO_NAMESPACE_END
}  // namespace turbo
