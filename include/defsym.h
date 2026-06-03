/* NetHack 5.0 defsym.h $NHDT-Date: 1725653007 2024/09/06 20:03:27 $ $NHDT-Branch: NetHack-3.7 $ $NHDT-Revision: 1.25 $ */
/*      Copyright (c) 2016 by Pasi Kallinen              */
/* NetHack may be freely redistributed.  See license for details. */

/*
    This header is included in multiple places to produce
    different code depending on its use. Its purpose is to
    ensure that there is only one definitive source for
    pchar, objclass and mon symbols.

    The morphing macro expansions are used in these places:
  - in include/sym.h for enums of some S_* symbol values
    (define PCHAR_S_ENUM, MONSYMS_S_ENUM prior to #include defsym.h)
  - in include/sym.h for enums of some DEF_* symbol values
    (define MONSYMS_DEFCHAR_ENUM prior to #include defsym.h)
  - in include/objclass.h for enums of some default character values
    (define OBJCLASS_DEFCHAR_ENUM prior to #include defsym.h)
  - in include/objclass.h for enums of some *_CLASS values
    (define OBJCLASS_CLASS_ENUM prior to #include defsym.h)
  - in include/objclass.h for enums of S_* symbol values
    (define OBJCLASS_S_ENUM prior to #include defsym.h)
  - in src/symbols.c for parsing S_ entries in config files
    (define PCHAR_PARSE, MONSYMS_PARSE, OBJCLASS_PARSE prior
    to #include defsym.h)
  - in src/drawing.c for initializing some data structures/arrays
    (define PCHAR_DRAWING, MONSYMS_DRAWING, OBJCLASS_DRAWING prior
    to #include defsym.h)
  - in win/share/tilemap.c for processing a tile file
    (define PCHAR_TILES prior to #include defsym.h).
  - in src/allmain.c for setting up the dumping of several enums
    (define DUMP_ENUMS_PCHAR, DUMP_ENUMS_MONSYS, DUMP_ENUMS_MONSYMS_DEFCHAR
     DUMP_ENUMS_OBJCLASS_DEFCHARS, DUMP_ENUMS_OBJCLASS_DEFCHARS
     DUMP_ENUMS_OBJCLASS_CLASSES, DUMP_ENUMS_OBJCLASS_SYMS)
*/

#if defined(PCHAR_S_ENUM)                   || defined(PCHAR_PARSE)                 || defined(PCHAR_DRAWING)               || defined(PCHAR_TILES)                 || defined(DUMP_ENUMS_PCHAR)

/*
   PCHAR(idx, ch, sym, desc, clr)
       idx:     index used in enum
       ch:      character symbol
       sym:     symbol name for parsing purposes (also tile name)
       desc:    description
       clr:     color

   PCHAR2(idx, ch, sym, tilenm, desc, clr)
       idx:     index used in enum
       ch:      character symbol
       sym:     symbol name for parsing purposes
       tilenm:  if the name in the tile txt file differs from desc (below),
                the name in the tile txt file can be specified here.
       desc:    description
       clr:     color
*/

#if defined(PCHAR_S_ENUM)
/* sym.h */
#define PCHAR(idx, ch, sym, desc, clr) sym = idx,

#elif defined(PCHAR_PARSE)
/* symbols.c */
#define PCHAR(idx, ch, sym, desc, clr) { SYM_PCHAR, sym, #sym },

#elif defined(PCHAR_DRAWING)
/* drawing.c */
#define PCHAR(idx, ch, sym, desc, clr) { ch, desc, clr },

#elif defined(PCHAR_TILES)
/* win/share/tilemap.c */
#define PCHAR(idx, ch, sym, desc, clr) { sym, desc, desc },
#define PCHAR2(idx, ch, sym, tilenm, desc, clr) { sym, tilenm, desc },

#elif defined(DUMP_ENUMS_PCHAR)
/* allmain.c */
#define PCHAR(idx, ch, sym, desc, clr) { sym, #sym },
#ifndef PCHAR2
#define PCHAR2(idx, ch, sym, tilenm, desc, clr) { sym, #sym },
#endif
#endif

