/*
Copyright (c) 2018, Patrick Lafferty
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

    * Redistributions of source code must retain the above copyright
      notice, this list of conditions and the following disclaimer.
    * Redistributions in binary form must reproduce the above copyright
      notice, this list of conditions and the following disclaimer in the
      documentation and/or other materials provided with the distribution.
    * Neither the name of the copyright holder nor the names of its 
      contributors may be used to endorse or promote products derived from 
      this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE FOR 
ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/
#pragma once

#include <log.h>

namespace Assert {

    template<class T>
    decltype(auto) isTrue(T&& actual, const char* message) {
        return [&, message = message]() { 
            auto result = actual == true;

            if (!result) log("        %s", message);

            return result;
        };
    }

    template<class T>
    decltype(auto) isFalse(T&& actual, const char* message) {
        return [&, message = message]() { 
            auto result = actual == false;

            if (!result) log("        %s", message);

            return result;
        };
    }

    template<class T, class U>
    decltype(auto) isGreater(T&& actual, U&& expected, const char* message) {
        return [&, message = message]() { 
            auto result = actual > expected;

            if (!result) log("        %s", message);

            return result;
        };
    }

    template<class T, class U>
    decltype(auto) isEqual(T&& actual, U&& expected, const char* message) {
        return [&, message = message]() { 
            auto result = actual == expected;

            if (!result) log("        %s", message);

            return result;
        };
    }

    template<class T, class U>
    decltype(auto) arraySame(T&& actual, U&& expected, const char* message) {
        return [&, message = message]() {
            bool same = true;

            if (actual.size() != expected.size()) {
                same = false;
            }
            else {
                auto length = actual.size();

                for (auto i = 0u; i < length; i++) {
                    if (actual[i] != expected[i]) {
                        same = false;
                        break;
                    }
                }
            }

            if (!same) log("        %s", message);
            return same;
        };
    }

    template<typename... Assertions>
    decltype(auto) all(Assertions... assertions) {
        return (... && (assertions)());
    }
}

#include <utility>

namespace Preflight {

    /*
    A test suite is a class that has multiple test functions.
    Test functions must be static bool(void);
    Suites must contain a static function called run that
    calls all of the tests, and returns bool.

    class SomeTest {
    public:

        static constexpr const char* name = "Suite A";

        static bool testA() {
            auto assertion = Assert::something(actual, expected, message);

            return Assert::all(assertion, ...);
        }

        static bool run() {
            using namespace Preflight;
            return runTests(
                test(testA, "description for testA"),
                 ...);
        }
    };

    Then to run test suites, call 

    Preflight::runTestSuites<SomeTest, ...>();

    */   

    using TestPair = std::pair<bool(*)(), const char*>;

    inline TestPair test(bool (*f)(), const char* description) {
        return std::make_pair(f, description);
    }

    template<typename F, typename Arg>
    bool runCases(F f, Arg arg) {
        return f(arg);
    }

    template<typename F, typename Arg0, typename Arg1, typename... Args>
    bool runCases(F f, Arg0 arg0, Arg1 arg1, Args... args) {
        return f(arg0) && runCases(f, arg1, args...);
    }

    template<typename Test>
    bool runTests(Test test) {
        log("    %s: ", test.second);
        auto result = test.first();
        log("    %s", result ? "passed" : "failed");
        return result;
    }

    template<typename Test, typename Test2, typename... Tests>
    bool runTests(Test test, Test2 test2, Tests... tests) {
        log("    %s: ", test.second);
        auto result = test.first();
        log("    %s", result ? "passed" : "failed");
        return result && runTests(test2, tests...);
    }

    template<typename Test>
    bool runTestSuites() {
        log("Running suite %s", Test::name);
        return Test::run();
    }

    template<typename Test, typename Test2, typename... Tests>
    bool runTestSuites() {
        log("Running suite %s", Test::name);
        return Test::run() && runTestSuites<Test2, Tests...>();
    }
}
