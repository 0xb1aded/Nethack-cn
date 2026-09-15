## Nethack-cn

[![Build Status](https://github.com/StackC00ki3/nethack-cn/actions/workflows//nethack-vs-package.yml/badge.svg)](http://github.com/stackC00ki3/nethack-cn/releases)

Chinese README：[README.md](README.md)

### Quick Start
No need to compile locally; you can download the automatically built Chinese localization preview version directly from [this project’s Release page](http://github.com/stackC00ki3/nethack-cn/releases)

### To Be Completed

Files in the “sys” directory: Interface files for various platforms, which also contain text (the Windows and Linux versions are mostly translated)

data.base: Encyclopedia, totaling several thousand lines

### Known Issues to Be Resolved

On Windows TTY, if you enter the game using a non-English input method (launching the main game from the Start menu), the game freezes and becomes unresponsive (though it runs fine once you’re inside)

(Specifically, this happens when the game asks if you want to keep your save in Exploration Mode and you press ‘y’)

In curses mode, the animation for engraving Chinese characters is much slower.

The layout of the status bar below is a bit too wide; sometimes, if too much information is displayed, it may overflow.

~~In tty mode, some Chinese strings result in garbled text (curses mode works fine). There’s a comment at the end of the line: // Chinese garbled text~~ Resolved

~~But there’s still a question mark in curses!~~ Resolved

~~Also, in the code, all comments like “//debugfuzzer issue: number” have a number following them that records the number of times the issue occurred~~ Resolved

~~There are some buffer overflows (this section is too long to include here)~~ Resolved

~~Putting on clothes causes the AC to change, but the screen refresh is limited; some parts of the status bar below don’t refresh (possibly [botl.c](src/botl.c)?)~~ Resolved

In the Windows curses interface, after typing \# and then entering Chinese characters, the backspace key does not fully clear the text (is there a need or risk associated with entering Chinese characters here???)

It is now possible to set fruit names in Chinese characters (pretending this name doesn’t require `sanitize_name()`), but I expect this will cause problems in the future

### Roadmap

- [x] UTF-8 support for the tty interface
- [x] UTF-8 support for the curses interface
- [x] UTF-8 support for the win32 interface
- [x] Merged translations from [SunnyYuer/NetHack-cn](https://github.com/SunnyYuer/NetHack-cn)
- [x] Completed preliminary AI translation using deepseek-v4-flash
- [x] Monster translations
- [x] Item translations
- [x] Cross-platform frontend: **[Launch the browser version directly](https://stackc00ki3.github.io/nethack-3d/)** Repository: [nethack-3d](https://github.com/StackC00ki3/nethack-3d)
- [x] Android version: **[Click here to download the APK](https://github.com/StackC00ki3/ANetHack-cn/releases)** Repository: [Anethack-cn](https://github.com/StackC00ki3/ANetHack-cn)
- [x] Chinese input
- [x] Wish Mechanism (still needs testing)
- [x] Extinction Mechanism (still needs testing)
- [x] Cross-Floor Teleportation Mechanism (still needs testing (if you can press the question mark, why bother typing in Chinese?))

### Translation Standardization

[Simplified Chinese Translation Standards for Items (objects.h)](doc/objects_translation_standard_zh_cn.md)

[Simplified Chinese Translation Standards for Monsters (monsters.h)](doc/monsters_translation_standard_zh_cn.md)

[General Simplified Chinese Translation Standards (Predicates, Nouns, Causes of Death, Sounds, Skills, etc.)](doc/common_translation_standard_zh_cn.md)

**The current translation standards still require more player input for discussion and proofreading. Please feel free to share your feedback in the following discussions:**

[Discussion on Simplified Chinese Translations for Items](https://github.com/StackC00ki3/Nethack-cn/discussions/3)

[Discussion on Simplified Chinese Translations for Monsters](https://github.com/StackC00ki3/Nethack-cn/discussions/4)

[Discuss General Translations](https://github.com/StackC00ki3/Nethack-cn/discussions/7)

#### Coding Guidelines

When translating, please remove all spaces between English words; use spaces only to separate numbers, for example: “There are %ld coins here.”

When translating, please use half-width punctuation marks (i.e., English punctuation).

When encountering a comma, please add a space after it.

If you need to change the word order (the order in which variables appear in a string), please add a comment at the end of the line: /\* Changed word order: (original code)\*/

If you need to use a function that does not yet exist (to be added later), please write the modified code in a comment at the end of the line: /\* To be written: (modified code)\*/

If there is redundant code, please mark the commented-out code with “Redundant:” before it: /\*Redundant: (redundant code)\*/

If you are unsure about the modified code, please add a comment at the end of the line: /\*Danger: (original code)\*/

If you are simply replacing a function in the `pline` class (such as `You`, `Your`, or `pline_The`), please add a comment at the end of the line: `/* Replace pline: (original function) */`

If the debugfuzzer crashes at a specific point, and if you can locate it, add the following after that line of code: //debugfuzzer issue (if already present, increment the number by 1)

#### Manual Review
##### Source Code

- [x] allmain.c
- [x] alloc.c
- [x] apply.c
- [x] artifact.c
- [x] attrib.c
- [x] ball.c
- [x] bones.c
- [x] botl.c
- [x] calendar.c
- [x] cfgfiles.c
- [x] cmd.c
- [x] coloratt.c
- [x] date.c
- [x] dbridge.c
- [x] decl.c
- [x] detect.c
- [x] dig.c
- [x] display.c (no translation needed)
- [x] dlb.c (no translation needed)
- [x] do.c
- [x] dog.c
- [x] dogmove.c
- [x] dokick.c
- [x] dothrow.c
- [x] do_name.c
- [x] do_wear.c
- [x] drawing.c
- [x] dungeon.c
- [x] earlyarg.c (No translation needed)
- [x] eat.c
- [x] end.c
- [x] engrave.c
- [x] exper.c
- [x] explode.c
- [x] extralev.c (No translation needed)
- [x] files.c (No translation needed)
- [x] fountain.c
- [x] getpos.c
- [x] glyphs.c
- [x] hack.c
- [x] hacklib.c (No translation needed)
- [x] iactions.c
- [x] insight.c
- [x] invent.c
- [x] isaac64.c (No translation needed)
- [x] light.c
- [x] lock.c
- [x] mail.c
- [x] makemon.c
- [x] mcastu.c
- [x] mdlib.c
- [x] mhitm.c
- [x] mhitu.c
- [x] minion.c
- [x] mklev.c
- [x] mkmap.c
- [x] mkmaze.c
- [x] mkobj.c
- [x] mkroom.c
- [x] mon.c
- [x] mondata.c
- [x] monmove.c
- [x] monst.c
- [x] mplayer.c
- [x] mthrowu.c
- [x] muse.c
- [x] music.c
- [x] nhlobj.c (No translation needed)
- [x] nhlsel.c (No translation needed)
- [x] nhlua.c (No translation needed)
- [x] nhmd4.c
- [x] objects.c
- [x] objnam.c
- [x] options.c
- [x] o_init.c
- [x] pager.c
- [x] pickup.c
- [x] pline.c
- [x] polyself.c
- [x] potion.c
- [x] pray.c
- [x] priest.c
- [x] quest.c
- [x] questpgr.c
- [x] read.c
- [x] rect.c (No translation needed)
- [x] region.c
- [x] report.c
- [x] restore.c
- [x] rip.c
- [x] rnd.c (No translation needed)
- [x] role.c
- [x] rumors.c
- [x] save.c
- [x] selvar.c (No translation needed)
- [x] sfbase.c (No translation needed)
- [x] sfstruct.c (No translation needed)
- [x] shk.c
- [x] shknam.c
- [x] sit.c
- [x] sounds.c
- [x] spell.c
- [x] sp_lev.c (No translation needed)
- [x] stairs.c
- [x] steal.c
- [x] steed.c
- [x] strutil.c
- [x] symbols.c (No translation needed)
- [x] sys.c (No translation needed)
- [x] teleport.c
- [x] tile.c
- [x] timeout.c
- [x] topten.c
- [x] track.c (No translation needed)
- [x] trap.c
- [x] uhitm.c
- [x] utf8map.c (no translation needed)
- [x] u_init.c (no translation needed)
- [x] vault.c
- [x] version.c (no translation needed)
- [x] vision.c (no translation needed)
- [x] weapon.c
- [x] were.c
- [x] wield.c
- [x] windows.c
- [x] wizard.c
- [x] wizcmds.c
- [x] worm.c
- [x] worn.c
- [x] write.c
- [x] zap.c

##### Text Files

- [x] bogusmon.txt
- [x] dungeon.lua
- [x] engrave.txt
- [x] epitaph.txt
- [x] oracles.txt
- [x] rumors.fal
- [x] rumors.tru
- [x] quest.lua
- [x] optlist.h
- [x] hh
- [ ] data.base (重中之重)
- [ ] tribute
- [x] help
- [x] hh
- [x] history
- [x] keyhelp
- [x] license
- [x] opthelp
- [x] usagehelp
- [x] wizhelp

### Technical Details

#### TTY UTF-8 Support

I discovered that the final output uses the `putchar` function to output characters one by one, and `putchar` supports wide characters.

So I adjusted the output logic: when the current pointer points to UTF-8 content, the entire string is converted to `wchar_t *` before being output.

At the same time, I adjusted the `console.cursor` screen pointer movement logic so that it moves two characters at a time when dealing with wide characters.

A new cell type, `wide_char_follower_cell`, was added to mark the cell following a wide character as occupied, ensuring that operations such as screen clearing render correctly.

#### curses UTF-8 Support

When compiling Nethack, define the macros `CURSES_UNICODE`, `PDC_WIDE`, `PDC_FORCE_UTF8`, and `PDC_RGB`

A patch was applied to `pdcursesmod/pdcurses/refresh.c` to fix a crash caused by an `assert`.

#### Win32 UTF-8 Support

Macros are used to intercept the Windows API functions `drawTextA`, `drawText`, `ListView_InsertColumn`, and `SetWindowText`. These are replaced with custom versions that support UTF-8.

#### English Grammar Functions

##### plur(x)

Location: [hack.h](include/hack.h)

Function: A macro that returns the plural suffix based on the quantity parameter x.

**Solution**: Always return an empty string, regardless of singular or plural form.

##### makeplural(const char *oldstr)

Location: [objnam.c](src/objnam.c)

Function: Converts `oldstr` to the plural form and returns it.

**Solution**: Replace all instances where the suffix `s` is appended with an empty string.

##### an(const char *str) / An / just_an

Location: [objnam.c](src/objnam.c)

Functionality: Calls `just_an()`; after processing, it typically prepends `“a ”` or `“an ”` to the string

**Solution**: `just_an()` returns `“一个”`

##### s_suffix(const char *s)

Location: [hacklib.c](src/hacklib.c)

Functionality: Appends the suffix `“s”` to the string

**Solution**: Return `s` directly.

##### ing_suffix(const char *s)

Location: [hacklib.c](src/hacklib.c)

Function: Appends the suffix `“s”` to the string.

**Solution**: Return `s` directly.

##### vtense(const char *subj, const char *verb)

Location: [objnam.c](src/objnam.c)

Function: Returns the correct form of the verb `verb` in the third-person present tense

**Solution**: Replace the `s` suffix with an empty string

##### uhe(), uhim(), uhis()

Location: [you.h](include/you.h)

Function: Returns the nominative, accusative, and possessive forms of personal pronouns (male: “he,” “him,” ‘his’; female: “she,” “her,” “her”)

**Solution**: “he,” “she”

##### ordin(int n)

Location: [hacklib.c](src/hacklib.c)

Function: Returns the ordinal suffix corresponding to the number n (1→st, 2→nd, 3→rd, etc.)

**Solution**: Return an empty string “”

##### arti_light_description(wep)

Location: [light.c](src/light.c)

Function: Returns “radiantly”/“brilliantly”/“brightly”/“dimly”/“strangely”

**Solution**: Return only the content word without “的”; when using it, please append “的光芒” to the end.

##### objdescr_is(struct obj *obj, const char *descr)

Location: [o_init.c](src\o_init.c)

Function: Checks whether the description of an item ((obj_descr[(obj).oc_descr_idx].oc_descr)) is equal to descr.

**Solution**: Change it to compare its edesc; please retain the English text when calling this function.

##### getobj(const char *word, int (*obj_ok)(OBJ_P), unsigned int ctrlflags)

Location: [invent.c](src/invent.c)

Function: Finds all items that match the behavior of obj_ok for the player to choose from (if none exist, all items are displayed by default).

**Solution**: This \*word is not case-sensitive. It will ask you: “你想要” + the passed \*word + “?” (In Chinese, the word entered here may be a compound phrase, such as “写在什么上面”). Note that the word entered here should remain grammatically correct even after removing “什么.” “你想要**写在**什么**上**” and “你想要**写在**什么**上面**” are both valid, but “你没有可以**写在上**的东西” is less natural than “你没有可以**写在上面**的东西.”

##### classifier(struct obj * )

Location: [objnam.c](src/objnam.c)

Function: Takes an `obj` structure as input and returns the corresponding classifier. (The same applies to `pm_to_classifier(struct permonst *pm)`, `terrain_classifier(int sym)`, `sym_to_classifier(int sym)`, and `mon_classifier(struct monst *mon)`.)