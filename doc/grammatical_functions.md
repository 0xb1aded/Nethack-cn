# 英语语法函数

## plur(x)

位置: [hack.h](include/hack.h)

功能: 根据数量参数 x 获取复数后缀的宏。

**处理方案**: 统一返回空字符串，不区分单复数形式。

## makeplural(const char \*oldstr)

位置: [objnam.c](src/objnam.c)

功能: 将 oldstr 转成复数形式返回

**处理方案**: 将加后缀 s 的位置全部改成加空字符串

## an(const char \*str) / An / just_an

位置: [objnam.c](src/objnam.c)

功能: 调用了 `just_an()`，处理后，一般会给字符串前面加上 `"a "` 或者 `"an "`

**处理方案**: `just_an()` 返回 `"一个"`

## s_suffix(const char \*s)

位置: [hacklib.c](src/hacklib.c)

功能: 给字符串加 `"s"` 后缀

**处理方案**: 直接返回 `s`

## ing_suffix(const char \*s)

位置: [hacklib.c](src/hacklib.c)

功能: 给字符串加 `"s"` 后缀

**处理方案**: 直接返回 `s`

## vtense(const char \*subj, const char \*verb)

位置: [objnam.c](src/objnam.c)

功能: 返回在现在时第三人称下动词 `verb` 的正确形式

**处理方案**: 将加后缀 s 的位置改成加空字符串

## uhe(), uhim(), uhis()

位置: [you.h](include/you.h)

功能: 返回人称代词的主格、宾格、形容词性物主代词（男："he"、"him"、"his"；女："she"、"her"、"her"；）

**处理方案**: "他"、"她"

## ordin(int n)

位置: [hacklib.c](src/hacklib.c)

功能: 返回数字 n 对应的序数词后缀（1→st，2→nd，3→rd……）

**处理方案**: 返回一个空字符串""

## arti_light_description(wep)

位置: [light.c](src/light.c)

功能: 返回“radiantly”/“brilliantly”/“brightly”/“dimly”/“strangely”

**处理方案**: 只返回一个不带“的”的实词，使用时请在后面加上“的光芒”。

## objdescr_is(struct obj \*obj, const char \*descr)

位置: [o_init.c](src\o_init.c)

功能: 对比某物品的描述（(obj_descr[(obj).oc_descr_idx].oc_descr)）与descr是否相等

**处理方案**: 改为对比其edescr，调用时请保留英文。

## getobj(const char \*word, int (\*obj_ok)(OBJ_P), unsigned int ctrlflags)

位置: [invent.c](src/invent.c)

功能: 寻找适合obj_ok行为的所有物品供玩家选择（若没有则默认展示所有物品）。

**处理方案**: 这个\*word对字符串不敏感。它会问你："你想要"+传入的\*word+"?"（汉语的这个地方填的词可能是离合的，如：“写在什么上面”）。注意此处填写的词应该保证去掉“什么”后仍通顺。“你想要**写在**什么**上**”和“你想要**写在**什么**上面**”都是合理的，但是“你没有可以**写在上**的东西”就不如“你没有可以**写在上面**的东西”通顺。

## classifier(struct obj * )

位置: [objnam.c](src/objnam.c)

功能：传入一个obj结构体，传出它对应的量词。（pm_to_classifier(struct permonst \*pm)、terrain_classifier(int sym)、sym_to_classifier(int sym)、mon_classifier(struct monst \*mon)同理）