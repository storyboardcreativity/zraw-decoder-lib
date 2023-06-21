# zraw-decoder-lib

This is a ZRAW video decoding library. It provides a C interface for decoding ZRAW frames from bitstream.

### What ZRAW versions are compatible with this software?

All ZRAW bitstreams with Bayer CFA, created on Z CAM cameras with firmware version before v0.94 (+ new hacked firmwares with that codec), are supported.

### How to build?

#### For Ubuntu:

0. You just need `libcrypto`, `libpthread` and `libstdc++` to build this code.

1. Type `make` and have fun!

2. To use library just include `./include/libzraw.h` to your C/C++ project and link with `./build/libzraw.so` after building it.

#### For Windows:

0. You need Visual Studio 2017 + environment variable OPENSSL_INSTALL_DIR that points to your OpenSSL installation folder

1. Run `MSBuild.exe vc2017/zraw-decoder-lib.sln` from a `Developer Command Promt for VS 2017` (or open `vc2017/zraw-decoder-lib.sln` and build it from Visual Studio) and have fun!

2. To use library just include `./include/libzraw.h` to your C/C++ project and link with `./build/libzraw.lib` after building it.

#### For Mac OS:

0. You need 2 versions of Homebrew for each architecture -> x86-64 and ARM64:
- Install Homebrew for ARM64:
```
cd <root_folder_to_homebrew_installations>
setopt sh_word_split
mkdir arm64-homebrew && curl -L https://github.com/Homebrew/brew/tarball/master | tar xz --strip 1 -C arm64-homebrew
alias arm64-brew='$(pwd)/arm64-homebrew/bin/brew'
response=$(arm64-brew fetch --force --bottle-tag=arm64_big_sur boost | grep "Downloaded to")
parsed=($response)
arm64-brew install $parsed[3]
```
- Install Homebrew for x86-64:
```
cd <root_folder_to_homebrew_installations>
setopt sh_word_split
mkdir x86_64-homebrew && curl -L https://github.com/Homebrew/brew/tarball/master | tar xz --strip 1 -C x86_64-homebrew
alias x86_64-brew='$(pwd)/x86_64-homebrew/bin/brew'
response=$(x86_64-brew fetch --force --bottle-tag=arm64_big_sur boost | grep "Downloaded to")
parsed=($response)
x86_64-brew install $parsed[3]
```
1. You need OpenSSL for each architecture -> x86-64 and ARM64:
- Install binaries for x86-64:
```
x86_64-brew fetch --force --bottle-tag=x86_64_big_sur openssl
x86_64-brew install $(brew --cache --bottle-tag=x86_64_big_sur openssl)
```
- Install binaries for ARM64:
```
arm64-brew fetch --force --bottle-tag=arm64_big_sur openssl
arm64-brew install $(brew --cache --bottle-tag=arm64_big_sur openssl)
```
2. Type `make` and have fun!

##### Testing:

[Acutest](https://github.com/mity/acutest) library is used for unit-testing. If you want, you can type `make test` - tests will be executed.

### License

Copyright 2021 storyboardcreativity

This software is created solely on a non-commercial basis to ensure compatibility with ZRAW file formats and is licensed under the GNU General Public License version 2 (the "GPL License").

You may obtain a copy of the GPL License in the LICENSE file, or at:

http://www.gnu.org/licenses/gpl-2.0.html

Unless required by applicable law or agreed to in writing, software distributed under the GPL Licesnse is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the GPL License for the specific language governing permissions and limitations under the GPL License.
