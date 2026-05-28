## Nethack-cn

[![Build Status](https://github.com/StackC00ki3/nethack-cn/actions/workflows//nethack-vs-package.yml/badge.svg)](http://github.com/stackC00ki3/nethack-cn/releases)

English README：[README_en.md](README_en.md)

### 简中译名标准

[物品简中译名标准（objects.h）](doc/objects_translation_standard_zh_cn.md)

[怪物简中译名标准（monsters.h）](doc/monsters_translation_standard_zh_cn.md)

**当前译名标准仍需要更多玩家参与讨论和校对，欢迎在以下讨论中提出意见：**

[讨论物品简中译名](https://github.com/StackC00ki3/Nethack-cn/discussions/3)

[讨论怪物简中译名](https://github.com/StackC00ki3/Nethack-cn/discussions/4)



### 快速开始
无需本地编译，可直接在[本项目 Release 页面](http://github.com/stackC00ki3/nethack-cn/releases)下载自动构建的汉化预览版

### 路线图

- [x] tty 界面 UTF-8 支持
- [x] curses 界面 UTF-8 支持
- [x] win32 界面 UTF-8 支持
- [x] 合并来自 [SunnyYuer/NetHack-cn](https://github.com/SunnyYuer/NetHack-cn) 的翻译
- [x] 使用 deepseek-v4-flash 完成初步 AI 翻译
- [x] 怪物翻译
- [x] 物品翻译
- [ ] 许愿机制
- [ ] 灭绝机制

#### 人工审校
##### 源代码

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
- [x] dog.c
- [ ] dogmove.c
- [x] dokick.c
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
- [x] selvar.c (无需翻译)
- [x] sfbase.c (无需翻译)
- [x] sfstruct.c (无需翻译)
- [ ] shk.c
- [ ] shknam.c
- [ ] sit.c
- [ ] sounds.c
- [ ] spell.c
- [x] sp_lev.c (无需翻译)
- [ ] stairs.c
- [ ] steal.c
- [ ] steed.c
- [ ] strutil.c
- [x] symbols.c (无需翻译)
- [x] sys.c (无需翻译)
- [ ] teleport.c
- [ ] tile.c
- [ ] timeout.c
- [ ] topten.c
- [x] track.c (无需翻译)
- [ ] trap.c
- [ ] uhitm.c
- [ ] utf8map.c
- [ ] u_init.c
- [ ] vault.c
- [x] version.c (无需翻译)
- [x] vision.c (无需翻译)
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

##### 文本文件(主要的几个，还有好多)

- [ ] bogusmon.txt
- [ ] dungeon.lua
- [ ] engrave.txt
- [ ] epitaph.txt
- [ ] oracles.txt
- [ ] rumors.fal
- [ ] rumors.tru
- [ ] tribute

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

##### ordin(int n)

位置: [hacklib.c](src/hacklib.c)

功能: 返回数字 n 对应的序数词后缀（1→st，2→nd，3→rd……）

**处理方案**: 返回一个空字符串""

##### arti_light_description(wep)

位置: [light.c](src/light.c)

功能: 返回“radiantly”/“brilliantly”/“brightly”/“dimly”/“strangely”

**处理方案**: 只返回一个不带“的”的实词，使用时请在后面加上“的光芒”。

##### objdescr_is(struct obj *obj, const char *descr)  

位置: [o_init.c](src\o_init.c)

功能: 对比某物品的描述（(obj_descr[(obj).oc_descr_idx].oc_descr)）与descr是否相等

**处理方案**: 改为对比其edescr，调用时请保留英文。

#### 翻译标准化

有些译名在[这个页面](https://nethackwiki.com/wiki/NetHackWiki:%E7%AE%80%E4%B8%AD%E8%AF%91%E5%90%8D%E6%A0%87%E5%87%86%E5%8C%96)没有出现，所以我把我翻译的写到这里（以防翻译不统一导致对字符串敏感的函数出问题）：

|谓词|Francium-223||
|----|----|----|
|bounce|反弹||
|reflect|反射||
|hit(结果)|击中||
|shine|照耀||
|better（恢复）|好些了||
|much better|好多了||
|glow|发/散发||
|violently glow|爆发||
|shatter|粉碎||
|wobble|摇晃||
|flutter|扑腾||
|stagger|踉跄||
|slither|蠕动||
|falter|蹒跚||
|flop|扑腾||
|ooze|滑动||
|tremble|颤抖||
|wiggle|扭动||
|pulsate|震动（注意不同于“振动”）||
|crawl|爬行||
|bash|猛击||
|lash|抽打||
|smite|重击||
|hit(攻击动作)|打||
|sear(银质物品对恶魔)|烧灼||
|crush(攻击)|挤压||
|queasy|反胃||
|sick(如果没有致病)|不适||
|squeak|嘎吱作响||
|sluggish|呆滞||
|lethargic|疲惫||
|gallop|疾驰||
|(being worn)|(穿戴中)||
|(alternate weapon; not wielded)|(副武器;未装备)||
|(on left hand)|(在左手上)||
|(on right hand)|(在右手上)||

|体词|Francium-223||
|----|----|----|
|form(变形)|形态||
|quantum mechanic(怪物)|量子技工<sup>[1](#note1)</sup>||
|thou, thee, thy, thine|汝，汝，尔，尔<sup>[2](#note2)</sup>||
|arrow, bolt|箭<sup>[3](#note3)</sup>||
|barb|倒刺||
|debris(空气元素)|碎片||
|flash|闪光||
|aura|光晕||
|light|光线||
|anti-magic field|反魔法力场||
|spike(陷阱里的)|尖刺||
|sleep resistance|睡眠抗性||
|warning|警觉||
|infravision|红外视觉||
|slow digestion|减慢消化||
|protection from shape changers|变形怪防护||
|fixed abilities|属性维持||

|死因|Francium-223||
|----|----|----|
|shattered potion|药水冻裂||
|boiling potion|药水沸腾||
|exploding potion|药水爆炸||
|burning scroll|卷轴燃烧||
|boiling potion|魔杖爆炸||
|burning book|书燃烧||
|exploding wand|魔杖爆炸||
|wielding %s bare-handed|徒手手持%s||
|falling off %s|从%s身上跌落||
|stolen|偷窃||
|expire|消散||

|声音|Francium-223||
|----|----|----|
|cracking sound|破裂声||
|tipping sound|撕裂声||
|crumbling sound|碎裂声||
|clank|当啷声||
|crackling|劈啪声||
|hissing|嘶嘶声||
|cough|咳嗽声||
|click|咔哒声||
|rumbling|隆隆声||
|slow drip|滴答声||
|gurgling|咕噜声||

|感叹词/拟声词|Francium-223||
|----|----|----|
|Wait!|等等!||
|Ouch!|哎呦!||
|Splat!|啪!||
|Splash!|哗啦!||
|Burrrrp!|嗝!||
|KAABLAMM!|轰隆!||
|Kaablamm!|轰!||
|Click!|咔哒!||
|Thump!|砰!||
|Bummer!|真倒霉!||
|Phew|呼!||
|Whoops...|哎呀...||
|KABOOM|嘭!||
|Egads!|天哪!||

|技能|Francium-223||
|----|----|----|
|no skill|无技能||
|bare hands|徒手||
|two weapon combat|双持||
|riding|骑乘||
|polearms|长棍||
|saber|军刀||
|hammer|锤子||
|whip|鞭子||
|attack spells|攻击法术||
|healing spells|治疗法术||
|divination spells|预测法术||
|enchantment spells|附魔法术||
|clerical spells|神圣法术||
|escape spells|逃脱法术||
|matter spells|物质法术||
|bare handed combat|徒手格斗||
|martial arts|武术||
|Unskilled（熟练度）|无技能||
|Basic|基本||
|Skilled|熟练||
|Expert|专家||
|Master|大师||
|Grand Master|宗师||
|Unknown|未知||

<a id="note1">1</a> 根据[Wiki](https://nethackwiki.com/wiki/Quantum_mechanic#Origin)，这是一个对quantum mechanics（单数，“量子力学”）错误逆构词导致的双关，且从[贴图](https://nethackwiki.com/wiki/File:Quantum_mechanic.png)和游戏内信息可以推断出quantum mechanic显然是人形生物，不应翻译为“量子力学”。

<a id="note2">2</a> 含有这种人称代词或shalt(shall 2nd sg)、art(be 2nd sg)、-est(2nd sg)、-eth(3rd sg)等的句子当译为文言。  
也不一定是文言吧，或者像[浅文理和合本](https://www.bible.com/bible/1577/)那样的浅近文言？

<a id="note3">3</a> bolt只有crossbow bolt（弩箭）一种，和arrow没有最小对立。

#### 代码规范

如果要修改语序（变量在字符串中出现的顺序），请在行最后添加注释：/*修改语序:(修改前的代码)*/

如果要用到不存在的（待补充的）函数，请把修改后的代码写到行最后的注释里：/*待写:(修改后的代码)*/

如果有冗余的代码，请在注释掉的代码前标注上“冗余：”：/*冗余:(冗余的代码)*/

如果对修改后的代码没有把握，请在行最后添加注释：/*危险:(修改前的代码)*/

