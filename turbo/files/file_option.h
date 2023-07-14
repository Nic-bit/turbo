//
// Created by jeff on 23-7-14.
//

#ifndef TURBO_FILES_FILE_OPTION_H_
#define TURBO_FILES_FILE_OPTION_H_

#include <cstdint>

namespace turbo {

    struct  FileOption {
        int32_t open_tries{1};
        uint32_t open_interval{0};
        bool create_dir_if_miss{false};
        bool prevent_child{false};
    };
}  // namespace turbo

#endif  // TURBO_FILES_FILE_OPTION_H_