/* PCHAR with extra arg */
#ifndef PCHAR2
#define PCHAR2(idx, ch, sym, tilenm, desc, clr) PCHAR(idx, ch, sym, desc, clr)
#endif

    PCHAR2( 0, ' ',  S_stone,  "房间的阴暗处", "stone",  NO_COLOR)
    PCHAR2( 1, '|',  S_vwall,  "垂直墙壁", "wall",  CLR_GRAY)
    PCHAR2( 2, '-',  S_hwall,  "水平墙壁", "wall",  CLR_GRAY)
    PCHAR2( 3, '-',  S_tlcorn, "左上角墙壁", "wall",  CLR_GRAY)
    PCHAR2( 4, '-',  S_trcorn, "右上角墙壁", "wall",  CLR_GRAY)
    PCHAR2( 5, '-',  S_blcorn, "左下角墙壁", "wall", CLR_GRAY)
    PCHAR2( 6, '-',  S_brcorn, "右下角墙壁", "wall", CLR_GRAY)
    PCHAR2( 7, '-',  S_crwall, "交叉墙壁", "wall", CLR_GRAY)
    PCHAR2( 8, '-',  S_tuwall, "tuwall", "wall", CLR_GRAY)
    PCHAR2( 9, '-',  S_tdwall, "tdwall", "wall", CLR_GRAY)
    PCHAR2(10, '|',  S_tlwall, "tlwall", "wall", CLR_GRAY)
    PCHAR2(11, '|',  S_trwall, "trwall", "wall", CLR_GRAY)
    /* start cmap A                                                      */
    PCHAR2(12, '.',  S_ndoor,  "没有门", "doorway", CLR_GRAY)
    PCHAR2(13, '-',  S_vodoor, "垂直的打开的门", "open door", CLR_BROWN)
    PCHAR2(14, '|',  S_hodoor, "水平的打开的门", "open door", CLR_BROWN)
    PCHAR2(15, '+',  S_vcdoor, "垂直的关上的门",
                               "closed door", CLR_BROWN)
    PCHAR2(16, '+',  S_hcdoor, "水平的观赏的门",
                               "closed door", CLR_BROWN)
    PCHAR( 17, '#',  S_bars,   "铁栅栏", HI_METAL)
    PCHAR( 18, '#',  S_tree,   "树", CLR_GREEN)
    PCHAR( 19, '.',  S_room,   "房间的地板", CLR_GRAY)
    PCHAR( 20, '.',  S_darkroom, "房间的阴暗处", CLR_BLACK)
    PCHAR2(21, '`',  S_engroom, "房间内的雕刻", "engraving",
                                CLR_BRIGHT_BLUE)
    PCHAR2(22, '#',  S_corr,   "黑暗的走廊", "corridor", CLR_GRAY)
    PCHAR( 23, '#',  S_litcorr, "有灯光的走廊", CLR_GRAY)
    PCHAR2(24, '#',  S_engrcorr, "走廊内的雕刻", "engraving",
                                 CLR_BRIGHT_BLUE)
    PCHAR2(25, '<',  S_upstair, "上楼梯", "staircase up", CLR_GRAY)
    PCHAR2(26, '>',  S_dnstair, "下楼梯", "staircase down", CLR_GRAY)
    PCHAR2(27, '<',  S_upladder, "上梯子", "ladder up", CLR_BROWN)
    PCHAR2(28, '>',  S_dnladder, "下梯子", "ladder down", CLR_BROWN)
    PCHAR( 29, '<',  S_brupstair, "分支楼梯上行", CLR_YELLOW)
    PCHAR( 30, '>',  S_brdnstair, "分支楼梯下行", CLR_YELLOW)
    PCHAR( 31, '<',  S_brupladder, "分支梯子上行", CLR_YELLOW)
    PCHAR( 32, '>',  S_brdnladder, "分支梯子下行", CLR_YELLOW)
    /* end cmap A */
    PCHAR( 33, '_',  S_altar,  "祭坛", CLR_GRAY)
    /* start cmap B */
    PCHAR( 34, '|',  S_grave,  "坟墓", CLR_WHITE)
    PCHAR2(35, '\\', S_throne, "王座", "opulent throne", HI_GOLD)
    PCHAR( 36, '{',  S_sink,   "水槽", CLR_WHITE)
    PCHAR( 37, '{',  S_fountain, "喷泉", CLR_BRIGHT_BLUE)
    /* the S_pool symbol is used for both POOL terrain and MOAT terrain */
    PCHAR2(38, '}',  S_pool,   "水池", "water", CLR_BLUE)
    PCHAR( 39, '.',  S_ice,    "冰", CLR_CYAN)
    PCHAR( 40, '}',  S_lava,   "熔岩", CLR_RED)
    PCHAR( 41, '}',  S_lavawall,  "熔岩墙", CLR_ORANGE)
    PCHAR2(42, '.',  S_vodbridge, "垂直打开吊桥",
                                  "lowered drawbridge", CLR_BROWN)
    PCHAR2(43, '.',  S_hodbridge, "水平打开吊桥",
                                  "lowered drawbridge", CLR_BROWN)
    PCHAR2(44, '#',  S_vcdbridge, "垂直关上吊桥",
                                  "raised drawbridge", CLR_BROWN)
    PCHAR2(45, '#',  S_hcdbridge, "水平关上吊桥",
                                  "raised drawbridge", CLR_BROWN)
    PCHAR( 46, ' ',  S_air,    "空气", CLR_CYAN)
    PCHAR( 47, '#',  S_cloud,  "云层", CLR_GRAY)
    /* the S_water symbol is used for WATER terrain: wall of water in the
       dungeon and Plane of Water in the endgame */
    PCHAR( 48, '}',  S_water,  "水域", CLR_BRIGHT_BLUE)
    /* end dungeon characters                                          */
    /*                                                                 */
    /* begin traps                                                     */
    /*                                                                 */
    PCHAR( 49, '^',  S_arrow_trap, "箭矢陷阱", HI_METAL)
    PCHAR( 50, '^',  S_dart_trap, "飞镖陷阱", HI_METAL)
    PCHAR( 51, '^',  S_falling_rock_trap, "落石陷阱", CLR_GRAY)
    PCHAR( 52, '^',  S_squeaky_board, "吱呀作响的木板", CLR_BROWN)
    PCHAR( 53, '^',  S_bear_trap, "熊陷阱", HI_METAL)
    PCHAR( 54, '^',  S_land_mine, "地雷", CLR_RED)
    PCHAR( 55, '^',  S_rolling_boulder_trap, "滚石陷阱", CLR_GRAY)
    PCHAR( 56, '^',  S_sleeping_gas_trap, "催眠气体陷阱", HI_ZAP)
    PCHAR( 57, '^',  S_rust_trap, "锈蚀陷阱", CLR_BLUE)
    PCHAR( 58, '^',  S_fire_trap, "火焰陷阱", CLR_ORANGE)
    PCHAR( 59, '^',  S_pit, "深坑", CLR_BLACK)
    PCHAR( 60, '^',  S_spiked_pit, "带刺深坑", CLR_BLACK)
    PCHAR( 61, '^',  S_hole, "洞穴", CLR_BROWN)
    PCHAR( 62, '^',  S_trap_door, "密道门", CLR_BROWN)
    PCHAR( 63, '^',  S_teleportation_trap, "传送陷阱", CLR_MAGENTA)
    PCHAR( 64, '^',  S_level_teleporter, "平面传送器", CLR_MAGENTA)
    PCHAR( 65, '^',  S_magic_portal, "魔法传送门", CLR_BRIGHT_MAGENTA)
    PCHAR( 66, '"',  S_web, "蜘蛛网", CLR_GRAY)
    PCHAR( 67, '^',  S_statue_trap, "雕像陷阱", CLR_GRAY)
    PCHAR( 68, '^',  S_magic_trap, "魔法陷阱", HI_ZAP)
    PCHAR2(69, '^',  S_anti_magic_trap, "反魔法陷阱", "anti-magic field",
                                        HI_ZAP)
    PCHAR( 70, '^',  S_polymorph_trap, "变形陷阱", CLR_BRIGHT_GREEN)
    PCHAR( 71, '~',  S_vibrating_square, "震动方块", CLR_MAGENTA)
    PCHAR( 72, '^',  S_trapped_door, "带陷阱的门", CLR_ORANGE)
    PCHAR( 73, '^',  S_trapped_chest, "带陷阱的箱子", CLR_ORANGE)
    /* end traps                                                       */
    /* end cmap B */
    /*                                                                   */
    /* begin special effects                                             */
    /*                                                                   */
    /* zap colors are changed by reset_glyphmap() to match type of beam */
    /*                                                                   */
    PCHAR2(74, '|',  S_vbeam, "垂直光束", "", CLR_GRAY)
    PCHAR2(75, '-',  S_hbeam, "水平光束", "", CLR_GRAY)
    PCHAR2(76, '\\', S_lslant, "左斜光束", "", CLR_GRAY)
    PCHAR2(77, '/',  S_rslant, "右斜光束", "", CLR_GRAY)
    /* start cmap C */
    PCHAR2(78, '*',  S_digbeam, "挖掘光束", "", CLR_WHITE)
    PCHAR2(79, '!',  S_flashbeam, "闪光光束", "", CLR_WHITE)
    PCHAR2(80, ')',  S_boomleft, "左侧爆炸", "", HI_WOOD)
    PCHAR2(81, '(',  S_boomright, "右侧爆炸", "", HI_WOOD)
    /* 4 magic shield symbols                                          */
    PCHAR2(82, '0',  S_ss1, "护盾1", "", HI_ZAP)
    PCHAR2(83, '#',  S_ss2, "护盾2", "", HI_ZAP)
    PCHAR2(84, '@',  S_ss3, "护盾3", "", HI_ZAP)
    PCHAR2(85, '*',  S_ss4, "护盾4", "", HI_ZAP)
    PCHAR( 86, '#',  S_poisoncloud, "毒云", CLR_BRIGHT_GREEN)
    /* for a time S_goodpos was a question mark, but dollar sign is the
       default keystroke for getpos() to toggle goodpos glyphs on or off */
    PCHAR( 87, '$',  S_goodpos, "有效位置", HI_ZAP)
    /* end cmap C */
    /*                                                             */
    /* The 8 swallow symbols.  Do NOT separate.                    */
    /* To change order or add, see the function swallow_to_glyph() */
    /* in display.c. swallow colors are changed by                 */
    /* reset_glyphmap() to match the engulfing monst.              */
    /*                                                             */
    /*  Order:                                                     */
    /*                                                             */
    /*      1 2 3                                                  */
    /*      4 5 6                                                  */
    /*      7 8 9                                                  */
    /*                                                             */
    PCHAR2(88, '/',  S_sw_tl, "吞噬左上角", "", CLR_GREEN)      /*1*/
    PCHAR2(89, '-',  S_sw_tc, "吞噬顶部中央", "", CLR_GREEN)    /*2*/
    PCHAR2(90, '\\', S_sw_tr, "吞噬右上角", "", CLR_GREEN)     /*3*/
    PCHAR2(91, '|',  S_sw_ml, "吞噬左中角", "", CLR_GREEN)   /*4*/
    PCHAR2(92, '|',  S_sw_mr, "吞噬右中角", "", CLR_GREEN)  /*6*/
    PCHAR2(93, '\\', S_sw_bl, "吞噬左下角", "", CLR_GREEN)   /*7*/
    PCHAR2(94, '-',  S_sw_bc, "吞噬底部中央", "", CLR_GREEN) /*8*/
    PCHAR2(95, '/',  S_sw_br, "吞噬右下角", "", CLR_GREEN)  /*9*/
    /*                                                             */
    /* explosion colors are changed by reset_glyphmap() to match   */
    /* the type of expl.                                           */
    /*                                                             */
    /*    Ex.                                                      */
    /*                                                             */
    /*      /-\                                                    */
    /*      |@|                                                    */
    /*      \-/                                                    */
    /*                                                             */
    PCHAR2(96, '/',  S_expl_tl, "左上角爆炸", "", CLR_ORANGE)
    PCHAR2(97, '-',  S_expl_tc, "顶部中央爆炸", "", CLR_ORANGE)
    PCHAR2(98, '\\', S_expl_tr, "右上爆炸", "", CLR_ORANGE)
    PCHAR2(99, '|',  S_expl_ml, "左中爆炸", "", CLR_ORANGE)
    PCHAR2(100, ' ',  S_expl_mc, "正中爆炸", "", CLR_ORANGE)
    PCHAR2(101, '|',  S_expl_mr, "右中爆炸", "", CLR_ORANGE)
    PCHAR2(102, '\\', S_expl_bl, "左下爆炸", "", CLR_ORANGE)
    PCHAR2(103, '-', S_expl_bc, "正下爆炸", "", CLR_ORANGE)
    PCHAR2(104, '/', S_expl_br, "右下爆炸", "", CLR_ORANGE)
