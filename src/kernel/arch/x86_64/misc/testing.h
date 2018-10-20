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

namespace Assert {

    template<class T>
    decltype(auto) isTrue(T&& t, const char* message) {
        return [&, message = message]() { 
            auto result = t == true;

            if (!result) log(message);

            return result;
        };
    }

    template<class T, class U>
    decltype(auto) isGreater(T&& t, U&& u, const char* message) {
        return [&, message = message]() { 
            auto result = t > u;

            if (!result) log(message);

            return result;
        };
    }

    template<typename... Assertions>
    decltype(auto) all(Assertions... assertions) {
        return (... && (assertions)());
    }
}

namespace Preflight {

    /*
    A test suite is a class that has multiple test functions.
    Test functions must be static bool(void);
    Suites must contain a static function called run that
    calls all of the tests, and returns bool.

    class SomeTest {
    public:

        static bool test() {
            auto assertion = Assert::something(actual, expected, message);

            return Assert::all(assertion, ...);
        }

        static bool run() {
            return Preflight::runTests(test, ...);
        }
    };

    Then to run test suites, call 

    Preflight::runTestSuites<SomeTest, ...>();

    */    

    template<typename... Tests>
    decltype(auto) runTests(Tests... tests) {
        return (... && (tests)());
    }
    
    template<typename... Tests>
    decltype(auto) runTestSuites() {
        return (... && (Tests::run()));
    }
}
