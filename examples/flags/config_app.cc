// Copyright 2023 The Turbo Authors.
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
//
#include "turbo/flags/flags.h"
#include <iostream>
#include <string>

int main(int argc, char **argv) {

    turbo::App app("configuration print example");

    app.add_flag("-p,--print", "Print configuration and exit")->configurable(false);  // NEW: print flag

    std::string file;
    turbo::Option *opt = app.add_option("-f,--file,file", file, "File name")
                           ->capture_default_str()
                           ->run_callback_for_default();  // NEW: capture_default_str()

    int count{0};
    turbo::Option *copt =
        app.add_option("-c,--count", count, "Counter")->capture_default_str();  // NEW: capture_default_str()

    int v{0};
    turbo::Option *flag = app.add_flag("--flag", v, "Some flag that can be passed multiple times")
                            ->capture_default_str();  // NEW: capture_default_str()

    double value{0.0};                                                          // = 3.14;
    app.add_option("-d,--double", value, "Some Value")->capture_default_str();  // NEW: capture_default_str()

    app.get_config_formatter_base()->quoteCharacter('"', '"');

    TURBO_FLAGS_PARSE(app, argc, argv);

    if(app.get_option("--print")->as<bool>()) {  // NEW: print configuration and exit
        std::cout << app.config_to_str(true, false);
        return 0;
    }

    std::cout << "Working on file: " << file << ", direct count: " << app.count("--file")
              << ", opt count: " << opt->count() << std::endl;
    std::cout << "Working on count: " << count << ", direct count: " << app.count("--count")
              << ", opt count: " << copt->count() << std::endl;
    std::cout << "Received flag: " << v << " (" << flag->count() << ") times\n";
    std::cout << "Some value: " << value << std::endl;

    return 0;
}