#undef PCHAR
#undef PCHAR2
#endif /* PCHAR_S_ENUM || PCHAR_PARSE || PCHAR_DRAWING || PCHAR_TILES
        * || DUMP_ENUMS_PCHAR */

#if defined(MONSYMS_S_ENUM)                         || defined(MONSYMS_DEFCHAR_ENUM)                || defined(MONSYMS_PARSE)                       || defined(MONSYMS_DRAWING)                     || defined(DUMP_ENUMS_MONSYMS)                  || defined(DUMP_ENUMS_MONSYMS_DEFCHAR)

/*
    MONSYM(idx, ch, sym desc)
        idx:     index used in enum
        ch:      character symbol
        sym:     symbol name for parsing purposes
        desc:    description
*/

#if defined(MONSYMS_S_ENUM)
/* sym.h */
#define MONSYM(idx, ch, basename, sym, desc) sym = idx,

#elif defined(MONSYMS_DEFCHAR_ENUM)
/* sym.h */
#define MONSYM(idx, ch, basename, sym,  desc) DEF_##basename = ch,

#elif defined(MONSYMS_PARSE)
/* symbols.c */
#define MONSYM(idx, ch, basename, sym, desc)     { SYM_MON, sym + SYM_OFF_M, #sym },

#elif defined(MONSYMS_DRAWING)
/* drawing.c */
#define MONSYM(idx, ch, basename, sym, desc) { DEF_##basename, "", desc },

