## Nethack-cn

[![Build Status](https://github.com/StackC00ki3/nethack-cn/actions/workflows//nethack-vs-package.yml/badge.svg)](http://github.com/stackC00ki3/nethack-cn/releases)
![Version](https://img.shields.io/badge/version-5.0.1-blue)
[![License](https://img.shields.io/badge/license-NGPL-green)](dat/license)

English README: [README_en.md](README_en.md)

### 快速开始
无需本地编译，可直接在[本项目 Release 页面](http://github.com/stackC00ki3/nethack-cn/releases)下载自动构建的汉化预览版

### 已知待解决的问题

见[known_bugs.txt](doc/known_bugs.txt)。

### 路线图

- [x] tty 界面 UTF-8 支持
- [x] curses 界面 UTF-8 支持
- [x] win32 界面 UTF-8 支持
- [x] 合并来自 [SunnyYuer/NetHack-cn](https://github.com/SunnyYuer/NetHack-cn) 的翻译
- [x] 使用 deepseek-v4-flash 完成初步 AI 翻译
- [x] 怪物翻译
- [x] 物品翻译
- [x] 跨平台前端：**[直接启动浏览器版](https://stackc00ki3.github.io/nethack-3d/)** 仓库：[nethack-3d](https://github.com/StackC00ki3/nethack-3d)
- [x] 安卓版：**[点我下载apk](https://github.com/StackC00ki3/ANetHack-cn/releases)** 仓库：[Anethack-cn](https://github.com/StackC00ki3/ANetHack-cn)
- [x] 中文输入
- [x] 许愿机制 (仍需测试)
- [x] 灭绝机制 (仍需测试)
- [x] 跨层传送机制 (仍需测试(都能按问号了还输中文干什么))

### 翻译标准化

[物品译名标准（objects.h）](doc/objects_translation_standard_zh_cn.md)

[怪物译名标准（monsters.h）](doc/monsters_translation_standard_zh_cn.md)

[通用翻译标准](doc/common_translation_standard_zh_cn.md)

**当前译名标准仍需要更多玩家参与讨论和校对，欢迎在以下讨论中提出意见：**

[讨论物品简中译名](https://github.com/StackC00ki3/Nethack-cn/discussions/3)

[讨论怪物简中译名](https://github.com/StackC00ki3/Nethack-cn/discussions/4)

[讨论通用译名](https://github.com/StackC00ki3/Nethack-cn/discussions/7)

#### 代码规范

见[coding_standards.txt](docs/coding_standards.txt)。

### 技术细节

#### tty utf-8 支持

发现最后输出使用函数 `putchar` 逐个字符输出，而 `putchar` 支持宽字节。

于是调整输出逻辑：在当前指针指向的是 utf-8 内容时将整个字符串转为 `wchar_t *` 然后输出。

同时要调整 `console.cursor` 屏幕指针移动逻辑，当是宽字节时一次移动两个字符。

新增一种 cell 类型 `wide_char_follower_cell`, 用于标记宽字符的下一个cell为占用状态，使得清屏等操作能正确渲染。

#### curses utf-8 支持

编译 Nethack 时定义宏 `CURSES_UNICODE`, `PDC_WIDE`, `PDC_FORCE_UTF8`, `PDC_RGB`

对 pdcursesmod/pdcurses/refresh.c 进行了补丁，修复了一处 assert 引起的崩溃 bug。

#### win32 utf-8 支持

使用宏劫持 windows API 函数 `drawTextA`, `drawText`, `ListView_InsertColumn`, `SetWindowText`。将它们替换成自定义的支持 utf8 的版本。

#### 英语语法函数

见[grammatical_functions.md](doc/grammatical_functions.md)