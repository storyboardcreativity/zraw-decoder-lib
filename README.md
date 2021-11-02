# zraw-decoder-lib

This is a ZRAW video decoding library. It provides a C interface for decoding ZRAW frames from bitstream.

### What ZRAW versions are compatible with this software?

All ZRAW bitstreams, created on Z CAM cameras with firmware version before v0.94, are supported.

### How to build?

#### For Ubuntu:

0. You just need `libcrypto`, `libpthread` and `libstdc++` to build this code.

1. Type `make` and have fun!

2. To use library just include `./include/libzraw.h` to your C/C++ project and link with `./build/libzraw.so` after building it.

#### For Windows:

0. You need Visual Studio 2017 + environment variable OPENSSL_INSTALL_DIR that points to your OpenSSL installation folder

1. Run `MSBuild.exe vc2017/zraw-decoder-lib.sln` from a `Developer Command Promt for VS 2017` (or open `vc2017/zraw-decoder-lib.sln` and build it from Visual Studio) and have fun!

2. To use library just include `./include/libzraw.h` to your C/C++ project and link with `./build/libzraw.lib` after building it.

##### Testing:

[Acutest](https://github.com/mity/acutest) library is used for unit-testing. If you want, you can type `make test` - tests will be executed.

### License

Copyright 2021 storyboardcreativity

This software is created solely on a non-commercial basis to ensure compatibility with ZRAW file formats and is licensed under the GNU General Public License version 2 (the "GPL License").

You may obtain a copy of the GPL License in the LICENSE file, or at:

http://www.gnu.org/licenses/gpl-2.0.html

Unless required by applicable law or agreed to in writing, software distributed under the GPL Licesnse is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the GPL License for the specific language governing permissions and limitations under the GPL License.