/* allmain.c */
#elif defined(DUMP_ENUMS_MONSYMS)
#define MONSYM(idx, ch, basename, sym, desc) { sym, #sym },

#elif defined(DUMP_ENUMS_MONSYMS_DEFCHAR)
#define MONSYM(idx, ch, basename, sym, desc)     { DEF_##basename, "DEF_" #basename },

#endif

    MONSYM( 1, 'a', ANT, S_ANT,   "")
    MONSYM( 2, 'b', BLOB, S_BLOB, "蚂蚁或其他昆虫")
    MONSYM( 3, 'c', COCKATRICE, S_COCKATRICE, "粘液怪")
    MONSYM( 4, 'd', DOG, S_DOG, "蛇鹫")
    MONSYM( 5, 'e', EYE, S_EYE, "狗或其他犬类")
    MONSYM( 6, 'f', FELINE, S_FELINE, "眼睛或球体")
    MONSYM( 7, 'g', GREMLIN, S_GREMLIN, "猫或其他猫科动物")
    /* small humanoids: hobbit, dwarf */
    MONSYM( 8, 'h', HUMANOID, S_HUMANOID, "小妖")
    /* minor demons */
    MONSYM( 9, 'i', IMP, S_IMP, "类人生物")
    MONSYM(10, 'j', JELLY, S_JELLY, "小恶魔或低级恶魔")
    MONSYM(11, 'k', KOBOLD, S_KOBOLD, "水母")
    MONSYM(12, 'l', LEPRECHAUN, S_LEPRECHAUN, "科博尔德")
    MONSYM(13, 'm', MIMIC, S_MIMIC, "小精灵")
    MONSYM(14, 'n', NYMPH, S_NYMPH, "拟态怪")
    MONSYM(15, 'o', ORC, S_ORC, "仙女")
    MONSYM(16, 'p', PIERCER, S_PIERCER, "兽人")
    /* quadruped excludes horses */
    MONSYM(17, 'q', QUADRUPED, S_QUADRUPED, "穿刺者")
    MONSYM(18, 'r', RODENT, S_RODENT, "四足动物")
    MONSYM(19, 's', SPIDER, S_SPIDER, "啮齿类")
    MONSYM(20, 't', TRAPPER, S_TRAPPER, "蛛形类或蜈蚣")
    /* unicorn, horses */
    MONSYM(21, 'u', UNICORN, S_UNICORN, "上方陷阱或潜伏者")
    MONSYM(22, 'v', VORTEX, S_VORTEX, "独角兽或马")
    MONSYM(23, 'w', WORM, S_WORM, "漩涡")
    MONSYM(24, 'x', XAN, S_XAN, "蠕虫")
    /* yellow light, black light */
    MONSYM(25, 'y', LIGHT, S_LIGHT, "玄蚊或其他虚构昆虫")
    MONSYM(26, 'z', ZRUTY, S_ZRUTY, "光")
    MONSYM(27, 'A', ANGEL, S_ANGEL, "山区巨人")
    MONSYM(28, 'B', BAT, S_BAT, "天使类生物")
    MONSYM(29, 'C', CENTAUR, S_CENTAUR, "蝙蝠或鸟类")
    MONSYM(30, 'D', DRAGON, S_DRAGON, "半人马")
    /* elemental includes invisible stalker */
    MONSYM(31, 'E', ELEMENTAL, S_ELEMENTAL, "龙")
    MONSYM(32, 'F', FUNGUS, S_FUNGUS, "元素生物")
    MONSYM(33, 'G', GNOME, S_GNOME, "真菌或霉菌")
    /* large humanoid: giant, ettin, minotaur */
    MONSYM(34, 'H', GIANT, S_GIANT, "地精")
    MONSYM(35, 'I', INVISIBLE, S_invisible, "巨型类人生物")
    MONSYM(36, 'J', JABBERWOCK, S_JABBERWOCK, "隐形怪物")
    MONSYM(37, 'K', KOP, S_KOP, "颊脖龙")
    MONSYM(38, 'L', LICH, S_LICH, "笨拙警察")
    MONSYM(39, 'M', MUMMY, S_MUMMY, "巫妖")
    MONSYM(40, 'N', NAGA, S_NAGA, "木乃伊")
    MONSYM(41, 'O', OGRE, S_OGRE, "娜迦")
    MONSYM(42, 'P', PUDDING, S_PUDDING, "食人魔")
    MONSYM(43, 'Q', QUANTMECH, S_QUANTMECH, "布丁或粘液")
    MONSYM(44, 'R', RUSTMONST, S_RUSTMONST, "量子机械师")
    MONSYM(45, 'S', SNAKE, S_SNAKE, "锈蚀怪或破除魔法者")
    MONSYM(46, 'T', TROLL, S_TROLL, "蛇")
    /* umber hulk */
    MONSYM(47, 'U', UMBER, S_UMBER, "巨魔")
    MONSYM(48, 'V', VAMPIRE, S_VAMPIRE, "赭色巨兽")
    MONSYM(49, 'W', WRAITH, S_WRAITH, "吸血鬼")
    MONSYM(50, 'X', XORN, S_XORN, "幽灵")
    /* apelike creature includes owlbear, monkey */
    MONSYM(51, 'Y', YETI, S_YETI, "佐恩")
    MONSYM(52, 'Z', ZOMBIE, S_ZOMBIE, "类猿生物")
    MONSYM(53, '@', HUMAN, S_HUMAN, "僵尸")
    /* space symbol*/
    MONSYM(54, ' ', GHOST, S_GHOST, "人类或精灵")
    MONSYM(55, '\'', GOLEM, S_GOLEM, "鬼魂")
    MONSYM(56, '&', DEMON, S_DEMON, "傀儡")
    /* fish */
    MONSYM(57, ';', EEL, S_EEL,  "大恶魔")
    /* reptiles */
    MONSYM(58, ':', LIZARD, S_LIZARD, "海怪")
    MONSYM(59, '~', WORM_TAIL, S_WORM_TAIL, "蜥蜴")
    MONSYM(60, ']', MIMIC_DEF, S_MIMIC_DEF, "长虫尾")

