## Nethack-cn


[![Build Status](https://github.com/StackC00ki3/nethack-cn/actions/workflows//nethack-vs-package.yml/badge.svg)](http://github.com/stackC00ki3/nethack-cn/releases)

Chinese README: [README.md](README.md)

### Quick Start
No local build is required. You can download the auto-built Chinese preview release directly from the [project Releases page](http://github.com/stackC00ki3/nethack-cn/releases).

### Roadmap

- [x] UTF-8 support for tty interface
- [x] UTF-8 support for curses interface
- [x] UTF-8 support for win32 interface
- [x] Merged translations from [SunnyYuer/NetHack-cn](https://github.com/SunnyYuer/NetHack-cn)
- [x] Initial AI translation via deepseek-v4-flash
- [ ] Monster translation
- [ ] Object translation
- [ ] Wishing mechanism
- [ ] Extinction mechanism

#### Human Review
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

### Technical Details

#### tty utf-8 support

The final output uses the `getchar` function to output characters one by one, and `getchar` supports wide characters.

So the output logic is adjusted: when the current pointer points to utf-8 content, the entire string is converted to `wchar_t *` and then output.

At the same time, the `console.cursor` screen pointer movement logic needs to be adjusted. When it is a wide character, move two characters at a time.

A new cell type `wide_char_follower_cell` is added to mark the next cell of a wide character as occupied, so that operations such as clearing the screen can render correctly.


#### curses utf-8 support

When compiling Nethack, define macros `CURSES_UNICODE`, `PDC_WIDE`, `PDC_FORCE_UTF8`, `PDC_RGB`.

Patched `pdcursesmod/pdcurses/refresh.c` to fix a crash bug caused by an assert.

#### win32 utf-8 support

Use macro hooks to intercept Windows API functions `drawTextA`, `drawText`, `ListView_InsertColumn`, replacing them with custom utf8-supporting versions.

#### English Grammar Functions

##### plur(x)

Location: [hack.h](include/hack.h)

Function: Macro to get plural suffix based on quantity parameter x.

**Solution**: Always return an empty string, no distinction between singular and plural.

##### makeplural(const char *oldstr)

Location: [objnam.c](src/objnam.c)

Function: Convert oldstr to plural form and return.

**Solution**: All places that add the suffix s are changed to add an empty string.

##### an(const char *str) / An / just_an

Location: [objnam.c](src/objnam.c)

Function: Calls `just_an()`, which usually adds "a " or "an " to the front of the string.

**Solution**: `just_an()` returns "一个" (Chinese for "a").

##### s_suffix(const char *s)

Location: [hacklib.c](src/hacklib.c)

Function: Add "s" suffix to the string.

**Solution**: Directly return `s`.
