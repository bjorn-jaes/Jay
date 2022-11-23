#
# Copyright (c) 2020 Bjørn Fuglestad, Jaersense AS (bjorn@jaersense.no)
#
# Distributed under the MIT License, Version 1.0. (See accompanying
# file LICENSE or copy at http://www.boost.org/LICENSE_1_0.txt)
#
# Official repository: https://github.com/bjorn-jaes/jay
#

include(CMakeFindDependencyMacro)

find_dependency(Boost COMPONENTS system)
find_dependency(canary)
find_dependency(Threads)

include("${CMAKE_CURRENT_LIST_DIR}/jayTargets.cmake")