#undef MONSYM
#endif /* MONSYMS_S_ENUM || MONSYMS_DEFCHAR_ENUM || MONSYMS_PARSE
        * || MONSYMS_DRAWING || DUMP_ENUMS_MONSYMS)
        * || DUMP_ENUMS_MONSYMS_DEFCHAR */

#if defined(OBJCLASS_S_ENUM)                        || defined(OBJCLASS_DEFCHAR_ENUM)               || defined(OBJCLASS_CLASS_ENUM)                 || defined(OBJCLASS_PARSE)                      || defined(OBJCLASS_DRAWING)                    || defined(DUMP_ENUMS_OBJCLASS_DEFCHARS)        || defined(DUMP_ENUMS_OBJCLASS_CLASSES)         || defined(DUMP_ENUMS_OBJCLASS_SYMS)

/*
    OBJCLASS(idx, ch, basename, sym, name, explain)
        idx:      index used in enum
        ch:       default character
        basename: unadorned base name of objclass, used
                  to construct enums through suffixes/prefixes
        sym:      symbol name for enum and parsing purposes
        name:     used in object_detect()
        explain:  used in do_look()

    OBJCLASS2(idx, ch, basename, sname, sym, name, explain)
        idx:      index used in enum
        ch:       default character
        basename: unadorned base name of objclass, used
                  to construct enums through suffixes/prefixes
        sname:    hardcoded *_SYM value for this entry (required
                  only because basename and GOLD_SYM differ
        sym:      symbol name for enum and parsing purposes
        name:     used in object_detect()
        explain:  used in do_look()
*/

