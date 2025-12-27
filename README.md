一个音乐播放器 + Ballance mod，能够根据玩家球的速度同时改变音乐的速度和音高<br>
本人不喜欢 C++，打算以 C++ 程序写入 Rust 程序内存的方式写这个 mod<br>
这么做会导致用法变得繁琐
## 构建
**C++**<br>
环境配置参照 [BallanceModLoaderPlus](https://github.com/doyaGu/BallanceModLoaderPlus)<br>其中 Virtools SDK 选择 [Virtools-SDK-2.1](https://github.com/doyaGu/Virtools-SDK-2.1)<br>
```
cd cpp
cmake -B build -S . -A Win32 -DCMAKE_PREFIX_PATH="E:\Desktop\BallanceModLoaderPlus\install\lib\cmake;E:\Desktop\Virtools-SDK-2.1"
cmake --build build --config Release --parallel 4
```
**Rust**<br>
工具链：x86_64-pc-windows-msvc<br>
Target：i686-pc-windows-msvc
```
cd rust
cargo build --target i686-pc-windows-msvc --release -j 4
```
## 用法
打开本软件随便播放一首歌<br>
运行 [Cheat Engine](https://www.cheatengine.org/)，搜索本软件内存中值为 1919810 的 double<br>
蹦出来 3 个左右的匹配项，一边改值一边找到能影响音调的唯一项<br>
复制该项的地址，打开 Ballance 输入指令 twistedmusic<br>
粘贴地址到地址输入框，点击“连接”按钮<br>