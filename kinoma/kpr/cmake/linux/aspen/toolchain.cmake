#
#     Copyright (C) 2010-2015 Marvell International Ltd.
#     Copyright (C) 2002-2010 Kinoma, Inc.
#
#     Licensed under the Apache License, Version 2.0 (the "License");
#     you may not use this file except in compliance with the License.
#     You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#     Unless required by applicable law or agreed to in writing, software
#     distributed under the License is distributed on an "AS IS" BASIS,
#     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#     See the License for the specific language governing permissions and
#     limitations under the License.
#
SET(CMAKE_SYSTEM_NAME Linux)
SET(CMAKE_SYSTEM_VERSION 1)

SET(CMAKE_C_COMPILER $ENV{ARM_MARVELL_LINUX_GNUEABI}/bin/arm-marvell-linux-gnueabi-gcc)
SET(CMAKE_CXX_COMPILER $ENV{ARM_MARVELL_LINUX_GNUEABI}/bin/arm-marvell-linux-gnueabi-gcc)
SET(CMAKE_STRIP $ENV{ARM_MARVELL_LINUX_GNUEABI}/bin/arm-marvell-linux-gnueabi-strip CACHE STRING "Tool to strip binary")

SET(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
SET(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)

SET(CMAKE_ASM_SOURCE_FILE_EXTENSIONS gas7;gas;s)
set(CMAKE_ASM_COMPILER "${CMAKE_C_COMPILER}"     CACHE PATH "Assembler (gas)" )
SET(CMAKE_ASM_COMPILE_OBJECT "${CMAKE_C_COMPILER} -c -x assembler-with-cpp <FLAGS> -o <OBJECT> <SOURCE>")