#if defined(OBJCLASS_CLASS_ENUM)
/* objclass.h */
#define OBJCLASS(idx, ch, basename, sym, name, explain)     basename##_CLASS = idx,

#elif defined(OBJCLASS_DEFCHAR_ENUM)
/* objclass.h */
#define OBJCLASS(idx, ch, basename, sym, name, explain)     basename##_SYM = ch,

#elif defined(OBJCLASS_S_ENUM)
/* objclass.h */
#define OBJCLASS(idx, ch, basename, sym, name, explain)     sym = idx,

#elif defined(OBJCLASS_PARSE)
/* symbols.c */
#define OBJCLASS(idx, ch, basename, sym, name, explain)     { SYM_OC, sym + SYM_OFF_O, #sym },

#elif defined(OBJCLASS_DRAWING)
/* drawing.c */
#define OBJCLASS(idx, ch, basename, sym, name, explain)     { basename##_SYM, name, explain },

#elif defined(DUMP_ENUMS_OBJCLASS_DEFCHARS)
/* allmain.c */
#define OBJCLASS(idx, ch, basename, sym, name, explain)     { basename##_SYM, #basename "_SYM" },

#elif defined(DUMP_ENUMS_OBJCLASS_CLASSES)
/* allmain.c */
#define OBJCLASS(idx, ch, basename, sym, name, explain)     { basename##_CLASS, #basename "_CLASS" },

