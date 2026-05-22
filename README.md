## Nethack-cn

[![Build Status](https://github.com/StackC00ki3/nethack-cn/actions/workflows//nethack-vs-package.yml/badge.svg)](http://github.com/stackC00ki3/nethack-cn/releases)

English README：[README_en.md](README_en.md)

### 快速开始
无需本地编译，可直接在[本项目 Release 页面](http://github.com/stackC00ki3/nethack-cn/releases)下载自动构建的汉化预览版

### 路线图

- [x] tty 界面 UTF-8 支持
- [x] curses 界面 UTF-8 支持
- [x] win32 界面 UTF-8 支持
- [x] 合并来自 [SunnyYuer/NetHack-cn](https://github.com/SunnyYuer/NetHack-cn) 的翻译
- [x] 使用 deepseek-v4-flash 完成初步 AI 翻译
- [x] 怪物翻译
- [ ] 物品翻译
- [ ] 许愿机制
- [ ] 灭绝机制

#### 人工审校
- [x] allmain.c
- [x] alloc.c
- [x] apply.c
- [x] artifact.c
- [x] attrib.c
- [x] ball.c
- [x] bones.c
- [ ] botl.c
- [x] calendar.c
- [ ] cfgfiles.c
- [ ] cmd.c
- [x] coloratt.c
- [x] date.c
- [ ] dbridge.c
- [ ] decl.c
- [ ] detect.c
- [ ] dig.c
- [ ] display.c
- [ ] dlb.c
- [ ] do.c
- [ ] dog.c
- [ ] dogmove.c
- [ ] dokick.c
- [ ] dothrow.c
- [ ] do_name.c
- [ ] do_wear.c
- [ ] drawing.c
- [ ] dungeon.c
- [ ] earlyarg.c
- [ ] eat.c
- [ ] end.c
- [ ] engrave.c
- [ ] exper.c
- [ ] explode.c
- [ ] extralev.c
- [ ] files.c
- [ ] fountain.c
- [ ] getpos.c
- [ ] glyphs.c
- [ ] hack.c
- [ ] hacklib.c
- [ ] iactions.c
- [ ] insight.c
- [ ] invent.c
- [ ] isaac64.c
- [ ] light.c
- [ ] lock.c
- [ ] mail.c
- [ ] makemon.c
- [ ] mcastu.c
- [ ] mdlib.c
- [ ] mhitm.c
- [ ] mhitu.c
- [ ] minion.c
- [ ] mklev.c
- [ ] mkmap.c
- [ ] mkmaze.c
- [ ] mkobj.c
- [ ] mkroom.c
- [ ] mon.c
- [ ] mondata.c
- [ ] monmove.c
- [ ] monst.c
- [ ] mplayer.c
- [ ] mthrowu.c
- [ ] muse.c
- [ ] music.c
- [ ] nhlobj.c
- [ ] nhlsel.c
- [x] nhlua.c
- [ ] nhmd4.c
- [ ] objects.c
- [ ] objnam.c
- [ ] options.c
- [ ] o_init.c
- [ ] pager.c
- [ ] pickup.c
- [ ] pline.c
- [ ] polyself.c
- [ ] potion.c
- [ ] pray.c
- [ ] priest.c
- [ ] quest.c
- [ ] questpgr.c
- [ ] read.c
- [ ] rect.c
- [ ] region.c
- [ ] report.c
- [ ] restore.c
- [ ] rip.c
- [ ] rnd.c
- [ ] role.c
- [ ] rumors.c
- [ ] save.c
- [ ] selvar.c
- [ ] sfbase.c
- [ ] sfstruct.c
- [ ] shk.c
- [ ] shknam.c
- [ ] sit.c
- [ ] sounds.c
- [ ] spell.c
- [ ] sp_lev.c
- [ ] stairs.c
- [ ] steal.c
- [ ] steed.c
- [ ] strutil.c
- [ ] symbols.c
- [ ] sys.c
- [ ] teleport.c
- [ ] tile.c
- [ ] timeout.c
- [ ] topten.c
- [ ] track.c
- [ ] trap.c
- [ ] uhitm.c
- [ ] utf8map.c
- [ ] u_init.c
- [ ] vault.c
- [ ] version.c
- [ ] vision.c
- [ ] weapon.c
- [ ] were.c
- [ ] wield.c
- [ ] windows.c
- [ ] wizard.c
- [ ] wizcmds.c
- [ ] worm.c
- [ ] worn.c
- [ ] write.c
- [ ] zap.c

### 技术细节

#### tty utf-8 支持

发现最后输出使用函数 `getchar` 逐个字符输出，而 `getchar` 支持宽字节。

于是调整输出逻辑：在当前指针指向的是 utf-8 内容时将整个字符串转为 `wchar_t *` 然后输出。

同时要调整 `console.cursor` 屏幕指针移动逻辑，当是宽字节时一次移动两个字符。

新增一种 cell 类型 `wide_char_follower_cell`, 用于标记宽字符的下一个cell为占用状态，使得清屏等操作能正确渲染。

#### curses utf-8 支持

编译 Nethack 时定义宏 `CURSES_UNICODE`, `PDC_WIDE`, `PDC_FORCE_UTF8`, `PDC_RGB`

对 pdcursesmod/pdcurses/refresh.c 进行了补丁，修复了一处 assert 引起的崩溃 bug。

#### win32 utf-8 支持

使用宏劫持 windows API 函数 `drawTextA`, `drawText`, `ListView_InsertColumn`。将它们替换成自定义的支持 utf8 的版本。

#### 英语语法函数

##### plur(x)

位置: [hack.h](include/hack.h)

功能: 根据数量参数 x 获取复数后缀的宏。

**处理方案**: 统一返回空字符串，不区分单复数形式。

##### makeplural(const char *oldstr)

位置: [objnam.c](src/objnam.c)

功能: 将 oldstr 转成复数形式返回

**处理方案**: 将加后缀 s 的位置全部改成加空字符串

##### an(const char *str) / An / just_an

位置: [objnam.c](src/objnam.c)

功能: 调用了 `just_an()`，处理后，一般会给字符串前面加上 `"a "` 或者 `"an "`

**处理方案**: `just_an()` 返回 `"一个"`

##### s_suffix(const char *s)

位置: [hacklib.c](src/hacklib.c)

功能: 给字符串加 `"s"` 后缀

**处理方案**: 直接返回 `s`

##### vtense(const char *subj, const char *verb)

位置: [objnam.c](src/objnam.c)

功能: 返回在现在时第三人称下动词 `verb` 的正确形式

**处理方案**: 将加后缀 s 的位置改成加空字符串

##### uhe(), uhim(), uhis()

位置: [you.h](include/you.h)

功能: 返回人称代词的主格、宾格、形容词性物主代词（男："he"、"him"、"his"；女："she"、"her"、"her"；）

**处理方案**: 返回相同形式（男："他"；女："她"）

##### src/light.c

位置: [light.c](src/light.c)

功能: 返回“radiantly”/“brilliantly”/“brightly”/“dimly”/“strangely”

**处理方案**: 只返回一个不带“的”的实词，使用时请在后面加上“的光芒”。

##### arti_light_description(wep)

### 翻译标准化

有些译名在[这个页面](https://nethackwiki.com/wiki/NetHackWiki:%E7%AE%80%E4%B8%AD%E8%AF%91%E5%90%8D%E6%A0%87%E5%87%86%E5%8C%96)没有出现，所以我把我翻译的写到这里（以防翻译不统一导致对字符串敏感的函数出问题）：
|原文|Francium-223||
|----|----|----|
|shattered potion（死因，下同）|药水冻裂||
|boiling potion|药水沸腾||
|exploding potion|药水爆炸||
|burning scroll|卷轴燃烧||
|boiling potion|魔杖爆炸||
|burning book|书燃烧||
|exploding wand|魔杖爆炸||
|wielding %s bare-handed|徒手手持%s||
|falling off %s|从%s身上跌落||
|better（恢复）|好些了||
|much better|好多了||
|cracking sound|破裂声||
|tipping sound|撕裂声||
|clank|当啷声||
|crackling|劈啪声||
|form（变形）|形态||
|quantum mechanic（怪物）|量子技工^[1](#note1)^||
|thou, thee, thy, thine|汝，汝，尔, 尔^[2](#note2)^||

<a id="note1">1</a> 根据[Wiki](https://nethackwiki.com/wiki/Quantum_mechanic#Origin)，这是一个对quantum mechanics（单数，“量子力学”）错误逆构词导致的双关，且从[贴图](https://nethackwiki.com/wiki/File:Quantum_mechanic.png)和游戏内信息可以推断出quantum mechanic显然是人形生物，不应翻译为“量子力学”。

<a id="note2">2</a> 含有这种人称代词或shalt(shall 2nd sg)、art(be 2nd sg)、-est(2nd sg)、-eth(3rd sg)等的句子当译为文言。  
不一定是文言吧，或者像[浅文理和合本](https://www.bible.com/bible/1577/)那样的？

### 代码规范

如果要修改语序（变量在字符串中出现的顺序），请在行最后添加注释：/*修改语序:(修改前的代码)*/

如果要用到不存在的（待补充的）函数，请把修改后的代码写到行最后的注释里：/*待写:(修改后的代码)*/

如果有冗余的代码，请在注释掉的代码前标注上“冗余：”：/*冗余:(冗余的代码)*/

如果对修改后的代码没有把我，请在行最后添加注释：/*危险:(修改前的代码)*/