#elif defined(DUMP_ENUMS_OBJCLASS_SYMS)
/* allmain.c */
#define OBJCLASS(idx, ch, basename, sym, name, explain)     { sym , #sym },
#endif

/* OBJCLASS with extra arg */
#if defined(OBJCLASS_DEFCHAR_ENUM)
#define OBJCLASS2(idx, ch, basename, sname, sym, name, explain)     sname = ch,
#elif defined(OBJCLASS_DRAWING)
#define OBJCLASS2(idx, ch, basename, sname, sym, name, explain)     { sname, name, explain },
#elif defined(DUMP_ENUMS_OBJCLASS_DEFCHARS)
#define OBJCLASS2(idx, ch, basename, sname, sym, name, explain)     { sname, #sname },
#elif defined(DUMP_ENUMS_OBJCLASS_CLASSES)
#define OBJCLASS2(idx, ch, basename, sname, sym, name, explain)     { basename##_CLASS, #basename "_CLASS" },
#elif defined(DUMP_ENUMS_OBJCLASS_SYMS)
#define OBJCLASS2(idx, ch, basename, sname, sym, name, explain)     { sym , #sym },
#else
#define OBJCLASS2(idx, ch, basename, sname, sym, name, explain)     OBJCLASS(idx, ch, basename, sym, name, explain)
#endif

    OBJCLASS( 1,  ']', ILLOBJ, S_strange_obj, "illegal objects",
                                              "strange object")
    OBJCLASS( 2,  ')', WEAPON, S_weapon, "weapons", "weapon")
    OBJCLASS( 3,  '[', ARMOR,  S_armor, "armor", "suit or piece of armor")
    OBJCLASS( 4,  '=', RING,   S_ring, "rings", "ring")
    OBJCLASS( 5,  '"', AMULET, S_amulet, "amulets", "amulet")
    OBJCLASS( 6,  '(', TOOL,   S_tool, "tools",
                                       "useful item (pick-axe, key, lamp...)")
    OBJCLASS( 7,  '%', FOOD,   S_food, "food", "piece of food")
    OBJCLASS( 8,  '!', POTION, S_potion, "potions", "potion")
    OBJCLASS( 9,  '?', SCROLL, S_scroll, "scrolls", "scroll")
    OBJCLASS(10,  '+', SPBOOK, S_book, "spellbooks", "spellbook")
    OBJCLASS(11,  '/', WAND,   S_wand, "wands", "wand")
    OBJCLASS2(12, '$', COIN,   GOLD_SYM, S_coin, "coins", "pile of coins")
    OBJCLASS(13,  '*', GEM,    S_gem, "rocks", "gem or rock")
    OBJCLASS(14,  '`', ROCK,   S_rock, "large stones", "boulder or statue")
    OBJCLASS(15,  '0', BALL,   S_ball, "iron balls", "iron ball")
    OBJCLASS(16,  '_', CHAIN,  S_chain, "chains", "iron chain")
    OBJCLASS(17,  '.', VENOM,  S_venom, "venoms", "splash of venom")

#undef OBJCLASS
#undef OBJCLASS2
#endif /* OBJCLASS_S_ENUM || OBJCLASS_DEFCHAR_ENUM || OBJCLASS_CLASS_ENUM
        * || OBJCLASS_PARSE || OBJCLASS_DRAWING
        * || DUMP_ENUMS_OBJCLASS_DEFCHARS || DUMP_ENUMS_OBJCLASS_CLASSES
        * || DUMP_ENUMS_OBJCLASS_SYMS */

#ifdef DEBUG
#if !defined(PCHAR_S_ENUM) && !defined(PCHAR_DRAWING)     && !defined(PCHAR_PARSE) && !defined(PCHAR_TILES)     && !defined(DUMP_ENUMS_PCHAR)     && !defined(MONSYMS_S_ENUM) && !defined(MONSYMS_DEFCHAR_ENUM)     && !defined(MONSYMS_PARSE) && !defined(MONSYMS_DRAWING)     && !defined(DUMP_ENUMS_MONSYMS)     && !defined(DUMP_ENUMS_MONSYMS_DEFCHAR)     && !defined(OBJCLASS_S_ENUM) && !defined(OBJCLASS_DEFCHAR_ENUM)     && !defined(OBJCLASS_CLASS_ENUM) && !defined(OBJCLASS_PARSE)     && !defined (OBJCLASS_DRAWING)     && !defined(DUMP_ENUMS_OBJCLASS_DEFCHARS)     && !defined(DUMP_ENUMS_OBJCLASS_CLASSES)     && !defined(DUMP_ENUMS_OBJCLASS_SYMS)
#error Non-productive inclusion of defsym.h
#endif
#endif /* DEBUG */

/* end of defsym.h */
