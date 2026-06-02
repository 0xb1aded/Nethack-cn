/* NetHack 5.0	objnam.c	$NHDT-Date: 1745114235 2025/04/19 17:57:15 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.453 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"

/* "an uncursed greased partly eaten guardian naga hatchling [corpse]" */
#define PREFIX 80 /* (56) */
#define SCHAR_LIM 127
#define NUMOBUF 12

struct _readobjnam_data {
    struct obj *otmp;
    char *bp;
    char *origbp;
    char oclass;
    char *un, *dn, *actualn;
    const char *name;
    char *p;
    int cnt, spe, spesgn, typ, very, rechrg;
    int blessed, uncursed, iscursed, ispoisoned, isgreased;
    int eroded, eroded2, erodeproof, locked, unlocked, broken, real, fake;
    int halfeaten, mntmp, contents;
    int islit, unlabeled, ishistoric, isdiluted, trapped;
    int doorless, open, closed, looted;
    int tmp, tinv, tvariety, mgend;
    int wetness, gsize;
    int ftype;
    boolean zombify;
    char globbuf[BUFSZ];
    char fruitbuf[BUFSZ];
};

staticfn char *strprepend(char *, const char *) NONNULL NONNULLARG1;
staticfn char *nextobuf(void) NONNULL;
staticfn void releaseobuf(char *) NONNULLARG1;
staticfn void xcalled(char *, int, const char *, const char *);
staticfn void xcallede(char *, int, const char *, const char *);
staticfn char *xname_flags(struct obj *, unsigned);
staticfn char *xename_flags(struct obj *, unsigned);
staticfn char *minimal_xname(struct obj *);
staticfn char *minimal_xename(struct obj *);
staticfn void add_erosion_words(struct obj *, char *);
staticfn char *doname_base(struct obj *obj, unsigned);
staticfn char *doename_base(struct obj *obj, unsigned);
staticfn boolean singplur_lookup(char *, char *, boolean,
                               const char *const *);
staticfn char *singplur_compound(char *);
staticfn boolean ch_ksound(const char *basestr);
staticfn boolean badman(const char *, boolean);
staticfn boolean wishymatch(const char *, const char *, boolean);
staticfn boolean wishyematch(const char *, const char *, boolean);
staticfn short rnd_otyp_by_wpnskill(schar);
staticfn short rnd_otyp_by_namedesc(const char *, char, int);
staticfn short rnd_otyp_by_enameedesc(const char *, char, int);
staticfn void set_wallprop_from_str(char *) NONNULLARG1;
staticfn struct obj *wizterrainwish(struct _readobjnam_data *);
staticfn void dbterrainmesg(const char *, coordxy, coordxy) NONNULLARG1;
staticfn void readobjnam_init(char *, struct _readobjnam_data *);
staticfn int readobjnam_preparse(struct _readobjnam_data *);
staticfn int readobjenam_preparse(struct _readobjnam_data *);
staticfn void readobjnam_parse_charges(struct _readobjnam_data *);
staticfn void readobjenam_parse_charges(struct _readobjnam_data *);
staticfn int readobjnam_postparse1(struct _readobjnam_data *);
staticfn int readobjenam_postparse1(struct _readobjnam_data *);
staticfn int readobjnam_postparse2(struct _readobjnam_data *);
staticfn int readobjenam_postparse2(struct _readobjnam_data *);
staticfn int readobjnam_postparse3(struct _readobjnam_data *);
staticfn int readobjenam_postparse3(struct _readobjnam_data *);

struct Jitem {
    int item;
    const char *name;
};

#define BSTRCMPI(base, ptr, str) ((ptr) < base || strcmpi((ptr), str))
#define BSTRNCMPI(base, ptr, str, num) \
    ((ptr) < base || strncmpi((ptr), str, num))
#define Strcasecpy(dst, src) (void) strcasecpy(dst, src)
#define Strncat(dst, src, cnt) (void) strncat(dst, src, cnt)

/* Concat(): append text to base, adjusted by delta, with bounds checking
   via a pair of behind-the-scenes variables; delta is either 0 for normal
   concatenation or 1 to replace the final character with something */
#define Concat(base, delta, text) \
    do {                                                                \
        Strncat(base ## _eos - delta, text, base ## spaceleft + delta); \
        ConcUpdate(base);                                               \
    } while (0)
#define ConcatF1(base, delta, fmt, arg1) \
    do {                                                                \
        Snprintf(base ## _eos - delta, base ## spaceleft + delta,       \
                 fmt, arg1);                                            \
        ConcUpdate(base);                                               \
    } while (0)
#define ConcatF2(base, delta, fmt, arg1, arg2) \
    do {                                                                \
        Snprintf(base ## _eos - delta, base ## spaceleft + delta,       \
                 fmt, arg1, arg2);                                      \
        ConcUpdate(base);                                               \
    } while (0)
#define ConcUpdate(base) \
    base ## _eos = eos(base),                                           \
    /* convert signed ptrdiff_t to unsigned size_t */                   \
    base ## spaceleft = (size_t) (base ## _end - base ## _eos)

/* true for gems/rocks that should have " stone" appended to their names */
#define GemStone(typ)                                                  \
    (typ == FLINT                                                      \
     || (objects[typ].oc_material == GEMSTONE                          \
         && (typ != DILITHIUM_CRYSTAL && typ != RUBY && typ != DIAMOND \
             && typ != SAPPHIRE && typ != BLACK_OPAL && typ != EMERALD \
             && typ != OPAL)))

static const struct Jitem Japanese_items[] = {
    { SHORT_SWORD, "胁差" },
    { BROADSWORD, "忍者刀" },
    { FLAIL, "双截棍" },
    { GLAIVE, "薙刀" },
    { LOCK_PICK, "御凿" },
    { WOODEN_HARP, "琴" },
    { MAGIC_HARP, "魔琴" },
    { KNIFE, "刺刀" },
    { PLATE_MAIL, "短甲" },
    { HELMET, "兜" },
    { LEATHER_GLOVES, "弽" },
    { FOOD_RATION, "兵粮" },
    { POT_BOOZE, "烧酒" },
    { 0, "" }
};

static const struct Jitem eJapanese_items[] = {
    { SHORT_SWORD, "wakizashi" },
    { BROADSWORD, "ninja-to" },
    { FLAIL, "nunchaku" },
    { GLAIVE, "naginata" },
    { LOCK_PICK, "osaku" },
    { WOODEN_HARP, "koto" },
    { MAGIC_HARP, "magic koto" },
    { KNIFE, "shito" },
    { PLATE_MAIL, "tanko" },
    { HELMET, "kabuto" },
    { LEATHER_GLOVES, "yugake" },
    { FOOD_RATION, "gunyoki" },
    { POT_BOOZE, "sake" },
    { 0, "" }
};

const char *
Japanese_item_name(int i, const char *ordinaryname)
{
    const struct Jitem *j = Japanese_items;

    while (j->item) {
        if (i == j->item)
            return j->name;
        j++;
    }
    return ordinaryname;
}

const char *
Japanese_item_ename(int i, const char *ordinaryname)
{
    const struct Jitem *j = eJapanese_items;

    while (j->item) {
        if (i == j->item)
            return j->name;
        j++;
    }
    return ordinaryname;
}

staticfn char *
strprepend(char *s, const char *pref)
{
    char star_s = *s;
    int i = (int) strlen(pref);

    if (i > PREFIX) {
        impossible("PREFIX too short (for %d).", i);
        return s;
    }
    copynchars(s - i, pref, i + 1);
    *s = star_s;
    return s - i;
}

/* manage a pool of BUFSZ buffers, so callers don't have to */
static char NEARDATA obufs[NUMOBUF][BUFSZ];
static int obufidx = 0;

staticfn char *
nextobuf(void)
{
    obufidx = (obufidx + 1) % NUMOBUF;
    return obufs[obufidx];
}

/* put the most recently allocated buffer back if possible */
staticfn void
releaseobuf(char *bufp)
{
    /* caller may not know whether bufp is the most recently allocated
       buffer; if it isn't, do nothing; note that because of the somewhat
       obscure PREFIX handling for object name formatting by xname(),
       the pointer our caller has and is passing to us might be into the
       middle of an obuf rather than the address returned by nextobuf() */
    if (bufp >= obufs[obufidx]
        && bufp < obufs[obufidx] + sizeof obufs[obufidx]) /* obufs[][BUFSZ] */
        obufidx = (obufidx - 1 + NUMOBUF) % NUMOBUF;
}

/* used by display_pickinv (invent.c, main whole-inventory routine) to
   release each successive doname() result in order to try to avoid
   clobbering all the obufs when 'perm_invent' is enabled and updated
   while one or more obufs have been allocated but not released yet */
void
maybereleaseobuf(char *obuffer)
{
    releaseobuf(obuffer);

    /*
     * An example from 3.6.x where all obufs got clobbered was when a
     * monster used a bullwhip to disarm the hero of a two-handed weapon:
     * "The ogre lord yanks Cleaver from your corpses!"
     |
     | hand = body_part(HAND);
     | if (use_plural)      // switches 'hand' from static buffer to an obuf
     |   hand = makeplural(hand);
      ...
     | release_worn_item(); // triggers full inventory update for perm_invent
      ...
     | pline(..., hand);    // the obuf[] for "hands" was clobbered with the
     |                      //+ partial formatting of an item from invent
     *
     * Another example was from writing a scroll without room in invent to
     * hold it after being split from a stack of blank scrolls:
     * "Oops!  food rations out of your grasp!"
     * hold_another_object() was passed 'the(aobjnam(newscroll, "slip"))'
     * as an argument and that should have yielded
     * "Oops!  The scroll of <foo> slips out of your grasp!"
     * but attempting to add the item to inventory triggered update for
     * perm_invent and the result from 'the(...)' was clobbered by partial
     * formatting of some inventory item.  [It happened in a shop and the
     * shk claimed ownership of the new scroll, but that wasn't relevant.]
     * That got fixed earlier, by delaying update_inventory() during
     * hold_another_object() rather than by avoiding using all the obufs.
     */
}

char *
obj_typename(int otyp)
{
    char *buf = nextobuf();
    struct objclass *ocl = &objects[otyp];
    const char *actualn = OBJ_NAME(*ocl);
    const char *dn = OBJ_DESCR(*ocl);
    const char *un = ocl->oc_uname;
    int nn = ocl->oc_name_known;

    if (Role_if(PM_SAMURAI)) {
        actualn = Japanese_item_name(otyp, actualn);
        if (otyp == WOODEN_HARP || otyp == MAGIC_HARP)
            dn = "琴";
    }
    /* generic items don't have an actual-name; we shouldn't ever be called
       for those; pacify static analyzer without resorting to impossible() */
    if (!actualn)
        actualn = (otyp > 0 && otyp < MAXOCLASSES) ? "通用" : "物品?";

    buf[0] = '\0'; /* redundant */
    /* here for ring/scroll/potion/wand */
    if (nn) {
        Sprintf(eos(buf), "%s", actualn);
    }
    switch (ocl->oc_class) {
    case COIN_CLASS:
        return strcpy(buf, actualn); /* "gold piece" */
    case POTION_CLASS:
        Strcat(buf, "药水");
        break;
    case SCROLL_CLASS:
        Strcat(buf, "卷轴");
        break;
    case WAND_CLASS:
        Strcat(buf, "魔杖");
        break;
    case SPBOOK_CLASS:
        if (otyp != SPE_NOVEL) {
            Strcat(buf, "魔法书");
        } else {
            Strcat(buf, !nn ? "书" : "小说");
            nn = 0;
        }
        break;
    case RING_CLASS:
        Strcat(buf, "戒指");
        break;
    case AMULET_CLASS:
        if (nn)
            Strcat(buf, actualn);
        else
            Strcat(buf, "护身符");
        if (un)
            xcalled(buf, BUFSZ - (dn ? (int) strlen(dn) + 3 : 0), "", un);
        if (dn)
            Sprintf(eos(buf), " (%s)", dn);
        return buf;
    case ARMOR_CLASS:
        if (objects[otyp].oc_armcat == ARM_GLOVES
            || objects[otyp].oc_armcat == ARM_BOOTS)
            Strcpy(buf, "一双");
        else if (otyp >= GRAY_DRAGON_SCALES && otyp <= YELLOW_DRAGON_SCALES)
            Strcpy(buf, "一套");
        FALLTHROUGH;
        /*FALLTHRU*/
    default:
        if (nn) {
            Strcat(buf, actualn);
            if (GemStone(otyp))
                Strcat(buf, "石");
            if (un) /* 3: length of " (" + ")" which will enclose 'dn' */
                xcalled(buf, BUFSZ - (dn ? (int) strlen(dn) + 3 : 0), "", un);
            if (dn)
                Sprintf(eos(buf), " (%s)", dn);
        } else {
            Strcat(buf, dn ? dn : actualn);
            if (ocl->oc_class == GEM_CLASS)
                Strcat(buf,
                       (ocl->oc_material == MINERAL) ? "石" : "宝石");
            if (un)
                xcalled(buf, BUFSZ, "", un);
        }
        return buf;
    }
    if (nn) {
        if (ocl->oc_unique)
        {
            Strcpy(buf, actualn);
        }
    }
    if (un) /* 3: length of " (" + ")" which will enclose 'dn' */
        xcalled(buf, BUFSZ - (dn ? (int) strlen(dn) + 3 : 0), "", un);
    if (dn)
        Sprintf(eos(buf), " (%s)", dn);
    return buf;
}

char *
obj_etypename(int otyp)
{
    char *buf = nextobuf();
    struct objclass *ocl = &objects[otyp];
    const char *actualn = OBJ_ENAME(*ocl);
    const char *dn = OBJ_EDESCR(*ocl);
    const char *un = ocl->oc_uname;
    int nn = ocl->oc_name_known;

    if (Role_if(PM_SAMURAI)) {
        actualn = Japanese_item_ename(otyp, actualn);
        if (otyp == WOODEN_HARP || otyp == MAGIC_HARP)
            dn = "koto";
    }
    /* generic items don't have an actual-name; we shouldn't ever be called
       for those; pacify static analyzer without resorting to impossible() */
    if (!actualn)
        actualn = (otyp > 0 && otyp < MAXOCLASSES) ? "generic" : "object?";

    buf[0] = '\0'; /* redundant */
    switch (ocl->oc_class) {
    case COIN_CLASS:
        return strcpy(buf, actualn); /* "gold piece" */
    case POTION_CLASS:
        Strcpy(buf, "potion");
        break;
    case SCROLL_CLASS:
        Strcpy(buf, "scroll");
        break;
    case WAND_CLASS:
        Strcpy(buf, "wand");
        break;
    case SPBOOK_CLASS:
        if (otyp != SPE_NOVEL) {
            Strcpy(buf, "spellbook");
        } else {
            Strcpy(buf, !nn ? "book" : "novel");
            nn = 0;
        }
        break;
    case RING_CLASS:
        Strcpy(buf, "ring");
        break;
    case AMULET_CLASS:
        if (nn)
            Strcpy(buf, actualn);
        else
            Strcpy(buf, "amulet");
        if (un)
            xcalled(buf, BUFSZ - (dn ? (int) strlen(dn) + 3 : 0), "", un);
        if (dn)
            Sprintf(eos(buf), " (%s)", dn);
        return buf;
    case ARMOR_CLASS:
        if (objects[otyp].oc_armcat == ARM_GLOVES
            || objects[otyp].oc_armcat == ARM_BOOTS)
            Strcpy(buf, "pair of ");
        else if (otyp >= GRAY_DRAGON_SCALES && otyp <= YELLOW_DRAGON_SCALES)
            Strcpy(buf, "set of ");
        FALLTHROUGH;
        /*FALLTHRU*/
    default:
        if (nn) {
            Strcat(buf, actualn);
            if (GemStone(otyp))
                Strcat(buf, " stone");
            if (un) /* 3: length of " (" + ")" which will enclose 'dn' */
                xcalled(buf, BUFSZ - (dn ? (int) strlen(dn) + 3 : 0), "", un);
            if (dn)
                Sprintf(eos(buf), " (%s)", dn);
        } else {
            Strcat(buf, dn ? dn : actualn);
            if (ocl->oc_class == GEM_CLASS)
                Strcat(buf,
                       (ocl->oc_material == MINERAL) ? " stone" : " gem");
            if (un)
                xcalled(buf, BUFSZ, "", un);
        }
        return buf;
    }
    /* here for ring/scroll/potion/wand */
    if (nn) {
        if (ocl->oc_unique)
            Strcpy(buf, actualn); /* avoid spellbook of Book of the Dead */
        else
            Sprintf(eos(buf), " of %s", actualn);
    }
    if (un) /* 3: length of " (" + ")" which will enclose 'dn' */
        xcalled(buf, BUFSZ - (dn ? (int) strlen(dn) + 3 : 0), "", un);
    if (dn)
        Sprintf(eos(buf), " (%s)", dn);
    return buf;
}

/* less verbose result than obj_typename(); either the actual name
   or the description (but not both); user-assigned name is ignored */
char *
simple_typename(int otyp)
{
    char *bufp, *pp, *save_uname = objects[otyp].oc_uname;

    objects[otyp].oc_uname = 0; /* suppress any name given by user */
    bufp = obj_typename(otyp);
    objects[otyp].oc_uname = save_uname;
    if ((pp = strstri(bufp, " (")) != 0)
        *pp = '\0'; /* strip the appended description */
    return bufp;
}

char *
simple_etypename(int otyp)
{
    char *bufp, *pp, *save_uname = objects[otyp].oc_uname;

    objects[otyp].oc_uname = 0; /* suppress any name given by user */
    bufp = obj_etypename(otyp);
    objects[otyp].oc_uname = save_uname;
    if ((pp = strstri(bufp, " (")) != 0)
        *pp = '\0'; /* strip the appended description */
    return bufp;
}

/* typename for debugging feedback where data involved might be suspect */
char *
safe_typename(int otyp)
{
    unsigned save_nameknown;
    char *res = 0;

    if (otyp < STRANGE_OBJECT || otyp >= NUM_OBJECTS
        || !OBJ_NAME(objects[otyp])) {
        res = nextobuf();
        Sprintf(res, "glorkum[%d]", otyp);
        impossible("safe_typename: %s", res);
    } else {
        /* force it to be treated as fully discovered */
        save_nameknown = objects[otyp].oc_name_known;
        objects[otyp].oc_name_known = 1;
        res = simple_typename(otyp);
        objects[otyp].oc_name_known = save_nameknown;
    }
    return res;
}

char *
safe_etypename(int otyp)
{
    unsigned save_nameknown;
    char *res = 0;

    if (otyp < STRANGE_OBJECT || otyp >= NUM_OBJECTS
        || !OBJ_ENAME(objects[otyp])) {
        res = nextobuf();
        Sprintf(res, "glorkum[%d]", otyp);
        impossible("safe_typename: %s", res);
    } else {
        /* force it to be treated as fully discovered */
        save_nameknown = objects[otyp].oc_name_known;
        objects[otyp].oc_name_known = 1;
        res = simple_etypename(otyp);
        objects[otyp].oc_name_known = save_nameknown;
    }
    return res;
}

boolean
obj_is_pname(struct obj *obj)
{
    if (!obj->oartifact || !has_oname(obj))
        return FALSE;
    if (!program_state.gameover && !iflags.override_ID) {
        if (not_fully_identified(obj))
            return FALSE;
    }
    return TRUE;
}

/* Give the name of an object seen at a distance.  Unlike xname/doname,
   we usually don't want to set dknown if it's not set already. */
char *
distant_name(
    struct obj *obj, /* object to be formatted */
    char *(*func)(OBJ_P)) /* formatting routine (usually xname or doname) */
{
    char *str;
    unsigned save_oid;
    coordxy ox = 0, oy = 0;
        /*
         * (r * r): square of the x or y distance;
         * (r * r) * 2: sum of squares of both x and y distances
         * (r * r) * 2 - r: instead of a square extending from the hero,
         * round the corners (so shorter distance imposed for diagonal).
         *
         * distu() matrix covering a range of 3+ for one quadrant:
         *  16 17  -  -  -
         *   9 10 13 18  -
         *   4  5  8 13  -
         *   1  2  5 10 17
         *   @  1  4  9 16
         * Theoretical r==1 would yield 1.
         * r==2 yields 6, functionally equivalent to 5, a knight's jump,
         * r==3, the xray range of the Eyes of the Overworld, yields 15.
         */
    int r = (u.xray_range > 2) ? u.xray_range : 2,
        neardist = (r * r) * 2 - r; /* same as r*r + r*(r-1) */

   /* setting o_id to 0 prevents xname() from adding T-shirt or apron
      slogan, Hawaiian shirt motif, or candy wrapper label when called
      with 'program_state.gameover' set; we want this suppression for
      html-dump (not implemented in nethack) to prevent object-on-map
      tooltips from including that extra text; also guards against a
      potential change to minimal_xname() [indirectly used by attribute
      disclosure] that propagates o_id rather than leave it 0, and
      against a potential extra chance to browse the map with getpos()
      during final disclosure (not currently implemented, nor planned) */
    save_oid = obj->o_id;
    if (program_state.gameover)
        obj->o_id = 0;

    /* this maybe-nearby part used to be replicated in multiple callers */
    if (get_obj_location(obj, &ox, &oy, 0) && cansee(ox, oy)
        && (obj->oartifact || distu(ox, oy) <= neardist)) {
        /* side-effects:  treat as having been seen up close;
           cansee() is True hence hero isn't Blind so if 'func' is
           the usual doname or xname, obj->dknown will become set
           and then for an artifact, find_artifact() will be called */
        str = (*func)(obj);
    } else {
        /* prior to 3.6.1, this used to save current blindness state,
           explicitly set state to hero-is-blind, make the call (which
           won't set obj->dknown when blind), then restore the saved
           value; but the Eyes of the Overworld override blindness and
           would let characters wearing them get obj->dknown set for
           distant items, so the external flag was added */
        ++gd.distantname;
        str = (*func)(obj);
        --gd.distantname;
    }

    obj->o_id = save_oid; /* reset to normal */

    return str;
}

/* convert player specified fruit name into corresponding fruit juice name
   ("slice of pizza" -> "pizza juice" rather than "slice of pizza juice") */
char *
fruitname(
    boolean juice) /* whether or not to append " juice" to the name */
{
    char *buf = nextobuf();
    const char *fruit_nam = strstri(svp.pl_fruit, " of ");

    if (fruit_nam)
        fruit_nam += 4; /* skip past " of " */
    else
        fruit_nam = svp.pl_fruit; /* use it as is */

    Sprintf(buf, "%s%s", makesingular(fruit_nam), juice ? " 汁" : "");
    return buf;
}

/* look up a named fruit by index (1..127) */
struct fruit *
fruit_from_indx(int indx)
{
    struct fruit *f;

    for (f = gf.ffruit; f; f = f->nextf)
        if (f->fid == indx)
            break;
    return f;
}

/* look up a named fruit by name */
struct fruit *
fruit_from_name(
    const char *fname,
    boolean exact, /* False: prefix or exact match, True: exact match only */
    int *highest_fid) /* optional output; only valid if 'fname' isn't found */
{
    struct fruit *f, *tentativef;
    char *altfname;
    unsigned k;
    /*
     * note: named fruits are case-sensitive...
     */

    if (highest_fid)
        *highest_fid = 0;
    /* first try for an exact match */
    for (f = gf.ffruit; f; f = f->nextf)
        if (!strcmp(f->fname, fname))
            return f;
        else if (highest_fid && f->fid > *highest_fid)
            *highest_fid = f->fid;

    /* didn't match as-is; if caller is willing to accept a prefix
       match, try to find one; we want to find the longest prefix that
       matches, not the first */
    if (!exact) {
        tentativef = 0;
        for (f = gf.ffruit; f; f = f->nextf) {
            k = Strlen(f->fname);
            if (!strncmp(f->fname, fname, k)
                && (!fname[k] || fname[k] == ' ')
                && (!tentativef || k > strlen(tentativef->fname)))
                tentativef = f;
        }
        f = tentativef;
    }
    /* if we still don't have a match, try singularizing the target;
       for exact match, that's trivial, but for prefix, it's hard */
    if (!f) {
        altfname = makesingular(fname);
        for (f = gf.ffruit; f; f = f->nextf) {
            if (!strcmp(f->fname, altfname))
                break;
        }
        releaseobuf(altfname);
    }
    if (!f && !exact) {
        char fnamebuf[BUFSZ], *p;
        unsigned fname_k = Strlen(fname); /* length of assumed plural fname */

        tentativef = 0;
        for (f = gf.ffruit; f; f = f->nextf) {
            k = Strlen(f->fname);
            /* reload fnamebuf[] each iteration in case it gets modified;
               there's no need to recalculate fname_k */
            Strcpy(fnamebuf, fname);
            /* bug? if singular of fname is longer than plural,
               failing the 'fname_k > k' test could skip a viable
               candidate; unfortunately, we can't singularize until
               after stripping off trailing stuff and we can't get
               accurate fname_k until fname has been singularized;
               compromise and use 'fname_k >= k' instead of '>',
               accepting 1 char length discrepancy without risking
               false match (I hope...) */
            if (fname_k >= k && (p = strchr(&fnamebuf[k], ' ')) != 0) {
                *p = '\0'; /* truncate at 1st space past length of f->fname */
                altfname = makesingular(fnamebuf);
                k = Strlen(altfname); /* actually revised 'fname_k' */
                if (!strcmp(f->fname, altfname)
                    && (!tentativef || k > strlen(tentativef->fname)))
                    tentativef = f;
                releaseobuf(altfname); /* avoid churning through all obufs */
            }
        }
        f = tentativef;
    }
    return f;
}

/* sort the named-fruit linked list by fruit index number */
void
reorder_fruit(boolean forward)
{
    struct fruit *f, *allfr[1 + 127];
    int i, j, k = SIZE(allfr);

    for (i = 0; i < k; ++i)
        allfr[i] = (struct fruit *) 0;
    for (f = gf.ffruit; f; f = f->nextf) {
        /* without sanity checking, this would reduce to 'allfr[f->fid]=f' */
        j = f->fid;
        if (j < 1 || j >= k) {
            impossible("reorder_fruit: fruit index (%d) out of range", j);
            return; /* don't sort after all; should never happen... */
        } else if (allfr[j]) {
            impossible("reorder_fruit: duplicate fruit index (%d)", j);
            return;
        }
        allfr[j] = f;
    }
    gf.ffruit = 0; /* reset linked list; we're rebuilding it from scratch */
    /* slot [0] will always be empty; must start 'i' at 1 to avoid
       [k - i] being out of bounds during first iteration */
    for (i = 1; i < k; ++i) {
        /* for forward ordering, go through indices from high to low;
           for backward ordering, go from low to high */
        j = forward ? (k - i) : i;
        if (allfr[j]) {
            allfr[j]->nextf = gf.ffruit;
            gf.ffruit = allfr[j];
        }
    }
}

/* add "<pfx> called <sfx>" to end of buf, truncating if necessary */
staticfn void
xcalled(
    char *buf,       /* eos(obuf) or eos(&obuf[PREFIX]) */
    int siz,         /* BUFSZ or BUFSZ-PREFIX */
    const char *pfx, /* usually class string, sometimes more specific */
    const char *sfx) /* user assigned type name */
{
    int bufsiz = siz - 1 - (int) strlen(buf),
        pfxlen = (int) (strlen(pfx) + sizeof " called " - sizeof "");

    if (pfxlen > bufsiz)
        panic("xcalled: not enough room for prefix (%d > %d)",
              pfxlen, bufsiz);

    Sprintf(eos(buf), "%s,被称为%.*s", pfx, bufsiz - pfxlen, sfx);
}

staticfn void
xcallede(
    char *buf,       /* eos(obuf) or eos(&obuf[PREFIX]) */
    int siz,         /* BUFSZ or BUFSZ-PREFIX */
    const char *pfx, /* usually class string, sometimes more specific */
    const char *sfx) /* user assigned type name */
{
    int bufsiz = siz - 1 - (int) strlen(buf),
        pfxlen = (int) (strlen(pfx) + sizeof " called " - sizeof "");

    if (pfxlen > bufsiz)
        panic("xcalled: not enough room for prefix (%d > %d)",
              pfxlen, bufsiz);

    Sprintf(eos(buf), "%s called %.*s", pfx, bufsiz - pfxlen, sfx);
}

char *
xname(struct obj *obj)
{
    return xname_flags(obj, CXN_NORMAL);
}

char *
xename(struct obj *obj)
{
    return xename_flags(obj, CXN_NORMAL);
}

staticfn char *
xname_flags(
    struct obj *obj,
    unsigned cxn_flags) /* bitmask of CXN_xxx values */
{
    char *buf;
    char *obufp, *buf_end, *buf_eos;
    size_t bufspaceleft;
    int typ = obj->otyp;
    struct objclass *ocl = &objects[typ];
    int nn = ocl->oc_name_known, omndx = obj->corpsenm;
    const char *actualn = OBJ_NAME(*ocl);
    const char *dn = OBJ_DESCR(*ocl);
    const char *un = ocl->oc_uname;
    boolean pluralize = (obj->quan != 1L) && !(cxn_flags & CXN_SINGULAR);
    boolean known, dknown, bknown;

    gx.xnamep = nextobuf();
    /* set up primary work buffer; the first 'PREFIX' bytes are set
       aside for use by doname() */
    buf = gx.xnamep + PREFIX; /* leave room for "17 -3 " */
    buf_end = gx.xnamep + BUFSZ - 1; /* last byte within the obuf[] */
    buf[0] = '\0';
    ConcUpdate(buf); /* set buf_eos and bufspaceleft */

    if (Role_if(PM_SAMURAI)) {
        actualn = Japanese_item_name(typ, actualn);
        if (typ == WOODEN_HARP || typ == MAGIC_HARP)
            dn = "琴";
    }
    /* generic items don't have an actual-name; we shouldn't ever be called
       for those; pacify static analyzer without resorting to impossible() */
    if (!actualn)
        actualn = (typ > 0 && typ < MAXOCLASSES) ? "通用" : "物品?";
    /* 3.6.2: this used to be part of 'dn's initialization, but it
       needs to come after possibly overriding 'actualn' */
    if (!dn)
        dn = actualn;

    /*
     * clean up known when it's tied to oc_name_known, eg after AD_DRIN
     * This is only required for unique objects since the article
     * printed for the object is tied to the combination of the two
     * and printing the wrong article gives away information.
     */
    if (!nn && ocl->oc_uses_known && ocl->oc_unique)
        obj->known = 0;
    if (!Blind && !gd.distantname)
        observe_object(obj);
    if (Role_if(PM_CLERIC))
        obj->bknown = 1; /* avoid set_bknown() to bypass update_inventory() */

    if (iflags.override_ID) {
        known = dknown = bknown = TRUE;
        nn = 1;
    } else {
        known = obj->known;
        dknown = obj->dknown;
        bknown = obj->bknown;
    }

    /*
     * Maybe find a previously unseen artifact.
     *
     * Assumption 1: if an artifact object is being formatted, it is
     *  being shown to the hero (on floor, or looking into container,
     *  or probing a monster, or seeing a monster wield it).
     * Assumption 2: if in a pile that has been stepped on, the
     *  artifact won't be noticed for cases where the pile to too deep
     *  to be auto-shown, unless the player explicitly looks at that
     *  spot (via ':').  Might need to make an exception somehow (at
     *  the point where the decision whether to auto-show gets made?)
     *  when an artifact is on the top of the pile.
     * Assumption 3: since this is used for livelog events, not being
     *  100% correct won't negatively affect the player's current game.
     *
     * We use the real obj->dknown rather than the override_ID variant
     * so that wizard-mode ^I doesn't cause a not-yet-seen artifact in
     * inventory (picked up while blind, still blind) to become found.
     */
    if (obj->oartifact && obj->dknown)
        find_artifact(obj);

    if (obj_is_pname(obj))
        goto nameit;

    /* Some classes use strcpy(buf, something)+strcat(buf, otherthing).
       In those cases, ConcUpdate() is needed in between if Concat()
       will be used for the strcat() part.  Other classes just use
       strcpy(buf, something) and the ConcUpdate() can be deferred
       until after the switch. */
    switch (obj->oclass) {
    case AMULET_CLASS:
        if (!dknown)
            Strcpy(buf, "护身符");
        else if (typ == AMULET_OF_YENDOR || typ == FAKE_AMULET_OF_YENDOR)
            /* each must be identified individually */
            Strcpy(buf, known ? actualn : dn);
        else if (nn)
            Strcpy(buf, actualn);
        else if (un)
            xcalled(buf, BUFSZ - PREFIX, "护身符", un);
        else
            Sprintf(buf, "%s护身符", dn);
        break;
    case WEAPON_CLASS:
        if (is_poisonable(obj) && obj->opoisoned)
            Strcpy(buf, "有毒的");
        FALLTHROUGH;
        /*FALLTHRU*/
    case VENOM_CLASS:
    case TOOL_CLASS:
        /* note: lenses or towel prefix would overwrite poisoned weapon
           prefix if both were simultaneously possible, but they aren't */
        if (typ == LENSES)
            Strcpy(buf, "一对");
        else if (is_wet_towel(obj))
            Strcpy(buf, (obj->spe < 3) ? "湿润的" : "湿的");

        if (!dknown)
            Strcat(buf, dn);
        else if (nn)
            Strcat(buf, actualn);
        else if (un)
            xcalled(buf, BUFSZ - PREFIX, dn, un);
        else
            Strcat(buf, dn);
        ConcUpdate(buf);

        if (typ == FIGURINE && omndx != NON_PM) {
            char anbuf[10]; /* [4] would be enough: 'a','n',' ','\0' */
            const char *pm_name = obj_pmname(obj);

            Sprintf(buf, "%s%s的%s", just_an(anbuf, pm_name), pm_name, actualn); /*危险:ConcatF2(buf, 0, " of %s%s", just_an(anbuf, pm_name), pm_name);*/
        } else if (is_wet_towel(obj)) {
            if (wizard)
                ConcatF1(buf, 0, " (%d)", obj->spe);
        }
        break;
    case ARMOR_CLASS:
        /* depends on order of the dragon scales objects */
        if (typ >= GRAY_DRAGON_SCALES && typ <= YELLOW_DRAGON_SCALES) {
            Sprintf(buf, "一套%s", actualn);
            break;
        } else if (is_boots(obj) || is_gloves(obj)) {
            Strcpy(buf, "一双");
            /*FALLTHRU*/
        } else if (is_shield(obj) && !dknown) {
            if (obj->otyp >= ELVEN_SHIELD && obj->otyp <= ORCISH_SHIELD) {
                Strcpy(buf, "盾牌");
                break;
            } else if (obj->otyp == SHIELD_OF_REFLECTION) {
                Strcpy(buf, "平滑的盾");
                break;
            }
        }
        ConcUpdate(buf);

        if (nn)
            Concat(buf, 0, actualn);
        else if (un)
            xcalled(buf, BUFSZ - PREFIX, armor_simple_name(obj), un);
        else
            Concat(buf, 0, dn);
        break;
    case FOOD_CLASS:
        /* we could include partly-eaten-hack on fruit but don't need to */
        if (typ == SLIME_MOLD) {
            struct fruit *f = fruit_from_indx(obj->spe);

            if (!f) {
                impossible("Bad fruit #%d?", obj->spe);
                Strcpy(buf, "水果");
            } else {
                /* fruit name is limited in length to PL_FSIZ; converting
                   to/from singular/plural might increase the length a
                   little but not enough to pose a risk of overflowing buf */
                Strcpy(buf, f->fname);
                if (pluralize) {
                    /* ick: already pluralized fruit names are allowed--we
                       want to try to avoid adding a redundant plural suffix;
                       double ick: makesingular() and makeplural() each use
                       and return an obuf but we don't want any particular
                       xname() call to consume more than one of those
                       [note: makeXXX() will be fully evaluated and done with
                       'buf' before strcpy() touches its output buffer] */
                    Strcpy(buf, obufp = makesingular(buf));
                    releaseobuf(obufp);
                    Strcpy(buf, obufp = makeplural(buf));
                    releaseobuf(obufp);

                    pluralize = FALSE;
                }
            }
            break;
        }
        if (iflags.partly_eaten_hack && obj->oeaten) {
            /* normally "partly eaten" is supplied by doname() when
               appropriate and omitted by xname(); shrink_glob() wants
               it but uses Yname2() -> yname() -> xname() rather than
               doname() so we've added an external flag to request it */
            Concat(buf, 0, "部分吃掉的");
        }
        if (obj->globby) { /* 5.0 added "medium" to replace no-prefix */
            ConcatF2(buf, 0, "%s%s", (obj->owt <= 100) ? "小"
                                      : (obj->owt <= 300) ? "中"
                                        : (obj->owt <= 500) ? "大"
                                          : "特大",
                     actualn);
            break;
        }

        Concat(buf, 0, actualn);
        if (typ == TIN && known)
            tin_details(obj, omndx, buf);
        break;
    case COIN_CLASS:
    case CHAIN_CLASS:
        Strcpy(buf, actualn);
        break;
    case ROCK_CLASS:
        if (typ == STATUE && omndx != NON_PM) {
            char anbuf[10];
            const char *statue_pmname = obj_pmname(obj);

            Snprintf(buf, bufspaceleft, "%s%s%s%s",
                     (Role_if(PM_ARCHEOLOGIST)
                      && (obj->spe & CORPSTAT_HISTORIC) != 0) ? "历史感的"
                       : "",
                     type_is_pname(&mons[omndx]) ? ""
                       : the_unique_pm(&mons[omndx]) ? ""
                         : just_an(anbuf, statue_pmname),
                     statue_pmname,
                     actualn);
        } else if (typ == BOULDER && obj->next_boulder == 1) {
            /* sometimes caller wants "next boulder" rather than just
               "boulder" (when pushing against a pile of more than one);
               originally we just tested for non-0 but checking for 1 is
               more robust because the default value for that overloaded
               field (obj->corpsenm) is NON_PM (-1) rather than 0 */
            Strcat(strcpy(buf, "下一块"), actualn); /* "next boulder" */
            /* once "next boulder" occurs, subsequent messages should just
               use ordinary "boulder" */
            obj->next_boulder = 0;
        } else {
            Strcpy(buf, actualn); /* "boulder" or "statue" */
        }
        break;
    case BALL_CLASS:
        Sprintf(buf, "%s沉重的铁球",
                (obj->owt > ocl->oc_weight) ? "非常 " : "");
        break;
    case POTION_CLASS:
        if (dknown && obj->odiluted)
            Strcpy(buf, "稀释的");
        if (nn || un || !dknown) {
            if (!dknown)
                break;
            if (nn) {
                if (typ == POT_WATER && bknown
                    && (obj->blessed || obj->cursed)) {
                    Strcat(buf, obj->blessed ? "圣" : "邪");
                }
                Strcat(buf, actualn);
                /*Strcat(buf, "之");*/
            } else {
                xcalled(buf, BUFSZ - PREFIX, "", un);
            }
            Strcat(buf, "药水");
        } else {
            Strcat(buf, dn);
            Strcat(buf, "药水");
        }
        break;
    case SCROLL_CLASS:
        if (!dknown)
            break;
        if (nn) {
            Strcat(buf, actualn);
            /*Strcat(buf, "之");*/
            Strcat(buf, "卷轴");
        } else if (un) {
            xcalled(buf, BUFSZ - PREFIX, "", un);
        } else if (ocl->oc_magic) {
            Strcpy(buf, "写着");
            Strcat(buf, dn);
            Strcat(buf, "的卷轴");
        } else {
            Strcpy(buf, dn);
            Strcat(buf, "卷轴");
        }
        break;
    case WAND_CLASS:
        if (!dknown)
            Strcpy(buf, "魔杖");
        else if (nn)
            Sprintf(buf, "%s魔杖", actualn);
        else if (un)
            xcalled(buf, BUFSZ - PREFIX, "魔杖", un);
        else
            Sprintf(buf, "%s魔杖", dn);
        break;
    case SPBOOK_CLASS:
        if (typ == SPE_NOVEL) { /* 3.6 tribute */
            if (!dknown)
                Strcpy(buf, "书");
            else if (nn)
                Strcpy(buf, actualn);
            else if (un)
                xcalled(buf, BUFSZ - PREFIX, "小说", un);
            else
                Sprintf(buf, "%s书", dn);
            break;
            /* end of tribute */
        } else if (!dknown) {
            Strcpy(buf, "魔法书");
        } else if (nn) {
            if (typ != SPE_BOOK_OF_THE_DEAD)
                Strcpy(buf, "魔法书");
            Strcat(buf, actualn);
        } else if (un) {
            xcalled(buf, BUFSZ - PREFIX, "魔法书", un);
        } else
            Sprintf(buf, "%s魔法书", dn);
        break;
    case RING_CLASS:
        if (!dknown)
            Strcpy(buf, "戒指");
        else if (nn)
            Sprintf(buf, "%s戒指", actualn);
        else if (un)
            xcalled(buf, BUFSZ - PREFIX, "戒指", un);
        else
            Sprintf(buf, "%s戒指", dn);
        break;
    case GEM_CLASS: {
        const char *rock = (ocl->oc_material == MINERAL) ? "石" : "宝石";

        if (!dknown) {
            Strcpy(buf, rock);
        } else if (!nn) {
            if (un)
                xcalled(buf, BUFSZ - PREFIX, rock, un);
            else
                Sprintf(buf, "%s%s", dn, rock);
        } else {
            Strcpy(buf, actualn);
            if (GemStone(typ))
                Strcat(buf, "石");
        }
        break;
    } /* gem */
    default:
        Sprintf(buf, "glorkum %d %d %d", obj->oclass, typ, obj->spe);
        impossible("xname_flags: %s", buf);
        break;
    } /* switch */

    /* check whether we've already gone out of bounds of the obuf[], prior
       to pluralization and end-of-game shirt and apron text */
    buf_eos = eos(buf);
    if (buf_eos > buf_end) {
        /* PREFIX is bigger than 6 so there will always be room within the
           obuf[] in front of buf to insert "buf[]="; strncpy(,,N) doesn't
           add '\0' terminator unless fewer than N chars are copied, which
           is what we want, but gcc complains about that so use memcpy() */
        paniclog("xname", (char *) memcpy(buf - 6, "buf[]=", 6));
        panic("xname: buffer overflow before appending name.");
        /*NOTREACHED*/
    }
    bufspaceleft = (size_t) (buf_end - buf_eos);

    /* if the name should be plural, do that now, after overflow check;
       it could make buf[] become shorter */
    if (pluralize) {
        obufp = makeplural(buf);
        buf[0] = '\0'; /* replace the whole string */
        ConcUpdate(buf); /* reset buf_eos and bufspaceleft */
        Concat(buf, 0, obufp);
        releaseobuf(obufp);
    }

    /* give some extra information when game is over; for end-of-game
       attribute disclosure in wizard mode, ysimple_name() calls
       minimal_xname() which passes us a dummy object with o_id==0;
       tshirt_text(), apron_text(), and so forth base their result on
       o_id and would give inconsistent information compared to what
       just got shown for inventory disclosure; fortunately, we want to
       avoid the 'with text' part of
           "You were acid resistant because of your alchemy smock \
           with text \"Kiss the cook\"."
       when disclosing attributes anyway */
    if (program_state.gameover && obj->o_id && bufspaceleft > 0) {
        const char *lbl;
        char tmpbuf[BUFSZ];

        /* disclose without breaking illiterate conduct, but mainly tip off
           players who aren't aware that something readable is present */
        switch (obj->otyp) {
        case T_SHIRT:
        case ALCHEMY_SMOCK:
            ConcatF1(buf, 0, ",上面写着\"%s\"",
                     (obj->otyp == T_SHIRT) ? tshirt_text(obj, tmpbuf)
                                            : apron_text(obj, tmpbuf));
            break;
        case CANDY_BAR:
            lbl = candy_wrapper_text(obj);
            if (*lbl)
                ConcatF1(buf, 0, ",上面写着\"%s\"", lbl);
            break;
        case HAWAIIAN_SHIRT:
            ConcatF1(buf, 0, ",上面有%s",
                     an(hawaiian_motif(obj, tmpbuf)));
            break;
        default:
            break;
        }
    }

    if (has_oname(obj) && dknown) {
        Concat(buf, 0, ",被称为");

        /* jump directly here if obj passes the has-personal-name test */
 nameit:
        /*assert(has_oname(obj));*/
        obufp = eos(buf); /* remember where the name will start */
        Concat(buf, 0, ONAME(obj));
        /* downcase "The" in "<quest-artifact-item> named The ..." */
        /*冗余:if (obj->oartifact && !strncmp(obufp, "The ", 4))
            *obufp = lowc(*obufp);*/ /* change 'T' in "The " to 't' */
    }
    /*冗余:
    if (!strncmpi(buf, "the ", 4))
        buf += 4;
    */
    buf_eos = eos(buf); /* pointer to '\0' terminator somewhere in obuf[] */
    if (buf_eos >= buf_end) { /* ('>' shouldn't be possible) */
        static int xname_full = 0;

        /* we want a record of something needing more buffer space than
           anticipated; since we aren't panicking here, this could happen
           repeatedly and we don't want to spam the paniclog file */
        if (!xname_full++) {
            paniclog("xname", (char *) memcpy(buf - 6, "buf[]=", 6));
            /* 'PREFIX' ought to be 'PREFIX+4' if we stripped leading "the" */
            paniclog("xname", "used up entire obuf[PREFIX..BUFSX-1]");
        }
    }

    return buf;
}

staticfn char *
xename_flags(
    struct obj *obj,
    unsigned cxn_flags) /* bitmask of CXN_xxx values */
{
    char *buf;
    char *obufp, *buf_end, *buf_eos;
    size_t bufspaceleft;
    int typ = obj->otyp;
    struct objclass *ocl = &objects[typ];
    int nn = ocl->oc_name_known, omndx = obj->corpsenm;
    const char *actualn = OBJ_ENAME(*ocl);
    const char *dn = OBJ_EDESCR(*ocl);
    const char *un = ocl->oc_uname;
    boolean pluralize = (obj->quan != 1L) && !(cxn_flags & CXN_SINGULAR);
    boolean known, dknown, bknown;

    gx.xnamep = nextobuf();
    /* set up primary work buffer; the first 'PREFIX' bytes are set
       aside for use by doname() */
    buf = gx.xnamep + PREFIX; /* leave room for "17 -3 " */
    buf_end = gx.xnamep + BUFSZ - 1; /* last byte within the obuf[] */
    buf[0] = '\0';
    ConcUpdate(buf); /* set buf_eos and bufspaceleft */

    if (Role_if(PM_SAMURAI)) {
        actualn = Japanese_item_name(typ, actualn);
        if (typ == WOODEN_HARP || typ == MAGIC_HARP)
            dn = "koto";
    }
    /* generic items don't have an actual-name; we shouldn't ever be called
       for those; pacify static analyzer without resorting to impossible() */
    if (!actualn)
        actualn = (typ > 0 && typ < MAXOCLASSES) ? "generic" : "object?";
    /* 3.6.2: this used to be part of 'dn's initialization, but it
       needs to come after possibly overriding 'actualn' */
    if (!dn)
        dn = actualn;

    /*
     * clean up known when it's tied to oc_name_known, eg after AD_DRIN
     * This is only required for unique objects since the article
     * printed for the object is tied to the combination of the two
     * and printing the wrong article gives away information.
     */
    if (!nn && ocl->oc_uses_known && ocl->oc_unique)
        obj->known = 0;
    if (!Blind && !gd.distantname)
        observe_object(obj);
    if (Role_if(PM_CLERIC))
        obj->bknown = 1; /* avoid set_bknown() to bypass update_inventory() */

    if (iflags.override_ID) {
        known = dknown = bknown = TRUE;
        nn = 1;
    } else {
        known = obj->known;
        dknown = obj->dknown;
        bknown = obj->bknown;
    }

    /*
     * Maybe find a previously unseen artifact.
     *
     * Assumption 1: if an artifact object is being formatted, it is
     *  being shown to the hero (on floor, or looking into container,
     *  or probing a monster, or seeing a monster wield it).
     * Assumption 2: if in a pile that has been stepped on, the
     *  artifact won't be noticed for cases where the pile to too deep
     *  to be auto-shown, unless the player explicitly looks at that
     *  spot (via ':').  Might need to make an exception somehow (at
     *  the point where the decision whether to auto-show gets made?)
     *  when an artifact is on the top of the pile.
     * Assumption 3: since this is used for livelog events, not being
     *  100% correct won't negatively affect the player's current game.
     *
     * We use the real obj->dknown rather than the override_ID variant
     * so that wizard-mode ^I doesn't cause a not-yet-seen artifact in
     * inventory (picked up while blind, still blind) to become found.
     */
    if (obj->oartifact && obj->dknown)
        find_artifact(obj);

    if (obj_is_pname(obj))
        goto nameit;

    /* Some classes use strcpy(buf, something)+strcat(buf, otherthing).
       In those cases, ConcUpdate() is needed in between if Concat()
       will be used for the strcat() part.  Other classes just use
       strcpy(buf, something) and the ConcUpdate() can be deferred
       until after the switch. */
    switch (obj->oclass) {
    case AMULET_CLASS:
        if (!dknown)
            Strcpy(buf, "amulet");
        else if (typ == AMULET_OF_YENDOR || typ == FAKE_AMULET_OF_YENDOR)
            /* each must be identified individually */
            Strcpy(buf, known ? actualn : dn);
        else if (nn)
            Strcpy(buf, actualn);
        else if (un)
            xcallede(buf, BUFSZ - PREFIX, "amulet", un);
        else
            Sprintf(buf, "%s amulet", dn);
        break;
    case WEAPON_CLASS:
        if (is_poisonable(obj) && obj->opoisoned)
            Strcpy(buf, "poisoned ");
        FALLTHROUGH;
        /*FALLTHRU*/
    case VENOM_CLASS:
    case TOOL_CLASS:
        /* note: lenses or towel prefix would overwrite poisoned weapon
           prefix if both were simultaneously possible, but they aren't */
        if (typ == LENSES)
            Strcpy(buf, "pair of ");
        else if (is_wet_towel(obj))
            Strcpy(buf, (obj->spe < 3) ? "moist " : "wet ");

        if (!dknown)
            Strcat(buf, dn);
        else if (nn)
            Strcat(buf, actualn);
        else if (un)
            xcallede(buf, BUFSZ - PREFIX, dn, un);
        else
            Strcat(buf, dn);
        ConcUpdate(buf);

        if (typ == FIGURINE && omndx != NON_PM) {
            char anbuf[10]; /* [4] would be enough: 'a','n',' ','\0' */
            const char *pm_name = obj_pmname(obj);

            ConcatF2(buf, 0, " of %s%s", just_an(anbuf, pm_name), pm_name);
        } else if (is_wet_towel(obj)) {
            if (wizard)
                ConcatF1(buf, 0, " (%d)", obj->spe);
        }
        break;
    case ARMOR_CLASS:
        /* depends on order of the dragon scales objects */
        if (typ >= GRAY_DRAGON_SCALES && typ <= YELLOW_DRAGON_SCALES) {
            Sprintf(buf, "set of %s", actualn);
            break;
        } else if (is_boots(obj) || is_gloves(obj)) {
            Strcpy(buf, "pair of ");
            /*FALLTHRU*/
        } else if (is_shield(obj) && !dknown) {
            if (obj->otyp >= ELVEN_SHIELD && obj->otyp <= ORCISH_SHIELD) {
                Strcpy(buf, "shield");
                break;
            } else if (obj->otyp == SHIELD_OF_REFLECTION) {
                Strcpy(buf, "smooth shield");
                break;
            }
        }
        ConcUpdate(buf);

        if (nn)
            Concat(buf, 0, actualn);
        else if (un)
            xcallede(buf, BUFSZ - PREFIX, armor_simple_name(obj), un);
        else
            Concat(buf, 0, dn);
        break;
    case FOOD_CLASS:
        /* we could include partly-eaten-hack on fruit but don't need to */
        if (typ == SLIME_MOLD) {
            struct fruit *f = fruit_from_indx(obj->spe);

            if (!f) {
                impossible("Bad fruit #%d?", obj->spe);
                Strcpy(buf, "fruit");
            } else {
                /* fruit name is limited in length to PL_FSIZ; converting
                   to/from singular/plural might increase the length a
                   little but not enough to pose a risk of overflowing buf */
                Strcpy(buf, f->fname);
                if (pluralize) {
                    /* ick: already pluralized fruit names are allowed--we
                       want to try to avoid adding a redundant plural suffix;
                       double ick: makesingular() and makeplural() each use
                       and return an obuf but we don't want any particular
                       xname() call to consume more than one of those
                       [note: makeXXX() will be fully evaluated and done with
                       'buf' before strcpy() touches its output buffer] */
                    Strcpy(buf, obufp = makesingular(buf));
                    releaseobuf(obufp);
                    Strcpy(buf, obufp = makeplural(buf));
                    releaseobuf(obufp);

                    pluralize = FALSE;
                }
            }
            break;
        }
        if (iflags.partly_eaten_hack && obj->oeaten) {
            /* normally "partly eaten" is supplied by doname() when
               appropriate and omitted by xname(); shrink_glob() wants
               it but uses Yname2() -> yname() -> xname() rather than
               doname() so we've added an external flag to request it */
            Concat(buf, 0, "partly eaten ");
        }
        if (obj->globby) { /* 5.0 added "medium" to replace no-prefix */
            ConcatF2(buf, 0, "%s %s", (obj->owt <= 100) ? "small"
                                      : (obj->owt <= 300) ? "medium"
                                        : (obj->owt <= 500) ? "large"
                                          : "very large",
                     actualn);
            break;
        }

        Concat(buf, 0, actualn);
        if (typ == TIN && known)
            tin_details(obj, omndx, buf);
        break;
    case COIN_CLASS:
    case CHAIN_CLASS:
        Strcpy(buf, actualn);
        break;
    case ROCK_CLASS:
        if (typ == STATUE && omndx != NON_PM) {
            char anbuf[10];
            const char *statue_pmname = obj_pmname(obj);

            Snprintf(buf, bufspaceleft, "%s%s of %s%s",
                     (Role_if(PM_ARCHEOLOGIST)
                      && (obj->spe & CORPSTAT_HISTORIC) != 0) ? "historic "
                       : "",
                     actualn,
                     type_is_pname(&mons[omndx]) ? ""
                       : the_unique_pm(&mons[omndx]) ? "the "
                         : just_an(anbuf, statue_pmname),
                     statue_pmname);
        } else if (typ == BOULDER && obj->next_boulder == 1) {
            /* sometimes caller wants "next boulder" rather than just
               "boulder" (when pushing against a pile of more than one);
               originally we just tested for non-0 but checking for 1 is
               more robust because the default value for that overloaded
               field (obj->corpsenm) is NON_PM (-1) rather than 0 */
            Strcat(strcpy(buf, "next "), actualn); /* "next boulder" */
            /* once "next boulder" occurs, subsequent messages should just
               use ordinary "boulder" */
            obj->next_boulder = 0;
        } else {
            Strcpy(buf, actualn); /* "boulder" or "statue" */
        }
        break;
    case BALL_CLASS:
        Sprintf(buf, "%sheavy iron ball",
                (obj->owt > ocl->oc_weight) ? "very " : "");
        break;
    case POTION_CLASS:
        if (dknown && obj->odiluted)
            Strcpy(buf, "diluted ");
        if (nn || un || !dknown) {
            Strcat(buf, "potion");
            if (!dknown)
                break;
            if (nn) {
                Strcat(buf, " of ");
                if (typ == POT_WATER && bknown
                    && (obj->blessed || obj->cursed)) {
                    Strcat(buf, obj->blessed ? "holy " : "unholy ");
                }
                Strcat(buf, actualn);
            } else {
                xcallede(buf, BUFSZ - PREFIX, "", un);
            }
        } else {
            Strcat(buf, dn);
            Strcat(buf, " potion");
        }
        break;
    case SCROLL_CLASS:
        Strcpy(buf, "scroll");
        if (!dknown)
            break;
        if (nn) {
            Strcat(buf, " of ");
            Strcat(buf, actualn);
        } else if (un) {
            xcallede(buf, BUFSZ - PREFIX, "", un);
        } else if (ocl->oc_magic) {
            Strcat(buf, " labeled ");
            Strcat(buf, dn);
        } else {
            Strcpy(buf, dn);
            Strcat(buf, " scroll");
        }
        break;
    case WAND_CLASS:
        if (!dknown)
            Strcpy(buf, "wand");
        else if (nn)
            Sprintf(buf, "wand of %s", actualn);
        else if (un)
            xcallede(buf, BUFSZ - PREFIX, "wand", un);
        else
            Sprintf(buf, "%s wand", dn);
        break;
    case SPBOOK_CLASS:
        if (typ == SPE_NOVEL) { /* 3.6 tribute */
            if (!dknown)
                Strcpy(buf, "book");
            else if (nn)
                Strcpy(buf, actualn);
            else if (un)
                xcallede(buf, BUFSZ - PREFIX, "novel", un);
            else
                Sprintf(buf, "%s book", dn);
            break;
            /* end of tribute */
        } else if (!dknown) {
            Strcpy(buf, "spellbook");
        } else if (nn) {
            if (typ != SPE_BOOK_OF_THE_DEAD)
                Strcpy(buf, "spellbook of ");
            Strcat(buf, actualn);
        } else if (un) {
            xcallede(buf, BUFSZ - PREFIX, "spellbook", un);
        } else
            Sprintf(buf, "%s spellbook", dn);
        break;
    case RING_CLASS:
        if (!dknown)
            Strcpy(buf, "ring");
        else if (nn)
            Sprintf(buf, "ring of %s", actualn);
        else if (un)
            xcallede(buf, BUFSZ - PREFIX, "ring", un);
        else
            Sprintf(buf, "%s ring", dn);
        break;
    case GEM_CLASS: {
        const char *rock = (ocl->oc_material == MINERAL) ? "stone" : "gem";

        if (!dknown) {
            Strcpy(buf, rock);
        } else if (!nn) {
            if (un)
                xcallede(buf, BUFSZ - PREFIX, rock, un);
            else
                Sprintf(buf, "%s %s", dn, rock);
        } else {
            Strcpy(buf, actualn);
            if (GemStone(typ))
                Strcat(buf, " stone");
        }
        break;
    } /* gem */
    default:
        Sprintf(buf, "glorkum %d %d %d", obj->oclass, typ, obj->spe);
        impossible("xname_flags: %s", buf);
        break;
    } /* switch */

    /* check whether we've already gone out of bounds of the obuf[], prior
       to pluralization and end-of-game shirt and apron text */
    buf_eos = eos(buf);
    if (buf_eos > buf_end) {
        /* PREFIX is bigger than 6 so there will always be room within the
           obuf[] in front of buf to insert "buf[]="; strncpy(,,N) doesn't
           add '\0' terminator unless fewer than N chars are copied, which
           is what we want, but gcc complains about that so use memcpy() */
        paniclog("xname", (char *) memcpy(buf - 6, "buf[]=", 6));
        panic("xname: buffer overflow before appending name.");
        /*NOTREACHED*/
    }
    bufspaceleft = (size_t) (buf_end - buf_eos);

    /* if the name should be plural, do that now, after overflow check;
       it could make buf[] become shorter */
    if (pluralize) {
        obufp = makeplural(buf);
        buf[0] = '\0'; /* replace the whole string */
        ConcUpdate(buf); /* reset buf_eos and bufspaceleft */
        Concat(buf, 0, obufp);
        releaseobuf(obufp);
    }

    /* give some extra information when game is over; for end-of-game
       attribute disclosure in wizard mode, ysimple_name() calls
       minimal_xname() which passes us a dummy object with o_id==0;
       tshirt_text(), apron_text(), and so forth base their result on
       o_id and would give inconsistent information compared to what
       just got shown for inventory disclosure; fortunately, we want to
       avoid the 'with text' part of
           "You were acid resistant because of your alchemy smock \
           with text \"Kiss the cook\"."
       when disclosing attributes anyway */
    if (program_state.gameover && obj->o_id && bufspaceleft > 0) {
        const char *lbl;
        char tmpbuf[BUFSZ];

        /* disclose without breaking illiterate conduct, but mainly tip off
           players who aren't aware that something readable is present */
        switch (obj->otyp) {
        case T_SHIRT:
        case ALCHEMY_SMOCK:
            ConcatF1(buf, 0, " with text \"%s\"",
                     (obj->otyp == T_SHIRT) ? tshirt_text(obj, tmpbuf)
                                            : apron_text(obj, tmpbuf));
            break;
        case CANDY_BAR:
            lbl = candy_wrapper_text(obj);
            if (*lbl)
                ConcatF1(buf, 0, " labeled \"%s\"", lbl);
            break;
        case HAWAIIAN_SHIRT:
            ConcatF1(buf, 0, " with %s motif",
                     an(hawaiian_motif(obj, tmpbuf)));
            break;
        default:
            break;
        }
    }

    if (has_oname(obj) && dknown) {
        Concat(buf, 0, " named ");

        /* jump directly here if obj passes the has-personal-name test */
 nameit:
        /*assert(has_oname(obj));*/
        obufp = eos(buf); /* remember where the name will start */
        Concat(buf, 0, ONAME(obj));
        /* downcase "The" in "<quest-artifact-item> named The ..." */
        if (obj->oartifact && !strncmp(obufp, "The ", 4))
            *obufp = lowc(*obufp); /* change 'T' in "The " to 't' */
    }

    if (!strncmpi(buf, "the ", 4))
        buf += 4;

    buf_eos = eos(buf); /* pointer to '\0' terminator somewhere in obuf[] */
    if (buf_eos >= buf_end) { /* ('>' shouldn't be possible) */
        static int xname_full = 0;

        /* we want a record of something needing more buffer space than
           anticipated; since we aren't panicking here, this could happen
           repeatedly and we don't want to spam the paniclog file */
        if (!xname_full++) {
            paniclog("xname", (char *) memcpy(buf - 6, "buf[]=", 6));
            /* 'PREFIX' ought to be 'PREFIX+4' if we stripped leading "the" */
            paniclog("xname", "used up entire obuf[PREFIX..BUFSX-1]");
        }
    }

    return buf;
}

/* similar to simple_typename but minimal_xname operates on a particular
   object rather than its general type; it formats the most basic info:
     potion                     -- if description not known
     brown potion               -- if oc_name_known not set
     potion of object detection -- if discovered
 */
staticfn char *
minimal_xname(struct obj *obj)
{
    char *bufp;
    struct obj bareobj;
    struct objclass saveobcls;
    int otyp = obj->otyp;

    /* suppress user-supplied name */
    saveobcls.oc_uname = objects[otyp].oc_uname;
    objects[otyp].oc_uname = 0;
    /* suppress actual name if object's description is unknown */
    saveobcls.oc_name_known = objects[otyp].oc_name_known;
    if (iflags.override_ID)
        objects[otyp].oc_name_known = 1;
    else if (!obj->dknown)
        objects[otyp].oc_name_known = 0;

    /* caveat: this makes a lot of assumptions about which fields
       are required in order for xname() to yield a sensible result */
    bareobj = cg.zeroobj;
    bareobj.otyp = otyp;
    bareobj.oclass = obj->oclass;
    /* not observe_object, either the hero observed the object already or this
       is overriding ID and shouldn't discover the object */
    bareobj.dknown = (obj->dknown || iflags.override_ID) ? 1 : 0;
    /* suppress known except for amulets (needed for fakes and real A-of-Y) */
    bareobj.known = (obj->oclass == AMULET_CLASS)
                        ? obj->known
                        /* default is "on" for types which don't use it */
                        : !objects[otyp].oc_uses_known;
    bareobj.quan = 1L;         /* don't want plural */
    /* for a boulder, leave corpsenm as 0; non-zero produces "next boulder" */
    if (otyp != BOULDER)
        bareobj.corpsenm = NON_PM; /* suppress statue and figurine details */
    /* but suppressing fruit details leads to "bad fruit #0"
       [perhaps we should force "slime mold" rather than use xname?] */
    if (obj->otyp == SLIME_MOLD)
        bareobj.spe = obj->spe;

    bufp = distant_name(&bareobj, xname);
    /* undo forced setting of bareobj.blessed for cleric (priest[ess]);
       bufp is an obuf[] so a pointer into the middle of that is viable */
    if (!cnstrcmp(bufp, "无诅咒的"))
        bufp += strlen("无诅咒的");

    objects[otyp].oc_uname = saveobcls.oc_uname;
    objects[otyp].oc_name_known = saveobcls.oc_name_known;
    return bufp;
}

staticfn char *
minimal_xename(struct obj *obj)
{
    char *bufp;
    struct obj bareobj;
    struct objclass saveobcls;
    int otyp = obj->otyp;

    /* suppress user-supplied name */
    saveobcls.oc_uname = objects[otyp].oc_uname;
    objects[otyp].oc_uname = 0;
    /* suppress actual name if object's description is unknown */
    saveobcls.oc_name_known = objects[otyp].oc_name_known;
    if (iflags.override_ID)
        objects[otyp].oc_name_known = 1;
    else if (!obj->dknown)
        objects[otyp].oc_name_known = 0;

    /* caveat: this makes a lot of assumptions about which fields
       are required in order for xname() to yield a sensible result */
    bareobj = cg.zeroobj;
    bareobj.otyp = otyp;
    bareobj.oclass = obj->oclass;
    /* not observe_object, either the hero observed the object already or this
       is overriding ID and shouldn't discover the object */
    bareobj.dknown = (obj->dknown || iflags.override_ID) ? 1 : 0;
    /* suppress known except for amulets (needed for fakes and real A-of-Y) */
    bareobj.known = (obj->oclass == AMULET_CLASS)
                        ? obj->known
                        /* default is "on" for types which don't use it */
                        : !objects[otyp].oc_uses_known;
    bareobj.quan = 1L;         /* don't want plural */
    /* for a boulder, leave corpsenm as 0; non-zero produces "next boulder" */
    if (otyp != BOULDER)
        bareobj.corpsenm = NON_PM; /* suppress statue and figurine details */
    /* but suppressing fruit details leads to "bad fruit #0"
       [perhaps we should force "slime mold" rather than use xname?] */
    if (obj->otyp == SLIME_MOLD)
        bareobj.spe = obj->spe;

    bufp = distant_name(&bareobj, xename);
    /* undo forced setting of bareobj.blessed for cleric (priest[ess]);
       bufp is an obuf[] so a pointer into the middle of that is viable */
    if (!strncmp(bufp, "uncursed ", 9))
        bufp += 9;

    objects[otyp].oc_uname = saveobcls.oc_uname;
    objects[otyp].oc_name_known = saveobcls.oc_name_known;
    return bufp;
}

/* xname() output augmented for multishot missile feedback */
char *
mshot_xname(struct obj *obj)
{
    char tmpbuf[BUFSZ];
    char *onm = xname(obj);

    if (gm.m_shot.n > 1 && gm.m_shot.o == obj->otyp) {
        /* "the Nth arrow"; value will eventually be passed to an() or
           The(), both of which correctly handle this "the " prefix */
        Sprintf(tmpbuf, "第%d个", gm.m_shot.i);
        onm = strprepend(onm, tmpbuf);
    }
    return onm;
}

char *
mshot_xename(struct obj *obj)
{
    char tmpbuf[BUFSZ];
    char *onm = xname(obj);

    if (gm.m_shot.n > 1 && gm.m_shot.o == obj->otyp) {
        /* "the Nth arrow"; value will eventually be passed to an() or
           The(), both of which correctly handle this "the " prefix */
        Sprintf(tmpbuf, "the %d%s ", gm.m_shot.i, ordin(gm.m_shot.i));
        onm = strprepend(onm, tmpbuf);
    }
    return onm;
}

/* used for naming "the unique_item" instead of "a unique_item" */
boolean
the_unique_obj(struct obj *obj)
{
    boolean known = (obj->known || iflags.override_ID);

    if (!obj->dknown && !iflags.override_ID)
        return FALSE;
    else if (obj->otyp == FAKE_AMULET_OF_YENDOR && !known)
        return TRUE; /* lie */
    else
        return (boolean) (objects[obj->otyp].oc_unique
                          && (known || obj->otyp == AMULET_OF_YENDOR));
}

/* should monster type be prefixed with "the"? (mostly used for corpses) */
boolean
the_unique_pm(struct permonst *ptr)
{
    boolean uniq;

    /* even though monsters with personal names are unique, we want to
       describe them as "Name" rather than "the Name" */
    if (type_is_pname(ptr))
        return FALSE;

    uniq = (ptr->geno & G_UNIQ) ? TRUE : FALSE;
    /* high priest is unique if it includes "of <deity>", otherwise not
       (caller needs to handle the 1st possibility; we assume the 2nd);
       worm tail should be irrelevant but is included for completeness */
    if (ptr == &mons[PM_HIGH_CLERIC] || ptr == &mons[PM_LONG_WORM_TAIL])
        uniq = FALSE;
    /* Wizard no longer needs this; he's flagged as unique these days */
    if (ptr == &mons[PM_WIZARD_OF_YENDOR])
        uniq = TRUE;
    return uniq;
}

staticfn void
add_erosion_words(struct obj *obj, char *prefix)
{
    boolean iscrys = (obj->otyp == CRYSKNIFE);
    boolean rknown;

    rknown = (iflags.override_ID == 0) ? obj->rknown : TRUE;

    if (!is_damageable(obj) && !iscrys)
        return;

    /* The only cases where any of these bits do double duty are for
     * rotted food and diluted potions, which are all not is_damageable().
     */
    if (obj->oeroded && !iscrys) {
        switch (obj->oeroded) {
        case 2:
            Strcat(prefix, "非常");
            break;
        case 3:
            Strcat(prefix, "彻底");
            break;
        }
        Strcat(prefix, is_rustprone(obj) ? "生锈的"
                       : is_crackable(obj) ? "破裂的"
                         : "烧焦的");
    }
    if (obj->oeroded2 && !iscrys) {
        switch (obj->oeroded2) {
        case 2:
            Strcat(prefix, "非常");
            break;
        case 3:
            Strcat(prefix, "彻底");
            break;
        }
        Strcat(prefix, is_corrodeable(obj) ? "腐蚀的" : "腐烂的");
    }
    /* note: it is possible for an item to be both eroded and erodeproof
       (cursed scroll of destroy armor read while confused erodeproofs an
       item of armor without repairing existing erosion) */
    if (rknown && obj->oerodeproof)
        Strcat(prefix, iscrys ? "固定的"
                       : is_rustprone(obj) ? "防锈的"
                         : is_corrodeable(obj) ? "防腐蚀的"
                           : is_flammable(obj) ? "防火的"
                             : is_crackable(obj) ? "淬火的" /* hardened */
                               : is_rottable(obj) ? "防腐烂的"
                                 : "");
}

staticfn void
add_erosion_ewords(struct obj *obj, char *prefix)
{
    boolean iscrys = (obj->otyp == CRYSKNIFE);
    boolean rknown;

    rknown = (iflags.override_ID == 0) ? obj->rknown : TRUE;

    if (!is_damageable(obj) && !iscrys)
        return;

    /* The only cases where any of these bits do double duty are for
     * rotted food and diluted potions, which are all not is_damageable().
     */
    if (obj->oeroded && !iscrys) {
        switch (obj->oeroded) {
        case 2:
            Strcat(prefix, "very ");
            break;
        case 3:
            Strcat(prefix, "thoroughly ");
            break;
        }
        Strcat(prefix, is_rustprone(obj) ? "rusty "
                       : is_crackable(obj) ? "cracked "
                         : "burnt ");
    }
    if (obj->oeroded2 && !iscrys) {
        switch (obj->oeroded2) {
        case 2:
            Strcat(prefix, "very ");
            break;
        case 3:
            Strcat(prefix, "thoroughly ");
            break;
        }
        Strcat(prefix, is_corrodeable(obj) ? "corroded " : "rotted ");
    }
    /* note: it is possible for an item to be both eroded and erodeproof
       (cursed scroll of destroy armor read while confused erodeproofs an
       item of armor without repairing existing erosion) */
    if (rknown && obj->oerodeproof)
        Strcat(prefix, iscrys ? "fixed "
                       : is_rustprone(obj) ? "rustproof "
                         : is_corrodeable(obj) ? "corrodeproof "
                           : is_flammable(obj) ? "fireproof "
                             : is_crackable(obj) ? "tempered " /* hardened */
                               : is_rottable(obj) ? "rotproof "
                                 : "");
}

/* used to prevent rust on items where rust makes no difference */
boolean
erosion_matters(struct obj *obj)
{
    switch (obj->oclass) {
    case TOOL_CLASS:
        /* it's possible for a rusty weptool to be polymorphed into some
           non-weptool iron tool, in which case the rust implicitly goes
           away, but it's also possible for it to be polymorphed into a
           non-iron tool, in which case rust also implicitly goes away,
           so there's no particular reason to try to handle the first
           instance differently [this comment belongs in poly_obj()...] */
        return is_weptool(obj) ? TRUE : FALSE;
    case WEAPON_CLASS:
    case ARMOR_CLASS:
    case BALL_CLASS:
    case CHAIN_CLASS:
        return TRUE;
    default:
        break;
    }
    return FALSE;
}

#define DONAME_WITH_PRICE 1
#define DONAME_VAGUE_QUAN 2
#define DONAME_FOR_MENU   4 /* [not used anywhere yet] */

/* core of doname() */
staticfn char *
doname_base(
    struct obj *obj,       /* object to format */
    unsigned doname_flags) /* special case requests */
{
    boolean ispoisoned = FALSE,
            with_price = (doname_flags & DONAME_WITH_PRICE) != 0,
            vague_quan = (doname_flags & DONAME_VAGUE_QUAN) != 0,
            for_menu = (doname_flags & DONAME_FOR_MENU) != 0;
    boolean known, dknown, cknown, bknown, lknown,
            fake_arti, force_the;
    char prefix[PREFIX];
    char tmpbuf[PREFIX + 1]; /* for when we have to add something at
                              * the start of prefix instead of the
                              * end (Strcat is used on the end) */
    const char *aname = 0;
    int omndx = obj->corpsenm;
    char *bp;
    char *bp_eos, *bp_end;
    size_t bpspaceleft;

    /* 'bp' will be within an obuf[] rather than at the start of one,
       usually (but not always) pointing at &obuf[PREFIX];
       gx.xnamep always points to the start of that buffer;
       'bp_eos' and 'bpspaceleft' are used and updated by Concat*() macros */
    bp = xname(obj);
    bp_end = gx.xnamep + BUFSZ - 1;
    bp_eos = eos(bp);
    assert(bp_end >= bp_eos); /* ok provided xname() bounds checking works */
    /* size_t cast: convert signed ptrdiff_t to unsigned size_t */
    bpspaceleft = (size_t) (bp_end - bp_eos);

    if (iflags.override_ID) {
        known = dknown = cknown = bknown = lknown = TRUE;
    } else {
        known = obj->known;
        dknown = obj->dknown;
        cknown = obj->cknown;
        bknown = obj->bknown;
        lknown = obj->lknown;
    }

    /* When using xname, we want "poisoned arrow", and when using
     * doname, we want "poisoned +0 arrow".  This kludge is about the only
     * way to do it, at least until someone overhauls xname() and doname(),
     * combining both into one function taking a parameter.
     */
    /* must check opoisoned--someone can have a weirdly-named fruit */
    if (!strncmp(bp, "poisoned ", 9) && obj->opoisoned) {
        bp += 9; /* doesn't affect bp_eos or bpspaceleft */
        ispoisoned = TRUE;
    }

    /* fruits are allowed to be given artifact names; when that happens,
       format the name like the corresponding artifact, which may or may not
       want "the" prefix and when it doesn't, avoid "a"/"an" prefix too */
    fake_arti = (obj->otyp == SLIME_MOLD
                 && (aname = artifact_name(bp, (short *) 0, FALSE)) != 0);
    force_the = (fake_arti && !strncmpi(aname, "the ", 4));

    prefix[0] = '\0';
    if (obj->quan != 1L) {
        if (dknown || !vague_quan)
            Sprintf(prefix, "%ld ", obj->quan);
        else
            Strcpy(prefix, "几个");
    } else if (obj->otyp == CORPSE) {
        /* skip article prefix for corpses [else corpse_xname()
           would have to be taught how to strip it off again] */
        ;
    } else if (force_the || obj_is_pname(obj) || the_unique_obj(obj)) {
        if (!strncmpi(bp, "the ", 4))
            bp += 4; /* doesn't affect bp_eos or bpspaceleft */
        Strcpy(prefix, "");
    } else if (!fake_arti) {
        /* default prefix */
        Strcpy(prefix, "1 ");
    }

    /* "empty" goes at the beginning, but item count goes at the end */
    if (cknown
        /* bag of tricks: include "empty" prefix if it's known to
           be empty but its precise number of charges isn't known
           (when that is known, suffix of "(n:0)" will be appended,
           making the prefix be redundant; note that 'known' flag
           isn't set when emptiness gets discovered because then
           charging magic would yield known number of new charges);
           horn of plenty isn't a container but is close enough */
        && ((obj->otyp == BAG_OF_TRICKS || obj->otyp == HORN_OF_PLENTY)
             ? (obj->spe == 0 && !known)
             /* not a bag of tricks or horn of plenty: it's empty if
                it is a container that has no contents */
             : ((Is_container(obj) || obj->otyp == STATUE)
                && !Has_contents(obj))))
        Strcat(prefix, "空的");

    if (bknown && obj->oclass != COIN_CLASS
        && (obj->otyp != POT_WATER || !objects[POT_WATER].oc_name_known
            || (!obj->cursed && !obj->blessed))) {
        /* allow 'blessed clear potion' if we don't know it's holy water;
         * always allow "uncursed potion of water"
         */
        if (obj->cursed)
            Strcat(prefix, "被诅咒的");
        else if (obj->blessed)
            Strcat(prefix, "被祝福的");
        else if (!flags.implicit_uncursed
            /* For most items with charges or +/-, if you know how many
             * charges are left or what the +/- is, then you must have
             * totally identified the item, so "uncursed" is unnecessary,
             * because an identified object not described as "blessed" or
             * "cursed" must be uncursed.
             *
             * If the charges or +/- is not known, "uncursed" must be
             * printed to avoid ambiguity between an item whose curse
             * status is unknown, and an item known to be uncursed.
             */
                 || ((!known || !objects[obj->otyp].oc_charged
                      || obj->oclass == ARMOR_CLASS
                      || obj->oclass == RING_CLASS)
#ifdef MAIL_STRUCTURES
                     && obj->otyp != SCR_MAIL
#endif
                     && obj->otyp != FAKE_AMULET_OF_YENDOR
                     && obj->otyp != AMULET_OF_YENDOR
                     && !Role_if(PM_CLERIC)))
            Strcat(prefix, "无诅咒的");
    }

    /* "a large trapped box" would perhaps be more correct; [no!]
       what about ``(obj->tknown && !obj->otrapped)''? shouldn't that
       yield "a non-trapped large box"? (not "an untrapped large box");
       TODO: this should be ``(Is_box(obj) || obj->otyp == TIN) && ...''
       but at present there's no way to set obj->tknown for tins */
    if (Is_box(obj) && obj->otrapped && obj->tknown && obj->dknown)
        Strcat(prefix,"有陷阱的");
    if (lknown && Is_box(obj)) {
        if (obj->obroken)
            /* 3.6.0 used "unlockable" here but that could be misunderstood
               to mean "capable of being unlocked" rather than the intended
               "not capable of being locked" */
            Strcat(prefix, "坏锁的");
        else if (obj->olocked)
            Strcat(prefix, "上锁的");
        else
            Strcat(prefix, "未上锁的");
    }

    if (obj->greased)
        Strcat(prefix, "上油的");

    if (cknown && Has_contents(obj) && bpspaceleft > 0) {
        /* we count the number of separate stacks, which corresponds
           to the number of inventory slots needed to be able to take
           everything out if no merges occur */
        long itemcount = count_contents(obj, FALSE, FALSE, TRUE, FALSE);

        ConcatF2(bp, 0, ",包含%ld个物品%s", itemcount, plur(itemcount)); /*危险:ConcatF2(bp, 0, " containing %ld item%s", itemcount, plur(itemcount)); ConcatF1(prefix, 0, "包含%ld个物品的", itemcount);*/
    }

    switch (is_weptool(obj) ? WEAPON_CLASS : obj->oclass) {
    case AMULET_CLASS:
        if (obj->owornmask & W_AMUL)
            Concat(bp, 0, " (已穿戴)");
        break;
    case ARMOR_CLASS:
        if (obj->owornmask & W_ARMOR) {
            Concat(bp, 0,
                   (obj == uskin) ? " (贴在你的皮肤上)"
                   /* in case of perm_invent update while Wear/Takeoff
                      is in progress; check doffing() before donning()
                      because donning() returns True for both cases */
                   : doffing(obj) ? " (正在脱下)"
                     : donning(obj) ? " (正在穿上)"
                       : " (已穿戴)");
            /* we just added a parenthesized phrase, but the right paren
               might be absent if the appended string got truncated */
            if (bp_eos[-1] == ')') {
                /* slippery fingers is an intrinsic condition of the hero
                   rather than extrinsic condition of objects, but gloves
                   are described as slippery when hero has slippery fingers */
                if (obj == uarmg && Glib) /* just appended "(something)",
                                           * replace paren, changing that
                                           * to be "(something; slippery)" */
                    Concat(bp,  1, "; 滑)");
            }
            if (bp_eos[-1] == ')') {
                /* there could be light-emitting artifact gloves someday,
                   so add 'lit' separately from 'slippery' rather than via
                   'else if' after uarmg+Glib */
                if (!Blind && obj->lamplit && artifact_light(obj))
                    ConcatF1(bp, 1, ",发出%s的光芒)", arti_light_description(obj));
            }
        }
        FALLTHROUGH;
        /*FALLTHRU*/
    case WEAPON_CLASS:
        if (ispoisoned)
            Strcat(prefix, "有毒的");
        add_erosion_words(obj, prefix);
        if (known) {
            Sprintf(eos(prefix), "%+d ", obj->spe); /* sitoa(obj->spe)+" " */
        }
        break;
    case TOOL_CLASS:
        if (obj->owornmask & (W_TOOL | W_SADDLE)) { /* blindfold */
            Concat(bp, 0, " (已穿戴)");
            break;
        }
        if (obj->otyp == LEASH && obj->leashmon != 0) {
            struct monst *mlsh = find_mid(obj->leashmon, FM_FMON);

            if (mlsh && !DEADMONSTER(mlsh)) {
                ConcatF1(bp, 0, " (连接到%s上)", noit_mon_nam(mlsh));
            } else {
                if (mlsh) /*&& DEADMONSTER(mlsh)*/
                    impossible("leashed %s #%u is dead",
                               mon_pmname(mlsh), (unsigned) obj->leashmon);
                else
                    impossible("leashed monster #%u not found",
                               (unsigned) obj->leashmon);
                obj->leashmon = 0;
            }
            break;
        }
        if (obj->otyp == CANDELABRUM_OF_INVOCATION) {
            char suffix[20]; /* longest value is "s attached" */

            /* separately formatted suffix avoids need for ConcatF3() */
            Sprintf(suffix, "%s%s", plur(obj->spe),
                    !obj->lamplit ? "已插上" : "已点燃");
            ConcatF2(bp, 0, " (7个蜡烛中%d个%s)", obj->spe, suffix);
            break;
        } else if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
                   || obj->otyp == BRASS_LANTERN || Is_candle(obj)) {
            if (Is_candle(obj)) {
                anything timer;
                long full_burn_time = 20L * (long) objects[obj->otyp].oc_cost,
                     turns_left = obj->age;

                if (obj->lamplit) {
                    timer = cg.zeroany;
                    timer.a_obj = obj;
                    /* without this, wishing for "lit candle" yields
                       "partly used candle (lit)" because the time it can
                       burn gets adjusted when it becomes lit; matters for
                       the message as it gets added to invent and also if it
                       gets snuffed out immediately (where it will end up as
                       not partly used after all) */
                    turns_left += peek_timer(BURN_OBJECT, &timer) - svm.moves;
                }
                if (turns_left < full_burn_time)
                    Strcat(prefix, "烧掉部分的");
            }
            if (obj->lamplit)
                Concat(bp, 0, " (已点燃)");
            break;
        }
        if (objects[obj->otyp].oc_charged)
            goto charges;
        break;
    case WAND_CLASS:
 charges:
        if (known)
            ConcatF2(bp, 0, " (%d:%d)", (int) obj->recharged, obj->spe);
        break;
    case POTION_CLASS:
        if (obj->otyp == POT_OIL && obj->lamplit)
            Concat(bp, 0, " (已点燃)");
        break;
    case RING_CLASS:
 ring:  /* normal rings reach here 'naturally'; meat ring jumps here */
        if (obj->owornmask & W_RINGR)
            Concat(bp, 0, " (在右");
        if (obj->owornmask & W_RINGL)
            Concat(bp, 0, " (在右");
        if (obj->owornmask & W_RING) /* either left or right */
            ConcatF1(bp, 0,"%s上)", body_part(HAND));
        if (known && objects[obj->otyp].oc_charged) {
            Sprintf(eos(prefix), "%+d ", obj->spe); /* sitoa(obj->spe)+" " */
        }
        break;
    case FOOD_CLASS:
        if (obj->oeaten)
            Strcat(prefix, "部分吃掉的");
        if (obj->otyp == CORPSE) {
            /* (quan == 1) => want corpse_xname() to supply article,
               (quan != 1) => already have count or "some" as prefix;
               "corpse" is already in the buffer returned by xname() */
            unsigned cxarg = (((obj->quan != 1L) ? 0 : CXN_ARTICLE)
                              | CXN_NOCORPSE);
            char *cxstr, *save_xnamep;

            /* corpse_xname() sets xnamep; callers other than doname_base()
               itself shouldn't care about xnamep (pointer to start of
               current obuf[]) but keep it accurate anyway */
            save_xnamep = gx.xnamep;
            cxstr = corpse_xname(obj, prefix, cxarg);
            Sprintf(prefix, "%s", cxstr);
            /* avoid having doname(corpse) consume an extra obuf */
            releaseobuf(cxstr);
            gx.xnamep = save_xnamep;
        } else if (obj->otyp == EGG) {
#if 0 /* corpses don't tell if they're stale either */
            if (known && stale_egg(obj))
                Strcat(prefix, "stale ");
#endif
            if (ismnum(omndx)
                && (known || (svm.mvitals[omndx].mvflags & MV_KNOWS_EGG))) {
                Strcat(prefix, mons[omndx].pmnames[NEUTRAL]);
                Strcat(prefix, " ");
                if (obj->spe == 1)
                    Concat(bp, 0, " (你下的)");
            }
        } else if (obj->otyp == MEAT_RING) {
            goto ring;
        }
        break;
    case BALL_CLASS:
    case CHAIN_CLASS:
        add_erosion_words(obj, prefix);
        if (obj->owornmask & (W_BALL | W_CHAIN))
            ConcatF1(bp, 0, " (%s你身上)",
                     (obj->owornmask & W_BALL) ? "通过铁链连接到" : "连接到");
        break;
    }

    if ((obj->otyp == STATUE || obj->otyp == CORPSE || obj->otyp == FIGURINE)
        && wizard && iflags.wizmgender) {
        int cgend = (obj->spe & CORPSTAT_GENDER),
            mgend = ((cgend == CORPSTAT_MALE) ? MALE
                     : (cgend == CORPSTAT_FEMALE) ? FEMALE
                       : NEUTRAL);

        ConcatF1(bp, 0, " (%s)",
                 (cgend != CORPSTAT_RANDOM) ? genders[mgend].adj
                                            : "未指定性别");
    }

    if ((obj->owornmask & W_WEP) && !gm.mrg_to_wielded) {
        boolean twoweap_primary = (obj == uwep && u.twoweap),
                tethered = (obj->otyp == AKLYS);


        /* use alternate phrasing for non-weapons and for wielded ammo
           (arrows, bolts), or missiles (darts, shuriken, boomerangs)
           except when those are being actively dual-wielded where the
           regular phrasing will list them as "in right hand" to
           contrast with secondary weapon's "in left hand" */
        if ((obj->quan != 1L
             || ((obj->oclass == WEAPON_CLASS)
                 ? (is_ammo(obj) || is_missile(obj))
                 : !is_weptool(obj)))
            && !twoweap_primary) {
            Concat(bp, 0, " (已装备)");
        } else {
            const char *hand_s = body_part(HAND);
            char *obufp, handsbuf[40];

            if (bimanual(obj)) { /* "hands" */
                hand_s = strcpy(handsbuf, obufp = makeplural(hand_s));
                releaseobuf(obufp);
            } else { /* "right hand" or "left hand" */
                Sprintf(handsbuf, "%s%s",
                        URIGHTY ? "右" : "左", hand_s);
                hand_s = handsbuf;
            }
            /* note: Sting's glow message, if added, will insert text
               in front of "(weapon in hand)"'s closing paren */
            ConcatF2(bp, 0, " (%s%s)", /*修改语序:懒得写了。自己看吧。*/
                     hand_s,
                     tethered ? "拿住"
                     : twoweap_primary ? "装备"
                       : "武器");

            /* we just added a parenthesized phrase, but the right paren
               might be absent if the appended string got truncated */
            if (!Blind && bpspaceleft && bp_eos[-1] == ')') {
                if (gw.warn_obj_cnt && obj == uwep
                    && (EWarn_of_mon & W_WEP) != 0L)
                    /* we know bp[] ends with ')'; overwrite that */
                    ConcatF2(bp, 1, ", %s%s)",
                             glow_verb(gw.warn_obj_cnt, TRUE),
                             glow_color(obj->oartifact));
                else if (obj->lamplit && artifact_light(obj))
                    /* as above, overwrite known closing paren */
                    ConcatF1(bp, 1, ",发出%s的光芒)",
                             arti_light_description(obj));
            }
        }
    }
    if (obj->owornmask & W_SWAPWEP) {
        if (u.twoweap)
            ConcatF2(bp, 0, " (%s%s装备)",
                     URIGHTY ? "左" : "右", body_part(HAND));
        else
            /* TODO: rephrase this when obj isn't a weapon or weptool */
            ConcatF1(bp, 0, " (副武器%s;未装备)",
                     plur(obj->quan));
    }
    if (obj->owornmask & W_QUIVER) {
        int Qtyp;

        switch (obj->oclass) {
        case WEAPON_CLASS:
            Qtyp = !is_ammo(obj) ? 3 /* not ammo: "at the ready" */
                   : (objects[obj->otyp].oc_skill != -P_BOW) ? 2 /* non-bow */
                     : 1; /* ammo for a bow: "in quiver" */
            break;
        case RING_CLASS:
        case AMULET_CLASS:
        case WAND_CLASS:
        case COIN_CLASS:
        case GEM_CLASS:
            Qtyp = 2; /* small, non-bow: "in quiver pouch" */
            break;
        default: /* odd things */
            Qtyp = 3; /* "at the ready" */
            break;
        }
        ConcatF1(bp, 0, " (%s)",
                 (Qtyp == 1) ? "在箭筒中"
                 : (Qtyp == 2) ? "在箭袋中"
                   : "准备好");
    }

    /* treat 'restoring' like suppress_price because shopkeeper and
       bill might not be available yet while restore is in progress
       (objects won't normally be formatted during that time, but if
       'perm_invent' is enabled then they might be [not any more...]) */
    if (iflags.suppress_price || program_state.restoring) {
        ; /* don't attempt to obtain any shop pricing, even if 'with_price' */
    } else if (is_unpaid(obj)) { /* in inventory or in container in invent */
        char pricebuf[40];
        long quotedprice = unpaid_cost(obj, COST_CONTENTS);

        /* separately formatted suffix avoids need for ConcatF3() */
        Sprintf(pricebuf, "%ld%s", quotedprice, currency(quotedprice));
        ConcatF2(bp, 0, " (%s,%s)",
                 obj->unpaid ? "未付款" : "内容物", pricebuf);

        record_price_quote(obj->otyp, quotedprice / obj->quan, TRUE);
    } else if (with_price) { /* on floor or in container on floor */
        int nochrg = 0;
        long price = get_cost_of_shop_item(obj, &nochrg);

        if (price > 0L) {
            char pricebuf[40];

            Sprintf(pricebuf, "%ld%s", price, currency(price));
            ConcatF2(bp, 0, " (%s,%s)",
                     nochrg ? "内容物" : "出售", pricebuf);
        } else if (nochrg > 0) {
            Concat(bp, 0, " (免费)");
        } else if (iflags.pricequotes && !objects[obj->otyp].oc_name_known) {
            append_price_quote(bp, &bp_eos, obj->otyp);
        }

        if (price > 0L)
            record_price_quote(obj->otyp, price / obj->quan, TRUE);
    } else if (iflags.pricequotes && !objects[obj->otyp].oc_name_known) {
        append_price_quote(bp, &bp_eos, obj->otyp);
    }

    if (!strncmp(prefix, "a ", 2)) {
        /* save current prefix, without "a "; might be empty */
        Strcpy(tmpbuf, prefix + 2);
        /* set prefix[] to "", "a ", or "an " */
        (void) just_an(prefix, *tmpbuf ? tmpbuf : bp);
        /* append remainder of original prefix */
        Strcat(prefix, tmpbuf);
    }

    /* show weight for items (debug tourist info);
       "aum" is stolen from Crawl's "Arbitrary Unit of Measure" */
    if (wizard && iflags.wizweight) {
        /* wizard mode user has asked to see object weights */
        if (with_price && bp_eos[-1] == ')')
            ConcatF1(bp, 1, ", %u aum)", obj->owt);
        else
            ConcatF1(bp, 0, " (%u aum)", obj->owt);

        /* ConcatF1(bp) updates bp_eos and bpspaceleft but we're done
           with them now; add a fake use so compiler won't complain
           about a variable assignment that won't be subsequently used */
        nhUse(bp_eos);
        nhUse(bpspaceleft);
    }

    bp = strprepend(bp, prefix);

    /*
     * Last gasp bounds check.
     *
     * If caller intends this to be for a menu entry, make sure that
     * there is some room to combine with menu selector prefix without
     * exceeding BUFSZ-1.
     *
     * offsetbp=4: width of menu entry selector text: "c - " for tty.
     * For curses, that wastes a char since it only needs 3: "c) ".
     *
     * Reaching full BUFSZ-1 length can't happen unless both doname
     * (BUFSZ-PREFIX) and strprepend (PREFIX) use up all available
     * space or one of them overflows without being detected.
     */
    if (strlen(bp) > BUFSZ - 1) {
        paniclog("doname", bp);
        /* ideally this will never happen; if xnamep is any obuf[]
           other than the last, overflow here would be relatively
           benign and we could probably keep going */
        panic("doname: long object description overflow.");
        /*NOTREACHED*/
    } else {
        static int doname_full = 0;
        int offsetbp = for_menu ? 4 : 0;

        if (strlen(bp) + offsetbp >= BUFSZ - 1) {
            /* for !offsetbp, we'll only get here if strlen(bp)==BUFSZ-1 */
            if (!doname_full++) {
                paniclog("doname", bp);
                Sprintf(tmpbuf, "long object description%s.",
                        offsetbp ? " truncated for menu use" : "");
                paniclog("doname", tmpbuf);
            }
            bp[BUFSZ - 1 - offsetbp] = '\0';
        }
    }

    return bp;
}

/* format a corpse name (xname() omits monster type; doname() calls us);
   eatcorpse() also uses us for death reason when eating tainted glob */
char *
corpse_xname(
    struct obj *otmp,
    const char *adjective,
    unsigned cxn_flags) /* bitmask of CXN_xxx values */
{
    char *nambuf;
    int omndx = otmp->corpsenm;
    boolean ignore_quan = (cxn_flags & CXN_SINGULAR) != 0,
            /* suppress "the" from "the unique monster corpse" */
        no_prefix = (cxn_flags & CXN_NO_PFX) != 0,
            /* include "the" for "the woodchuck corpse */
        the_prefix = (cxn_flags & CXN_PFX_THE) != 0,
            /* include "an" for "an ogre corpse */
        any_prefix = (cxn_flags & CXN_ARTICLE) != 0,
            /* leave off suffix (do_name() appends "corpse" itself) */
        omit_corpse = (cxn_flags & CXN_NOCORPSE) != 0,
        possessive = FALSE,
        glob = (otmp->otyp != CORPSE && otmp->globby);
    const char *mnam;

    /* some callers [aobjnam()] rely on prefix area that xname() sets aside */
    gx.xnamep = nextobuf();
    nambuf = gx.xnamep + PREFIX;

    if (glob) {
        mnam = OBJ_NAME(objects[otmp->otyp]); /* "glob of <monster>" */
    } else if (omndx == NON_PM) { /* paranoia */
        mnam = "thing";
    } else {
        mnam = obj_pmname(otmp);
        if (the_unique_pm(&mons[omndx]) || type_is_pname(&mons[omndx])) {
            mnam = s_suffix(mnam);
            possessive = TRUE;
            /* don't precede personal name like "Medusa" with an article */
            if (type_is_pname(&mons[omndx]))
                no_prefix = TRUE;
            /* always precede non-personal unique monster name like
               "Oracle" with "the" unless explicitly overridden */
            else if (the_unique_pm(&mons[omndx]) && !no_prefix)
                the_prefix = TRUE;
        }
    }
    if (no_prefix)
        the_prefix = any_prefix = FALSE;
    else if (the_prefix)
        any_prefix = FALSE; /* mutually exclusive */

    *nambuf = '\0';
    /* can't use the() the way we use an() below because any capitalized
       Name causes it to assume a personal name and return Name as-is;
       that's usually the behavior wanted, but here we need to force "the"
       to precede capitalized unique monsters (pnames are handled above) */
    if (the_prefix)
        Strcat(nambuf, "");
    /* note: over time, various instances of the(mon_name()) have crept
       into the code, so the() has been modified to deal with capitalized
       monster names; we could switch to using it below like an() */

    if (!adjective || !*adjective) {
        /* normal case:  newt corpse */
        Strcat(nambuf, mnam);
    } else {
        /* adjective positioning depends upon format of monster name */
        if (possessive) /* Medusa's cursed partly eaten corpse */
            Sprintf(eos(nambuf), "%s的%s", mnam, adjective);
        else /* cursed partly eaten troll corpse */
            Sprintf(eos(nambuf), "%s的%s", adjective, mnam);
        /* in case adjective has a trailing space, squeeze it out */
        mungspaces(nambuf);
        /* doname() might include a count in the adjective argument;
           if so, don't prepend an article */
        if (digit(*adjective))
            any_prefix = FALSE;
    }

    if (glob) {
        ; /* omit_corpse doesn't apply; quantity is always 1 */
    } else if (!omit_corpse) {
        Strcat(nambuf, "尸体");
        /* makeplural(nambuf) => append "s" to "corpse" */
        if (otmp->quan > 1L && !ignore_quan) {
            Strcat(nambuf, "");
            any_prefix = FALSE; /* avoid "a newt corpses" */
        }
    }

    /* it's safe to overwrite our nambuf[] after an() has copied its
       old value into another buffer; and once _that_ has been copied,
       the obuf[] returned by an() can be made available for re-use */
    if (any_prefix) {
        char *obufp;

        Strcpy(nambuf, obufp = an(nambuf));
        releaseobuf(obufp);
    }
    return nambuf;
}

char *
corpse_xename(
    struct obj *otmp,
    const char *adjective,
    unsigned cxn_flags) /* bitmask of CXN_xxx values */
{
    char *nambuf;
    int omndx = otmp->corpsenm;
    boolean ignore_quan = (cxn_flags & CXN_SINGULAR) != 0,
            /* suppress "the" from "the unique monster corpse" */
        no_prefix = (cxn_flags & CXN_NO_PFX) != 0,
            /* include "the" for "the woodchuck corpse */
        the_prefix = (cxn_flags & CXN_PFX_THE) != 0,
            /* include "an" for "an ogre corpse */
        any_prefix = (cxn_flags & CXN_ARTICLE) != 0,
            /* leave off suffix (do_name() appends "corpse" itself) */
        omit_corpse = (cxn_flags & CXN_NOCORPSE) != 0,
        possessive = FALSE,
        glob = (otmp->otyp != CORPSE && otmp->globby);
    const char *mnam;

    /* some callers [aobjnam()] rely on prefix area that xname() sets aside */
    gx.xnamep = nextobuf();
    nambuf = gx.xnamep + PREFIX;

    if (glob) {
        mnam = OBJ_ENAME(objects[otmp->otyp]); /* "glob of <monster>" */
    } else if (omndx == NON_PM) { /* paranoia */
        mnam = "thing";
    } else {
        mnam = obj_pmname(otmp);
        if (the_unique_pm(&mons[omndx]) || type_is_pname(&mons[omndx])) {
            mnam = s_suffix(mnam);
            possessive = TRUE;
            /* don't precede personal name like "Medusa" with an article */
            if (type_is_pname(&mons[omndx]))
                no_prefix = TRUE;
            /* always precede non-personal unique monster name like
               "Oracle" with "the" unless explicitly overridden */
            else if (the_unique_pm(&mons[omndx]) && !no_prefix)
                the_prefix = TRUE;
        }
    }
    if (no_prefix)
        the_prefix = any_prefix = FALSE;
    else if (the_prefix)
        any_prefix = FALSE; /* mutually exclusive */

    *nambuf = '\0';
    /* can't use the() the way we use an() below because any capitalized
       Name causes it to assume a personal name and return Name as-is;
       that's usually the behavior wanted, but here we need to force "the"
       to precede capitalized unique monsters (pnames are handled above) */
    if (the_prefix)
        Strcat(nambuf, "the ");
    /* note: over time, various instances of the(mon_name()) have crept
       into the code, so the() has been modified to deal with capitalized
       monster names; we could switch to using it below like an() */

    if (!adjective || !*adjective) {
        /* normal case:  newt corpse */
        Strcat(nambuf, mnam);
    } else {
        /* adjective positioning depends upon format of monster name */
        if (possessive) /* Medusa's cursed partly eaten corpse */
            Sprintf(eos(nambuf), "%s %s", mnam, adjective);
        else /* cursed partly eaten troll corpse */
            Sprintf(eos(nambuf), "%s %s", adjective, mnam);
        /* in case adjective has a trailing space, squeeze it out */
        mungspaces(nambuf);
        /* doname() might include a count in the adjective argument;
           if so, don't prepend an article */
        if (digit(*adjective))
            any_prefix = FALSE;
    }

    if (glob) {
        ; /* omit_corpse doesn't apply; quantity is always 1 */
    } else if (!omit_corpse) {
        Strcat(nambuf, " corpse");
        /* makeplural(nambuf) => append "s" to "corpse" */
        if (otmp->quan > 1L && !ignore_quan) {
            Strcat(nambuf, "s");
            any_prefix = FALSE; /* avoid "a newt corpses" */
        }
    }

    /* it's safe to overwrite our nambuf[] after an() has copied its
       old value into another buffer; and once _that_ has been copied,
       the obuf[] returned by an() can be made available for re-use */
    if (any_prefix) {
        char *obufp;

        Strcpy(nambuf, obufp = an(nambuf));
        releaseobuf(obufp);
    }
    return nambuf;
}

staticfn char *
doename_base(
    struct obj *obj,       /* object to format */
    unsigned doname_flags) /* special case requests */
{
    boolean ispoisoned = FALSE,
            with_price = (doname_flags & DONAME_WITH_PRICE) != 0,
            vague_quan = (doname_flags & DONAME_VAGUE_QUAN) != 0,
            for_menu = (doname_flags & DONAME_FOR_MENU) != 0;
    boolean known, dknown, cknown, bknown, lknown,
            fake_arti, force_the;
    char prefix[PREFIX];
    char tmpbuf[PREFIX + 1]; /* for when we have to add something at
                              * the start of prefix instead of the
                              * end (Strcat is used on the end) */
    const char *aname = 0;
    int omndx = obj->corpsenm;
    char *bp;
    char *bp_eos, *bp_end;
    size_t bpspaceleft;

    /* 'bp' will be within an obuf[] rather than at the start of one,
       usually (but not always) pointing at &obuf[PREFIX];
       gx.xnamep always points to the start of that buffer;
       'bp_eos' and 'bpspaceleft' are used and updated by Concat*() macros */
    bp = xename(obj);
    bp_end = gx.xnamep + BUFSZ - 1;
    bp_eos = eos(bp);
    assert(bp_end >= bp_eos); /* ok provided xname() bounds checking works */
    /* size_t cast: convert signed ptrdiff_t to unsigned size_t */
    bpspaceleft = (size_t) (bp_end - bp_eos);

    if (iflags.override_ID) {
        known = dknown = cknown = bknown = lknown = TRUE;
    } else {
        known = obj->known;
        dknown = obj->dknown;
        cknown = obj->cknown;
        bknown = obj->bknown;
        lknown = obj->lknown;
    }

    /* When using xname, we want "poisoned arrow", and when using
     * doname, we want "poisoned +0 arrow".  This kludge is about the only
     * way to do it, at least until someone overhauls xname() and doname(),
     * combining both into one function taking a parameter.
     */
    /* must check opoisoned--someone can have a weirdly-named fruit */
    if (!strncmp(bp, "poisoned ", 9) && obj->opoisoned) {
        bp += 9; /* doesn't affect bp_eos or bpspaceleft */
        ispoisoned = TRUE;
    }

    /* fruits are allowed to be given artifact names; when that happens,
       format the name like the corresponding artifact, which may or may not
       want "the" prefix and when it doesn't, avoid "a"/"an" prefix too */
    fake_arti = (obj->otyp == SLIME_MOLD
                 && (aname = artifact_ename(bp, (short *) 0, FALSE)) != 0);
    force_the = (fake_arti && !strncmpi(aname, "the ", 4));

    prefix[0] = '\0';
    if (obj->quan != 1L) {
        if (dknown || !vague_quan)
            Sprintf(prefix, "%ld ", obj->quan);
        else
            Strcpy(prefix, "some ");
    } else if (obj->otyp == CORPSE) {
        /* skip article prefix for corpses [else corpse_xname()
           would have to be taught how to strip it off again] */
        ;
    } else if (force_the || obj_is_pname(obj) || the_unique_obj(obj)) {
        if (!strncmpi(bp, "the ", 4))
            bp += 4; /* doesn't affect bp_eos or bpspaceleft */
        Strcpy(prefix, "the ");
    } else if (!fake_arti) {
        /* default prefix */
        Strcpy(prefix, "a ");
    }

    /* "empty" goes at the beginning, but item count goes at the end */
    if (cknown
        /* bag of tricks: include "empty" prefix if it's known to
           be empty but its precise number of charges isn't known
           (when that is known, suffix of "(n:0)" will be appended,
           making the prefix be redundant; note that 'known' flag
           isn't set when emptiness gets discovered because then
           charging magic would yield known number of new charges);
           horn of plenty isn't a container but is close enough */
        && ((obj->otyp == BAG_OF_TRICKS || obj->otyp == HORN_OF_PLENTY)
             ? (obj->spe == 0 && !known)
             /* not a bag of tricks or horn of plenty: it's empty if
                it is a container that has no contents */
             : ((Is_container(obj) || obj->otyp == STATUE)
                && !Has_contents(obj))))
        Strcat(prefix, "empty ");

    if (bknown && obj->oclass != COIN_CLASS
        && (obj->otyp != POT_WATER || !objects[POT_WATER].oc_name_known
            || (!obj->cursed && !obj->blessed))) {
        /* allow 'blessed clear potion' if we don't know it's holy water;
         * always allow "uncursed potion of water"
         */
        if (obj->cursed)
            Strcat(prefix, "cursed ");
        else if (obj->blessed)
            Strcat(prefix, "blessed ");
        else if (!flags.implicit_uncursed
            /* For most items with charges or +/-, if you know how many
             * charges are left or what the +/- is, then you must have
             * totally identified the item, so "uncursed" is unnecessary,
             * because an identified object not described as "blessed" or
             * "cursed" must be uncursed.
             *
             * If the charges or +/- is not known, "uncursed" must be
             * printed to avoid ambiguity between an item whose curse
             * status is unknown, and an item known to be uncursed.
             */
                 || ((!known || !objects[obj->otyp].oc_charged
                      || obj->oclass == ARMOR_CLASS
                      || obj->oclass == RING_CLASS)
#ifdef MAIL_STRUCTURES
                     && obj->otyp != SCR_MAIL
#endif
                     && obj->otyp != FAKE_AMULET_OF_YENDOR
                     && obj->otyp != AMULET_OF_YENDOR
                     && !Role_if(PM_CLERIC)))
            Strcat(prefix, "uncursed ");
    }

    /* "a large trapped box" would perhaps be more correct; [no!]
       what about ``(obj->tknown && !obj->otrapped)''? shouldn't that
       yield "a non-trapped large box"? (not "an untrapped large box");
       TODO: this should be ``(Is_box(obj) || obj->otyp == TIN) && ...''
       but at present there's no way to set obj->tknown for tins */
    if (Is_box(obj) && obj->otrapped && obj->tknown && obj->dknown)
        Strcat(prefix,"trapped ");
    if (lknown && Is_box(obj)) {
        if (obj->obroken)
            /* 3.6.0 used "unlockable" here but that could be misunderstood
               to mean "capable of being unlocked" rather than the intended
               "not capable of being locked" */
            Strcat(prefix, "broken ");
        else if (obj->olocked)
            Strcat(prefix, "locked ");
        else
            Strcat(prefix, "unlocked ");
    }

    if (obj->greased)
        Strcat(prefix, "greased ");

    if (cknown && Has_contents(obj) && bpspaceleft > 0) {
        /* we count the number of separate stacks, which corresponds
           to the number of inventory slots needed to be able to take
           everything out if no merges occur */
        long itemcount = count_contents(obj, FALSE, FALSE, TRUE, FALSE);

        ConcatF2(bp, 0, " containing %ld item%s", itemcount, plur(itemcount));
    }

    switch (is_weptool(obj) ? WEAPON_CLASS : obj->oclass) {
    case AMULET_CLASS:
        if (obj->owornmask & W_AMUL)
            Concat(bp, 0, " (being worn)");
        break;
    case ARMOR_CLASS:
        if (obj->owornmask & W_ARMOR) {
            Concat(bp, 0,
                   (obj == uskin) ? " (embedded in your skin)"
                   /* in case of perm_invent update while Wear/Takeoff
                      is in progress; check doffing() before donning()
                      because donning() returns True for both cases */
                   : doffing(obj) ? " (being doffed)"
                     : donning(obj) ? " (being donned)"
                       : " (being worn)");
            /* we just added a parenthesized phrase, but the right paren
               might be absent if the appended string got truncated */
            if (bp_eos[-1] == ')') {
                /* slippery fingers is an intrinsic condition of the hero
                   rather than extrinsic condition of objects, but gloves
                   are described as slippery when hero has slippery fingers */
                if (obj == uarmg && Glib) /* just appended "(something)",
                                           * replace paren, changing that
                                           * to be "(something; slippery)" */
                    Concat(bp,  1, "; slippery)");
            }
            if (bp_eos[-1] == ')') {
                /* there could be light-emitting artifact gloves someday,
                   so add 'lit' separately from 'slippery' rather than via
                   'else if' after uarmg+Glib */
                if (!Blind && obj->lamplit && artifact_light(obj))
                    ConcatF1(bp, 1, ", %s lit)", arti_light_description(obj));
            }
        }
        FALLTHROUGH;
        /*FALLTHRU*/
    case WEAPON_CLASS:
        if (ispoisoned)
            Strcat(prefix, "poisoned ");
        add_erosion_words(obj, prefix);
        if (known) {
            Sprintf(eos(prefix), "%+d ", obj->spe); /* sitoa(obj->spe)+" " */
        }
        break;
    case TOOL_CLASS:
        if (obj->owornmask & (W_TOOL | W_SADDLE)) { /* blindfold */
            Concat(bp, 0, " (being worn)");
            break;
        }
        if (obj->otyp == LEASH && obj->leashmon != 0) {
            struct monst *mlsh = find_mid(obj->leashmon, FM_FMON);

            if (mlsh && !DEADMONSTER(mlsh)) {
                ConcatF1(bp, 0, " (attached to %s)", noit_mon_nam(mlsh));
            } else {
                if (mlsh) /*&& DEADMONSTER(mlsh)*/
                    impossible("leashed %s #%u is dead",
                               mon_pmname(mlsh), (unsigned) obj->leashmon);
                else
                    impossible("leashed monster #%u not found",
                               (unsigned) obj->leashmon);
                obj->leashmon = 0;
            }
            break;
        }
        if (obj->otyp == CANDELABRUM_OF_INVOCATION) {
            char suffix[20]; /* longest value is "s attached" */

            /* separately formatted suffix avoids need for ConcatF3() */
            Sprintf(suffix, "%s%s", plur(obj->spe),
                    !obj->lamplit ? " attached" : ", lit");
            ConcatF2(bp, 0, " (%d of 7 candle%s)", obj->spe, suffix);
            break;
        } else if (obj->otyp == OIL_LAMP || obj->otyp == MAGIC_LAMP
                   || obj->otyp == BRASS_LANTERN || Is_candle(obj)) {
            if (Is_candle(obj)) {
                anything timer;
                long full_burn_time = 20L * (long) objects[obj->otyp].oc_cost,
                     turns_left = obj->age;

                if (obj->lamplit) {
                    timer = cg.zeroany;
                    timer.a_obj = obj;
                    /* without this, wishing for "lit candle" yields
                       "partly used candle (lit)" because the time it can
                       burn gets adjusted when it becomes lit; matters for
                       the message as it gets added to invent and also if it
                       gets snuffed out immediately (where it will end up as
                       not partly used after all) */
                    turns_left += peek_timer(BURN_OBJECT, &timer) - svm.moves;
                }
                if (turns_left < full_burn_time)
                    Strcat(prefix, "partly used ");
            }
            if (obj->lamplit)
                Concat(bp, 0, " (lit)");
            break;
        }
        if (objects[obj->otyp].oc_charged)
            goto charges;
        break;
    case WAND_CLASS:
 charges:
        if (known)
            ConcatF2(bp, 0, " (%d:%d)", (int) obj->recharged, obj->spe);
        break;
    case POTION_CLASS:
        if (obj->otyp == POT_OIL && obj->lamplit)
            Concat(bp, 0, " (lit)");
        break;
    case RING_CLASS:
 ring:  /* normal rings reach here 'naturally'; meat ring jumps here */
        if (obj->owornmask & W_RINGR)
            Concat(bp, 0, " (on right ");
        if (obj->owornmask & W_RINGL)
            Concat(bp, 0, " (on left ");
        if (obj->owornmask & W_RING) /* either left or right */
            ConcatF1(bp, 0,"%s)", body_part(HAND));
        if (known && objects[obj->otyp].oc_charged) {
            Sprintf(eos(prefix), "%+d ", obj->spe); /* sitoa(obj->spe)+" " */
        }
        break;
    case FOOD_CLASS:
        if (obj->oeaten)
            Strcat(prefix, "partly eaten ");
        if (obj->otyp == CORPSE) {
            /* (quan == 1) => want corpse_xname() to supply article,
               (quan != 1) => already have count or "some" as prefix;
               "corpse" is already in the buffer returned by xname() */
            unsigned cxarg = (((obj->quan != 1L) ? 0 : CXN_ARTICLE)
                              | CXN_NOCORPSE);
            char *cxstr, *save_xnamep;

            /* corpse_xname() sets xnamep; callers other than doname_base()
               itself shouldn't care about xnamep (pointer to start of
               current obuf[]) but keep it accurate anyway */
            save_xnamep = gx.xnamep;
            cxstr = corpse_xename(obj, prefix, cxarg);
            Sprintf(prefix, "%s ", cxstr);
            /* avoid having doname(corpse) consume an extra obuf */
            releaseobuf(cxstr);
            gx.xnamep = save_xnamep;
        } else if (obj->otyp == EGG) {
#if 0 /* corpses don't tell if they're stale either */
            if (known && stale_egg(obj))
                Strcat(prefix, "stale ");
#endif
            if (ismnum(omndx)
                && (known || (svm.mvitals[omndx].mvflags & MV_KNOWS_EGG))) {
                Strcat(prefix, mons[omndx].pmnames[NEUTRAL]);
                Strcat(prefix, " ");
                if (obj->spe == 1)
                    Concat(bp, 0, " (laid by you)");
            }
        } else if (obj->otyp == MEAT_RING) {
            goto ring;
        }
        break;
    case BALL_CLASS:
    case CHAIN_CLASS:
        add_erosion_words(obj, prefix);
        if (obj->owornmask & (W_BALL | W_CHAIN))
            ConcatF1(bp, 0, " (%s to you)",
                     (obj->owornmask & W_BALL) ? "chained" : "attached");
        break;
    }

    if ((obj->otyp == STATUE || obj->otyp == CORPSE || obj->otyp == FIGURINE)
        && wizard && iflags.wizmgender) {
        int cgend = (obj->spe & CORPSTAT_GENDER),
            mgend = ((cgend == CORPSTAT_MALE) ? MALE
                     : (cgend == CORPSTAT_FEMALE) ? FEMALE
                       : NEUTRAL);

        ConcatF1(bp, 0, " (%s)",
                 (cgend != CORPSTAT_RANDOM) ? genders[mgend].adj
                                            : "unspecified gender");
    }

    if ((obj->owornmask & W_WEP) && !gm.mrg_to_wielded) {
        boolean twoweap_primary = (obj == uwep && u.twoweap),
                tethered = (obj->otyp == AKLYS);


        /* use alternate phrasing for non-weapons and for wielded ammo
           (arrows, bolts), or missiles (darts, shuriken, boomerangs)
           except when those are being actively dual-wielded where the
           regular phrasing will list them as "in right hand" to
           contrast with secondary weapon's "in left hand" */
        if ((obj->quan != 1L
             || ((obj->oclass == WEAPON_CLASS)
                 ? (is_ammo(obj) || is_missile(obj))
                 : !is_weptool(obj)))
            && !twoweap_primary) {
            Concat(bp, 0, " (wielded)");
        } else {
            const char *hand_s = body_part(HAND);
            char *obufp, handsbuf[40];

            if (bimanual(obj)) { /* "hands" */
                hand_s = strcpy(handsbuf, obufp = makeplural(hand_s));
                releaseobuf(obufp);
            } else { /* "right hand" or "left hand" */
                Sprintf(handsbuf, "%s %s",
                        URIGHTY ? "right" : "left", hand_s);
                hand_s = handsbuf;
            }
            /* note: Sting's glow message, if added, will insert text
               in front of "(weapon in hand)"'s closing paren */
            ConcatF2(bp, 0, " (%s %s)",
                     tethered ? "tethered to"
                     : twoweap_primary ? "wielded in"
                       : "weapon in",
                     hand_s);

            /* we just added a parenthesized phrase, but the right paren
               might be absent if the appended string got truncated */
            if (!Blind && bpspaceleft && bp_eos[-1] == ')') {
                if (gw.warn_obj_cnt && obj == uwep
                    && (EWarn_of_mon & W_WEP) != 0L)
                    /* we know bp[] ends with ')'; overwrite that */
                    ConcatF2(bp, 1, ", %s %s)",
                             glow_verb(gw.warn_obj_cnt, TRUE),
                             glow_color(obj->oartifact));
                else if (obj->lamplit && artifact_light(obj))
                    /* as above, overwrite known closing paren */
                    ConcatF1(bp, 1, ", %s lit)",
                             arti_light_description(obj));
            }
        }
    }
    if (obj->owornmask & W_SWAPWEP) {
        if (u.twoweap)
            ConcatF2(bp, 0, " (wielded in %s %s)",
                     URIGHTY ? "left" : "right", body_part(HAND));
        else
            /* TODO: rephrase this when obj isn't a weapon or weptool */
            ConcatF1(bp, 0, " (alternate weapon%s; not wielded)",
                     plur(obj->quan));
    }
    if (obj->owornmask & W_QUIVER) {
        int Qtyp;

        switch (obj->oclass) {
        case WEAPON_CLASS:
            Qtyp = !is_ammo(obj) ? 3 /* not ammo: "at the ready" */
                   : (objects[obj->otyp].oc_skill != -P_BOW) ? 2 /* non-bow */
                     : 1; /* ammo for a bow: "in quiver" */
            break;
        case RING_CLASS:
        case AMULET_CLASS:
        case WAND_CLASS:
        case COIN_CLASS:
        case GEM_CLASS:
            Qtyp = 2; /* small, non-bow: "in quiver pouch" */
            break;
        default: /* odd things */
            Qtyp = 3; /* "at the ready" */
            break;
        }
        ConcatF1(bp, 0, " (%s)",
                 (Qtyp == 1) ? "in quiver"
                 : (Qtyp == 2) ? "in quiver pouch"
                   : "at the ready");
    }

    /* treat 'restoring' like suppress_price because shopkeeper and
       bill might not be available yet while restore is in progress
       (objects won't normally be formatted during that time, but if
       'perm_invent' is enabled then they might be [not any more...]) */
    if (iflags.suppress_price || program_state.restoring) {
        ; /* don't attempt to obtain any shop pricing, even if 'with_price' */
    } else if (is_unpaid(obj)) { /* in inventory or in container in invent */
        char pricebuf[40];
        long quotedprice = unpaid_cost(obj, COST_CONTENTS);

        /* separately formatted suffix avoids need for ConcatF3() */
        Sprintf(pricebuf, "%ld %s", quotedprice, currency(quotedprice));
        ConcatF2(bp, 0, " (%s, %s)",
                 obj->unpaid ? "unpaid" : "contents", pricebuf);

        record_price_quote(obj->otyp, quotedprice / obj->quan, TRUE);
    } else if (with_price) { /* on floor or in container on floor */
        int nochrg = 0;
        long price = get_cost_of_shop_item(obj, &nochrg);

        if (price > 0L) {
            char pricebuf[40];

            Sprintf(pricebuf, "%ld %s", price, currency(price));
            ConcatF2(bp, 0, " (%s, %s)",
                     nochrg ? "contents" : "for sale", pricebuf);
        } else if (nochrg > 0) {
            Concat(bp, 0, " (no charge)");
        } else if (iflags.pricequotes && !objects[obj->otyp].oc_name_known) {
            append_price_quote(bp, &bp_eos, obj->otyp);
        }

        if (price > 0L)
            record_price_quote(obj->otyp, price / obj->quan, TRUE);
    } else if (iflags.pricequotes && !objects[obj->otyp].oc_name_known) {
        append_price_quote(bp, &bp_eos, obj->otyp);
    }

    if (!strncmp(prefix, "a ", 2)) {
        /* save current prefix, without "a "; might be empty */
        Strcpy(tmpbuf, prefix + 2);
        /* set prefix[] to "", "a ", or "an " */
        (void) just_an(prefix, *tmpbuf ? tmpbuf : bp);
        /* append remainder of original prefix */
        Strcat(prefix, tmpbuf);
    }

    /* show weight for items (debug tourist info);
       "aum" is stolen from Crawl's "Arbitrary Unit of Measure" */
    if (wizard && iflags.wizweight) {
        /* wizard mode user has asked to see object weights */
        if (with_price && bp_eos[-1] == ')')
            ConcatF1(bp, 1, ", %u aum)", obj->owt);
        else
            ConcatF1(bp, 0, " (%u aum)", obj->owt);

        /* ConcatF1(bp) updates bp_eos and bpspaceleft but we're done
           with them now; add a fake use so compiler won't complain
           about a variable assignment that won't be subsequently used */
        nhUse(bp_eos);
        nhUse(bpspaceleft);
    }

    bp = strprepend(bp, prefix);

    /*
     * Last gasp bounds check.
     *
     * If caller intends this to be for a menu entry, make sure that
     * there is some room to combine with menu selector prefix without
     * exceeding BUFSZ-1.
     *
     * offsetbp=4: width of menu entry selector text: "c - " for tty.
     * For curses, that wastes a char since it only needs 3: "c) ".
     *
     * Reaching full BUFSZ-1 length can't happen unless both doname
     * (BUFSZ-PREFIX) and strprepend (PREFIX) use up all available
     * space or one of them overflows without being detected.
     */
    if (strlen(bp) > BUFSZ - 1) {
        paniclog("doname", bp);
        /* ideally this will never happen; if xnamep is any obuf[]
           other than the last, overflow here would be relatively
           benign and we could probably keep going */
        panic("doname: long object description overflow.");
        /*NOTREACHED*/
    } else {
        static int doname_full = 0;
        int offsetbp = for_menu ? 4 : 0;

        if (strlen(bp) + offsetbp >= BUFSZ - 1) {
            /* for !offsetbp, we'll only get here if strlen(bp)==BUFSZ-1 */
            if (!doname_full++) {
                paniclog("doname", bp);
                Sprintf(tmpbuf, "long object description%s.",
                        offsetbp ? " truncated for menu use" : "");
                paniclog("doname", tmpbuf);
            }
            bp[BUFSZ - 1 - offsetbp] = '\0';
        }
    }

    return bp;
}

char *
doname(struct obj *obj)
{
    return doname_base(obj, (unsigned) 0);
}

char *
doename(struct obj *obj)
{
    return doename_base(obj, (unsigned) 0);
}

/* Name of object including price. */
char *
doname_with_price(struct obj *obj)
{
    return doname_base(obj, DONAME_WITH_PRICE);
}

char *
doename_with_price(struct obj *obj)
{
    return doename_base(obj, DONAME_WITH_PRICE);
}

/* "some" instead of precise quantity if obj->dknown not set */
char *
doname_vague_quan(struct obj *obj)
{
    /* Used by farlook.
     * If it hasn't been seen up close and quantity is more than one,
     * use "some" instead of the quantity: "some gold pieces" rather
     * than "25 gold pieces".  This is suboptimal, to put it mildly,
     * because lookhere and pickup report the precise amount.
     * Picking the item up while blind also shows the precise amount
     * for inventory display, then dropping it while still blind leaves
     * obj->dknown unset so the count reverts to "some" for farlook.
     *
     * TODO: add obj->qknown flag for 'quantity known' on stackable
     * items; it could overlay obj->cknown since no containers stack.
     */
    return doname_base(obj, DONAME_VAGUE_QUAN);
}

char *
doename_vague_quan(struct obj *obj)
{
    /* Used by farlook.
     * If it hasn't been seen up close and quantity is more than one,
     * use "some" instead of the quantity: "some gold pieces" rather
     * than "25 gold pieces".  This is suboptimal, to put it mildly,
     * because lookhere and pickup report the precise amount.
     * Picking the item up while blind also shows the precise amount
     * for inventory display, then dropping it while still blind leaves
     * obj->dknown unset so the count reverts to "some" for farlook.
     *
     * TODO: add obj->qknown flag for 'quantity known' on stackable
     * items; it could overlay obj->cknown since no containers stack.
     */
    return doename_base(obj, DONAME_VAGUE_QUAN);
}

/* used from invent.c */
boolean
not_fully_identified(struct obj *otmp)
{
    /* gold doesn't have any interesting attributes [yet?] */
    if (otmp->oclass == COIN_CLASS)
        return FALSE; /* always fully ID'd */
    /* check fundamental ID hallmarks first */
    if (!otmp->known || !otmp->dknown
#ifdef MAIL_STRUCTURES
        || (!otmp->bknown && otmp->otyp != SCR_MAIL)
#else
        || !otmp->bknown
#endif
        || !objects[otmp->otyp].oc_name_known)
        return TRUE;
    if ((!otmp->cknown && (Is_container(otmp) || otmp->otyp == STATUE))
        || (!otmp->lknown && Is_box(otmp)))
        return TRUE;
    if (otmp->oartifact && undiscovered_artifact(otmp->oartifact))
        return TRUE;
    /* otmp->rknown is the only item of interest if we reach here */
    /*
     *  Note:  if a revision ever allows scrolls to become fireproof or
     *  rings to become shockproof, this checking will need to be revised.
     *  `rknown' ID only matters if xname() will provide the info about it.
     */
    if (otmp->rknown
        || (otmp->oclass != ARMOR_CLASS && otmp->oclass != WEAPON_CLASS
            && !is_weptool(otmp)            /* (redundant) */
            && otmp->oclass != BALL_CLASS)) /* (useless) */
        return FALSE;
    else /* lack of `rknown' only matters for vulnerable objects */
        return (boolean) is_damageable(otmp);
}

/* xname doesn't include monster type for "corpse"; cxname does */
char *
cxname(struct obj *obj)
{
    if (obj->otyp == CORPSE)
        return corpse_xname(obj, (const char *) 0, CXN_NORMAL);
    return xname(obj);
}

char *
cxename(struct obj *obj)
{
    if (obj->otyp == CORPSE)
        return corpse_xename(obj, (const char *) 0, CXN_NORMAL);
    return xename(obj);
}

/* like cxname, but ignores quantity */
char *
cxname_singular(struct obj *obj)
{
    if (obj->otyp == CORPSE)
        return corpse_xname(obj, (const char *) 0, CXN_SINGULAR);
    return xname_flags(obj, CXN_SINGULAR);
}

char *
cxename_singular(struct obj *obj)
{
    if (obj->otyp == CORPSE)
        return corpse_xename(obj, (const char *) 0, CXN_SINGULAR);
    return xename_flags(obj, CXN_SINGULAR);
}

/* treat an object as fully ID'd when it might be used as reason for death */
char *
killer_xname(struct obj *obj)
{
    struct obj save_obj;
    unsigned save_ocknown;
    char *buf, *save_ocuname, *save_oname = (char *) 0;

    /* bypass object twiddling for artifacts */
    if (obj->oartifact)
        return bare_artifactname(obj);

    /* remember original settings for core of the object;
       oextra structs other than oname don't matter here--since they
       aren't modified they don't need to be saved and restored */
    save_obj = *obj;
    if (has_oname(obj))
        save_oname = ONAME(obj);

    /* killer name should be more specific than general xname; however, exact
       info like blessed/cursed and rustproof makes things be too verbose; set
       dknown (not observe_object) because dead characters don't observe */
    obj->known = obj->dknown = 1;
    obj->bknown = obj->rknown = obj->greased = 0;
    /* if character is a priest[ess], bknown will get toggled back on */
    if (obj->otyp != POT_WATER)
        obj->blessed = obj->cursed = 0;
    else
        obj->bknown = 1; /* describe holy/unholy water as such */
    /* "killed by poisoned <obj>" would be misleading when poison is
       not the cause of death and "poisoned by poisoned <obj>" would
       be redundant when it is, so suppress "poisoned" prefix */
    obj->opoisoned = 0;
    /* strip user-supplied name; artifacts keep theirs */
    if (!obj->oartifact && save_oname)
        ONAME(obj) = (char *) 0;
    /* temporarily identify the type of object */
    save_ocknown = objects[obj->otyp].oc_name_known;
    objects[obj->otyp].oc_name_known = 1;
    save_ocuname = objects[obj->otyp].oc_uname;
    objects[obj->otyp].oc_uname = 0; /* avoid "foo called bar" */

    /* format the object */
    if (obj->otyp == CORPSE) {
        buf = corpse_xname(obj, (const char *) 0, CXN_NORMAL);
    } else if (obj->otyp == SLIME_MOLD) {
        /* concession to "most unique deaths competition" in the annual
           devnull tournament, suppress player supplied fruit names because
           those can be used to fake other objects and dungeon features */
        buf = nextobuf();
        Sprintf(buf, "致命的粘液霉菌%s", plur(obj->quan));
    } else {
        buf = xname(obj);
    }
    /* apply an article if appropriate; caller should always use KILLED_BY */
    if (obj->quan == 1L && !strstri(buf, "'s ") && !strstri(buf, "s' "))
        buf = (obj_is_pname(obj) || the_unique_obj(obj)) ? the(buf) : an(buf);

    objects[obj->otyp].oc_name_known = save_ocknown;
    objects[obj->otyp].oc_uname = save_ocuname;
    *obj = save_obj; /* restore object's core settings */
    if (!obj->oartifact && save_oname)
        ONAME(obj) = save_oname;

    return buf;
}

/* xname,doname,&c with long results reformatted to omit some stuff */
char *
short_oname(
    struct obj *obj,
    char *(*func)(OBJ_P),    /* main formatting routine */
    char *(*altfunc)(OBJ_P), /* alternate for shortest result */
    unsigned lenlimit)
{
    struct obj save_obj;
    char unamebuf[12], onamebuf[12], *save_oname, *save_uname, *outbuf;

    outbuf = (*func)(obj);
    if ((unsigned) strlen(outbuf) <= lenlimit)
        return outbuf;

    /* shorten called string to fairly small amount */
    save_uname = objects[obj->otyp].oc_uname;
    if (save_uname && strlen(save_uname) >= sizeof unamebuf) {
        (void) strncpy(unamebuf, save_uname, sizeof unamebuf - 4);
        Strcpy(unamebuf + sizeof unamebuf - 4, "...");
        objects[obj->otyp].oc_uname = unamebuf;
        releaseobuf(outbuf);
        outbuf = (*func)(obj);
        objects[obj->otyp].oc_uname = save_uname; /* restore called string */
        if ((unsigned) strlen(outbuf) <= lenlimit)
            return outbuf;
    }

    /* shorten named string to fairly small amount */
    save_oname = has_oname(obj) ? ONAME(obj) : 0;
    if (save_oname && strlen(save_oname) >= sizeof onamebuf) {
        (void) strncpy(onamebuf, save_oname, sizeof onamebuf - 4);
        Strcpy(onamebuf + sizeof onamebuf - 4, "...");
        ONAME(obj) = onamebuf;
        releaseobuf(outbuf);
        outbuf = (*func)(obj);
        ONAME(obj) = save_oname; /* restore named string */
        if ((unsigned) strlen(outbuf) <= lenlimit)
            return outbuf;
    }

    /* shorten both called and named strings;
       unamebuf and onamebuf have both already been populated */
    if (save_uname && strlen(save_uname) >= sizeof unamebuf && save_oname
        && strlen(save_oname) >= sizeof onamebuf) {
        objects[obj->otyp].oc_uname = unamebuf;
        ONAME(obj) = onamebuf;
        releaseobuf(outbuf);
        outbuf = (*func)(obj);
        if ((unsigned) strlen(outbuf) <= lenlimit) {
            objects[obj->otyp].oc_uname = save_uname;
            ONAME(obj) = save_oname;
            return outbuf;
        }
    }

    /* still long; strip several name-lengthening attributes;
       called and named strings are still in truncated form */
    save_obj = *obj;
    obj->bknown = obj->rknown = obj->greased = 0;
    obj->oeroded = obj->oeroded2 = 0;
    releaseobuf(outbuf);
    outbuf = (*func)(obj);
    if (altfunc && (unsigned) strlen(outbuf) > lenlimit) {
        /* still long; use the alternate function (usually one of
           the jackets around minimal_xname()) */
        releaseobuf(outbuf);
        outbuf = (*altfunc)(obj);
    }
    /* restore the object */
    *obj = save_obj;
    if (save_oname)
        ONAME(obj) = save_oname;
    if (save_uname)
        objects[obj->otyp].oc_uname = save_uname;

    /* use whatever we've got, whether it's too long or not */
    return outbuf;
}

/*
 * Used if only one of a collection of objects is named (e.g. in eat.c).
 */
const char *
singular(struct obj *otmp, char *(*func)(OBJ_P))
{
    long savequan;
    char *nam;

    /* using xname for corpses does not give the monster type */
    if (otmp->otyp == CORPSE && func == xname)
        func = cxname;

    savequan = otmp->quan;
    otmp->quan = 1L;
    nam = (*func)(otmp);
    otmp->quan = savequan;
    return nam;
}

/* pick "", "a ", or "an " as article for 'str'; used by an() and doname() */
char *
just_an(char *outbuf, const char *str)
{
    *outbuf = '\0';
    Strcpy(outbuf, "一个");
    return outbuf;
}

char *
an(const char *str)
{
    char *buf = nextobuf();

    if (!str || !*str) {
        impossible("Alphabet soup: 'an(%s)'.", str ? "\"\"" : "<null>");
        return strcpy(buf, "an []");
    }
    (void) just_an(buf, str);
    return strncat(buf, str, BUFSZ - 1 - Strlen(buf));
}

char *
An(const char *str)
{
    char *tmp = an(str);

    *tmp = highc(*tmp);
    return tmp;
}

/*
 * Prepend "the" if necessary; assumes str is a subject derived from xname.
 * Use type_is_pname() for monster names, not the().  the() is idempotent.
 */
char *
the(const char *str)
{
    const char *aname;
    char *buf = nextobuf();
    boolean insert_the = FALSE;

    if (!str || !*str) {
        impossible("Alphabet soup: 'the(%s)'.", str ? "\"\"" : "<null>");
        return strcpy(buf, "the []");
    }
    if (!strncmpi(str, "the ", 4)) {
        buf[0] = lowc(*str);
        Strcpy(&buf[1], str + 1);
        return buf;
    } else if (*str < 'A' || *str > 'Z'
               /* some capitalized monster names want "the", others don't */
               || CapitalMon(str)
               /* treat named fruit as not a proper name, even if player
                  has assigned a capitalized proper name as his/her fruit,
                  unless it matches an artifact name */
               || (fruit_from_name(str, TRUE, (int *) 0)
                   && ((aname = artifact_name(str, (short *) 0, FALSE)) == 0
                       || strncmpi(aname, "the ", 4) == 0))) {
        /* not a proper name, needs an article */
        insert_the = TRUE;
    } else {
        /* Probably a proper name, might not need an article */
        char *named, *called;
        const char *tmp;
        int l;

        /* some objects have capitalized adjectives in their names */
        if (((tmp = strrchr(str, ' ')) != 0 || (tmp = strrchr(str, '-')) != 0)
            && (tmp[1] < 'A' || tmp[1] > 'Z')) {
            /* insert "the" unless we have an apostrophe (where we assume
               we're dealing with "Unique's corpse" when "Unique" wasn't
               caught by CapitalMon() above) */
            insert_the = !strchr(str, '\'');
        } else if (tmp && strchr(str, ' ') < tmp) { /* has spaces */
            /* it needs an article if the name contains "of" */
            tmp = strstri(str, " of ");
            named = strstri(str, " named ");
            called = strstri(str, " called ");
            if (called && (!named || called < named))
                named = called;

            if (tmp && (!named || tmp < named)) /* found an "of" */
                insert_the = TRUE;
            /* stupid special case: lacks "of" but needs "the" */
            else if (!named && (l = Strlen(str)) >= 31
                     && !strcmp(&str[l - 31],
                                "Platinum Yendorian Express Card"))
                insert_the = TRUE;
        }
    }
    if (insert_the)
        Strcpy(buf, ""); /*危险:你自己看着办吧*/
    else
        buf[0] = '\0';
    return strncat(buf, str, BUFSZ - 1 - Strlen(buf));
}

char *
The(const char *str)
{
    char *tmp = the(str);

    *tmp = highc(*tmp);
    return tmp;
}

/* returns "count cxname(otmp)" or just cxname(otmp) if count == 1 */
char *
aobjnam(struct obj *otmp, const char *verb)
{
    char prefix[PREFIX];
    char *bp = cxname(otmp);

    if (otmp->quan != 1L) {
        Sprintf(prefix, "%ld个", otmp->quan);
        bp = strprepend(bp, prefix);
    }
    if (verb) {
        Strcat(bp, "");
        Strcat(bp, otense(otmp, verb));
    }
    return bp;
}

/* combine yname and aobjnam eg "your count cxname(otmp)" */
char *
yobjnam(struct obj *obj, const char *verb)
{
    char *s = aobjnam(obj, verb);

    /* leave off "your" for most of your artifacts, but prepend
     * "your" for unique objects and "foo of bar" quest artifacts */
    if (!carried(obj) || !obj_is_pname(obj)
        || obj->oartifact >= ART_ORB_OF_DETECTION) {
        char *outbuf = shk_your(nextobuf(), obj);
        int space_left = BUFSZ - 1 - Strlen(outbuf);

        s = strncat(outbuf, s, space_left);
    }
    return s;
}

/* combine Yname2 and aobjnam eg "Your count cxname(otmp)" */
char *
Yobjnam2(struct obj *obj, const char *verb)
{
    char *s = yobjnam(obj, verb);

    *s = highc(*s);
    return s;
}

/* like aobjnam, but prepend "The", not count, and use xname */
char *
Tobjnam(struct obj *otmp, const char *verb)
{
    char *bp = The(xname(otmp));

    if (verb) {
        Strcat(bp, "");
        Strcat(bp, otense(otmp, verb));
    }
    return bp;
}

/* capitalized variant of doname() */
char *
Doname2(struct obj *obj)
{
    char *s = doname(obj);

    *s = highc(*s);
    return s;
}

/* doname() for itemized buying of 'obj' from a shop */
char *
paydoname(struct obj *obj)
{
    static const char and_contents[] = "及其内容物";
    char *p;
    unsigned save_cknown = obj->cknown;
    boolean save_wizweight = iflags.wizweight;

    if (Has_contents(obj))
        obj->cknown = 0;
    /* avoid showing item weights to unclutter billing's pay-menu a bit */
    iflags.wizweight = FALSE;
    /* suppress invent-style price; caller will add billing-style price */
    iflags.suppress_price++;
    p = doname_base(obj, 0U);
    iflags.suppress_price--;
    iflags.wizweight = save_wizweight;

    if (Has_contents(obj)) {
        /* buy_container() sets no_charge for a container that has just
           been purchased so that when paydoname() is called by
           shk_names_obj(), we'll provide "a/an <container>" instead of
           "your <container>" */
        if (!obj->no_charge) {
            if (!cnstrcmp(p, "一个"))
                p += strlen("一个");
            /*冗余:else if (!strncmp(p, "an ", 3))
                p += 3;*/
            p = strprepend(p, obj->unpaid ? "一个未付款的" : "你的");
        }

        if (!obj->cknown) {
            if (obj->unpaid) {
                if ((int) strlen(p) + sizeof and_contents - 1
                    < BUFSZ - PREFIX)
                    Strcat(p, and_contents);
            } else {
                p = strcat(p, "的内容物");
            }
        }
    }
    obj->cknown = save_cknown;
    return p;
}

/* returns "[your ]xname(obj)" or "Foobar's xname(obj)" or "the xname(obj)" */
char *
yname(struct obj *obj)
{
    char *s = cxname(obj);

    /* leave off "your" for most of your artifacts, but prepend
     * "your" for unique objects and "foo of bar" quest artifacts */
    if (!carried(obj) || !obj_is_pname(obj)
        || obj->oartifact >= ART_ORB_OF_DETECTION) {
        char *outbuf = shk_your(nextobuf(), obj);
        int space_left = BUFSZ - 1 - Strlen(outbuf);

        s = strncat(outbuf, s, space_left);
    }

    return s;
}

/* capitalized variant of yname() */
char *
Yname2(struct obj *obj)
{
    char *s = yname(obj);

    *s = highc(*s);
    return s;
}

/* returns "your minimal_xname(obj)"
 * or "Foobar's minimal_xname(obj)"
 * or "the minimal_xname(obj)"
 */
char *
ysimple_name(struct obj *obj)
{
    char *outbuf = nextobuf();
    char *s = shk_your(outbuf, obj); /* assert( s == outbuf ); */
    int space_left = BUFSZ - 1 - Strlen(s);

    return strncat(s, minimal_xname(obj), space_left);
}

/* capitalized variant of ysimple_name() */
char *
Ysimple_name2(struct obj *obj)
{
    char *s = ysimple_name(obj);

    *s = highc(*s);
    return s;
}

    /*
     * FIXME:
     *  simpleonames(), ansimpleoname(), and thesimpleoname() need to
     *  know the beginning of the obuf[] they use so that they can
     *  guard against buffer overflow when pluralizing (is that an
     *  actual word?) or inserting "an" or "the".
     *
     *  minimal_xname() returns a call to xname() which writes into
     *  the middle of its obuf[] then backs up to accomodate a prefix,
     *  so BUFSZ is not a reliable limit for the length of the result.
     *
     *  [Overflow likely moot.  Since the formatted object name has
     *  user-supplied name suppressed, the length is sure to be short
     *  enough to added plural suffix or "an" or "the" prefix.]
     */

/* "scroll" or "scrolls" */
char *
simpleonames(struct obj *obj)
{
    char *obufp, *simpleoname = minimal_xname(obj);

    if (obj->quan != 1L) {
        /* 'simpleoname' points to an obuf; makeplural() will allocate
           another one and only that one can be explicitly released for
           re-use, so this is slightly convoluted to cope with that;
           makeplural() will be fully evaluated and done with its input
           argument before strcpy() touches its output argument */
        Strcpy(simpleoname, obufp = makeplural(simpleoname));
        releaseobuf(obufp);
    }
    return simpleoname;
}

char *
simpleoenames(struct obj *obj)
{
    char *obufp, *simpleoname = minimal_xename(obj);

    if (obj->quan != 1L) {
        /* 'simpleoname' points to an obuf; makeplural() will allocate
           another one and only that one can be explicitly released for
           re-use, so this is slightly convoluted to cope with that;
           makeplural() will be fully evaluated and done with its input
           argument before strcpy() touches its output argument */
        Strcpy(simpleoname, obufp = makeplural(simpleoname));
        releaseobuf(obufp);
    }
    return simpleoname;
}

/* "a scroll" or "scrolls"; "a silver bell" or "the Bell of Opening" */
char *
ansimpleoname(struct obj *obj)
{
    char *obufp, *simpleoname = simpleonames(obj);
    int otyp = obj->otyp;

    /* prefix with "the" if a unique item, or a fake one imitating same,
       has been formatted with its actual name (we let minimal_xname() handle
       any `known' and `dknown' checking necessary) */
    if (otyp == FAKE_AMULET_OF_YENDOR)
        otyp = AMULET_OF_YENDOR;
    if (objects[otyp].oc_unique && OBJ_NAME(objects[otyp])
        && !strcmp(simpleoname, OBJ_NAME(objects[otyp]))) {
        /* the() will allocate another obuf[]; we want to avoid using two */
        obufp = the(simpleoname);
        Strcpy(simpleoname, obufp);
        releaseobuf(obufp);
    } else if (obj->quan == 1L) {
        /* simpleoname[] is singular if quan==1, plural otherwise;
           an() will allocate another obuf[]; we want to avoid using two */
        obufp = an(simpleoname);
        Strcpy(simpleoname, obufp);
        releaseobuf(obufp);
    }
    return simpleoname;
}

char *
ansimpleoename(struct obj *obj)
{
    char *obufp, *simpleoname = simpleoenames(obj);
    int otyp = obj->otyp;

    /* prefix with "the" if a unique item, or a fake one imitating same,
       has been formatted with its actual name (we let minimal_xname() handle
       any `known' and `dknown' checking necessary) */
    if (otyp == FAKE_AMULET_OF_YENDOR)
        otyp = AMULET_OF_YENDOR;
    if (objects[otyp].oc_unique && OBJ_ENAME(objects[otyp])
        && !strcmp(simpleoname, OBJ_ENAME(objects[otyp]))) {
        /* the() will allocate another obuf[]; we want to avoid using two */
        obufp = the(simpleoname);
        Strcpy(simpleoname, obufp);
        releaseobuf(obufp);
    } else if (obj->quan == 1L) {
        /* simpleoname[] is singular if quan==1, plural otherwise;
           an() will allocate another obuf[]; we want to avoid using two */
        obufp = an(simpleoname);
        Strcpy(simpleoname, obufp);
        releaseobuf(obufp);
    }
    return simpleoname;
}

/* "the scroll" or "the scrolls" */
char *
thesimpleoname(struct obj *obj)
{
    char *obufp, *simpleoname = simpleonames(obj);

    /* the() will allocate another obuf[]; we want to avoid using two */
    obufp = the(simpleoname);
    Strcpy(simpleoname, obufp);
    releaseobuf(obufp);
    return simpleoname;
}

char *
thesimpleoename(struct obj *obj)
{
    char *obufp, *simpleoname = simpleoenames(obj);

    /* the() will allocate another obuf[]; we want to avoid using two */
    obufp = the(simpleoname);
    Strcpy(simpleoname, obufp);
    releaseobuf(obufp);
    return simpleoname;
}

/* basic name of obj, as if it has been discovered; for some types of
   items, we can't just use OBJ_NAME() because it doesn't always include
   the class (for instance "light" when we want "spellbook of light");
   minimal_xname() uses xname() to get that */
char *
actualoname(struct obj *obj)
{
    char *res;

    iflags.override_ID = TRUE;
    res = minimal_xname(obj);
    iflags.override_ID = FALSE;
    return res;
}

char *
actualoename(struct obj *obj)
{
    char *res;

    iflags.override_ID = TRUE;
    res = minimal_xename(obj);
    iflags.override_ID = FALSE;
    return res;
}

/* artifact's name without any object type or known/dknown/&c feedback */
char *
bare_artifactname(struct obj *obj)
{
    char *outbuf;

    if (obj->oartifact) {
        outbuf = nextobuf();
        Strcpy(outbuf, artiname(obj->oartifact));
        if (!strncmp(outbuf, "The ", 4))
            outbuf[0] = lowc(outbuf[0]);
    } else {
        outbuf = xname(obj);
    }
    return outbuf;
}

char *
bare_artifactename(struct obj *obj)
{
    char *outbuf;

    if (obj->oartifact) {
        outbuf = nextobuf();
        Strcpy(outbuf, artiename(obj->oartifact));
        if (!strncmp(outbuf, "The ", 4))
            outbuf[0] = lowc(outbuf[0]);
    } else {
        outbuf = xename(obj);
    }
    return outbuf;
}

static const char *const wrp[] = {
    "wand",   "ring",      "potion",     "scroll", "gem",
    "amulet", "spellbook", "spell book",
    /* for non-specific wishes */
    "weapon", "armor",     "tool",       "food",   "comestible",
};
static const char wrpsym[] = { WAND_CLASS,   RING_CLASS,   POTION_CLASS,
                               SCROLL_CLASS, GEM_CLASS,    AMULET_CLASS,
                               SPBOOK_CLASS, SPBOOK_CLASS, WEAPON_CLASS,
                               ARMOR_CLASS,  TOOL_CLASS,   FOOD_CLASS,
                               FOOD_CLASS };

/* return form of the verb (input plural) if xname(otmp) were the subject */
char *
otense(struct obj *otmp, const char *verb)
{
    char *buf;

    /*
     * verb is given in plural (without trailing s).  Return as input
     * if the result of xname(otmp) would be plural.  Don't bother
     * recomputing xname(otmp) at this time.
     */
    if (!is_plural(otmp))
        return vtense((char *) 0, verb);

    buf = nextobuf();
    Strcpy(buf, verb);
    return buf;
}

/* various singular words that vtense would otherwise categorize as plural;
   also used by makesingular() to catch some special cases */
static const char *const special_subjs[] = {
    "erinys",  "manes", /* this one is ambiguous */
    "Cyclops", "Hippocrates",     "Pelias",    "aklys",
    "amnesia", "detect monsters", "paralysis", "shape changers",
    "nemesis", 0
    /* note: "detect monsters" and "shape changers" are normally
       caught via "<something>(s) of <whatever>", but they can be
       wished for using the shorter form, so we include them here
       to accommodate usage by makesingular during wishing */
};

/* return form of the verb (input plural) for present tense 3rd person subj */
char *
vtense(const char *subj, const char *verb)
{
    char *buf = nextobuf(), *bspot;
    int len, ltmp;
    const char *sp, *spot;
    const char *const *spec;

    /*
     * verb is given in plural (without trailing s).  Return as input
     * if subj appears to be plural.  Add special cases as necessary.
     * Many hard cases can already be handled by using otense() instead.
     * If this gets much bigger, consider decomposing makeplural.
     * Note: monster names are not expected here (except before corpse).
     *
     * Special case: allow null sobj to get the singular 3rd person
     * present tense form so we don't duplicate this code elsewhere.
     */
    if (subj) {
        if (!strncmpi(subj, "a ", 2) || !strncmpi(subj, "an ", 3))
            goto sing;
        spot = (const char *) 0;
        for (sp = subj; (sp = strchr(sp, ' ')) != 0; ++sp) {
            if (!strncmpi(sp, " of ", 4) || !strncmpi(sp, " from ", 6)
                || !strncmpi(sp, " called ", 8) || !strncmpi(sp, " named ", 7)
                || !strncmpi(sp, " labeled ", 9)) {
                if (sp != subj)
                    spot = sp - 1;
                break;
            }
        }
        len = (int) strlen(subj);
        if (!spot)
            spot = subj + len - 1;

        /*
         * plural: anything that ends in 's', but not '*us' or '*ss'.
         * Guess at a few other special cases that makeplural creates.
         */
        if ((lowc(*spot) == 's' && spot != subj
             && !strchr("us", lowc(*(spot - 1))))
            || !BSTRNCMPI(subj, spot - 3, "eeth", 4)
            || !BSTRNCMPI(subj, spot - 3, "feet", 4)
            || !BSTRNCMPI(subj, spot - 1, "ia", 2)
            || !BSTRNCMPI(subj, spot - 1, "ae", 2)) {
            /* check for special cases to avoid false matches */
            len = (int) (spot - subj) + 1;
            for (spec = special_subjs; *spec; spec++) {
                ltmp = Strlen(*spec);
                if (len == ltmp && !strncmpi(*spec, subj, len))
                    goto sing;
                /* also check for <prefix><space><special_subj>
                   to catch things like "the invisible erinys" */
                if (len > ltmp && *(spot - ltmp) == ' '
                    && !strncmpi(*spec, spot - ltmp + 1, ltmp))
                    goto sing;
            }

            return strcpy(buf, verb);
        }
        /*
         * 3rd person plural doesn't end in telltale 's';
         * 2nd person singular behaves as if plural.
         */
        if (!strcmpi(subj, "they") || !strcmpi(subj, "you"))
            return strcpy(buf, verb);
    }

 sing:
    Strcpy(buf, verb);
    len = (int) strlen(buf);
    bspot = buf + len - 1;

    if (!strcmpi(buf, "are")) {
        Strcasecpy(buf, "is");
    } else if (!strcmpi(buf, "have")) {
        Strcasecpy(bspot - 1, "s");
    } else if (strchr("zxs", lowc(*bspot))
               || (len >= 2 && lowc(*bspot) == 'h'
                   && strchr("cs", lowc(*(bspot - 1))))
               || (len == 2 && lowc(*bspot) == 'o')) {
        /* Ends in z, x, s, ch, sh; add an "es" */
        Strcasecpy(bspot + 1, "es");
    } else if (lowc(*bspot) == 'y' && !strchr(vowels, lowc(*(bspot - 1)))) {
        /* like "y" case in makeplural */
        Strcasecpy(bspot, "ies");
    } else {
        Strcasecpy(bspot + 1, "");
    }

    return buf;
}

struct sing_plur {
    const char *sing, *plur;
};

/* word pairs that don't fit into formula-based transformations;
   also some suffices which have very few--often one--matches or
   which aren't systematically reversible (knives, staves) */
static const struct sing_plur one_off[] = {
    { "child",
      "children" },      /* (for wise guys who give their food funny names) */
    { "cubus", "cubi" }, /* in-/suc-cubus */
    { "culus", "culi" }, /* homunculus */
    { "Cyclops", "Cyclopes" },
    { "djinni", "djinn" },
    { "erinys", "erinyes" },
    { "foot", "feet" },
    { "fungus", "fungi" },
    { "goose", "geese" },
    { "knife", "knives" },
    { "labrum", "labra" }, /* candelabrum */
    { "louse", "lice" },
    { "mouse", "mice" },
    { "mumak", "mumakil" },
    { "nemesis", "nemeses" },
    { "ovum", "ova" },
    { "ox", "oxen" },
    { "passerby", "passersby" },
    { "rtex", "rtices" }, /* vortex */
    { "serum", "sera" },
    { "staff", "staves" },
    { "tooth", "teeth" },
    { 0, 0 }
};

static const char *const as_is[] = {
    /* makesingular() leaves these plural due to how they're used */
    "boots",   "shoes",     "gloves",    "lenses",   "scales",
    "eyes",    "gauntlets", "iron bars",
    /* both singular and plural are spelled the same */
    "bison",   "deer",      "elk",       "fish",      "fowl",
    "tuna",    "yaki",      "-hai",      "krill",     "manes",
    "moose",   "ninja",     "sheep",     "ronin",     "roshi",
    "shito",   "tengu",     "ki-rin",    "Nazgul",    "gunyoki",
    "piranha", "samurai",   "shuriken",  "haggis",    "Bordeaux",
    0,
    /* Note:  "fish" and "piranha" are collective plurals, suitable
       for "wiped out all <foo>".  For "3 <foo>", they should be
       "fishes" and "piranhas" instead.  We settle for collective
       variant instead of attempting to support both. */
};

/* singularize/pluralize decisions common to both makesingular & makeplural */
staticfn boolean
singplur_lookup(
    char *basestr, char *endstring,  /* base string, pointer to eos(string) */
    boolean to_plural,         /* true => makeplural, false => makesingular */
    const char *const *alt_as_is)    /* another set like as_is[] */
{
    const struct sing_plur *sp;
    const char *same, *other, *const *as;
    int al;
    int baselen = Strlen(basestr);

    for (as = as_is; *as; ++as) {
        al = (int) strlen(*as);
        if (!BSTRCMPI(basestr, endstring - al, *as))
            return TRUE;
    }
    if (alt_as_is) {
        for (as = alt_as_is; *as; ++as) {
            al = (int) strlen(*as);
            if (!BSTRCMPI(basestr, endstring - al, *as))
                return TRUE;
        }
    }

   /* Leave "craft" as a suffix as-is (aircraft, hovercraft);
      "craft" itself is (arguably) not included in our likely context */
   if ((baselen > 5) && (!BSTRCMPI(basestr, endstring - 5, "craft")))
       return TRUE;
   /* avoid false hit on one_off[].plur == "lice" or .sing == "goose";
       if more of these turn up, one_off[] entries will need to flagged
       as to which are whole words and which are matchable as suffices
       then matching in the loop below will end up becoming more complex */
    if (!strcmpi(basestr, "slice")
        || !strcmpi(basestr, "mongoose")) {
        if (to_plural)
            Strcasecpy(endstring, "s");
        return TRUE;
    }
    /* skip "ox" -> "oxen" entry when pluralizing "<something>ox"
       unless it is muskox */
    if (to_plural && baselen > 2 && !strcmpi(endstring - 2, "ox")
        && !(baselen > 5 && !strcmpi(endstring - 6, "muskox"))) {
        /* "fox" -> "foxes" */
        Strcasecpy(endstring, "es");
        return TRUE;
    }
    if (to_plural) {
        if (baselen > 2 && !strcmpi(endstring - 3, "man")
            && badman(basestr, to_plural)) {
            Strcasecpy(endstring, "s");
            return TRUE;
        }
    } else {
        if (baselen > 2 && !strcmpi(endstring - 3, "men")
            && badman(basestr, to_plural))
            return TRUE;
    }
    for (sp = one_off; sp->sing; sp++) {
        /* check whether endstring already matches */
        same = to_plural ? sp->plur : sp->sing;
        al = (int) strlen(same);
        if (!BSTRCMPI(basestr, endstring - al, same))
            return TRUE; /* use as-is */
        /* check whether it matches the inverse; if so, transform it */
        other = to_plural ? sp->sing : sp->plur;
        al = (int) strlen(other);
        if (!BSTRCMPI(basestr, endstring - al, other)) {
            Strcasecpy(endstring - al, same);
            return TRUE; /* one_off[] transformation */
        }
    }
    return FALSE;
}

/* searches for common compounds, ex. lump of royal jelly */
staticfn char *
singplur_compound(char *str)
{
    /* if new entries are added, be sure to keep compound_start[] in sync */
    static const char *const compounds[] =
        {
          " of ",     " labeled ", " called ",
          " named ",  " above", /* lurkers above */
          " versus ", " from ",    " in ",
          " on ",     " a la ",    " with", /* " with "? */
          " de ",     " d'",       " du ",
          " au ",     "-in-",      "-at-",
          0
        }, /* list of first characters for all compounds[] entries */
        compound_start[] = " -";

    const char *const *cmpd;
    char *p;

    for (p = str; *p; ++p) {
        /* substring starting at p can only match if *p is found
           within compound_start[] */
        if (!strchr(compound_start, *p))
            continue;

        /* check current substring against all words in the compound[] list */
        for (cmpd = compounds; *cmpd; ++cmpd)
            if (!strncmpi(p, *cmpd, (int) strlen(*cmpd)))
                return p;
    }
    /* wasn't recognized as a compound phrase */
    return 0;
}

/* Plural routine; once upon a time it may have been chiefly used for
 * user-defined fruits, but it is now used extensively throughout the
 * program.
 *
 * For fruit, we have to try to account for everything reasonable the
 * player has; something unreasonable can still break the code.
 * However, it's still a lot more accurate than "just add an 's' at the
 * end", which Rogue uses...
 *
 * Also used for plural monster names ("Wiped out all homunculi." or the
 * vanquished monsters list) and body parts.  A lot of unique monsters have
 * names which get mangled by makeplural and/or makesingular.  They're not
 * genocidable, and vanquished-mon handling does its own special casing
 * (for uniques who've been revived and re-killed), so we don't bother
 * trying to get those right here.
 *
 * Also misused by muse.c to convert 1st person present verbs to 2nd person.
 * 3.6.0: made case-insensitive.
 */
char *
makeplural(const char *oldstr)
{
    char *spot;
    char lo_c, *str = nextobuf();
    const char *excess = (char *) 0;
    int len, i;

    if (oldstr)
        while (*oldstr == ' ')
            oldstr++;
    if (!oldstr || !*oldstr) {
        impossible("plural of null?");
        Strcpy(str, "s");
        return str;
    }
    /* makeplural() is sometimes used on monsters rather than objects
       and sometimes pronouns are used for monsters, so check those;
       unfortunately, "her" (which matches genders[1].him and [1].his)
       and "it" (which matches genders[2].he and [2].him) are ambiguous;
       we'll live with that; caller can fix things up if necessary */
    *str = '\0';
    for (i = 0; i <= 2; ++i) {
        if (!strcmpi(genders[i].he, oldstr))
            Strcpy(str, genders[3].he); /* "they" */
        else if (!strcmpi(genders[i].him, oldstr))
            Strcpy(str, genders[3].him); /* "them" */
        else if (!strcmpi(genders[i].his, oldstr))
            Strcpy(str, genders[3].his); /* "their" */
        if (*str) {
            if (oldstr[0] == highc(oldstr[0]))
                str[0] = highc(str[0]);
            return str;
        }
    }

    Strcpy(str, oldstr);

    /*
     * Skip changing "pair of" to "pairs of".  According to Webster, usual
     * English usage is use pairs for humans, e.g. 3 pairs of dancers,
     * and pair for objects and non-humans, e.g. 3 pair of boots.  We don't
     * refer to pairs of humans in this game so just skip to the bottom.
     */
    if (!strncmpi(str, "pair of ", 8))
        goto bottom;

    /* look for "foo of bar" so that we can focus on "foo" */
    if ((spot = singplur_compound(str)) != 0) {
        excess = oldstr + (int) (spot - str);
        *spot = '\0';
    } else
        spot = eos(str);

    spot--;
    while (spot > str && *spot == ' ')
        spot--; /* Strip blanks from end */
    *(spot + 1) = '\0';
    /* Now spot is the last character of the string */

    len = Strlen(str);

    /* Single letters */
    if (len == 1 || !letter(*spot)) {
        Strcpy(spot + 1, "");
        goto bottom;
    }

    /* dispense with some words which don't need pluralization */
    {
        static const char *const already_plural[] = {
            "ae",  /* algae, larvae, &c */
            "eaux", /* chateaux, gateaux */
            "matzot", 0,
        };

        /* spot+1: synch up with makesingular's usage */
        if (singplur_lookup(str, spot + 1, TRUE, already_plural))
            goto bottom;

        /* more of same, but not suitable for blanket loop checking */
        if ((len == 2 && !strcmpi(str, "ya"))
            || (len >= 3 && !strcmpi(spot - 2, " ya")))
            goto bottom;
    }

    /* man/men ("Wiped out all cavemen.") */
    if (len >= 3 && !strcmpi(spot - 2, "man")
        /* exclude shamans and humans etc */
        && !badman(str, TRUE)) {
        Strcasecpy(spot - 1, "en");
        goto bottom;
    }
    if (lowc(*spot) == 'f') { /* (staff handled via one_off[]) */
        lo_c = lowc(*(spot - 1));
        if (len >= 3 && !strcmpi(spot - 2, "erf")) {
            /* avoid "nerf" -> "nerves", "serf" -> "serves" */
            ; /* fall through to default (append 's') */
        } else if (strchr("lr", lo_c) || strchr(vowels, lo_c)) {
            /* [aeioulr]f to [aeioulr]ves */
            Strcasecpy(spot, "ves");
            goto bottom;
        }
    }
    /* ium/ia (mycelia, baluchitheria) */
    if (len >= 3 && !strcmpi(spot - 2, "ium")) {
        Strcasecpy(spot - 2, "ia");
        goto bottom;
    }
    /* algae, larvae, hyphae (another fungus part) */
    if ((len >= 4 && !strcmpi(spot - 3, "alga"))
        || (len >= 5
            && (!strcmpi(spot - 4, "hypha") || !strcmpi(spot - 4, "larva")))
        || (len >= 6 && !strcmpi(spot - 5, "amoeba"))
        || (len >= 8 && (!strcmpi(spot - 7, "vertebra")))) {
        /* a to ae */
        Strcasecpy(spot + 1, "e");
        goto bottom;
    }
    /* fungus/fungi, homunculus/homunculi, but buses, lotuses, wumpuses */
    if (len > 3 && !strcmpi(spot - 1, "us")
        && !((len >= 5 && !strcmpi(spot - 4, "lotus"))
             || (len >= 6 && !strcmpi(spot - 5, "wumpus")))) {
        Strcasecpy(spot - 1, "i");
        goto bottom;
    }
    /* sis/ses (nemesis) */
    if (len >= 3 && !strcmpi(spot - 2, "sis")) {
        Strcasecpy(spot - 1, "es");
        goto bottom;
    }
    /* -eau/-eaux (gateau, chapeau...) */
    if (len >= 3 && !strcmpi(spot - 2, "eau")
        /* 'bureaus' is the more common plural of 'bureau' */
        && BSTRCMPI(str, spot - 5, "bureau")) {
        Strcasecpy(spot + 1, "x");
        goto bottom;
    }
    /* matzoh/matzot, possible food name */
    if (len >= 6
        && (!strcmpi(spot - 5, "matzoh") || !strcmpi(spot - 5, "matzah"))) {
        Strcasecpy(spot - 1, "ot"); /* oh/ah -> ot */
        goto bottom;
    }
    if (len >= 5
        && (!strcmpi(spot - 4, "matzo") || !strcmpi(spot - 4, "matza"))) {
        Strcasecpy(spot, "ot"); /* o/a -> ot */
        goto bottom;
    }

    /* note: ox/oxen, VAX/VAXen, goose/geese */

    lo_c = lowc(*spot);

    /* codex/spadix/neocortex and the like */
    if (len >= 5
        && (!strcmpi(spot - 2, "dex")
            ||!strcmpi(spot - 2, "dix")
            ||!strcmpi(spot - 2, "tex"))
           /* indices would have been ok too, but stick with indexes */
        && (strcmpi(spot - 4,"index") != 0)) {
        Strcasecpy(spot - 1, "ices"); /* ex|ix -> ices */
        goto bottom;
    }
    /* Ends in z, x, s, ch, sh; add an "es" */
    if (strchr("zxs", lo_c)
        || (len >= 2 && lo_c == 'h' && strchr("cs", lowc(*(spot - 1)))
            /* 21st century k-sound */
            && !(len >= 4 && lowc(*(spot - 1)) == 'c' && ch_ksound(str)))
        /* Kludge to get "tomatoes" and "potatoes" right */
        || (len >= 4 && !strcmpi(spot - 2, "ato"))
        || (len >= 5 && !strcmpi(spot - 4, "dingo"))) {
        Strcasecpy(spot + 1, "es"); /* append es */
        goto bottom;
    }
    /* Ends in y preceded by consonant (note: also "qu") change to "ies" */
    if (lo_c == 'y' && !strchr(vowels, lowc(*(spot - 1)))) {
        Strcasecpy(spot, "ies"); /* y -> ies */
        goto bottom;
    }
    /* Default: append an 's' */
    Strcasecpy(spot + 1, "");

 bottom:
    if (excess)
        Strcat(str, excess);
    return str;
}

/*
 * Singularize a string the user typed in; this helps reduce the complexity
 * of readobjnam, and is also used in pager.c to singularize the string
 * for which help is sought.
 *
 * "Manes" is ambiguous: monster type (keep s), or horse body part (drop s)?
 * Its inclusion in as_is[]/special_subj[] makes it get treated as the former.
 *
 * A lot of unique monsters have names ending in s; plural, or singular
 * from plural, doesn't make much sense for them so we don't bother trying.
 * 3.6.0: made case-insensitive.
 */
char *
makesingular(const char *oldstr)
{
    char *p, *bp;
    const char *excess = 0;
    char *str = nextobuf();

    if (oldstr)
        while (*oldstr == ' ')
            oldstr++;
    if (!oldstr || !*oldstr) {
        impossible("singular of null?");
        str[0] = '\0';
        return str;
    }
    /* makeplural() of pronouns isn't reversible but at least we can
       force a singular value */
    *str = '\0';
    if (!strcmpi(genders[3].he, oldstr)) /* "they" */
        Strcpy(str, genders[2].he); /* "it" */
    else if (!strcmpi(genders[3].him, oldstr)) /* "them" */
        Strcpy(str, genders[2].him); /* also "it" */
    else if (!strcmpi(genders[3].his, oldstr)) /* "their" */
        Strcpy(str, genders[2].his); /* "its" */
    if (*str) {
        if (oldstr[0] == highc(oldstr[0]))
            str[0] = highc(str[0]);
        return str;
    }

    bp = strcpy(str, oldstr);

    /* check for "foo of bar" so that we can focus on "foo" */
    if ((p = singplur_compound(bp)) != 0) {
        excess = oldstr + (int) (p - bp);
        *p = '\0';
    } else
        p = eos(bp);

    /* dispense with some words which don't need singularization */
    if (singplur_lookup(bp, p, FALSE, special_subjs))
        goto bottom;

    /* remove -s or -es (boxes) or -ies (rubies) */
    if (p >= bp + 1 && lowc(p[-1]) == 's') {
        if (p >= bp + 2 && lowc(p[-2]) == 'e') {
            if (p >= bp + 3 && lowc(p[-3]) == 'i') { /* "ies" */
                if (!BSTRCMPI(bp, p - 7, "cookies")
                    || (!BSTRCMPI(bp, p - 4, "pies")
                        /* avoid false match for "harpies" */
                        && (p - 4 == bp || p[-5] == ' '))
                    /* alternate djinni/djinn spelling; not really needed */
                    || (!BSTRCMPI(bp, p - 6, "genies")
                        /* avoid false match for "progenies" */
                        && (p - 6 == bp || p[-7] == ' '))
                    || !BSTRCMPI(bp, p - 5, "mbies") /* zombie */
                    || !BSTRCMPI(bp, p - 5, "yries")) /* valkyrie */
                    goto mins;
                Strcasecpy(p - 3, "y"); /* ies -> y */
                goto bottom;
            }
            /* wolves, but f to ves isn't fully reversible */
            if (p - 4 >= bp && (strchr("lr", lowc(*(p - 4)))
                                || strchr(vowels, lowc(*(p - 4))))
                && !BSTRCMPI(bp, p - 3, "ves")) {
                if (!BSTRCMPI(bp, p - 6, "cloves")
                    || !BSTRCMPI(bp, p - 6, "nerves"))
                    goto mins;
                Strcasecpy(p - 3, "f"); /* ves -> f */
                goto bottom;
            }
            /* note: nurses, axes but boxes, wumpuses */
            if (!BSTRCMPI(bp, p - 4, "eses")
                || !BSTRCMPI(bp, p - 4, "oxes") /* boxes, foxes */
                || !BSTRCMPI(bp, p - 4, "nxes") /* lynxes */
                || !BSTRCMPI(bp, p - 4, "ches")
                || !BSTRCMPI(bp, p - 4, "uses") /* lotuses */
                || !BSTRCMPI(bp, p - 4, "shes") /* splashes [of venom] */
                || !BSTRCMPI(bp, p - 4, "sses") /* priestesses */
                || !BSTRCMPI(bp, p - 5, "atoes") /* tomatoes */
                || !BSTRCMPI(bp, p - 7, "dingoes")
                || !BSTRCMPI(bp, p - 7, "Aleaxes")) {
                *(p - 2) = '\0'; /* drop es */
                goto bottom;
            } /* else fall through to mins */

            /* ends in 's' but not 'es' */
        } else if (!BSTRCMPI(bp, p - 2, "us")) { /* lotus, fungus... */
            if (BSTRCMPI(bp, p - 6, "tengus") /* but not these... */
                && BSTRCMPI(bp, p - 7, "hezrous"))
                goto bottom;
        } else if (!BSTRCMPI(bp, p - 2, "ss")
                   || !BSTRCMPI(bp, p - 5, " lens")
                   || (p - 4 == bp && !strcmpi(p - 4, "lens"))) {
            goto bottom;
        }
 mins:
        *(p - 1) = '\0'; /* drop s */

    } else { /* input doesn't end in 's' */

        if (!BSTRCMPI(bp, p - 3, "men")
            && !badman(bp, FALSE)) {
            Strcasecpy(p - 2, "an");
            goto bottom;
        }
        /* matzot -> matzo, algae -> alga */
        if (!BSTRCMPI(bp, p - 6, "matzot") || !BSTRCMPI(bp, p - 2, "ae")
            || !BSTRCMPI(bp, p - 4, "eaux")) {
            *(p - 1) = '\0'; /* drop t/e/x */
            goto bottom;
        }
        /* balactheria -> balactherium */
        if (p - 4 >= bp && !strcmpi(p - 2, "ia")
            && strchr("lr", lowc(*(p - 3))) && lowc(*(p - 4)) == 'e') {
            Strcasecpy(p - 1, "um"); /* a -> um */
        }

        /* here we cannot find the plural suffix */
    }

 bottom:
    /* if we stripped off a suffix (" of bar" from "foo of bar"),
       put it back now [strcat() isn't actually 100% safe here...] */
    if (excess)
        Strcat(bp, excess);

    return bp;
}


staticfn boolean
ch_ksound(const char *basestr)
{
    /* these are some *ch words/suffixes that make a k-sound. They pluralize by
       adding 's' rather than 'es' */
    static const char *const ch_k[] = {
        "monarch",     "poch",    "tech",     "mech",      "stomach", "psych",
        "amphibrach",  "anarch",  "atriarch", "azedarach", "broch",
        "gastrotrich", "isopach", "loch",     "oligarch",  "peritrich",
        "sandarach",   "sumach",  "symposiarch",
    };
    int i, al;
    const char *endstr;

    if (!basestr || strlen(basestr) < 4)
        return FALSE;

    endstr = eos((char *) basestr);
    for (i = 0; i < SIZE(ch_k); i++) {
        al = (int) strlen(ch_k[i]);
        if (!BSTRCMPI(basestr, endstr - al, ch_k[i]))
            return TRUE;
    }
    return FALSE;
}

staticfn boolean
badman(
    const char *basestr,
    boolean to_plural)  /* True: makeplural, False: makesingular */
{
    /* these are all the prefixes for *man that don't have a *men plural */
    static const char *const no_men[] = {
        "albu", "antihu", "anti", "ata", "auto", "bildungsro", "cai", "cay",
        "ceru", "corner", "decu", "des", "dura", "fir", "hanu", "het",
        "infrahu", "inhu", "nonhu", "otto", "out", "prehu", "protohu",
        "subhu", "superhu", "talis", "unhu", "sha",
        "hu", "un", "le", "re", "so", "to", "at", "a",
    };
    /* these are all the prefixes for *men that don't have a *man singular */
    static const char *const no_man[] = {
        "abdo", "acu", "agno", "ceru", "cogno", "cycla", "fleh", "grava",
        "hegu", "preno", "sonar", "speci", "dai", "exa", "fla", "sta", "teg",
        "tegu", "vela", "da", "hy", "lu", "no", "nu", "ra", "ru", "se", "vi",
        "ya", "o", "a",
    };
    int i, al;
    const char *endstr, *spot;

    if (!basestr || strlen(basestr) < 4)
        return FALSE;

    endstr = eos((char *) basestr);

    if (to_plural) {
        for (i = 0; i < SIZE(no_men); i++) {
            al = (int) strlen(no_men[i]);
            spot = endstr - (al + 3);
            if (!BSTRNCMPI(basestr, spot, no_men[i], al)
                && (spot == basestr || *(spot - 1) == ' '))
                return TRUE;
        }
    } else {
        for (i = 0; i < SIZE(no_man); i++) {
            al = (int) strlen(no_man[i]);
            spot = endstr - (al + 3);
            if (!BSTRNCMPI(basestr, spot, no_man[i], al)
                && (spot == basestr || *(spot - 1) == ' '))
                return TRUE;
        }
    }
    return FALSE;
}

/* compare user string against object name string using fuzzy matching */
staticfn boolean
wishymatch(
    const char *u_str,      /* from user, so might be variant spelling */
    const char *o_str,      /* from objects[], so is in canonical form */
    boolean retry_inverted) /* optional extra "of" handling */
{
    static NEARDATA const char detect_SP[] = "探测",
                               SP_detection[] = "探测";
    char *p, buf[BUFSZ];

    /* ignore spaces & hyphens and upper/lower case when comparing */
    if (fuzzymatch(u_str, o_str, " -", TRUE))
        return TRUE;

    if (retry_inverted) {
        const char *u_of, *o_of;

        /* when just one of the strings is in the form "foo of bar",
           convert it into "bar foo" and perform another comparison */
        u_of = strstri(u_str, "的");
        o_of = strstri(o_str, "的");
        if (u_of && !o_of) {
            Strcpy(buf, u_of + strlen("的")); /*危险:" of" ，4*/
            copynchars(eos(strcat(buf, " ")), u_str, (int) (u_of - u_str));
            if (fuzzymatch(buf, o_str, " -", TRUE))
                return TRUE;
        } else if (o_of && !u_of) {
            Strcpy(buf, o_of + strlen("的"));
            copynchars(eos(strcat(buf, " ")), o_str, (int) (o_of - o_str));
            if (fuzzymatch(u_str, buf, " -", TRUE))
                return TRUE;
        }
    }

    /* [note: if something like "elven speed boots" ever gets added, these
       special cases should be changed to call wishymatch() recursively in
       order to get the "of" inversion handling] */
    if (!strncmp(o_str, "dwarvish ", 9)) {
        if (!strncmpi(u_str, "dwarven ", 8))
            return fuzzymatch(u_str + 8, o_str + 9, " -", TRUE);
    } else if (!strncmp(o_str, "elven ", 6)) {
        if (!strncmpi(u_str, "elvish ", 7))
            return fuzzymatch(u_str + 7, o_str + 6, " -", TRUE);
        else if (!strncmpi(u_str, "elfin ", 6))
            return fuzzymatch(u_str + 6, o_str + 6, " -", TRUE);
    } else if (strstri(o_str, "helm") && strstri(u_str, "helmet")) {
        copynchars(buf, u_str, (int) sizeof buf - 1);
        (void) strsubst(buf, "helmet", "helm");
        return wishymatch(buf, o_str,  TRUE);
    } else if (strstri(o_str, "gauntlets") && strstri(u_str, "gloves")) {
        /* -3: room to replace shorter "gloves" with longer "gauntlets" */
        copynchars(buf, u_str, (int) sizeof buf - 1 - 3);
        (void) strsubst(buf, "gloves", "gauntlets");
        return wishymatch(buf, o_str, TRUE);
    } else if (!strncmp(o_str, detect_SP, sizeof detect_SP - 1)) {
        /* check for "detect <foo>" vs "<foo> detection" */
        if ((p = strstri(u_str, SP_detection)) != 0
            && !*(p + sizeof SP_detection - 1)) {
            /* convert "<foo> detection" into "detect <foo>" */
            *p = '\0';
            Strcat(strcpy(buf, detect_SP), u_str);
            /* "detect monster" -> "detect monsters" */
            if (!strcmpi(u_str, "monster"))
                Strcat(buf, "s");
            *p = ' ';
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (strstri(o_str, SP_detection)) {
        /* and the inverse, "<foo> detection" vs "detect <foo>" */
        if (!strncmpi(u_str, detect_SP, sizeof detect_SP - 1)) {
            /* convert "detect <foo>s" into "<foo> detection" */
            p = makesingular(u_str + sizeof detect_SP - 1);
            Strcat(strcpy(buf, p), SP_detection);
            /* caller may be looping through objects[], so avoid
               churning through all the obufs */
            releaseobuf(p);
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (strstri(o_str, "ability")) {
        /* when presented with "foo of bar", makesingular() used to
           singularize both foo & bar, but now only does so for foo */
        /* catch "{potion(s),ring} of {gain,restore,sustain} abilities" */
        if ((p = strstri(u_str, "abilities")) != 0
            && !*(p + sizeof "abilities" - 1)) {
            (void) strncpy(buf, u_str, (unsigned) (p - u_str));
            Strcpy(buf + (p - u_str), "能力");
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (!strcmp(o_str, "aluminum")) {
        /* this special case doesn't really fit anywhere else... */
        /* (note that " wand" will have been stripped off by now) */
        if (!strcmpi(u_str, "aluminium"))
            return fuzzymatch(u_str + 9, o_str + 8, " -", TRUE);
    }

    return FALSE;
}


/* compare user string against object name string using fuzzy matching */
staticfn boolean
wishyematch(
    const char *u_str,      /* from user, so might be variant spelling */
    const char *o_str,      /* from objects[], so is in canonical form */
    boolean retry_inverted) /* optional extra "of" handling */
{
    static NEARDATA const char detect_SP[] = "detect ",
                               SP_detection[] = " detection";
    char *p, buf[BUFSZ];

    /* ignore spaces & hyphens and upper/lower case when comparing */
    if (fuzzymatch(u_str, o_str, " -", TRUE))
        return TRUE;

    if (retry_inverted) {
        const char *u_of, *o_of;

        /* when just one of the strings is in the form "foo of bar",
           convert it into "bar foo" and perform another comparison */
        u_of = strstri(u_str, " of ");
        o_of = strstri(o_str, " of ");
        if (u_of && !o_of) {
            Strcpy(buf, u_of + 4);
            copynchars(eos(strcat(buf, " ")), u_str, (int) (u_of - u_str));
            if (fuzzymatch(buf, o_str, " -", TRUE))
                return TRUE;
        } else if (o_of && !u_of) {
            Strcpy(buf, o_of + 4);
            copynchars(eos(strcat(buf, " ")), o_str, (int) (o_of - o_str));
            if (fuzzymatch(u_str, buf, " -", TRUE))
                return TRUE;
        }
    }

    /* [note: if something like "elven speed boots" ever gets added, these
       special cases should be changed to call wishymatch() recursively in
       order to get the "of" inversion handling] */
    if (!strncmp(o_str, "dwarvish ", 9)) {
        if (!strncmpi(u_str, "dwarven ", 8))
            return fuzzymatch(u_str + 8, o_str + 9, " -", TRUE);
    } else if (!strncmp(o_str, "elven ", 6)) {
        if (!strncmpi(u_str, "elvish ", 7))
            return fuzzymatch(u_str + 7, o_str + 6, " -", TRUE);
        else if (!strncmpi(u_str, "elfin ", 6))
            return fuzzymatch(u_str + 6, o_str + 6, " -", TRUE);
    } else if (strstri(o_str, "helm") && strstri(u_str, "helmet")) {
        copynchars(buf, u_str, (int) sizeof buf - 1);
        (void) strsubst(buf, "helmet", "helm");
        return wishymatch(buf, o_str,  TRUE);
    } else if (strstri(o_str, "gauntlets") && strstri(u_str, "gloves")) {
        /* -3: room to replace shorter "gloves" with longer "gauntlets" */
        copynchars(buf, u_str, (int) sizeof buf - 1 - 3);
        (void) strsubst(buf, "gloves", "gauntlets");
        return wishymatch(buf, o_str, TRUE);
    } else if (!strncmp(o_str, detect_SP, sizeof detect_SP - 1)) {
        /* check for "detect <foo>" vs "<foo> detection" */
        if ((p = strstri(u_str, SP_detection)) != 0
            && !*(p + sizeof SP_detection - 1)) {
            /* convert "<foo> detection" into "detect <foo>" */
            *p = '\0';
            Strcat(strcpy(buf, detect_SP), u_str);
            /* "detect monster" -> "detect monsters" */
            if (!strcmpi(u_str, "monster"))
                Strcat(buf, "s");
            *p = ' ';
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (strstri(o_str, SP_detection)) {
        /* and the inverse, "<foo> detection" vs "detect <foo>" */
        if (!strncmpi(u_str, detect_SP, sizeof detect_SP - 1)) {
            /* convert "detect <foo>s" into "<foo> detection" */
            p = makesingular(u_str + sizeof detect_SP - 1);
            Strcat(strcpy(buf, p), SP_detection);
            /* caller may be looping through objects[], so avoid
               churning through all the obufs */
            releaseobuf(p);
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (strstri(o_str, "ability")) {
        /* when presented with "foo of bar", makesingular() used to
           singularize both foo & bar, but now only does so for foo */
        /* catch "{potion(s),ring} of {gain,restore,sustain} abilities" */
        if ((p = strstri(u_str, "abilities")) != 0
            && !*(p + sizeof "abilities" - 1)) {
            (void) strncpy(buf, u_str, (unsigned) (p - u_str));
            Strcpy(buf + (p - u_str), "ability");
            return fuzzymatch(buf, o_str, " -", TRUE);
        }
    } else if (!strcmp(o_str, "aluminum")) {
        /* this special case doesn't really fit anywhere else... */
        /* (note that " wand" will have been stripped off by now) */
        if (!strcmpi(u_str, "aluminium"))
            return fuzzymatch(u_str + 9, o_str + 8, " -", TRUE);
    }

    return FALSE;
}

struct o_range {
    const char *name, oclass;
    int f_o_range, l_o_range;
};

/* wishable subranges of objects */
static NEARDATA const struct o_range o_ranges[] = {
    { "bag", TOOL_CLASS, SACK, BAG_OF_TRICKS },
    { "包", TOOL_CLASS, SACK, BAG_OF_TRICKS },
    { "lamp", TOOL_CLASS, OIL_LAMP, MAGIC_LAMP },
    { "灯", TOOL_CLASS, OIL_LAMP, MAGIC_LAMP },
    { "candle", TOOL_CLASS, TALLOW_CANDLE, WAX_CANDLE },
    { "蜡烛", TOOL_CLASS, TALLOW_CANDLE, WAX_CANDLE },
    { "horn", TOOL_CLASS, TOOLED_HORN, HORN_OF_PLENTY },
    { "号", TOOL_CLASS, TOOLED_HORN, HORN_OF_PLENTY },
    { "号角", TOOL_CLASS, TOOLED_HORN, HORN_OF_PLENTY },
    { "shield", ARMOR_CLASS, SMALL_SHIELD, SHIELD_OF_REFLECTION },
    { "盾", ARMOR_CLASS, SMALL_SHIELD, SHIELD_OF_REFLECTION },
    { "盾牌", ARMOR_CLASS, SMALL_SHIELD, SHIELD_OF_REFLECTION },
    { "hat", ARMOR_CLASS, FEDORA, DUNCE_CAP },
    { "帽", ARMOR_CLASS, FEDORA, DUNCE_CAP },
    { "帽子", ARMOR_CLASS, FEDORA, DUNCE_CAP },
    { "helm", ARMOR_CLASS, ELVEN_LEATHER_HELM, HELM_OF_TELEPATHY },
    { "盔", ARMOR_CLASS, ELVEN_LEATHER_HELM, HELM_OF_TELEPATHY },
    { "头盔", ARMOR_CLASS, ELVEN_LEATHER_HELM, HELM_OF_TELEPATHY },
    { "gloves", ARMOR_CLASS, LEATHER_GLOVES, GAUNTLETS_OF_DEXTERITY },
    { "手套", ARMOR_CLASS, LEATHER_GLOVES, GAUNTLETS_OF_DEXTERITY },
    { "gauntlets", ARMOR_CLASS, LEATHER_GLOVES, GAUNTLETS_OF_DEXTERITY },
    { "拳套", ARMOR_CLASS, LEATHER_GLOVES, GAUNTLETS_OF_DEXTERITY },
    { "boots", ARMOR_CLASS, LOW_BOOTS, LEVITATION_BOOTS },
    { "靴", ARMOR_CLASS, LOW_BOOTS, LEVITATION_BOOTS },
    { "靴子", ARMOR_CLASS, LOW_BOOTS, LEVITATION_BOOTS },
    { "shoes", ARMOR_CLASS, LOW_BOOTS, IRON_SHOES },
    { "鞋", ARMOR_CLASS, LOW_BOOTS, IRON_SHOES },
    { "鞋子", ARMOR_CLASS, LOW_BOOTS, IRON_SHOES },
    { "cloak", ARMOR_CLASS, MUMMY_WRAPPING, CLOAK_OF_DISPLACEMENT },
    { "斗篷", ARMOR_CLASS, MUMMY_WRAPPING, CLOAK_OF_DISPLACEMENT },
    { "披风", ARMOR_CLASS, MUMMY_WRAPPING, CLOAK_OF_DISPLACEMENT },
    { "shirt", ARMOR_CLASS, HAWAIIAN_SHIRT, T_SHIRT },
    { "衬衫", ARMOR_CLASS, HAWAIIAN_SHIRT, T_SHIRT },
    { "dragon scales", ARMOR_CLASS, GRAY_DRAGON_SCALES, YELLOW_DRAGON_SCALES },
    { "龙鳞", ARMOR_CLASS, GRAY_DRAGON_SCALES, YELLOW_DRAGON_SCALES },
    { "dragon scale mail", ARMOR_CLASS, GRAY_DRAGON_SCALE_MAIL, YELLOW_DRAGON_SCALE_MAIL },
    { "龙甲", ARMOR_CLASS, GRAY_DRAGON_SCALE_MAIL, YELLOW_DRAGON_SCALE_MAIL },
    { "龙鳞甲", ARMOR_CLASS, GRAY_DRAGON_SCALE_MAIL, YELLOW_DRAGON_SCALE_MAIL },
    { "sword", WEAPON_CLASS, SHORT_SWORD, KATANA },
    { "剑", WEAPON_CLASS, SHORT_SWORD, KATANA },
    { "venom", VENOM_CLASS, BLINDING_VENOM, ACID_VENOM },
    { "毒液", VENOM_CLASS, BLINDING_VENOM, ACID_VENOM },
    { "gray stone", GEM_CLASS, LUCKSTONE, FLINT },
    { "grey stone", GEM_CLASS, LUCKSTONE, FLINT },
    { "灰石", GEM_CLASS, LUCKSTONE, FLINT },
    { "灰石头", GEM_CLASS, LUCKSTONE, FLINT },
    { "灰色石头", GEM_CLASS, LUCKSTONE, FLINT },
    { "卷轴", SCROLL_CLASS },
    { "药水", POTION_CLASS },
    { "魔杖", WAND_CLASS },
    { "戒指", RING_CLASS },
    { "护身符", AMULET_CLASS },
    { "护符", AMULET_CLASS },
    { "魔法书", SPBOOK_CLASS },
};

/* alternate spellings; if the difference is only the presence or
   absence of spaces and/or hyphens (such as "pickaxe" vs "pick axe"
   vs "pick-axe") then there is no need for inclusion in this list;
   likewise for ``"of" inversions'' ("boots of speed" vs "speed boots") */
static const struct alt_spellings {
    const char *sp;
    int ob;
} spellings[] = {
    { "pickax", PICK_AXE },
    { "whip", BULLWHIP },
    { "saber", SILVER_SABER },
    { "silver sabre", SILVER_SABER },
    { "smooth shield", SHIELD_OF_REFLECTION },
    { "grey dragon scale mail", GRAY_DRAGON_SCALE_MAIL },
    { "grey dragon scales", GRAY_DRAGON_SCALES },
    { "iron ball", HEAVY_IRON_BALL },
    { "lantern", BRASS_LANTERN },
    { "mattock", DWARVISH_MATTOCK },
    { "amulet of poison resistance", AMULET_VERSUS_POISON },
    { "amulet of protection", AMULET_OF_GUARDING },
    { "amulet of telepathy", AMULET_OF_ESP },
    { "helm of esp", HELM_OF_TELEPATHY },
    { "gauntlets of ogre power", GAUNTLETS_OF_POWER },
    { "gauntlets of giant strength", GAUNTLETS_OF_POWER },
    { "elven chain mail", ELVEN_MITHRIL_COAT },
    { "silver shield", SHIELD_OF_REFLECTION },
    { "potion of sleep", POT_SLEEPING },
    { "scroll of recharging", SCR_CHARGING },
    { "recharging", SCR_CHARGING },
    { "stone", ROCK },
    { "camera", EXPENSIVE_CAMERA },
    { "tee shirt", T_SHIRT },
    { "can", TIN },
    { "can opener", TIN_OPENER },
    { "kelp", KELP_FROND },
    { "eucalyptus", EUCALYPTUS_LEAF },
    { "lembas", LEMBAS_WAFER },
    { "tripe", TRIPE_RATION },
    { "cookie", FORTUNE_COOKIE },
    { "pie", CREAM_PIE },
    { "huge meatball", ENORMOUS_MEATBALL }, /* likely conflated name */
    { "huge chunk of meat", ENORMOUS_MEATBALL }, /* original name */
    { "marker", MAGIC_MARKER },
    { "hook", GRAPPLING_HOOK },
    { "grappling iron", GRAPPLING_HOOK },
    { "grapnel", GRAPPLING_HOOK },
    { "grapple", GRAPPLING_HOOK },
    { "protection from shape shifters", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "accuracy", RIN_INCREASE_ACCURACY },
    /* if we ever add other sizes, move this to o_ranges[] with "bag" */
    { "box", LARGE_BOX },
    /* normally we wouldn't have to worry about unnecessary <space>, but
       " stone" will get stripped off, preventing a wishymatch; that actually
       lets "flint stone" be a match, so we also accept bogus "flintstone" */
    { "luck stone", LUCKSTONE },
    { "load stone", LOADSTONE },
    { "touch stone", TOUCHSTONE },
    { "flintstone", FLINT },
    /*修改语序，危险，冗余，待写:我知道这样很搞笑，但是既然能跑为什么不呢？*/
    /*所有可能的戒指*/
    { "装饰戒指", RIN_ADORNMENT },
    { "装饰品戒指", RIN_ADORNMENT },
    { "装饰用戒指", RIN_ADORNMENT },
    { "装饰用的戒指", RIN_ADORNMENT },
    { "装饰品之戒指", RIN_ADORNMENT },
    { "装饰品之戒", RIN_ADORNMENT },
    { "力量戒指", RIN_GAIN_STRENGTH },
    { "力量的戒指", RIN_GAIN_STRENGTH },
    { "力量之戒指", RIN_GAIN_STRENGTH },
    { "力量之戒", RIN_GAIN_STRENGTH },
    { "增加力量戒指", RIN_GAIN_STRENGTH },
    { "增加力量的戒指", RIN_GAIN_STRENGTH },
    { "增加力量之戒指", RIN_GAIN_STRENGTH },
    { "增加力量之戒", RIN_GAIN_STRENGTH },
    { "提升力量戒指", RIN_GAIN_STRENGTH },
    { "提升力量的戒指", RIN_GAIN_STRENGTH },
    { "提升力量之戒指", RIN_GAIN_STRENGTH },
    { "提升力量之戒", RIN_GAIN_STRENGTH },
    { "提高力量戒指", RIN_GAIN_STRENGTH },
    { "提高力量的戒指", RIN_GAIN_STRENGTH },
    { "提高力量之戒指", RIN_GAIN_STRENGTH },
    { "提高力量之戒", RIN_GAIN_STRENGTH },
    { "体质戒指", RIN_GAIN_CONSTITUTION },
    { "体质的戒指", RIN_GAIN_CONSTITUTION },
    { "体质之戒指", RIN_GAIN_CONSTITUTION },
    { "体质之戒", RIN_GAIN_CONSTITUTION },
    { "增加体质戒指", RIN_GAIN_CONSTITUTION },
    { "增加体质的戒指", RIN_GAIN_CONSTITUTION },
    { "增加体质之戒指", RIN_GAIN_CONSTITUTION },
    { "增加体质之戒", RIN_GAIN_CONSTITUTION },
    { "提升体质戒指", RIN_GAIN_CONSTITUTION },
    { "提升体质的戒指", RIN_GAIN_CONSTITUTION },
    { "提升体质之戒指", RIN_GAIN_CONSTITUTION },
    { "提升体质之戒", RIN_GAIN_CONSTITUTION },
    { "提高体质戒指", RIN_GAIN_CONSTITUTION },
    { "提高体质的戒指", RIN_GAIN_CONSTITUTION },
    { "提高体质之戒指", RIN_GAIN_CONSTITUTION },
    { "提高体质之戒", RIN_GAIN_CONSTITUTION },
    { "精确戒指", RIN_INCREASE_ACCURACY },
    { "精确的戒指", RIN_INCREASE_ACCURACY },
    { "精确之戒指", RIN_INCREASE_ACCURACY },
    { "精确之戒", RIN_INCREASE_ACCURACY },
    { "增加精确戒指", RIN_INCREASE_ACCURACY },
    { "增加精确的戒指", RIN_INCREASE_ACCURACY },
    { "增加精确之戒指", RIN_INCREASE_ACCURACY },
    { "增加精确之戒", RIN_INCREASE_ACCURACY },
    { "提升精确戒指", RIN_INCREASE_ACCURACY },
    { "提升精确的戒指", RIN_INCREASE_ACCURACY },
    { "提升精确之戒指", RIN_INCREASE_ACCURACY },
    { "提升精确之戒", RIN_INCREASE_ACCURACY },
    { "提高精确戒指", RIN_INCREASE_ACCURACY },
    { "提高精确的戒指", RIN_INCREASE_ACCURACY },
    { "提高精确之戒指", RIN_INCREASE_ACCURACY },
    { "提高精确之戒", RIN_INCREASE_ACCURACY },
    { "增加精度戒指", RIN_INCREASE_ACCURACY },
    { "增加精度的戒指", RIN_INCREASE_ACCURACY },
    { "增加精度之戒指", RIN_INCREASE_ACCURACY },
    { "增加精度之戒", RIN_INCREASE_ACCURACY },
    { "提升精度戒指", RIN_INCREASE_ACCURACY },
    { "提升精度的戒指", RIN_INCREASE_ACCURACY },
    { "提升精度之戒指", RIN_INCREASE_ACCURACY },
    { "提升精度之戒", RIN_INCREASE_ACCURACY },
    { "提高精度戒指", RIN_INCREASE_ACCURACY },
    { "提高精度的戒指", RIN_INCREASE_ACCURACY },
    { "提高精度之戒指", RIN_INCREASE_ACCURACY },
    { "提高精度之戒", RIN_INCREASE_ACCURACY },
    { "伤害戒指", RIN_INCREASE_DAMAGE },
    { "伤害的戒指", RIN_INCREASE_DAMAGE },
    { "伤害之戒指", RIN_INCREASE_DAMAGE },
    { "伤害之戒", RIN_INCREASE_DAMAGE },
    { "增加伤害戒指", RIN_INCREASE_DAMAGE },
    { "增加伤害的戒指", RIN_INCREASE_DAMAGE },
    { "增加伤害之戒指", RIN_INCREASE_DAMAGE },
    { "增加伤害之戒", RIN_INCREASE_DAMAGE },
    { "提升伤害戒指", RIN_INCREASE_DAMAGE },
    { "提升伤害的戒指", RIN_INCREASE_DAMAGE },
    { "提升伤害之戒指", RIN_INCREASE_DAMAGE },
    { "提升伤害之戒", RIN_INCREASE_DAMAGE },
    { "提高伤害戒指", RIN_INCREASE_DAMAGE },
    { "提高伤害的戒指", RIN_INCREASE_DAMAGE },
    { "提高伤害之戒指", RIN_INCREASE_DAMAGE },
    { "提高伤害之戒", RIN_INCREASE_DAMAGE },
    { "保护戒指", RIN_PROTECTION },
    { "保护的戒指", RIN_PROTECTION },
    { "保护之戒指", RIN_PROTECTION },
    { "保护之戒", RIN_PROTECTION },
    { "增加保护戒指", RIN_PROTECTION },
    { "增加保护的戒指", RIN_PROTECTION },
    { "增加保护之戒指", RIN_PROTECTION },
    { "增加保护之戒", RIN_PROTECTION },
    { "提升保护戒指", RIN_PROTECTION },
    { "提升保护的戒指", RIN_PROTECTION },
    { "提升保护之戒指", RIN_PROTECTION },
    { "提升保护之戒", RIN_PROTECTION },
    { "提高保护戒指", RIN_PROTECTION },
    { "提高保护的戒指", RIN_PROTECTION },
    { "提高保护之戒指", RIN_PROTECTION },
    { "提高保护之戒", RIN_PROTECTION },
    { "力量增加戒指", RIN_GAIN_STRENGTH },
    { "力量增加的戒指", RIN_GAIN_STRENGTH },
    { "力量增加之戒指", RIN_GAIN_STRENGTH },
    { "力量增加之戒", RIN_GAIN_STRENGTH },
    { "力量提升戒指", RIN_GAIN_STRENGTH },
    { "力量提升的戒指", RIN_GAIN_STRENGTH },
    { "力量提升之戒指", RIN_GAIN_STRENGTH },
    { "力量提升之戒", RIN_GAIN_STRENGTH },
    { "力量提高戒指", RIN_GAIN_STRENGTH },
    { "力量提高的戒指", RIN_GAIN_STRENGTH },
    { "力量提高之戒指", RIN_GAIN_STRENGTH },
    { "力量提高之戒", RIN_GAIN_STRENGTH },
    { "体质增加戒指", RIN_GAIN_CONSTITUTION },
    { "体质增加的戒指", RIN_GAIN_CONSTITUTION },
    { "体质增加之戒指", RIN_GAIN_CONSTITUTION },
    { "体质增加之戒", RIN_GAIN_CONSTITUTION },
    { "体质提升戒指", RIN_GAIN_CONSTITUTION },
    { "体质提升的戒指", RIN_GAIN_CONSTITUTION },
    { "体质提升之戒指", RIN_GAIN_CONSTITUTION },
    { "体质提升之戒", RIN_GAIN_CONSTITUTION },
    { "体质提高戒指", RIN_GAIN_CONSTITUTION },
    { "体质提高的戒指", RIN_GAIN_CONSTITUTION },
    { "体质提高之戒指", RIN_GAIN_CONSTITUTION },
    { "体质提高之戒", RIN_GAIN_CONSTITUTION },
    { "精确增加戒指", RIN_INCREASE_ACCURACY },
    { "精确增加的戒指", RIN_INCREASE_ACCURACY },
    { "精确增加之戒指", RIN_INCREASE_ACCURACY },
    { "精确增加之戒", RIN_INCREASE_ACCURACY },
    { "精确提升戒指", RIN_INCREASE_ACCURACY },
    { "精确提升的戒指", RIN_INCREASE_ACCURACY },
    { "精确提升之戒指", RIN_INCREASE_ACCURACY },
    { "精确提升之戒", RIN_INCREASE_ACCURACY },
    { "精确提高戒指", RIN_INCREASE_ACCURACY },
    { "精确提高的戒指", RIN_INCREASE_ACCURACY },
    { "精确提高之戒指", RIN_INCREASE_ACCURACY },
    { "精确提高之戒", RIN_INCREASE_ACCURACY },
    { "精度增加戒指", RIN_INCREASE_ACCURACY },
    { "精度增加的戒指", RIN_INCREASE_ACCURACY },
    { "精度增加之戒指", RIN_INCREASE_ACCURACY },
    { "精度增加之戒", RIN_INCREASE_ACCURACY },
    { "精度提升戒指", RIN_INCREASE_ACCURACY },
    { "精度提升的戒指", RIN_INCREASE_ACCURACY },
    { "精度提升之戒指", RIN_INCREASE_ACCURACY },
    { "精度提升之戒", RIN_INCREASE_ACCURACY },
    { "精度提高戒指", RIN_INCREASE_ACCURACY },
    { "精度提高的戒指", RIN_INCREASE_ACCURACY },
    { "精度提高之戒指", RIN_INCREASE_ACCURACY },
    { "精度提高之戒", RIN_INCREASE_ACCURACY },
    { "伤害增加戒指", RIN_INCREASE_DAMAGE },
    { "伤害增加的戒指", RIN_INCREASE_DAMAGE },
    { "伤害增加之戒指", RIN_INCREASE_DAMAGE },
    { "伤害增加之戒", RIN_INCREASE_DAMAGE },
    { "伤害提升戒指", RIN_INCREASE_DAMAGE },
    { "伤害提升的戒指", RIN_INCREASE_DAMAGE },
    { "伤害提升之戒指", RIN_INCREASE_DAMAGE },
    { "伤害提升之戒", RIN_INCREASE_DAMAGE },
    { "伤害提高戒指", RIN_INCREASE_DAMAGE },
    { "伤害提高的戒指", RIN_INCREASE_DAMAGE },
    { "伤害提高之戒指", RIN_INCREASE_DAMAGE },
    { "伤害提高之戒", RIN_INCREASE_DAMAGE },
    { "保护增加戒指", RIN_PROTECTION },
    { "保护增加的戒指", RIN_PROTECTION },
    { "保护增加之戒指", RIN_PROTECTION },
    { "保护增加之戒", RIN_PROTECTION },
    { "保护提升戒指", RIN_PROTECTION },
    { "保护提升的戒指", RIN_PROTECTION },
    { "保护提升之戒指", RIN_PROTECTION },
    { "保护提升之戒", RIN_PROTECTION },
    { "保护提高戒指", RIN_PROTECTION },
    { "保护提高的戒指", RIN_PROTECTION },
    { "保护提高之戒指", RIN_PROTECTION },
    { "保护提高之戒", RIN_PROTECTION },
    { "再生戒指", RIN_REGENERATION },
    { "再生的戒指", RIN_REGENERATION },
    { "再生之戒指", RIN_REGENERATION },
    { "再生之戒", RIN_REGENERATION },
    { "搜索戒指", RIN_SEARCHING },
    { "搜索的戒指", RIN_SEARCHING },
    { "搜索之戒指", RIN_SEARCHING },
    { "搜索之戒", RIN_SEARCHING },
    { "搜寻戒指", RIN_SEARCHING },
    { "搜寻的戒指", RIN_SEARCHING },
    { "搜寻之戒指", RIN_SEARCHING },
    { "搜寻之戒", RIN_SEARCHING },
    { "潜行戒指", RIN_STEALTH },
    { "潜行之戒指", RIN_STEALTH },
    { "潜行之戒", RIN_STEALTH },
    { "维持能力戒指", RIN_SUSTAIN_ABILITY },
    { "维持能力的戒指", RIN_SUSTAIN_ABILITY },
    { "维持能力之戒指", RIN_SUSTAIN_ABILITY },
    { "维持能力之戒", RIN_SUSTAIN_ABILITY },
    { "保持能力戒指", RIN_SUSTAIN_ABILITY },
    { "保持能力的戒指", RIN_SUSTAIN_ABILITY },
    { "保持能力之戒指", RIN_SUSTAIN_ABILITY },
    { "保持能力之戒", RIN_SUSTAIN_ABILITY },
    { "能力维持戒指", RIN_SUSTAIN_ABILITY },
    { "能力维持的戒指", RIN_SUSTAIN_ABILITY },
    { "能力维持之戒指", RIN_SUSTAIN_ABILITY },
    { "能力维持之戒", RIN_SUSTAIN_ABILITY },
    { "能力保持戒指", RIN_SUSTAIN_ABILITY },
    { "能力保持的戒指", RIN_SUSTAIN_ABILITY },
    { "能力保持之戒指", RIN_SUSTAIN_ABILITY },
    { "能力保持之戒", RIN_SUSTAIN_ABILITY },
    { "飘浮戒指", RIN_LEVITATION },
    { "飘浮的戒指", RIN_LEVITATION },
    { "飘浮之戒指", RIN_LEVITATION },
    { "飘浮之戒", RIN_LEVITATION },
    { "悬浮戒指", RIN_LEVITATION },
    { "悬浮的戒指", RIN_LEVITATION },
    { "悬浮之戒指", RIN_LEVITATION },
    { "悬浮之戒", RIN_LEVITATION },
    { "饥饿戒指", RIN_HUNGER },
    { "饥饿的戒指", RIN_HUNGER },
    { "饥饿之戒指", RIN_HUNGER },
    { "饥饿之戒", RIN_HUNGER },
    { "激怒怪物戒指", RIN_AGGRAVATE_MONSTER },
    { "激怒怪物的戒指", RIN_AGGRAVATE_MONSTER },
    { "激怒怪物之戒指", RIN_AGGRAVATE_MONSTER },
    { "激怒怪物之戒", RIN_AGGRAVATE_MONSTER },
    { "怪物激怒戒指", RIN_AGGRAVATE_MONSTER },
    { "怪物激怒的戒指", RIN_AGGRAVATE_MONSTER },
    { "怪物激怒之戒指", RIN_AGGRAVATE_MONSTER },
    { "怪物激怒之戒", RIN_AGGRAVATE_MONSTER },
    { "冲突戒指", RIN_CONFLICT },
    { "冲突的戒指", RIN_CONFLICT },
    { "冲突之戒指", RIN_CONFLICT },
    { "冲突之戒", RIN_CONFLICT },
    { "警报戒指", RIN_WARNING },
    { "警报的戒指", RIN_WARNING },
    { "警报之戒指", RIN_WARNING },
    { "警报之戒", RIN_WARNING },
    { "警觉戒指", RIN_WARNING },
    { "警觉的戒指", RIN_WARNING },
    { "警觉之戒指", RIN_WARNING },
    { "警觉之戒", RIN_WARNING },
    { "毒抗戒指", RIN_POISON_RESISTANCE },
    { "毒抗的戒指", RIN_POISON_RESISTANCE },
    { "毒抗之戒指", RIN_POISON_RESISTANCE },
    { "毒抗之戒", RIN_POISON_RESISTANCE },
    { "抗毒戒指", RIN_POISON_RESISTANCE },
    { "抗毒的戒指", RIN_POISON_RESISTANCE },
    { "抗毒之戒指", RIN_POISON_RESISTANCE },
    { "抗毒之戒", RIN_POISON_RESISTANCE },
    { "毒抗性戒指", RIN_POISON_RESISTANCE },
    { "毒抗性的戒指", RIN_POISON_RESISTANCE },
    { "毒抗性之戒指", RIN_POISON_RESISTANCE },
    { "毒抗性之戒", RIN_POISON_RESISTANCE },
    { "抗毒性戒指", RIN_POISON_RESISTANCE },
    { "抗毒性的戒指", RIN_POISON_RESISTANCE },
    { "抗毒性之戒指", RIN_POISON_RESISTANCE },
    { "抗毒性之戒", RIN_POISON_RESISTANCE },
    { "毒性免疫戒指", RIN_POISON_RESISTANCE },
    { "毒性免疫的戒指", RIN_POISON_RESISTANCE },
    { "毒性免疫之戒指", RIN_POISON_RESISTANCE },
    { "毒性免疫之戒", RIN_POISON_RESISTANCE },
    { "火抗戒指", RIN_FIRE_RESISTANCE },
    { "火抗的戒指", RIN_FIRE_RESISTANCE },
    { "火抗之戒指", RIN_FIRE_RESISTANCE },
    { "火抗之戒", RIN_FIRE_RESISTANCE },
    { "抗火戒指", RIN_FIRE_RESISTANCE },
    { "抗火的戒指", RIN_FIRE_RESISTANCE },
    { "抗火之戒指", RIN_FIRE_RESISTANCE },
    { "抗火之戒", RIN_FIRE_RESISTANCE },
    { "火抗性戒指", RIN_FIRE_RESISTANCE },
    { "火抗性的戒指", RIN_FIRE_RESISTANCE },
    { "火抗性之戒指", RIN_FIRE_RESISTANCE },
    { "火抗性之戒", RIN_FIRE_RESISTANCE },
    { "火焰抗性戒指", RIN_FIRE_RESISTANCE },
    { "火焰抗性的戒指", RIN_FIRE_RESISTANCE },
    { "火焰抗性之戒指", RIN_FIRE_RESISTANCE },
    { "火焰抗性之戒", RIN_FIRE_RESISTANCE },
    { "燃烧抗性戒指", RIN_FIRE_RESISTANCE },
    { "燃烧抗性的戒指", RIN_FIRE_RESISTANCE },
    { "燃烧抗性之戒指", RIN_FIRE_RESISTANCE },
    { "燃烧抗性之戒", RIN_FIRE_RESISTANCE },
    { "火免疫戒指", RIN_FIRE_RESISTANCE },
    { "火免疫的戒指", RIN_FIRE_RESISTANCE },
    { "火免疫之戒指", RIN_FIRE_RESISTANCE },
    { "火免疫之戒", RIN_FIRE_RESISTANCE },
    { "火焰免疫戒指", RIN_FIRE_RESISTANCE },
    { "火焰免疫的戒指", RIN_FIRE_RESISTANCE },
    { "火焰免疫之戒指", RIN_FIRE_RESISTANCE },
    { "火焰免疫之戒", RIN_FIRE_RESISTANCE },
    { "燃烧免疫戒指", RIN_FIRE_RESISTANCE },
    { "燃烧免疫的戒指", RIN_FIRE_RESISTANCE },
    { "燃烧免疫之戒指", RIN_FIRE_RESISTANCE },
    { "燃烧免疫之戒", RIN_FIRE_RESISTANCE },
    { "寒抗戒指", RIN_COLD_RESISTANCE },
    { "寒抗的戒指", RIN_COLD_RESISTANCE },
    { "寒抗之戒指", RIN_COLD_RESISTANCE },
    { "寒抗之戒", RIN_COLD_RESISTANCE },
    { "冰抗戒指", RIN_COLD_RESISTANCE },
    { "冰抗的戒指", RIN_COLD_RESISTANCE },
    { "冰抗之戒指", RIN_COLD_RESISTANCE },
    { "冰抗之戒", RIN_COLD_RESISTANCE },
    { "抗寒戒指", RIN_COLD_RESISTANCE },
    { "抗寒的戒指", RIN_COLD_RESISTANCE },
    { "抗寒之戒指", RIN_COLD_RESISTANCE },
    { "抗寒之戒", RIN_COLD_RESISTANCE },
    { "抗冰戒指", RIN_COLD_RESISTANCE },
    { "抗冰的戒指", RIN_COLD_RESISTANCE },
    { "抗冰之戒指", RIN_COLD_RESISTANCE },
    { "抗冰之戒", RIN_COLD_RESISTANCE },
    { "寒冷抗性戒指", RIN_COLD_RESISTANCE },
    { "寒冷抗性的戒指", RIN_COLD_RESISTANCE },
    { "寒冷抗性之戒指", RIN_COLD_RESISTANCE },
    { "寒冷抗性之戒", RIN_COLD_RESISTANCE },
    { "寒冰抗性戒指", RIN_COLD_RESISTANCE },
    { "寒冰抗性的戒指", RIN_COLD_RESISTANCE },
    { "寒冰抗性之戒指", RIN_COLD_RESISTANCE },
    { "寒冰抗性之戒", RIN_COLD_RESISTANCE },
    { "冰抗性戒指", RIN_COLD_RESISTANCE },
    { "冰抗性的戒指", RIN_COLD_RESISTANCE },
    { "冰抗性之戒指", RIN_COLD_RESISTANCE },
    { "冰抗性之戒", RIN_COLD_RESISTANCE },
    { "冰冻抗性戒指", RIN_COLD_RESISTANCE },
    { "冰冻抗性的戒指", RIN_COLD_RESISTANCE },
    { "冰冻抗性之戒指", RIN_COLD_RESISTANCE },
    { "冰冻抗性之戒", RIN_COLD_RESISTANCE },
    { "寒冷免疫戒指", RIN_COLD_RESISTANCE },
    { "寒冷免疫的戒指", RIN_COLD_RESISTANCE },
    { "寒冷免疫之戒指", RIN_COLD_RESISTANCE },
    { "寒冷免疫之戒", RIN_COLD_RESISTANCE },
    { "寒冰免疫戒指", RIN_COLD_RESISTANCE },
    { "寒冰免疫的戒指", RIN_COLD_RESISTANCE },
    { "寒冰免疫之戒指", RIN_COLD_RESISTANCE },
    { "寒冰免疫之戒", RIN_COLD_RESISTANCE },
    { "冰免疫戒指", RIN_COLD_RESISTANCE },
    { "冰免疫的戒指", RIN_COLD_RESISTANCE },
    { "冰免疫之戒指", RIN_COLD_RESISTANCE },
    { "冰免疫之戒", RIN_COLD_RESISTANCE },
    { "冰冻免疫戒指", RIN_COLD_RESISTANCE },
    { "冰冻免疫的戒指", RIN_COLD_RESISTANCE },
    { "冰冻免疫之戒指", RIN_COLD_RESISTANCE },
    { "冰冻免疫之戒", RIN_COLD_RESISTANCE },
    { "电抗戒指", RIN_SHOCK_RESISTANCE },
    { "电抗的戒指", RIN_SHOCK_RESISTANCE },
    { "电抗之戒指", RIN_SHOCK_RESISTANCE },
    { "电抗之戒", RIN_SHOCK_RESISTANCE },
    { "雷抗戒指", RIN_SHOCK_RESISTANCE },
    { "雷抗的戒指", RIN_SHOCK_RESISTANCE },
    { "雷抗之戒指", RIN_SHOCK_RESISTANCE },
    { "雷抗之戒", RIN_SHOCK_RESISTANCE },
    { "抗电戒指", RIN_SHOCK_RESISTANCE },
    { "抗电的戒指", RIN_SHOCK_RESISTANCE },
    { "抗电之戒指", RIN_SHOCK_RESISTANCE },
    { "抗电之戒", RIN_SHOCK_RESISTANCE },
    { "抗雷戒指", RIN_SHOCK_RESISTANCE },
    { "抗雷的戒指", RIN_SHOCK_RESISTANCE },
    { "抗雷之戒指", RIN_SHOCK_RESISTANCE },
    { "抗雷之戒", RIN_SHOCK_RESISTANCE },
    { "电抗性戒指", RIN_SHOCK_RESISTANCE },
    { "电抗性的戒指", RIN_SHOCK_RESISTANCE },
    { "电抗性之戒指", RIN_SHOCK_RESISTANCE },
    { "电抗性之戒", RIN_SHOCK_RESISTANCE },
    { "闪电抗性戒指", RIN_SHOCK_RESISTANCE },
    { "闪电抗性的戒指", RIN_SHOCK_RESISTANCE },
    { "闪电抗性之戒指", RIN_SHOCK_RESISTANCE },
    { "闪电抗性之戒", RIN_SHOCK_RESISTANCE },
    { "雷电抗性戒指", RIN_SHOCK_RESISTANCE },
    { "雷电抗性的戒指", RIN_SHOCK_RESISTANCE },
    { "雷电抗性之戒指", RIN_SHOCK_RESISTANCE },
    { "雷电抗性之戒", RIN_SHOCK_RESISTANCE },
    { "电击抗性戒指", RIN_SHOCK_RESISTANCE },
    { "电击抗性的戒指", RIN_SHOCK_RESISTANCE },
    { "电击抗性之戒指", RIN_SHOCK_RESISTANCE },
    { "电击抗性之戒", RIN_SHOCK_RESISTANCE },
    { "雷击抗性戒指", RIN_SHOCK_RESISTANCE },
    { "雷击抗性的戒指", RIN_SHOCK_RESISTANCE },
    { "雷击抗性之戒指", RIN_SHOCK_RESISTANCE },
    { "雷击抗性之戒", RIN_SHOCK_RESISTANCE },
    { "电免疫戒指", RIN_SHOCK_RESISTANCE },
    { "电免疫的戒指", RIN_SHOCK_RESISTANCE },
    { "电免疫之戒指", RIN_SHOCK_RESISTANCE },
    { "电免疫之戒", RIN_SHOCK_RESISTANCE },
    { "闪电免疫戒指", RIN_SHOCK_RESISTANCE },
    { "闪电免疫的戒指", RIN_SHOCK_RESISTANCE },
    { "闪电免疫之戒指", RIN_SHOCK_RESISTANCE },
    { "闪电免疫之戒", RIN_SHOCK_RESISTANCE },
    { "雷电免疫戒指", RIN_SHOCK_RESISTANCE },
    { "雷电免疫的戒指", RIN_SHOCK_RESISTANCE },
    { "雷电免疫之戒指", RIN_SHOCK_RESISTANCE },
    { "雷电免疫之戒", RIN_SHOCK_RESISTANCE },
    { "电击免疫戒指", RIN_SHOCK_RESISTANCE },
    { "电击免疫的戒指", RIN_SHOCK_RESISTANCE },
    { "电击免疫之戒指", RIN_SHOCK_RESISTANCE },
    { "电击免疫之戒", RIN_SHOCK_RESISTANCE },
    { "雷击免疫戒指", RIN_SHOCK_RESISTANCE },
    { "雷击免疫的戒指", RIN_SHOCK_RESISTANCE },
    { "雷击免疫之戒指", RIN_SHOCK_RESISTANCE },
    { "雷击免疫之戒", RIN_SHOCK_RESISTANCE },
    { "自由行动戒指", RIN_FREE_ACTION },
    { "自由行动的戒指", RIN_FREE_ACTION },
    { "自由行动之戒指", RIN_FREE_ACTION },
    { "自由行动之戒", RIN_FREE_ACTION },
    { "自由移动戒指", RIN_FREE_ACTION },
    { "自由移动的戒指", RIN_FREE_ACTION },
    { "自由移动之戒指", RIN_FREE_ACTION },
    { "自由移动之戒", RIN_FREE_ACTION },
    { "自由戒指", RIN_FREE_ACTION },
    { "自由的戒指", RIN_FREE_ACTION },
    { "自由之戒指", RIN_FREE_ACTION },
    { "自由之戒", RIN_FREE_ACTION },
    { "抗定身戒指", RIN_FREE_ACTION },
    { "抗定身的戒指", RIN_FREE_ACTION },
    { "抗定身之戒指", RIN_FREE_ACTION },
    { "抗定身之戒", RIN_FREE_ACTION },
    { "防定身戒指", RIN_FREE_ACTION },
    { "防定身的戒指", RIN_FREE_ACTION },
    { "防定身之戒指", RIN_FREE_ACTION },
    { "防定身之戒", RIN_FREE_ACTION },
    { "防止定身戒指", RIN_FREE_ACTION },
    { "防止定身的戒指", RIN_FREE_ACTION },
    { "防止定身之戒指", RIN_FREE_ACTION },
    { "防止定身之戒", RIN_FREE_ACTION },
    { "定身抗性戒指", RIN_FREE_ACTION },
    { "定身抗性的戒指", RIN_FREE_ACTION },
    { "定身抗性之戒指", RIN_FREE_ACTION },
    { "定身抗性之戒", RIN_FREE_ACTION },
    { "慢消化戒指", RIN_SLOW_DIGESTION },
    { "慢消化的戒指", RIN_SLOW_DIGESTION },
    { "慢消化之戒指", RIN_SLOW_DIGESTION },
    { "慢消化之戒", RIN_SLOW_DIGESTION },
    { "减慢消化戒指", RIN_SLOW_DIGESTION },
    { "减慢消化的戒指", RIN_SLOW_DIGESTION },
    { "减慢消化之戒指", RIN_SLOW_DIGESTION },
    { "减慢消化之戒", RIN_SLOW_DIGESTION },
    { "减速消化戒指", RIN_SLOW_DIGESTION },
    { "减速消化的戒指", RIN_SLOW_DIGESTION },
    { "减速消化之戒指", RIN_SLOW_DIGESTION },
    { "减速消化之戒", RIN_SLOW_DIGESTION },
    { "传送戒指", RIN_TELEPORTATION },
    { "传送的戒指", RIN_TELEPORTATION },
    { "传送之戒指", RIN_TELEPORTATION },
    { "传送之戒", RIN_TELEPORTATION },
    { "传送控制戒指", RIN_TELEPORT_CONTROL },
    { "传送控制的戒指", RIN_TELEPORT_CONTROL },
    { "传送控制之戒指", RIN_TELEPORT_CONTROL },
    { "传送控制之戒", RIN_TELEPORT_CONTROL },
    { "控制传送戒指", RIN_TELEPORT_CONTROL },
    { "控制传送的戒指", RIN_TELEPORT_CONTROL },
    { "控制传送之戒指", RIN_TELEPORT_CONTROL },
    { "控制传送之戒", RIN_TELEPORT_CONTROL },
    { "可控传送戒指", RIN_TELEPORT_CONTROL },
    { "可控传送的戒指", RIN_TELEPORT_CONTROL },
    { "可控传送之戒指", RIN_TELEPORT_CONTROL },
    { "可控传送之戒", RIN_TELEPORT_CONTROL },
    { "变形戒指", RIN_POLYMORPH },
    { "变形的戒指", RIN_POLYMORPH },
    { "变形之戒指", RIN_POLYMORPH },
    { "变形之戒", RIN_POLYMORPH },
    { "变形控制戒指", RIN_POLYMORPH_CONTROL },
    { "变形控制的戒指", RIN_POLYMORPH_CONTROL },
    { "变形控制之戒指", RIN_POLYMORPH_CONTROL },
    { "变形控制之戒", RIN_POLYMORPH_CONTROL },
    { "控制变形戒指", RIN_POLYMORPH_CONTROL },
    { "控制变形的戒指", RIN_POLYMORPH_CONTROL },
    { "控制变形之戒指", RIN_POLYMORPH_CONTROL },
    { "控制变形之戒", RIN_POLYMORPH_CONTROL },
    { "可控变形戒指", RIN_POLYMORPH_CONTROL },
    { "可控变形的戒指", RIN_POLYMORPH_CONTROL },
    { "可控变形之戒指", RIN_POLYMORPH_CONTROL },
    { "可控变形之戒", RIN_POLYMORPH_CONTROL },
    { "隐身戒指", RIN_INVISIBILITY },
    { "隐身的戒指", RIN_INVISIBILITY },
    { "隐身之戒指", RIN_INVISIBILITY },
    { "隐身之戒", RIN_INVISIBILITY },
    { "隐形戒指", RIN_INVISIBILITY },
    { "隐形的戒指", RIN_INVISIBILITY },
    { "隐形之戒指", RIN_INVISIBILITY },
    { "隐形之戒", RIN_INVISIBILITY },
    { "看见隐身戒指", RIN_SEE_INVISIBLE },
    { "看见隐身的戒指", RIN_SEE_INVISIBLE },
    { "看见隐身之戒指", RIN_SEE_INVISIBLE },
    { "看见隐身之戒", RIN_SEE_INVISIBLE },
    { "看见隐形戒指", RIN_SEE_INVISIBLE },
    { "看见隐形的戒指", RIN_SEE_INVISIBLE },
    { "看见隐形之戒指", RIN_SEE_INVISIBLE },
    { "看见隐形之戒", RIN_SEE_INVISIBLE },
    { "看穿隐身戒指", RIN_SEE_INVISIBLE },
    { "看穿隐身的戒指", RIN_SEE_INVISIBLE },
    { "看穿隐身之戒指", RIN_SEE_INVISIBLE },
    { "看穿隐身之戒", RIN_SEE_INVISIBLE },
    { "看穿隐形戒指", RIN_SEE_INVISIBLE },
    { "看穿隐形的戒指", RIN_SEE_INVISIBLE },
    { "看穿隐形之戒指", RIN_SEE_INVISIBLE },
    { "看穿隐形之戒", RIN_SEE_INVISIBLE },
    { "看透隐身戒指", RIN_SEE_INVISIBLE },
    { "看透隐身的戒指", RIN_SEE_INVISIBLE },
    { "看透隐身之戒指", RIN_SEE_INVISIBLE },
    { "看透隐身之戒", RIN_SEE_INVISIBLE },
    { "看透隐形戒指", RIN_SEE_INVISIBLE },
    { "看透隐形的戒指", RIN_SEE_INVISIBLE },
    { "看透隐形之戒指", RIN_SEE_INVISIBLE },
    { "看透隐形之戒", RIN_SEE_INVISIBLE },
    { "怪物现形戒指", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "怪物现形的戒指", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "怪物现形之戒指", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "怪物现形之戒", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "变形怪现形戒指", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "变形怪现形的戒指", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "变形怪现形之戒指", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "变形怪现形之戒", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "防变形怪戒指", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "防变形怪的戒指", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "防变形怪之戒指", RIN_PROTECTION_FROM_SHAPE_CHAN },
    { "防变形怪之戒", RIN_PROTECTION_FROM_SHAPE_CHAN },
    /*所有可能的护身符*/
    { "感知护身符", AMULET_OF_ESP },
    { "感知的护身符", AMULET_OF_ESP },
    { "感知之护身符", AMULET_OF_ESP },
    { "感知护符", AMULET_OF_ESP },
    { "感知的护符", AMULET_OF_ESP },
    { "感知之护符", AMULET_OF_ESP },
    { "ESP护身符", AMULET_OF_ESP },
    { "ESP的护身符", AMULET_OF_ESP },
    { "ESP之护身符", AMULET_OF_ESP },
    { "ESP护符", AMULET_OF_ESP },
    { "ESP的护符", AMULET_OF_ESP },
    { "ESP之护符", AMULET_OF_ESP },
    { "透视护身符", AMULET_OF_ESP },
    { "透视的护身符", AMULET_OF_ESP },
    { "透视之护身符", AMULET_OF_ESP },
    { "透视护符", AMULET_OF_ESP },
    { "透视的护符", AMULET_OF_ESP },
    { "透视之护符", AMULET_OF_ESP },
    { "心灵感应护身符", AMULET_OF_ESP },
    { "心灵感应的护身符", AMULET_OF_ESP },
    { "心灵感应之护身符", AMULET_OF_ESP },
    { "心灵感应护符", AMULET_OF_ESP },
    { "心灵感应的护符", AMULET_OF_ESP },
    { "心灵感应之护符", AMULET_OF_ESP },
    { "复活护身符", AMULET_OF_LIFE_SAVING },
    { "复活的护身符", AMULET_OF_LIFE_SAVING },
    { "复活之护身符", AMULET_OF_LIFE_SAVING },
    { "复活护符", AMULET_OF_LIFE_SAVING },
    { "复活的护符", AMULET_OF_LIFE_SAVING },
    { "复活之护符", AMULET_OF_LIFE_SAVING },
    { "救命护身符", AMULET_OF_LIFE_SAVING },
    { "救命的护身符", AMULET_OF_LIFE_SAVING },
    { "救命之护身符", AMULET_OF_LIFE_SAVING },
    { "救命护符", AMULET_OF_LIFE_SAVING },
    { "救命的护符", AMULET_OF_LIFE_SAVING },
    { "救命之护符", AMULET_OF_LIFE_SAVING },
    { "保命护身符", AMULET_OF_LIFE_SAVING },
    { "保命的护身符", AMULET_OF_LIFE_SAVING },
    { "保命之护身符", AMULET_OF_LIFE_SAVING },
    { "保命护符", AMULET_OF_LIFE_SAVING },
    { "保命的护符", AMULET_OF_LIFE_SAVING },
    { "保命之护符", AMULET_OF_LIFE_SAVING },
    { "窒息护身符", AMULET_OF_STRANGULATION },
    { "窒息的护身符", AMULET_OF_STRANGULATION },
    { "窒息之护身符", AMULET_OF_STRANGULATION },
    { "窒息护符", AMULET_OF_STRANGULATION },
    { "窒息的护符", AMULET_OF_STRANGULATION },
    { "窒息之护符", AMULET_OF_STRANGULATION },
    { "深度睡眠护身符", AMULET_OF_RESTFUL_SLEEP },
    { "深度睡眠的护身符", AMULET_OF_RESTFUL_SLEEP },
    { "深度睡眠之护身符", AMULET_OF_RESTFUL_SLEEP },
    { "深度睡眠护符", AMULET_OF_RESTFUL_SLEEP },
    { "深度睡眠的护符", AMULET_OF_RESTFUL_SLEEP },
    { "深度睡眠之护符", AMULET_OF_RESTFUL_SLEEP },
    { "毒抗护身符", AMULET_VERSUS_POISON },
    { "毒抗的护身符", AMULET_VERSUS_POISON },
    { "毒抗之护身符", AMULET_VERSUS_POISON },
    { "毒抗护符", AMULET_VERSUS_POISON },
    { "毒抗的护符", AMULET_VERSUS_POISON },
    { "毒抗之护符", AMULET_VERSUS_POISON },
    { "抗毒护身符", AMULET_VERSUS_POISON },
    { "抗毒的护身符", AMULET_VERSUS_POISON },
    { "抗毒之护身符", AMULET_VERSUS_POISON },
    { "抗毒护符", AMULET_VERSUS_POISON },
    { "抗毒的护符", AMULET_VERSUS_POISON },
    { "抗毒之护符", AMULET_VERSUS_POISON },
    { "毒抗性护身符", AMULET_VERSUS_POISON },
    { "毒抗性的护身符", AMULET_VERSUS_POISON },
    { "毒抗性之护身符", AMULET_VERSUS_POISON },
    { "毒抗性护符", AMULET_VERSUS_POISON },
    { "毒抗性的护符", AMULET_VERSUS_POISON },
    { "毒抗性之护符", AMULET_VERSUS_POISON },
    { "抗毒性护身符", AMULET_VERSUS_POISON },
    { "抗毒性的护身符", AMULET_VERSUS_POISON },
    { "抗毒性之护身符", AMULET_VERSUS_POISON },
    { "抗毒性护符", AMULET_VERSUS_POISON },
    { "抗毒性的护符", AMULET_VERSUS_POISON },
    { "抗毒性之护符", AMULET_VERSUS_POISON },
    { "毒性免疫护身符", AMULET_VERSUS_POISON },
    { "毒性免疫的护身符", AMULET_VERSUS_POISON },
    { "毒性免疫之护身符", AMULET_VERSUS_POISON },
    { "毒性免疫护符", AMULET_VERSUS_POISON },
    { "毒性免疫的护符", AMULET_VERSUS_POISON },
    { "毒性免疫之护符", AMULET_VERSUS_POISON },
    { "变性护身符", AMULET_OF_CHANGE },
    { "变性的护身符", AMULET_OF_CHANGE },
    { "变性之护身符", AMULET_OF_CHANGE },
    { "变性护符", AMULET_OF_CHANGE },
    { "变性的护符", AMULET_OF_CHANGE },
    { "变性之护符", AMULET_OF_CHANGE },
    { "防变形护身符", AMULET_OF_UNCHANGING },
    { "防变形的护身符", AMULET_OF_UNCHANGING },
    { "防变形之护身符", AMULET_OF_UNCHANGING },
    { "防变形护符", AMULET_OF_UNCHANGING },
    { "防变形的护符", AMULET_OF_UNCHANGING },
    { "防变形之护符", AMULET_OF_UNCHANGING },
    { "抗变形护身符", AMULET_OF_UNCHANGING },
    { "抗变形的护身符", AMULET_OF_UNCHANGING },
    { "抗变形之护身符", AMULET_OF_UNCHANGING },
    { "抗变形护符", AMULET_OF_UNCHANGING },
    { "抗变形的护符", AMULET_OF_UNCHANGING },
    { "抗变形之护符", AMULET_OF_UNCHANGING },
    { "阻止变形护身符", AMULET_OF_UNCHANGING },
    { "阻止变形的护身符", AMULET_OF_UNCHANGING },
    { "阻止变形之护身符", AMULET_OF_UNCHANGING },
    { "阻止变形护符", AMULET_OF_UNCHANGING },
    { "阻止变形的护符", AMULET_OF_UNCHANGING },
    { "阻止变形之护符", AMULET_OF_UNCHANGING },
    { "防止变形护身符", AMULET_OF_UNCHANGING },
    { "防止变形的护身符", AMULET_OF_UNCHANGING },
    { "防止变形之护身符", AMULET_OF_UNCHANGING },
    { "防止变形护符", AMULET_OF_UNCHANGING },
    { "防止变形的护符", AMULET_OF_UNCHANGING },
    { "防止变形之护符", AMULET_OF_UNCHANGING },
    { "变形抗性护身符", AMULET_OF_UNCHANGING },
    { "变形抗性的护身符", AMULET_OF_UNCHANGING },
    { "变形抗性之护身符", AMULET_OF_UNCHANGING },
    { "变形抗性护符", AMULET_OF_UNCHANGING },
    { "变形抗性的护符", AMULET_OF_UNCHANGING },
    { "变形抗性之护符", AMULET_OF_UNCHANGING },
    { "变形免疫护身符", AMULET_OF_UNCHANGING },
    { "变形免疫的护身符", AMULET_OF_UNCHANGING },
    { "变形免疫之护身符", AMULET_OF_UNCHANGING },
    { "变形免疫护符", AMULET_OF_UNCHANGING },
    { "变形免疫的护符", AMULET_OF_UNCHANGING },
    { "变形免疫之护符", AMULET_OF_UNCHANGING },
    { "反射护身符", AMULET_OF_REFLECTION },
    { "反射的护身符", AMULET_OF_REFLECTION },
    { "反射之护身符", AMULET_OF_REFLECTION },
    { "反射护符", AMULET_OF_REFLECTION },
    { "反射的护符", AMULET_OF_REFLECTION },
    { "反射之护符", AMULET_OF_REFLECTION },
    { "魔法呼吸护身符", AMULET_OF_MAGICAL_BREATHING },
    { "魔法呼吸的护身符", AMULET_OF_MAGICAL_BREATHING },
    { "魔法呼吸之护身符", AMULET_OF_MAGICAL_BREATHING },
    { "魔法呼吸护符", AMULET_OF_MAGICAL_BREATHING },
    { "魔法呼吸的护符", AMULET_OF_MAGICAL_BREATHING },
    { "魔法呼吸之护符", AMULET_OF_MAGICAL_BREATHING },
    { "保护护身符", AMULET_OF_GUARDING },
    { "保护的护身符", AMULET_OF_GUARDING },
    { "保护之护身符", AMULET_OF_GUARDING },
    { "保护护符", AMULET_OF_GUARDING },
    { "保护的护符", AMULET_OF_GUARDING },
    { "保护之护符", AMULET_OF_GUARDING },
    { "飞行护身符", AMULET_OF_FLYING },
    { "飞行的护身符", AMULET_OF_FLYING },
    { "飞行之护身符", AMULET_OF_FLYING },
    { "飞行护符", AMULET_OF_FLYING },
    { "飞行的护符", AMULET_OF_FLYING },
    { "飞行之护符", AMULET_OF_FLYING },
    { "岩德护身符", AMULET_OF_YENDOR },
    { "岩德的护身符", AMULET_OF_YENDOR },
    { "岩德之护身符", AMULET_OF_YENDOR },
    { "岩德护符", AMULET_OF_YENDOR },
    { "岩德的护符", AMULET_OF_YENDOR },
    { "岩德之护符", AMULET_OF_YENDOR },
    { "假岩德护身符", FAKE_AMULET_OF_YENDOR },
    { "假岩德的护身符", FAKE_AMULET_OF_YENDOR },
    { "假岩德之护身符", FAKE_AMULET_OF_YENDOR },
    { "假岩德护符", FAKE_AMULET_OF_YENDOR },
    { "假岩德的护符", FAKE_AMULET_OF_YENDOR },
    { "假岩德之护符", FAKE_AMULET_OF_YENDOR },
    { "假的岩德护身符", FAKE_AMULET_OF_YENDOR },
    { "假的岩德的护身符", FAKE_AMULET_OF_YENDOR },
    { "假的岩德之护身符", FAKE_AMULET_OF_YENDOR },
    { "假的岩德护符", FAKE_AMULET_OF_YENDOR },
    { "假的岩德的护符", FAKE_AMULET_OF_YENDOR },
    { "假的岩德之护符", FAKE_AMULET_OF_YENDOR },
    { "伪造的岩德护身符", FAKE_AMULET_OF_YENDOR },
    { "伪造的岩德的护身符", FAKE_AMULET_OF_YENDOR },
    { "伪造的岩德之护身符", FAKE_AMULET_OF_YENDOR },
    { "伪造的岩德护符", FAKE_AMULET_OF_YENDOR },
    { "伪造的岩德的护符", FAKE_AMULET_OF_YENDOR },
    { "伪造的岩德之护符", FAKE_AMULET_OF_YENDOR },
    { "岩德护身符的廉价塑料仿制品", FAKE_AMULET_OF_YENDOR },
    { "岩德的护身符的廉价塑料仿制品", FAKE_AMULET_OF_YENDOR },
    { "岩德之护身符的廉价塑料仿制品", FAKE_AMULET_OF_YENDOR },
    { "岩德护符的廉价塑料仿制品", FAKE_AMULET_OF_YENDOR },
    { "岩德的护符的廉价塑料仿制品", FAKE_AMULET_OF_YENDOR },
    { "岩德之护符的廉价塑料仿制品", FAKE_AMULET_OF_YENDOR },
    /*所有可能的药水*/
    { "增强能力药水", POT_GAIN_ABILITY },
    { "增强能力的药水", POT_GAIN_ABILITY },
    { "增强能力之药水", POT_GAIN_ABILITY },
    { "恢复能力药水", POT_RESTORE_ABILITY },
    { "恢复能力的药水", POT_RESTORE_ABILITY },
    { "恢复能力之药水", POT_RESTORE_ABILITY },
    { "混乱药水", POT_CONFUSION },
    { "混乱的药水", POT_CONFUSION },
    { "混乱之药水", POT_CONFUSION },
    { "失明药水", POT_BLINDNESS },
    { "失明的药水", POT_BLINDNESS },
    { "失明之药水", POT_BLINDNESS },
    { "麻痹药水", POT_PARALYSIS },
    { "麻痹的药水", POT_PARALYSIS },
    { "麻痹之药水", POT_PARALYSIS },
    { "加速药水", POT_SPEED },
    { "加速的药水", POT_SPEED },
    { "加速之药水", POT_SPEED },
    { "飘浮药水", POT_LEVITATION },
    { "飘浮的药水", POT_LEVITATION },
    { "飘浮之药水", POT_LEVITATION },
    { "幻觉药水", POT_HALLUCINATION },
    { "幻觉的药水", POT_HALLUCINATION },
    { "幻觉之药水", POT_HALLUCINATION },
    { "隐身药水", POT_INVISIBILITY },
    { "隐身的药水", POT_INVISIBILITY },
    { "隐身之药水", POT_INVISIBILITY },
    { "看见隐形药水", POT_SEE_INVISIBLE },
    { "看见隐形的药水", POT_SEE_INVISIBLE },
    { "看见隐形之药水", POT_SEE_INVISIBLE },
    { "看见隐身药水", POT_SEE_INVISIBLE },
    { "看见隐身的药水", POT_SEE_INVISIBLE },
    { "看见隐身之药水", POT_SEE_INVISIBLE },
    { "看穿隐形药水", POT_SEE_INVISIBLE },
    { "看穿隐形的药水", POT_SEE_INVISIBLE },
    { "看穿隐形之药水", POT_SEE_INVISIBLE },
    { "看穿隐身药水", POT_SEE_INVISIBLE },
    { "看穿隐身的药水", POT_SEE_INVISIBLE },
    { "看穿隐身之药水", POT_SEE_INVISIBLE },
    { "看透隐形药水", POT_SEE_INVISIBLE },
    { "看透隐形的药水", POT_SEE_INVISIBLE },
    { "看透隐形之药水", POT_SEE_INVISIBLE },
    { "看透隐身药水", POT_SEE_INVISIBLE },
    { "看透隐身的药水", POT_SEE_INVISIBLE },
    { "看透隐身之药水", POT_SEE_INVISIBLE },
    { "治愈药水", POT_HEALING },
    { "治愈的药水", POT_HEALING },
    { "治愈之药水", POT_HEALING },
    { "强力治愈药水", POT_EXTRA_HEALING },
    { "强力治愈的药水", POT_EXTRA_HEALING },
    { "强力治愈之药水", POT_EXTRA_HEALING },
    { "完全治愈药水", POT_FULL_HEALING },
    { "完全治愈的药水", POT_FULL_HEALING },
    { "完全治愈之药水", POT_FULL_HEALING },
    { "治疗药水", POT_HEALING },
    { "治疗的药水", POT_HEALING },
    { "治疗之药水", POT_HEALING },
    { "强力治疗药水", POT_EXTRA_HEALING },
    { "强力治疗的药水", POT_EXTRA_HEALING },
    { "强力治疗之药水", POT_EXTRA_HEALING },
    { "完全治疗药水", POT_FULL_HEALING },
    { "完全治疗的药水", POT_FULL_HEALING },
    { "完全治疗之药水", POT_FULL_HEALING },
    { "升级药水", POT_GAIN_LEVEL },
    { "升级的药水", POT_GAIN_LEVEL },
    { "升级之药水", POT_GAIN_LEVEL },
    { "获得等级药水", POT_GAIN_LEVEL },
    { "获得等级的药水", POT_GAIN_LEVEL },
    { "获得等级之药水", POT_GAIN_LEVEL },
    { "提高等级药水", POT_GAIN_LEVEL },
    { "提高等级的药水", POT_GAIN_LEVEL },
    { "提高等级之药水", POT_GAIN_LEVEL },
    { "提升等级药水", POT_GAIN_LEVEL },
    { "提升等级的药水", POT_GAIN_LEVEL },
    { "提升等级之药水", POT_GAIN_LEVEL },
    { "启蒙药水", POT_ENLIGHTENMENT },
    { "启蒙的药水", POT_ENLIGHTENMENT },
    { "启蒙之药水", POT_ENLIGHTENMENT },
    { "自知药水", POT_ENLIGHTENMENT },
    { "自知的药水", POT_ENLIGHTENMENT },
    { "自知之药水", POT_ENLIGHTENMENT },
    { "了解自身药水", POT_ENLIGHTENMENT },
    { "了解自身的药水", POT_ENLIGHTENMENT },
    { "了解自身之药水", POT_ENLIGHTENMENT },
    { "了解自己药水", POT_ENLIGHTENMENT },
    { "了解自己的药水", POT_ENLIGHTENMENT },
    { "了解自己之药水", POT_ENLIGHTENMENT },
    { "怪物探测药水", POT_MONSTER_DETECTION },
    { "怪物探测的药水", POT_MONSTER_DETECTION },
    { "怪物探测之药水", POT_MONSTER_DETECTION },
    { "探测怪物药水", POT_MONSTER_DETECTION },
    { "探测怪物的药水", POT_MONSTER_DETECTION },
    { "探测怪物之药水", POT_MONSTER_DETECTION },
    { "怪物发现药水", POT_MONSTER_DETECTION },
    { "怪物发现的药水", POT_MONSTER_DETECTION },
    { "怪物发现之药水", POT_MONSTER_DETECTION },
    { "发现怪物药水", POT_MONSTER_DETECTION },
    { "发现怪物的药水", POT_MONSTER_DETECTION },
    { "发现怪物之药水", POT_MONSTER_DETECTION },
    { "物品探测药水", POT_OBJECT_DETECTION },
    { "物品探测的药水", POT_OBJECT_DETECTION },
    { "物品探测之药水", POT_OBJECT_DETECTION },
    { "探测物品药水", POT_OBJECT_DETECTION },
    { "探测物品的药水", POT_OBJECT_DETECTION },
    { "探测物品之药水", POT_OBJECT_DETECTION },
    { "物品发现药水", POT_OBJECT_DETECTION },
    { "物品发现的药水", POT_OBJECT_DETECTION },
    { "物品发现之药水", POT_OBJECT_DETECTION },
    { "发现物品药水", POT_OBJECT_DETECTION },
    { "发现物品的药水", POT_OBJECT_DETECTION },
    { "发现物品之药水", POT_OBJECT_DETECTION },
    { "获得能量药水", POT_GAIN_ENERGY },
    { "获得能量的药水", POT_GAIN_ENERGY },
    { "获得能量之药水", POT_GAIN_ENERGY },
    { "能量药水", POT_GAIN_ENERGY },
    { "能量的药水", POT_GAIN_ENERGY },
    { "能量之药水", POT_GAIN_ENERGY },
    { "获得魔法能量药水", POT_GAIN_ENERGY },
    { "获得魔法能量的药水", POT_GAIN_ENERGY },
    { "获得魔法能量之药水", POT_GAIN_ENERGY },
    { "魔法能量药水", POT_GAIN_ENERGY },
    { "魔法能量的药水", POT_GAIN_ENERGY },
    { "魔法能量之药水", POT_GAIN_ENERGY },
    { "沉睡药水", POT_SLEEPING },
    { "沉睡的药水", POT_SLEEPING },
    { "沉睡之药水", POT_SLEEPING },
    { "睡眠药水", POT_SLEEPING },
    { "睡眠的药水", POT_SLEEPING },
    { "睡眠之药水", POT_SLEEPING },
    { "变形药水", POT_POLYMORPH },
    { "变形的药水", POT_POLYMORPH },
    { "变形之药水", POT_POLYMORPH },
    { "酒", POT_BOOZE },
    { "烧酒", POT_BOOZE },
    { "烈酒", POT_BOOZE },
    { "酒药水", POT_BOOZE },
    { "酒的药水", POT_BOOZE },
    { "酒之药水", POT_BOOZE },
    { "疾病药水", POT_SICKNESS },
    { "疾病的药水", POT_SICKNESS },
    { "疾病之药水", POT_SICKNESS },
    { "生病药水", POT_SICKNESS },
    { "生病的药水", POT_SICKNESS },
    { "生病之药水", POT_SICKNESS },
    { "果汁", POT_FRUIT_JUICE },
    { "果汁药水", POT_FRUIT_JUICE },
    { "果汁的药水", POT_FRUIT_JUICE },
    { "果汁之药水", POT_FRUIT_JUICE },
    { "酸", POT_ACID },
    { "酸液", POT_ACID },
    { "酸药水", POT_ACID },
    { "酸的药水", POT_ACID },
    { "酸之药水", POT_ACID },
    { "油", POT_OIL },
    { "油药水", POT_OIL },
    { "油的药水", POT_OIL },
    { "油之药水", POT_OIL },
    { "水药水", POT_WATER },
    { "水的药水", POT_WATER },
    { "水之药水", POT_WATER },
    /*所有可能的卷轴*/
    { "防具附魔卷轴", SCR_ENCHANT_ARMOR },
    { "防具附魔的卷轴", SCR_ENCHANT_ARMOR },
    { "防具附魔之卷轴", SCR_ENCHANT_ARMOR },
    { "防具加强卷轴", SCR_ENCHANT_ARMOR },
    { "防具加强的卷轴", SCR_ENCHANT_ARMOR },
    { "防具加强之卷轴", SCR_ENCHANT_ARMOR },
    { "防具增强卷轴", SCR_ENCHANT_ARMOR },
    { "防具增强的卷轴", SCR_ENCHANT_ARMOR },
    { "防具增强之卷轴", SCR_ENCHANT_ARMOR },
    { "防具毁坏卷轴", SCR_DESTROY_ARMOR },
    { "防具毁坏的卷轴", SCR_DESTROY_ARMOR },
    { "防具毁坏之卷轴", SCR_DESTROY_ARMOR },
    { "防具破坏卷轴", SCR_DESTROY_ARMOR },
    { "防具破坏的卷轴", SCR_DESTROY_ARMOR },
    { "防具破坏之卷轴", SCR_DESTROY_ARMOR },
    { "盔甲附魔卷轴", SCR_ENCHANT_ARMOR },
    { "盔甲附魔的卷轴", SCR_ENCHANT_ARMOR },
    { "盔甲附魔之卷轴", SCR_ENCHANT_ARMOR },
    { "盔甲加强卷轴", SCR_ENCHANT_ARMOR },
    { "盔甲加强的卷轴", SCR_ENCHANT_ARMOR },
    { "盔甲加强之卷轴", SCR_ENCHANT_ARMOR },
    { "盔甲增强卷轴", SCR_ENCHANT_ARMOR },
    { "盔甲增强的卷轴", SCR_ENCHANT_ARMOR },
    { "盔甲增强之卷轴", SCR_ENCHANT_ARMOR },
    { "盔甲毁坏卷轴", SCR_DESTROY_ARMOR },
    { "盔甲毁坏的卷轴", SCR_DESTROY_ARMOR },
    { "盔甲毁坏之卷轴", SCR_DESTROY_ARMOR },
    { "盔甲破坏卷轴", SCR_DESTROY_ARMOR },
    { "盔甲破坏的卷轴", SCR_DESTROY_ARMOR },
    { "盔甲破坏之卷轴", SCR_DESTROY_ARMOR },
    { "护甲附魔卷轴", SCR_ENCHANT_ARMOR },
    { "护甲附魔的卷轴", SCR_ENCHANT_ARMOR },
    { "护甲附魔之卷轴", SCR_ENCHANT_ARMOR },
    { "护甲加强卷轴", SCR_ENCHANT_ARMOR },
    { "护甲加强的卷轴", SCR_ENCHANT_ARMOR },
    { "护甲加强之卷轴", SCR_ENCHANT_ARMOR },
    { "护甲增强卷轴", SCR_ENCHANT_ARMOR },
    { "护甲增强的卷轴", SCR_ENCHANT_ARMOR },
    { "护甲增强之卷轴", SCR_ENCHANT_ARMOR },
    { "护甲毁坏卷轴", SCR_DESTROY_ARMOR },
    { "护甲毁坏的卷轴", SCR_DESTROY_ARMOR },
    { "护甲毁坏之卷轴", SCR_DESTROY_ARMOR },
    { "护甲破坏卷轴", SCR_DESTROY_ARMOR },
    { "护甲破坏的卷轴", SCR_DESTROY_ARMOR },
    { "护甲破坏之卷轴", SCR_DESTROY_ARMOR },
    { "附魔防具卷轴", SCR_ENCHANT_ARMOR },
    { "附魔防具的卷轴", SCR_ENCHANT_ARMOR },
    { "附魔防具之卷轴", SCR_ENCHANT_ARMOR },
    { "加强防具卷轴", SCR_ENCHANT_ARMOR },
    { "加强防具的卷轴", SCR_ENCHANT_ARMOR },
    { "加强防具之卷轴", SCR_ENCHANT_ARMOR },
    { "增强防具卷轴", SCR_ENCHANT_ARMOR },
    { "增强防具的卷轴", SCR_ENCHANT_ARMOR },
    { "增强防具之卷轴", SCR_ENCHANT_ARMOR },
    { "毁坏防具卷轴", SCR_DESTROY_ARMOR },
    { "毁坏防具的卷轴", SCR_DESTROY_ARMOR },
    { "毁坏防具之卷轴", SCR_DESTROY_ARMOR },
    { "破坏防具卷轴", SCR_DESTROY_ARMOR },
    { "破坏防具的卷轴", SCR_DESTROY_ARMOR },
    { "破坏防具之卷轴", SCR_DESTROY_ARMOR },
    { "附魔盔甲卷轴", SCR_ENCHANT_ARMOR },
    { "附魔盔甲的卷轴", SCR_ENCHANT_ARMOR },
    { "附魔盔甲之卷轴", SCR_ENCHANT_ARMOR },
    { "加强盔甲卷轴", SCR_ENCHANT_ARMOR },
    { "加强盔甲的卷轴", SCR_ENCHANT_ARMOR },
    { "加强盔甲之卷轴", SCR_ENCHANT_ARMOR },
    { "增强盔甲卷轴", SCR_ENCHANT_ARMOR },
    { "增强盔甲的卷轴", SCR_ENCHANT_ARMOR },
    { "增强盔甲之卷轴", SCR_ENCHANT_ARMOR },
    { "毁坏盔甲卷轴", SCR_DESTROY_ARMOR },
    { "毁坏盔甲的卷轴", SCR_DESTROY_ARMOR },
    { "毁坏盔甲之卷轴", SCR_DESTROY_ARMOR },
    { "破坏盔甲卷轴", SCR_DESTROY_ARMOR },
    { "破坏盔甲的卷轴", SCR_DESTROY_ARMOR },
    { "破坏盔甲之卷轴", SCR_DESTROY_ARMOR },
    { "附魔护甲卷轴", SCR_ENCHANT_ARMOR },
    { "附魔护甲的卷轴", SCR_ENCHANT_ARMOR },
    { "附魔护甲之卷轴", SCR_ENCHANT_ARMOR },
    { "加强护甲卷轴", SCR_ENCHANT_ARMOR },
    { "加强护甲的卷轴", SCR_ENCHANT_ARMOR },
    { "加强护甲之卷轴", SCR_ENCHANT_ARMOR },
    { "增强护甲卷轴", SCR_ENCHANT_ARMOR },
    { "增强护甲的卷轴", SCR_ENCHANT_ARMOR },
    { "增强护甲之卷轴", SCR_ENCHANT_ARMOR },
    { "毁坏护甲卷轴", SCR_DESTROY_ARMOR },
    { "毁坏护甲的卷轴", SCR_DESTROY_ARMOR },
    { "毁坏护甲之卷轴", SCR_DESTROY_ARMOR },
    { "破坏护甲卷轴", SCR_DESTROY_ARMOR },
    { "破坏护甲的卷轴", SCR_DESTROY_ARMOR },
    { "破坏护甲之卷轴", SCR_DESTROY_ARMOR },
    { "混乱怪物卷轴", SCR_CONFUSE_MONSTER },
    { "混乱怪物的卷轴", SCR_CONFUSE_MONSTER },
    { "混乱怪物之卷轴", SCR_CONFUSE_MONSTER },
    { "迷惑怪物卷轴", SCR_CONFUSE_MONSTER },
    { "迷惑怪物的卷轴", SCR_CONFUSE_MONSTER },
    { "迷惑怪物之卷轴", SCR_CONFUSE_MONSTER },
    { "恐吓怪物卷轴", SCR_SCARE_MONSTER },
    { "恐吓怪物的卷轴", SCR_SCARE_MONSTER },
    { "恐吓怪物之卷轴", SCR_SCARE_MONSTER },
    { "吓唬怪物卷轴", SCR_SCARE_MONSTER },
    { "吓唬怪物的卷轴", SCR_SCARE_MONSTER },
    { "吓唬怪物之卷轴", SCR_SCARE_MONSTER },
    { "解除诅咒卷轴", SCR_REMOVE_CURSE },
    { "解除诅咒的卷轴", SCR_REMOVE_CURSE },
    { "解除诅咒之卷轴", SCR_REMOVE_CURSE },
    { "诅咒解除卷轴", SCR_REMOVE_CURSE },
    { "诅咒解除的卷轴", SCR_REMOVE_CURSE },
    { "诅咒解除之卷轴", SCR_REMOVE_CURSE },
    { "解咒卷轴", SCR_REMOVE_CURSE },
    { "解咒的卷轴", SCR_REMOVE_CURSE },
    { "解咒之卷轴", SCR_REMOVE_CURSE },
    { "武器附魔卷轴", SCR_ENCHANT_WEAPON },
    { "武器附魔的卷轴", SCR_ENCHANT_WEAPON },
    { "武器附魔之卷轴", SCR_ENCHANT_WEAPON },
    { "制造怪物卷轴", SCR_CREATE_MONSTER },
    { "制造怪物的卷轴", SCR_CREATE_MONSTER },
    { "制造怪物之卷轴", SCR_CREATE_MONSTER },
    { "生成怪物卷轴", SCR_CREATE_MONSTER },
    { "生成怪物的卷轴", SCR_CREATE_MONSTER },
    { "生成怪物之卷轴", SCR_CREATE_MONSTER },
    { "召唤怪物卷轴", SCR_CREATE_MONSTER },
    { "召唤怪物的卷轴", SCR_CREATE_MONSTER },
    { "召唤怪物之卷轴", SCR_CREATE_MONSTER },
    { "驯化卷轴", SCR_TAMING },
    { "驯化的卷轴", SCR_TAMING },
    { "驯化之卷轴", SCR_TAMING },
    { "驯服卷轴", SCR_TAMING },
    { "驯服的卷轴", SCR_TAMING },
    { "驯服之卷轴", SCR_TAMING },
    { "驯化怪物卷轴", SCR_TAMING },
    { "驯化怪物的卷轴", SCR_TAMING },
    { "驯化怪物之卷轴", SCR_TAMING },
    { "驯服怪物卷轴", SCR_TAMING },
    { "驯服怪物的卷轴", SCR_TAMING },
    { "驯服怪物之卷轴", SCR_TAMING },
    { "灭绝卷轴", SCR_GENOCIDE },
    { "灭绝的卷轴", SCR_GENOCIDE },
    { "灭绝之卷轴", SCR_GENOCIDE },
    { "光亮卷轴", SCR_LIGHT },
    { "光亮的卷轴", SCR_LIGHT },
    { "光亮之卷轴", SCR_LIGHT },
    { "亮光卷轴", SCR_LIGHT },
    { "亮光的卷轴", SCR_LIGHT },
    { "亮光之卷轴", SCR_LIGHT },
    { "照明卷轴", SCR_LIGHT },
    { "照明的卷轴", SCR_LIGHT },
    { "照明之卷轴", SCR_LIGHT },
    { "传送卷轴", SCR_TELEPORTATION },
    { "传送的卷轴", SCR_TELEPORTATION },
    { "传送之卷轴", SCR_TELEPORTATION },
    { "金钱探测卷轴", SCR_GOLD_DETECTION },
    { "金钱探测的卷轴", SCR_GOLD_DETECTION },
    { "金钱探测之卷轴", SCR_GOLD_DETECTION },
    { "黄金探测卷轴", SCR_GOLD_DETECTION },
    { "黄金探测的卷轴", SCR_GOLD_DETECTION },
    { "黄金探测之卷轴", SCR_GOLD_DETECTION },
    { "探测金钱卷轴", SCR_GOLD_DETECTION },
    { "探测金钱的卷轴", SCR_GOLD_DETECTION },
    { "探测金钱之卷轴", SCR_GOLD_DETECTION },
    { "探测黄金卷轴", SCR_GOLD_DETECTION },
    { "探测黄金的卷轴", SCR_GOLD_DETECTION },
    { "探测黄金之卷轴", SCR_GOLD_DETECTION },
    { "金钱发现卷轴", SCR_GOLD_DETECTION },
    { "金钱发现的卷轴", SCR_GOLD_DETECTION },
    { "金钱发现之卷轴", SCR_GOLD_DETECTION },
    { "黄金发现卷轴", SCR_GOLD_DETECTION },
    { "黄金发现的卷轴", SCR_GOLD_DETECTION },
    { "黄金发现之卷轴", SCR_GOLD_DETECTION },
    { "发现金钱卷轴", SCR_GOLD_DETECTION },
    { "发现金钱的卷轴", SCR_GOLD_DETECTION },
    { "发现金钱之卷轴", SCR_GOLD_DETECTION },
    { "发现黄金卷轴", SCR_GOLD_DETECTION },
    { "发现黄金的卷轴", SCR_GOLD_DETECTION },
    { "发现黄金之卷轴", SCR_GOLD_DETECTION },
    { "食物探测卷轴", SCR_FOOD_DETECTION },
    { "食物探测的卷轴", SCR_FOOD_DETECTION },
    { "食物探测之卷轴", SCR_FOOD_DETECTION },
    { "探测食物卷轴", SCR_FOOD_DETECTION },
    { "探测食物的卷轴", SCR_FOOD_DETECTION },
    { "探测食物之卷轴", SCR_FOOD_DETECTION },
    { "食物发现卷轴", SCR_FOOD_DETECTION },
    { "食物发现的卷轴", SCR_FOOD_DETECTION },
    { "食物发现之卷轴", SCR_FOOD_DETECTION },
    { "发现食物卷轴", SCR_FOOD_DETECTION },
    { "发现食物的卷轴", SCR_FOOD_DETECTION },
    { "发现食物之卷轴", SCR_FOOD_DETECTION },
    { "鉴定卷轴", SCR_IDENTIFY },
    { "鉴定的卷轴", SCR_IDENTIFY },
    { "鉴定之卷轴", SCR_IDENTIFY },
    { "魔法地图卷轴", SCR_MAGIC_MAPPING },
    { "魔法地图的卷轴", SCR_MAGIC_MAPPING },
    { "魔法地图之卷轴", SCR_MAGIC_MAPPING },
    { "地图卷轴", SCR_MAGIC_MAPPING },
    { "地图的卷轴", SCR_MAGIC_MAPPING },
    { "地图之卷轴", SCR_MAGIC_MAPPING },
    { "失忆卷轴", SCR_AMNESIA },
    { "失忆的卷轴", SCR_AMNESIA },
    { "失忆之卷轴", SCR_AMNESIA },
    { "健忘卷轴", SCR_AMNESIA },
    { "健忘的卷轴", SCR_AMNESIA },
    { "健忘之卷轴", SCR_AMNESIA },
    { "健忘症卷轴", SCR_AMNESIA },
    { "健忘症的卷轴", SCR_AMNESIA },
    { "健忘症之卷轴", SCR_AMNESIA },
    { "火卷轴", SCR_FIRE },
    { "火的卷轴", SCR_FIRE },
    { "火之卷轴", SCR_FIRE },
    { "大地卷轴", SCR_EARTH },
    { "大地的卷轴", SCR_EARTH },
    { "大地之卷轴", SCR_EARTH },
    { "地卷轴", SCR_EARTH },
    { "地的卷轴", SCR_EARTH },
    { "地之卷轴", SCR_EARTH },
    { "惩罚卷轴", SCR_PUNISHMENT },
    { "惩罚的卷轴", SCR_PUNISHMENT },
    { "惩罚之卷轴", SCR_PUNISHMENT },
    { "充能卷轴", SCR_CHARGING },
    { "充能的卷轴", SCR_CHARGING },
    { "充能之卷轴", SCR_CHARGING },
    { "臭云卷轴", SCR_STINKING_CLOUD },
    { "臭云的卷轴", SCR_STINKING_CLOUD },
    { "臭云之卷轴", SCR_STINKING_CLOUD },
    { "有邮戳的卷轴", SCR_MAIL },
    { "邮件卷轴", SCR_MAIL },
    { "邮件", SCR_MAIL },
    { "信", SCR_MAIL },
    { "纸", SCR_BLANK_PAPER },
    { "白纸", SCR_BLANK_PAPER },
    { "白纸卷轴", SCR_BLANK_PAPER },
    { "白纸的卷轴", SCR_BLANK_PAPER },
    { "白纸之卷轴", SCR_BLANK_PAPER },
    { "空白卷轴", SCR_BLANK_PAPER },
    { "空白的卷轴", SCR_BLANK_PAPER },
    { "空白之卷轴", SCR_BLANK_PAPER },
    { "无标签卷轴", SCR_BLANK_PAPER },
    { "无标签的卷轴", SCR_BLANK_PAPER },
    { "无标签之卷轴", SCR_BLANK_PAPER },
    /*所有可能的书*/
    { "挖掘书", SPE_DIG },
    { "挖掘的书", SPE_DIG },
    { "挖掘之书", SPE_DIG },
    { "挖掘魔法书", SPE_DIG },
    { "挖掘的魔法书", SPE_DIG },
    { "挖掘之魔法书", SPE_DIG },
    { "魔法飞弹书", SPE_MAGIC_MISSILE },
    { "魔法飞弹的书", SPE_MAGIC_MISSILE },
    { "魔法飞弹之书", SPE_MAGIC_MISSILE },
    { "魔法飞弹魔法书", SPE_MAGIC_MISSILE },
    { "魔法飞弹的魔法书", SPE_MAGIC_MISSILE },
    { "魔法飞弹之魔法书", SPE_MAGIC_MISSILE },
    { "火球书", SPE_FIREBALL },
    { "火球的书", SPE_FIREBALL },
    { "火球之书", SPE_FIREBALL },
    { "火球魔法书", SPE_FIREBALL },
    { "火球的魔法书", SPE_FIREBALL },
    { "火球之魔法书", SPE_FIREBALL },
    { "冰锥书", SPE_CONE_OF_COLD },
    { "冰锥的书", SPE_CONE_OF_COLD },
    { "冰锥之书", SPE_CONE_OF_COLD },
    { "冰锥魔法书", SPE_CONE_OF_COLD },
    { "冰锥的魔法书", SPE_CONE_OF_COLD },
    { "冰锥之魔法书", SPE_CONE_OF_COLD },
    { "沉睡书", SPE_SLEEP },
    { "沉睡的书", SPE_SLEEP },
    { "沉睡之书", SPE_SLEEP },
    { "沉睡魔法书", SPE_SLEEP },
    { "沉睡的魔法书", SPE_SLEEP },
    { "沉睡之魔法书", SPE_SLEEP },
    { "睡眠书", SPE_SLEEP },
    { "睡眠的书", SPE_SLEEP },
    { "睡眠之书", SPE_SLEEP },
    { "睡眠魔法书", SPE_SLEEP },
    { "睡眠的魔法书", SPE_SLEEP },
    { "睡眠之魔法书", SPE_SLEEP },
    { "死亡之指书", SPE_FINGER_OF_DEATH },
    { "死亡之指的书", SPE_FINGER_OF_DEATH },
    { "死亡之指之书", SPE_FINGER_OF_DEATH },
    { "死亡之指魔法书", SPE_FINGER_OF_DEATH },
    { "死亡之指的魔法书", SPE_FINGER_OF_DEATH },
    { "死亡之指之魔法书", SPE_FINGER_OF_DEATH },
    { "死亡一指书", SPE_FINGER_OF_DEATH },
    { "死亡一指的书", SPE_FINGER_OF_DEATH },
    { "死亡一指之书", SPE_FINGER_OF_DEATH },
    { "死亡一指魔法书", SPE_FINGER_OF_DEATH },
    { "死亡一指的魔法书", SPE_FINGER_OF_DEATH },
    { "死亡一指之魔法书", SPE_FINGER_OF_DEATH },
    { "光亮书", SPE_LIGHT },
    { "光亮的书", SPE_LIGHT },
    { "光亮之书", SPE_LIGHT },
    { "光亮魔法书", SPE_LIGHT },
    { "光亮的魔法书", SPE_LIGHT },
    { "光亮之魔法书", SPE_LIGHT },
    { "亮光书", SPE_LIGHT },
    { "亮光的书", SPE_LIGHT },
    { "亮光之书", SPE_LIGHT },
    { "亮光魔法书", SPE_LIGHT },
    { "亮光的魔法书", SPE_LIGHT },
    { "亮光之魔法书", SPE_LIGHT },
    { "照明书", SPE_LIGHT },
    { "照明的书", SPE_LIGHT },
    { "照明之书", SPE_LIGHT },
    { "照明魔法书", SPE_LIGHT },
    { "照明的魔法书", SPE_LIGHT },
    { "照明之魔法书", SPE_LIGHT },
    { "探测怪物书", SPE_DETECT_MONSTERS },
    { "探测怪物的书", SPE_DETECT_MONSTERS },
    { "探测怪物之书", SPE_DETECT_MONSTERS },
    { "探测怪物魔法书", SPE_DETECT_MONSTERS },
    { "探测怪物的魔法书", SPE_DETECT_MONSTERS },
    { "探测怪物之魔法书", SPE_DETECT_MONSTERS },
    { "怪物探测书", SPE_DETECT_MONSTERS },
    { "怪物探测的书", SPE_DETECT_MONSTERS },
    { "怪物探测之书", SPE_DETECT_MONSTERS },
    { "怪物探测魔法书", SPE_DETECT_MONSTERS },
    { "怪物探测的魔法书", SPE_DETECT_MONSTERS },
    { "怪物探测之魔法书", SPE_DETECT_MONSTERS },
    { "发现怪物书", SPE_DETECT_MONSTERS },
    { "发现怪物的书", SPE_DETECT_MONSTERS },
    { "发现怪物之书", SPE_DETECT_MONSTERS },
    { "发现怪物魔法书", SPE_DETECT_MONSTERS },
    { "发现怪物的魔法书", SPE_DETECT_MONSTERS },
    { "发现怪物之魔法书", SPE_DETECT_MONSTERS },
    { "怪物发现书", SPE_DETECT_MONSTERS },
    { "怪物发现的书", SPE_DETECT_MONSTERS },
    { "怪物发现之书", SPE_DETECT_MONSTERS },
    { "怪物发现魔法书", SPE_DETECT_MONSTERS },
    { "怪物发现的魔法书", SPE_DETECT_MONSTERS },
    { "怪物发现之魔法书", SPE_DETECT_MONSTERS },
    { "治愈书", SPE_HEALING },
    { "治愈的书", SPE_HEALING },
    { "治愈之书", SPE_HEALING },
    { "治愈魔法书", SPE_HEALING },
    { "治愈的魔法书", SPE_HEALING },
    { "治愈之魔法书", SPE_HEALING },
    { "治疗书", SPE_HEALING },
    { "治疗的书", SPE_HEALING },
    { "治疗之书", SPE_HEALING },
    { "治疗魔法书", SPE_HEALING },
    { "治疗的魔法书", SPE_HEALING },
    { "治疗之魔法书", SPE_HEALING },
    { "敲击书", SPE_KNOCK },
    { "敲击的书", SPE_KNOCK },
    { "敲击之书", SPE_KNOCK },
    { "敲击魔法书", SPE_KNOCK },
    { "敲击的魔法书", SPE_KNOCK },
    { "敲击之魔法书", SPE_KNOCK },
    { "力冲击书", SPE_FORCE_BOLT },
    { "力冲击的书", SPE_FORCE_BOLT },
    { "力冲击之书", SPE_FORCE_BOLT },
    { "力冲击魔法书", SPE_FORCE_BOLT },
    { "力冲击的魔法书", SPE_FORCE_BOLT },
    { "力冲击之魔法书", SPE_FORCE_BOLT },
    { "混乱怪物书", SPE_CONFUSE_MONSTER },
    { "混乱怪物的书", SPE_CONFUSE_MONSTER },
    { "混乱怪物之书", SPE_CONFUSE_MONSTER },
    { "混乱怪物魔法书", SPE_CONFUSE_MONSTER },
    { "混乱怪物的魔法书", SPE_CONFUSE_MONSTER },
    { "混乱怪物之魔法书", SPE_CONFUSE_MONSTER },
    { "迷惑怪物书", SPE_CONFUSE_MONSTER },
    { "迷惑怪物的书", SPE_CONFUSE_MONSTER },
    { "迷惑怪物之书", SPE_CONFUSE_MONSTER },
    { "迷惑怪物魔法书", SPE_CONFUSE_MONSTER },
    { "迷惑怪物的魔法书", SPE_CONFUSE_MONSTER },
    { "迷惑怪物之魔法书", SPE_CONFUSE_MONSTER },
    { "治疗失明书", SPE_CURE_BLINDNESS },
    { "治疗失明的书", SPE_CURE_BLINDNESS },
    { "治疗失明之书", SPE_CURE_BLINDNESS },
    { "治疗失明魔法书", SPE_CURE_BLINDNESS },
    { "治疗失明的魔法书", SPE_CURE_BLINDNESS },
    { "治疗失明之魔法书", SPE_CURE_BLINDNESS },
    { "吸血书", SPE_DRAIN_LIFE },
    { "吸血的书", SPE_DRAIN_LIFE },
    { "吸血之书", SPE_DRAIN_LIFE },
    { "吸血魔法书", SPE_DRAIN_LIFE },
    { "吸血的魔法书", SPE_DRAIN_LIFE },
    { "吸血之魔法书", SPE_DRAIN_LIFE },
    { "生命吸取书", SPE_DRAIN_LIFE },
    { "生命吸取的书", SPE_DRAIN_LIFE },
    { "生命吸取之书", SPE_DRAIN_LIFE },
    { "生命吸取魔法书", SPE_DRAIN_LIFE },
    { "生命吸取的魔法书", SPE_DRAIN_LIFE },
    { "生命吸取之魔法书", SPE_DRAIN_LIFE },
    { "生命汲取书", SPE_DRAIN_LIFE },
    { "生命汲取的书", SPE_DRAIN_LIFE },
    { "生命汲取之书", SPE_DRAIN_LIFE },
    { "生命汲取魔法书", SPE_DRAIN_LIFE },
    { "生命汲取的魔法书", SPE_DRAIN_LIFE },
    { "生命汲取之魔法书", SPE_DRAIN_LIFE },
    { "减慢怪物书", SPE_SLOW_MONSTER },
    { "减慢怪物的书", SPE_SLOW_MONSTER },
    { "减慢怪物之书", SPE_SLOW_MONSTER },
    { "减慢怪物魔法书", SPE_SLOW_MONSTER },
    { "减慢怪物的魔法书", SPE_SLOW_MONSTER },
    { "减慢怪物之魔法书", SPE_SLOW_MONSTER },
    { "减速怪物书", SPE_SLOW_MONSTER },
    { "减速怪物的书", SPE_SLOW_MONSTER },
    { "减速怪物之书", SPE_SLOW_MONSTER },
    { "减速怪物魔法书", SPE_SLOW_MONSTER },
    { "减速怪物的魔法书", SPE_SLOW_MONSTER },
    { "减速怪物之魔法书", SPE_SLOW_MONSTER },
    { "巫师锁书", SPE_WIZARD_LOCK },
    { "巫师锁的书", SPE_WIZARD_LOCK },
    { "巫师锁之书", SPE_WIZARD_LOCK },
    { "巫师锁魔法书", SPE_WIZARD_LOCK },
    { "巫师锁的魔法书", SPE_WIZARD_LOCK },
    { "巫师锁之魔法书", SPE_WIZARD_LOCK },
    { "制造怪物书", SPE_CREATE_MONSTER },
    { "制造怪物的书", SPE_CREATE_MONSTER },
    { "制造怪物之书", SPE_CREATE_MONSTER },
    { "制造怪物魔法书", SPE_CREATE_MONSTER },
    { "制造怪物的魔法书", SPE_CREATE_MONSTER },
    { "制造怪物之魔法书", SPE_CREATE_MONSTER },
    { "生成怪物书", SPE_CREATE_MONSTER },
    { "生成怪物的书", SPE_CREATE_MONSTER },
    { "生成怪物之书", SPE_CREATE_MONSTER },
    { "生成怪物魔法书", SPE_CREATE_MONSTER },
    { "生成怪物的魔法书", SPE_CREATE_MONSTER },
    { "生成怪物之魔法书", SPE_CREATE_MONSTER },
    { "召唤怪物书", SPE_CREATE_MONSTER },
    { "召唤怪物的书", SPE_CREATE_MONSTER },
    { "召唤怪物之书", SPE_CREATE_MONSTER },
    { "召唤怪物魔法书", SPE_CREATE_MONSTER },
    { "召唤怪物的魔法书", SPE_CREATE_MONSTER },
    { "召唤怪物之魔法书", SPE_CREATE_MONSTER },
    { "怪物制造书", SPE_CREATE_MONSTER },
    { "怪物制造的书", SPE_CREATE_MONSTER },
    { "怪物制造之书", SPE_CREATE_MONSTER },
    { "怪物制造魔法书", SPE_CREATE_MONSTER },
    { "怪物制造的魔法书", SPE_CREATE_MONSTER },
    { "怪物制造之魔法书", SPE_CREATE_MONSTER },
    { "怪物生成书", SPE_CREATE_MONSTER },
    { "怪物生成的书", SPE_CREATE_MONSTER },
    { "怪物生成之书", SPE_CREATE_MONSTER },
    { "怪物生成魔法书", SPE_CREATE_MONSTER },
    { "怪物生成的魔法书", SPE_CREATE_MONSTER },
    { "怪物生成之魔法书", SPE_CREATE_MONSTER },
    { "怪物召唤书", SPE_CREATE_MONSTER },
    { "怪物召唤的书", SPE_CREATE_MONSTER },
    { "怪物召唤之书", SPE_CREATE_MONSTER },
    { "怪物召唤魔法书", SPE_CREATE_MONSTER },
    { "怪物召唤的魔法书", SPE_CREATE_MONSTER },
    { "怪物召唤之魔法书", SPE_CREATE_MONSTER },
    { "探测食物书", SPE_DETECT_FOOD },
    { "探测食物的书", SPE_DETECT_FOOD },
    { "探测食物之书", SPE_DETECT_FOOD },
    { "探测食物魔法书", SPE_DETECT_FOOD },
    { "探测食物的魔法书", SPE_DETECT_FOOD },
    { "探测食物之魔法书", SPE_DETECT_FOOD },
    { "发现食物书", SPE_DETECT_FOOD },
    { "发现食物的书", SPE_DETECT_FOOD },
    { "发现食物之书", SPE_DETECT_FOOD },
    { "发现食物魔法书", SPE_DETECT_FOOD },
    { "发现食物的魔法书", SPE_DETECT_FOOD },
    { "发现食物之魔法书", SPE_DETECT_FOOD },
    { "食物探测书", SPE_DETECT_FOOD },
    { "食物探测的书", SPE_DETECT_FOOD },
    { "食物探测之书", SPE_DETECT_FOOD },
    { "食物探测魔法书", SPE_DETECT_FOOD },
    { "食物探测的魔法书", SPE_DETECT_FOOD },
    { "食物探测之魔法书", SPE_DETECT_FOOD },
    { "食物发现书", SPE_DETECT_FOOD },
    { "食物发现的书", SPE_DETECT_FOOD },
    { "食物发现之书", SPE_DETECT_FOOD },
    { "食物发现魔法书", SPE_DETECT_FOOD },
    { "食物发现的魔法书", SPE_DETECT_FOOD },
    { "食物发现之魔法书", SPE_DETECT_FOOD },
    { "造成恐惧书", SPE_CAUSE_FEAR },
    { "造成恐惧的书", SPE_CAUSE_FEAR },
    { "造成恐惧之书", SPE_CAUSE_FEAR },
    { "造成恐惧魔法书", SPE_CAUSE_FEAR },
    { "造成恐惧的魔法书", SPE_CAUSE_FEAR },
    { "造成恐惧之魔法书", SPE_CAUSE_FEAR },
    { "千里眼书", SPE_CLAIRVOYANCE },
    { "千里眼的书", SPE_CLAIRVOYANCE },
    { "千里眼之书", SPE_CLAIRVOYANCE },
    { "千里眼魔法书", SPE_CLAIRVOYANCE },
    { "千里眼的魔法书", SPE_CLAIRVOYANCE },
    { "千里眼之魔法书", SPE_CLAIRVOYANCE },
    { "鹰眼术书", SPE_CLAIRVOYANCE },
    { "鹰眼术的书", SPE_CLAIRVOYANCE },
    { "鹰眼术之书", SPE_CLAIRVOYANCE },
    { "鹰眼术魔法书", SPE_CLAIRVOYANCE },
    { "鹰眼术的魔法书", SPE_CLAIRVOYANCE },
    { "鹰眼术之魔法书", SPE_CLAIRVOYANCE },
    { "超视书", SPE_CLAIRVOYANCE },
    { "超视的书", SPE_CLAIRVOYANCE },
    { "超视之书", SPE_CLAIRVOYANCE },
    { "超视魔法书", SPE_CLAIRVOYANCE },
    { "超视的魔法书", SPE_CLAIRVOYANCE },
    { "超视之魔法书", SPE_CLAIRVOYANCE },
    { "治疗疾病书", SPE_CURE_SICKNESS },
    { "治疗疾病的书", SPE_CURE_SICKNESS },
    { "治疗疾病之书", SPE_CURE_SICKNESS },
    { "治疗疾病魔法书", SPE_CURE_SICKNESS },
    { "治疗疾病的魔法书", SPE_CURE_SICKNESS },
    { "治疗疾病之魔法书", SPE_CURE_SICKNESS },
    { "魅惑怪物书", SPE_CHARM_MONSTER },
    { "魅惑怪物的书", SPE_CHARM_MONSTER },
    { "魅惑怪物之书", SPE_CHARM_MONSTER },
    { "魅惑怪物魔法书", SPE_CHARM_MONSTER },
    { "魅惑怪物的魔法书", SPE_CHARM_MONSTER },
    { "魅惑怪物之魔法书", SPE_CHARM_MONSTER },
    { "自我加速书", SPE_HASTE_SELF },
    { "自我加速的书", SPE_HASTE_SELF },
    { "自我加速之书", SPE_HASTE_SELF },
    { "自我加速魔法书", SPE_HASTE_SELF },
    { "自我加速的魔法书", SPE_HASTE_SELF },
    { "自我加速之魔法书", SPE_HASTE_SELF },
    { "探测隐形书", SPE_DETECT_UNSEEN },
    { "探测隐形的书", SPE_DETECT_UNSEEN },
    { "探测隐形之书", SPE_DETECT_UNSEEN },
    { "探测隐形魔法书", SPE_DETECT_UNSEEN },
    { "探测隐形的魔法书", SPE_DETECT_UNSEEN },
    { "探测隐形之魔法书", SPE_DETECT_UNSEEN },
    { "探测隐身书", SPE_DETECT_UNSEEN },
    { "探测隐身的书", SPE_DETECT_UNSEEN },
    { "探测隐身之书", SPE_DETECT_UNSEEN },
    { "探测隐身魔法书", SPE_DETECT_UNSEEN },
    { "探测隐身的魔法书", SPE_DETECT_UNSEEN },
    { "探测隐身之魔法书", SPE_DETECT_UNSEEN },
    { "发现隐形书", SPE_DETECT_UNSEEN },
    { "发现隐形的书", SPE_DETECT_UNSEEN },
    { "发现隐形之书", SPE_DETECT_UNSEEN },
    { "发现隐形魔法书", SPE_DETECT_UNSEEN },
    { "发现隐形的魔法书", SPE_DETECT_UNSEEN },
    { "发现隐形之魔法书", SPE_DETECT_UNSEEN },
    { "发现隐身书", SPE_DETECT_UNSEEN },
    { "发现隐身的书", SPE_DETECT_UNSEEN },
    { "发现隐身之书", SPE_DETECT_UNSEEN },
    { "发现隐身魔法书", SPE_DETECT_UNSEEN },
    { "发现隐身的魔法书", SPE_DETECT_UNSEEN },
    { "发现隐身之魔法书", SPE_DETECT_UNSEEN },
    { "飘浮书", SPE_LEVITATION },
    { "飘浮的书", SPE_LEVITATION },
    { "飘浮之书", SPE_LEVITATION },
    { "飘浮魔法书", SPE_LEVITATION },
    { "飘浮的魔法书", SPE_LEVITATION },
    { "飘浮之魔法书", SPE_LEVITATION },
    { "悬浮书", SPE_LEVITATION },
    { "悬浮的书", SPE_LEVITATION },
    { "悬浮之书", SPE_LEVITATION },
    { "悬浮魔法书", SPE_LEVITATION },
    { "悬浮的魔法书", SPE_LEVITATION },
    { "悬浮之魔法书", SPE_LEVITATION },
    { "强力治愈书", SPE_EXTRA_HEALING },
    { "强力治愈的书", SPE_EXTRA_HEALING },
    { "强力治愈之书", SPE_EXTRA_HEALING },
    { "强力治愈魔法书", SPE_EXTRA_HEALING },
    { "强力治愈的魔法书", SPE_EXTRA_HEALING },
    { "强力治愈之魔法书", SPE_EXTRA_HEALING },
    { "强力治疗书", SPE_EXTRA_HEALING },
    { "强力治疗的书", SPE_EXTRA_HEALING },
    { "强力治疗之书", SPE_EXTRA_HEALING },
    { "强力治疗魔法书", SPE_EXTRA_HEALING },
    { "强力治疗的魔法书", SPE_EXTRA_HEALING },
    { "强力治疗之魔法书", SPE_EXTRA_HEALING },
    { "恢复能力书", SPE_RESTORE_ABILITY },
    { "恢复能力的书", SPE_RESTORE_ABILITY },
    { "恢复能力之书", SPE_RESTORE_ABILITY },
    { "恢复能力魔法书", SPE_RESTORE_ABILITY },
    { "恢复能力的魔法书", SPE_RESTORE_ABILITY },
    { "恢复能力之魔法书", SPE_RESTORE_ABILITY },
    { "隐身书", SPE_INVISIBILITY },
    { "隐身的书", SPE_INVISIBILITY },
    { "隐身之书", SPE_INVISIBILITY },
    { "隐身魔法书", SPE_INVISIBILITY },
    { "隐身的魔法书", SPE_INVISIBILITY },
    { "隐身之魔法书", SPE_INVISIBILITY },
    { "隐形书", SPE_INVISIBILITY },
    { "隐形的书", SPE_INVISIBILITY },
    { "隐形之书", SPE_INVISIBILITY },
    { "隐形魔法书", SPE_INVISIBILITY },
    { "隐形的魔法书", SPE_INVISIBILITY },
    { "隐形之魔法书", SPE_INVISIBILITY },
    { "探测宝藏书", SPE_DETECT_TREASURE },
    { "探测宝藏的书", SPE_DETECT_TREASURE },
    { "探测宝藏之书", SPE_DETECT_TREASURE },
    { "探测宝藏魔法书", SPE_DETECT_TREASURE },
    { "探测宝藏的魔法书", SPE_DETECT_TREASURE },
    { "探测宝藏之魔法书", SPE_DETECT_TREASURE },
    { "发现宝藏书", SPE_DETECT_TREASURE },
    { "发现宝藏的书", SPE_DETECT_TREASURE },
    { "发现宝藏之书", SPE_DETECT_TREASURE },
    { "发现宝藏魔法书", SPE_DETECT_TREASURE },
    { "发现宝藏的魔法书", SPE_DETECT_TREASURE },
    { "发现宝藏之魔法书", SPE_DETECT_TREASURE },
    { "宝藏探测书", SPE_DETECT_TREASURE },
    { "宝藏探测的书", SPE_DETECT_TREASURE },
    { "宝藏探测之书", SPE_DETECT_TREASURE },
    { "宝藏探测魔法书", SPE_DETECT_TREASURE },
    { "宝藏探测的魔法书", SPE_DETECT_TREASURE },
    { "宝藏探测之魔法书", SPE_DETECT_TREASURE },
    { "宝藏发现书", SPE_DETECT_TREASURE },
    { "宝藏发现的书", SPE_DETECT_TREASURE },
    { "宝藏发现之书", SPE_DETECT_TREASURE },
    { "宝藏发现魔法书", SPE_DETECT_TREASURE },
    { "宝藏发现的魔法书", SPE_DETECT_TREASURE },
    { "宝藏发现之魔法书", SPE_DETECT_TREASURE },
    { "解除诅咒书", SPE_REMOVE_CURSE },
    { "解除诅咒的书", SPE_REMOVE_CURSE },
    { "解除诅咒之书", SPE_REMOVE_CURSE },
    { "解除诅咒魔法书", SPE_REMOVE_CURSE },
    { "解除诅咒的魔法书", SPE_REMOVE_CURSE },
    { "解除诅咒之魔法书", SPE_REMOVE_CURSE },
    { "诅咒解除书", SPE_REMOVE_CURSE },
    { "诅咒解除的书", SPE_REMOVE_CURSE },
    { "诅咒解除之书", SPE_REMOVE_CURSE },
    { "诅咒解除魔法书", SPE_REMOVE_CURSE },
    { "诅咒解除的魔法书", SPE_REMOVE_CURSE },
    { "诅咒解除之魔法书", SPE_REMOVE_CURSE },
    { "解咒书", SPE_REMOVE_CURSE },
    { "解咒的书", SPE_REMOVE_CURSE },
    { "解咒之书", SPE_REMOVE_CURSE },
    { "解咒魔法书", SPE_REMOVE_CURSE },
    { "解咒的魔法书", SPE_REMOVE_CURSE },
    { "解咒之魔法书", SPE_REMOVE_CURSE },
    { "魔法地图书", SPE_MAGIC_MAPPING },
    { "魔法地图的书", SPE_MAGIC_MAPPING },
    { "魔法地图之书", SPE_MAGIC_MAPPING },
    { "魔法地图魔法书", SPE_MAGIC_MAPPING },
    { "魔法地图的魔法书", SPE_MAGIC_MAPPING },
    { "魔法地图之魔法书", SPE_MAGIC_MAPPING },
    { "鉴定书", SPE_IDENTIFY },
    { "鉴定的书", SPE_IDENTIFY },
    { "鉴定之书", SPE_IDENTIFY },
    { "鉴定魔法书", SPE_IDENTIFY },
    { "鉴定的魔法书", SPE_IDENTIFY },
    { "鉴定之魔法书", SPE_IDENTIFY },
    { "超度书", SPE_TURN_UNDEAD },
    { "超度的书", SPE_TURN_UNDEAD },
    { "超度之书", SPE_TURN_UNDEAD },
    { "超度魔法书", SPE_TURN_UNDEAD },
    { "超度的魔法书", SPE_TURN_UNDEAD },
    { "超度之魔法书", SPE_TURN_UNDEAD },
    { "驱赶亡灵书", SPE_TURN_UNDEAD },
    { "驱赶亡灵的书", SPE_TURN_UNDEAD },
    { "驱赶亡灵之书", SPE_TURN_UNDEAD },
    { "驱赶亡灵魔法书", SPE_TURN_UNDEAD },
    { "驱赶亡灵的魔法书", SPE_TURN_UNDEAD },
    { "驱赶亡灵之魔法书", SPE_TURN_UNDEAD },
    { "驱散亡灵书", SPE_TURN_UNDEAD },
    { "驱散亡灵的书", SPE_TURN_UNDEAD },
    { "驱散亡灵之书", SPE_TURN_UNDEAD },
    { "驱散亡灵魔法书", SPE_TURN_UNDEAD },
    { "驱散亡灵的魔法书", SPE_TURN_UNDEAD },
    { "驱散亡灵之魔法书", SPE_TURN_UNDEAD },
    { "驱赶亡者书", SPE_TURN_UNDEAD },
    { "驱赶亡者的书", SPE_TURN_UNDEAD },
    { "驱赶亡者之书", SPE_TURN_UNDEAD },
    { "驱赶亡者魔法书", SPE_TURN_UNDEAD },
    { "驱赶亡者的魔法书", SPE_TURN_UNDEAD },
    { "驱赶亡者之魔法书", SPE_TURN_UNDEAD },
    { "驱散亡者书", SPE_TURN_UNDEAD },
    { "驱散亡者的书", SPE_TURN_UNDEAD },
    { "驱散亡者之书", SPE_TURN_UNDEAD },
    { "驱散亡者魔法书", SPE_TURN_UNDEAD },
    { "驱散亡者的魔法书", SPE_TURN_UNDEAD },
    { "驱散亡者之魔法书", SPE_TURN_UNDEAD },
    { "驱赶不死生物书", SPE_TURN_UNDEAD },
    { "驱赶不死生物的书", SPE_TURN_UNDEAD },
    { "驱赶不死生物之书", SPE_TURN_UNDEAD },
    { "驱赶不死生物魔法书", SPE_TURN_UNDEAD },
    { "驱赶不死生物的魔法书", SPE_TURN_UNDEAD },
    { "驱赶不死生物之魔法书", SPE_TURN_UNDEAD },
    { "驱散不死生物书", SPE_TURN_UNDEAD },
    { "驱散不死生物的书", SPE_TURN_UNDEAD },
    { "驱散不死生物之书", SPE_TURN_UNDEAD },
    { "驱散不死生物魔法书", SPE_TURN_UNDEAD },
    { "驱散不死生物的魔法书", SPE_TURN_UNDEAD },
    { "驱散不死生物之魔法书", SPE_TURN_UNDEAD },
    { "变形书", SPE_POLYMORPH },
    { "变形的书", SPE_POLYMORPH },
    { "变形之书", SPE_POLYMORPH },
    { "变形魔法书", SPE_POLYMORPH },
    { "变形的魔法书", SPE_POLYMORPH },
    { "变形之魔法书", SPE_POLYMORPH },
    { "传送书", SPE_TELEPORT_AWAY },
    { "传送的书", SPE_TELEPORT_AWAY },
    { "传送之书", SPE_TELEPORT_AWAY },
    { "传送魔法书", SPE_TELEPORT_AWAY },
    { "传送的魔法书", SPE_TELEPORT_AWAY },
    { "传送之魔法书", SPE_TELEPORT_AWAY },
    { "生成宠物书", SPE_CREATE_FAMILIAR },
    { "生成宠物的书", SPE_CREATE_FAMILIAR },
    { "生成宠物之书", SPE_CREATE_FAMILIAR },
    { "生成宠物魔法书", SPE_CREATE_FAMILIAR },
    { "生成宠物的魔法书", SPE_CREATE_FAMILIAR },
    { "生成宠物之魔法书", SPE_CREATE_FAMILIAR },
    { "消除书", SPE_CANCELLATION },
    { "消除的书", SPE_CANCELLATION },
    { "消除之书", SPE_CANCELLATION },
    { "消除魔法书", SPE_CANCELLATION },
    { "消除的魔法书", SPE_CANCELLATION },
    { "消除之魔法书", SPE_CANCELLATION },
    { "取消书", SPE_CANCELLATION },
    { "取消的书", SPE_CANCELLATION },
    { "取消之书", SPE_CANCELLATION },
    { "取消魔法书", SPE_CANCELLATION },
    { "取消的魔法书", SPE_CANCELLATION },
    { "取消之魔法书", SPE_CANCELLATION },
    { "保护书", SPE_PROTECTION },
    { "保护的书", SPE_PROTECTION },
    { "保护之书", SPE_PROTECTION },
    { "保护魔法书", SPE_PROTECTION },
    { "保护的魔法书", SPE_PROTECTION },
    { "保护之魔法书", SPE_PROTECTION },
    { "跳跃书", SPE_JUMPING },
    { "跳跃的书", SPE_JUMPING },
    { "跳跃之书", SPE_JUMPING },
    { "跳跃魔法书", SPE_JUMPING },
    { "跳跃的魔法书", SPE_JUMPING },
    { "跳跃之魔法书", SPE_JUMPING },
    { "点石成肉书", SPE_STONE_TO_FLESH },
    { "点石成肉的书", SPE_STONE_TO_FLESH },
    { "点石成肉之书", SPE_STONE_TO_FLESH },
    { "点石成肉魔法书", SPE_STONE_TO_FLESH },
    { "点石成肉的魔法书", SPE_STONE_TO_FLESH },
    { "点石成肉之魔法书", SPE_STONE_TO_FLESH },
    { "连锁闪电书", SPE_CHAIN_LIGHTNING },
    { "连锁闪电的书", SPE_CHAIN_LIGHTNING },
    { "连锁闪电之书", SPE_CHAIN_LIGHTNING },
    { "连锁闪电魔法书", SPE_CHAIN_LIGHTNING },
    { "连锁闪电的魔法书", SPE_CHAIN_LIGHTNING },
    { "连锁闪电之魔法书", SPE_CHAIN_LIGHTNING },
    { "白纸书", HI_PAPER },
    { "白纸的书", HI_PAPER },
    { "白纸之书", HI_PAPER },
    { "白纸魔法书", HI_PAPER },
    { "白纸的魔法书", HI_PAPER },
    { "白纸之魔法书", HI_PAPER },
    { "小说", SPE_NOVEL},
    { "死亡之书", SPE_BOOK_OF_THE_DEAD},
    { "死亡的书", SPE_BOOK_OF_THE_DEAD},
    { "死灵之书", SPE_BOOK_OF_THE_DEAD},
    { "死灵的书", SPE_BOOK_OF_THE_DEAD},
    { "莎草魔法书", SPE_BOOK_OF_THE_DEAD},
    { "纸莎草魔法书", SPE_BOOK_OF_THE_DEAD},
    { "莎草纸魔法书", SPE_BOOK_OF_THE_DEAD},
    { "纸莎草纸魔法书", SPE_BOOK_OF_THE_DEAD},
    /*所有可能的魔杖*/
    { "光亮魔杖", WAN_LIGHT },
    { "光亮的魔杖", WAN_LIGHT },
    { "光亮之魔杖", WAN_LIGHT },
    { "光亮之杖", WAN_LIGHT },
    { "亮光魔杖", WAN_LIGHT },
    { "亮光的魔杖", WAN_LIGHT },
    { "亮光之魔杖", WAN_LIGHT },
    { "亮光之杖", WAN_LIGHT },
    { "照明魔杖", WAN_LIGHT },
    { "照明的魔杖", WAN_LIGHT },
    { "照明之魔杖", WAN_LIGHT },
    { "照明之杖", WAN_LIGHT },
    { "暗门探测魔杖", WAN_SECRET_DOOR_DETECTION },
    { "暗门探测的魔杖", WAN_SECRET_DOOR_DETECTION },
    { "暗门探测之魔杖", WAN_SECRET_DOOR_DETECTION },
    { "暗门探测之杖", WAN_SECRET_DOOR_DETECTION },
    { "暗门发现魔杖", WAN_SECRET_DOOR_DETECTION },
    { "暗门发现的魔杖", WAN_SECRET_DOOR_DETECTION },
    { "暗门发现之魔杖", WAN_SECRET_DOOR_DETECTION },
    { "暗门发现之杖", WAN_SECRET_DOOR_DETECTION },
    { "探测暗门魔杖", WAN_SECRET_DOOR_DETECTION },
    { "探测暗门的魔杖", WAN_SECRET_DOOR_DETECTION },
    { "探测暗门之魔杖", WAN_SECRET_DOOR_DETECTION },
    { "探测暗门之杖", WAN_SECRET_DOOR_DETECTION },
    { "发现暗门魔杖", WAN_SECRET_DOOR_DETECTION },
    { "发现暗门的魔杖", WAN_SECRET_DOOR_DETECTION },
    { "发现暗门之魔杖", WAN_SECRET_DOOR_DETECTION },
    { "发现暗门之杖", WAN_SECRET_DOOR_DETECTION },
    { "隐藏门探测魔杖", WAN_SECRET_DOOR_DETECTION },
    { "隐藏门探测的魔杖", WAN_SECRET_DOOR_DETECTION },
    { "隐藏门探测之魔杖", WAN_SECRET_DOOR_DETECTION },
    { "隐藏门探测之杖", WAN_SECRET_DOOR_DETECTION },
    { "隐藏门发现魔杖", WAN_SECRET_DOOR_DETECTION },
    { "隐藏门发现的魔杖", WAN_SECRET_DOOR_DETECTION },
    { "隐藏门发现之魔杖", WAN_SECRET_DOOR_DETECTION },
    { "隐藏门发现之杖", WAN_SECRET_DOOR_DETECTION },
    { "探测隐藏门魔杖", WAN_SECRET_DOOR_DETECTION },
    { "探测隐藏门的魔杖", WAN_SECRET_DOOR_DETECTION },
    { "探测隐藏门之魔杖", WAN_SECRET_DOOR_DETECTION },
    { "探测隐藏门之杖", WAN_SECRET_DOOR_DETECTION },
    { "发现隐藏门魔杖", WAN_SECRET_DOOR_DETECTION },
    { "发现隐藏门的魔杖", WAN_SECRET_DOOR_DETECTION },
    { "发现隐藏门之魔杖", WAN_SECRET_DOOR_DETECTION },
    { "发现隐藏门之杖", WAN_SECRET_DOOR_DETECTION },
    { "启蒙魔杖", WAN_ENLIGHTENMENT },
    { "启蒙的魔杖", WAN_ENLIGHTENMENT },
    { "启蒙之魔杖", WAN_ENLIGHTENMENT },
    { "启蒙之杖", WAN_ENLIGHTENMENT },
    { "自知魔杖", WAN_ENLIGHTENMENT },
    { "自知的魔杖", WAN_ENLIGHTENMENT },
    { "自知之魔杖", WAN_ENLIGHTENMENT },
    { "自知之杖", WAN_ENLIGHTENMENT },
    { "了解自身魔杖", WAN_ENLIGHTENMENT },
    { "了解自身的魔杖", WAN_ENLIGHTENMENT },
    { "了解自身之魔杖", WAN_ENLIGHTENMENT },
    { "了解自身之杖", WAN_ENLIGHTENMENT },
    { "了解自己魔杖", WAN_ENLIGHTENMENT },
    { "了解自己的魔杖", WAN_ENLIGHTENMENT },
    { "了解自己之魔杖", WAN_ENLIGHTENMENT },
    { "了解自己之杖", WAN_ENLIGHTENMENT },
    { "制造怪物魔杖", WAN_CREATE_MONSTER },
    { "制造怪物的魔杖", WAN_CREATE_MONSTER },
    { "制造怪物之魔杖", WAN_CREATE_MONSTER },
    { "制造怪物之杖", WAN_CREATE_MONSTER },
    { "生成怪物魔杖", WAN_CREATE_MONSTER },
    { "生成怪物的魔杖", WAN_CREATE_MONSTER },
    { "生成怪物之魔杖", WAN_CREATE_MONSTER },
    { "生成怪物之杖", WAN_CREATE_MONSTER },
    { "召唤怪物魔杖", WAN_CREATE_MONSTER },
    { "召唤怪物的魔杖", WAN_CREATE_MONSTER },
    { "召唤怪物之魔杖", WAN_CREATE_MONSTER },
    { "召唤怪物之杖", WAN_CREATE_MONSTER },
    { "怪物制造魔杖", WAN_CREATE_MONSTER },
    { "怪物制造的魔杖", WAN_CREATE_MONSTER },
    { "怪物制造之魔杖", WAN_CREATE_MONSTER },
    { "怪物制造之杖", WAN_CREATE_MONSTER },
    { "怪物生成魔杖", WAN_CREATE_MONSTER },
    { "怪物生成的魔杖", WAN_CREATE_MONSTER },
    { "怪物生成之魔杖", WAN_CREATE_MONSTER },
    { "怪物生成之杖", WAN_CREATE_MONSTER },
    { "怪物召唤魔杖", WAN_CREATE_MONSTER },
    { "怪物召唤的魔杖", WAN_CREATE_MONSTER },
    { "怪物召唤之魔杖", WAN_CREATE_MONSTER },
    { "怪物召唤之杖", WAN_CREATE_MONSTER },
    { "许愿魔杖", WAN_WISHING },
    { "许愿的魔杖", WAN_WISHING },
    { "许愿之魔杖", WAN_WISHING },
    { "许愿之杖", WAN_WISHING },
    { "停滞魔杖", WAN_STASIS },
    { "停滞的魔杖", WAN_STASIS },
    { "停滞之魔杖", WAN_STASIS },
    { "停滞之杖", WAN_STASIS },
    { "无魔杖", WAN_NOTHING },
    { "无的魔杖", WAN_NOTHING },
    { "无之魔杖", WAN_NOTHING },
    { "无之杖", WAN_NOTHING },
    { "冲击魔杖", WAN_STRIKING },
    { "冲击的魔杖", WAN_STRIKING },
    { "冲击之魔杖", WAN_STRIKING },
    { "冲击之杖", WAN_STRIKING },
    { "冲击波杖", WAN_STRIKING },
    { "冲击波魔杖", WAN_STRIKING },
    { "冲击波魔杖", WAN_STRIKING },
    { "冲击波杖", WAN_STRIKING },
    { "隐身魔杖", WAN_MAKE_INVISIBLE },
    { "隐身的魔杖", WAN_MAKE_INVISIBLE },
    { "隐身之魔杖", WAN_MAKE_INVISIBLE },
    { "隐身之杖", WAN_MAKE_INVISIBLE },
    { "隐形魔杖", WAN_MAKE_INVISIBLE },
    { "隐形的魔杖", WAN_MAKE_INVISIBLE },
    { "隐形之魔杖", WAN_MAKE_INVISIBLE },
    { "隐形之杖", WAN_MAKE_INVISIBLE },
    { "减慢怪物魔杖", WAN_SLOW_MONSTER },
    { "减慢怪物的魔杖", WAN_SLOW_MONSTER },
    { "减慢怪物之魔杖", WAN_SLOW_MONSTER },
    { "减慢怪物之杖", WAN_SLOW_MONSTER },
    { "减速怪物魔杖", WAN_SLOW_MONSTER },
    { "减速怪物的魔杖", WAN_SLOW_MONSTER },
    { "减速怪物之魔杖", WAN_SLOW_MONSTER },
    { "减速怪物之杖", WAN_SLOW_MONSTER },
    { "加速怪物魔杖", WAN_SPEED_MONSTER },
    { "加速怪物的魔杖", WAN_SPEED_MONSTER },
    { "加速怪物之魔杖", WAN_SPEED_MONSTER },
    { "加速怪物之杖", WAN_SPEED_MONSTER },
    { "超度魔杖", WAN_UNDEAD_TURNING },
    { "超度的魔杖", WAN_UNDEAD_TURNING },
    { "超度之魔杖", WAN_UNDEAD_TURNING },
    { "超度之杖", WAN_UNDEAD_TURNING },
    { "驱赶亡灵魔杖", WAN_UNDEAD_TURNING },
    { "驱赶亡灵的魔杖", WAN_UNDEAD_TURNING },
    { "驱赶亡灵之魔杖", WAN_UNDEAD_TURNING },
    { "驱赶亡灵之杖", WAN_UNDEAD_TURNING },
    { "驱散亡灵魔杖", WAN_UNDEAD_TURNING },
    { "驱散亡灵的魔杖", WAN_UNDEAD_TURNING },
    { "驱散亡灵之魔杖", WAN_UNDEAD_TURNING },
    { "驱散亡灵之杖", WAN_UNDEAD_TURNING },
    { "驱赶亡者魔杖", WAN_UNDEAD_TURNING },
    { "驱赶亡者的魔杖", WAN_UNDEAD_TURNING },
    { "驱赶亡者之魔杖", WAN_UNDEAD_TURNING },
    { "驱赶亡者之杖", WAN_UNDEAD_TURNING },
    { "驱散亡者魔杖", WAN_UNDEAD_TURNING },
    { "驱散亡者的魔杖", WAN_UNDEAD_TURNING },
    { "驱散亡者之魔杖", WAN_UNDEAD_TURNING },
    { "驱散亡者之杖", WAN_UNDEAD_TURNING },
    { "驱赶不死生物魔杖", WAN_UNDEAD_TURNING },
    { "驱赶不死生物的魔杖", WAN_UNDEAD_TURNING },
    { "驱赶不死生物之魔杖", WAN_UNDEAD_TURNING },
    { "驱赶不死生物之杖", WAN_UNDEAD_TURNING },
    { "驱散不死生物魔杖", WAN_UNDEAD_TURNING },
    { "驱散不死生物的魔杖", WAN_UNDEAD_TURNING },
    { "驱散不死生物之魔杖", WAN_UNDEAD_TURNING },
    { "驱散不死生物之杖", WAN_UNDEAD_TURNING },
    { "变形魔杖", WAN_POLYMORPH },
    { "变形的魔杖", WAN_POLYMORPH },
    { "变形之魔杖", WAN_POLYMORPH },
    { "变形之杖", WAN_POLYMORPH },
    { "消除魔杖", WAN_CANCELLATION },
    { "消除的魔杖", WAN_CANCELLATION },
    { "消除之魔杖", WAN_CANCELLATION },
    { "消除之杖", WAN_CANCELLATION },
    { "传送魔杖", WAN_TELEPORTATION },
    { "传送的魔杖", WAN_TELEPORTATION },
    { "传送之魔杖", WAN_TELEPORTATION },
    { "传送之杖", WAN_TELEPORTATION },
    { "解锁魔杖", WAN_OPENING },
    { "解锁的魔杖", WAN_OPENING },
    { "解锁之魔杖", WAN_OPENING },
    { "解锁之杖", WAN_OPENING },
    { "上锁魔杖", WAN_LOCKING },
    { "上锁的魔杖", WAN_LOCKING },
    { "上锁之魔杖", WAN_LOCKING },
    { "上锁之杖", WAN_LOCKING },
    { "侦查魔杖", WAN_PROBING },
    { "侦查的魔杖", WAN_PROBING },
    { "侦查之魔杖", WAN_PROBING },
    { "侦查之杖", WAN_PROBING },
    { "挖掘魔杖", WAN_DIGGING },
    { "挖掘的魔杖", WAN_DIGGING },
    { "挖掘之魔杖", WAN_DIGGING },
    { "挖掘之杖", WAN_DIGGING },
    { "魔法飞弹魔杖", WAN_MAGIC_MISSILE },
    { "魔法飞弹的魔杖", WAN_MAGIC_MISSILE },
    { "魔法飞弹之魔杖", WAN_MAGIC_MISSILE },
    { "魔法飞弹之杖", WAN_MAGIC_MISSILE },
    { "火魔杖", WAN_FIRE },
    { "火的魔杖", WAN_FIRE },
    { "火之魔杖", WAN_FIRE },
    { "火之杖", WAN_FIRE },
    { "火焰魔杖", WAN_FIRE },
    { "火焰的魔杖", WAN_FIRE },
    { "火焰之魔杖", WAN_FIRE },
    { "火焰之杖", WAN_FIRE },
    { "寒冷魔杖", WAN_COLD },
    { "寒冷的魔杖", WAN_COLD },
    { "寒冷之魔杖", WAN_COLD },
    { "寒冷之杖", WAN_COLD },
    { "寒冰魔杖", WAN_COLD },
    { "寒冰的魔杖", WAN_COLD },
    { "寒冰之魔杖", WAN_COLD },
    { "寒冰之杖", WAN_COLD },
    { "沉睡魔杖", WAN_SLEEP },
    { "沉睡的魔杖", WAN_SLEEP },
    { "沉睡之魔杖", WAN_SLEEP },
    { "沉睡之杖", WAN_SLEEP },
    { "睡眠魔杖", WAN_SLEEP },
    { "睡眠的魔杖", WAN_SLEEP },
    { "睡眠之魔杖", WAN_SLEEP },
    { "睡眠之杖", WAN_SLEEP },
    { "死亡魔杖", WAN_DEATH },
    { "死亡的魔杖", WAN_DEATH },
    { "死亡之魔杖", WAN_DEATH },
    { "死亡之杖", WAN_DEATH },
    { "闪电魔杖", WAN_LIGHTNING },
    { "闪电的魔杖", WAN_LIGHTNING },
    { "闪电之魔杖", WAN_LIGHTNING },
    { "闪电之杖", WAN_LIGHTNING },
    { "雷电魔杖", WAN_LIGHTNING },
    { "雷电的魔杖", WAN_LIGHTNING },
    { "雷电之魔杖", WAN_LIGHTNING },
    { "雷电之杖", WAN_LIGHTNING },
    /*所有可能的宝石*/
    { "双锂水晶", DILITHIUM_CRYSTAL},
    { "钻石", DIAMOND },
    { "红宝石", RUBY },
    { "红锆石", JACINTH },
    { "蓝宝石", SAPPHIRE },
    { "黑蛋白石", BLACK_OPAL },
    { "祖母绿", EMERALD },
    { "绿松石", TURQUOISE },
    { "黄水晶", CITRINE },
    { "海蓝宝石", AQUAMARINE },
    { "琥珀", AMBER },
    { "黄宝石", TOPAZ },
    { "黑玉", JET },
    { "蛋白石", OPAL },
    { "金绿玉", CHRYSOBERYL },
    { "石榴石", GARNET },
    { "紫水晶", AMETHYST },
    { "碧玉", JASPER },
    { "萤石", FLUORITE },
    { "黑曜石", OBSIDIAN },
    { "玛瑙", AGATE },
    { "翡翠", JADE },
    { "毫无价值的白色玻璃碎片", WORTHLESS_WHITE_GLASS },
    { "毫无价值的蓝色玻璃碎片", WORTHLESS_BLUE_GLASS },
    { "毫无价值的红色玻璃碎片", WORTHLESS_RED_GLASS },
    { "毫无价值的杏色玻璃碎片", WORTHLESS_YELLOWBROWN_GLASS },
    { "毫无价值的橙色玻璃碎片", WORTHLESS_ORANGE_GLASS },
    { "毫无价值的黄色玻璃碎片", WORTHLESS_YELLOW_GLASS },
    { "毫无价值的黑色玻璃碎片", WORTHLESS_BLACK_GLASS },
    { "毫无价值的绿色玻璃碎片", WORTHLESS_GREEN_GLASS },
    { "毫无价值的紫色玻璃碎片", WORTHLESS_VIOLET_GLASS },
    { "不值钱的白色玻璃碎片", WORTHLESS_WHITE_GLASS },
    { "不值钱的蓝色玻璃碎片", WORTHLESS_BLUE_GLASS },
    { "不值钱的红色玻璃碎片", WORTHLESS_RED_GLASS },
    { "不值钱的杏色玻璃碎片", WORTHLESS_YELLOWBROWN_GLASS },
    { "不值钱的橙色玻璃碎片", WORTHLESS_ORANGE_GLASS },
    { "不值钱的黄色玻璃碎片", WORTHLESS_YELLOW_GLASS },
    { "不值钱的黑色玻璃碎片", WORTHLESS_BLACK_GLASS },
    { "不值钱的绿色玻璃碎片", WORTHLESS_GREEN_GLASS },
    { "不值钱的紫色玻璃碎片", WORTHLESS_VIOLET_GLASS },
    { "白色玻璃碎片", WORTHLESS_WHITE_GLASS },
    { "蓝色玻璃碎片", WORTHLESS_BLUE_GLASS },
    { "红色玻璃碎片", WORTHLESS_RED_GLASS },
    { "杏色玻璃碎片", WORTHLESS_YELLOWBROWN_GLASS },
    { "橙色玻璃碎片", WORTHLESS_ORANGE_GLASS },
    { "黄色玻璃碎片", WORTHLESS_YELLOW_GLASS },
    { "黑色玻璃碎片", WORTHLESS_BLACK_GLASS },
    { "绿色玻璃碎片", WORTHLESS_GREEN_GLASS },
    { "紫色玻璃碎片", WORTHLESS_VIOLET_GLASS },
    { (const char *) 0, 0 },
};
static const struct figurine_spellings {
    const char *sp;
    int itsmonster;
    int itsgender;
    int whatitis;
} figurines[] = {
    { "巨型蚂蚁雕像", PM_GIANT_ANT, NEUTRAL, STATUE },
    { "巨型蚂蚁的雕像", PM_GIANT_ANT, NEUTRAL, STATUE },
    { "巨型蚂蚁小雕像", PM_GIANT_ANT, NEUTRAL, FIGURINE },
    { "巨型蚂蚁的小雕像", PM_GIANT_ANT, NEUTRAL, FIGURINE },
    { "巨型蚂蚁罐头", PM_GIANT_ANT, NEUTRAL, TIN },
    { "巨型蚂蚁的罐头", PM_GIANT_ANT, NEUTRAL, TIN },
    { "巨型蚂蚁肉罐头", PM_GIANT_ANT, NEUTRAL, TIN },
    { "巨蚂蚁雕像", PM_GIANT_ANT, NEUTRAL, STATUE },
    { "巨蚂蚁的雕像", PM_GIANT_ANT, NEUTRAL, STATUE },
    { "巨蚂蚁小雕像", PM_GIANT_ANT, NEUTRAL, FIGURINE },
    { "巨蚂蚁的小雕像", PM_GIANT_ANT, NEUTRAL, FIGURINE },
    { "巨蚂蚁罐头", PM_GIANT_ANT, NEUTRAL, TIN },
    { "巨蚂蚁的罐头", PM_GIANT_ANT, NEUTRAL, TIN },
    { "巨蚂蚁肉罐头", PM_GIANT_ANT, NEUTRAL, TIN },
    { "巨蚁雕像", PM_GIANT_ANT, NEUTRAL, STATUE },
    { "巨蚁的雕像", PM_GIANT_ANT, NEUTRAL, STATUE },
    { "巨蚁小雕像", PM_GIANT_ANT, NEUTRAL, FIGURINE },
    { "巨蚁的小雕像", PM_GIANT_ANT, NEUTRAL, FIGURINE },
    { "巨蚁罐头", PM_GIANT_ANT, NEUTRAL, TIN },
    { "巨蚁的罐头", PM_GIANT_ANT, NEUTRAL, TIN },
    { "巨蚁肉罐头", PM_GIANT_ANT, NEUTRAL, TIN },
    { "杀人蜂雕像", PM_KILLER_BEE, NEUTRAL, STATUE },
    { "杀人蜂的雕像", PM_KILLER_BEE, NEUTRAL, STATUE },
    { "杀人蜂小雕像", PM_KILLER_BEE, NEUTRAL, FIGURINE },
    { "杀人蜂的小雕像", PM_KILLER_BEE, NEUTRAL, FIGURINE },
    { "杀人蜂罐头", PM_KILLER_BEE, NEUTRAL, TIN },
    { "杀人蜂的罐头", PM_KILLER_BEE, NEUTRAL, TIN },
    { "杀人蜂肉罐头", PM_KILLER_BEE, NEUTRAL, TIN },
    { "巨蜂雕像", PM_KILLER_BEE, NEUTRAL, STATUE },
    { "巨蜂的雕像", PM_KILLER_BEE, NEUTRAL, STATUE },
    { "巨蜂小雕像", PM_KILLER_BEE, NEUTRAL, FIGURINE },
    { "巨蜂的小雕像", PM_KILLER_BEE, NEUTRAL, FIGURINE },
    { "巨蜂罐头", PM_KILLER_BEE, NEUTRAL, TIN },
    { "巨蜂的罐头", PM_KILLER_BEE, NEUTRAL, TIN },
    { "巨蜂肉罐头", PM_KILLER_BEE, NEUTRAL, TIN },
    { "兵蚁雕像", PM_SOLDIER_ANT, NEUTRAL, STATUE },
    { "兵蚁的雕像", PM_SOLDIER_ANT, NEUTRAL, STATUE },
    { "兵蚁小雕像", PM_SOLDIER_ANT, NEUTRAL, FIGURINE },
    { "兵蚁的小雕像", PM_SOLDIER_ANT, NEUTRAL, FIGURINE },
    { "兵蚁罐头", PM_SOLDIER_ANT, NEUTRAL, TIN },
    { "兵蚁的罐头", PM_SOLDIER_ANT, NEUTRAL, TIN },
    { "兵蚁肉罐头", PM_SOLDIER_ANT, NEUTRAL, TIN },
    { "火蚁雕像", PM_FIRE_ANT, NEUTRAL, STATUE },
    { "火蚁的雕像", PM_FIRE_ANT, NEUTRAL, STATUE },
    { "火蚁小雕像", PM_FIRE_ANT, NEUTRAL, FIGURINE },
    { "火蚁的小雕像", PM_FIRE_ANT, NEUTRAL, FIGURINE },
    { "火蚁罐头", PM_FIRE_ANT, NEUTRAL, TIN },
    { "火蚁的罐头", PM_FIRE_ANT, NEUTRAL, TIN },
    { "火蚁肉罐头", PM_FIRE_ANT, NEUTRAL, TIN },
    { "巨型甲虫雕像", PM_GIANT_BEETLE, NEUTRAL, STATUE },
    { "巨型甲虫的雕像", PM_GIANT_BEETLE, NEUTRAL, STATUE },
    { "巨型甲虫小雕像", PM_GIANT_BEETLE, NEUTRAL, FIGURINE },
    { "巨型甲虫的小雕像", PM_GIANT_BEETLE, NEUTRAL, FIGURINE },
    { "巨型甲虫罐头", PM_GIANT_BEETLE, NEUTRAL, TIN },
    { "巨型甲虫的罐头", PM_GIANT_BEETLE, NEUTRAL, TIN },
    { "巨型甲虫肉罐头", PM_GIANT_BEETLE, NEUTRAL, TIN },
    { "巨甲虫雕像", PM_GIANT_BEETLE, NEUTRAL, STATUE },
    { "巨甲虫的雕像", PM_GIANT_BEETLE, NEUTRAL, STATUE },
    { "巨甲虫小雕像", PM_GIANT_BEETLE, NEUTRAL, FIGURINE },
    { "巨甲虫的小雕像", PM_GIANT_BEETLE, NEUTRAL, FIGURINE },
    { "巨甲虫罐头", PM_GIANT_BEETLE, NEUTRAL, TIN },
    { "巨甲虫的罐头", PM_GIANT_BEETLE, NEUTRAL, TIN },
    { "巨甲虫肉罐头", PM_GIANT_BEETLE, NEUTRAL, TIN },
    { "蜂后雕像", PM_QUEEN_BEE, NEUTRAL, STATUE },
    { "蜂后的雕像", PM_QUEEN_BEE, NEUTRAL, STATUE },
    { "蜂后小雕像", PM_QUEEN_BEE, NEUTRAL, FIGURINE },
    { "蜂后的小雕像", PM_QUEEN_BEE, NEUTRAL, FIGURINE },
    { "蜂后罐头", PM_QUEEN_BEE, NEUTRAL, TIN },
    { "蜂后的罐头", PM_QUEEN_BEE, NEUTRAL, TIN },
    { "蜂后肉罐头", PM_QUEEN_BEE, NEUTRAL, TIN },
    { "酸滴雕像", PM_ACID_BLOB, NEUTRAL, STATUE },
    { "酸滴的雕像", PM_ACID_BLOB, NEUTRAL, STATUE },
    { "酸滴小雕像", PM_ACID_BLOB, NEUTRAL, FIGURINE },
    { "酸滴的小雕像", PM_ACID_BLOB, NEUTRAL, FIGURINE },
    { "酸滴罐头", PM_ACID_BLOB, NEUTRAL, TIN },
    { "酸滴的罐头", PM_ACID_BLOB, NEUTRAL, TIN },
    { "酸滴肉罐头", PM_ACID_BLOB, NEUTRAL, TIN },
    { "酸性团块雕像", PM_ACID_BLOB, NEUTRAL, STATUE },
    { "酸性团块的雕像", PM_ACID_BLOB, NEUTRAL, STATUE },
    { "酸性团块小雕像", PM_ACID_BLOB, NEUTRAL, FIGURINE },
    { "酸性团块的小雕像", PM_ACID_BLOB, NEUTRAL, FIGURINE },
    { "酸性团块罐头", PM_ACID_BLOB, NEUTRAL, TIN },
    { "酸性团块的罐头", PM_ACID_BLOB, NEUTRAL, TIN },
    { "酸性团块肉罐头", PM_ACID_BLOB, NEUTRAL, TIN },
    { "酸块雕像", PM_ACID_BLOB, NEUTRAL, STATUE },
    { "酸块的雕像", PM_ACID_BLOB, NEUTRAL, STATUE },
    { "酸块小雕像", PM_ACID_BLOB, NEUTRAL, FIGURINE },
    { "酸块的小雕像", PM_ACID_BLOB, NEUTRAL, FIGURINE },
    { "酸块罐头", PM_ACID_BLOB, NEUTRAL, TIN },
    { "酸块的罐头", PM_ACID_BLOB, NEUTRAL, TIN },
    { "酸块肉罐头", PM_ACID_BLOB, NEUTRAL, TIN },
    { "颤抖的斑点雕像", PM_QUIVERING_BLOB, NEUTRAL, STATUE },
    { "颤抖的斑点的雕像", PM_QUIVERING_BLOB, NEUTRAL, STATUE },
    { "颤抖的斑点小雕像", PM_QUIVERING_BLOB, NEUTRAL, FIGURINE },
    { "颤抖的斑点的小雕像", PM_QUIVERING_BLOB, NEUTRAL, FIGURINE },
    { "颤抖的斑点罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "颤抖的斑点的罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "颤抖的斑点肉罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "颤抖斑点雕像", PM_QUIVERING_BLOB, NEUTRAL, STATUE },
    { "颤抖斑点的雕像", PM_QUIVERING_BLOB, NEUTRAL, STATUE },
    { "颤抖斑点小雕像", PM_QUIVERING_BLOB, NEUTRAL, FIGURINE },
    { "颤抖斑点的小雕像", PM_QUIVERING_BLOB, NEUTRAL, FIGURINE },
    { "颤抖斑点罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "颤抖斑点的罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "颤抖斑点肉罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "颤抖的团块雕像", PM_QUIVERING_BLOB, NEUTRAL, STATUE },
    { "颤抖的团块的雕像", PM_QUIVERING_BLOB, NEUTRAL, STATUE },
    { "颤抖的团块小雕像", PM_QUIVERING_BLOB, NEUTRAL, FIGURINE },
    { "颤抖的团块的小雕像", PM_QUIVERING_BLOB, NEUTRAL, FIGURINE },
    { "颤抖的团块罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "颤抖的团块的罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "颤抖的团块肉罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "颤抖团块雕像", PM_QUIVERING_BLOB, NEUTRAL, STATUE },
    { "颤抖团块的雕像", PM_QUIVERING_BLOB, NEUTRAL, STATUE },
    { "颤抖团块小雕像", PM_QUIVERING_BLOB, NEUTRAL, FIGURINE },
    { "颤抖团块的小雕像", PM_QUIVERING_BLOB, NEUTRAL, FIGURINE },
    { "颤抖团块罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "颤抖团块的罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "颤抖团块肉罐头", PM_QUIVERING_BLOB, NEUTRAL, TIN },
    { "黏胶立方怪雕像", PM_GELATINOUS_CUBE, NEUTRAL, STATUE },
    { "黏胶立方怪的雕像", PM_GELATINOUS_CUBE, NEUTRAL, STATUE },
    { "黏胶立方怪小雕像", PM_GELATINOUS_CUBE, NEUTRAL, FIGURINE },
    { "黏胶立方怪的小雕像", PM_GELATINOUS_CUBE, NEUTRAL, FIGURINE },
    { "黏胶立方怪罐头", PM_GELATINOUS_CUBE, NEUTRAL, TIN },
    { "黏胶立方怪的罐头", PM_GELATINOUS_CUBE, NEUTRAL, TIN },
    { "黏胶立方怪肉罐头", PM_GELATINOUS_CUBE, NEUTRAL, TIN },
    { "黏胶立方雕像", PM_GELATINOUS_CUBE, NEUTRAL, STATUE },
    { "黏胶立方的雕像", PM_GELATINOUS_CUBE, NEUTRAL, STATUE },
    { "黏胶立方小雕像", PM_GELATINOUS_CUBE, NEUTRAL, FIGURINE },
    { "黏胶立方的小雕像", PM_GELATINOUS_CUBE, NEUTRAL, FIGURINE },
    { "黏胶立方罐头", PM_GELATINOUS_CUBE, NEUTRAL, TIN },
    { "黏胶立方的罐头", PM_GELATINOUS_CUBE, NEUTRAL, TIN },
    { "黏胶立方肉罐头", PM_GELATINOUS_CUBE, NEUTRAL, TIN },
    { "小鸡蛇雕像", PM_CHICKATRICE, NEUTRAL, STATUE },
    { "小鸡蛇的雕像", PM_CHICKATRICE, NEUTRAL, STATUE },
    { "小鸡蛇小雕像", PM_CHICKATRICE, NEUTRAL, FIGURINE },
    { "小鸡蛇的小雕像", PM_CHICKATRICE, NEUTRAL, FIGURINE },
    { "小鸡蛇罐头", PM_CHICKATRICE, NEUTRAL, TIN },
    { "小鸡蛇的罐头", PM_CHICKATRICE, NEUTRAL, TIN },
    { "小鸡蛇肉罐头", PM_CHICKATRICE, NEUTRAL, TIN },
    { "鸡蛇雕像", PM_COCKATRICE, NEUTRAL, STATUE },
    { "鸡蛇的雕像", PM_COCKATRICE, NEUTRAL, STATUE },
    { "鸡蛇小雕像", PM_COCKATRICE, NEUTRAL, FIGURINE },
    { "鸡蛇的小雕像", PM_COCKATRICE, NEUTRAL, FIGURINE },
    { "鸡蛇罐头", PM_COCKATRICE, NEUTRAL, TIN },
    { "鸡蛇的罐头", PM_COCKATRICE, NEUTRAL, TIN },
    { "鸡蛇肉罐头", PM_COCKATRICE, NEUTRAL, TIN },
    { "火鸡蛇雕像", PM_PYROLISK, NEUTRAL, STATUE },
    { "火鸡蛇的雕像", PM_PYROLISK, NEUTRAL, STATUE },
    { "火鸡蛇小雕像", PM_PYROLISK, NEUTRAL, FIGURINE },
    { "火鸡蛇的小雕像", PM_PYROLISK, NEUTRAL, FIGURINE },
    { "火鸡蛇罐头", PM_PYROLISK, NEUTRAL, TIN },
    { "火鸡蛇的罐头", PM_PYROLISK, NEUTRAL, TIN },
    { "火鸡蛇肉罐头", PM_PYROLISK, NEUTRAL, TIN },
    { "豺狼雕像", PM_JACKAL, NEUTRAL, STATUE },
    { "豺狼的雕像", PM_JACKAL, NEUTRAL, STATUE },
    { "豺狼小雕像", PM_JACKAL, NEUTRAL, FIGURINE },
    { "豺狼的小雕像", PM_JACKAL, NEUTRAL, FIGURINE },
    { "豺狼罐头", PM_JACKAL, NEUTRAL, TIN },
    { "豺狼的罐头", PM_JACKAL, NEUTRAL, TIN },
    { "豺狼肉罐头", PM_JACKAL, NEUTRAL, TIN },
    { "狐狸雕像", PM_FOX, NEUTRAL, STATUE },
    { "狐狸的雕像", PM_FOX, NEUTRAL, STATUE },
    { "狐狸小雕像", PM_FOX, NEUTRAL, FIGURINE },
    { "狐狸的小雕像", PM_FOX, NEUTRAL, FIGURINE },
    { "狐狸罐头", PM_FOX, NEUTRAL, TIN },
    { "狐狸的罐头", PM_FOX, NEUTRAL, TIN },
    { "狐狸肉罐头", PM_FOX, NEUTRAL, TIN },
    { "土狼雕像", PM_COYOTE, NEUTRAL, STATUE },
    { "土狼的雕像", PM_COYOTE, NEUTRAL, STATUE },
    { "土狼小雕像", PM_COYOTE, NEUTRAL, FIGURINE },
    { "土狼的小雕像", PM_COYOTE, NEUTRAL, FIGURINE },
    { "土狼罐头", PM_COYOTE, NEUTRAL, TIN },
    { "土狼的罐头", PM_COYOTE, NEUTRAL, TIN },
    { "土狼肉罐头", PM_COYOTE, NEUTRAL, TIN },
    { "豺狼人雕像", PM_WEREJACKAL, NEUTRAL, STATUE },
    { "豺狼人的雕像", PM_WEREJACKAL, NEUTRAL, STATUE },
    { "豺狼人小雕像", PM_WEREJACKAL, NEUTRAL, FIGURINE },
    { "豺狼人的小雕像", PM_WEREJACKAL, NEUTRAL, FIGURINE },
    { "豺狼人罐头", PM_WEREJACKAL, NEUTRAL, TIN },
    { "豺狼人的罐头", PM_WEREJACKAL, NEUTRAL, TIN },
    { "豺狼人肉罐头", PM_WEREJACKAL, NEUTRAL, TIN },
    { "小狗雕像", PM_LITTLE_DOG, NEUTRAL, STATUE },
    { "小狗的雕像", PM_LITTLE_DOG, NEUTRAL, STATUE },
    { "小狗小雕像", PM_LITTLE_DOG, NEUTRAL, FIGURINE },
    { "小狗的小雕像", PM_LITTLE_DOG, NEUTRAL, FIGURINE },
    { "小狗罐头", PM_LITTLE_DOG, NEUTRAL, TIN },
    { "小狗的罐头", PM_LITTLE_DOG, NEUTRAL, TIN },
    { "小狗肉罐头", PM_LITTLE_DOG, NEUTRAL, TIN },
    { "澳洲野狗雕像", PM_DINGO, NEUTRAL, STATUE },
    { "澳洲野狗的雕像", PM_DINGO, NEUTRAL, STATUE },
    { "澳洲野狗小雕像", PM_DINGO, NEUTRAL, FIGURINE },
    { "澳洲野狗的小雕像", PM_DINGO, NEUTRAL, FIGURINE },
    { "澳洲野狗罐头", PM_DINGO, NEUTRAL, TIN },
    { "澳洲野狗的罐头", PM_DINGO, NEUTRAL, TIN },
    { "澳洲野狗肉罐头", PM_DINGO, NEUTRAL, TIN },
    { "狗雕像", PM_DOG, NEUTRAL, STATUE },
    { "狗的雕像", PM_DOG, NEUTRAL, STATUE },
    { "狗小雕像", PM_DOG, NEUTRAL, FIGURINE },
    { "狗的小雕像", PM_DOG, NEUTRAL, FIGURINE },
    { "狗罐头", PM_DOG, NEUTRAL, TIN },
    { "狗的罐头", PM_DOG, NEUTRAL, TIN },
    { "狗肉罐头", PM_DOG, NEUTRAL, TIN },
    { "大狗雕像", PM_LARGE_DOG, NEUTRAL, STATUE },
    { "大狗的雕像", PM_LARGE_DOG, NEUTRAL, STATUE },
    { "大狗小雕像", PM_LARGE_DOG, NEUTRAL, FIGURINE },
    { "大狗的小雕像", PM_LARGE_DOG, NEUTRAL, FIGURINE },
    { "大狗罐头", PM_LARGE_DOG, NEUTRAL, TIN },
    { "大狗的罐头", PM_LARGE_DOG, NEUTRAL, TIN },
    { "大狗肉罐头", PM_LARGE_DOG, NEUTRAL, TIN },
    { "狼雕像", PM_WOL, STATUEF },
    { "狼的雕像", PM_WOL, STATUEF },
    { "狼小雕像", PM_WOL, FIGURINEF },
    { "狼的小雕像", PM_WOL, FIGURINEF },
    { "狼罐头", PM_WOL, TINF },
    { "狼的罐头", PM_WOL, TINF },
    { "狼肉罐头", PM_WOL, TINF },
    { "狼人雕像", PM_WEREWOLF, NEUTRAL, STATUE },
    { "狼人的雕像", PM_WEREWOLF, NEUTRAL, STATUE },
    { "狼人小雕像", PM_WEREWOLF, NEUTRAL, FIGURINE },
    { "狼人的小雕像", PM_WEREWOLF, NEUTRAL, FIGURINE },
    { "狼人罐头", PM_WEREWOLF, NEUTRAL, TIN },
    { "狼人的罐头", PM_WEREWOLF, NEUTRAL, TIN },
    { "狼人肉罐头", PM_WEREWOLF, NEUTRAL, TIN },
    { "冬狼崽雕像", PM_WINTER_WOLF_CUB, NEUTRAL, STATUE },
    { "冬狼崽的雕像", PM_WINTER_WOLF_CUB, NEUTRAL, STATUE },
    { "冬狼崽小雕像", PM_WINTER_WOLF_CUB, NEUTRAL, FIGURINE },
    { "冬狼崽的小雕像", PM_WINTER_WOLF_CUB, NEUTRAL, FIGURINE },
    { "冬狼崽罐头", PM_WINTER_WOLF_CUB, NEUTRAL, TIN },
    { "冬狼崽的罐头", PM_WINTER_WOLF_CUB, NEUTRAL, TIN },
    { "冬狼崽肉罐头", PM_WINTER_WOLF_CUB, NEUTRAL, TIN },
    { "小冬狼雕像", PM_WINTER_WOLF_CUB, NEUTRAL, STATUE },
    { "小冬狼的雕像", PM_WINTER_WOLF_CUB, NEUTRAL, STATUE },
    { "小冬狼小雕像", PM_WINTER_WOLF_CUB, NEUTRAL, FIGURINE },
    { "小冬狼的小雕像", PM_WINTER_WOLF_CUB, NEUTRAL, FIGURINE },
    { "小冬狼罐头", PM_WINTER_WOLF_CUB, NEUTRAL, TIN },
    { "小冬狼的罐头", PM_WINTER_WOLF_CUB, NEUTRAL, TIN },
    { "小冬狼肉罐头", PM_WINTER_WOLF_CUB, NEUTRAL, TIN },
    { "座狼雕像", PM_WARG, NEUTRAL, STATUE },
    { "座狼的雕像", PM_WARG, NEUTRAL, STATUE },
    { "座狼小雕像", PM_WARG, NEUTRAL, FIGURINE },
    { "座狼的小雕像", PM_WARG, NEUTRAL, FIGURINE },
    { "座狼罐头", PM_WARG, NEUTRAL, TIN },
    { "座狼的罐头", PM_WARG, NEUTRAL, TIN },
    { "座狼肉罐头", PM_WARG, NEUTRAL, TIN },
    { "冬狼雕像", PM_WINTER_WOLF, NEUTRAL, STATUE },
    { "冬狼的雕像", PM_WINTER_WOLF, NEUTRAL, STATUE },
    { "冬狼小雕像", PM_WINTER_WOLF, NEUTRAL, FIGURINE },
    { "冬狼的小雕像", PM_WINTER_WOLF, NEUTRAL, FIGURINE },
    { "冬狼罐头", PM_WINTER_WOLF, NEUTRAL, TIN },
    { "冬狼的罐头", PM_WINTER_WOLF, NEUTRAL, TIN },
    { "冬狼肉罐头", PM_WINTER_WOLF, NEUTRAL, TIN },
    { "地狱小猎犬雕像", PM_HELL_HOUND_PUP, NEUTRAL, STATUE },
    { "地狱小猎犬的雕像", PM_HELL_HOUND_PUP, NEUTRAL, STATUE },
    { "地狱小猎犬小雕像", PM_HELL_HOUND_PUP, NEUTRAL, FIGURINE },
    { "地狱小猎犬的小雕像", PM_HELL_HOUND_PUP, NEUTRAL, FIGURINE },
    { "地狱小猎犬罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "地狱小猎犬的罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "地狱小猎犬肉罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "地狱小狗雕像", PM_HELL_HOUND_PUP, NEUTRAL, STATUE },
    { "地狱小狗的雕像", PM_HELL_HOUND_PUP, NEUTRAL, STATUE },
    { "地狱小狗小雕像", PM_HELL_HOUND_PUP, NEUTRAL, FIGURINE },
    { "地狱小狗的小雕像", PM_HELL_HOUND_PUP, NEUTRAL, FIGURINE },
    { "地狱小狗罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "地狱小狗的罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "地狱小狗肉罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "小地狱猎犬雕像", PM_HELL_HOUND_PUP, NEUTRAL, STATUE },
    { "小地狱猎犬的雕像", PM_HELL_HOUND_PUP, NEUTRAL, STATUE },
    { "小地狱猎犬小雕像", PM_HELL_HOUND_PUP, NEUTRAL, FIGURINE },
    { "小地狱猎犬的小雕像", PM_HELL_HOUND_PUP, NEUTRAL, FIGURINE },
    { "小地狱猎犬罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "小地狱猎犬的罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "小地狱猎犬肉罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "小地狱狗雕像", PM_HELL_HOUND_PUP, NEUTRAL, STATUE },
    { "小地狱狗的雕像", PM_HELL_HOUND_PUP, NEUTRAL, STATUE },
    { "小地狱狗小雕像", PM_HELL_HOUND_PUP, NEUTRAL, FIGURINE },
    { "小地狱狗的小雕像", PM_HELL_HOUND_PUP, NEUTRAL, FIGURINE },
    { "小地狱狗罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "小地狱狗的罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "小地狱狗肉罐头", PM_HELL_HOUND_PUP, NEUTRAL, TIN },
    { "地狱猎犬雕像", PM_HELL_HOUND, NEUTRAL, STATUE },
    { "地狱猎犬的雕像", PM_HELL_HOUND, NEUTRAL, STATUE },
    { "地狱猎犬小雕像", PM_HELL_HOUND, NEUTRAL, FIGURINE },
    { "地狱猎犬的小雕像", PM_HELL_HOUND, NEUTRAL, FIGURINE },
    { "地狱猎犬罐头", PM_HELL_HOUND, NEUTRAL, TIN },
    { "地狱猎犬的罐头", PM_HELL_HOUND, NEUTRAL, TIN },
    { "地狱猎犬肉罐头", PM_HELL_HOUND, NEUTRAL, TIN },
    { "地狱狗雕像", PM_HELL_HOUND, NEUTRAL, STATUE },
    { "地狱狗的雕像", PM_HELL_HOUND, NEUTRAL, STATUE },
    { "地狱狗小雕像", PM_HELL_HOUND, NEUTRAL, FIGURINE },
    { "地狱狗的小雕像", PM_HELL_HOUND, NEUTRAL, FIGURINE },
    { "地狱狗罐头", PM_HELL_HOUND, NEUTRAL, TIN },
    { "地狱狗的罐头", PM_HELL_HOUND, NEUTRAL, TIN },
    { "地狱狗肉罐头", PM_HELL_HOUND, NEUTRAL, TIN },
    { "刻耳柏洛斯雕像", PM_CERBERUS, NEUTRAL, STATUE },
    { "刻耳柏洛斯的雕像", PM_CERBERUS, NEUTRAL, STATUE },
    { "刻耳柏洛斯小雕像", PM_CERBERUS, NEUTRAL, FIGURINE },
    { "刻耳柏洛斯的小雕像", PM_CERBERUS, NEUTRAL, FIGURINE },
    { "刻耳柏洛斯罐头", PM_CERBERUS, NEUTRAL, TIN },
    { "刻耳柏洛斯的罐头", PM_CERBERUS, NEUTRAL, TIN },
    { "刻耳柏洛斯肉罐头", PM_CERBERUS, NEUTRAL, TIN },
    { "气体孢子雕像", PM_GAS_SPORE, NEUTRAL, STATUE },
    { "气体孢子的雕像", PM_GAS_SPORE, NEUTRAL, STATUE },
    { "气体孢子小雕像", PM_GAS_SPORE, NEUTRAL, FIGURINE },
    { "气体孢子的小雕像", PM_GAS_SPORE, NEUTRAL, FIGURINE },
    { "气体孢子罐头", PM_GAS_SPORE, NEUTRAL, TIN },
    { "气体孢子的罐头", PM_GAS_SPORE, NEUTRAL, TIN },
    { "气体孢子肉罐头", PM_GAS_SPORE, NEUTRAL, TIN },
    { "浮眼雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "浮眼的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "浮眼小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "浮眼的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "浮眼罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮眼的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮眼肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮眼雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "悬浮眼的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "悬浮眼小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "悬浮眼的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "悬浮眼罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮眼的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮眼肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空眼雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "浮空眼的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "浮空眼小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "浮空眼的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "浮空眼罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空眼的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空眼肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮眼雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "漂浮眼的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "漂浮眼小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "漂浮眼的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "漂浮眼罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮眼的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮眼肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮的眼雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "悬浮的眼的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "悬浮的眼小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "悬浮的眼的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "悬浮的眼罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮的眼的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮的眼肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮之眼雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "悬浮之眼的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "悬浮之眼小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "悬浮之眼的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "悬浮之眼罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮之眼的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮之眼肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮的眼睛雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "悬浮的眼睛的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "悬浮的眼睛小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "悬浮的眼睛的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "悬浮的眼睛罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮的眼睛的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "悬浮的眼睛肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空的眼雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "浮空的眼的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "浮空的眼小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "浮空的眼的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "浮空的眼罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空的眼的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空的眼肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空之眼雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "浮空之眼的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "浮空之眼小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "浮空之眼的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "浮空之眼罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空之眼的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空之眼肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空的眼睛雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "浮空的眼睛的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "浮空的眼睛小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "浮空的眼睛的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "浮空的眼睛罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空的眼睛的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "浮空的眼睛肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮的眼雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "漂浮的眼的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "漂浮的眼小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "漂浮的眼的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "漂浮的眼罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮的眼的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮的眼肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮之眼雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "漂浮之眼的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "漂浮之眼小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "漂浮之眼的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "漂浮之眼罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮之眼的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮之眼肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮的眼睛雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "漂浮的眼睛的雕像", PM_FLOATING_EYE, NEUTRAL, STATUE },
    { "漂浮的眼睛小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "漂浮的眼睛的小雕像", PM_FLOATING_EYE, NEUTRAL, FIGURINE },
    { "漂浮的眼睛罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮的眼睛的罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "漂浮的眼睛肉罐头", PM_FLOATING_EYE, NEUTRAL, TIN },
    { "冻结球雕像", PM_FREEZING_SPHERE, NEUTRAL, STATUE },
    { "冻结球的雕像", PM_FREEZING_SPHERE, NEUTRAL, STATUE },
    { "冻结球小雕像", PM_FREEZING_SPHERE, NEUTRAL, FIGURINE },
    { "冻结球的小雕像", PM_FREEZING_SPHERE, NEUTRAL, FIGURINE },
    { "冻结球罐头", PM_FREEZING_SPHERE, NEUTRAL, TIN },
    { "冻结球的罐头", PM_FREEZING_SPHERE, NEUTRAL, TIN },
    { "冻结球肉罐头", PM_FREEZING_SPHERE, NEUTRAL, TIN },
    { "冰球雕像", PM_FREEZING_SPHERE, NEUTRAL, STATUE },
    { "冰球的雕像", PM_FREEZING_SPHERE, NEUTRAL, STATUE },
    { "冰球小雕像", PM_FREEZING_SPHERE, NEUTRAL, FIGURINE },
    { "冰球的小雕像", PM_FREEZING_SPHERE, NEUTRAL, FIGURINE },
    { "冰球罐头", PM_FREEZING_SPHERE, NEUTRAL, TIN },
    { "冰球的罐头", PM_FREEZING_SPHERE, NEUTRAL, TIN },
    { "冰球肉罐头", PM_FREEZING_SPHERE, NEUTRAL, TIN },
    { "火焰球雕像", PM_FLAMING_SPHERE, NEUTRAL, STATUE },
    { "火焰球的雕像", PM_FLAMING_SPHERE, NEUTRAL, STATUE },
    { "火焰球小雕像", PM_FLAMING_SPHERE, NEUTRAL, FIGURINE },
    { "火焰球的小雕像", PM_FLAMING_SPHERE, NEUTRAL, FIGURINE },
    { "火焰球罐头", PM_FLAMING_SPHERE, NEUTRAL, TIN },
    { "火焰球的罐头", PM_FLAMING_SPHERE, NEUTRAL, TIN },
    { "火焰球肉罐头", PM_FLAMING_SPHERE, NEUTRAL, TIN },
    { "火球雕像", PM_FLAMING_SPHERE, NEUTRAL, STATUE },
    { "火球的雕像", PM_FLAMING_SPHERE, NEUTRAL, STATUE },
    { "火球小雕像", PM_FLAMING_SPHERE, NEUTRAL, FIGURINE },
    { "火球的小雕像", PM_FLAMING_SPHERE, NEUTRAL, FIGURINE },
    { "火球罐头", PM_FLAMING_SPHERE, NEUTRAL, TIN },
    { "火球的罐头", PM_FLAMING_SPHERE, NEUTRAL, TIN },
    { "火球肉罐头", PM_FLAMING_SPHERE, NEUTRAL, TIN },
    { "电球雕像", PM_SHOCKING_SPHERE, NEUTRAL, STATUE },
    { "电球的雕像", PM_SHOCKING_SPHERE, NEUTRAL, STATUE },
    { "电球小雕像", PM_SHOCKING_SPHERE, NEUTRAL, FIGURINE },
    { "电球的小雕像", PM_SHOCKING_SPHERE, NEUTRAL, FIGURINE },
    { "电球罐头", PM_SHOCKING_SPHERE, NEUTRAL, TIN },
    { "电球的罐头", PM_SHOCKING_SPHERE, NEUTRAL, TIN },
    { "电球肉罐头", PM_SHOCKING_SPHERE, NEUTRAL, TIN },
    { "小猫雕像", PM_KITTEN, NEUTRAL, STATUE },
    { "小猫的雕像", PM_KITTEN, NEUTRAL, STATUE },
    { "小猫小雕像", PM_KITTEN, NEUTRAL, FIGURINE },
    { "小猫的小雕像", PM_KITTEN, NEUTRAL, FIGURINE },
    { "小猫罐头", PM_KITTEN, NEUTRAL, TIN },
    { "小猫的罐头", PM_KITTEN, NEUTRAL, TIN },
    { "小猫肉罐头", PM_KITTEN, NEUTRAL, TIN },
    { "家猫雕像", PM_HOUSECAT, NEUTRAL, STATUE },
    { "家猫的雕像", PM_HOUSECAT, NEUTRAL, STATUE },
    { "家猫小雕像", PM_HOUSECAT, NEUTRAL, FIGURINE },
    { "家猫的小雕像", PM_HOUSECAT, NEUTRAL, FIGURINE },
    { "家猫罐头", PM_HOUSECAT, NEUTRAL, TIN },
    { "家猫的罐头", PM_HOUSECAT, NEUTRAL, TIN },
    { "家猫肉罐头", PM_HOUSECAT, NEUTRAL, TIN },
    { "美洲豹雕像", PM_JAGUAR, NEUTRAL, STATUE },
    { "美洲豹的雕像", PM_JAGUAR, NEUTRAL, STATUE },
    { "美洲豹小雕像", PM_JAGUAR, NEUTRAL, FIGURINE },
    { "美洲豹的小雕像", PM_JAGUAR, NEUTRAL, FIGURINE },
    { "美洲豹罐头", PM_JAGUAR, NEUTRAL, TIN },
    { "美洲豹的罐头", PM_JAGUAR, NEUTRAL, TIN },
    { "美洲豹肉罐头", PM_JAGUAR, NEUTRAL, TIN },
    { "猞猁雕像", PM_LYNX, NEUTRAL, STATUE },
    { "猞猁的雕像", PM_LYNX, NEUTRAL, STATUE },
    { "猞猁小雕像", PM_LYNX, NEUTRAL, FIGURINE },
    { "猞猁的小雕像", PM_LYNX, NEUTRAL, FIGURINE },
    { "猞猁罐头", PM_LYNX, NEUTRAL, TIN },
    { "猞猁的罐头", PM_LYNX, NEUTRAL, TIN },
    { "猞猁肉罐头", PM_LYNX, NEUTRAL, TIN },
    { "黑豹雕像", PM_PANTHER, NEUTRAL, STATUE },
    { "黑豹的雕像", PM_PANTHER, NEUTRAL, STATUE },
    { "黑豹小雕像", PM_PANTHER, NEUTRAL, FIGURINE },
    { "黑豹的小雕像", PM_PANTHER, NEUTRAL, FIGURINE },
    { "黑豹罐头", PM_PANTHER, NEUTRAL, TIN },
    { "黑豹的罐头", PM_PANTHER, NEUTRAL, TIN },
    { "黑豹肉罐头", PM_PANTHER, NEUTRAL, TIN },
    { "大猫雕像", PM_LARGE_CAT, NEUTRAL, STATUE },
    { "大猫的雕像", PM_LARGE_CAT, NEUTRAL, STATUE },
    { "大猫小雕像", PM_LARGE_CAT, NEUTRAL, FIGURINE },
    { "大猫的小雕像", PM_LARGE_CAT, NEUTRAL, FIGURINE },
    { "大猫罐头", PM_LARGE_CAT, NEUTRAL, TIN },
    { "大猫的罐头", PM_LARGE_CAT, NEUTRAL, TIN },
    { "大猫肉罐头", PM_LARGE_CAT, NEUTRAL, TIN },
    { "老虎雕像", PM_TIGER, NEUTRAL, STATUE },
    { "老虎的雕像", PM_TIGER, NEUTRAL, STATUE },
    { "老虎小雕像", PM_TIGER, NEUTRAL, FIGURINE },
    { "老虎的小雕像", PM_TIGER, NEUTRAL, FIGURINE },
    { "老虎罐头", PM_TIGER, NEUTRAL, TIN },
    { "老虎的罐头", PM_TIGER, NEUTRAL, TIN },
    { "老虎肉罐头", PM_TIGER, NEUTRAL, TIN },
    { "幻影兽雕像", PM_DISPLACER_BEAST, NEUTRAL, STATUE },
    { "幻影兽的雕像", PM_DISPLACER_BEAST, NEUTRAL, STATUE },
    { "幻影兽小雕像", PM_DISPLACER_BEAST, NEUTRAL, FIGURINE },
    { "幻影兽的小雕像", PM_DISPLACER_BEAST, NEUTRAL, FIGURINE },
    { "幻影兽罐头", PM_DISPLACER_BEAST, NEUTRAL, TIN },
    { "幻影兽的罐头", PM_DISPLACER_BEAST, NEUTRAL, TIN },
    { "幻影兽肉罐头", PM_DISPLACER_BEAST, NEUTRAL, TIN },
    { "移位兽雕像", PM_DISPLACER_BEAST, NEUTRAL, STATUE },
    { "移位兽的雕像", PM_DISPLACER_BEAST, NEUTRAL, STATUE },
    { "移位兽小雕像", PM_DISPLACER_BEAST, NEUTRAL, FIGURINE },
    { "移位兽的小雕像", PM_DISPLACER_BEAST, NEUTRAL, FIGURINE },
    { "移位兽罐头", PM_DISPLACER_BEAST, NEUTRAL, TIN },
    { "移位兽的罐头", PM_DISPLACER_BEAST, NEUTRAL, TIN },
    { "移位兽肉罐头", PM_DISPLACER_BEAST, NEUTRAL, TIN },
    { "小鬼雕像", PM_GREMLIN, NEUTRAL, STATUE },
    { "小鬼的雕像", PM_GREMLIN, NEUTRAL, STATUE },
    { "小鬼小雕像", PM_GREMLIN, NEUTRAL, FIGURINE },
    { "小鬼的小雕像", PM_GREMLIN, NEUTRAL, FIGURINE },
    { "小鬼罐头", PM_GREMLIN, NEUTRAL, TIN },
    { "小鬼的罐头", PM_GREMLIN, NEUTRAL, TIN },
    { "小鬼肉罐头", PM_GREMLIN, NEUTRAL, TIN },
    { "石像鬼雕像", PM_GARGOYLE, NEUTRAL, STATUE },
    { "石像鬼的雕像", PM_GARGOYLE, NEUTRAL, STATUE },
    { "石像鬼小雕像", PM_GARGOYLE, NEUTRAL, FIGURINE },
    { "石像鬼的小雕像", PM_GARGOYLE, NEUTRAL, FIGURINE },
    { "石像鬼罐头", PM_GARGOYLE, NEUTRAL, TIN },
    { "石像鬼的罐头", PM_GARGOYLE, NEUTRAL, TIN },
    { "石像鬼肉罐头", PM_GARGOYLE, NEUTRAL, TIN },
    { "飞翼石像鬼雕像", PM_WINGED_GARGOYLE, NEUTRAL, STATUE },
    { "飞翼石像鬼的雕像", PM_WINGED_GARGOYLE, NEUTRAL, STATUE },
    { "飞翼石像鬼小雕像", PM_WINGED_GARGOYLE, NEUTRAL, FIGURINE },
    { "飞翼石像鬼的小雕像", PM_WINGED_GARGOYLE, NEUTRAL, FIGURINE },
    { "飞翼石像鬼罐头", PM_WINGED_GARGOYLE, NEUTRAL, TIN },
    { "飞翼石像鬼的罐头", PM_WINGED_GARGOYLE, NEUTRAL, TIN },
    { "飞翼石像鬼肉罐头", PM_WINGED_GARGOYLE, NEUTRAL, TIN },
    { "霍比特人雕像", PM_HOBBIT, NEUTRAL, STATUE },
    { "霍比特人的雕像", PM_HOBBIT, NEUTRAL, STATUE },
    { "霍比特人小雕像", PM_HOBBIT, NEUTRAL, FIGURINE },
    { "霍比特人的小雕像", PM_HOBBIT, NEUTRAL, FIGURINE },
    { "霍比特人罐头", PM_HOBBIT, NEUTRAL, TIN },
    { "霍比特人的罐头", PM_HOBBIT, NEUTRAL, TIN },
    { "霍比特人肉罐头", PM_HOBBIT, NEUTRAL, TIN },
    { "矮人雕像", PM_DWARF, NEUTRAL, STATUE },
    { "矮人的雕像", PM_DWARF, NEUTRAL, STATUE },
    { "矮人小雕像", PM_DWARF, NEUTRAL, FIGURINE },
    { "矮人的小雕像", PM_DWARF, NEUTRAL, FIGURINE },
    { "矮人罐头", PM_DWARF, NEUTRAL, TIN },
    { "矮人的罐头", PM_DWARF, NEUTRAL, TIN },
    { "矮人肉罐头", PM_DWARF, NEUTRAL, TIN },
    { "熊地精雕像", PM_BUGBEAR, NEUTRAL, STATUE },
    { "熊地精的雕像", PM_BUGBEAR, NEUTRAL, STATUE },
    { "熊地精小雕像", PM_BUGBEAR, NEUTRAL, FIGURINE },
    { "熊地精的小雕像", PM_BUGBEAR, NEUTRAL, FIGURINE },
    { "熊地精罐头", PM_BUGBEAR, NEUTRAL, TIN },
    { "熊地精的罐头", PM_BUGBEAR, NEUTRAL, TIN },
    { "熊地精肉罐头", PM_BUGBEAR, NEUTRAL, TIN },
    { "矮人领主雕像", PM_DWARF_LEADER, MALE, STATUE },
    { "矮人领主的雕像", PM_DWARF_LEADER, MALE, STATUE },
    { "矮人领主小雕像", PM_DWARF_LEADER, MALE, FIGURINE },
    { "矮人领主的小雕像", PM_DWARF_LEADER, MALE, FIGURINE },
    { "矮人领主罐头", PM_DWARF_LEADER, MALE, TIN },
    { "矮人领主的罐头", PM_DWARF_LEADER, MALE, TIN },
    { "矮人领主肉罐头", PM_DWARF_LEADER, MALE, TIN },
    { "矮人女领主雕像", PM_DWARF_LEADER, FEMALE, STATUE },
    { "矮人女领主的雕像", PM_DWARF_LEADER, FEMALE, STATUE },
    { "矮人女领主小雕像", PM_DWARF_LEADER, FEMALE, FIGURINE },
    { "矮人女领主的小雕像", PM_DWARF_LEADER, FEMALE, FIGURINE },
    { "矮人女领主罐头", PM_DWARF_LEADER, FEMALE, TIN },
    { "矮人女领主的罐头", PM_DWARF_LEADER, FEMALE, TIN },
    { "矮人女领主肉罐头", PM_DWARF_LEADER, FEMALE, TIN },
    { "矮人领袖雕像", PM_DWARF_LEADER, NEUTRAL, STATUE },
    { "矮人领袖的雕像", PM_DWARF_LEADER, NEUTRAL, STATUE },
    { "矮人领袖小雕像", PM_DWARF_LEADER, NEUTRAL, FIGURINE },
    { "矮人领袖的小雕像", PM_DWARF_LEADER, NEUTRAL, FIGURINE },
    { "矮人领袖罐头", PM_DWARF_LEADER, NEUTRAL, TIN },
    { "矮人领袖的罐头", PM_DWARF_LEADER, NEUTRAL, TIN },
    { "矮人领袖肉罐头", PM_DWARF_LEADER, NEUTRAL, TIN },
    { "矮人王雕像", PM_DWARF_RULER, MALE, STATUE },
    { "矮人王的雕像", PM_DWARF_RULER, MALE, STATUE },
    { "矮人王小雕像", PM_DWARF_RULER, MALE, FIGURINE },
    { "矮人王的小雕像", PM_DWARF_RULER, MALE, FIGURINE },
    { "矮人王罐头", PM_DWARF_RULER, MALE, TIN },
    { "矮人王的罐头", PM_DWARF_RULER, MALE, TIN },
    { "矮人王肉罐头", PM_DWARF_RULER, MALE, TIN },
    { "矮人女王雕像", PM_DWARF_RULER, FEMALE, STATUE },
    { "矮人女王的雕像", PM_DWARF_RULER, FEMALE, STATUE },
    { "矮人女王小雕像", PM_DWARF_RULER, FEMALE, FIGURINE },
    { "矮人女王的小雕像", PM_DWARF_RULER, FEMALE, FIGURINE },
    { "矮人女王罐头", PM_DWARF_RULER, FEMALE, TIN },
    { "矮人女王的罐头", PM_DWARF_RULER, FEMALE, TIN },
    { "矮人女王肉罐头", PM_DWARF_RULER, FEMALE, TIN },
    { "矮人统治者雕像", PM_DWARF_RULER, NEUTRAL, STATUE },
    { "矮人统治者的雕像", PM_DWARF_RULER, NEUTRAL, STATUE },
    { "矮人统治者小雕像", PM_DWARF_RULER, NEUTRAL, FIGURINE },
    { "矮人统治者的小雕像", PM_DWARF_RULER, NEUTRAL, FIGURINE },
    { "矮人统治者罐头", PM_DWARF_RULER, NEUTRAL, TIN },
    { "矮人统治者的罐头", PM_DWARF_RULER, NEUTRAL, TIN },
    { "矮人统治者肉罐头", PM_DWARF_RULER, NEUTRAL, TIN },
    { "夺心魔雕像", PM_MIND_FLAYER, NEUTRAL, STATUE },
    { "夺心魔的雕像", PM_MIND_FLAYER, NEUTRAL, STATUE },
    { "夺心魔小雕像", PM_MIND_FLAYER, NEUTRAL, FIGURINE },
    { "夺心魔的小雕像", PM_MIND_FLAYER, NEUTRAL, FIGURINE },
    { "夺心魔罐头", PM_MIND_FLAYER, NEUTRAL, TIN },
    { "夺心魔的罐头", PM_MIND_FLAYER, NEUTRAL, TIN },
    { "夺心魔肉罐头", PM_MIND_FLAYER, NEUTRAL, TIN },
    { "夺心魔大师雕像", PM_MASTER_MIND_FLAYER, NEUTRAL, STATUE },
    { "夺心魔大师的雕像", PM_MASTER_MIND_FLAYER, NEUTRAL, STATUE },
    { "夺心魔大师小雕像", PM_MASTER_MIND_FLAYER, NEUTRAL, FIGURINE },
    { "夺心魔大师的小雕像", PM_MASTER_MIND_FLAYER, NEUTRAL, FIGURINE },
    { "夺心魔大师罐头", PM_MASTER_MIND_FLAYER, NEUTRAL, TIN },
    { "夺心魔大师的罐头", PM_MASTER_MIND_FLAYER, NEUTRAL, TIN },
    { "夺心魔大师肉罐头", PM_MASTER_MIND_FLAYER, NEUTRAL, TIN },
    { "主宰夺心魔雕像", PM_MASTER_MIND_FLAYER, NEUTRAL, STATUE },
    { "主宰夺心魔的雕像", PM_MASTER_MIND_FLAYER, NEUTRAL, STATUE },
    { "主宰夺心魔小雕像", PM_MASTER_MIND_FLAYER, NEUTRAL, FIGURINE },
    { "主宰夺心魔的小雕像", PM_MASTER_MIND_FLAYER, NEUTRAL, FIGURINE },
    { "主宰夺心魔罐头", PM_MASTER_MIND_FLAYER, NEUTRAL, TIN },
    { "主宰夺心魔的罐头", PM_MASTER_MIND_FLAYER, NEUTRAL, TIN },
    { "主宰夺心魔肉罐头", PM_MASTER_MIND_FLAYER, NEUTRAL, TIN },
    { "灵魂雕像", PM_MANES, NEUTRAL, STATUE },
    { "灵魂的雕像", PM_MANES, NEUTRAL, STATUE },
    { "灵魂小雕像", PM_MANES, NEUTRAL, FIGURINE },
    { "灵魂的小雕像", PM_MANES, NEUTRAL, FIGURINE },
    { "灵魂罐头", PM_MANES, NEUTRAL, TIN },
    { "灵魂的罐头", PM_MANES, NEUTRAL, TIN },
    { "灵魂肉罐头", PM_MANES, NEUTRAL, TIN },
    { "幽魂雕像", PM_MANES, NEUTRAL, STATUE },
    { "幽魂的雕像", PM_MANES, NEUTRAL, STATUE },
    { "幽魂小雕像", PM_MANES, NEUTRAL, FIGURINE },
    { "幽魂的小雕像", PM_MANES, NEUTRAL, FIGURINE },
    { "幽魂罐头", PM_MANES, NEUTRAL, TIN },
    { "幽魂的罐头", PM_MANES, NEUTRAL, TIN },
    { "幽魂肉罐头", PM_MANES, NEUTRAL, TIN },
    { "雏形人雕像", PM_HOMUNCULUS, NEUTRAL, STATUE },
    { "雏形人的雕像", PM_HOMUNCULUS, NEUTRAL, STATUE },
    { "雏形人小雕像", PM_HOMUNCULUS, NEUTRAL, FIGURINE },
    { "雏形人的小雕像", PM_HOMUNCULUS, NEUTRAL, FIGURINE },
    { "雏形人罐头", PM_HOMUNCULUS, NEUTRAL, TIN },
    { "雏形人的罐头", PM_HOMUNCULUS, NEUTRAL, TIN },
    { "雏形人肉罐头", PM_HOMUNCULUS, NEUTRAL, TIN },
    { "人造人雕像", PM_HOMUNCULUS, NEUTRAL, STATUE },
    { "人造人的雕像", PM_HOMUNCULUS, NEUTRAL, STATUE },
    { "人造人小雕像", PM_HOMUNCULUS, NEUTRAL, FIGURINE },
    { "人造人的小雕像", PM_HOMUNCULUS, NEUTRAL, FIGURINE },
    { "人造人罐头", PM_HOMUNCULUS, NEUTRAL, TIN },
    { "人造人的罐头", PM_HOMUNCULUS, NEUTRAL, TIN },
    { "人造人肉罐头", PM_HOMUNCULUS, NEUTRAL, TIN },
    { "造妖雕像", PM_HOMUNCULUS, NEUTRAL, STATUE },
    { "造妖的雕像", PM_HOMUNCULUS, NEUTRAL, STATUE },
    { "造妖小雕像", PM_HOMUNCULUS, NEUTRAL, FIGURINE },
    { "造妖的小雕像", PM_HOMUNCULUS, NEUTRAL, FIGURINE },
    { "造妖罐头", PM_HOMUNCULUS, NEUTRAL, TIN },
    { "造妖的罐头", PM_HOMUNCULUS, NEUTRAL, TIN },
    { "造妖肉罐头", PM_HOMUNCULUS, NEUTRAL, TIN },
    { "小恶魔雕像", PM_IMP, NEUTRAL, STATUE },
    { "小恶魔的雕像", PM_IMP, NEUTRAL, STATUE },
    { "小恶魔小雕像", PM_IMP, NEUTRAL, FIGURINE },
    { "小恶魔的小雕像", PM_IMP, NEUTRAL, FIGURINE },
    { "小恶魔罐头", PM_IMP, NEUTRAL, TIN },
    { "小恶魔的罐头", PM_IMP, NEUTRAL, TIN },
    { "小恶魔肉罐头", PM_IMP, NEUTRAL, TIN },
    { "劣魔雕像", PM_LEMURE, NEUTRAL, STATUE },
    { "劣魔的雕像", PM_LEMURE, NEUTRAL, STATUE },
    { "劣魔小雕像", PM_LEMURE, NEUTRAL, FIGURINE },
    { "劣魔的小雕像", PM_LEMURE, NEUTRAL, FIGURINE },
    { "劣魔罐头", PM_LEMURE, NEUTRAL, TIN },
    { "劣魔的罐头", PM_LEMURE, NEUTRAL, TIN },
    { "劣魔肉罐头", PM_LEMURE, NEUTRAL, TIN },
    { "夸塞魔雕像", PM_QUASIT, NEUTRAL, STATUE },
    { "夸塞魔的雕像", PM_QUASIT, NEUTRAL, STATUE },
    { "夸塞魔小雕像", PM_QUASIT, NEUTRAL, FIGURINE },
    { "夸塞魔的小雕像", PM_QUASIT, NEUTRAL, FIGURINE },
    { "夸塞魔罐头", PM_QUASIT, NEUTRAL, TIN },
    { "夸塞魔的罐头", PM_QUASIT, NEUTRAL, TIN },
    { "夸塞魔肉罐头", PM_QUASIT, NEUTRAL, TIN },
    { "天狗雕像", PM_TENGU, NEUTRAL, STATUE },
    { "天狗的雕像", PM_TENGU, NEUTRAL, STATUE },
    { "天狗小雕像", PM_TENGU, NEUTRAL, FIGURINE },
    { "天狗的小雕像", PM_TENGU, NEUTRAL, FIGURINE },
    { "天狗罐头", PM_TENGU, NEUTRAL, TIN },
    { "天狗的罐头", PM_TENGU, NEUTRAL, TIN },
    { "天狗肉罐头", PM_TENGU, NEUTRAL, TIN },
    { "蓝色果冻雕像", PM_BLUE_JELLY, NEUTRAL, STATUE },
    { "蓝色果冻的雕像", PM_BLUE_JELLY, NEUTRAL, STATUE },
    { "蓝色果冻小雕像", PM_BLUE_JELLY, NEUTRAL, FIGURINE },
    { "蓝色果冻的小雕像", PM_BLUE_JELLY, NEUTRAL, FIGURINE },
    { "蓝色果冻罐头", PM_BLUE_JELLY, NEUTRAL, TIN },
    { "蓝色果冻的罐头", PM_BLUE_JELLY, NEUTRAL, TIN },
    { "蓝色果冻肉罐头", PM_BLUE_JELLY, NEUTRAL, TIN },
    { "蓝冻怪雕像", PM_BLUE_JELLY, NEUTRAL, STATUE },
    { "蓝冻怪的雕像", PM_BLUE_JELLY, NEUTRAL, STATUE },
    { "蓝冻怪小雕像", PM_BLUE_JELLY, NEUTRAL, FIGURINE },
    { "蓝冻怪的小雕像", PM_BLUE_JELLY, NEUTRAL, FIGURINE },
    { "蓝冻怪罐头", PM_BLUE_JELLY, NEUTRAL, TIN },
    { "蓝冻怪的罐头", PM_BLUE_JELLY, NEUTRAL, TIN },
    { "蓝冻怪肉罐头", PM_BLUE_JELLY, NEUTRAL, TIN },
    { "珍珠果冻雕像", PM_SPOTTED_JELLY, NEUTRAL, STATUE },
    { "珍珠果冻的雕像", PM_SPOTTED_JELLY, NEUTRAL, STATUE },
    { "珍珠果冻小雕像", PM_SPOTTED_JELLY, NEUTRAL, FIGURINE },
    { "珍珠果冻的小雕像", PM_SPOTTED_JELLY, NEUTRAL, FIGURINE },
    { "珍珠果冻罐头", PM_SPOTTED_JELLY, NEUTRAL, TIN },
    { "珍珠果冻的罐头", PM_SPOTTED_JELLY, NEUTRAL, TIN },
    { "珍珠果冻肉罐头", PM_SPOTTED_JELLY, NEUTRAL, TIN },
    { "斑点凝胶怪雕像", PM_SPOTTED_JELLY, NEUTRAL, STATUE },
    { "斑点凝胶怪的雕像", PM_SPOTTED_JELLY, NEUTRAL, STATUE },
    { "斑点凝胶怪小雕像", PM_SPOTTED_JELLY, NEUTRAL, FIGURINE },
    { "斑点凝胶怪的小雕像", PM_SPOTTED_JELLY, NEUTRAL, FIGURINE },
    { "斑点凝胶怪罐头", PM_SPOTTED_JELLY, NEUTRAL, TIN },
    { "斑点凝胶怪的罐头", PM_SPOTTED_JELLY, NEUTRAL, TIN },
    { "斑点凝胶怪肉罐头", PM_SPOTTED_JELLY, NEUTRAL, TIN },
    { "斑冻怪雕像", PM_SPOTTED_JELLY, NEUTRAL, STATUE },
    { "斑冻怪的雕像", PM_SPOTTED_JELLY, NEUTRAL, STATUE },
    { "斑冻怪小雕像", PM_SPOTTED_JELLY, NEUTRAL, FIGURINE },
    { "斑冻怪的小雕像", PM_SPOTTED_JELLY, NEUTRAL, FIGURINE },
    { "斑冻怪罐头", PM_SPOTTED_JELLY, NEUTRAL, TIN },
    { "斑冻怪的罐头", PM_SPOTTED_JELLY, NEUTRAL, TIN },
    { "斑冻怪肉罐头", PM_SPOTTED_JELLY, NEUTRAL, TIN },
    { "赭冻怪雕像", PM_OCHRE_JELLY, NEUTRAL, STATUE },
    { "赭冻怪的雕像", PM_OCHRE_JELLY, NEUTRAL, STATUE },
    { "赭冻怪小雕像", PM_OCHRE_JELLY, NEUTRAL, FIGURINE },
    { "赭冻怪的小雕像", PM_OCHRE_JELLY, NEUTRAL, FIGURINE },
    { "赭冻怪罐头", PM_OCHRE_JELLY, NEUTRAL, TIN },
    { "赭冻怪的罐头", PM_OCHRE_JELLY, NEUTRAL, TIN },
    { "赭冻怪肉罐头", PM_OCHRE_JELLY, NEUTRAL, TIN },
    { "赭色凝胶怪雕像", PM_OCHRE_JELLY, NEUTRAL, STATUE },
    { "赭色凝胶怪的雕像", PM_OCHRE_JELLY, NEUTRAL, STATUE },
    { "赭色凝胶怪小雕像", PM_OCHRE_JELLY, NEUTRAL, FIGURINE },
    { "赭色凝胶怪的小雕像", PM_OCHRE_JELLY, NEUTRAL, FIGURINE },
    { "赭色凝胶怪罐头", PM_OCHRE_JELLY, NEUTRAL, TIN },
    { "赭色凝胶怪的罐头", PM_OCHRE_JELLY, NEUTRAL, TIN },
    { "赭色凝胶怪肉罐头", PM_OCHRE_JELLY, NEUTRAL, TIN },
    { "狗头人雕像", PM_KOBOLD, NEUTRAL, STATUE },
    { "狗头人的雕像", PM_KOBOLD, NEUTRAL, STATUE },
    { "狗头人小雕像", PM_KOBOLD, NEUTRAL, FIGURINE },
    { "狗头人的小雕像", PM_KOBOLD, NEUTRAL, FIGURINE },
    { "狗头人罐头", PM_KOBOLD, NEUTRAL, TIN },
    { "狗头人的罐头", PM_KOBOLD, NEUTRAL, TIN },
    { "狗头人肉罐头", PM_KOBOLD, NEUTRAL, TIN },
    { "大狗头人雕像", PM_LARGE_KOBOLD, NEUTRAL, STATUE },
    { "大狗头人的雕像", PM_LARGE_KOBOLD, NEUTRAL, STATUE },
    { "大狗头人小雕像", PM_LARGE_KOBOLD, NEUTRAL, FIGURINE },
    { "大狗头人的小雕像", PM_LARGE_KOBOLD, NEUTRAL, FIGURINE },
    { "大狗头人罐头", PM_LARGE_KOBOLD, NEUTRAL, TIN },
    { "大狗头人的罐头", PM_LARGE_KOBOLD, NEUTRAL, TIN },
    { "大狗头人肉罐头", PM_LARGE_KOBOLD, NEUTRAL, TIN },
    { "狗头人领主雕像", PM_KOBOLD_LEADER, MALE, STATUE },
    { "狗头人领主的雕像", PM_KOBOLD_LEADER, MALE, STATUE },
    { "狗头人领主小雕像", PM_KOBOLD_LEADER, MALE, FIGURINE },
    { "狗头人领主的小雕像", PM_KOBOLD_LEADER, MALE, FIGURINE },
    { "狗头人领主罐头", PM_KOBOLD_LEADER, MALE, TIN },
    { "狗头人领主的罐头", PM_KOBOLD_LEADER, MALE, TIN },
    { "狗头人领主肉罐头", PM_KOBOLD_LEADER, MALE, TIN },
    { "狗头人女领主雕像", PM_KOBOLD_LEADER, FEMALE, STATUE },
    { "狗头人女领主的雕像", PM_KOBOLD_LEADER, FEMALE, STATUE },
    { "狗头人女领主小雕像", PM_KOBOLD_LEADER, FEMALE, FIGURINE },
    { "狗头人女领主的小雕像", PM_KOBOLD_LEADER, FEMALE, FIGURINE },
    { "狗头人女领主罐头", PM_KOBOLD_LEADER, FEMALE, TIN },
    { "狗头人女领主的罐头", PM_KOBOLD_LEADER, FEMALE, TIN },
    { "狗头人女领主肉罐头", PM_KOBOLD_LEADER, FEMALE, TIN },
    { "狗头人领袖雕像", PM_KOBOLD_LEADER, NEUTRAL, STATUE },
    { "狗头人领袖的雕像", PM_KOBOLD_LEADER, NEUTRAL, STATUE },
    { "狗头人领袖小雕像", PM_KOBOLD_LEADER, NEUTRAL, FIGURINE },
    { "狗头人领袖的小雕像", PM_KOBOLD_LEADER, NEUTRAL, FIGURINE },
    { "狗头人领袖罐头", PM_KOBOLD_LEADER, NEUTRAL, TIN },
    { "狗头人领袖的罐头", PM_KOBOLD_LEADER, NEUTRAL, TIN },
    { "狗头人领袖肉罐头", PM_KOBOLD_LEADER, NEUTRAL, TIN },
    { "狗头人萨满雕像", PM_KOBOLD_SHAMAN, NEUTRAL, STATUE },
    { "狗头人萨满的雕像", PM_KOBOLD_SHAMAN, NEUTRAL, STATUE },
    { "狗头人萨满小雕像", PM_KOBOLD_SHAMAN, NEUTRAL, FIGURINE },
    { "狗头人萨满的小雕像", PM_KOBOLD_SHAMAN, NEUTRAL, FIGURINE },
    { "狗头人萨满罐头", PM_KOBOLD_SHAMAN, NEUTRAL, TIN },
    { "狗头人萨满的罐头", PM_KOBOLD_SHAMAN, NEUTRAL, TIN },
    { "狗头人萨满肉罐头", PM_KOBOLD_SHAMAN, NEUTRAL, TIN },
    { "小矮妖雕像", PM_LEPRECHAUN, NEUTRAL, STATUE },
    { "小矮妖的雕像", PM_LEPRECHAUN, NEUTRAL, STATUE },
    { "小矮妖小雕像", PM_LEPRECHAUN, NEUTRAL, FIGURINE },
    { "小矮妖的小雕像", PM_LEPRECHAUN, NEUTRAL, FIGURINE },
    { "小矮妖罐头", PM_LEPRECHAUN, NEUTRAL, TIN },
    { "小矮妖的罐头", PM_LEPRECHAUN, NEUTRAL, TIN },
    { "小矮妖肉罐头", PM_LEPRECHAUN, NEUTRAL, TIN },
    { "小拟形怪雕像", PM_SMALL_MIMIC, NEUTRAL, STATUE },
    { "小拟形怪的雕像", PM_SMALL_MIMIC, NEUTRAL, STATUE },
    { "小拟形怪小雕像", PM_SMALL_MIMIC, NEUTRAL, FIGURINE },
    { "小拟形怪的小雕像", PM_SMALL_MIMIC, NEUTRAL, FIGURINE },
    { "小拟形怪罐头", PM_SMALL_MIMIC, NEUTRAL, TIN },
    { "小拟形怪的罐头", PM_SMALL_MIMIC, NEUTRAL, TIN },
    { "小拟形怪肉罐头", PM_SMALL_MIMIC, NEUTRAL, TIN },
    { "大拟形怪雕像", PM_LARGE_MIMIC, NEUTRAL, STATUE },
    { "大拟形怪的雕像", PM_LARGE_MIMIC, NEUTRAL, STATUE },
    { "大拟形怪小雕像", PM_LARGE_MIMIC, NEUTRAL, FIGURINE },
    { "大拟形怪的小雕像", PM_LARGE_MIMIC, NEUTRAL, FIGURINE },
    { "大拟形怪罐头", PM_LARGE_MIMIC, NEUTRAL, TIN },
    { "大拟形怪的罐头", PM_LARGE_MIMIC, NEUTRAL, TIN },
    { "大拟形怪肉罐头", PM_LARGE_MIMIC, NEUTRAL, TIN },
    { "巨型拟形怪雕像", PM_GIANT_MIMIC, NEUTRAL, STATUE },
    { "巨型拟形怪的雕像", PM_GIANT_MIMIC, NEUTRAL, STATUE },
    { "巨型拟形怪小雕像", PM_GIANT_MIMIC, NEUTRAL, FIGURINE },
    { "巨型拟形怪的小雕像", PM_GIANT_MIMIC, NEUTRAL, FIGURINE },
    { "巨型拟形怪罐头", PM_GIANT_MIMIC, NEUTRAL, TIN },
    { "巨型拟形怪的罐头", PM_GIANT_MIMIC, NEUTRAL, TIN },
    { "巨型拟形怪肉罐头", PM_GIANT_MIMIC, NEUTRAL, TIN },
    { "木仙女雕像", PM_WOOD_NYMPH, NEUTRAL, STATUE },
    { "木仙女的雕像", PM_WOOD_NYMPH, NEUTRAL, STATUE },
    { "木仙女小雕像", PM_WOOD_NYMPH, NEUTRAL, FIGURINE },
    { "木仙女的小雕像", PM_WOOD_NYMPH, NEUTRAL, FIGURINE },
    { "木仙女罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "木仙女的罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "木仙女肉罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "水仙女雕像", PM_WATER_NYMPH, NEUTRAL, STATUE },
    { "水仙女的雕像", PM_WATER_NYMPH, NEUTRAL, STATUE },
    { "水仙女小雕像", PM_WATER_NYMPH, NEUTRAL, FIGURINE },
    { "水仙女的小雕像", PM_WATER_NYMPH, NEUTRAL, FIGURINE },
    { "水仙女罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "水仙女的罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "水仙女肉罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "山仙女雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, STATUE },
    { "山仙女的雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, STATUE },
    { "山仙女小雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, FIGURINE },
    { "山仙女的小雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, FIGURINE },
    { "山仙女罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "山仙女的罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "山仙女肉罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "木仙子雕像", PM_WOOD_NYMPH, NEUTRAL, STATUE },
    { "木仙子的雕像", PM_WOOD_NYMPH, NEUTRAL, STATUE },
    { "木仙子小雕像", PM_WOOD_NYMPH, NEUTRAL, FIGURINE },
    { "木仙子的小雕像", PM_WOOD_NYMPH, NEUTRAL, FIGURINE },
    { "木仙子罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "木仙子的罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "木仙子肉罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "水仙子雕像", PM_WATER_NYMPH, NEUTRAL, STATUE },
    { "水仙子的雕像", PM_WATER_NYMPH, NEUTRAL, STATUE },
    { "水仙子小雕像", PM_WATER_NYMPH, NEUTRAL, FIGURINE },
    { "水仙子的小雕像", PM_WATER_NYMPH, NEUTRAL, FIGURINE },
    { "水仙子罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "水仙子的罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "水仙子肉罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "山仙子雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, STATUE },
    { "山仙子的雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, STATUE },
    { "山仙子小雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, FIGURINE },
    { "山仙子的小雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, FIGURINE },
    { "山仙子罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "山仙子的罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "山仙子肉罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "木妖精雕像", PM_WOOD_NYMPH, NEUTRAL, STATUE },
    { "木妖精的雕像", PM_WOOD_NYMPH, NEUTRAL, STATUE },
    { "木妖精小雕像", PM_WOOD_NYMPH, NEUTRAL, FIGURINE },
    { "木妖精的小雕像", PM_WOOD_NYMPH, NEUTRAL, FIGURINE },
    { "木妖精罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "木妖精的罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "木妖精肉罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "水妖精雕像", PM_WATER_NYMPH, NEUTRAL, STATUE },
    { "水妖精的雕像", PM_WATER_NYMPH, NEUTRAL, STATUE },
    { "水妖精小雕像", PM_WATER_NYMPH, NEUTRAL, FIGURINE },
    { "水妖精的小雕像", PM_WATER_NYMPH, NEUTRAL, FIGURINE },
    { "水妖精罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "水妖精的罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "水妖精肉罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "山妖精雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, STATUE },
    { "山妖精的雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, STATUE },
    { "山妖精小雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, FIGURINE },
    { "山妖精的小雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, FIGURINE },
    { "山妖精罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "山妖精的罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "山妖精肉罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "木宁芙雕像", PM_WOOD_NYMPH, NEUTRAL, STATUE },
    { "木宁芙的雕像", PM_WOOD_NYMPH, NEUTRAL, STATUE },
    { "木宁芙小雕像", PM_WOOD_NYMPH, NEUTRAL, FIGURINE },
    { "木宁芙的小雕像", PM_WOOD_NYMPH, NEUTRAL, FIGURINE },
    { "木宁芙罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "木宁芙的罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "木宁芙肉罐头", PM_WOOD_NYMPH, NEUTRAL, TIN },
    { "水宁芙雕像", PM_WATER_NYMPH, NEUTRAL, STATUE },
    { "水宁芙的雕像", PM_WATER_NYMPH, NEUTRAL, STATUE },
    { "水宁芙小雕像", PM_WATER_NYMPH, NEUTRAL, FIGURINE },
    { "水宁芙的小雕像", PM_WATER_NYMPH, NEUTRAL, FIGURINE },
    { "水宁芙罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "水宁芙的罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "水宁芙肉罐头", PM_WATER_NYMPH, NEUTRAL, TIN },
    { "山宁芙雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, STATUE },
    { "山宁芙的雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, STATUE },
    { "山宁芙小雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, FIGURINE },
    { "山宁芙的小雕像", PM_MOUNTAIN_NYMPH, NEUTRAL, FIGURINE },
    { "山宁芙罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "山宁芙的罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "山宁芙肉罐头", PM_MOUNTAIN_NYMPH, NEUTRAL, TIN },
    { "地精雕像", PM_GOBLIN, NEUTRAL, STATUE },
    { "地精的雕像", PM_GOBLIN, NEUTRAL, STATUE },
    { "地精小雕像", PM_GOBLIN, NEUTRAL, FIGURINE },
    { "地精的小雕像", PM_GOBLIN, NEUTRAL, FIGURINE },
    { "地精罐头", PM_GOBLIN, NEUTRAL, TIN },
    { "地精的罐头", PM_GOBLIN, NEUTRAL, TIN },
    { "地精肉罐头", PM_GOBLIN, NEUTRAL, TIN },
    { "哥布林雕像", PM_GOBLIN, NEUTRAL, STATUE },
    { "哥布林的雕像", PM_GOBLIN, NEUTRAL, STATUE },
    { "哥布林小雕像", PM_GOBLIN, NEUTRAL, FIGURINE },
    { "哥布林的小雕像", PM_GOBLIN, NEUTRAL, FIGURINE },
    { "哥布林罐头", PM_GOBLIN, NEUTRAL, TIN },
    { "哥布林的罐头", PM_GOBLIN, NEUTRAL, TIN },
    { "哥布林肉罐头", PM_GOBLIN, NEUTRAL, TIN },
    { "大地精雕像", PM_HOBGOBLIN, NEUTRAL, STATUE },
    { "大地精的雕像", PM_HOBGOBLIN, NEUTRAL, STATUE },
    { "大地精小雕像", PM_HOBGOBLIN, NEUTRAL, FIGURINE },
    { "大地精的小雕像", PM_HOBGOBLIN, NEUTRAL, FIGURINE },
    { "大地精罐头", PM_HOBGOBLIN, NEUTRAL, TIN },
    { "大地精的罐头", PM_HOBGOBLIN, NEUTRAL, TIN },
    { "大地精肉罐头", PM_HOBGOBLIN, NEUTRAL, TIN },
    { "大哥布林雕像", PM_HOBGOBLIN, NEUTRAL, STATUE },
    { "大哥布林的雕像", PM_HOBGOBLIN, NEUTRAL, STATUE },
    { "大哥布林小雕像", PM_HOBGOBLIN, NEUTRAL, FIGURINE },
    { "大哥布林的小雕像", PM_HOBGOBLIN, NEUTRAL, FIGURINE },
    { "大哥布林罐头", PM_HOBGOBLIN, NEUTRAL, TIN },
    { "大哥布林的罐头", PM_HOBGOBLIN, NEUTRAL, TIN },
    { "大哥布林肉罐头", PM_HOBGOBLIN, NEUTRAL, TIN },
    { "兽人雕像", PM_ORC, NEUTRAL, STATUE },
    { "兽人的雕像", PM_ORC, NEUTRAL, STATUE },
    { "兽人小雕像", PM_ORC, NEUTRAL, FIGURINE },
    { "兽人的小雕像", PM_ORC, NEUTRAL, FIGURINE },
    { "兽人罐头", PM_ORC, NEUTRAL, TIN },
    { "兽人的罐头", PM_ORC, NEUTRAL, TIN },
    { "兽人肉罐头", PM_ORC, NEUTRAL, TIN },
    { "丘陵兽人雕像", PM_HILL_ORC, NEUTRAL, STATUE },
    { "丘陵兽人的雕像", PM_HILL_ORC, NEUTRAL, STATUE },
    { "丘陵兽人小雕像", PM_HILL_ORC, NEUTRAL, FIGURINE },
    { "丘陵兽人的小雕像", PM_HILL_ORC, NEUTRAL, FIGURINE },
    { "丘陵兽人罐头", PM_HILL_ORC, NEUTRAL, TIN },
    { "丘陵兽人的罐头", PM_HILL_ORC, NEUTRAL, TIN },
    { "丘陵兽人肉罐头", PM_HILL_ORC, NEUTRAL, TIN },
    { "魔多兽人雕像", PM_MORDOR_ORC, NEUTRAL, STATUE },
    { "魔多兽人的雕像", PM_MORDOR_ORC, NEUTRAL, STATUE },
    { "魔多兽人小雕像", PM_MORDOR_ORC, NEUTRAL, FIGURINE },
    { "魔多兽人的小雕像", PM_MORDOR_ORC, NEUTRAL, FIGURINE },
    { "魔多兽人罐头", PM_MORDOR_ORC, NEUTRAL, TIN },
    { "魔多兽人的罐头", PM_MORDOR_ORC, NEUTRAL, TIN },
    { "魔多兽人肉罐头", PM_MORDOR_ORC, NEUTRAL, TIN },
    { "强兽人雕像", PM_URUK_HAI, NEUTRAL, STATUE },
    { "强兽人的雕像", PM_URUK_HAI, NEUTRAL, STATUE },
    { "强兽人小雕像", PM_URUK_HAI, NEUTRAL, FIGURINE },
    { "强兽人的小雕像", PM_URUK_HAI, NEUTRAL, FIGURINE },
    { "强兽人罐头", PM_URUK_HAI, NEUTRAL, TIN },
    { "强兽人的罐头", PM_URUK_HAI, NEUTRAL, TIN },
    { "强兽人肉罐头", PM_URUK_HAI, NEUTRAL, TIN },
    { "乌鲁克雕像", PM_URUK_HAI, NEUTRAL, STATUE },
    { "乌鲁克的雕像", PM_URUK_HAI, NEUTRAL, STATUE },
    { "乌鲁克小雕像", PM_URUK_HAI, NEUTRAL, FIGURINE },
    { "乌鲁克的小雕像", PM_URUK_HAI, NEUTRAL, FIGURINE },
    { "乌鲁克罐头", PM_URUK_HAI, NEUTRAL, TIN },
    { "乌鲁克的罐头", PM_URUK_HAI, NEUTRAL, TIN },
    { "乌鲁克肉罐头", PM_URUK_HAI, NEUTRAL, TIN },
    { "兽人萨满雕像", PM_ORC_SHAMAN, NEUTRAL, STATUE },
    { "兽人萨满的雕像", PM_ORC_SHAMAN, NEUTRAL, STATUE },
    { "兽人萨满小雕像", PM_ORC_SHAMAN, NEUTRAL, FIGURINE },
    { "兽人萨满的小雕像", PM_ORC_SHAMAN, NEUTRAL, FIGURINE },
    { "兽人萨满罐头", PM_ORC_SHAMAN, NEUTRAL, TIN },
    { "兽人萨满的罐头", PM_ORC_SHAMAN, NEUTRAL, TIN },
    { "兽人萨满肉罐头", PM_ORC_SHAMAN, NEUTRAL, TIN },
    { "兽人队长雕像", PM_ORC_CAPTAIN, NEUTRAL, STATUE },
    { "兽人队长的雕像", PM_ORC_CAPTAIN, NEUTRAL, STATUE },
    { "兽人队长小雕像", PM_ORC_CAPTAIN, NEUTRAL, FIGURINE },
    { "兽人队长的小雕像", PM_ORC_CAPTAIN, NEUTRAL, FIGURINE },
    { "兽人队长罐头", PM_ORC_CAPTAIN, NEUTRAL, TIN },
    { "兽人队长的罐头", PM_ORC_CAPTAIN, NEUTRAL, TIN },
    { "兽人队长肉罐头", PM_ORC_CAPTAIN, NEUTRAL, TIN },
    { "岩石锥子雕像", PM_ROCK_PIERCER, NEUTRAL, STATUE },
    { "岩石锥子的雕像", PM_ROCK_PIERCER, NEUTRAL, STATUE },
    { "岩石锥子小雕像", PM_ROCK_PIERCER, NEUTRAL, FIGURINE },
    { "岩石锥子的小雕像", PM_ROCK_PIERCER, NEUTRAL, FIGURINE },
    { "岩石锥子罐头", PM_ROCK_PIERCER, NEUTRAL, TIN },
    { "岩石锥子的罐头", PM_ROCK_PIERCER, NEUTRAL, TIN },
    { "岩石锥子肉罐头", PM_ROCK_PIERCER, NEUTRAL, TIN },
    { "铁锥子雕像", PM_IRON_PIERCER, NEUTRAL, STATUE },
    { "铁锥子的雕像", PM_IRON_PIERCER, NEUTRAL, STATUE },
    { "铁锥子小雕像", PM_IRON_PIERCER, NEUTRAL, FIGURINE },
    { "铁锥子的小雕像", PM_IRON_PIERCER, NEUTRAL, FIGURINE },
    { "铁锥子罐头", PM_IRON_PIERCER, NEUTRAL, TIN },
    { "铁锥子的罐头", PM_IRON_PIERCER, NEUTRAL, TIN },
    { "铁锥子肉罐头", PM_IRON_PIERCER, NEUTRAL, TIN },
    { "玻璃锥子雕像", PM_GLASS_PIERCER, NEUTRAL, STATUE },
    { "玻璃锥子的雕像", PM_GLASS_PIERCER, NEUTRAL, STATUE },
    { "玻璃锥子小雕像", PM_GLASS_PIERCER, NEUTRAL, FIGURINE },
    { "玻璃锥子的小雕像", PM_GLASS_PIERCER, NEUTRAL, FIGURINE },
    { "玻璃锥子罐头", PM_GLASS_PIERCER, NEUTRAL, TIN },
    { "玻璃锥子的罐头", PM_GLASS_PIERCER, NEUTRAL, TIN },
    { "玻璃锥子肉罐头", PM_GLASS_PIERCER, NEUTRAL, TIN },
    { "洛斯兽雕像", PM_ROTHE, NEUTRAL, STATUE },
    { "洛斯兽的雕像", PM_ROTHE, NEUTRAL, STATUE },
    { "洛斯兽小雕像", PM_ROTHE, NEUTRAL, FIGURINE },
    { "洛斯兽的小雕像", PM_ROTHE, NEUTRAL, FIGURINE },
    { "洛斯兽罐头", PM_ROTHE, NEUTRAL, TIN },
    { "洛斯兽的罐头", PM_ROTHE, NEUTRAL, TIN },
    { "洛斯兽肉罐头", PM_ROTHE, NEUTRAL, TIN },
    { "猛犸雕像", PM_MUMAK, NEUTRAL, STATUE },
    { "猛犸的雕像", PM_MUMAK, NEUTRAL, STATUE },
    { "猛犸小雕像", PM_MUMAK, NEUTRAL, FIGURINE },
    { "猛犸的小雕像", PM_MUMAK, NEUTRAL, FIGURINE },
    { "猛犸罐头", PM_MUMAK, NEUTRAL, TIN },
    { "猛犸的罐头", PM_MUMAK, NEUTRAL, TIN },
    { "猛犸肉罐头", PM_MUMAK, NEUTRAL, TIN },
    { "狼狗雕像", PM_LEOCROTTA, NEUTRAL, STATUE },
    { "狼狗的雕像", PM_LEOCROTTA, NEUTRAL, STATUE },
    { "狼狗小雕像", PM_LEOCROTTA, NEUTRAL, FIGURINE },
    { "狼狗的小雕像", PM_LEOCROTTA, NEUTRAL, FIGURINE },
    { "狼狗罐头", PM_LEOCROTTA, NEUTRAL, TIN },
    { "狼狗的罐头", PM_LEOCROTTA, NEUTRAL, TIN },
    { "狼狗肉罐头", PM_LEOCROTTA, NEUTRAL, TIN },
    { "狮头象雕像", PM_WUMPUS, NEUTRAL, STATUE },
    { "狮头象的雕像", PM_WUMPUS, NEUTRAL, STATUE },
    { "狮头象小雕像", PM_WUMPUS, NEUTRAL, FIGURINE },
    { "狮头象的小雕像", PM_WUMPUS, NEUTRAL, FIGURINE },
    { "狮头象罐头", PM_WUMPUS, NEUTRAL, TIN },
    { "狮头象的罐头", PM_WUMPUS, NEUTRAL, TIN },
    { "狮头象肉罐头", PM_WUMPUS, NEUTRAL, TIN },
    { "雷兽雕像", PM_TITANOTHERE, NEUTRAL, STATUE },
    { "雷兽的雕像", PM_TITANOTHERE, NEUTRAL, STATUE },
    { "雷兽小雕像", PM_TITANOTHERE, NEUTRAL, FIGURINE },
    { "雷兽的小雕像", PM_TITANOTHERE, NEUTRAL, FIGURINE },
    { "雷兽罐头", PM_TITANOTHERE, NEUTRAL, TIN },
    { "雷兽的罐头", PM_TITANOTHERE, NEUTRAL, TIN },
    { "雷兽肉罐头", PM_TITANOTHERE, NEUTRAL, TIN },
    { "俾路支兽雕像", PM_BALUCHITHERIUM, NEUTRAL, STATUE },
    { "俾路支兽的雕像", PM_BALUCHITHERIUM, NEUTRAL, STATUE },
    { "俾路支兽小雕像", PM_BALUCHITHERIUM, NEUTRAL, FIGURINE },
    { "俾路支兽的小雕像", PM_BALUCHITHERIUM, NEUTRAL, FIGURINE },
    { "俾路支兽罐头", PM_BALUCHITHERIUM, NEUTRAL, TIN },
    { "俾路支兽的罐头", PM_BALUCHITHERIUM, NEUTRAL, TIN },
    { "俾路支兽肉罐头", PM_BALUCHITHERIUM, NEUTRAL, TIN },
    { "巨犀雕像", PM_BALUCHITHERIUM, NEUTRAL, STATUE },
    { "巨犀的雕像", PM_BALUCHITHERIUM, NEUTRAL, STATUE },
    { "巨犀小雕像", PM_BALUCHITHERIUM, NEUTRAL, FIGURINE },
    { "巨犀的小雕像", PM_BALUCHITHERIUM, NEUTRAL, FIGURINE },
    { "巨犀罐头", PM_BALUCHITHERIUM, NEUTRAL, TIN },
    { "巨犀的罐头", PM_BALUCHITHERIUM, NEUTRAL, TIN },
    { "巨犀肉罐头", PM_BALUCHITHERIUM, NEUTRAL, TIN },
    { "乳齿象雕像", PM_MASTODON, NEUTRAL, STATUE },
    { "乳齿象的雕像", PM_MASTODON, NEUTRAL, STATUE },
    { "乳齿象小雕像", PM_MASTODON, NEUTRAL, FIGURINE },
    { "乳齿象的小雕像", PM_MASTODON, NEUTRAL, FIGURINE },
    { "乳齿象罐头", PM_MASTODON, NEUTRAL, TIN },
    { "乳齿象的罐头", PM_MASTODON, NEUTRAL, TIN },
    { "乳齿象肉罐头", PM_MASTODON, NEUTRAL, TIN },
    { "褐鼠雕像", PM_SEWER_RAT, NEUTRAL, STATUE },
    { "褐鼠的雕像", PM_SEWER_RAT, NEUTRAL, STATUE },
    { "褐鼠小雕像", PM_SEWER_RAT, NEUTRAL, FIGURINE },
    { "褐鼠的小雕像", PM_SEWER_RAT, NEUTRAL, FIGURINE },
    { "褐鼠罐头", PM_SEWER_RAT, NEUTRAL, TIN },
    { "褐鼠的罐头", PM_SEWER_RAT, NEUTRAL, TIN },
    { "褐鼠肉罐头", PM_SEWER_RAT, NEUTRAL, TIN },
    { "巨鼠雕像", PM_GIANT_RAT, NEUTRAL, STATUE },
    { "巨鼠的雕像", PM_GIANT_RAT, NEUTRAL, STATUE },
    { "巨鼠小雕像", PM_GIANT_RAT, NEUTRAL, FIGURINE },
    { "巨鼠的小雕像", PM_GIANT_RAT, NEUTRAL, FIGURINE },
    { "巨鼠罐头", PM_GIANT_RAT, NEUTRAL, TIN },
    { "巨鼠的罐头", PM_GIANT_RAT, NEUTRAL, TIN },
    { "巨鼠肉罐头", PM_GIANT_RAT, NEUTRAL, TIN },
    { "狂鼠雕像", PM_RABID_RAT, NEUTRAL, STATUE },
    { "狂鼠的雕像", PM_RABID_RAT, NEUTRAL, STATUE },
    { "狂鼠小雕像", PM_RABID_RAT, NEUTRAL, FIGURINE },
    { "狂鼠的小雕像", PM_RABID_RAT, NEUTRAL, FIGURINE },
    { "狂鼠罐头", PM_RABID_RAT, NEUTRAL, TIN },
    { "狂鼠的罐头", PM_RABID_RAT, NEUTRAL, TIN },
    { "狂鼠肉罐头", PM_RABID_RAT, NEUTRAL, TIN },
    { "鼠人雕像", PM_WERERAT, NEUTRAL, STATUE },
    { "鼠人的雕像", PM_WERERAT, NEUTRAL, STATUE },
    { "鼠人小雕像", PM_WERERAT, NEUTRAL, FIGURINE },
    { "鼠人的小雕像", PM_WERERAT, NEUTRAL, FIGURINE },
    { "鼠人罐头", PM_WERERAT, NEUTRAL, TIN },
    { "鼠人的罐头", PM_WERERAT, NEUTRAL, TIN },
    { "鼠人肉罐头", PM_WERERAT, NEUTRAL, TIN },
    { "岩石鼹鼠雕像", PM_ROCK_MOLE, NEUTRAL, STATUE },
    { "岩石鼹鼠的雕像", PM_ROCK_MOLE, NEUTRAL, STATUE },
    { "岩石鼹鼠小雕像", PM_ROCK_MOLE, NEUTRAL, FIGURINE },
    { "岩石鼹鼠的小雕像", PM_ROCK_MOLE, NEUTRAL, FIGURINE },
    { "岩石鼹鼠罐头", PM_ROCK_MOLE, NEUTRAL, TIN },
    { "岩石鼹鼠的罐头", PM_ROCK_MOLE, NEUTRAL, TIN },
    { "岩石鼹鼠肉罐头", PM_ROCK_MOLE, NEUTRAL, TIN },
    { "岩鼹鼠雕像", PM_ROCK_MOLE, NEUTRAL, STATUE },
    { "岩鼹鼠的雕像", PM_ROCK_MOLE, NEUTRAL, STATUE },
    { "岩鼹鼠小雕像", PM_ROCK_MOLE, NEUTRAL, FIGURINE },
    { "岩鼹鼠的小雕像", PM_ROCK_MOLE, NEUTRAL, FIGURINE },
    { "岩鼹鼠罐头", PM_ROCK_MOLE, NEUTRAL, TIN },
    { "岩鼹鼠的罐头", PM_ROCK_MOLE, NEUTRAL, TIN },
    { "岩鼹鼠肉罐头", PM_ROCK_MOLE, NEUTRAL, TIN },
    { "土拨鼠雕像", PM_WOODCHUCK, NEUTRAL, STATUE },
    { "土拨鼠的雕像", PM_WOODCHUCK, NEUTRAL, STATUE },
    { "土拨鼠小雕像", PM_WOODCHUCK, NEUTRAL, FIGURINE },
    { "土拨鼠的小雕像", PM_WOODCHUCK, NEUTRAL, FIGURINE },
    { "土拨鼠罐头", PM_WOODCHUCK, NEUTRAL, TIN },
    { "土拨鼠的罐头", PM_WOODCHUCK, NEUTRAL, TIN },
    { "土拨鼠肉罐头", PM_WOODCHUCK, NEUTRAL, TIN },
    { "洞穴蜘蛛雕像", PM_CAVE_SPIDER, NEUTRAL, STATUE },
    { "洞穴蜘蛛的雕像", PM_CAVE_SPIDER, NEUTRAL, STATUE },
    { "洞穴蜘蛛小雕像", PM_CAVE_SPIDER, NEUTRAL, FIGURINE },
    { "洞穴蜘蛛的小雕像", PM_CAVE_SPIDER, NEUTRAL, FIGURINE },
    { "洞穴蜘蛛罐头", PM_CAVE_SPIDER, NEUTRAL, TIN },
    { "洞穴蜘蛛的罐头", PM_CAVE_SPIDER, NEUTRAL, TIN },
    { "洞穴蜘蛛肉罐头", PM_CAVE_SPIDER, NEUTRAL, TIN },
    { "蜈蚣雕像", PM_CENTIPEDE, NEUTRAL, STATUE },
    { "蜈蚣的雕像", PM_CENTIPEDE, NEUTRAL, STATUE },
    { "蜈蚣小雕像", PM_CENTIPEDE, NEUTRAL, FIGURINE },
    { "蜈蚣的小雕像", PM_CENTIPEDE, NEUTRAL, FIGURINE },
    { "蜈蚣罐头", PM_CENTIPEDE, NEUTRAL, TIN },
    { "蜈蚣的罐头", PM_CENTIPEDE, NEUTRAL, TIN },
    { "蜈蚣肉罐头", PM_CENTIPEDE, NEUTRAL, TIN },
    { "巨型蜘蛛雕像", PM_GIANT_SPIDER, NEUTRAL, STATUE },
    { "巨型蜘蛛的雕像", PM_GIANT_SPIDER, NEUTRAL, STATUE },
    { "巨型蜘蛛小雕像", PM_GIANT_SPIDER, NEUTRAL, FIGURINE },
    { "巨型蜘蛛的小雕像", PM_GIANT_SPIDER, NEUTRAL, FIGURINE },
    { "巨型蜘蛛罐头", PM_GIANT_SPIDER, NEUTRAL, TIN },
    { "巨型蜘蛛的罐头", PM_GIANT_SPIDER, NEUTRAL, TIN },
    { "巨型蜘蛛肉罐头", PM_GIANT_SPIDER, NEUTRAL, TIN },
    { "巨蜘蛛雕像", PM_GIANT_SPIDER, NEUTRAL, STATUE },
    { "巨蜘蛛的雕像", PM_GIANT_SPIDER, NEUTRAL, STATUE },
    { "巨蜘蛛小雕像", PM_GIANT_SPIDER, NEUTRAL, FIGURINE },
    { "巨蜘蛛的小雕像", PM_GIANT_SPIDER, NEUTRAL, FIGURINE },
    { "巨蜘蛛罐头", PM_GIANT_SPIDER, NEUTRAL, TIN },
    { "巨蜘蛛的罐头", PM_GIANT_SPIDER, NEUTRAL, TIN },
    { "巨蜘蛛肉罐头", PM_GIANT_SPIDER, NEUTRAL, TIN },
    { "巨蛛雕像", PM_GIANT_SPIDER, NEUTRAL, STATUE },
    { "巨蛛的雕像", PM_GIANT_SPIDER, NEUTRAL, STATUE },
    { "巨蛛小雕像", PM_GIANT_SPIDER, NEUTRAL, FIGURINE },
    { "巨蛛的小雕像", PM_GIANT_SPIDER, NEUTRAL, FIGURINE },
    { "巨蛛罐头", PM_GIANT_SPIDER, NEUTRAL, TIN },
    { "巨蛛的罐头", PM_GIANT_SPIDER, NEUTRAL, TIN },
    { "巨蛛肉罐头", PM_GIANT_SPIDER, NEUTRAL, TIN },
    { "蝎子雕像", PM_SCORPION, NEUTRAL, STATUE },
    { "蝎子的雕像", PM_SCORPION, NEUTRAL, STATUE },
    { "蝎子小雕像", PM_SCORPION, NEUTRAL, FIGURINE },
    { "蝎子的小雕像", PM_SCORPION, NEUTRAL, FIGURINE },
    { "蝎子罐头", PM_SCORPION, NEUTRAL, TIN },
    { "蝎子的罐头", PM_SCORPION, NEUTRAL, TIN },
    { "蝎子肉罐头", PM_SCORPION, NEUTRAL, TIN },
    { "潜伏者雕像", PM_LURKER_ABOVE, NEUTRAL, STATUE },
    { "潜伏者的雕像", PM_LURKER_ABOVE, NEUTRAL, STATUE },
    { "潜伏者小雕像", PM_LURKER_ABOVE, NEUTRAL, FIGURINE },
    { "潜伏者的小雕像", PM_LURKER_ABOVE, NEUTRAL, FIGURINE },
    { "潜伏者罐头", PM_LURKER_ABOVE, NEUTRAL, TIN },
    { "潜伏者的罐头", PM_LURKER_ABOVE, NEUTRAL, TIN },
    { "潜伏者肉罐头", PM_LURKER_ABOVE, NEUTRAL, TIN },
    { "蛰伏怪雕像", PM_LURKER_ABOVE, NEUTRAL, STATUE },
    { "蛰伏怪的雕像", PM_LURKER_ABOVE, NEUTRAL, STATUE },
    { "蛰伏怪小雕像", PM_LURKER_ABOVE, NEUTRAL, FIGURINE },
    { "蛰伏怪的小雕像", PM_LURKER_ABOVE, NEUTRAL, FIGURINE },
    { "蛰伏怪罐头", PM_LURKER_ABOVE, NEUTRAL, TIN },
    { "蛰伏怪的罐头", PM_LURKER_ABOVE, NEUTRAL, TIN },
    { "蛰伏怪肉罐头", PM_LURKER_ABOVE, NEUTRAL, TIN },
    { "捕兽者雕像", PM_TRAPPER, NEUTRAL, STATUE },
    { "捕兽者的雕像", PM_TRAPPER, NEUTRAL, STATUE },
    { "捕兽者小雕像", PM_TRAPPER, NEUTRAL, FIGURINE },
    { "捕兽者的小雕像", PM_TRAPPER, NEUTRAL, FIGURINE },
    { "捕兽者罐头", PM_TRAPPER, NEUTRAL, TIN },
    { "捕兽者的罐头", PM_TRAPPER, NEUTRAL, TIN },
    { "捕兽者肉罐头", PM_TRAPPER, NEUTRAL, TIN },
    { "诱陷者雕像", PM_TRAPPER, NEUTRAL, STATUE },
    { "诱陷者的雕像", PM_TRAPPER, NEUTRAL, STATUE },
    { "诱陷者小雕像", PM_TRAPPER, NEUTRAL, FIGURINE },
    { "诱陷者的小雕像", PM_TRAPPER, NEUTRAL, FIGURINE },
    { "诱陷者罐头", PM_TRAPPER, NEUTRAL, TIN },
    { "诱陷者的罐头", PM_TRAPPER, NEUTRAL, TIN },
    { "诱陷者肉罐头", PM_TRAPPER, NEUTRAL, TIN },
    { "小马雕像", PM_PONY, NEUTRAL, STATUE },
    { "小马的雕像", PM_PONY, NEUTRAL, STATUE },
    { "小马小雕像", PM_PONY, NEUTRAL, FIGURINE },
    { "小马的小雕像", PM_PONY, NEUTRAL, FIGURINE },
    { "小马罐头", PM_PONY, NEUTRAL, TIN },
    { "小马的罐头", PM_PONY, NEUTRAL, TIN },
    { "小马肉罐头", PM_PONY, NEUTRAL, TIN },
    { "白色独角兽雕像", PM_WHITE_UNICORN, NEUTRAL, STATUE },
    { "白色独角兽的雕像", PM_WHITE_UNICORN, NEUTRAL, STATUE },
    { "白色独角兽小雕像", PM_WHITE_UNICORN, NEUTRAL, FIGURINE },
    { "白色独角兽的小雕像", PM_WHITE_UNICORN, NEUTRAL, FIGURINE },
    { "白色独角兽罐头", PM_WHITE_UNICORN, NEUTRAL, TIN },
    { "白色独角兽的罐头", PM_WHITE_UNICORN, NEUTRAL, TIN },
    { "白色独角兽肉罐头", PM_WHITE_UNICORN, NEUTRAL, TIN },
    { "白独角兽雕像", PM_WHITE_UNICORN, NEUTRAL, STATUE },
    { "白独角兽的雕像", PM_WHITE_UNICORN, NEUTRAL, STATUE },
    { "白独角兽小雕像", PM_WHITE_UNICORN, NEUTRAL, FIGURINE },
    { "白独角兽的小雕像", PM_WHITE_UNICORN, NEUTRAL, FIGURINE },
    { "白独角兽罐头", PM_WHITE_UNICORN, NEUTRAL, TIN },
    { "白独角兽的罐头", PM_WHITE_UNICORN, NEUTRAL, TIN },
    { "白独角兽肉罐头", PM_WHITE_UNICORN, NEUTRAL, TIN },
    { "灰色独角兽雕像", PM_GRAY_UNICORN, NEUTRAL, STATUE },
    { "灰色独角兽的雕像", PM_GRAY_UNICORN, NEUTRAL, STATUE },
    { "灰色独角兽小雕像", PM_GRAY_UNICORN, NEUTRAL, FIGURINE },
    { "灰色独角兽的小雕像", PM_GRAY_UNICORN, NEUTRAL, FIGURINE },
    { "灰色独角兽罐头", PM_GRAY_UNICORN, NEUTRAL, TIN },
    { "灰色独角兽的罐头", PM_GRAY_UNICORN, NEUTRAL, TIN },
    { "灰色独角兽肉罐头", PM_GRAY_UNICORN, NEUTRAL, TIN },
    { "灰独角兽雕像", PM_GRAY_UNICORN, NEUTRAL, STATUE },
    { "灰独角兽的雕像", PM_GRAY_UNICORN, NEUTRAL, STATUE },
    { "灰独角兽小雕像", PM_GRAY_UNICORN, NEUTRAL, FIGURINE },
    { "灰独角兽的小雕像", PM_GRAY_UNICORN, NEUTRAL, FIGURINE },
    { "灰独角兽罐头", PM_GRAY_UNICORN, NEUTRAL, TIN },
    { "灰独角兽的罐头", PM_GRAY_UNICORN, NEUTRAL, TIN },
    { "灰独角兽肉罐头", PM_GRAY_UNICORN, NEUTRAL, TIN },
    { "黑色独角兽雕像", PM_BLACK_UNICORN, NEUTRAL, STATUE },
    { "黑色独角兽的雕像", PM_BLACK_UNICORN, NEUTRAL, STATUE },
    { "黑色独角兽小雕像", PM_BLACK_UNICORN, NEUTRAL, FIGURINE },
    { "黑色独角兽的小雕像", PM_BLACK_UNICORN, NEUTRAL, FIGURINE },
    { "黑色独角兽罐头", PM_BLACK_UNICORN, NEUTRAL, TIN },
    { "黑色独角兽的罐头", PM_BLACK_UNICORN, NEUTRAL, TIN },
    { "黑色独角兽肉罐头", PM_BLACK_UNICORN, NEUTRAL, TIN },
    { "黑独角兽雕像", PM_BLACK_UNICORN, NEUTRAL, STATUE },
    { "黑独角兽的雕像", PM_BLACK_UNICORN, NEUTRAL, STATUE },
    { "黑独角兽小雕像", PM_BLACK_UNICORN, NEUTRAL, FIGURINE },
    { "黑独角兽的小雕像", PM_BLACK_UNICORN, NEUTRAL, FIGURINE },
    { "黑独角兽罐头", PM_BLACK_UNICORN, NEUTRAL, TIN },
    { "黑独角兽的罐头", PM_BLACK_UNICORN, NEUTRAL, TIN },
    { "黑独角兽肉罐头", PM_BLACK_UNICORN, NEUTRAL, TIN },
    { "马雕像", PM_HORSE, NEUTRAL, STATUE },
    { "马的雕像", PM_HORSE, NEUTRAL, STATUE },
    { "马小雕像", PM_HORSE, NEUTRAL, FIGURINE },
    { "马的小雕像", PM_HORSE, NEUTRAL, FIGURINE },
    { "马罐头", PM_HORSE, NEUTRAL, TIN },
    { "马的罐头", PM_HORSE, NEUTRAL, TIN },
    { "马肉罐头", PM_HORSE, NEUTRAL, TIN },
    { "战马雕像", PM_WARHORSE, NEUTRAL, STATUE },
    { "战马的雕像", PM_WARHORSE, NEUTRAL, STATUE },
    { "战马小雕像", PM_WARHORSE, NEUTRAL, FIGURINE },
    { "战马的小雕像", PM_WARHORSE, NEUTRAL, FIGURINE },
    { "战马罐头", PM_WARHORSE, NEUTRAL, TIN },
    { "战马的罐头", PM_WARHORSE, NEUTRAL, TIN },
    { "战马肉罐头", PM_WARHORSE, NEUTRAL, TIN },
    { "雾云雕像", PM_FOG_CLOUD, NEUTRAL, STATUE },
    { "雾云的雕像", PM_FOG_CLOUD, NEUTRAL, STATUE },
    { "雾云小雕像", PM_FOG_CLOUD, NEUTRAL, FIGURINE },
    { "雾云的小雕像", PM_FOG_CLOUD, NEUTRAL, FIGURINE },
    { "雾云罐头", PM_FOG_CLOUD, NEUTRAL, TIN },
    { "雾云的罐头", PM_FOG_CLOUD, NEUTRAL, TIN },
    { "雾云肉罐头", PM_FOG_CLOUD, NEUTRAL, TIN },
    { "云雾雕像", PM_FOG_CLOUD, NEUTRAL, STATUE },
    { "云雾的雕像", PM_FOG_CLOUD, NEUTRAL, STATUE },
    { "云雾小雕像", PM_FOG_CLOUD, NEUTRAL, FIGURINE },
    { "云雾的小雕像", PM_FOG_CLOUD, NEUTRAL, FIGURINE },
    { "云雾罐头", PM_FOG_CLOUD, NEUTRAL, TIN },
    { "云雾的罐头", PM_FOG_CLOUD, NEUTRAL, TIN },
    { "云雾肉罐头", PM_FOG_CLOUD, NEUTRAL, TIN },
    { "尘埃漩涡雕像", PM_DUST_VORTEX, NEUTRAL, STATUE },
    { "尘埃漩涡的雕像", PM_DUST_VORTEX, NEUTRAL, STATUE },
    { "尘埃漩涡小雕像", PM_DUST_VORTEX, NEUTRAL, FIGURINE },
    { "尘埃漩涡的小雕像", PM_DUST_VORTEX, NEUTRAL, FIGURINE },
    { "尘埃漩涡罐头", PM_DUST_VORTEX, NEUTRAL, TIN },
    { "尘埃漩涡的罐头", PM_DUST_VORTEX, NEUTRAL, TIN },
    { "尘埃漩涡肉罐头", PM_DUST_VORTEX, NEUTRAL, TIN },
    { "尘埃旋涡雕像", PM_DUST_VORTEX, NEUTRAL, STATUE },
    { "尘埃旋涡的雕像", PM_DUST_VORTEX, NEUTRAL, STATUE },
    { "尘埃旋涡小雕像", PM_DUST_VORTEX, NEUTRAL, FIGURINE },
    { "尘埃旋涡的小雕像", PM_DUST_VORTEX, NEUTRAL, FIGURINE },
    { "尘埃旋涡罐头", PM_DUST_VORTEX, NEUTRAL, TIN },
    { "尘埃旋涡的罐头", PM_DUST_VORTEX, NEUTRAL, TIN },
    { "尘埃旋涡肉罐头", PM_DUST_VORTEX, NEUTRAL, TIN },
    { "冰漩涡雕像", PM_ICE_VORTEX, NEUTRAL, STATUE },
    { "冰漩涡的雕像", PM_ICE_VORTEX, NEUTRAL, STATUE },
    { "冰漩涡小雕像", PM_ICE_VORTEX, NEUTRAL, FIGURINE },
    { "冰漩涡的小雕像", PM_ICE_VORTEX, NEUTRAL, FIGURINE },
    { "冰漩涡罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "冰漩涡的罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "冰漩涡肉罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "冰旋涡雕像", PM_ICE_VORTEX, NEUTRAL, STATUE },
    { "冰旋涡的雕像", PM_ICE_VORTEX, NEUTRAL, STATUE },
    { "冰旋涡小雕像", PM_ICE_VORTEX, NEUTRAL, FIGURINE },
    { "冰旋涡的小雕像", PM_ICE_VORTEX, NEUTRAL, FIGURINE },
    { "冰旋涡罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "冰旋涡的罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "冰旋涡肉罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "寒冰漩涡雕像", PM_ICE_VORTEX, NEUTRAL, STATUE },
    { "寒冰漩涡的雕像", PM_ICE_VORTEX, NEUTRAL, STATUE },
    { "寒冰漩涡小雕像", PM_ICE_VORTEX, NEUTRAL, FIGURINE },
    { "寒冰漩涡的小雕像", PM_ICE_VORTEX, NEUTRAL, FIGURINE },
    { "寒冰漩涡罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "寒冰漩涡的罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "寒冰漩涡肉罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "寒冰旋涡雕像", PM_ICE_VORTEX, NEUTRAL, STATUE },
    { "寒冰旋涡的雕像", PM_ICE_VORTEX, NEUTRAL, STATUE },
    { "寒冰旋涡小雕像", PM_ICE_VORTEX, NEUTRAL, FIGURINE },
    { "寒冰旋涡的小雕像", PM_ICE_VORTEX, NEUTRAL, FIGURINE },
    { "寒冰旋涡罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "寒冰旋涡的罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "寒冰旋涡肉罐头", PM_ICE_VORTEX, NEUTRAL, TIN },
    { "能量漩涡雕像", PM_ENERGY_VORTEX, NEUTRAL, STATUE },
    { "能量漩涡的雕像", PM_ENERGY_VORTEX, NEUTRAL, STATUE },
    { "能量漩涡小雕像", PM_ENERGY_VORTEX, NEUTRAL, FIGURINE },
    { "能量漩涡的小雕像", PM_ENERGY_VORTEX, NEUTRAL, FIGURINE },
    { "能量漩涡罐头", PM_ENERGY_VORTEX, NEUTRAL, TIN },
    { "能量漩涡的罐头", PM_ENERGY_VORTEX, NEUTRAL, TIN },
    { "能量漩涡肉罐头", PM_ENERGY_VORTEX, NEUTRAL, TIN },
    { "能量旋涡雕像", PM_ENERGY_VORTEX, NEUTRAL, STATUE },
    { "能量旋涡的雕像", PM_ENERGY_VORTEX, NEUTRAL, STATUE },
    { "能量旋涡小雕像", PM_ENERGY_VORTEX, NEUTRAL, FIGURINE },
    { "能量旋涡的小雕像", PM_ENERGY_VORTEX, NEUTRAL, FIGURINE },
    { "能量旋涡罐头", PM_ENERGY_VORTEX, NEUTRAL, TIN },
    { "能量旋涡的罐头", PM_ENERGY_VORTEX, NEUTRAL, TIN },
    { "能量旋涡肉罐头", PM_ENERGY_VORTEX, NEUTRAL, TIN },
    { "蒸汽漩涡雕像", PM_STEAM_VORTEX, NEUTRAL, STATUE },
    { "蒸汽漩涡的雕像", PM_STEAM_VORTEX, NEUTRAL, STATUE },
    { "蒸汽漩涡小雕像", PM_STEAM_VORTEX, NEUTRAL, FIGURINE },
    { "蒸汽漩涡的小雕像", PM_STEAM_VORTEX, NEUTRAL, FIGURINE },
    { "蒸汽漩涡罐头", PM_STEAM_VORTEX, NEUTRAL, TIN },
    { "蒸汽漩涡的罐头", PM_STEAM_VORTEX, NEUTRAL, TIN },
    { "蒸汽漩涡肉罐头", PM_STEAM_VORTEX, NEUTRAL, TIN },
    { "蒸汽旋涡雕像", PM_STEAM_VORTEX, NEUTRAL, STATUE },
    { "蒸汽旋涡的雕像", PM_STEAM_VORTEX, NEUTRAL, STATUE },
    { "蒸汽旋涡小雕像", PM_STEAM_VORTEX, NEUTRAL, FIGURINE },
    { "蒸汽旋涡的小雕像", PM_STEAM_VORTEX, NEUTRAL, FIGURINE },
    { "蒸汽旋涡罐头", PM_STEAM_VORTEX, NEUTRAL, TIN },
    { "蒸汽旋涡的罐头", PM_STEAM_VORTEX, NEUTRAL, TIN },
    { "蒸汽旋涡肉罐头", PM_STEAM_VORTEX, NEUTRAL, TIN },
    { "火焰漩涡雕像", PM_FIRE_VORTEX, NEUTRAL, STATUE },
    { "火焰漩涡的雕像", PM_FIRE_VORTEX, NEUTRAL, STATUE },
    { "火焰漩涡小雕像", PM_FIRE_VORTEX, NEUTRAL, FIGURINE },
    { "火焰漩涡的小雕像", PM_FIRE_VORTEX, NEUTRAL, FIGURINE },
    { "火焰漩涡罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "火焰漩涡的罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "火焰漩涡肉罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "火焰旋涡雕像", PM_FIRE_VORTEX, NEUTRAL, STATUE },
    { "火焰旋涡的雕像", PM_FIRE_VORTEX, NEUTRAL, STATUE },
    { "火焰旋涡小雕像", PM_FIRE_VORTEX, NEUTRAL, FIGURINE },
    { "火焰旋涡的小雕像", PM_FIRE_VORTEX, NEUTRAL, FIGURINE },
    { "火焰旋涡罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "火焰旋涡的罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "火焰旋涡肉罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "火漩涡雕像", PM_FIRE_VORTEX, NEUTRAL, STATUE },
    { "火漩涡的雕像", PM_FIRE_VORTEX, NEUTRAL, STATUE },
    { "火漩涡小雕像", PM_FIRE_VORTEX, NEUTRAL, FIGURINE },
    { "火漩涡的小雕像", PM_FIRE_VORTEX, NEUTRAL, FIGURINE },
    { "火漩涡罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "火漩涡的罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "火漩涡肉罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "火旋涡雕像", PM_FIRE_VORTEX, NEUTRAL, STATUE },
    { "火旋涡的雕像", PM_FIRE_VORTEX, NEUTRAL, STATUE },
    { "火旋涡小雕像", PM_FIRE_VORTEX, NEUTRAL, FIGURINE },
    { "火旋涡的小雕像", PM_FIRE_VORTEX, NEUTRAL, FIGURINE },
    { "火旋涡罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "火旋涡的罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "火旋涡肉罐头", PM_FIRE_VORTEX, NEUTRAL, TIN },
    { "幼长蠕虫雕像", PM_BABY_LONG_WORM, NEUTRAL, STATUE },
    { "幼长蠕虫的雕像", PM_BABY_LONG_WORM, NEUTRAL, STATUE },
    { "幼长蠕虫小雕像", PM_BABY_LONG_WORM, NEUTRAL, FIGURINE },
    { "幼长蠕虫的小雕像", PM_BABY_LONG_WORM, NEUTRAL, FIGURINE },
    { "幼长蠕虫罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "幼长蠕虫的罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "幼长蠕虫肉罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "长蠕虫幼体雕像", PM_BABY_LONG_WORM, NEUTRAL, STATUE },
    { "长蠕虫幼体的雕像", PM_BABY_LONG_WORM, NEUTRAL, STATUE },
    { "长蠕虫幼体小雕像", PM_BABY_LONG_WORM, NEUTRAL, FIGURINE },
    { "长蠕虫幼体的小雕像", PM_BABY_LONG_WORM, NEUTRAL, FIGURINE },
    { "长蠕虫幼体罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "长蠕虫幼体的罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "长蠕虫幼体肉罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "幼紫蠕虫雕像", PM_BABY_PURPLE_WORM, NEUTRAL, STATUE },
    { "幼紫蠕虫的雕像", PM_BABY_PURPLE_WORM, NEUTRAL, STATUE },
    { "幼紫蠕虫小雕像", PM_BABY_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "幼紫蠕虫的小雕像", PM_BABY_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "幼紫蠕虫罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "幼紫蠕虫的罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "幼紫蠕虫肉罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "紫蠕虫幼体雕像", PM_BABY_PURPLE_WORM, NEUTRAL, STATUE },
    { "紫蠕虫幼体的雕像", PM_BABY_PURPLE_WORM, NEUTRAL, STATUE },
    { "紫蠕虫幼体小雕像", PM_BABY_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "紫蠕虫幼体的小雕像", PM_BABY_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "紫蠕虫幼体罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "紫蠕虫幼体的罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "紫蠕虫幼体肉罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "长蠕虫雕像", PM_LONG_WORM, NEUTRAL, STATUE },
    { "长蠕虫的雕像", PM_LONG_WORM, NEUTRAL, STATUE },
    { "长蠕虫小雕像", PM_LONG_WORM, NEUTRAL, FIGURINE },
    { "长蠕虫的小雕像", PM_LONG_WORM, NEUTRAL, FIGURINE },
    { "长蠕虫罐头", PM_LONG_WORM, NEUTRAL, TIN },
    { "长蠕虫的罐头", PM_LONG_WORM, NEUTRAL, TIN },
    { "长蠕虫肉罐头", PM_LONG_WORM, NEUTRAL, TIN },
    { "紫蠕虫雕像", PM_PURPLE_WORM, NEUTRAL, STATUE },
    { "紫蠕虫的雕像", PM_PURPLE_WORM, NEUTRAL, STATUE },
    { "紫蠕虫小雕像", PM_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "紫蠕虫的小雕像", PM_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "紫蠕虫罐头", PM_PURPLE_WORM, NEUTRAL, TIN },
    { "紫蠕虫的罐头", PM_PURPLE_WORM, NEUTRAL, TIN },
    { "紫蠕虫肉罐头", PM_PURPLE_WORM, NEUTRAL, TIN },
    { "幼长虫雕像", PM_BABY_LONG_WORM, NEUTRAL, STATUE },
    { "幼长虫的雕像", PM_BABY_LONG_WORM, NEUTRAL, STATUE },
    { "幼长虫小雕像", PM_BABY_LONG_WORM, NEUTRAL, FIGURINE },
    { "幼长虫的小雕像", PM_BABY_LONG_WORM, NEUTRAL, FIGURINE },
    { "幼长虫罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "幼长虫的罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "幼长虫肉罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "长虫幼体雕像", PM_BABY_LONG_WORM, NEUTRAL, STATUE },
    { "长虫幼体的雕像", PM_BABY_LONG_WORM, NEUTRAL, STATUE },
    { "长虫幼体小雕像", PM_BABY_LONG_WORM, NEUTRAL, FIGURINE },
    { "长虫幼体的小雕像", PM_BABY_LONG_WORM, NEUTRAL, FIGURINE },
    { "长虫幼体罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "长虫幼体的罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "长虫幼体肉罐头", PM_BABY_LONG_WORM, NEUTRAL, TIN },
    { "幼紫虫雕像", PM_BABY_PURPLE_WORM, NEUTRAL, STATUE },
    { "幼紫虫的雕像", PM_BABY_PURPLE_WORM, NEUTRAL, STATUE },
    { "幼紫虫小雕像", PM_BABY_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "幼紫虫的小雕像", PM_BABY_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "幼紫虫罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "幼紫虫的罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "幼紫虫肉罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "紫虫幼体雕像", PM_BABY_PURPLE_WORM, NEUTRAL, STATUE },
    { "紫虫幼体的雕像", PM_BABY_PURPLE_WORM, NEUTRAL, STATUE },
    { "紫虫幼体小雕像", PM_BABY_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "紫虫幼体的小雕像", PM_BABY_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "紫虫幼体罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "紫虫幼体的罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "紫虫幼体肉罐头", PM_BABY_PURPLE_WORM, NEUTRAL, TIN },
    { "长虫雕像", PM_LONG_WORM, NEUTRAL, STATUE },
    { "长虫的雕像", PM_LONG_WORM, NEUTRAL, STATUE },
    { "长虫小雕像", PM_LONG_WORM, NEUTRAL, FIGURINE },
    { "长虫的小雕像", PM_LONG_WORM, NEUTRAL, FIGURINE },
    { "长虫罐头", PM_LONG_WORM, NEUTRAL, TIN },
    { "长虫的罐头", PM_LONG_WORM, NEUTRAL, TIN },
    { "长虫肉罐头", PM_LONG_WORM, NEUTRAL, TIN },
    { "紫虫雕像", PM_PURPLE_WORM, NEUTRAL, STATUE },
    { "紫虫的雕像", PM_PURPLE_WORM, NEUTRAL, STATUE },
    { "紫虫小雕像", PM_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "紫虫的小雕像", PM_PURPLE_WORM, NEUTRAL, FIGURINE },
    { "紫虫罐头", PM_PURPLE_WORM, NEUTRAL, TIN },
    { "紫虫的罐头", PM_PURPLE_WORM, NEUTRAL, TIN },
    { "紫虫肉罐头", PM_PURPLE_WORM, NEUTRAL, TIN },
    { "电子虫雕像", PM_GRID_BUG, NEUTRAL, STATUE },
    { "电子虫的雕像", PM_GRID_BUG, NEUTRAL, STATUE },
    { "电子虫小雕像", PM_GRID_BUG, NEUTRAL, FIGURINE },
    { "电子虫的小雕像", PM_GRID_BUG, NEUTRAL, FIGURINE },
    { "电子虫罐头", PM_GRID_BUG, NEUTRAL, TIN },
    { "电子虫的罐头", PM_GRID_BUG, NEUTRAL, TIN },
    { "电子虫肉罐头", PM_GRID_BUG, NEUTRAL, TIN },
    { "玄蚊雕像", PM_XAN, NEUTRAL, STATUE },
    { "玄蚊的雕像", PM_XAN, NEUTRAL, STATUE },
    { "玄蚊小雕像", PM_XAN, NEUTRAL, FIGURINE },
    { "玄蚊的小雕像", PM_XAN, NEUTRAL, FIGURINE },
    { "玄蚊罐头", PM_XAN, NEUTRAL, TIN },
    { "玄蚊的罐头", PM_XAN, NEUTRAL, TIN },
    { "玄蚊肉罐头", PM_XAN, NEUTRAL, TIN },
    { "黄光雕像", PM_YELLOW_LIGHT, NEUTRAL, STATUE },
    { "黄光的雕像", PM_YELLOW_LIGHT, NEUTRAL, STATUE },
    { "黄光小雕像", PM_YELLOW_LIGHT, NEUTRAL, FIGURINE },
    { "黄光的小雕像", PM_YELLOW_LIGHT, NEUTRAL, FIGURINE },
    { "黄光罐头", PM_YELLOW_LIGHT, NEUTRAL, TIN },
    { "黄光的罐头", PM_YELLOW_LIGHT, NEUTRAL, TIN },
    { "黄光肉罐头", PM_YELLOW_LIGHT, NEUTRAL, TIN },
    { "黑光雕像", PM_BLACK_LIGHT, NEUTRAL, STATUE },
    { "黑光的雕像", PM_BLACK_LIGHT, NEUTRAL, STATUE },
    { "黑光小雕像", PM_BLACK_LIGHT, NEUTRAL, FIGURINE },
    { "黑光的小雕像", PM_BLACK_LIGHT, NEUTRAL, FIGURINE },
    { "黑光罐头", PM_BLACK_LIGHT, NEUTRAL, TIN },
    { "黑光的罐头", PM_BLACK_LIGHT, NEUTRAL, TIN },
    { "黑光肉罐头", PM_BLACK_LIGHT, NEUTRAL, TIN },
    { "山区巨人雕像", PM_ZRUTY, NEUTRAL, STATUE },
    { "山区巨人的雕像", PM_ZRUTY, NEUTRAL, STATUE },
    { "山区巨人小雕像", PM_ZRUTY, NEUTRAL, FIGURINE },
    { "山区巨人的小雕像", PM_ZRUTY, NEUTRAL, FIGURINE },
    { "山区巨人罐头", PM_ZRUTY, NEUTRAL, TIN },
    { "山区巨人的罐头", PM_ZRUTY, NEUTRAL, TIN },
    { "山区巨人肉罐头", PM_ZRUTY, NEUTRAL, TIN },
    { "羽蛇雕像", PM_COUATL, NEUTRAL, STATUE },
    { "羽蛇的雕像", PM_COUATL, NEUTRAL, STATUE },
    { "羽蛇小雕像", PM_COUATL, NEUTRAL, FIGURINE },
    { "羽蛇的小雕像", PM_COUATL, NEUTRAL, FIGURINE },
    { "羽蛇罐头", PM_COUATL, NEUTRAL, TIN },
    { "羽蛇的罐头", PM_COUATL, NEUTRAL, TIN },
    { "羽蛇肉罐头", PM_COUATL, NEUTRAL, TIN },
    { "亚历克斯雕像", PM_ALEA, STATUEX }
    { "亚历克斯的雕像", PM_ALEA, STATUEX }
    { "亚历克斯小雕像", PM_ALEA, FIGURINEX }
    { "亚历克斯的小雕像", PM_ALEA, FIGURINEX }
    { "亚历克斯罐头", PM_ALEA, TINX }
    { "亚历克斯的罐头", PM_ALEA, TINX }
    { "亚历克斯肉罐头", PM_ALEA, TINX }
    { "神罚化身雕像", PM_ALEA, STATUEX }
    { "神罚化身的雕像", PM_ALEA, STATUEX }
    { "神罚化身小雕像", PM_ALEA, FIGURINEX }
    { "神罚化身的小雕像", PM_ALEA, FIGURINEX }
    { "神罚化身罐头", PM_ALEA, TINX }
    { "神罚化身的罐头", PM_ALEA, TINX }
    { "神罚化身肉罐头", PM_ALEA, TINX }
    { "天使雕像", PM_ANG, STATUEEL}
    { "天使的雕像", PM_ANG, STATUEEL}
    { "天使小雕像", PM_ANG, FIGURINEEL}
    { "天使的小雕像", PM_ANG, FIGURINEEL}
    { "天使罐头", PM_ANG, TINEL}
    { "天使的罐头", PM_ANG, TINEL}
    { "天使肉罐头", PM_ANG, TINEL}
    { "麒麟雕像", PM_KI_RIN, NEUTRAL, STATUE },
    { "麒麟的雕像", PM_KI_RIN, NEUTRAL, STATUE },
    { "麒麟小雕像", PM_KI_RIN, NEUTRAL, FIGURINE },
    { "麒麟的小雕像", PM_KI_RIN, NEUTRAL, FIGURINE },
    { "麒麟罐头", PM_KI_RIN, NEUTRAL, TIN },
    { "麒麟的罐头", PM_KI_RIN, NEUTRAL, TIN },
    { "麒麟肉罐头", PM_KI_RIN, NEUTRAL, TIN },
    { "执政官雕像", PM_ARCHON, NEUTRAL, STATUE },
    { "执政官的雕像", PM_ARCHON, NEUTRAL, STATUE },
    { "执政官小雕像", PM_ARCHON, NEUTRAL, FIGURINE },
    { "执政官的小雕像", PM_ARCHON, NEUTRAL, FIGURINE },
    { "执政官罐头", PM_ARCHON, NEUTRAL, TIN },
    { "执政官的罐头", PM_ARCHON, NEUTRAL, TIN },
    { "执政官肉罐头", PM_ARCHON, NEUTRAL, TIN },
    { "亚空天族雕像", PM_ARCHON, NEUTRAL, STATUE },
    { "亚空天族的雕像", PM_ARCHON, NEUTRAL, STATUE },
    { "亚空天族小雕像", PM_ARCHON, NEUTRAL, FIGURINE },
    { "亚空天族的小雕像", PM_ARCHON, NEUTRAL, FIGURINE },
    { "亚空天族罐头", PM_ARCHON, NEUTRAL, TIN },
    { "亚空天族的罐头", PM_ARCHON, NEUTRAL, TIN },
    { "亚空天族肉罐头", PM_ARCHON, NEUTRAL, TIN },
    { "蝙蝠雕像", PM_BAT, NEUTRAL, STATUE },
    { "蝙蝠的雕像", PM_BAT, NEUTRAL, STATUE },
    { "蝙蝠小雕像", PM_BAT, NEUTRAL, FIGURINE },
    { "蝙蝠的小雕像", PM_BAT, NEUTRAL, FIGURINE },
    { "蝙蝠罐头", PM_BAT, NEUTRAL, TIN },
    { "蝙蝠的罐头", PM_BAT, NEUTRAL, TIN },
    { "蝙蝠肉罐头", PM_BAT, NEUTRAL, TIN },
    { "巨型蝙蝠雕像", PM_GIANT_BAT, NEUTRAL, STATUE },
    { "巨型蝙蝠的雕像", PM_GIANT_BAT, NEUTRAL, STATUE },
    { "巨型蝙蝠小雕像", PM_GIANT_BAT, NEUTRAL, FIGURINE },
    { "巨型蝙蝠的小雕像", PM_GIANT_BAT, NEUTRAL, FIGURINE },
    { "巨型蝙蝠罐头", PM_GIANT_BAT, NEUTRAL, TIN },
    { "巨型蝙蝠的罐头", PM_GIANT_BAT, NEUTRAL, TIN },
    { "巨型蝙蝠肉罐头", PM_GIANT_BAT, NEUTRAL, TIN },
    { "巨蝙蝠雕像", PM_GIANT_BAT, NEUTRAL, STATUE },
    { "巨蝙蝠的雕像", PM_GIANT_BAT, NEUTRAL, STATUE },
    { "巨蝙蝠小雕像", PM_GIANT_BAT, NEUTRAL, FIGURINE },
    { "巨蝙蝠的小雕像", PM_GIANT_BAT, NEUTRAL, FIGURINE },
    { "巨蝙蝠罐头", PM_GIANT_BAT, NEUTRAL, TIN },
    { "巨蝙蝠的罐头", PM_GIANT_BAT, NEUTRAL, TIN },
    { "巨蝙蝠肉罐头", PM_GIANT_BAT, NEUTRAL, TIN },
    { "巨蝠雕像", PM_GIANT_BAT, NEUTRAL, STATUE },
    { "巨蝠的雕像", PM_GIANT_BAT, NEUTRAL, STATUE },
    { "巨蝠小雕像", PM_GIANT_BAT, NEUTRAL, FIGURINE },
    { "巨蝠的小雕像", PM_GIANT_BAT, NEUTRAL, FIGURINE },
    { "巨蝠罐头", PM_GIANT_BAT, NEUTRAL, TIN },
    { "巨蝠的罐头", PM_GIANT_BAT, NEUTRAL, TIN },
    { "巨蝠肉罐头", PM_GIANT_BAT, NEUTRAL, TIN },
    { "乌鸦雕像", PM_RAVEN, NEUTRAL, STATUE },
    { "乌鸦的雕像", PM_RAVEN, NEUTRAL, STATUE },
    { "乌鸦小雕像", PM_RAVEN, NEUTRAL, FIGURINE },
    { "乌鸦的小雕像", PM_RAVEN, NEUTRAL, FIGURINE },
    { "乌鸦罐头", PM_RAVEN, NEUTRAL, TIN },
    { "乌鸦的罐头", PM_RAVEN, NEUTRAL, TIN },
    { "乌鸦肉罐头", PM_RAVEN, NEUTRAL, TIN },
    { "吸血蝙蝠雕像", PM_VAMPIRE_BAT, NEUTRAL, STATUE },
    { "吸血蝙蝠的雕像", PM_VAMPIRE_BAT, NEUTRAL, STATUE },
    { "吸血蝙蝠小雕像", PM_VAMPIRE_BAT, NEUTRAL, FIGURINE },
    { "吸血蝙蝠的小雕像", PM_VAMPIRE_BAT, NEUTRAL, FIGURINE },
    { "吸血蝙蝠罐头", PM_VAMPIRE_BAT, NEUTRAL, TIN },
    { "吸血蝙蝠的罐头", PM_VAMPIRE_BAT, NEUTRAL, TIN },
    { "吸血蝙蝠肉罐头", PM_VAMPIRE_BAT, NEUTRAL, TIN },
    { "平原半人马雕像", PM_PLAINS_CENTAUR, NEUTRAL, STATUE },
    { "平原半人马的雕像", PM_PLAINS_CENTAUR, NEUTRAL, STATUE },
    { "平原半人马小雕像", PM_PLAINS_CENTAUR, NEUTRAL, FIGURINE },
    { "平原半人马的小雕像", PM_PLAINS_CENTAUR, NEUTRAL, FIGURINE },
    { "平原半人马罐头", PM_PLAINS_CENTAUR, NEUTRAL, TIN },
    { "平原半人马的罐头", PM_PLAINS_CENTAUR, NEUTRAL, TIN },
    { "平原半人马肉罐头", PM_PLAINS_CENTAUR, NEUTRAL, TIN },
    { "森林半人马雕像", PM_FOREST_CENTAUR, NEUTRAL, STATUE },
    { "森林半人马的雕像", PM_FOREST_CENTAUR, NEUTRAL, STATUE },
    { "森林半人马小雕像", PM_FOREST_CENTAUR, NEUTRAL, FIGURINE },
    { "森林半人马的小雕像", PM_FOREST_CENTAUR, NEUTRAL, FIGURINE },
    { "森林半人马罐头", PM_FOREST_CENTAUR, NEUTRAL, TIN },
    { "森林半人马的罐头", PM_FOREST_CENTAUR, NEUTRAL, TIN },
    { "森林半人马肉罐头", PM_FOREST_CENTAUR, NEUTRAL, TIN },
    { "山地半人马雕像", PM_MOUNTAIN_CENTA, STATUEUR}
    { "山地半人马的雕像", PM_MOUNTAIN_CENTA, STATUEUR}
    { "山地半人马小雕像", PM_MOUNTAIN_CENTA, FIGURINEUR}
    { "山地半人马的小雕像", PM_MOUNTAIN_CENTA, FIGURINEUR}
    { "山地半人马罐头", PM_MOUNTAIN_CENTA, TINUR}
    { "山地半人马的罐头", PM_MOUNTAIN_CENTA, TINUR}
    { "山地半人马肉罐头", PM_MOUNTAIN_CENTA, TINUR}
    { "山半人马雕像", PM_MOUNTAIN_CENTA, STATUEUR}
    { "山半人马的雕像", PM_MOUNTAIN_CENTA, STATUEUR}
    { "山半人马小雕像", PM_MOUNTAIN_CENTA, FIGURINEUR}
    { "山半人马的小雕像", PM_MOUNTAIN_CENTA, FIGURINEUR}
    { "山半人马罐头", PM_MOUNTAIN_CENTA, TINUR}
    { "山半人马的罐头", PM_MOUNTAIN_CENTA, TINUR}
    { "山半人马肉罐头", PM_MOUNTAIN_CENTA, TINUR}
    { "幼灰龙雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, STATUE },
    { "幼灰龙的雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, STATUE },
    { "幼灰龙小雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, FIGURINE },
    { "幼灰龙的小雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, FIGURINE },
    { "幼灰龙罐头", PM_BABY_GRAY_DRAGON, NEUTRAL, TIN },
    { "幼灰龙的罐头", PM_BABY_GRAY_DRAGON, NEUTRAL, TIN },
    { "幼灰龙肉罐头", PM_BABY_GRAY_DRAGON, NEUTRAL, TIN },
    { "幼金龙雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, STATUE },
    { "幼金龙的雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, STATUE },
    { "幼金龙小雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, FIGURINE },
    { "幼金龙的小雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, FIGURINE },
    { "幼金龙罐头", PM_BABY_GOLD_DRAGON, NEUTRAL, TIN },
    { "幼金龙的罐头", PM_BABY_GOLD_DRAGON, NEUTRAL, TIN },
    { "幼金龙肉罐头", PM_BABY_GOLD_DRAGON, NEUTRAL, TIN },
    { "幼银龙雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, STATUE },
    { "幼银龙的雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, STATUE },
    { "幼银龙小雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, FIGURINE },
    { "幼银龙的小雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, FIGURINE },
    { "幼银龙罐头", PM_BABY_SILVER_DRAGON, NEUTRAL, TIN },
    { "幼银龙的罐头", PM_BABY_SILVER_DRAGON, NEUTRAL, TIN },
    { "幼银龙肉罐头", PM_BABY_SILVER_DRAGON, NEUTRAL, TIN },
    { "幼红龙雕像", PM_BABY_RED_DRAGON, NEUTRAL, STATUE },
    { "幼红龙的雕像", PM_BABY_RED_DRAGON, NEUTRAL, STATUE },
    { "幼红龙小雕像", PM_BABY_RED_DRAGON, NEUTRAL, FIGURINE },
    { "幼红龙的小雕像", PM_BABY_RED_DRAGON, NEUTRAL, FIGURINE },
    { "幼红龙罐头", PM_BABY_RED_DRAGON, NEUTRAL, TIN },
    { "幼红龙的罐头", PM_BABY_RED_DRAGON, NEUTRAL, TIN },
    { "幼红龙肉罐头", PM_BABY_RED_DRAGON, NEUTRAL, TIN },
    { "幼白龙雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, STATUE },
    { "幼白龙的雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, STATUE },
    { "幼白龙小雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, FIGURINE },
    { "幼白龙的小雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, FIGURINE },
    { "幼白龙罐头", PM_BABY_WHITE_DRAGON, NEUTRAL, TIN },
    { "幼白龙的罐头", PM_BABY_WHITE_DRAGON, NEUTRAL, TIN },
    { "幼白龙肉罐头", PM_BABY_WHITE_DRAGON, NEUTRAL, TIN },
    { "幼橙龙雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, STATUE },
    { "幼橙龙的雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, STATUE },
    { "幼橙龙小雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, FIGURINE },
    { "幼橙龙的小雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, FIGURINE },
    { "幼橙龙罐头", PM_BABY_ORANGE_DRAGON, NEUTRAL, TIN },
    { "幼橙龙的罐头", PM_BABY_ORANGE_DRAGON, NEUTRAL, TIN },
    { "幼橙龙肉罐头", PM_BABY_ORANGE_DRAGON, NEUTRAL, TIN },
    { "幼黑龙雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, STATUE },
    { "幼黑龙的雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, STATUE },
    { "幼黑龙小雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, FIGURINE },
    { "幼黑龙的小雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, FIGURINE },
    { "幼黑龙罐头", PM_BABY_BLACK_DRAGON, NEUTRAL, TIN },
    { "幼黑龙的罐头", PM_BABY_BLACK_DRAGON, NEUTRAL, TIN },
    { "幼黑龙肉罐头", PM_BABY_BLACK_DRAGON, NEUTRAL, TIN },
    { "幼蓝龙雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, STATUE },
    { "幼蓝龙的雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, STATUE },
    { "幼蓝龙小雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, FIGURINE },
    { "幼蓝龙的小雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, FIGURINE },
    { "幼蓝龙罐头", PM_BABY_BLUE_DRAGON, NEUTRAL, TIN },
    { "幼蓝龙的罐头", PM_BABY_BLUE_DRAGON, NEUTRAL, TIN },
    { "幼蓝龙肉罐头", PM_BABY_BLUE_DRAGON, NEUTRAL, TIN },
    { "幼绿龙雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, STATUE },
    { "幼绿龙的雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, STATUE },
    { "幼绿龙小雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, FIGURINE },
    { "幼绿龙的小雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, FIGURINE },
    { "幼绿龙罐头", PM_BABY_GREEN_DRAGON, NEUTRAL, TIN },
    { "幼绿龙的罐头", PM_BABY_GREEN_DRAGON, NEUTRAL, TIN },
    { "幼绿龙肉罐头", PM_BABY_GREEN_DRAGON, NEUTRAL, TIN },
    { "幼黄龙雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, STATUE },
    { "幼黄龙的雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, STATUE },
    { "幼黄龙小雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, FIGURINE },
    { "幼黄龙的小雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, FIGURINE },
    { "幼黄龙罐头", PM_BABY_YELLOW_DRAGON, NEUTRAL, TIN },
    { "幼黄龙的罐头", PM_BABY_YELLOW_DRAGON, NEUTRAL, TIN },
    { "幼黄龙肉罐头", PM_BABY_YELLOW_DRAGON, NEUTRAL, TIN },
    { "小灰龙雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, STATUE },
    { "小灰龙的雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, STATUE },
    { "小灰龙小雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, FIGURINE },
    { "小灰龙的小雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, FIGURINE },
    { "小灰龙罐头", PM_BABY_GRAY_DRAGON, NEUTRAL, TIN },
    { "小灰龙的罐头", PM_BABY_GRAY_DRAGON, NEUTRAL, TIN },
    { "小灰龙肉罐头", PM_BABY_GRAY_DRAGON, NEUTRAL, TIN },
    { "小金龙雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, STATUE },
    { "小金龙的雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, STATUE },
    { "小金龙小雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, FIGURINE },
    { "小金龙的小雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, FIGURINE },
    { "小金龙罐头", PM_BABY_GOLD_DRAGON, NEUTRAL, TIN },
    { "小金龙的罐头", PM_BABY_GOLD_DRAGON, NEUTRAL, TIN },
    { "小金龙肉罐头", PM_BABY_GOLD_DRAGON, NEUTRAL, TIN },
    { "小银龙雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, STATUE },
    { "小银龙的雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, STATUE },
    { "小银龙小雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, FIGURINE },
    { "小银龙的小雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, FIGURINE },
    { "小银龙罐头", PM_BABY_SILVER_DRAGON, NEUTRAL, TIN },
    { "小银龙的罐头", PM_BABY_SILVER_DRAGON, NEUTRAL, TIN },
    { "小银龙肉罐头", PM_BABY_SILVER_DRAGON, NEUTRAL, TIN },
    { "小红龙雕像", PM_BABY_RED_DRAGON, NEUTRAL, STATUE },
    { "小红龙的雕像", PM_BABY_RED_DRAGON, NEUTRAL, STATUE },
    { "小红龙小雕像", PM_BABY_RED_DRAGON, NEUTRAL, FIGURINE },
    { "小红龙的小雕像", PM_BABY_RED_DRAGON, NEUTRAL, FIGURINE },
    { "小红龙罐头", PM_BABY_RED_DRAGON, NEUTRAL, TIN },
    { "小红龙的罐头", PM_BABY_RED_DRAGON, NEUTRAL, TIN },
    { "小红龙肉罐头", PM_BABY_RED_DRAGON, NEUTRAL, TIN },
    { "小白龙雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, STATUE },
    { "小白龙的雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, STATUE },
    { "小白龙小雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, FIGURINE },
    { "小白龙的小雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, FIGURINE },
    { "小白龙罐头", PM_BABY_WHITE_DRAGON, NEUTRAL, TIN },
    { "小白龙的罐头", PM_BABY_WHITE_DRAGON, NEUTRAL, TIN },
    { "小白龙肉罐头", PM_BABY_WHITE_DRAGON, NEUTRAL, TIN },
    { "小橙龙雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, STATUE },
    { "小橙龙的雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, STATUE },
    { "小橙龙小雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, FIGURINE },
    { "小橙龙的小雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, FIGURINE },
    { "小橙龙罐头", PM_BABY_ORANGE_DRAGON, NEUTRAL, TIN },
    { "小橙龙的罐头", PM_BABY_ORANGE_DRAGON, NEUTRAL, TIN },
    { "小橙龙肉罐头", PM_BABY_ORANGE_DRAGON, NEUTRAL, TIN },
    { "小黑龙雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, STATUE },
    { "小黑龙的雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, STATUE },
    { "小黑龙小雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, FIGURINE },
    { "小黑龙的小雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, FIGURINE },
    { "小黑龙罐头", PM_BABY_BLACK_DRAGON, NEUTRAL, TIN },
    { "小黑龙的罐头", PM_BABY_BLACK_DRAGON, NEUTRAL, TIN },
    { "小黑龙肉罐头", PM_BABY_BLACK_DRAGON, NEUTRAL, TIN },
    { "小蓝龙雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, STATUE },
    { "小蓝龙的雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, STATUE },
    { "小蓝龙小雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, FIGURINE },
    { "小蓝龙的小雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, FIGURINE },
    { "小蓝龙罐头", PM_BABY_BLUE_DRAGON, NEUTRAL, TIN },
    { "小蓝龙的罐头", PM_BABY_BLUE_DRAGON, NEUTRAL, TIN },
    { "小蓝龙肉罐头", PM_BABY_BLUE_DRAGON, NEUTRAL, TIN },
    { "小绿龙雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, STATUE },
    { "小绿龙的雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, STATUE },
    { "小绿龙小雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, FIGURINE },
    { "小绿龙的小雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, FIGURINE },
    { "小绿龙罐头", PM_BABY_GREEN_DRAGON, NEUTRAL, TIN },
    { "小绿龙的罐头", PM_BABY_GREEN_DRAGON, NEUTRAL, TIN },
    { "小绿龙肉罐头", PM_BABY_GREEN_DRAGON, NEUTRAL, TIN },
    { "小黄龙雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, STATUE },
    { "小黄龙的雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, STATUE },
    { "小黄龙小雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, FIGURINE },
    { "小黄龙的小雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, FIGURINE },
    { "小黄龙罐头", PM_BABY_YELLOW_DRAGON, NEUTRAL, TIN },
    { "小黄龙的罐头", PM_BABY_YELLOW_DRAGON, NEUTRAL, TIN },
    { "小黄龙肉罐头", PM_BABY_YELLOW_DRAGON, NEUTRAL, TIN },
    { "灰龙宝宝雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, STATUE },
    { "灰龙宝宝的雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, STATUE },
    { "灰龙宝宝小雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, FIGURINE },
    { "灰龙宝宝的小雕像", PM_BABY_GRAY_DRAGON, NEUTRAL, FIGURINE },
    { "灰龙宝宝罐头", PM_BABY_GRAY_DRAGON, NEUTRAL, TIN },
    { "灰龙宝宝的罐头", PM_BABY_GRAY_DRAGON, NEUTRAL, TIN },
    { "灰龙宝宝肉罐头", PM_BABY_GRAY_DRAGON, NEUTRAL, TIN },
    { "金龙宝宝雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, STATUE },
    { "金龙宝宝的雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, STATUE },
    { "金龙宝宝小雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, FIGURINE },
    { "金龙宝宝的小雕像", PM_BABY_GOLD_DRAGON, NEUTRAL, FIGURINE },
    { "金龙宝宝罐头", PM_BABY_GOLD_DRAGON, NEUTRAL, TIN },
    { "金龙宝宝的罐头", PM_BABY_GOLD_DRAGON, NEUTRAL, TIN },
    { "金龙宝宝肉罐头", PM_BABY_GOLD_DRAGON, NEUTRAL, TIN },
    { "银龙宝宝雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, STATUE },
    { "银龙宝宝的雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, STATUE },
    { "银龙宝宝小雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, FIGURINE },
    { "银龙宝宝的小雕像", PM_BABY_SILVER_DRAGON, NEUTRAL, FIGURINE },
    { "银龙宝宝罐头", PM_BABY_SILVER_DRAGON, NEUTRAL, TIN },
    { "银龙宝宝的罐头", PM_BABY_SILVER_DRAGON, NEUTRAL, TIN },
    { "银龙宝宝肉罐头", PM_BABY_SILVER_DRAGON, NEUTRAL, TIN },
    { "红龙宝宝雕像", PM_BABY_RED_DRAGON, NEUTRAL, STATUE },
    { "红龙宝宝的雕像", PM_BABY_RED_DRAGON, NEUTRAL, STATUE },
    { "红龙宝宝小雕像", PM_BABY_RED_DRAGON, NEUTRAL, FIGURINE },
    { "红龙宝宝的小雕像", PM_BABY_RED_DRAGON, NEUTRAL, FIGURINE },
    { "红龙宝宝罐头", PM_BABY_RED_DRAGON, NEUTRAL, TIN },
    { "红龙宝宝的罐头", PM_BABY_RED_DRAGON, NEUTRAL, TIN },
    { "红龙宝宝肉罐头", PM_BABY_RED_DRAGON, NEUTRAL, TIN },
    { "白龙宝宝雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, STATUE },
    { "白龙宝宝的雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, STATUE },
    { "白龙宝宝小雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, FIGURINE },
    { "白龙宝宝的小雕像", PM_BABY_WHITE_DRAGON, NEUTRAL, FIGURINE },
    { "白龙宝宝罐头", PM_BABY_WHITE_DRAGON, NEUTRAL, TIN },
    { "白龙宝宝的罐头", PM_BABY_WHITE_DRAGON, NEUTRAL, TIN },
    { "白龙宝宝肉罐头", PM_BABY_WHITE_DRAGON, NEUTRAL, TIN },
    { "橙龙宝宝雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, STATUE },
    { "橙龙宝宝的雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, STATUE },
    { "橙龙宝宝小雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, FIGURINE },
    { "橙龙宝宝的小雕像", PM_BABY_ORANGE_DRAGON, NEUTRAL, FIGURINE },
    { "橙龙宝宝罐头", PM_BABY_ORANGE_DRAGON, NEUTRAL, TIN },
    { "橙龙宝宝的罐头", PM_BABY_ORANGE_DRAGON, NEUTRAL, TIN },
    { "橙龙宝宝肉罐头", PM_BABY_ORANGE_DRAGON, NEUTRAL, TIN },
    { "黑龙宝宝雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, STATUE },
    { "黑龙宝宝的雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, STATUE },
    { "黑龙宝宝小雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, FIGURINE },
    { "黑龙宝宝的小雕像", PM_BABY_BLACK_DRAGON, NEUTRAL, FIGURINE },
    { "黑龙宝宝罐头", PM_BABY_BLACK_DRAGON, NEUTRAL, TIN },
    { "黑龙宝宝的罐头", PM_BABY_BLACK_DRAGON, NEUTRAL, TIN },
    { "黑龙宝宝肉罐头", PM_BABY_BLACK_DRAGON, NEUTRAL, TIN },
    { "蓝龙宝宝雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, STATUE },
    { "蓝龙宝宝的雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, STATUE },
    { "蓝龙宝宝小雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, FIGURINE },
    { "蓝龙宝宝的小雕像", PM_BABY_BLUE_DRAGON, NEUTRAL, FIGURINE },
    { "蓝龙宝宝罐头", PM_BABY_BLUE_DRAGON, NEUTRAL, TIN },
    { "蓝龙宝宝的罐头", PM_BABY_BLUE_DRAGON, NEUTRAL, TIN },
    { "蓝龙宝宝肉罐头", PM_BABY_BLUE_DRAGON, NEUTRAL, TIN },
    { "绿龙宝宝雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, STATUE },
    { "绿龙宝宝的雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, STATUE },
    { "绿龙宝宝小雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, FIGURINE },
    { "绿龙宝宝的小雕像", PM_BABY_GREEN_DRAGON, NEUTRAL, FIGURINE },
    { "绿龙宝宝罐头", PM_BABY_GREEN_DRAGON, NEUTRAL, TIN },
    { "绿龙宝宝的罐头", PM_BABY_GREEN_DRAGON, NEUTRAL, TIN },
    { "绿龙宝宝肉罐头", PM_BABY_GREEN_DRAGON, NEUTRAL, TIN },
    { "黄龙宝宝雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, STATUE },
    { "黄龙宝宝的雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, STATUE },
    { "黄龙宝宝小雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, FIGURINE },
    { "黄龙宝宝的小雕像", PM_BABY_YELLOW_DRAGON, NEUTRAL, FIGURINE },
    { "黄龙宝宝罐头", PM_BABY_YELLOW_DRAGON, NEUTRAL, TIN },
    { "黄龙宝宝的罐头", PM_BABY_YELLOW_DRAGON, NEUTRAL, TIN },
    { "黄龙宝宝肉罐头", PM_BABY_YELLOW_DRAGON, NEUTRAL, TIN },
    { "灰龙雕像", PM_GRAY_DRAGON, NEUTRAL, STATUE },
    { "灰龙的雕像", PM_GRAY_DRAGON, NEUTRAL, STATUE },
    { "灰龙小雕像", PM_GRAY_DRAGON, NEUTRAL, FIGURINE },
    { "灰龙的小雕像", PM_GRAY_DRAGON, NEUTRAL, FIGURINE },
    { "灰龙罐头", PM_GRAY_DRAGON, NEUTRAL, TIN },
    { "灰龙的罐头", PM_GRAY_DRAGON, NEUTRAL, TIN },
    { "灰龙肉罐头", PM_GRAY_DRAGON, NEUTRAL, TIN },
    { "金龙雕像", PM_GOLD_DRAGON, NEUTRAL, STATUE },
    { "金龙的雕像", PM_GOLD_DRAGON, NEUTRAL, STATUE },
    { "金龙小雕像", PM_GOLD_DRAGON, NEUTRAL, FIGURINE },
    { "金龙的小雕像", PM_GOLD_DRAGON, NEUTRAL, FIGURINE },
    { "金龙罐头", PM_GOLD_DRAGON, NEUTRAL, TIN },
    { "金龙的罐头", PM_GOLD_DRAGON, NEUTRAL, TIN },
    { "金龙肉罐头", PM_GOLD_DRAGON, NEUTRAL, TIN },
    { "银龙雕像", PM_SILVER_DRAGO, STATUEN},
    { "银龙的雕像", PM_SILVER_DRAGO, STATUEN},
    { "银龙小雕像", PM_SILVER_DRAGO, FIGURINEN},
    { "银龙的小雕像", PM_SILVER_DRAGO, FIGURINEN},
    { "银龙罐头", PM_SILVER_DRAGO, TINN},
    { "银龙的罐头", PM_SILVER_DRAGO, TINN},
    { "银龙肉罐头", PM_SILVER_DRAGO, TINN},
    { "红龙雕像", PM_RED_DRAGON, NEUTRAL, STATUE },
    { "红龙的雕像", PM_RED_DRAGON, NEUTRAL, STATUE },
    { "红龙小雕像", PM_RED_DRAGON, NEUTRAL, FIGURINE },
    { "红龙的小雕像", PM_RED_DRAGON, NEUTRAL, FIGURINE },
    { "红龙罐头", PM_RED_DRAGON, NEUTRAL, TIN },
    { "红龙的罐头", PM_RED_DRAGON, NEUTRAL, TIN },
    { "红龙肉罐头", PM_RED_DRAGON, NEUTRAL, TIN },
    { "白龙雕像", PM_WHITE_DRAGON, NEUTRAL, STATUE },
    { "白龙的雕像", PM_WHITE_DRAGON, NEUTRAL, STATUE },
    { "白龙小雕像", PM_WHITE_DRAGON, NEUTRAL, FIGURINE },
    { "白龙的小雕像", PM_WHITE_DRAGON, NEUTRAL, FIGURINE },
    { "白龙罐头", PM_WHITE_DRAGON, NEUTRAL, TIN },
    { "白龙的罐头", PM_WHITE_DRAGON, NEUTRAL, TIN },
    { "白龙肉罐头", PM_WHITE_DRAGON, NEUTRAL, TIN },
    { "橙龙雕像", PM_ORANGE_DRAGON, NEUTRAL, STATUE },
    { "橙龙的雕像", PM_ORANGE_DRAGON, NEUTRAL, STATUE },
    { "橙龙小雕像", PM_ORANGE_DRAGON, NEUTRAL, FIGURINE },
    { "橙龙的小雕像", PM_ORANGE_DRAGON, NEUTRAL, FIGURINE },
    { "橙龙罐头", PM_ORANGE_DRAGON, NEUTRAL, TIN },
    { "橙龙的罐头", PM_ORANGE_DRAGON, NEUTRAL, TIN },
    { "橙龙肉罐头", PM_ORANGE_DRAGON, NEUTRAL, TIN },
    { "黑龙雕像", PM_BLACK_DRAGON, NEUTRAL, STATUE },
    { "黑龙的雕像", PM_BLACK_DRAGON, NEUTRAL, STATUE },
    { "黑龙小雕像", PM_BLACK_DRAGON, NEUTRAL, FIGURINE },
    { "黑龙的小雕像", PM_BLACK_DRAGON, NEUTRAL, FIGURINE },
    { "黑龙罐头", PM_BLACK_DRAGON, NEUTRAL, TIN },
    { "黑龙的罐头", PM_BLACK_DRAGON, NEUTRAL, TIN },
    { "黑龙肉罐头", PM_BLACK_DRAGON, NEUTRAL, TIN },
    { "蓝龙雕像", PM_BLUE_DRAGON, NEUTRAL, STATUE },
    { "蓝龙的雕像", PM_BLUE_DRAGON, NEUTRAL, STATUE },
    { "蓝龙小雕像", PM_BLUE_DRAGON, NEUTRAL, FIGURINE },
    { "蓝龙的小雕像", PM_BLUE_DRAGON, NEUTRAL, FIGURINE },
    { "蓝龙罐头", PM_BLUE_DRAGON, NEUTRAL, TIN },
    { "蓝龙的罐头", PM_BLUE_DRAGON, NEUTRAL, TIN },
    { "蓝龙肉罐头", PM_BLUE_DRAGON, NEUTRAL, TIN },
    { "绿龙雕像", PM_GREEN_DRAGON, NEUTRAL, STATUE },
    { "绿龙的雕像", PM_GREEN_DRAGON, NEUTRAL, STATUE },
    { "绿龙小雕像", PM_GREEN_DRAGON, NEUTRAL, FIGURINE },
    { "绿龙的小雕像", PM_GREEN_DRAGON, NEUTRAL, FIGURINE },
    { "绿龙罐头", PM_GREEN_DRAGON, NEUTRAL, TIN },
    { "绿龙的罐头", PM_GREEN_DRAGON, NEUTRAL, TIN },
    { "绿龙肉罐头", PM_GREEN_DRAGON, NEUTRAL, TIN },
    { "黄龙雕像", PM_YELLOW_DRAGON, NEUTRAL, STATUE },
    { "黄龙的雕像", PM_YELLOW_DRAGON, NEUTRAL, STATUE },
    { "黄龙小雕像", PM_YELLOW_DRAGON, NEUTRAL, FIGURINE },
    { "黄龙的小雕像", PM_YELLOW_DRAGON, NEUTRAL, FIGURINE },
    { "黄龙罐头", PM_YELLOW_DRAGON, NEUTRAL, TIN },
    { "黄龙的罐头", PM_YELLOW_DRAGON, NEUTRAL, TIN },
    { "黄龙肉罐头", PM_YELLOW_DRAGON, NEUTRAL, TIN },
    { "潜行者雕像", PM_STALKER, NEUTRAL, STATUE },
    { "潜行者的雕像", PM_STALKER, NEUTRAL, STATUE },
    { "潜行者小雕像", PM_STALKER, NEUTRAL, FIGURINE },
    { "潜行者的小雕像", PM_STALKER, NEUTRAL, FIGURINE },
    { "潜行者罐头", PM_STALKER, NEUTRAL, TIN },
    { "潜行者的罐头", PM_STALKER, NEUTRAL, TIN },
    { "潜行者肉罐头", PM_STALKER, NEUTRAL, TIN },
    { "气元素雕像", PM_AIR_ELEMENTAL, NEUTRAL, STATUE },
    { "气元素的雕像", PM_AIR_ELEMENTAL, NEUTRAL, STATUE },
    { "气元素小雕像", PM_AIR_ELEMENTAL, NEUTRAL, FIGURINE },
    { "气元素的小雕像", PM_AIR_ELEMENTAL, NEUTRAL, FIGURINE },
    { "气元素罐头", PM_AIR_ELEMENTAL, NEUTRAL, TIN },
    { "气元素的罐头", PM_AIR_ELEMENTAL, NEUTRAL, TIN },
    { "气元素肉罐头", PM_AIR_ELEMENTAL, NEUTRAL, TIN },
    { "空气元素雕像", PM_AIR_ELEMENTAL, NEUTRAL, STATUE },
    { "空气元素的雕像", PM_AIR_ELEMENTAL, NEUTRAL, STATUE },
    { "空气元素小雕像", PM_AIR_ELEMENTAL, NEUTRAL, FIGURINE },
    { "空气元素的小雕像", PM_AIR_ELEMENTAL, NEUTRAL, FIGURINE },
    { "空气元素罐头", PM_AIR_ELEMENTAL, NEUTRAL, TIN },
    { "空气元素的罐头", PM_AIR_ELEMENTAL, NEUTRAL, TIN },
    { "空气元素肉罐头", PM_AIR_ELEMENTAL, NEUTRAL, TIN },
    { "火元素雕像", PM_FIRE_ELEMENTAL, NEUTRAL, STATUE },
    { "火元素的雕像", PM_FIRE_ELEMENTAL, NEUTRAL, STATUE },
    { "火元素小雕像", PM_FIRE_ELEMENTAL, NEUTRAL, FIGURINE },
    { "火元素的小雕像", PM_FIRE_ELEMENTAL, NEUTRAL, FIGURINE },
    { "火元素罐头", PM_FIRE_ELEMENTAL, NEUTRAL, TIN },
    { "火元素的罐头", PM_FIRE_ELEMENTAL, NEUTRAL, TIN },
    { "火元素肉罐头", PM_FIRE_ELEMENTAL, NEUTRAL, TIN },
    { "土元素雕像", PM_EARTH_ELEMENTAL, NEUTRAL, STATUE },
    { "土元素的雕像", PM_EARTH_ELEMENTAL, NEUTRAL, STATUE },
    { "土元素小雕像", PM_EARTH_ELEMENTAL, NEUTRAL, FIGURINE },
    { "土元素的小雕像", PM_EARTH_ELEMENTAL, NEUTRAL, FIGURINE },
    { "土元素罐头", PM_EARTH_ELEMENTAL, NEUTRAL, TIN },
    { "土元素的罐头", PM_EARTH_ELEMENTAL, NEUTRAL, TIN },
    { "土元素肉罐头", PM_EARTH_ELEMENTAL, NEUTRAL, TIN },
    { "水元素雕像", PM_WATER_ELEMENTAL, NEUTRAL, STATUE },
    { "水元素的雕像", PM_WATER_ELEMENTAL, NEUTRAL, STATUE },
    { "水元素小雕像", PM_WATER_ELEMENTAL, NEUTRAL, FIGURINE },
    { "水元素的小雕像", PM_WATER_ELEMENTAL, NEUTRAL, FIGURINE },
    { "水元素罐头", PM_WATER_ELEMENTAL, NEUTRAL, TIN },
    { "水元素的罐头", PM_WATER_ELEMENTAL, NEUTRAL, TIN },
    { "水元素肉罐头", PM_WATER_ELEMENTAL, NEUTRAL, TIN },
    { "地衣雕像", PM_LICHEN, NEUTRAL, STATUE },
    { "地衣的雕像", PM_LICHEN, NEUTRAL, STATUE },
    { "地衣小雕像", PM_LICHEN, NEUTRAL, FIGURINE },
    { "地衣的小雕像", PM_LICHEN, NEUTRAL, FIGURINE },
    { "地衣罐头", PM_LICHEN, NEUTRAL, TIN },
    { "地衣的罐头", PM_LICHEN, NEUTRAL, TIN },
    { "地衣肉罐头", PM_LICHEN, NEUTRAL, TIN },
    { "棕霉菌雕像", PM_BROWN_MOLD, NEUTRAL, STATUE },
    { "棕霉菌的雕像", PM_BROWN_MOLD, NEUTRAL, STATUE },
    { "棕霉菌小雕像", PM_BROWN_MOLD, NEUTRAL, FIGURINE },
    { "棕霉菌的小雕像", PM_BROWN_MOLD, NEUTRAL, FIGURINE },
    { "棕霉菌罐头", PM_BROWN_MOLD, NEUTRAL, TIN },
    { "棕霉菌的罐头", PM_BROWN_MOLD, NEUTRAL, TIN },
    { "棕霉菌肉罐头", PM_BROWN_MOLD, NEUTRAL, TIN },
    { "黄霉菌雕像", PM_YELLOW_MOLD, NEUTRAL, STATUE },
    { "黄霉菌的雕像", PM_YELLOW_MOLD, NEUTRAL, STATUE },
    { "黄霉菌小雕像", PM_YELLOW_MOLD, NEUTRAL, FIGURINE },
    { "黄霉菌的小雕像", PM_YELLOW_MOLD, NEUTRAL, FIGURINE },
    { "黄霉菌罐头", PM_YELLOW_MOLD, NEUTRAL, TIN },
    { "黄霉菌的罐头", PM_YELLOW_MOLD, NEUTRAL, TIN },
    { "黄霉菌肉罐头", PM_YELLOW_MOLD, NEUTRAL, TIN },
    { "绿霉菌雕像", PM_GREEN_MOLD, NEUTRAL, STATUE },
    { "绿霉菌的雕像", PM_GREEN_MOLD, NEUTRAL, STATUE },
    { "绿霉菌小雕像", PM_GREEN_MOLD, NEUTRAL, FIGURINE },
    { "绿霉菌的小雕像", PM_GREEN_MOLD, NEUTRAL, FIGURINE },
    { "绿霉菌罐头", PM_GREEN_MOLD, NEUTRAL, TIN },
    { "绿霉菌的罐头", PM_GREEN_MOLD, NEUTRAL, TIN },
    { "绿霉菌肉罐头", PM_GREEN_MOLD, NEUTRAL, TIN },
    { "红霉菌雕像", PM_RED_MOLD, NEUTRAL, STATUE },
    { "红霉菌的雕像", PM_RED_MOLD, NEUTRAL, STATUE },
    { "红霉菌小雕像", PM_RED_MOLD, NEUTRAL, FIGURINE },
    { "红霉菌的小雕像", PM_RED_MOLD, NEUTRAL, FIGURINE },
    { "红霉菌罐头", PM_RED_MOLD, NEUTRAL, TIN },
    { "红霉菌的罐头", PM_RED_MOLD, NEUTRAL, TIN },
    { "红霉菌肉罐头", PM_RED_MOLD, NEUTRAL, TIN },
    { "棕色霉菌雕像", PM_BROWN_MOLD, NEUTRAL, STATUE },
    { "棕色霉菌的雕像", PM_BROWN_MOLD, NEUTRAL, STATUE },
    { "棕色霉菌小雕像", PM_BROWN_MOLD, NEUTRAL, FIGURINE },
    { "棕色霉菌的小雕像", PM_BROWN_MOLD, NEUTRAL, FIGURINE },
    { "棕色霉菌罐头", PM_BROWN_MOLD, NEUTRAL, TIN },
    { "棕色霉菌的罐头", PM_BROWN_MOLD, NEUTRAL, TIN },
    { "棕色霉菌肉罐头", PM_BROWN_MOLD, NEUTRAL, TIN },
    { "黄色霉菌雕像", PM_YELLOW_MOLD, NEUTRAL, STATUE },
    { "黄色霉菌的雕像", PM_YELLOW_MOLD, NEUTRAL, STATUE },
    { "黄色霉菌小雕像", PM_YELLOW_MOLD, NEUTRAL, FIGURINE },
    { "黄色霉菌的小雕像", PM_YELLOW_MOLD, NEUTRAL, FIGURINE },
    { "黄色霉菌罐头", PM_YELLOW_MOLD, NEUTRAL, TIN },
    { "黄色霉菌的罐头", PM_YELLOW_MOLD, NEUTRAL, TIN },
    { "黄色霉菌肉罐头", PM_YELLOW_MOLD, NEUTRAL, TIN },
    { "绿色霉菌雕像", PM_GREEN_MOLD, NEUTRAL, STATUE },
    { "绿色霉菌的雕像", PM_GREEN_MOLD, NEUTRAL, STATUE },
    { "绿色霉菌小雕像", PM_GREEN_MOLD, NEUTRAL, FIGURINE },
    { "绿色霉菌的小雕像", PM_GREEN_MOLD, NEUTRAL, FIGURINE },
    { "绿色霉菌罐头", PM_GREEN_MOLD, NEUTRAL, TIN },
    { "绿色霉菌的罐头", PM_GREEN_MOLD, NEUTRAL, TIN },
    { "绿色霉菌肉罐头", PM_GREEN_MOLD, NEUTRAL, TIN },
    { "红色霉菌雕像", PM_RED_MOLD, NEUTRAL, STATUE },
    { "红色霉菌的雕像", PM_RED_MOLD, NEUTRAL, STATUE },
    { "红色霉菌小雕像", PM_RED_MOLD, NEUTRAL, FIGURINE },
    { "红色霉菌的小雕像", PM_RED_MOLD, NEUTRAL, FIGURINE },
    { "红色霉菌罐头", PM_RED_MOLD, NEUTRAL, TIN },
    { "红色霉菌的罐头", PM_RED_MOLD, NEUTRAL, TIN },
    { "红色霉菌肉罐头", PM_RED_MOLD, NEUTRAL, TIN },
    { "尖叫蕈雕像", PM_SHRIEKER, NEUTRAL, STATUE },
    { "尖叫蕈的雕像", PM_SHRIEKER, NEUTRAL, STATUE },
    { "尖叫蕈小雕像", PM_SHRIEKER, NEUTRAL, FIGURINE },
    { "尖叫蕈的小雕像", PM_SHRIEKER, NEUTRAL, FIGURINE },
    { "尖叫蕈罐头", PM_SHRIEKER, NEUTRAL, TIN },
    { "尖叫蕈的罐头", PM_SHRIEKER, NEUTRAL, TIN },
    { "尖叫蕈肉罐头", PM_SHRIEKER, NEUTRAL, TIN },
    { "紫真菌雕像", PM_VIOLET_FUNGUS, NEUTRAL, STATUE },
    { "紫真菌的雕像", PM_VIOLET_FUNGUS, NEUTRAL, STATUE },
    { "紫真菌小雕像", PM_VIOLET_FUNGUS, NEUTRAL, FIGURINE },
    { "紫真菌的小雕像", PM_VIOLET_FUNGUS, NEUTRAL, FIGURINE },
    { "紫真菌罐头", PM_VIOLET_FUNGUS, NEUTRAL, TIN },
    { "紫真菌的罐头", PM_VIOLET_FUNGUS, NEUTRAL, TIN },
    { "紫真菌肉罐头", PM_VIOLET_FUNGUS, NEUTRAL, TIN },
    { "紫色真菌雕像", PM_VIOLET_FUNGUS, NEUTRAL, STATUE },
    { "紫色真菌的雕像", PM_VIOLET_FUNGUS, NEUTRAL, STATUE },
    { "紫色真菌小雕像", PM_VIOLET_FUNGUS, NEUTRAL, FIGURINE },
    { "紫色真菌的小雕像", PM_VIOLET_FUNGUS, NEUTRAL, FIGURINE },
    { "紫色真菌罐头", PM_VIOLET_FUNGUS, NEUTRAL, TIN },
    { "紫色真菌的罐头", PM_VIOLET_FUNGUS, NEUTRAL, TIN },
    { "紫色真菌肉罐头", PM_VIOLET_FUNGUS, NEUTRAL, TIN },
    { "侏儒雕像", PM_GNOME, NEUTRAL, STATUE },
    { "侏儒的雕像", PM_GNOME, NEUTRAL, STATUE },
    { "侏儒小雕像", PM_GNOME, NEUTRAL, FIGURINE },
    { "侏儒的小雕像", PM_GNOME, NEUTRAL, FIGURINE },
    { "侏儒罐头", PM_GNOME, NEUTRAL, TIN },
    { "侏儒的罐头", PM_GNOME, NEUTRAL, TIN },
    { "侏儒肉罐头", PM_GNOME, NEUTRAL, TIN },
    { "侏儒领主雕像", PM_GNOME_LEADER, MALE, STATUE },
    { "侏儒领主的雕像", PM_GNOME_LEADER, MALE, STATUE },
    { "侏儒领主小雕像", PM_GNOME_LEADER, MALE, FIGURINE },
    { "侏儒领主的小雕像", PM_GNOME_LEADER, MALE, FIGURINE },
    { "侏儒领主罐头", PM_GNOME_LEADER, MALE, TIN },
    { "侏儒领主的罐头", PM_GNOME_LEADER, MALE, TIN },
    { "侏儒领主肉罐头", PM_GNOME_LEADER, MALE, TIN },
    { "侏儒女领主雕像", PM_GNOME_LEADER, FEMALE, STATUE },
    { "侏儒女领主的雕像", PM_GNOME_LEADER, FEMALE, STATUE },
    { "侏儒女领主小雕像", PM_GNOME_LEADER, FEMALE, FIGURINE },
    { "侏儒女领主的小雕像", PM_GNOME_LEADER, FEMALE, FIGURINE },
    { "侏儒女领主罐头", PM_GNOME_LEADER, FEMALE, TIN },
    { "侏儒女领主的罐头", PM_GNOME_LEADER, FEMALE, TIN },
    { "侏儒女领主肉罐头", PM_GNOME_LEADER, FEMALE, TIN },
    { "侏儒领袖雕像", PM_GNOME_LEADER, NEUTRAL, STATUE },
    { "侏儒领袖的雕像", PM_GNOME_LEADER, NEUTRAL, STATUE },
    { "侏儒领袖小雕像", PM_GNOME_LEADER, NEUTRAL, FIGURINE },
    { "侏儒领袖的小雕像", PM_GNOME_LEADER, NEUTRAL, FIGURINE },
    { "侏儒领袖罐头", PM_GNOME_LEADER, NEUTRAL, TIN },
    { "侏儒领袖的罐头", PM_GNOME_LEADER, NEUTRAL, TIN },
    { "侏儒领袖肉罐头", PM_GNOME_LEADER, NEUTRAL, TIN },
    { "侏儒巫师雕像", PM_GNOMISH_WIZARD, NEUTRAL, STATUE },
    { "侏儒巫师的雕像", PM_GNOMISH_WIZARD, NEUTRAL, STATUE },
    { "侏儒巫师小雕像", PM_GNOMISH_WIZARD, NEUTRAL, FIGURINE },
    { "侏儒巫师的小雕像", PM_GNOMISH_WIZARD, NEUTRAL, FIGURINE },
    { "侏儒巫师罐头", PM_GNOMISH_WIZARD, NEUTRAL, TIN },
    { "侏儒巫师的罐头", PM_GNOMISH_WIZARD, NEUTRAL, TIN },
    { "侏儒巫师肉罐头", PM_GNOMISH_WIZARD, NEUTRAL, TIN },
    { "侏儒王雕像", PM_GNOME_RULER, MALE, STATUE },
    { "侏儒王的雕像", PM_GNOME_RULER, MALE, STATUE },
    { "侏儒王小雕像", PM_GNOME_RULER, MALE, FIGURINE },
    { "侏儒王的小雕像", PM_GNOME_RULER, MALE, FIGURINE },
    { "侏儒王罐头", PM_GNOME_RULER, MALE, TIN },
    { "侏儒王的罐头", PM_GNOME_RULER, MALE, TIN },
    { "侏儒王肉罐头", PM_GNOME_RULER, MALE, TIN },
    { "侏儒女王雕像", PM_GNOME_RULER, FEMALE, STATUE },
    { "侏儒女王的雕像", PM_GNOME_RULER, FEMALE, STATUE },
    { "侏儒女王小雕像", PM_GNOME_RULER, FEMALE, FIGURINE },
    { "侏儒女王的小雕像", PM_GNOME_RULER, FEMALE, FIGURINE },
    { "侏儒女王罐头", PM_GNOME_RULER, FEMALE, TIN },
    { "侏儒女王的罐头", PM_GNOME_RULER, FEMALE, TIN },
    { "侏儒女王肉罐头", PM_GNOME_RULER, FEMALE, TIN },
    { "侏儒统治者雕像", PM_GNOME_RULER, NEUTRAL, STATUE },
    { "侏儒统治者的雕像", PM_GNOME_RULER, NEUTRAL, STATUE },
    { "侏儒统治者小雕像", PM_GNOME_RULER, NEUTRAL, FIGURINE },
    { "侏儒统治者的小雕像", PM_GNOME_RULER, NEUTRAL, FIGURINE },
    { "侏儒统治者罐头", PM_GNOME_RULER, NEUTRAL, TIN },
    { "侏儒统治者的罐头", PM_GNOME_RULER, NEUTRAL, TIN },
    { "侏儒统治者肉罐头", PM_GNOME_RULER, NEUTRAL, TIN },
    { "巨人雕像", PM_GIANT, NEUTRAL, STATUE },
    { "巨人的雕像", PM_GIANT, NEUTRAL, STATUE },
    { "巨人小雕像", PM_GIANT, NEUTRAL, FIGURINE },
    { "巨人的小雕像", PM_GIANT, NEUTRAL, FIGURINE },
    { "巨人罐头", PM_GIANT, NEUTRAL, TIN },
    { "巨人的罐头", PM_GIANT, NEUTRAL, TIN },
    { "巨人肉罐头", PM_GIANT, NEUTRAL, TIN },
    { "石头巨人雕像", PM_STONE_GIANT, NEUTRAL, STATUE },
    { "石头巨人的雕像", PM_STONE_GIANT, NEUTRAL, STATUE },
    { "石头巨人小雕像", PM_STONE_GIANT, NEUTRAL, FIGURINE },
    { "石头巨人的小雕像", PM_STONE_GIANT, NEUTRAL, FIGURINE },
    { "石头巨人罐头", PM_STONE_GIANT, NEUTRAL, TIN },
    { "石头巨人的罐头", PM_STONE_GIANT, NEUTRAL, TIN },
    { "石头巨人肉罐头", PM_STONE_GIANT, NEUTRAL, TIN },
    { "石巨人雕像", PM_STONE_GIANT, NEUTRAL, STATUE },
    { "石巨人的雕像", PM_STONE_GIANT, NEUTRAL, STATUE },
    { "石巨人小雕像", PM_STONE_GIANT, NEUTRAL, FIGURINE },
    { "石巨人的小雕像", PM_STONE_GIANT, NEUTRAL, FIGURINE },
    { "石巨人罐头", PM_STONE_GIANT, NEUTRAL, TIN },
    { "石巨人的罐头", PM_STONE_GIANT, NEUTRAL, TIN },
    { "石巨人肉罐头", PM_STONE_GIANT, NEUTRAL, TIN },
    { "丘陵巨人雕像", PM_HILL_GIANT, NEUTRAL, STATUE },
    { "丘陵巨人的雕像", PM_HILL_GIANT, NEUTRAL, STATUE },
    { "丘陵巨人小雕像", PM_HILL_GIANT, NEUTRAL, FIGURINE },
    { "丘陵巨人的小雕像", PM_HILL_GIANT, NEUTRAL, FIGURINE },
    { "丘陵巨人罐头", PM_HILL_GIANT, NEUTRAL, TIN },
    { "丘陵巨人的罐头", PM_HILL_GIANT, NEUTRAL, TIN },
    { "丘陵巨人肉罐头", PM_HILL_GIANT, NEUTRAL, TIN },
    { "火巨人雕像", PM_FIRE_GIANT, NEUTRAL, STATUE },
    { "火巨人的雕像", PM_FIRE_GIANT, NEUTRAL, STATUE },
    { "火巨人小雕像", PM_FIRE_GIANT, NEUTRAL, FIGURINE },
    { "火巨人的小雕像", PM_FIRE_GIANT, NEUTRAL, FIGURINE },
    { "火巨人罐头", PM_FIRE_GIANT, NEUTRAL, TIN },
    { "火巨人的罐头", PM_FIRE_GIANT, NEUTRAL, TIN },
    { "火巨人肉罐头", PM_FIRE_GIANT, NEUTRAL, TIN },
    { "火焰巨人雕像", PM_FIRE_GIANT, NEUTRAL, STATUE },
    { "火焰巨人的雕像", PM_FIRE_GIANT, NEUTRAL, STATUE },
    { "火焰巨人小雕像", PM_FIRE_GIANT, NEUTRAL, FIGURINE },
    { "火焰巨人的小雕像", PM_FIRE_GIANT, NEUTRAL, FIGURINE },
    { "火焰巨人罐头", PM_FIRE_GIANT, NEUTRAL, TIN },
    { "火焰巨人的罐头", PM_FIRE_GIANT, NEUTRAL, TIN },
    { "火焰巨人肉罐头", PM_FIRE_GIANT, NEUTRAL, TIN },
    { "霜巨人雕像", PM_FROST_GIANT, NEUTRAL, STATUE },
    { "霜巨人的雕像", PM_FROST_GIANT, NEUTRAL, STATUE },
    { "霜巨人小雕像", PM_FROST_GIANT, NEUTRAL, FIGURINE },
    { "霜巨人的小雕像", PM_FROST_GIANT, NEUTRAL, FIGURINE },
    { "霜巨人罐头", PM_FROST_GIANT, NEUTRAL, TIN },
    { "霜巨人的罐头", PM_FROST_GIANT, NEUTRAL, TIN },
    { "霜巨人肉罐头", PM_FROST_GIANT, NEUTRAL, TIN },
    { "冰霜巨人雕像", PM_FROST_GIANT, NEUTRAL, STATUE },
    { "冰霜巨人的雕像", PM_FROST_GIANT, NEUTRAL, STATUE },
    { "冰霜巨人小雕像", PM_FROST_GIANT, NEUTRAL, FIGURINE },
    { "冰霜巨人的小雕像", PM_FROST_GIANT, NEUTRAL, FIGURINE },
    { "冰霜巨人罐头", PM_FROST_GIANT, NEUTRAL, TIN },
    { "冰霜巨人的罐头", PM_FROST_GIANT, NEUTRAL, TIN },
    { "冰霜巨人肉罐头", PM_FROST_GIANT, NEUTRAL, TIN },
    { "双头巨人雕像", PM_ETTIN, NEUTRAL, STATUE },
    { "双头巨人的雕像", PM_ETTIN, NEUTRAL, STATUE },
    { "双头巨人小雕像", PM_ETTIN, NEUTRAL, FIGURINE },
    { "双头巨人的小雕像", PM_ETTIN, NEUTRAL, FIGURINE },
    { "双头巨人罐头", PM_ETTIN, NEUTRAL, TIN },
    { "双头巨人的罐头", PM_ETTIN, NEUTRAL, TIN },
    { "双头巨人肉罐头", PM_ETTIN, NEUTRAL, TIN },
    { "风巨人雕像", PM_STORM_GIANT, NEUTRAL, STATUE },
    { "风巨人的雕像", PM_STORM_GIANT, NEUTRAL, STATUE },
    { "风巨人小雕像", PM_STORM_GIANT, NEUTRAL, FIGURINE },
    { "风巨人的小雕像", PM_STORM_GIANT, NEUTRAL, FIGURINE },
    { "风巨人罐头", PM_STORM_GIANT, NEUTRAL, TIN },
    { "风巨人的罐头", PM_STORM_GIANT, NEUTRAL, TIN },
    { "风巨人肉罐头", PM_STORM_GIANT, NEUTRAL, TIN },
    { "风暴巨人雕像", PM_STORM_GIANT, NEUTRAL, STATUE },
    { "风暴巨人的雕像", PM_STORM_GIANT, NEUTRAL, STATUE },
    { "风暴巨人小雕像", PM_STORM_GIANT, NEUTRAL, FIGURINE },
    { "风暴巨人的小雕像", PM_STORM_GIANT, NEUTRAL, FIGURINE },
    { "风暴巨人罐头", PM_STORM_GIANT, NEUTRAL, TIN },
    { "风暴巨人的罐头", PM_STORM_GIANT, NEUTRAL, TIN },
    { "风暴巨人肉罐头", PM_STORM_GIANT, NEUTRAL, TIN },
    { "提坦雕像", PM_TITAN, NEUTRAL, STATUE },
    { "提坦的雕像", PM_TITAN, NEUTRAL, STATUE },
    { "提坦小雕像", PM_TITAN, NEUTRAL, FIGURINE },
    { "提坦的小雕像", PM_TITAN, NEUTRAL, FIGURINE },
    { "提坦罐头", PM_TITAN, NEUTRAL, TIN },
    { "提坦的罐头", PM_TITAN, NEUTRAL, TIN },
    { "提坦肉罐头", PM_TITAN, NEUTRAL, TIN },
    { "泰坦雕像", PM_TITAN, NEUTRAL, STATUE },
    { "泰坦的雕像", PM_TITAN, NEUTRAL, STATUE },
    { "泰坦小雕像", PM_TITAN, NEUTRAL, FIGURINE },
    { "泰坦的小雕像", PM_TITAN, NEUTRAL, FIGURINE },
    { "泰坦罐头", PM_TITAN, NEUTRAL, TIN },
    { "泰坦的罐头", PM_TITAN, NEUTRAL, TIN },
    { "泰坦肉罐头", PM_TITAN, NEUTRAL, TIN },
    { "弥诺陶洛斯雕像", PM_MINOTAUR, NEUTRAL, STATUE },
    { "弥诺陶洛斯的雕像", PM_MINOTAUR, NEUTRAL, STATUE },
    { "弥诺陶洛斯小雕像", PM_MINOTAUR, NEUTRAL, FIGURINE },
    { "弥诺陶洛斯的小雕像", PM_MINOTAUR, NEUTRAL, FIGURINE },
    { "弥诺陶洛斯罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "弥诺陶洛斯的罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "弥诺陶洛斯肉罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "米诺陶洛斯雕像", PM_MINOTAUR, NEUTRAL, STATUE },
    { "米诺陶洛斯的雕像", PM_MINOTAUR, NEUTRAL, STATUE },
    { "米诺陶洛斯小雕像", PM_MINOTAUR, NEUTRAL, FIGURINE },
    { "米诺陶洛斯的小雕像", PM_MINOTAUR, NEUTRAL, FIGURINE },
    { "米诺陶洛斯罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "米诺陶洛斯的罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "米诺陶洛斯肉罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "米诺陶雕像", PM_MINOTAUR, NEUTRAL, STATUE },
    { "米诺陶的雕像", PM_MINOTAUR, NEUTRAL, STATUE },
    { "米诺陶小雕像", PM_MINOTAUR, NEUTRAL, FIGURINE },
    { "米诺陶的小雕像", PM_MINOTAUR, NEUTRAL, FIGURINE },
    { "米诺陶罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "米诺陶的罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "米诺陶肉罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "牛头人雕像", PM_MINOTAUR, NEUTRAL, STATUE },
    { "牛头人的雕像", PM_MINOTAUR, NEUTRAL, STATUE },
    { "牛头人小雕像", PM_MINOTAUR, NEUTRAL, FIGURINE },
    { "牛头人的小雕像", PM_MINOTAUR, NEUTRAL, FIGURINE },
    { "牛头人罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "牛头人的罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "牛头人肉罐头", PM_MINOTAUR, NEUTRAL, TIN },
    { "颊脖龙雕像", PM_JABBERWOCK, NEUTRAL, STATUE },
    { "颊脖龙的雕像", PM_JABBERWOCK, NEUTRAL, STATUE },
    { "颊脖龙小雕像", PM_JABBERWOCK, NEUTRAL, FIGURINE },
    { "颊脖龙的小雕像", PM_JABBERWOCK, NEUTRAL, FIGURINE },
    { "颊脖龙罐头", PM_JABBERWOCK, NEUTRAL, TIN },
    { "颊脖龙的罐头", PM_JABBERWOCK, NEUTRAL, TIN },
    { "颊脖龙肉罐头", PM_JABBERWOCK, NEUTRAL, TIN },
    { "贾巴沃克雕像", PM_JABBERWOCK, NEUTRAL, STATUE },
    { "贾巴沃克的雕像", PM_JABBERWOCK, NEUTRAL, STATUE },
    { "贾巴沃克小雕像", PM_JABBERWOCK, NEUTRAL, FIGURINE },
    { "贾巴沃克的小雕像", PM_JABBERWOCK, NEUTRAL, FIGURINE },
    { "贾巴沃克罐头", PM_JABBERWOCK, NEUTRAL, TIN },
    { "贾巴沃克的罐头", PM_JABBERWOCK, NEUTRAL, TIN },
    { "贾巴沃克肉罐头", PM_JABBERWOCK, NEUTRAL, TIN },
    { "吉斯通警察雕像", PM_KEYSTONE_KOP, NEUTRAL, STATUE },
    { "吉斯通警察的雕像", PM_KEYSTONE_KOP, NEUTRAL, STATUE },
    { "吉斯通警察小雕像", PM_KEYSTONE_KOP, NEUTRAL, FIGURINE },
    { "吉斯通警察的小雕像", PM_KEYSTONE_KOP, NEUTRAL, FIGURINE },
    { "吉斯通警察罐头", PM_KEYSTONE_KOP, NEUTRAL, TIN },
    { "吉斯通警察的罐头", PM_KEYSTONE_KOP, NEUTRAL, TIN },
    { "吉斯通警察肉罐头", PM_KEYSTONE_KOP, NEUTRAL, TIN },
    { "吉斯通警司雕像", PM_KOP_SERGEANT, NEUTRAL, STATUE },
    { "吉斯通警司的雕像", PM_KOP_SERGEANT, NEUTRAL, STATUE },
    { "吉斯通警司小雕像", PM_KOP_SERGEANT, NEUTRAL, FIGURINE },
    { "吉斯通警司的小雕像", PM_KOP_SERGEANT, NEUTRAL, FIGURINE },
    { "吉斯通警司罐头", PM_KOP_SERGEANT, NEUTRAL, TIN },
    { "吉斯通警司的罐头", PM_KOP_SERGEANT, NEUTRAL, TIN },
    { "吉斯通警司肉罐头", PM_KOP_SERGEANT, NEUTRAL, TIN },
    { "吉斯通警督雕像", PM_KOP_LIEUTENANT, NEUTRAL, STATUE },
    { "吉斯通警督的雕像", PM_KOP_LIEUTENANT, NEUTRAL, STATUE },
    { "吉斯通警督小雕像", PM_KOP_LIEUTENANT, NEUTRAL, FIGURINE },
    { "吉斯通警督的小雕像", PM_KOP_LIEUTENANT, NEUTRAL, FIGURINE },
    { "吉斯通警督罐头", PM_KOP_LIEUTENANT, NEUTRAL, TIN },
    { "吉斯通警督的罐头", PM_KOP_LIEUTENANT, NEUTRAL, TIN },
    { "吉斯通警督肉罐头", PM_KOP_LIEUTENANT, NEUTRAL, TIN },
    { "吉斯通警监雕像", PM_KOP_KAPTAIN, NEUTRAL, STATUE },
    { "吉斯通警监的雕像", PM_KOP_KAPTAIN, NEUTRAL, STATUE },
    { "吉斯通警监小雕像", PM_KOP_KAPTAIN, NEUTRAL, FIGURINE },
    { "吉斯通警监的小雕像", PM_KOP_KAPTAIN, NEUTRAL, FIGURINE },
    { "吉斯通警监罐头", PM_KOP_KAPTAIN, NEUTRAL, TIN },
    { "吉斯通警监的罐头", PM_KOP_KAPTAIN, NEUTRAL, TIN },
    { "吉斯通警监肉罐头", PM_KOP_KAPTAIN, NEUTRAL, TIN },
    { "巫妖雕像", PM_LICH, NEUTRAL, STATUE },
    { "巫妖的雕像", PM_LICH, NEUTRAL, STATUE },
    { "巫妖小雕像", PM_LICH, NEUTRAL, FIGURINE },
    { "巫妖的小雕像", PM_LICH, NEUTRAL, FIGURINE },
    { "巫妖罐头", PM_LICH, NEUTRAL, TIN },
    { "巫妖的罐头", PM_LICH, NEUTRAL, TIN },
    { "巫妖肉罐头", PM_LICH, NEUTRAL, TIN },
    { "半巫妖雕像", PM_DEMILICH, NEUTRAL, STATUE },
    { "半巫妖的雕像", PM_DEMILICH, NEUTRAL, STATUE },
    { "半巫妖小雕像", PM_DEMILICH, NEUTRAL, FIGURINE },
    { "半巫妖的小雕像", PM_DEMILICH, NEUTRAL, FIGURINE },
    { "半巫妖罐头", PM_DEMILICH, NEUTRAL, TIN },
    { "半巫妖的罐头", PM_DEMILICH, NEUTRAL, TIN },
    { "半巫妖肉罐头", PM_DEMILICH, NEUTRAL, TIN },
    { "巫妖大师雕像", PM_MASTER_LICH, NEUTRAL, STATUE },
    { "巫妖大师的雕像", PM_MASTER_LICH, NEUTRAL, STATUE },
    { "巫妖大师小雕像", PM_MASTER_LICH, NEUTRAL, FIGURINE },
    { "巫妖大师的小雕像", PM_MASTER_LICH, NEUTRAL, FIGURINE },
    { "巫妖大师罐头", PM_MASTER_LICH, NEUTRAL, TIN },
    { "巫妖大师的罐头", PM_MASTER_LICH, NEUTRAL, TIN },
    { "巫妖大师肉罐头", PM_MASTER_LICH, NEUTRAL, TIN },
    { "主宰巫妖雕像", PM_MASTER_LICH, NEUTRAL, STATUE },
    { "主宰巫妖的雕像", PM_MASTER_LICH, NEUTRAL, STATUE },
    { "主宰巫妖小雕像", PM_MASTER_LICH, NEUTRAL, FIGURINE },
    { "主宰巫妖的小雕像", PM_MASTER_LICH, NEUTRAL, FIGURINE },
    { "主宰巫妖罐头", PM_MASTER_LICH, NEUTRAL, TIN },
    { "主宰巫妖的罐头", PM_MASTER_LICH, NEUTRAL, TIN },
    { "主宰巫妖肉罐头", PM_MASTER_LICH, NEUTRAL, TIN },
    { "大巫妖雕像", PM_ARCH_LICH, NEUTRAL, STATUE },
    { "大巫妖的雕像", PM_ARCH_LICH, NEUTRAL, STATUE },
    { "大巫妖小雕像", PM_ARCH_LICH, NEUTRAL, FIGURINE },
    { "大巫妖的小雕像", PM_ARCH_LICH, NEUTRAL, FIGURINE },
    { "大巫妖罐头", PM_ARCH_LICH, NEUTRAL, TIN },
    { "大巫妖的罐头", PM_ARCH_LICH, NEUTRAL, TIN },
    { "大巫妖肉罐头", PM_ARCH_LICH, NEUTRAL, TIN },
    { "狗头人木乃伊雕像", PM_KOBOLD_MUMMY, NEUTRAL, STATUE },
    { "狗头人木乃伊的雕像", PM_KOBOLD_MUMMY, NEUTRAL, STATUE },
    { "狗头人木乃伊小雕像", PM_KOBOLD_MUMMY, NEUTRAL, FIGURINE },
    { "狗头人木乃伊的小雕像", PM_KOBOLD_MUMMY, NEUTRAL, FIGURINE },
    { "狗头人木乃伊罐头", PM_KOBOLD_MUMMY, NEUTRAL, TIN },
    { "狗头人木乃伊的罐头", PM_KOBOLD_MUMMY, NEUTRAL, TIN },
    { "狗头人木乃伊肉罐头", PM_KOBOLD_MUMMY, NEUTRAL, TIN },
    { "侏儒木乃伊雕像", PM_GNOME_MUMMY, NEUTRAL, STATUE },
    { "侏儒木乃伊的雕像", PM_GNOME_MUMMY, NEUTRAL, STATUE },
    { "侏儒木乃伊小雕像", PM_GNOME_MUMMY, NEUTRAL, FIGURINE },
    { "侏儒木乃伊的小雕像", PM_GNOME_MUMMY, NEUTRAL, FIGURINE },
    { "侏儒木乃伊罐头", PM_GNOME_MUMMY, NEUTRAL, TIN },
    { "侏儒木乃伊的罐头", PM_GNOME_MUMMY, NEUTRAL, TIN },
    { "侏儒木乃伊肉罐头", PM_GNOME_MUMMY, NEUTRAL, TIN },
    { "兽人木乃伊雕像", PM_ORC_MUMMY, NEUTRAL, STATUE },
    { "兽人木乃伊的雕像", PM_ORC_MUMMY, NEUTRAL, STATUE },
    { "兽人木乃伊小雕像", PM_ORC_MUMMY, NEUTRAL, FIGURINE },
    { "兽人木乃伊的小雕像", PM_ORC_MUMMY, NEUTRAL, FIGURINE },
    { "兽人木乃伊罐头", PM_ORC_MUMMY, NEUTRAL, TIN },
    { "兽人木乃伊的罐头", PM_ORC_MUMMY, NEUTRAL, TIN },
    { "兽人木乃伊肉罐头", PM_ORC_MUMMY, NEUTRAL, TIN },
    { "矮人木乃伊雕像", PM_DWARF_MUMMY, NEUTRAL, STATUE },
    { "矮人木乃伊的雕像", PM_DWARF_MUMMY, NEUTRAL, STATUE },
    { "矮人木乃伊小雕像", PM_DWARF_MUMMY, NEUTRAL, FIGURINE },
    { "矮人木乃伊的小雕像", PM_DWARF_MUMMY, NEUTRAL, FIGURINE },
    { "矮人木乃伊罐头", PM_DWARF_MUMMY, NEUTRAL, TIN },
    { "矮人木乃伊的罐头", PM_DWARF_MUMMY, NEUTRAL, TIN },
    { "矮人木乃伊肉罐头", PM_DWARF_MUMMY, NEUTRAL, TIN },
    { "精灵木乃伊雕像", PM_ELF_MUMMY, NEUTRAL, STATUE },
    { "精灵木乃伊的雕像", PM_ELF_MUMMY, NEUTRAL, STATUE },
    { "精灵木乃伊小雕像", PM_ELF_MUMMY, NEUTRAL, FIGURINE },
    { "精灵木乃伊的小雕像", PM_ELF_MUMMY, NEUTRAL, FIGURINE },
    { "精灵木乃伊罐头", PM_ELF_MUMMY, NEUTRAL, TIN },
    { "精灵木乃伊的罐头", PM_ELF_MUMMY, NEUTRAL, TIN },
    { "精灵木乃伊肉罐头", PM_ELF_MUMMY, NEUTRAL, TIN },
    { "人类木乃伊雕像", PM_HUMAN_MUMMY, NEUTRAL, STATUE },
    { "人类木乃伊的雕像", PM_HUMAN_MUMMY, NEUTRAL, STATUE },
    { "人类木乃伊小雕像", PM_HUMAN_MUMMY, NEUTRAL, FIGURINE },
    { "人类木乃伊的小雕像", PM_HUMAN_MUMMY, NEUTRAL, FIGURINE },
    { "人类木乃伊罐头", PM_HUMAN_MUMMY, NEUTRAL, TIN },
    { "人类木乃伊的罐头", PM_HUMAN_MUMMY, NEUTRAL, TIN },
    { "人类木乃伊肉罐头", PM_HUMAN_MUMMY, NEUTRAL, TIN },
    { "双头木乃伊雕像", PM_ETTIN_MUMMY, NEUTRAL, STATUE },
    { "双头木乃伊的雕像", PM_ETTIN_MUMMY, NEUTRAL, STATUE },
    { "双头木乃伊小雕像", PM_ETTIN_MUMMY, NEUTRAL, FIGURINE },
    { "双头木乃伊的小雕像", PM_ETTIN_MUMMY, NEUTRAL, FIGURINE },
    { "双头木乃伊罐头", PM_ETTIN_MUMMY, NEUTRAL, TIN },
    { "双头木乃伊的罐头", PM_ETTIN_MUMMY, NEUTRAL, TIN },
    { "双头木乃伊肉罐头", PM_ETTIN_MUMMY, NEUTRAL, TIN },
    { "双头巨人木乃伊雕像", PM_ETTIN_MUMMY, NEUTRAL, STATUE },
    { "双头巨人木乃伊的雕像", PM_ETTIN_MUMMY, NEUTRAL, STATUE },
    { "双头巨人木乃伊小雕像", PM_ETTIN_MUMMY, NEUTRAL, FIGURINE },
    { "双头巨人木乃伊的小雕像", PM_ETTIN_MUMMY, NEUTRAL, FIGURINE },
    { "双头巨人木乃伊罐头", PM_ETTIN_MUMMY, NEUTRAL, TIN },
    { "双头巨人木乃伊的罐头", PM_ETTIN_MUMMY, NEUTRAL, TIN },
    { "双头巨人木乃伊肉罐头", PM_ETTIN_MUMMY, NEUTRAL, TIN },
    { "巨人木乃伊雕像", PM_GIANT_MUMMY, NEUTRAL, STATUE },
    { "巨人木乃伊的雕像", PM_GIANT_MUMMY, NEUTRAL, STATUE },
    { "巨人木乃伊小雕像", PM_GIANT_MUMMY, NEUTRAL, FIGURINE },
    { "巨人木乃伊的小雕像", PM_GIANT_MUMMY, NEUTRAL, FIGURINE },
    { "巨人木乃伊罐头", PM_GIANT_MUMMY, NEUTRAL, TIN },
    { "巨人木乃伊的罐头", PM_GIANT_MUMMY, NEUTRAL, TIN },
    { "巨人木乃伊肉罐头", PM_GIANT_MUMMY, NEUTRAL, TIN },
    { "红幼纳迦雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "红幼纳迦的雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "红幼纳迦小雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "红幼纳迦的小雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "红幼纳迦罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "红幼纳迦的罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "红幼纳迦肉罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "黑幼纳迦雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "黑幼纳迦的雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "黑幼纳迦小雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "黑幼纳迦的小雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "黑幼纳迦罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "黑幼纳迦的罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "黑幼纳迦肉罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "金幼纳迦雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "金幼纳迦的雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "金幼纳迦小雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "金幼纳迦的小雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "金幼纳迦罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "金幼纳迦的罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "金幼纳迦肉罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼纳迦守卫雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "幼纳迦守卫的雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "幼纳迦守卫小雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "幼纳迦守卫的小雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "幼纳迦守卫罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼纳迦守卫的罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼纳迦守卫肉罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼红纳迦雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "幼红纳迦的雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "幼红纳迦小雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "幼红纳迦的小雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "幼红纳迦罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼红纳迦的罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼红纳迦肉罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼黑纳迦雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "幼黑纳迦的雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "幼黑纳迦小雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "幼黑纳迦的小雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "幼黑纳迦罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼黑纳迦的罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼黑纳迦肉罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼金纳迦雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "幼金纳迦的雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "幼金纳迦小雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "幼金纳迦的小雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "幼金纳迦罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼金纳迦的罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼金纳迦肉罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼纳迦守卫雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "幼纳迦守卫的雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "幼纳迦守卫小雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "幼纳迦守卫的小雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "幼纳迦守卫罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼纳迦守卫的罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "幼纳迦守卫肉罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小红纳迦雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "小红纳迦的雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "小红纳迦小雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "小红纳迦的小雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "小红纳迦罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小红纳迦的罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小红纳迦肉罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小黑纳迦雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "小黑纳迦的雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "小黑纳迦小雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "小黑纳迦的小雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "小黑纳迦罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小黑纳迦的罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小黑纳迦肉罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小金纳迦雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "小金纳迦的雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "小金纳迦小雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "小金纳迦的小雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "小金纳迦罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小金纳迦的罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小金纳迦肉罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小纳迦守卫雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "小纳迦守卫的雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "小纳迦守卫小雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "小纳迦守卫的小雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "小纳迦守卫罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小纳迦守卫的罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "小纳迦守卫肉罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "红纳迦宝宝雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "红纳迦宝宝的雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "红纳迦宝宝小雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "红纳迦宝宝的小雕像", PM_RED_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "红纳迦宝宝罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "红纳迦宝宝的罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "红纳迦宝宝肉罐头", PM_RED_NAGA_HATCHLING, NEUTRAL, TIN },
    { "黑纳迦宝宝雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "黑纳迦宝宝的雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "黑纳迦宝宝小雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "黑纳迦宝宝的小雕像", PM_BLACK_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "黑纳迦宝宝罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "黑纳迦宝宝的罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "黑纳迦宝宝肉罐头", PM_BLACK_NAGA_HATCHLING, NEUTRAL, TIN },
    { "金纳迦宝宝雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "金纳迦宝宝的雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "金纳迦宝宝小雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "金纳迦宝宝的小雕像", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "金纳迦宝宝罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "金纳迦宝宝的罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "金纳迦宝宝肉罐头", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "纳迦守卫宝宝雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "纳迦守卫宝宝的雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, STATUE },
    { "纳迦守卫宝宝小雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "纳迦守卫宝宝的小雕像", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, FIGURINE },
    { "纳迦守卫宝宝罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "纳迦守卫宝宝的罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "纳迦守卫宝宝肉罐头", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL, TIN },
    { "红纳迦雕像", PM_RED_NAGA, NEUTRAL, STATUE },
    { "红纳迦的雕像", PM_RED_NAGA, NEUTRAL, STATUE },
    { "红纳迦小雕像", PM_RED_NAGA, NEUTRAL, FIGURINE },
    { "红纳迦的小雕像", PM_RED_NAGA, NEUTRAL, FIGURINE },
    { "红纳迦罐头", PM_RED_NAGA, NEUTRAL, TIN },
    { "红纳迦的罐头", PM_RED_NAGA, NEUTRAL, TIN },
    { "红纳迦肉罐头", PM_RED_NAGA, NEUTRAL, TIN },
    { "黑纳迦雕像", PM_BLACK_NAGA, NEUTRAL, STATUE },
    { "黑纳迦的雕像", PM_BLACK_NAGA, NEUTRAL, STATUE },
    { "黑纳迦小雕像", PM_BLACK_NAGA, NEUTRAL, FIGURINE },
    { "黑纳迦的小雕像", PM_BLACK_NAGA, NEUTRAL, FIGURINE },
    { "黑纳迦罐头", PM_BLACK_NAGA, NEUTRAL, TIN },
    { "黑纳迦的罐头", PM_BLACK_NAGA, NEUTRAL, TIN },
    { "黑纳迦肉罐头", PM_BLACK_NAGA, NEUTRAL, TIN },
    { "金纳迦雕像", PM_GOLDEN_NAGA, NEUTRAL, STATUE },
    { "金纳迦的雕像", PM_GOLDEN_NAGA, NEUTRAL, STATUE },
    { "金纳迦小雕像", PM_GOLDEN_NAGA, NEUTRAL, FIGURINE },
    { "金纳迦的小雕像", PM_GOLDEN_NAGA, NEUTRAL, FIGURINE },
    { "金纳迦罐头", PM_GOLDEN_NAGA, NEUTRAL, TIN },
    { "金纳迦的罐头", PM_GOLDEN_NAGA, NEUTRAL, TIN },
    { "金纳迦肉罐头", PM_GOLDEN_NAGA, NEUTRAL, TIN },
    { "纳迦守卫雕像", PM_GUARDIAN_NAGA, NEUTRAL, STATUE },
    { "纳迦守卫的雕像", PM_GUARDIAN_NAGA, NEUTRAL, STATUE },
    { "纳迦守卫小雕像", PM_GUARDIAN_NAGA, NEUTRAL, FIGURINE },
    { "纳迦守卫的小雕像", PM_GUARDIAN_NAGA, NEUTRAL, FIGURINE },
    { "纳迦守卫罐头", PM_GUARDIAN_NAGA, NEUTRAL, TIN },
    { "纳迦守卫的罐头", PM_GUARDIAN_NAGA, NEUTRAL, TIN },
    { "纳迦守卫肉罐头", PM_GUARDIAN_NAGA, NEUTRAL, TIN },
    { "食人魔雕像", PM_OGRE, NEUTRAL, STATUE },
    { "食人魔的雕像", PM_OGRE, NEUTRAL, STATUE },
    { "食人魔小雕像", PM_OGRE, NEUTRAL, FIGURINE },
    { "食人魔的小雕像", PM_OGRE, NEUTRAL, FIGURINE },
    { "食人魔罐头", PM_OGRE, NEUTRAL, TIN },
    { "食人魔的罐头", PM_OGRE, NEUTRAL, TIN },
    { "食人魔肉罐头", PM_OGRE, NEUTRAL, TIN },
    { "食人魔领主雕像", PM_OGRE_LEADER, MALE, STATUE },
    { "食人魔领主的雕像", PM_OGRE_LEADER, MALE, STATUE },
    { "食人魔领主小雕像", PM_OGRE_LEADER, MALE, FIGURINE },
    { "食人魔领主的小雕像", PM_OGRE_LEADER, MALE, FIGURINE },
    { "食人魔领主罐头", PM_OGRE_LEADER, MALE, TIN },
    { "食人魔领主的罐头", PM_OGRE_LEADER, MALE, TIN },
    { "食人魔领主肉罐头", PM_OGRE_LEADER, MALE, TIN },
    { "食人魔女领主雕像", PM_OGRE_LEADER, FEMALE, STATUE },
    { "食人魔女领主的雕像", PM_OGRE_LEADER, FEMALE, STATUE },
    { "食人魔女领主小雕像", PM_OGRE_LEADER, FEMALE, FIGURINE },
    { "食人魔女领主的小雕像", PM_OGRE_LEADER, FEMALE, FIGURINE },
    { "食人魔女领主罐头", PM_OGRE_LEADER, FEMALE, TIN },
    { "食人魔女领主的罐头", PM_OGRE_LEADER, FEMALE, TIN },
    { "食人魔女领主肉罐头", PM_OGRE_LEADER, FEMALE, TIN },
    { "食人魔领袖雕像", PM_OGRE_LEADER, NEUTRAL, STATUE },
    { "食人魔领袖的雕像", PM_OGRE_LEADER, NEUTRAL, STATUE },
    { "食人魔领袖小雕像", PM_OGRE_LEADER, NEUTRAL, FIGURINE },
    { "食人魔领袖的小雕像", PM_OGRE_LEADER, NEUTRAL, FIGURINE },
    { "食人魔领袖罐头", PM_OGRE_LEADER, NEUTRAL, TIN },
    { "食人魔领袖的罐头", PM_OGRE_LEADER, NEUTRAL, TIN },
    { "食人魔领袖肉罐头", PM_OGRE_LEADER, NEUTRAL, TIN },
    { "食人魔王雕像", PM_OGRE_TYRANT, MALE, STATUE },
    { "食人魔王的雕像", PM_OGRE_TYRANT, MALE, STATUE },
    { "食人魔王小雕像", PM_OGRE_TYRANT, MALE, FIGURINE },
    { "食人魔王的小雕像", PM_OGRE_TYRANT, MALE, FIGURINE },
    { "食人魔王罐头", PM_OGRE_TYRANT, MALE, TIN },
    { "食人魔王的罐头", PM_OGRE_TYRANT, MALE, TIN },
    { "食人魔王肉罐头", PM_OGRE_TYRANT, MALE, TIN },
    { "食人魔女王雕像", PM_OGRE_TYRANT, FEMALE, STATUE },
    { "食人魔女王的雕像", PM_OGRE_TYRANT, FEMALE, STATUE },
    { "食人魔女王小雕像", PM_OGRE_TYRANT, FEMALE, FIGURINE },
    { "食人魔女王的小雕像", PM_OGRE_TYRANT, FEMALE, FIGURINE },
    { "食人魔女王罐头", PM_OGRE_TYRANT, FEMALE, TIN },
    { "食人魔女王的罐头", PM_OGRE_TYRANT, FEMALE, TIN },
    { "食人魔女王肉罐头", PM_OGRE_TYRANT, FEMALE, TIN },
    { "食人魔暴君雕像", PM_OGRE_TYRANT, NEUTRAL, STATUE },
    { "食人魔暴君的雕像", PM_OGRE_TYRANT, NEUTRAL, STATUE },
    { "食人魔暴君小雕像", PM_OGRE_TYRANT, NEUTRAL, FIGURINE },
    { "食人魔暴君的小雕像", PM_OGRE_TYRANT, NEUTRAL, FIGURINE },
    { "食人魔暴君罐头", PM_OGRE_TYRANT, NEUTRAL, TIN },
    { "食人魔暴君的罐头", PM_OGRE_TYRANT, NEUTRAL, TIN },
    { "食人魔暴君肉罐头", PM_OGRE_TYRANT, NEUTRAL, TIN },
    { "食人魔统治者雕像", PM_OGRE_TYRANT, NEUTRAL, STATUE },
    { "食人魔统治者的雕像", PM_OGRE_TYRANT, NEUTRAL, STATUE },
    { "食人魔统治者小雕像", PM_OGRE_TYRANT, NEUTRAL, FIGURINE },
    { "食人魔统治者的小雕像", PM_OGRE_TYRANT, NEUTRAL, FIGURINE },
    { "食人魔统治者罐头", PM_OGRE_TYRANT, NEUTRAL, TIN },
    { "食人魔统治者的罐头", PM_OGRE_TYRANT, NEUTRAL, TIN },
    { "食人魔统治者肉罐头", PM_OGRE_TYRANT, NEUTRAL, TIN },
    { "灰色软泥雕像", PM_GRAY_OOZE, NEUTRAL, STATUE },
    { "灰色软泥的雕像", PM_GRAY_OOZE, NEUTRAL, STATUE },
    { "灰色软泥小雕像", PM_GRAY_OOZE, NEUTRAL, FIGURINE },
    { "灰色软泥的小雕像", PM_GRAY_OOZE, NEUTRAL, FIGURINE },
    { "灰色软泥罐头", PM_GRAY_OOZE, NEUTRAL, TIN },
    { "灰色软泥的罐头", PM_GRAY_OOZE, NEUTRAL, TIN },
    { "灰色软泥肉罐头", PM_GRAY_OOZE, NEUTRAL, TIN },
    { "灰泥怪雕像", PM_GRAY_OOZE, NEUTRAL, STATUE },
    { "灰泥怪的雕像", PM_GRAY_OOZE, NEUTRAL, STATUE },
    { "灰泥怪小雕像", PM_GRAY_OOZE, NEUTRAL, FIGURINE },
    { "灰泥怪的小雕像", PM_GRAY_OOZE, NEUTRAL, FIGURINE },
    { "灰泥怪罐头", PM_GRAY_OOZE, NEUTRAL, TIN },
    { "灰泥怪的罐头", PM_GRAY_OOZE, NEUTRAL, TIN },
    { "灰泥怪肉罐头", PM_GRAY_OOZE, NEUTRAL, TIN },
    { "棕色布丁雕像", PM_BROWN_PUDDING, NEUTRAL, STATUE },
    { "棕色布丁的雕像", PM_BROWN_PUDDING, NEUTRAL, STATUE },
    { "棕色布丁小雕像", PM_BROWN_PUDDING, NEUTRAL, FIGURINE },
    { "棕色布丁的小雕像", PM_BROWN_PUDDING, NEUTRAL, FIGURINE },
    { "棕色布丁罐头", PM_BROWN_PUDDING, NEUTRAL, TIN },
    { "棕色布丁的罐头", PM_BROWN_PUDDING, NEUTRAL, TIN },
    { "棕色布丁肉罐头", PM_BROWN_PUDDING, NEUTRAL, TIN },
    { "棕布丁雕像", PM_BROWN_PUDDING, NEUTRAL, STATUE },
    { "棕布丁的雕像", PM_BROWN_PUDDING, NEUTRAL, STATUE },
    { "棕布丁小雕像", PM_BROWN_PUDDING, NEUTRAL, FIGURINE },
    { "棕布丁的小雕像", PM_BROWN_PUDDING, NEUTRAL, FIGURINE },
    { "棕布丁罐头", PM_BROWN_PUDDING, NEUTRAL, TIN },
    { "棕布丁的罐头", PM_BROWN_PUDDING, NEUTRAL, TIN },
    { "棕布丁肉罐头", PM_BROWN_PUDDING, NEUTRAL, TIN },
    { "绿色黏液雕像", PM_GREEN_SLIME, NEUTRAL, STATUE },
    { "绿色黏液的雕像", PM_GREEN_SLIME, NEUTRAL, STATUE },
    { "绿色黏液小雕像", PM_GREEN_SLIME, NEUTRAL, FIGURINE },
    { "绿色黏液的小雕像", PM_GREEN_SLIME, NEUTRAL, FIGURINE },
    { "绿色黏液罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "绿色黏液的罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "绿色黏液肉罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "绿黏液雕像", PM_GREEN_SLIME, NEUTRAL, STATUE },
    { "绿黏液的雕像", PM_GREEN_SLIME, NEUTRAL, STATUE },
    { "绿黏液小雕像", PM_GREEN_SLIME, NEUTRAL, FIGURINE },
    { "绿黏液的小雕像", PM_GREEN_SLIME, NEUTRAL, FIGURINE },
    { "绿黏液罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "绿黏液的罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "绿黏液肉罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "绿色史莱姆雕像", PM_GREEN_SLIME, NEUTRAL, STATUE },
    { "绿色史莱姆的雕像", PM_GREEN_SLIME, NEUTRAL, STATUE },
    { "绿色史莱姆小雕像", PM_GREEN_SLIME, NEUTRAL, FIGURINE },
    { "绿色史莱姆的小雕像", PM_GREEN_SLIME, NEUTRAL, FIGURINE },
    { "绿色史莱姆罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "绿色史莱姆的罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "绿色史莱姆肉罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "绿史莱姆雕像", PM_GREEN_SLIME, NEUTRAL, STATUE },
    { "绿史莱姆的雕像", PM_GREEN_SLIME, NEUTRAL, STATUE },
    { "绿史莱姆小雕像", PM_GREEN_SLIME, NEUTRAL, FIGURINE },
    { "绿史莱姆的小雕像", PM_GREEN_SLIME, NEUTRAL, FIGURINE },
    { "绿史莱姆罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "绿史莱姆的罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "绿史莱姆肉罐头", PM_GREEN_SLIME, NEUTRAL, TIN },
    { "黑色布丁雕像", PM_BLACK_PUDDING, NEUTRAL, STATUE },
    { "黑色布丁的雕像", PM_BLACK_PUDDING, NEUTRAL, STATUE },
    { "黑色布丁小雕像", PM_BLACK_PUDDING, NEUTRAL, FIGURINE },
    { "黑色布丁的小雕像", PM_BLACK_PUDDING, NEUTRAL, FIGURINE },
    { "黑色布丁罐头", PM_BLACK_PUDDING, NEUTRAL, TIN },
    { "黑色布丁的罐头", PM_BLACK_PUDDING, NEUTRAL, TIN },
    { "黑色布丁肉罐头", PM_BLACK_PUDDING, NEUTRAL, TIN },
    { "黑布丁雕像", PM_BLACK_PUDDING, NEUTRAL, STATUE },
    { "黑布丁的雕像", PM_BLACK_PUDDING, NEUTRAL, STATUE },
    { "黑布丁小雕像", PM_BLACK_PUDDING, NEUTRAL, FIGURINE },
    { "黑布丁的小雕像", PM_BLACK_PUDDING, NEUTRAL, FIGURINE },
    { "黑布丁罐头", PM_BLACK_PUDDING, NEUTRAL, TIN },
    { "黑布丁的罐头", PM_BLACK_PUDDING, NEUTRAL, TIN },
    { "黑布丁肉罐头", PM_BLACK_PUDDING, NEUTRAL, TIN },
    { "量子力学雕像", PM_QUANTUM_MECHANIC, NEUTRAL, STATUE },
    { "量子力学的雕像", PM_QUANTUM_MECHANIC, NEUTRAL, STATUE },
    { "量子力学小雕像", PM_QUANTUM_MECHANIC, NEUTRAL, FIGURINE },
    { "量子力学的小雕像", PM_QUANTUM_MECHANIC, NEUTRAL, FIGURINE },
    { "量子力学罐头", PM_QUANTUM_MECHANIC, NEUTRAL, TIN },
    { "量子力学的罐头", PM_QUANTUM_MECHANIC, NEUTRAL, TIN },
    { "量子力学肉罐头", PM_QUANTUM_MECHANIC, NEUTRAL, TIN },
    { "量子技工雕像", PM_QUANTUM_MECHANIC, NEUTRAL, STATUE },
    { "量子技工的雕像", PM_QUANTUM_MECHANIC, NEUTRAL, STATUE },
    { "量子技工小雕像", PM_QUANTUM_MECHANIC, NEUTRAL, FIGURINE },
    { "量子技工的小雕像", PM_QUANTUM_MECHANIC, NEUTRAL, FIGURINE },
    { "量子技工罐头", PM_QUANTUM_MECHANIC, NEUTRAL, TIN },
    { "量子技工的罐头", PM_QUANTUM_MECHANIC, NEUTRAL, TIN },
    { "量子技工肉罐头", PM_QUANTUM_MECHANIC, NEUTRAL, TIN },
    { "量子工程师雕像", PM_QUANTUM_MECHANIC, NEUTRAL, STATUE },
    { "量子工程师的雕像", PM_QUANTUM_MECHANIC, NEUTRAL, STATUE },
    { "量子工程师小雕像", PM_QUANTUM_MECHANIC, NEUTRAL, FIGURINE },
    { "量子工程师的小雕像", PM_QUANTUM_MECHANIC, NEUTRAL, FIGURINE },
    { "量子工程师罐头", PM_QUANTUM_MECHANIC, NEUTRAL, TIN },
    { "量子工程师的罐头", PM_QUANTUM_MECHANIC, NEUTRAL, TIN },
    { "量子工程师肉罐头", PM_QUANTUM_MECHANIC, NEUTRAL, TIN },
    { "基因工程师雕像", PM_GENETIC_ENGINEER, NEUTRAL, STATUE },
    { "基因工程师的雕像", PM_GENETIC_ENGINEER, NEUTRAL, STATUE },
    { "基因工程师小雕像", PM_GENETIC_ENGINEER, NEUTRAL, FIGURINE },
    { "基因工程师的小雕像", PM_GENETIC_ENGINEER, NEUTRAL, FIGURINE },
    { "基因工程师罐头", PM_GENETIC_ENGINEER, NEUTRAL, TIN },
    { "基因工程师的罐头", PM_GENETIC_ENGINEER, NEUTRAL, TIN },
    { "基因工程师肉罐头", PM_GENETIC_ENGINEER, NEUTRAL, TIN },
    { "锈怪雕像", PM_RUST_MONSTER, NEUTRAL, STATUE },
    { "锈怪的雕像", PM_RUST_MONSTER, NEUTRAL, STATUE },
    { "锈怪小雕像", PM_RUST_MONSTER, NEUTRAL, FIGURINE },
    { "锈怪的小雕像", PM_RUST_MONSTER, NEUTRAL, FIGURINE },
    { "锈怪罐头", PM_RUST_MONSTER, NEUTRAL, TIN },
    { "锈怪的罐头", PM_RUST_MONSTER, NEUTRAL, TIN },
    { "锈怪肉罐头", PM_RUST_MONSTER, NEUTRAL, TIN },
    { "锈蚀怪雕像", PM_RUST_MONSTER, NEUTRAL, STATUE },
    { "锈蚀怪的雕像", PM_RUST_MONSTER, NEUTRAL, STATUE },
    { "锈蚀怪小雕像", PM_RUST_MONSTER, NEUTRAL, FIGURINE },
    { "锈蚀怪的小雕像", PM_RUST_MONSTER, NEUTRAL, FIGURINE },
    { "锈蚀怪罐头", PM_RUST_MONSTER, NEUTRAL, TIN },
    { "锈蚀怪的罐头", PM_RUST_MONSTER, NEUTRAL, TIN },
    { "锈蚀怪肉罐头", PM_RUST_MONSTER, NEUTRAL, TIN },
    { "解魔怪雕像", PM_DISENCHANTER, NEUTRAL, STATUE },
    { "解魔怪的雕像", PM_DISENCHANTER, NEUTRAL, STATUE },
    { "解魔怪小雕像", PM_DISENCHANTER, NEUTRAL, FIGURINE },
    { "解魔怪的小雕像", PM_DISENCHANTER, NEUTRAL, FIGURINE },
    { "解魔怪罐头", PM_DISENCHANTER, NEUTRAL, TIN },
    { "解魔怪的罐头", PM_DISENCHANTER, NEUTRAL, TIN },
    { "解魔怪肉罐头", PM_DISENCHANTER, NEUTRAL, TIN },
    { "祛魔怪雕像", PM_DISENCHANTER, NEUTRAL, STATUE },
    { "祛魔怪的雕像", PM_DISENCHANTER, NEUTRAL, STATUE },
    { "祛魔怪小雕像", PM_DISENCHANTER, NEUTRAL, FIGURINE },
    { "祛魔怪的小雕像", PM_DISENCHANTER, NEUTRAL, FIGURINE },
    { "祛魔怪罐头", PM_DISENCHANTER, NEUTRAL, TIN },
    { "祛魔怪的罐头", PM_DISENCHANTER, NEUTRAL, TIN },
    { "祛魔怪肉罐头", PM_DISENCHANTER, NEUTRAL, TIN },
    { "束带蛇雕像", PM_GARTER_SNAKE, NEUTRAL, STATUE },
    { "束带蛇的雕像", PM_GARTER_SNAKE, NEUTRAL, STATUE },
    { "束带蛇小雕像", PM_GARTER_SNAKE, NEUTRAL, FIGURINE },
    { "束带蛇的小雕像", PM_GARTER_SNAKE, NEUTRAL, FIGURINE },
    { "束带蛇罐头", PM_GARTER_SNAKE, NEUTRAL, TIN },
    { "束带蛇的罐头", PM_GARTER_SNAKE, NEUTRAL, TIN },
    { "束带蛇肉罐头", PM_GARTER_SNAKE, NEUTRAL, TIN },
    { "蛇雕像", PM_SNAKE, NEUTRAL, STATUE },
    { "蛇的雕像", PM_SNAKE, NEUTRAL, STATUE },
    { "蛇小雕像", PM_SNAKE, NEUTRAL, FIGURINE },
    { "蛇的小雕像", PM_SNAKE, NEUTRAL, FIGURINE },
    { "蛇罐头", PM_SNAKE, NEUTRAL, TIN },
    { "蛇的罐头", PM_SNAKE, NEUTRAL, TIN },
    { "蛇肉罐头", PM_SNAKE, NEUTRAL, TIN },
    { "水蝮蛇雕像", PM_WATER_MOCCASIN, NEUTRAL, STATUE },
    { "水蝮蛇的雕像", PM_WATER_MOCCASIN, NEUTRAL, STATUE },
    { "水蝮蛇小雕像", PM_WATER_MOCCASIN, NEUTRAL, FIGURINE },
    { "水蝮蛇的小雕像", PM_WATER_MOCCASIN, NEUTRAL, FIGURINE },
    { "水蝮蛇罐头", PM_WATER_MOCCASIN, NEUTRAL, TIN },
    { "水蝮蛇的罐头", PM_WATER_MOCCASIN, NEUTRAL, TIN },
    { "水蝮蛇肉罐头", PM_WATER_MOCCASIN, NEUTRAL, TIN },
    { "巨蟒雕像", PM_PYTHON, NEUTRAL, STATUE },
    { "巨蟒的雕像", PM_PYTHON, NEUTRAL, STATUE },
    { "巨蟒小雕像", PM_PYTHON, NEUTRAL, FIGURINE },
    { "巨蟒的小雕像", PM_PYTHON, NEUTRAL, FIGURINE },
    { "巨蟒罐头", PM_PYTHON, NEUTRAL, TIN },
    { "巨蟒的罐头", PM_PYTHON, NEUTRAL, TIN },
    { "巨蟒肉罐头", PM_PYTHON, NEUTRAL, TIN },
    { "响尾蛇雕像", PM_PIT_VIPER, NEUTRAL, STATUE },
    { "响尾蛇的雕像", PM_PIT_VIPER, NEUTRAL, STATUE },
    { "响尾蛇小雕像", PM_PIT_VIPER, NEUTRAL, FIGURINE },
    { "响尾蛇的小雕像", PM_PIT_VIPER, NEUTRAL, FIGURINE },
    { "响尾蛇罐头", PM_PIT_VIPER, NEUTRAL, TIN },
    { "响尾蛇的罐头", PM_PIT_VIPER, NEUTRAL, TIN },
    { "响尾蛇肉罐头", PM_PIT_VIPER, NEUTRAL, TIN },
    { "眼镜蛇雕像", PM_COBRA, NEUTRAL, STATUE },
    { "眼镜蛇的雕像", PM_COBRA, NEUTRAL, STATUE },
    { "眼镜蛇小雕像", PM_COBRA, NEUTRAL, FIGURINE },
    { "眼镜蛇的小雕像", PM_COBRA, NEUTRAL, FIGURINE },
    { "眼镜蛇罐头", PM_COBRA, NEUTRAL, TIN },
    { "眼镜蛇的罐头", PM_COBRA, NEUTRAL, TIN },
    { "眼镜蛇肉罐头", PM_COBRA, NEUTRAL, TIN },
    { "巨魔雕像", PM_TROLL, NEUTRAL, STATUE },
    { "巨魔的雕像", PM_TROLL, NEUTRAL, STATUE },
    { "巨魔小雕像", PM_TROLL, NEUTRAL, FIGURINE },
    { "巨魔的小雕像", PM_TROLL, NEUTRAL, FIGURINE },
    { "巨魔罐头", PM_TROLL, NEUTRAL, TIN },
    { "巨魔的罐头", PM_TROLL, NEUTRAL, TIN },
    { "巨魔肉罐头", PM_TROLL, NEUTRAL, TIN },
    { "冰巨魔雕像", PM_ICE_TROLL, NEUTRAL, STATUE },
    { "冰巨魔的雕像", PM_ICE_TROLL, NEUTRAL, STATUE },
    { "冰巨魔小雕像", PM_ICE_TROLL, NEUTRAL, FIGURINE },
    { "冰巨魔的小雕像", PM_ICE_TROLL, NEUTRAL, FIGURINE },
    { "冰巨魔罐头", PM_ICE_TROLL, NEUTRAL, TIN },
    { "冰巨魔的罐头", PM_ICE_TROLL, NEUTRAL, TIN },
    { "冰巨魔肉罐头", PM_ICE_TROLL, NEUTRAL, TIN },
    { "寒冰巨魔雕像", PM_ICE_TROLL, NEUTRAL, STATUE },
    { "寒冰巨魔的雕像", PM_ICE_TROLL, NEUTRAL, STATUE },
    { "寒冰巨魔小雕像", PM_ICE_TROLL, NEUTRAL, FIGURINE },
    { "寒冰巨魔的小雕像", PM_ICE_TROLL, NEUTRAL, FIGURINE },
    { "寒冰巨魔罐头", PM_ICE_TROLL, NEUTRAL, TIN },
    { "寒冰巨魔的罐头", PM_ICE_TROLL, NEUTRAL, TIN },
    { "寒冰巨魔肉罐头", PM_ICE_TROLL, NEUTRAL, TIN },
    { "岩石巨魔雕像", PM_ROCK_TROLL, NEUTRAL, STATUE },
    { "岩石巨魔的雕像", PM_ROCK_TROLL, NEUTRAL, STATUE },
    { "岩石巨魔小雕像", PM_ROCK_TROLL, NEUTRAL, FIGURINE },
    { "岩石巨魔的小雕像", PM_ROCK_TROLL, NEUTRAL, FIGURINE },
    { "岩石巨魔罐头", PM_ROCK_TROLL, NEUTRAL, TIN },
    { "岩石巨魔的罐头", PM_ROCK_TROLL, NEUTRAL, TIN },
    { "岩石巨魔肉罐头", PM_ROCK_TROLL, NEUTRAL, TIN },
    { "石巨魔雕像", PM_ROCK_TROLL, NEUTRAL, STATUE },
    { "石巨魔的雕像", PM_ROCK_TROLL, NEUTRAL, STATUE },
    { "石巨魔小雕像", PM_ROCK_TROLL, NEUTRAL, FIGURINE },
    { "石巨魔的小雕像", PM_ROCK_TROLL, NEUTRAL, FIGURINE },
    { "石巨魔罐头", PM_ROCK_TROLL, NEUTRAL, TIN },
    { "石巨魔的罐头", PM_ROCK_TROLL, NEUTRAL, TIN },
    { "石巨魔肉罐头", PM_ROCK_TROLL, NEUTRAL, TIN },
    { "水巨魔雕像", PM_WATER_TROLL, NEUTRAL, STATUE },
    { "水巨魔的雕像", PM_WATER_TROLL, NEUTRAL, STATUE },
    { "水巨魔小雕像", PM_WATER_TROLL, NEUTRAL, FIGURINE },
    { "水巨魔的小雕像", PM_WATER_TROLL, NEUTRAL, FIGURINE },
    { "水巨魔罐头", PM_WATER_TROLL, NEUTRAL, TIN },
    { "水巨魔的罐头", PM_WATER_TROLL, NEUTRAL, TIN },
    { "水巨魔肉罐头", PM_WATER_TROLL, NEUTRAL, TIN },
    { "欧罗海雕像", PM_OLOG_HAI, NEUTRAL, STATUE },
    { "欧罗海的雕像", PM_OLOG_HAI, NEUTRAL, STATUE },
    { "欧罗海小雕像", PM_OLOG_HAI, NEUTRAL, FIGURINE },
    { "欧罗海的小雕像", PM_OLOG_HAI, NEUTRAL, FIGURINE },
    { "欧罗海罐头", PM_OLOG_HAI, NEUTRAL, TIN },
    { "欧罗海的罐头", PM_OLOG_HAI, NEUTRAL, TIN },
    { "欧罗海肉罐头", PM_OLOG_HAI, NEUTRAL, TIN },
    { "奥洛格雕像", PM_OLOG_HAI, NEUTRAL, STATUE },
    { "奥洛格的雕像", PM_OLOG_HAI, NEUTRAL, STATUE },
    { "奥洛格小雕像", PM_OLOG_HAI, NEUTRAL, FIGURINE },
    { "奥洛格的小雕像", PM_OLOG_HAI, NEUTRAL, FIGURINE },
    { "奥洛格罐头", PM_OLOG_HAI, NEUTRAL, TIN },
    { "奥洛格的罐头", PM_OLOG_HAI, NEUTRAL, TIN },
    { "奥洛格肉罐头", PM_OLOG_HAI, NEUTRAL, TIN },
    { "土巨怪雕像", PM_UMBER_HULK, NEUTRAL, STATUE },
    { "土巨怪的雕像", PM_UMBER_HULK, NEUTRAL, STATUE },
    { "土巨怪小雕像", PM_UMBER_HULK, NEUTRAL, FIGURINE },
    { "土巨怪的小雕像", PM_UMBER_HULK, NEUTRAL, FIGURINE },
    { "土巨怪罐头", PM_UMBER_HULK, NEUTRAL, TIN },
    { "土巨怪的罐头", PM_UMBER_HULK, NEUTRAL, TIN },
    { "土巨怪肉罐头", PM_UMBER_HULK, NEUTRAL, TIN },
    { "吸血鬼雕像", PM_VAMPIRE, NEUTRAL, STATUE },
    { "吸血鬼的雕像", PM_VAMPIRE, NEUTRAL, STATUE },
    { "吸血鬼小雕像", PM_VAMPIRE, NEUTRAL, FIGURINE },
    { "吸血鬼的小雕像", PM_VAMPIRE, NEUTRAL, FIGURINE },
    { "吸血鬼罐头", PM_VAMPIRE, NEUTRAL, TIN },
    { "吸血鬼的罐头", PM_VAMPIRE, NEUTRAL, TIN },
    { "吸血鬼肉罐头", PM_VAMPIRE, NEUTRAL, TIN },
    { "vampire lady雕像", PM_VAMPIRE_LEADER, NEUTRAL, STATUE },
    { "vampire lady的雕像", PM_VAMPIRE_LEADER, NEUTRAL, STATUE },
    { "vampire lady小雕像", PM_VAMPIRE_LEADER, NEUTRAL, FIGURINE },
    { "vampire lady的小雕像", PM_VAMPIRE_LEADER, NEUTRAL, FIGURINE },
    { "vampire lady罐头", PM_VAMPIRE_LEADER, NEUTRAL, TIN },
    { "vampire lady的罐头", PM_VAMPIRE_LEADER, NEUTRAL, TIN },
    { "vampire lady肉罐头", PM_VAMPIRE_LEADER, NEUTRAL, TIN },
    吸血鬼领主雕像, STATUE领主、
    吸血鬼领主的雕像, STATUE领主、
    吸血鬼领主小雕像, FIGURINE领主、
    吸血鬼领主的小雕像, FIGURINE领主、
    吸血鬼领主罐头, TIN领主、
    吸血鬼领主的罐头, TIN领主、
    吸血鬼领主肉罐头, TIN领主、
    { "穿刺者弗拉德雕像", PM_VLAD_THE_IMPALER, NEUTRAL, STATUE },
    { "穿刺者弗拉德的雕像", PM_VLAD_THE_IMPALER, NEUTRAL, STATUE },
    { "穿刺者弗拉德小雕像", PM_VLAD_THE_IMPALER, NEUTRAL, FIGURINE },
    { "穿刺者弗拉德的小雕像", PM_VLAD_THE_IMPALER, NEUTRAL, FIGURINE },
    { "穿刺者弗拉德罐头", PM_VLAD_THE_IMPALER, NEUTRAL, TIN },
    { "穿刺者弗拉德的罐头", PM_VLAD_THE_IMPALER, NEUTRAL, TIN },
    { "穿刺者弗拉德肉罐头", PM_VLAD_THE_IMPALER, NEUTRAL, TIN },
    { "弗拉德雕像", PM_VLAD_THE_IMPALER, NEUTRAL, STATUE },
    { "弗拉德的雕像", PM_VLAD_THE_IMPALER, NEUTRAL, STATUE },
    { "弗拉德小雕像", PM_VLAD_THE_IMPALER, NEUTRAL, FIGURINE },
    { "弗拉德的小雕像", PM_VLAD_THE_IMPALER, NEUTRAL, FIGURINE },
    { "弗拉德罐头", PM_VLAD_THE_IMPALER, NEUTRAL, TIN },
    { "弗拉德的罐头", PM_VLAD_THE_IMPALER, NEUTRAL, TIN },
    { "弗拉德肉罐头", PM_VLAD_THE_IMPALER, NEUTRAL, TIN },
    { "古墓尸妖雕像", PM_BARROW_WIGHT, NEUTRAL, STATUE },
    { "古墓尸妖的雕像", PM_BARROW_WIGHT, NEUTRAL, STATUE },
    { "古墓尸妖小雕像", PM_BARROW_WIGHT, NEUTRAL, FIGURINE },
    { "古墓尸妖的小雕像", PM_BARROW_WIGHT, NEUTRAL, FIGURINE },
    { "古墓尸妖罐头", PM_BARROW_WIGHT, NEUTRAL, TIN },
    { "古墓尸妖的罐头", PM_BARROW_WIGHT, NEUTRAL, TIN },
    { "古墓尸妖肉罐头", PM_BARROW_WIGHT, NEUTRAL, TIN },
    { "古冢尸妖雕像", PM_BARROW_WIGHT, NEUTRAL, STATUE },
    { "古冢尸妖的雕像", PM_BARROW_WIGHT, NEUTRAL, STATUE },
    { "古冢尸妖小雕像", PM_BARROW_WIGHT, NEUTRAL, FIGURINE },
    { "古冢尸妖的小雕像", PM_BARROW_WIGHT, NEUTRAL, FIGURINE },
    { "古冢尸妖罐头", PM_BARROW_WIGHT, NEUTRAL, TIN },
    { "古冢尸妖的罐头", PM_BARROW_WIGHT, NEUTRAL, TIN },
    { "古冢尸妖肉罐头", PM_BARROW_WIGHT, NEUTRAL, TIN },
    { "尸妖雕像", PM_BARROW_WIGHT, NEUTRAL, STATUE },
    { "尸妖的雕像", PM_BARROW_WIGHT, NEUTRAL, STATUE },
    { "尸妖小雕像", PM_BARROW_WIGHT, NEUTRAL, FIGURINE },
    { "尸妖的小雕像", PM_BARROW_WIGHT, NEUTRAL, FIGURINE },
    { "尸妖罐头", PM_BARROW_WIGHT, NEUTRAL, TIN },
    { "尸妖的罐头", PM_BARROW_WIGHT, NEUTRAL, TIN },
    { "尸妖肉罐头", PM_BARROW_WIGHT, NEUTRAL, TIN },
    { "幽灵雕像", PM_WRAITH, NEUTRAL, STATUE },
    { "幽灵的雕像", PM_WRAITH, NEUTRAL, STATUE },
    { "幽灵小雕像", PM_WRAITH, NEUTRAL, FIGURINE },
    { "幽灵的小雕像", PM_WRAITH, NEUTRAL, FIGURINE },
    { "幽灵罐头", PM_WRAITH, NEUTRAL, TIN },
    { "幽灵的罐头", PM_WRAITH, NEUTRAL, TIN },
    { "幽灵肉罐头", PM_WRAITH, NEUTRAL, TIN },
    { "戒灵雕像", PM_NAZGUL, NEUTRAL, STATUE },
    { "戒灵的雕像", PM_NAZGUL, NEUTRAL, STATUE },
    { "戒灵小雕像", PM_NAZGUL, NEUTRAL, FIGURINE },
    { "戒灵的小雕像", PM_NAZGUL, NEUTRAL, FIGURINE },
    { "戒灵罐头", PM_NAZGUL, NEUTRAL, TIN },
    { "戒灵的罐头", PM_NAZGUL, NEUTRAL, TIN },
    { "戒灵肉罐头", PM_NAZGUL, NEUTRAL, TIN },
    { "索尔石怪雕像", PM_XORN, NEUTRAL, STATUE },
    { "索尔石怪的雕像", PM_XORN, NEUTRAL, STATUE },
    { "索尔石怪小雕像", PM_XORN, NEUTRAL, FIGURINE },
    { "索尔石怪的小雕像", PM_XORN, NEUTRAL, FIGURINE },
    { "索尔石怪罐头", PM_XORN, NEUTRAL, TIN },
    { "索尔石怪的罐头", PM_XORN, NEUTRAL, TIN },
    { "索尔石怪肉罐头", PM_XORN, NEUTRAL, TIN },
    { "猴子雕像", PM_MONKEY, NEUTRAL, STATUE },
    { "猴子的雕像", PM_MONKEY, NEUTRAL, STATUE },
    { "猴子小雕像", PM_MONKEY, NEUTRAL, FIGURINE },
    { "猴子的小雕像", PM_MONKEY, NEUTRAL, FIGURINE },
    { "猴子罐头", PM_MONKEY, NEUTRAL, TIN },
    { "猴子的罐头", PM_MONKEY, NEUTRAL, TIN },
    { "猴子肉罐头", PM_MONKEY, NEUTRAL, TIN },
    { "猴雕像", PM_MONKEY, NEUTRAL, STATUE },
    { "猴的雕像", PM_MONKEY, NEUTRAL, STATUE },
    { "猴小雕像", PM_MONKEY, NEUTRAL, FIGURINE },
    { "猴的小雕像", PM_MONKEY, NEUTRAL, FIGURINE },
    { "猴罐头", PM_MONKEY, NEUTRAL, TIN },
    { "猴的罐头", PM_MONKEY, NEUTRAL, TIN },
    { "猴肉罐头", PM_MONKEY, NEUTRAL, TIN },
    { "猿雕像", PM_APE, NEUTRAL, STATUE },
    { "猿的雕像", PM_APE, NEUTRAL, STATUE },
    { "猿小雕像", PM_APE, NEUTRAL, FIGURINE },
    { "猿的小雕像", PM_APE, NEUTRAL, FIGURINE },
    { "猿罐头", PM_APE, NEUTRAL, TIN },
    { "猿的罐头", PM_APE, NEUTRAL, TIN },
    { "猿肉罐头", PM_APE, NEUTRAL, TIN },
    { "枭熊雕像", PM_OWLBEAR, NEUTRAL, STATUE },
    { "枭熊的雕像", PM_OWLBEAR, NEUTRAL, STATUE },
    { "枭熊小雕像", PM_OWLBEAR, NEUTRAL, FIGURINE },
    { "枭熊的小雕像", PM_OWLBEAR, NEUTRAL, FIGURINE },
    { "枭熊罐头", PM_OWLBEAR, NEUTRAL, TIN },
    { "枭熊的罐头", PM_OWLBEAR, NEUTRAL, TIN },
    { "枭熊肉罐头", PM_OWLBEAR, NEUTRAL, TIN },
    { "雪人雕像", PM_YETI, NEUTRAL, STATUE },
    { "雪人的雕像", PM_YETI, NEUTRAL, STATUE },
    { "雪人小雕像", PM_YETI, NEUTRAL, FIGURINE },
    { "雪人的小雕像", PM_YETI, NEUTRAL, FIGURINE },
    { "雪人罐头", PM_YETI, NEUTRAL, TIN },
    { "雪人的罐头", PM_YETI, NEUTRAL, TIN },
    { "雪人肉罐头", PM_YETI, NEUTRAL, TIN },
    { "食肉猿雕像", PM_CARNIVOROUS_APE, NEUTRAL, STATUE },
    { "食肉猿的雕像", PM_CARNIVOROUS_APE, NEUTRAL, STATUE },
    { "食肉猿小雕像", PM_CARNIVOROUS_APE, NEUTRAL, FIGURINE },
    { "食肉猿的小雕像", PM_CARNIVOROUS_APE, NEUTRAL, FIGURINE },
    { "食肉猿罐头", PM_CARNIVOROUS_APE, NEUTRAL, TIN },
    { "食肉猿的罐头", PM_CARNIVOROUS_APE, NEUTRAL, TIN },
    { "食肉猿肉罐头", PM_CARNIVOROUS_APE, NEUTRAL, TIN },
    { "北美野人雕像", PM_SASQUATCH, NEUTRAL, STATUE },
    { "北美野人的雕像", PM_SASQUATCH, NEUTRAL, STATUE },
    { "北美野人小雕像", PM_SASQUATCH, NEUTRAL, FIGURINE },
    { "北美野人的小雕像", PM_SASQUATCH, NEUTRAL, FIGURINE },
    { "北美野人罐头", PM_SASQUATCH, NEUTRAL, TIN },
    { "北美野人的罐头", PM_SASQUATCH, NEUTRAL, TIN },
    { "北美野人肉罐头", PM_SASQUATCH, NEUTRAL, TIN },
    { "狗头人僵尸雕像", PM_KOBOLD_ZOMBIE, NEUTRAL, STATUE },
    { "狗头人僵尸的雕像", PM_KOBOLD_ZOMBIE, NEUTRAL, STATUE },
    { "狗头人僵尸小雕像", PM_KOBOLD_ZOMBIE, NEUTRAL, FIGURINE },
    { "狗头人僵尸的小雕像", PM_KOBOLD_ZOMBIE, NEUTRAL, FIGURINE },
    { "狗头人僵尸罐头", PM_KOBOLD_ZOMBIE, NEUTRAL, TIN },
    { "狗头人僵尸的罐头", PM_KOBOLD_ZOMBIE, NEUTRAL, TIN },
    { "狗头人僵尸肉罐头", PM_KOBOLD_ZOMBIE, NEUTRAL, TIN },
    { "侏儒僵尸雕像", PM_GNOME_ZOMBIE, NEUTRAL, STATUE },
    { "侏儒僵尸的雕像", PM_GNOME_ZOMBIE, NEUTRAL, STATUE },
    { "侏儒僵尸小雕像", PM_GNOME_ZOMBIE, NEUTRAL, FIGURINE },
    { "侏儒僵尸的小雕像", PM_GNOME_ZOMBIE, NEUTRAL, FIGURINE },
    { "侏儒僵尸罐头", PM_GNOME_ZOMBIE, NEUTRAL, TIN },
    { "侏儒僵尸的罐头", PM_GNOME_ZOMBIE, NEUTRAL, TIN },
    { "侏儒僵尸肉罐头", PM_GNOME_ZOMBIE, NEUTRAL, TIN },
    { "兽人僵尸雕像", PM_ORC_ZOMBIE, NEUTRAL, STATUE },
    { "兽人僵尸的雕像", PM_ORC_ZOMBIE, NEUTRAL, STATUE },
    { "兽人僵尸小雕像", PM_ORC_ZOMBIE, NEUTRAL, FIGURINE },
    { "兽人僵尸的小雕像", PM_ORC_ZOMBIE, NEUTRAL, FIGURINE },
    { "兽人僵尸罐头", PM_ORC_ZOMBIE, NEUTRAL, TIN },
    { "兽人僵尸的罐头", PM_ORC_ZOMBIE, NEUTRAL, TIN },
    { "兽人僵尸肉罐头", PM_ORC_ZOMBIE, NEUTRAL, TIN },
    { "矮人僵尸雕像", PM_DWARF_ZOMBIE, NEUTRAL, STATUE },
    { "矮人僵尸的雕像", PM_DWARF_ZOMBIE, NEUTRAL, STATUE },
    { "矮人僵尸小雕像", PM_DWARF_ZOMBIE, NEUTRAL, FIGURINE },
    { "矮人僵尸的小雕像", PM_DWARF_ZOMBIE, NEUTRAL, FIGURINE },
    { "矮人僵尸罐头", PM_DWARF_ZOMBIE, NEUTRAL, TIN },
    { "矮人僵尸的罐头", PM_DWARF_ZOMBIE, NEUTRAL, TIN },
    { "矮人僵尸肉罐头", PM_DWARF_ZOMBIE, NEUTRAL, TIN },
    { "精灵僵尸雕像", PM_ELF_ZOMBIE, NEUTRAL, STATUE },
    { "精灵僵尸的雕像", PM_ELF_ZOMBIE, NEUTRAL, STATUE },
    { "精灵僵尸小雕像", PM_ELF_ZOMBIE, NEUTRAL, FIGURINE },
    { "精灵僵尸的小雕像", PM_ELF_ZOMBIE, NEUTRAL, FIGURINE },
    { "精灵僵尸罐头", PM_ELF_ZOMBIE, NEUTRAL, TIN },
    { "精灵僵尸的罐头", PM_ELF_ZOMBIE, NEUTRAL, TIN },
    { "精灵僵尸肉罐头", PM_ELF_ZOMBIE, NEUTRAL, TIN },
    { "人类僵尸雕像", PM_HUMAN_ZOMBIE, NEUTRAL, STATUE },
    { "人类僵尸的雕像", PM_HUMAN_ZOMBIE, NEUTRAL, STATUE },
    { "人类僵尸小雕像", PM_HUMAN_ZOMBIE, NEUTRAL, FIGURINE },
    { "人类僵尸的小雕像", PM_HUMAN_ZOMBIE, NEUTRAL, FIGURINE },
    { "人类僵尸罐头", PM_HUMAN_ZOMBIE, NEUTRAL, TIN },
    { "人类僵尸的罐头", PM_HUMAN_ZOMBIE, NEUTRAL, TIN },
    { "人类僵尸肉罐头", PM_HUMAN_ZOMBIE, NEUTRAL, TIN },
    { "双头僵尸雕像", PM_ETTIN_ZOMBIE, NEUTRAL, STATUE },
    { "双头僵尸的雕像", PM_ETTIN_ZOMBIE, NEUTRAL, STATUE },
    { "双头僵尸小雕像", PM_ETTIN_ZOMBIE, NEUTRAL, FIGURINE },
    { "双头僵尸的小雕像", PM_ETTIN_ZOMBIE, NEUTRAL, FIGURINE },
    { "双头僵尸罐头", PM_ETTIN_ZOMBIE, NEUTRAL, TIN },
    { "双头僵尸的罐头", PM_ETTIN_ZOMBIE, NEUTRAL, TIN },
    { "双头僵尸肉罐头", PM_ETTIN_ZOMBIE, NEUTRAL, TIN },
    { "双头巨人僵尸雕像", PM_ETTIN_ZOMBIE, NEUTRAL, STATUE },
    { "双头巨人僵尸的雕像", PM_ETTIN_ZOMBIE, NEUTRAL, STATUE },
    { "双头巨人僵尸小雕像", PM_ETTIN_ZOMBIE, NEUTRAL, FIGURINE },
    { "双头巨人僵尸的小雕像", PM_ETTIN_ZOMBIE, NEUTRAL, FIGURINE },
    { "双头巨人僵尸罐头", PM_ETTIN_ZOMBIE, NEUTRAL, TIN },
    { "双头巨人僵尸的罐头", PM_ETTIN_ZOMBIE, NEUTRAL, TIN },
    { "双头巨人僵尸肉罐头", PM_ETTIN_ZOMBIE, NEUTRAL, TIN },
    { "食尸鬼雕像", PM_GHOUL, NEUTRAL, STATUE },
    { "食尸鬼的雕像", PM_GHOUL, NEUTRAL, STATUE },
    { "食尸鬼小雕像", PM_GHOUL, NEUTRAL, FIGURINE },
    { "食尸鬼的小雕像", PM_GHOUL, NEUTRAL, FIGURINE },
    { "食尸鬼罐头", PM_GHOUL, NEUTRAL, TIN },
    { "食尸鬼的罐头", PM_GHOUL, NEUTRAL, TIN },
    { "食尸鬼肉罐头", PM_GHOUL, NEUTRAL, TIN },
    { "巨人僵尸雕像", PM_GIANT_ZOMBIE, NEUTRAL, STATUE },
    { "巨人僵尸的雕像", PM_GIANT_ZOMBIE, NEUTRAL, STATUE },
    { "巨人僵尸小雕像", PM_GIANT_ZOMBIE, NEUTRAL, FIGURINE },
    { "巨人僵尸的小雕像", PM_GIANT_ZOMBIE, NEUTRAL, FIGURINE },
    { "巨人僵尸罐头", PM_GIANT_ZOMBIE, NEUTRAL, TIN },
    { "巨人僵尸的罐头", PM_GIANT_ZOMBIE, NEUTRAL, TIN },
    { "巨人僵尸肉罐头", PM_GIANT_ZOMBIE, NEUTRAL, TIN },
    { "骷髅雕像", PM_SKELETON, NEUTRAL, STATUE },
    { "骷髅的雕像", PM_SKELETON, NEUTRAL, STATUE },
    { "骷髅小雕像", PM_SKELETON, NEUTRAL, FIGURINE },
    { "骷髅的小雕像", PM_SKELETON, NEUTRAL, FIGURINE },
    { "骷髅罐头", PM_SKELETON, NEUTRAL, TIN },
    { "骷髅的罐头", PM_SKELETON, NEUTRAL, TIN },
    { "骷髅肉罐头", PM_SKELETON, NEUTRAL, TIN },
    { "稻草魔像雕像", PM_STRAW_GOLEM, NEUTRAL, STATUE },
    { "稻草魔像的雕像", PM_STRAW_GOLEM, NEUTRAL, STATUE },
    { "稻草魔像小雕像", PM_STRAW_GOLEM, NEUTRAL, FIGURINE },
    { "稻草魔像的小雕像", PM_STRAW_GOLEM, NEUTRAL, FIGURINE },
    { "稻草魔像罐头", PM_STRAW_GOLEM, NEUTRAL, TIN },
    { "稻草魔像的罐头", PM_STRAW_GOLEM, NEUTRAL, TIN },
    { "稻草魔像肉罐头", PM_STRAW_GOLEM, NEUTRAL, TIN },
    { "纸魔像雕像", PM_PAPER_GOLEM, NEUTRAL, STATUE },
    { "纸魔像的雕像", PM_PAPER_GOLEM, NEUTRAL, STATUE },
    { "纸魔像小雕像", PM_PAPER_GOLEM, NEUTRAL, FIGURINE },
    { "纸魔像的小雕像", PM_PAPER_GOLEM, NEUTRAL, FIGURINE },
    { "纸魔像罐头", PM_PAPER_GOLEM, NEUTRAL, TIN },
    { "纸魔像的罐头", PM_PAPER_GOLEM, NEUTRAL, TIN },
    { "纸魔像肉罐头", PM_PAPER_GOLEM, NEUTRAL, TIN },
    { "绳子魔像雕像", PM_ROPE_GOLEM, NEUTRAL, STATUE },
    { "绳子魔像的雕像", PM_ROPE_GOLEM, NEUTRAL, STATUE },
    { "绳子魔像小雕像", PM_ROPE_GOLEM, NEUTRAL, FIGURINE },
    { "绳子魔像的小雕像", PM_ROPE_GOLEM, NEUTRAL, FIGURINE },
    { "绳子魔像罐头", PM_ROPE_GOLEM, NEUTRAL, TIN },
    { "绳子魔像的罐头", PM_ROPE_GOLEM, NEUTRAL, TIN },
    { "绳子魔像肉罐头", PM_ROPE_GOLEM, NEUTRAL, TIN },
    { "金魔像雕像", PM_GOLD_GOLEM, NEUTRAL, STATUE },
    { "金魔像的雕像", PM_GOLD_GOLEM, NEUTRAL, STATUE },
    { "金魔像小雕像", PM_GOLD_GOLEM, NEUTRAL, FIGURINE },
    { "金魔像的小雕像", PM_GOLD_GOLEM, NEUTRAL, FIGURINE },
    { "金魔像罐头", PM_GOLD_GOLEM, NEUTRAL, TIN },
    { "金魔像的罐头", PM_GOLD_GOLEM, NEUTRAL, TIN },
    { "金魔像肉罐头", PM_GOLD_GOLEM, NEUTRAL, TIN },
    { "皮革魔像雕像", PM_LEATHER_GOLEM, NEUTRAL, STATUE },
    { "皮革魔像的雕像", PM_LEATHER_GOLEM, NEUTRAL, STATUE },
    { "皮革魔像小雕像", PM_LEATHER_GOLEM, NEUTRAL, FIGURINE },
    { "皮革魔像的小雕像", PM_LEATHER_GOLEM, NEUTRAL, FIGURINE },
    { "皮革魔像罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "皮革魔像的罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "皮革魔像肉罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "皮魔像雕像", PM_LEATHER_GOLEM, NEUTRAL, STATUE },
    { "皮魔像的雕像", PM_LEATHER_GOLEM, NEUTRAL, STATUE },
    { "皮魔像小雕像", PM_LEATHER_GOLEM, NEUTRAL, FIGURINE },
    { "皮魔像的小雕像", PM_LEATHER_GOLEM, NEUTRAL, FIGURINE },
    { "皮魔像罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "皮魔像的罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "皮魔像肉罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "木魔像雕像", PM_WOOD_GOLEM, NEUTRAL, STATUE },
    { "木魔像的雕像", PM_WOOD_GOLEM, NEUTRAL, STATUE },
    { "木魔像小雕像", PM_WOOD_GOLEM, NEUTRAL, FIGURINE },
    { "木魔像的小雕像", PM_WOOD_GOLEM, NEUTRAL, FIGURINE },
    { "木魔像罐头", PM_WOOD_GOLEM, NEUTRAL, TIN },
    { "木魔像的罐头", PM_WOOD_GOLEM, NEUTRAL, TIN },
    { "木魔像肉罐头", PM_WOOD_GOLEM, NEUTRAL, TIN },
    { "肉魔像雕像", PM_FLESH_GOLEM, NEUTRAL, STATUE },
    { "肉魔像的雕像", PM_FLESH_GOLEM, NEUTRAL, STATUE },
    { "肉魔像小雕像", PM_FLESH_GOLEM, NEUTRAL, FIGURINE },
    { "肉魔像的小雕像", PM_FLESH_GOLEM, NEUTRAL, FIGURINE },
    { "肉魔像罐头", PM_FLESH_GOLEM, NEUTRAL, TIN },
    { "肉魔像的罐头", PM_FLESH_GOLEM, NEUTRAL, TIN },
    { "肉魔像肉罐头", PM_FLESH_GOLEM, NEUTRAL, TIN },
    { "土魔像雕像", PM_CLAY_GOLEM, NEUTRAL, STATUE },
    { "土魔像的雕像", PM_CLAY_GOLEM, NEUTRAL, STATUE },
    { "土魔像小雕像", PM_CLAY_GOLEM, NEUTRAL, FIGURINE },
    { "土魔像的小雕像", PM_CLAY_GOLEM, NEUTRAL, FIGURINE },
    { "土魔像罐头", PM_CLAY_GOLEM, NEUTRAL, TIN },
    { "土魔像的罐头", PM_CLAY_GOLEM, NEUTRAL, TIN },
    { "土魔像肉罐头", PM_CLAY_GOLEM, NEUTRAL, TIN },
    { "石魔像雕像", PM_STONE_GOLEM, NEUTRAL, STATUE },
    { "石魔像的雕像", PM_STONE_GOLEM, NEUTRAL, STATUE },
    { "石魔像小雕像", PM_STONE_GOLEM, NEUTRAL, FIGURINE },
    { "石魔像的小雕像", PM_STONE_GOLEM, NEUTRAL, FIGURINE },
    { "石魔像罐头", PM_STONE_GOLEM, NEUTRAL, TIN },
    { "石魔像的罐头", PM_STONE_GOLEM, NEUTRAL, TIN },
    { "石魔像肉罐头", PM_STONE_GOLEM, NEUTRAL, TIN },
    { "玻璃魔像雕像", PM_GLASS_GOLEM, NEUTRAL, STATUE },
    { "玻璃魔像的雕像", PM_GLASS_GOLEM, NEUTRAL, STATUE },
    { "玻璃魔像小雕像", PM_GLASS_GOLEM, NEUTRAL, FIGURINE },
    { "玻璃魔像的小雕像", PM_GLASS_GOLEM, NEUTRAL, FIGURINE },
    { "玻璃魔像罐头", PM_GLASS_GOLEM, NEUTRAL, TIN },
    { "玻璃魔像的罐头", PM_GLASS_GOLEM, NEUTRAL, TIN },
    { "玻璃魔像肉罐头", PM_GLASS_GOLEM, NEUTRAL, TIN },
    { "铁魔像雕像", PM_IRON_GOLEM, NEUTRAL, STATUE },
    { "铁魔像的雕像", PM_IRON_GOLEM, NEUTRAL, STATUE },
    { "铁魔像小雕像", PM_IRON_GOLEM, NEUTRAL, FIGURINE },
    { "铁魔像的小雕像", PM_IRON_GOLEM, NEUTRAL, FIGURINE },
    { "铁魔像罐头", PM_IRON_GOLEM, NEUTRAL, TIN },
    { "铁魔像的罐头", PM_IRON_GOLEM, NEUTRAL, TIN },
    { "铁魔像肉罐头", PM_IRON_GOLEM, NEUTRAL, TIN },
    { "稻草傀儡雕像", PM_STRAW_GOLEM, NEUTRAL, STATUE },
    { "稻草傀儡的雕像", PM_STRAW_GOLEM, NEUTRAL, STATUE },
    { "稻草傀儡小雕像", PM_STRAW_GOLEM, NEUTRAL, FIGURINE },
    { "稻草傀儡的小雕像", PM_STRAW_GOLEM, NEUTRAL, FIGURINE },
    { "稻草傀儡罐头", PM_STRAW_GOLEM, NEUTRAL, TIN },
    { "稻草傀儡的罐头", PM_STRAW_GOLEM, NEUTRAL, TIN },
    { "稻草傀儡肉罐头", PM_STRAW_GOLEM, NEUTRAL, TIN },
    { "纸傀儡雕像", PM_PAPER_GOLEM, NEUTRAL, STATUE },
    { "纸傀儡的雕像", PM_PAPER_GOLEM, NEUTRAL, STATUE },
    { "纸傀儡小雕像", PM_PAPER_GOLEM, NEUTRAL, FIGURINE },
    { "纸傀儡的小雕像", PM_PAPER_GOLEM, NEUTRAL, FIGURINE },
    { "纸傀儡罐头", PM_PAPER_GOLEM, NEUTRAL, TIN },
    { "纸傀儡的罐头", PM_PAPER_GOLEM, NEUTRAL, TIN },
    { "纸傀儡肉罐头", PM_PAPER_GOLEM, NEUTRAL, TIN },
    { "绳子傀儡雕像", PM_ROPE_GOLEM, NEUTRAL, STATUE },
    { "绳子傀儡的雕像", PM_ROPE_GOLEM, NEUTRAL, STATUE },
    { "绳子傀儡小雕像", PM_ROPE_GOLEM, NEUTRAL, FIGURINE },
    { "绳子傀儡的小雕像", PM_ROPE_GOLEM, NEUTRAL, FIGURINE },
    { "绳子傀儡罐头", PM_ROPE_GOLEM, NEUTRAL, TIN },
    { "绳子傀儡的罐头", PM_ROPE_GOLEM, NEUTRAL, TIN },
    { "绳子傀儡肉罐头", PM_ROPE_GOLEM, NEUTRAL, TIN },
    { "金傀儡雕像", PM_GOLD_GOLEM, NEUTRAL, STATUE },
    { "金傀儡的雕像", PM_GOLD_GOLEM, NEUTRAL, STATUE },
    { "金傀儡小雕像", PM_GOLD_GOLEM, NEUTRAL, FIGURINE },
    { "金傀儡的小雕像", PM_GOLD_GOLEM, NEUTRAL, FIGURINE },
    { "金傀儡罐头", PM_GOLD_GOLEM, NEUTRAL, TIN },
    { "金傀儡的罐头", PM_GOLD_GOLEM, NEUTRAL, TIN },
    { "金傀儡肉罐头", PM_GOLD_GOLEM, NEUTRAL, TIN },
    { "皮革傀儡雕像", PM_LEATHER_GOLEM, NEUTRAL, STATUE },
    { "皮革傀儡的雕像", PM_LEATHER_GOLEM, NEUTRAL, STATUE },
    { "皮革傀儡小雕像", PM_LEATHER_GOLEM, NEUTRAL, FIGURINE },
    { "皮革傀儡的小雕像", PM_LEATHER_GOLEM, NEUTRAL, FIGURINE },
    { "皮革傀儡罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "皮革傀儡的罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "皮革傀儡肉罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "皮傀儡雕像", PM_LEATHER_GOLEM, NEUTRAL, STATUE },
    { "皮傀儡的雕像", PM_LEATHER_GOLEM, NEUTRAL, STATUE },
    { "皮傀儡小雕像", PM_LEATHER_GOLEM, NEUTRAL, FIGURINE },
    { "皮傀儡的小雕像", PM_LEATHER_GOLEM, NEUTRAL, FIGURINE },
    { "皮傀儡罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "皮傀儡的罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "皮傀儡肉罐头", PM_LEATHER_GOLEM, NEUTRAL, TIN },
    { "木傀儡雕像", PM_WOOD_GOLEM, NEUTRAL, STATUE },
    { "木傀儡的雕像", PM_WOOD_GOLEM, NEUTRAL, STATUE },
    { "木傀儡小雕像", PM_WOOD_GOLEM, NEUTRAL, FIGURINE },
    { "木傀儡的小雕像", PM_WOOD_GOLEM, NEUTRAL, FIGURINE },
    { "木傀儡罐头", PM_WOOD_GOLEM, NEUTRAL, TIN },
    { "木傀儡的罐头", PM_WOOD_GOLEM, NEUTRAL, TIN },
    { "木傀儡肉罐头", PM_WOOD_GOLEM, NEUTRAL, TIN },
    { "肉傀儡雕像", PM_FLESH_GOLEM, NEUTRAL, STATUE },
    { "肉傀儡的雕像", PM_FLESH_GOLEM, NEUTRAL, STATUE },
    { "肉傀儡小雕像", PM_FLESH_GOLEM, NEUTRAL, FIGURINE },
    { "肉傀儡的小雕像", PM_FLESH_GOLEM, NEUTRAL, FIGURINE },
    { "肉傀儡罐头", PM_FLESH_GOLEM, NEUTRAL, TIN },
    { "肉傀儡的罐头", PM_FLESH_GOLEM, NEUTRAL, TIN },
    { "肉傀儡肉罐头", PM_FLESH_GOLEM, NEUTRAL, TIN },
    { "土傀儡雕像", PM_CLAY_GOLEM, NEUTRAL, STATUE },
    { "土傀儡的雕像", PM_CLAY_GOLEM, NEUTRAL, STATUE },
    { "土傀儡小雕像", PM_CLAY_GOLEM, NEUTRAL, FIGURINE },
    { "土傀儡的小雕像", PM_CLAY_GOLEM, NEUTRAL, FIGURINE },
    { "土傀儡罐头", PM_CLAY_GOLEM, NEUTRAL, TIN },
    { "土傀儡的罐头", PM_CLAY_GOLEM, NEUTRAL, TIN },
    { "土傀儡肉罐头", PM_CLAY_GOLEM, NEUTRAL, TIN },
    { "石傀儡雕像", PM_STONE_GOLEM, NEUTRAL, STATUE },
    { "石傀儡的雕像", PM_STONE_GOLEM, NEUTRAL, STATUE },
    { "石傀儡小雕像", PM_STONE_GOLEM, NEUTRAL, FIGURINE },
    { "石傀儡的小雕像", PM_STONE_GOLEM, NEUTRAL, FIGURINE },
    { "石傀儡罐头", PM_STONE_GOLEM, NEUTRAL, TIN },
    { "石傀儡的罐头", PM_STONE_GOLEM, NEUTRAL, TIN },
    { "石傀儡肉罐头", PM_STONE_GOLEM, NEUTRAL, TIN },
    { "玻璃傀儡雕像", PM_GLASS_GOLEM, NEUTRAL, STATUE },
    { "玻璃傀儡的雕像", PM_GLASS_GOLEM, NEUTRAL, STATUE },
    { "玻璃傀儡小雕像", PM_GLASS_GOLEM, NEUTRAL, FIGURINE },
    { "玻璃傀儡的小雕像", PM_GLASS_GOLEM, NEUTRAL, FIGURINE },
    { "玻璃傀儡罐头", PM_GLASS_GOLEM, NEUTRAL, TIN },
    { "玻璃傀儡的罐头", PM_GLASS_GOLEM, NEUTRAL, TIN },
    { "玻璃傀儡肉罐头", PM_GLASS_GOLEM, NEUTRAL, TIN },
    { "铁傀儡雕像", PM_IRON_GOLEM, NEUTRAL, STATUE },
    { "铁傀儡的雕像", PM_IRON_GOLEM, NEUTRAL, STATUE },
    { "铁傀儡小雕像", PM_IRON_GOLEM, NEUTRAL, FIGURINE },
    { "铁傀儡的小雕像", PM_IRON_GOLEM, NEUTRAL, FIGURINE },
    { "铁傀儡罐头", PM_IRON_GOLEM, NEUTRAL, TIN },
    { "铁傀儡的罐头", PM_IRON_GOLEM, NEUTRAL, TIN },
    { "铁傀儡肉罐头", PM_IRON_GOLEM, NEUTRAL, TIN },
    { "人雕像", PM_HUMAN, NEUTRAL, STATUE },
    { "人的雕像", PM_HUMAN, NEUTRAL, STATUE },
    { "人小雕像", PM_HUMAN, NEUTRAL, FIGURINE },
    { "人的小雕像", PM_HUMAN, NEUTRAL, FIGURINE },
    { "人罐头", PM_HUMAN, NEUTRAL, TIN },
    { "人的罐头", PM_HUMAN, NEUTRAL, TIN },
    { "人肉罐头", PM_HUMAN, NEUTRAL, TIN },
    { "人类雕像", PM_HUMAN, NEUTRAL, STATUE },
    { "人类的雕像", PM_HUMAN, NEUTRAL, STATUE },
    { "人类小雕像", PM_HUMAN, NEUTRAL, FIGURINE },
    { "人类的小雕像", PM_HUMAN, NEUTRAL, FIGURINE },
    { "人类罐头", PM_HUMAN, NEUTRAL, TIN },
    { "人类的罐头", PM_HUMAN, NEUTRAL, TIN },
    { "人类肉罐头", PM_HUMAN, NEUTRAL, TIN },
    { "智人雕像", PM_HUMAN, NEUTRAL, STATUE },
    { "智人的雕像", PM_HUMAN, NEUTRAL, STATUE },
    { "智人小雕像", PM_HUMAN, NEUTRAL, FIGURINE },
    { "智人的小雕像", PM_HUMAN, NEUTRAL, FIGURINE },
    { "智人罐头", PM_HUMAN, NEUTRAL, TIN },
    { "智人的罐头", PM_HUMAN, NEUTRAL, TIN },
    { "智人肉罐头", PM_HUMAN, NEUTRAL, TIN },
    { "鼠人雕像", PM_HUMAN_WERERAT, NEUTRAL, STATUE },
    { "鼠人的雕像", PM_HUMAN_WERERAT, NEUTRAL, STATUE },
    { "鼠人小雕像", PM_HUMAN_WERERAT, NEUTRAL, FIGURINE },
    { "鼠人的小雕像", PM_HUMAN_WERERAT, NEUTRAL, FIGURINE },
    { "鼠人罐头", PM_HUMAN_WERERAT, NEUTRAL, TIN },
    { "鼠人的罐头", PM_HUMAN_WERERAT, NEUTRAL, TIN },
    { "鼠人肉罐头", PM_HUMAN_WERERAT, NEUTRAL, TIN },
    { "豺狼人雕像", PM_HUMAN_WEREJACKAL, NEUTRAL, STATUE },
    { "豺狼人的雕像", PM_HUMAN_WEREJACKAL, NEUTRAL, STATUE },
    { "豺狼人小雕像", PM_HUMAN_WEREJACKAL, NEUTRAL, FIGURINE },
    { "豺狼人的小雕像", PM_HUMAN_WEREJACKAL, NEUTRAL, FIGURINE },
    { "豺狼人罐头", PM_HUMAN_WEREJACKAL, NEUTRAL, TIN },
    { "豺狼人的罐头", PM_HUMAN_WEREJACKAL, NEUTRAL, TIN },
    { "豺狼人肉罐头", PM_HUMAN_WEREJACKAL, NEUTRAL, TIN },
    { "狼人雕像", PM_HUMAN_WEREWOLF, NEUTRAL, STATUE },
    { "狼人的雕像", PM_HUMAN_WEREWOLF, NEUTRAL, STATUE },
    { "狼人小雕像", PM_HUMAN_WEREWOLF, NEUTRAL, FIGURINE },
    { "狼人的小雕像", PM_HUMAN_WEREWOLF, NEUTRAL, FIGURINE },
    { "狼人罐头", PM_HUMAN_WEREWOLF, NEUTRAL, TIN },
    { "狼人的罐头", PM_HUMAN_WEREWOLF, NEUTRAL, TIN },
    { "狼人肉罐头", PM_HUMAN_WEREWOLF, NEUTRAL, TIN },
    { "精灵雕像", PM_ELF, NEUTRAL, STATUE },
    { "精灵的雕像", PM_ELF, NEUTRAL, STATUE },
    { "精灵小雕像", PM_ELF, NEUTRAL, FIGURINE },
    { "精灵的小雕像", PM_ELF, NEUTRAL, FIGURINE },
    { "精灵罐头", PM_ELF, NEUTRAL, TIN },
    { "精灵的罐头", PM_ELF, NEUTRAL, TIN },
    { "精灵肉罐头", PM_ELF, NEUTRAL, TIN },
    { "伍德兰精灵雕像", PM_WOODLAND_ELF, NEUTRAL, STATUE },
    { "伍德兰精灵的雕像", PM_WOODLAND_ELF, NEUTRAL, STATUE },
    { "伍德兰精灵小雕像", PM_WOODLAND_ELF, NEUTRAL, FIGURINE },
    { "伍德兰精灵的小雕像", PM_WOODLAND_ELF, NEUTRAL, FIGURINE },
    { "伍德兰精灵罐头", PM_WOODLAND_ELF, NEUTRAL, TIN },
    { "伍德兰精灵的罐头", PM_WOODLAND_ELF, NEUTRAL, TIN },
    { "伍德兰精灵肉罐头", PM_WOODLAND_ELF, NEUTRAL, TIN },
    { "林地精灵雕像", PM_WOODLAND_ELF, NEUTRAL, STATUE },
    { "林地精灵的雕像", PM_WOODLAND_ELF, NEUTRAL, STATUE },
    { "林地精灵小雕像", PM_WOODLAND_ELF, NEUTRAL, FIGURINE },
    { "林地精灵的小雕像", PM_WOODLAND_ELF, NEUTRAL, FIGURINE },
    { "林地精灵罐头", PM_WOODLAND_ELF, NEUTRAL, TIN },
    { "林地精灵的罐头", PM_WOODLAND_ELF, NEUTRAL, TIN },
    { "林地精灵肉罐头", PM_WOODLAND_ELF, NEUTRAL, TIN },
    { "西尔凡精灵雕像", PM_WOODLAND_ELF, NEUTRAL, STATUE },
    { "西尔凡精灵的雕像", PM_WOODLAND_ELF, NEUTRAL, STATUE },
    { "西尔凡精灵小雕像", PM_WOODLAND_ELF, NEUTRAL, FIGURINE },
    { "西尔凡精灵的小雕像", PM_WOODLAND_ELF, NEUTRAL, FIGURINE },
    { "西尔凡精灵罐头", PM_WOODLAND_ELF, NEUTRAL, TIN },
    { "西尔凡精灵的罐头", PM_WOODLAND_ELF, NEUTRAL, TIN },
    { "西尔凡精灵肉罐头", PM_WOODLAND_ELF, NEUTRAL, TIN },
    { "绿精灵雕像", PM_GREEN_ELF, NEUTRAL, STATUE },
    { "绿精灵的雕像", PM_GREEN_ELF, NEUTRAL, STATUE },
    { "绿精灵小雕像", PM_GREEN_ELF, NEUTRAL, FIGURINE },
    { "绿精灵的小雕像", PM_GREEN_ELF, NEUTRAL, FIGURINE },
    { "绿精灵罐头", PM_GREEN_ELF, NEUTRAL, TIN },
    { "绿精灵的罐头", PM_GREEN_ELF, NEUTRAL, TIN },
    { "绿精灵肉罐头", PM_GREEN_ELF, NEUTRAL, TIN },
    { "绿色精灵雕像", PM_GREEN_ELF, NEUTRAL, STATUE },
    { "绿色精灵的雕像", PM_GREEN_ELF, NEUTRAL, STATUE },
    { "绿色精灵小雕像", PM_GREEN_ELF, NEUTRAL, FIGURINE },
    { "绿色精灵的小雕像", PM_GREEN_ELF, NEUTRAL, FIGURINE },
    { "绿色精灵罐头", PM_GREEN_ELF, NEUTRAL, TIN },
    { "绿色精灵的罐头", PM_GREEN_ELF, NEUTRAL, TIN },
    { "绿色精灵肉罐头", PM_GREEN_ELF, NEUTRAL, TIN },
    { "灰精灵雕像", PM_GREY_ELF, NEUTRAL, STATUE },
    { "灰精灵的雕像", PM_GREY_ELF, NEUTRAL, STATUE },
    { "灰精灵小雕像", PM_GREY_ELF, NEUTRAL, FIGURINE },
    { "灰精灵的小雕像", PM_GREY_ELF, NEUTRAL, FIGURINE },
    { "灰精灵罐头", PM_GREY_ELF, NEUTRAL, TIN },
    { "灰精灵的罐头", PM_GREY_ELF, NEUTRAL, TIN },
    { "灰精灵肉罐头", PM_GREY_ELF, NEUTRAL, TIN },
    { "灰色精灵雕像", PM_GREY_ELF, NEUTRAL, STATUE },
    { "灰色精灵的雕像", PM_GREY_ELF, NEUTRAL, STATUE },
    { "灰色精灵小雕像", PM_GREY_ELF, NEUTRAL, FIGURINE },
    { "灰色精灵的小雕像", PM_GREY_ELF, NEUTRAL, FIGURINE },
    { "灰色精灵罐头", PM_GREY_ELF, NEUTRAL, TIN },
    { "灰色精灵的罐头", PM_GREY_ELF, NEUTRAL, TIN },
    { "灰色精灵肉罐头", PM_GREY_ELF, NEUTRAL, TIN },
    { "精灵领主雕像", PM_ELF_NOBLE, MALE, STATUE },
    { "精灵领主的雕像", PM_ELF_NOBLE, MALE, STATUE },
    { "精灵领主小雕像", PM_ELF_NOBLE, MALE, FIGURINE },
    { "精灵领主的小雕像", PM_ELF_NOBLE, MALE, FIGURINE },
    { "精灵领主罐头", PM_ELF_NOBLE, MALE, TIN },
    { "精灵领主的罐头", PM_ELF_NOBLE, MALE, TIN },
    { "精灵领主肉罐头", PM_ELF_NOBLE, MALE, TIN },
    { "精灵女领主雕像", PM_ELF_NOBLE, FEMALE, STATUE },
    { "精灵女领主的雕像", PM_ELF_NOBLE, FEMALE, STATUE },
    { "精灵女领主小雕像", PM_ELF_NOBLE, FEMALE, FIGURINE },
    { "精灵女领主的小雕像", PM_ELF_NOBLE, FEMALE, FIGURINE },
    { "精灵女领主罐头", PM_ELF_NOBLE, FEMALE, TIN },
    { "精灵女领主的罐头", PM_ELF_NOBLE, FEMALE, TIN },
    { "精灵女领主肉罐头", PM_ELF_NOBLE, FEMALE, TIN },
    { "精灵贵族雕像", PM_ELF_NOBLE, NEUTRAL, STATUE },
    { "精灵贵族的雕像", PM_ELF_NOBLE, NEUTRAL, STATUE },
    { "精灵贵族小雕像", PM_ELF_NOBLE, NEUTRAL, FIGURINE },
    { "精灵贵族的小雕像", PM_ELF_NOBLE, NEUTRAL, FIGURINE },
    { "精灵贵族罐头", PM_ELF_NOBLE, NEUTRAL, TIN },
    { "精灵贵族的罐头", PM_ELF_NOBLE, NEUTRAL, TIN },
    { "精灵贵族肉罐头", PM_ELF_NOBLE, NEUTRAL, TIN },
    { "精灵王雕像", PM_ELVEN_MONARCH, MALE, STATUE },
    { "精灵王的雕像", PM_ELVEN_MONARCH, MALE, STATUE },
    { "精灵王小雕像", PM_ELVEN_MONARCH, MALE, FIGURINE },
    { "精灵王的小雕像", PM_ELVEN_MONARCH, MALE, FIGURINE },
    { "精灵王罐头", PM_ELVEN_MONARCH, MALE, TIN },
    { "精灵王的罐头", PM_ELVEN_MONARCH, MALE, TIN },
    { "精灵王肉罐头", PM_ELVEN_MONARCH, MALE, TIN },
    { "精灵女王雕像", PM_ELVEN_MONARCH, FEMALE, STATUE },
    { "精灵女王的雕像", PM_ELVEN_MONARCH, FEMALE, STATUE },
    { "精灵女王小雕像", PM_ELVEN_MONARCH, FEMALE, FIGURINE },
    { "精灵女王的小雕像", PM_ELVEN_MONARCH, FEMALE, FIGURINE },
    { "精灵女王罐头", PM_ELVEN_MONARCH, FEMALE, TIN },
    { "精灵女王的罐头", PM_ELVEN_MONARCH, FEMALE, TIN },
    { "精灵女王肉罐头", PM_ELVEN_MONARCH, FEMALE, TIN },
    { "精灵统治者雕像", PM_ELVEN_MONARCH, NEUTRAL, STATUE },
    { "精灵统治者的雕像", PM_ELVEN_MONARCH, NEUTRAL, STATUE },
    { "精灵统治者小雕像", PM_ELVEN_MONARCH, NEUTRAL, FIGURINE },
    { "精灵统治者的小雕像", PM_ELVEN_MONARCH, NEUTRAL, FIGURINE },
    { "精灵统治者罐头", PM_ELVEN_MONARCH, NEUTRAL, TIN },
    { "精灵统治者的罐头", PM_ELVEN_MONARCH, NEUTRAL, TIN },
    { "精灵统治者肉罐头", PM_ELVEN_MONARCH, NEUTRAL, TIN },
    { "变形人雕像", PM_DOPPELGANGER, NEUTRAL, STATUE },
    { "变形人的雕像", PM_DOPPELGANGER, NEUTRAL, STATUE },
    { "变形人小雕像", PM_DOPPELGANGER, NEUTRAL, FIGURINE },
    { "变形人的小雕像", PM_DOPPELGANGER, NEUTRAL, FIGURINE },
    { "变形人罐头", PM_DOPPELGANGER, NEUTRAL, TIN },
    { "变形人的罐头", PM_DOPPELGANGER, NEUTRAL, TIN },
    { "变形人肉罐头", PM_DOPPELGANGER, NEUTRAL, TIN },
    { "二重身雕像", PM_DOPPELGANGER, NEUTRAL, STATUE },
    { "二重身的雕像", PM_DOPPELGANGER, NEUTRAL, STATUE },
    { "二重身小雕像", PM_DOPPELGANGER, NEUTRAL, FIGURINE },
    { "二重身的小雕像", PM_DOPPELGANGER, NEUTRAL, FIGURINE },
    { "二重身罐头", PM_DOPPELGANGER, NEUTRAL, TIN },
    { "二重身的罐头", PM_DOPPELGANGER, NEUTRAL, TIN },
    { "二重身肉罐头", PM_DOPPELGANGER, NEUTRAL, TIN },
    { "店主雕像", PM_SHOPKEEPER, NEUTRAL, STATUE },
    { "店主的雕像", PM_SHOPKEEPER, NEUTRAL, STATUE },
    { "店主小雕像", PM_SHOPKEEPER, NEUTRAL, FIGURINE },
    { "店主的小雕像", PM_SHOPKEEPER, NEUTRAL, FIGURINE },
    { "店主罐头", PM_SHOPKEEPER, NEUTRAL, TIN },
    { "店主的罐头", PM_SHOPKEEPER, NEUTRAL, TIN },
    { "店主肉罐头", PM_SHOPKEEPER, NEUTRAL, TIN },
    { "警卫雕像", PM_GUARD, NEUTRAL, STATUE },
    { "警卫的雕像", PM_GUARD, NEUTRAL, STATUE },
    { "警卫小雕像", PM_GUARD, NEUTRAL, FIGURINE },
    { "警卫的小雕像", PM_GUARD, NEUTRAL, FIGURINE },
    { "警卫罐头", PM_GUARD, NEUTRAL, TIN },
    { "警卫的罐头", PM_GUARD, NEUTRAL, TIN },
    { "警卫肉罐头", PM_GUARD, NEUTRAL, TIN },
    { "警官雕像", PM_GUARD, NEUTRAL, STATUE },
    { "警官的雕像", PM_GUARD, NEUTRAL, STATUE },
    { "警官小雕像", PM_GUARD, NEUTRAL, FIGURINE },
    { "警官的小雕像", PM_GUARD, NEUTRAL, FIGURINE },
    { "警官罐头", PM_GUARD, NEUTRAL, TIN },
    { "警官的罐头", PM_GUARD, NEUTRAL, TIN },
    { "警官肉罐头", PM_GUARD, NEUTRAL, TIN },
    { "囚犯雕像", PM_PRISONER, NEUTRAL, STATUE },
    { "囚犯的雕像", PM_PRISONER, NEUTRAL, STATUE },
    { "囚犯小雕像", PM_PRISONER, NEUTRAL, FIGURINE },
    { "囚犯的小雕像", PM_PRISONER, NEUTRAL, FIGURINE },
    { "囚犯罐头", PM_PRISONER, NEUTRAL, TIN },
    { "囚犯的罐头", PM_PRISONER, NEUTRAL, TIN },
    { "囚犯肉罐头", PM_PRISONER, NEUTRAL, TIN },
    { "神谕雕像", PM_ORACL, STATUEE},
    { "神谕的雕像", PM_ORACL, STATUEE},
    { "神谕小雕像", PM_ORACL, FIGURINEE},
    { "神谕的小雕像", PM_ORACL, FIGURINEE},
    { "神谕罐头", PM_ORACL, TINE},
    { "神谕的罐头", PM_ORACL, TINE},
    { "神谕肉罐头", PM_ORACL, TINE},
    { "神谕者雕像", PM_ORACL, STATUEE},
    { "神谕者的雕像", PM_ORACL, STATUEE},
    { "神谕者小雕像", PM_ORACL, FIGURINEE},
    { "神谕者的小雕像", PM_ORACL, FIGURINEE},
    { "神谕者罐头", PM_ORACL, TINE},
    { "神谕者的罐头", PM_ORACL, TINE},
    { "神谕者肉罐头", PM_ORACL, TINE},
    { "男牧师雕像", PM_ALIGNED_CLERIC, MALE, STATUE },
    { "男牧师的雕像", PM_ALIGNED_CLERIC, MALE, STATUE },
    { "男牧师小雕像", PM_ALIGNED_CLERIC, MALE, FIGURINE },
    { "男牧师的小雕像", PM_ALIGNED_CLERIC, MALE, FIGURINE },
    { "男牧师罐头", PM_ALIGNED_CLERIC, MALE, TIN },
    { "男牧师的罐头", PM_ALIGNED_CLERIC, MALE, TIN },
    { "男牧师肉罐头", PM_ALIGNED_CLERIC, MALE, TIN },
    { "女牧师雕像", PM_ALIGNED_CLERIC, FEMALE, STATUE },
    { "女牧师的雕像", PM_ALIGNED_CLERIC, FEMALE, STATUE },
    { "女牧师小雕像", PM_ALIGNED_CLERIC, FEMALE, FIGURINE },
    { "女牧师的小雕像", PM_ALIGNED_CLERIC, FEMALE, FIGURINE },
    { "女牧师罐头", PM_ALIGNED_CLERIC, FEMALE, TIN },
    { "女牧师的罐头", PM_ALIGNED_CLERIC, FEMALE, TIN },
    { "女牧师肉罐头", PM_ALIGNED_CLERIC, FEMALE, TIN },
    { "阵营牧师雕像", PM_ALIGNED_CLERIC, NEUTRAL, STATUE },
    { "阵营牧师的雕像", PM_ALIGNED_CLERIC, NEUTRAL, STATUE },
    { "阵营牧师小雕像", PM_ALIGNED_CLERIC, NEUTRAL, FIGURINE },
    { "阵营牧师的小雕像", PM_ALIGNED_CLERIC, NEUTRAL, FIGURINE },
    { "阵营牧师罐头", PM_ALIGNED_CLERIC, NEUTRAL, TIN },
    { "阵营牧师的罐头", PM_ALIGNED_CLERIC, NEUTRAL, TIN },
    { "阵营牧师肉罐头", PM_ALIGNED_CLERIC, NEUTRAL, TIN },
    { "男祭司雕像", PM_ALIGNED_CLERIC, MALE, STATUE },
    { "男祭司的雕像", PM_ALIGNED_CLERIC, MALE, STATUE },
    { "男祭司小雕像", PM_ALIGNED_CLERIC, MALE, FIGURINE },
    { "男祭司的小雕像", PM_ALIGNED_CLERIC, MALE, FIGURINE },
    { "男祭司罐头", PM_ALIGNED_CLERIC, MALE, TIN },
    { "男祭司的罐头", PM_ALIGNED_CLERIC, MALE, TIN },
    { "男祭司肉罐头", PM_ALIGNED_CLERIC, MALE, TIN },
    { "女祭司雕像", PM_ALIGNED_CLERIC, FEMALE, STATUE },
    { "女祭司的雕像", PM_ALIGNED_CLERIC, FEMALE, STATUE },
    { "女祭司小雕像", PM_ALIGNED_CLERIC, FEMALE, FIGURINE },
    { "女祭司的小雕像", PM_ALIGNED_CLERIC, FEMALE, FIGURINE },
    { "女祭司罐头", PM_ALIGNED_CLERIC, FEMALE, TIN },
    { "女祭司的罐头", PM_ALIGNED_CLERIC, FEMALE, TIN },
    { "女祭司肉罐头", PM_ALIGNED_CLERIC, FEMALE, TIN },
    { "阵营祭司雕像", PM_ALIGNED_CLERIC, NEUTRAL, STATUE },
    { "阵营祭司的雕像", PM_ALIGNED_CLERIC, NEUTRAL, STATUE },
    { "阵营祭司小雕像", PM_ALIGNED_CLERIC, NEUTRAL, FIGURINE },
    { "阵营祭司的小雕像", PM_ALIGNED_CLERIC, NEUTRAL, FIGURINE },
    { "阵营祭司罐头", PM_ALIGNED_CLERIC, NEUTRAL, TIN },
    { "阵营祭司的罐头", PM_ALIGNED_CLERIC, NEUTRAL, TIN },
    { "阵营祭司肉罐头", PM_ALIGNED_CLERIC, NEUTRAL, TIN },
    { "祭司雕像", PM_ALIGNED_CLERIC, NEUTRAL, STATUE },
    { "祭司的雕像", PM_ALIGNED_CLERIC, NEUTRAL, STATUE },
    { "祭司小雕像", PM_ALIGNED_CLERIC, NEUTRAL, FIGURINE },
    { "祭司的小雕像", PM_ALIGNED_CLERIC, NEUTRAL, FIGURINE },
    { "祭司罐头", PM_ALIGNED_CLERIC, NEUTRAL, TIN },
    { "祭司的罐头", PM_ALIGNED_CLERIC, NEUTRAL, TIN },
    { "祭司肉罐头", PM_ALIGNED_CLERIC, NEUTRAL, TIN },
    { "高阶男牧师雕像", PM_HIGH_CLERIC, MALE, STATUE },
    { "高阶男牧师的雕像", PM_HIGH_CLERIC, MALE, STATUE },
    { "高阶男牧师小雕像", PM_HIGH_CLERIC, MALE, FIGURINE },
    { "高阶男牧师的小雕像", PM_HIGH_CLERIC, MALE, FIGURINE },
    { "高阶男牧师罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高阶男牧师的罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高阶男牧师肉罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高阶女牧师雕像", PM_HIGH_CLERIC, FEMALE, STATUE },
    { "高阶女牧师的雕像", PM_HIGH_CLERIC, FEMALE, STATUE },
    { "高阶女牧师小雕像", PM_HIGH_CLERIC, FEMALE, FIGURINE },
    { "高阶女牧师的小雕像", PM_HIGH_CLERIC, FEMALE, FIGURINE },
    { "高阶女牧师罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高阶女牧师的罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高阶女牧师肉罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高阶牧师雕像", PM_HIGH_CLERIC, NEUTRAL, STATUE },
    { "高阶牧师的雕像", PM_HIGH_CLERIC, NEUTRAL, STATUE },
    { "高阶牧师小雕像", PM_HIGH_CLERIC, NEUTRAL, FIGURINE },
    { "高阶牧师的小雕像", PM_HIGH_CLERIC, NEUTRAL, FIGURINE },
    { "高阶牧师罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "高阶牧师的罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "高阶牧师肉罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "高阶男祭司雕像", PM_HIGH_CLERIC, MALE, STATUE },
    { "高阶男祭司的雕像", PM_HIGH_CLERIC, MALE, STATUE },
    { "高阶男祭司小雕像", PM_HIGH_CLERIC, MALE, FIGURINE },
    { "高阶男祭司的小雕像", PM_HIGH_CLERIC, MALE, FIGURINE },
    { "高阶男祭司罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高阶男祭司的罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高阶男祭司肉罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高阶女祭司雕像", PM_HIGH_CLERIC, FEMALE, STATUE },
    { "高阶女祭司的雕像", PM_HIGH_CLERIC, FEMALE, STATUE },
    { "高阶女祭司小雕像", PM_HIGH_CLERIC, FEMALE, FIGURINE },
    { "高阶女祭司的小雕像", PM_HIGH_CLERIC, FEMALE, FIGURINE },
    { "高阶女祭司罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高阶女祭司的罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高阶女祭司肉罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高阶祭司雕像", PM_HIGH_CLERIC, NEUTRAL, STATUE },
    { "高阶祭司的雕像", PM_HIGH_CLERIC, NEUTRAL, STATUE },
    { "高阶祭司小雕像", PM_HIGH_CLERIC, NEUTRAL, FIGURINE },
    { "高阶祭司的小雕像", PM_HIGH_CLERIC, NEUTRAL, FIGURINE },
    { "高阶祭司罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "高阶祭司的罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "高阶祭司肉罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "高级男牧师雕像", PM_HIGH_CLERIC, MALE, STATUE },
    { "高级男牧师的雕像", PM_HIGH_CLERIC, MALE, STATUE },
    { "高级男牧师小雕像", PM_HIGH_CLERIC, MALE, FIGURINE },
    { "高级男牧师的小雕像", PM_HIGH_CLERIC, MALE, FIGURINE },
    { "高级男牧师罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高级男牧师的罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高级男牧师肉罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高级女牧师雕像", PM_HIGH_CLERIC, FEMALE, STATUE },
    { "高级女牧师的雕像", PM_HIGH_CLERIC, FEMALE, STATUE },
    { "高级女牧师小雕像", PM_HIGH_CLERIC, FEMALE, FIGURINE },
    { "高级女牧师的小雕像", PM_HIGH_CLERIC, FEMALE, FIGURINE },
    { "高级女牧师罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高级女牧师的罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高级女牧师肉罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高级牧师雕像", PM_HIGH_CLERIC, NEUTRAL, STATUE },
    { "高级牧师的雕像", PM_HIGH_CLERIC, NEUTRAL, STATUE },
    { "高级牧师小雕像", PM_HIGH_CLERIC, NEUTRAL, FIGURINE },
    { "高级牧师的小雕像", PM_HIGH_CLERIC, NEUTRAL, FIGURINE },
    { "高级牧师罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "高级牧师的罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "高级牧师肉罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "高级男祭司雕像", PM_HIGH_CLERIC, MALE, STATUE },
    { "高级男祭司的雕像", PM_HIGH_CLERIC, MALE, STATUE },
    { "高级男祭司小雕像", PM_HIGH_CLERIC, MALE, FIGURINE },
    { "高级男祭司的小雕像", PM_HIGH_CLERIC, MALE, FIGURINE },
    { "高级男祭司罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高级男祭司的罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高级男祭司肉罐头", PM_HIGH_CLERIC, MALE, TIN },
    { "高级女祭司雕像", PM_HIGH_CLERIC, FEMALE, STATUE },
    { "高级女祭司的雕像", PM_HIGH_CLERIC, FEMALE, STATUE },
    { "高级女祭司小雕像", PM_HIGH_CLERIC, FEMALE, FIGURINE },
    { "高级女祭司的小雕像", PM_HIGH_CLERIC, FEMALE, FIGURINE },
    { "高级女祭司罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高级女祭司的罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高级女祭司肉罐头", PM_HIGH_CLERIC, FEMALE, TIN },
    { "高级祭司雕像", PM_HIGH_CLERIC, NEUTRAL, STATUE },
    { "高级祭司的雕像", PM_HIGH_CLERIC, NEUTRAL, STATUE },
    { "高级祭司小雕像", PM_HIGH_CLERIC, NEUTRAL, FIGURINE },
    { "高级祭司的小雕像", PM_HIGH_CLERIC, NEUTRAL, FIGURINE },
    { "高级祭司罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "高级祭司的罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "高级祭司肉罐头", PM_HIGH_CLERIC, NEUTRAL, TIN },
    { "士兵雕像", PM_SOLDIER, NEUTRAL, STATUE },
    { "士兵的雕像", PM_SOLDIER, NEUTRAL, STATUE },
    { "士兵小雕像", PM_SOLDIER, NEUTRAL, FIGURINE },
    { "士兵的小雕像", PM_SOLDIER, NEUTRAL, FIGURINE },
    { "士兵罐头", PM_SOLDIER, NEUTRAL, TIN },
    { "士兵的罐头", PM_SOLDIER, NEUTRAL, TIN },
    { "士兵肉罐头", PM_SOLDIER, NEUTRAL, TIN },
    { "下士雕像", PM_SOLDIER, NEUTRAL, STATUE },
    { "下士的雕像", PM_SOLDIER, NEUTRAL, STATUE },
    { "下士小雕像", PM_SOLDIER, NEUTRAL, FIGURINE },
    { "下士的小雕像", PM_SOLDIER, NEUTRAL, FIGURINE },
    { "下士罐头", PM_SOLDIER, NEUTRAL, TIN },
    { "下士的罐头", PM_SOLDIER, NEUTRAL, TIN },
    { "下士肉罐头", PM_SOLDIER, NEUTRAL, TIN },
    { "中士雕像", PM_SERGEANT, NEUTRAL, STATUE },
    { "中士的雕像", PM_SERGEANT, NEUTRAL, STATUE },
    { "中士小雕像", PM_SERGEANT, NEUTRAL, FIGURINE },
    { "中士的小雕像", PM_SERGEANT, NEUTRAL, FIGURINE },
    { "中士罐头", PM_SERGEANT, NEUTRAL, TIN },
    { "中士的罐头", PM_SERGEANT, NEUTRAL, TIN },
    { "中士肉罐头", PM_SERGEANT, NEUTRAL, TIN },
    { "护士雕像", PM_NURSE, NEUTRAL, STATUE },
    { "护士的雕像", PM_NURSE, NEUTRAL, STATUE },
    { "护士小雕像", PM_NURSE, NEUTRAL, FIGURINE },
    { "护士的小雕像", PM_NURSE, NEUTRAL, FIGURINE },
    { "护士罐头", PM_NURSE, NEUTRAL, TIN },
    { "护士的罐头", PM_NURSE, NEUTRAL, TIN },
    { "护士肉罐头", PM_NURSE, NEUTRAL, TIN },
    { "中尉雕像", PM_LIEUTENANT, NEUTRAL, STATUE },
    { "中尉的雕像", PM_LIEUTENANT, NEUTRAL, STATUE },
    { "中尉小雕像", PM_LIEUTENANT, NEUTRAL, FIGURINE },
    { "中尉的小雕像", PM_LIEUTENANT, NEUTRAL, FIGURINE },
    { "中尉罐头", PM_LIEUTENANT, NEUTRAL, TIN },
    { "中尉的罐头", PM_LIEUTENANT, NEUTRAL, TIN },
    { "中尉肉罐头", PM_LIEUTENANT, NEUTRAL, TIN },
    { "上尉雕像", PM_CAPTAIN, NEUTRAL, STATUE },
    { "上尉的雕像", PM_CAPTAIN, NEUTRAL, STATUE },
    { "上尉小雕像", PM_CAPTAIN, NEUTRAL, FIGURINE },
    { "上尉的小雕像", PM_CAPTAIN, NEUTRAL, FIGURINE },
    { "上尉罐头", PM_CAPTAIN, NEUTRAL, TIN },
    { "上尉的罐头", PM_CAPTAIN, NEUTRAL, TIN },
    { "上尉肉罐头", PM_CAPTAIN, NEUTRAL, TIN },
    { "警卫员雕像", PM_WATCHMAN, NEUTRAL, STATUE },
    { "警卫员的雕像", PM_WATCHMAN, NEUTRAL, STATUE },
    { "警卫员小雕像", PM_WATCHMAN, NEUTRAL, FIGURINE },
    { "警卫员的小雕像", PM_WATCHMAN, NEUTRAL, FIGURINE },
    { "警卫员罐头", PM_WATCHMAN, NEUTRAL, TIN },
    { "警卫员的罐头", PM_WATCHMAN, NEUTRAL, TIN },
    { "警卫员肉罐头", PM_WATCHMAN, NEUTRAL, TIN },
    { "警卫雕像", PM_WATCHMAN, NEUTRAL, STATUE },
    { "警卫的雕像", PM_WATCHMAN, NEUTRAL, STATUE },
    { "警卫小雕像", PM_WATCHMAN, NEUTRAL, FIGURINE },
    { "警卫的小雕像", PM_WATCHMAN, NEUTRAL, FIGURINE },
    { "警卫罐头", PM_WATCHMAN, NEUTRAL, TIN },
    { "警卫的罐头", PM_WATCHMAN, NEUTRAL, TIN },
    { "警卫肉罐头", PM_WATCHMAN, NEUTRAL, TIN },
    { "警卫员队长雕像", PM_WATCH_CAPTAIN, NEUTRAL, STATUE },
    { "警卫员队长的雕像", PM_WATCH_CAPTAIN, NEUTRAL, STATUE },
    { "警卫员队长小雕像", PM_WATCH_CAPTAIN, NEUTRAL, FIGURINE },
    { "警卫员队长的小雕像", PM_WATCH_CAPTAIN, NEUTRAL, FIGURINE },
    { "警卫员队长罐头", PM_WATCH_CAPTAIN, NEUTRAL, TIN },
    { "警卫员队长的罐头", PM_WATCH_CAPTAIN, NEUTRAL, TIN },
    { "警卫员队长肉罐头", PM_WATCH_CAPTAIN, NEUTRAL, TIN },
    { "警卫队长雕像", PM_WATCH_CAPTAIN, NEUTRAL, STATUE },
    { "警卫队长的雕像", PM_WATCH_CAPTAIN, NEUTRAL, STATUE },
    { "警卫队长小雕像", PM_WATCH_CAPTAIN, NEUTRAL, FIGURINE },
    { "警卫队长的小雕像", PM_WATCH_CAPTAIN, NEUTRAL, FIGURINE },
    { "警卫队长罐头", PM_WATCH_CAPTAIN, NEUTRAL, TIN },
    { "警卫队长的罐头", PM_WATCH_CAPTAIN, NEUTRAL, TIN },
    { "警卫队长肉罐头", PM_WATCH_CAPTAIN, NEUTRAL, TIN },
    { "警卫长雕像", PM_WATCH_CAPTAIN, NEUTRAL, STATUE },
    { "警卫长的雕像", PM_WATCH_CAPTAIN, NEUTRAL, STATUE },
    { "警卫长小雕像", PM_WATCH_CAPTAIN, NEUTRAL, FIGURINE },
    { "警卫长的小雕像", PM_WATCH_CAPTAIN, NEUTRAL, FIGURINE },
    { "警卫长罐头", PM_WATCH_CAPTAIN, NEUTRAL, TIN },
    { "警卫长的罐头", PM_WATCH_CAPTAIN, NEUTRAL, TIN },
    { "警卫长肉罐头", PM_WATCH_CAPTAIN, NEUTRAL, TIN },
    { "美杜莎雕像", PM_MEDUSA, NEUTRAL, STATUE },
    { "美杜莎的雕像", PM_MEDUSA, NEUTRAL, STATUE },
    { "美杜莎小雕像", PM_MEDUSA, NEUTRAL, FIGURINE },
    { "美杜莎的小雕像", PM_MEDUSA, NEUTRAL, FIGURINE },
    { "美杜莎罐头", PM_MEDUSA, NEUTRAL, TIN },
    { "美杜莎的罐头", PM_MEDUSA, NEUTRAL, TIN },
    { "美杜莎肉罐头", PM_MEDUSA, NEUTRAL, TIN },
    { "岩德巫师雕像", PM_WIZARD_OF_YENDOR, NEUTRAL, STATUE },
    { "岩德巫师的雕像", PM_WIZARD_OF_YENDOR, NEUTRAL, STATUE },
    { "岩德巫师小雕像", PM_WIZARD_OF_YENDOR, NEUTRAL, FIGURINE },
    { "岩德巫师的小雕像", PM_WIZARD_OF_YENDOR, NEUTRAL, FIGURINE },
    { "岩德巫师罐头", PM_WIZARD_OF_YENDOR, NEUTRAL, TIN },
    { "岩德巫师的罐头", PM_WIZARD_OF_YENDOR, NEUTRAL, TIN },
    { "岩德巫师肉罐头", PM_WIZARD_OF_YENDOR, NEUTRAL, TIN },
    { "岩德的巫师雕像", PM_WIZARD_OF_YENDOR, NEUTRAL, STATUE },
    { "岩德的巫师的雕像", PM_WIZARD_OF_YENDOR, NEUTRAL, STATUE },
    { "岩德的巫师小雕像", PM_WIZARD_OF_YENDOR, NEUTRAL, FIGURINE },
    { "岩德的巫师的小雕像", PM_WIZARD_OF_YENDOR, NEUTRAL, FIGURINE },
    { "岩德的巫师罐头", PM_WIZARD_OF_YENDOR, NEUTRAL, TIN },
    { "岩德的巫师的罐头", PM_WIZARD_OF_YENDOR, NEUTRAL, TIN },
    { "岩德的巫师肉罐头", PM_WIZARD_OF_YENDOR, NEUTRAL, TIN },
    { "克罗伊斯雕像", PM_CROESUS, NEUTRAL, STATUE },
    { "克罗伊斯的雕像", PM_CROESUS, NEUTRAL, STATUE },
    { "克罗伊斯小雕像", PM_CROESUS, NEUTRAL, FIGURINE },
    { "克罗伊斯的小雕像", PM_CROESUS, NEUTRAL, FIGURINE },
    { "克罗伊斯罐头", PM_CROESUS, NEUTRAL, TIN },
    { "克罗伊斯的罐头", PM_CROESUS, NEUTRAL, TIN },
    { "克罗伊斯肉罐头", PM_CROESUS, NEUTRAL, TIN },
    { "卡隆雕像", PM_CHARON, NEUTRAL, STATUE },
    { "卡隆的雕像", PM_CHARON, NEUTRAL, STATUE },
    { "卡隆小雕像", PM_CHARON, NEUTRAL, FIGURINE },
    { "卡隆的小雕像", PM_CHARON, NEUTRAL, FIGURINE },
    { "卡隆罐头", PM_CHARON, NEUTRAL, TIN },
    { "卡隆的罐头", PM_CHARON, NEUTRAL, TIN },
    { "卡隆肉罐头", PM_CHARON, NEUTRAL, TIN },
    { "喀戎雕像", PM_CHARON, NEUTRAL, STATUE },
    { "喀戎的雕像", PM_CHARON, NEUTRAL, STATUE },
    { "喀戎小雕像", PM_CHARON, NEUTRAL, FIGURINE },
    { "喀戎的小雕像", PM_CHARON, NEUTRAL, FIGURINE },
    { "喀戎罐头", PM_CHARON, NEUTRAL, TIN },
    { "喀戎的罐头", PM_CHARON, NEUTRAL, TIN },
    { "喀戎肉罐头", PM_CHARON, NEUTRAL, TIN },
    { "鬼魂雕像", PM_GHOST, NEUTRAL, STATUE },
    { "鬼魂的雕像", PM_GHOST, NEUTRAL, STATUE },
    { "鬼魂小雕像", PM_GHOST, NEUTRAL, FIGURINE },
    { "鬼魂的小雕像", PM_GHOST, NEUTRAL, FIGURINE },
    { "鬼魂罐头", PM_GHOST, NEUTRAL, TIN },
    { "鬼魂的罐头", PM_GHOST, NEUTRAL, TIN },
    { "鬼魂肉罐头", PM_GHOST, NEUTRAL, TIN },
    { "魂灵雕像", PM_SHADE, NEUTRAL, STATUE },
    { "魂灵的雕像", PM_SHADE, NEUTRAL, STATUE },
    { "魂灵小雕像", PM_SHADE, NEUTRAL, FIGURINE },
    { "魂灵的小雕像", PM_SHADE, NEUTRAL, FIGURINE },
    { "魂灵罐头", PM_SHADE, NEUTRAL, TIN },
    { "魂灵的罐头", PM_SHADE, NEUTRAL, TIN },
    { "魂灵肉罐头", PM_SHADE, NEUTRAL, TIN },
    { "暗影雕像", PM_SHADE, NEUTRAL, STATUE },
    { "暗影的雕像", PM_SHADE, NEUTRAL, STATUE },
    { "暗影小雕像", PM_SHADE, NEUTRAL, FIGURINE },
    { "暗影的小雕像", PM_SHADE, NEUTRAL, FIGURINE },
    { "暗影罐头", PM_SHADE, NEUTRAL, TIN },
    { "暗影的罐头", PM_SHADE, NEUTRAL, TIN },
    { "暗影肉罐头", PM_SHADE, NEUTRAL, TIN },
    { "黑影雕像", PM_SHADE, NEUTRAL, STATUE },
    { "黑影的雕像", PM_SHADE, NEUTRAL, STATUE },
    { "黑影小雕像", PM_SHADE, NEUTRAL, FIGURINE },
    { "黑影的小雕像", PM_SHADE, NEUTRAL, FIGURINE },
    { "黑影罐头", PM_SHADE, NEUTRAL, TIN },
    { "黑影的罐头", PM_SHADE, NEUTRAL, TIN },
    { "黑影肉罐头", PM_SHADE, NEUTRAL, TIN },
    { "水妖雕像", PM_WATER_DEMON, NEUTRAL, STATUE },
    { "水妖的雕像", PM_WATER_DEMON, NEUTRAL, STATUE },
    { "水妖小雕像", PM_WATER_DEMON, NEUTRAL, FIGURINE },
    { "水妖的小雕像", PM_WATER_DEMON, NEUTRAL, FIGURINE },
    { "水妖罐头", PM_WATER_DEMON, NEUTRAL, TIN },
    { "水妖的罐头", PM_WATER_DEMON, NEUTRAL, TIN },
    { "水妖肉罐头", PM_WATER_DEMON, NEUTRAL, TIN },
    { "梦魇雕像", PM_AMOROUS_DEMON, MALE, STATUE },
    { "梦魇的雕像", PM_AMOROUS_DEMON, MALE, STATUE },
    { "梦魇小雕像", PM_AMOROUS_DEMON, MALE, FIGURINE },
    { "梦魇的小雕像", PM_AMOROUS_DEMON, MALE, FIGURINE },
    { "梦魇罐头", PM_AMOROUS_DEMON, MALE, TIN },
    { "梦魇的罐头", PM_AMOROUS_DEMON, MALE, TIN },
    { "梦魇肉罐头", PM_AMOROUS_DEMON, MALE, TIN },
    { "魅魔雕像", PM_AMOROUS_DEMON, FEMALE, STATUE },
    { "魅魔的雕像", PM_AMOROUS_DEMON, FEMALE, STATUE },
    { "魅魔小雕像", PM_AMOROUS_DEMON, FEMALE, FIGURINE },
    { "魅魔的小雕像", PM_AMOROUS_DEMON, FEMALE, FIGURINE },
    { "魅魔罐头", PM_AMOROUS_DEMON, FEMALE, TIN },
    { "魅魔的罐头", PM_AMOROUS_DEMON, FEMALE, TIN },
    { "魅魔肉罐头", PM_AMOROUS_DEMON, FEMALE, TIN },
    { "多情的恶魔雕像", PM_AMOROUS_DEMON, NEUTRAL, STATUE },
    { "多情的恶魔的雕像", PM_AMOROUS_DEMON, NEUTRAL, STATUE },
    { "多情的恶魔小雕像", PM_AMOROUS_DEMON, NEUTRAL, FIGURINE },
    { "多情的恶魔的小雕像", PM_AMOROUS_DEMON, NEUTRAL, FIGURINE },
    { "多情的恶魔罐头", PM_AMOROUS_DEMON, NEUTRAL, TIN },
    { "多情的恶魔的罐头", PM_AMOROUS_DEMON, NEUTRAL, TIN },
    { "多情的恶魔肉罐头", PM_AMOROUS_DEMON, NEUTRAL, TIN },
    { "多情恶魔雕像", PM_AMOROUS_DEMON, NEUTRAL, STATUE },
    { "多情恶魔的雕像", PM_AMOROUS_DEMON, NEUTRAL, STATUE },
    { "多情恶魔小雕像", PM_AMOROUS_DEMON, NEUTRAL, FIGURINE },
    { "多情恶魔的小雕像", PM_AMOROUS_DEMON, NEUTRAL, FIGURINE },
    { "多情恶魔罐头", PM_AMOROUS_DEMON, NEUTRAL, TIN },
    { "多情恶魔的罐头", PM_AMOROUS_DEMON, NEUTRAL, TIN },
    { "多情恶魔肉罐头", PM_AMOROUS_DEMON, NEUTRAL, TIN },
    { "有角的魔鬼雕像", PM_HORNED_DEVIL, NEUTRAL, STATUE },
    { "有角的魔鬼的雕像", PM_HORNED_DEVIL, NEUTRAL, STATUE },
    { "有角的魔鬼小雕像", PM_HORNED_DEVIL, NEUTRAL, FIGURINE },
    { "有角的魔鬼的小雕像", PM_HORNED_DEVIL, NEUTRAL, FIGURINE },
    { "有角的魔鬼罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "有角的魔鬼的罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "有角的魔鬼肉罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "有角魔鬼雕像", PM_HORNED_DEVIL, NEUTRAL, STATUE },
    { "有角魔鬼的雕像", PM_HORNED_DEVIL, NEUTRAL, STATUE },
    { "有角魔鬼小雕像", PM_HORNED_DEVIL, NEUTRAL, FIGURINE },
    { "有角魔鬼的小雕像", PM_HORNED_DEVIL, NEUTRAL, FIGURINE },
    { "有角魔鬼罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "有角魔鬼的罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "有角魔鬼肉罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "有角的恶魔雕像", PM_HORNED_DEVIL, NEUTRAL, STATUE },
    { "有角的恶魔的雕像", PM_HORNED_DEVIL, NEUTRAL, STATUE },
    { "有角的恶魔小雕像", PM_HORNED_DEVIL, NEUTRAL, FIGURINE },
    { "有角的恶魔的小雕像", PM_HORNED_DEVIL, NEUTRAL, FIGURINE },
    { "有角的恶魔罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "有角的恶魔的罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "有角的恶魔肉罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "有角恶魔雕像", PM_HORNED_DEVIL, NEUTRAL, STATUE },
    { "有角恶魔的雕像", PM_HORNED_DEVIL, NEUTRAL, STATUE },
    { "有角恶魔小雕像", PM_HORNED_DEVIL, NEUTRAL, FIGURINE },
    { "有角恶魔的小雕像", PM_HORNED_DEVIL, NEUTRAL, FIGURINE },
    { "有角恶魔罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "有角恶魔的罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "有角恶魔肉罐头", PM_HORNED_DEVIL, NEUTRAL, TIN },
    { "伊里逆丝雕像", PM_ERINYS, NEUTRAL, STATUE },
    { "伊里逆丝的雕像", PM_ERINYS, NEUTRAL, STATUE },
    { "伊里逆丝小雕像", PM_ERINYS, NEUTRAL, FIGURINE },
    { "伊里逆丝的小雕像", PM_ERINYS, NEUTRAL, FIGURINE },
    { "伊里逆丝罐头", PM_ERINYS, NEUTRAL, TIN },
    { "伊里逆丝的罐头", PM_ERINYS, NEUTRAL, TIN },
    { "伊里逆丝肉罐头", PM_ERINYS, NEUTRAL, TIN },
    { "欲魔雕像", PM_ERINYS, NEUTRAL, STATUE },
    { "欲魔的雕像", PM_ERINYS, NEUTRAL, STATUE },
    { "欲魔小雕像", PM_ERINYS, NEUTRAL, FIGURINE },
    { "欲魔的小雕像", PM_ERINYS, NEUTRAL, FIGURINE },
    { "欲魔罐头", PM_ERINYS, NEUTRAL, TIN },
    { "欲魔的罐头", PM_ERINYS, NEUTRAL, TIN },
    { "欲魔肉罐头", PM_ERINYS, NEUTRAL, TIN },
    { "罪魔雕像", PM_ERINYS, NEUTRAL, STATUE },
    { "罪魔的雕像", PM_ERINYS, NEUTRAL, STATUE },
    { "罪魔小雕像", PM_ERINYS, NEUTRAL, FIGURINE },
    { "罪魔的小雕像", PM_ERINYS, NEUTRAL, FIGURINE },
    { "罪魔罐头", PM_ERINYS, NEUTRAL, TIN },
    { "罪魔的罐头", PM_ERINYS, NEUTRAL, TIN },
    { "罪魔肉罐头", PM_ERINYS, NEUTRAL, TIN },
    { "厄里倪厄斯雕像", PM_ERINYS, NEUTRAL, STATUE },
    { "厄里倪厄斯的雕像", PM_ERINYS, NEUTRAL, STATUE },
    { "厄里倪厄斯小雕像", PM_ERINYS, NEUTRAL, FIGURINE },
    { "厄里倪厄斯的小雕像", PM_ERINYS, NEUTRAL, FIGURINE },
    { "厄里倪厄斯罐头", PM_ERINYS, NEUTRAL, TIN },
    { "厄里倪厄斯的罐头", PM_ERINYS, NEUTRAL, TIN },
    { "厄里倪厄斯肉罐头", PM_ERINYS, NEUTRAL, TIN },
    { "哈玛魔雕像", PM_BARBED_DEVIL, NEUTRAL, STATUE },
    { "哈玛魔的雕像", PM_BARBED_DEVIL, NEUTRAL, STATUE },
    { "哈玛魔小雕像", PM_BARBED_DEVIL, NEUTRAL, FIGURINE },
    { "哈玛魔的小雕像", PM_BARBED_DEVIL, NEUTRAL, FIGURINE },
    { "哈玛魔罐头", PM_BARBED_DEVIL, NEUTRAL, TIN },
    { "哈玛魔的罐头", PM_BARBED_DEVIL, NEUTRAL, TIN },
    { "哈玛魔肉罐头", PM_BARBED_DEVIL, NEUTRAL, TIN },
    { "猬魔雕像", PM_BARBED_DEVIL, NEUTRAL, STATUE },
    { "猬魔的雕像", PM_BARBED_DEVIL, NEUTRAL, STATUE },
    { "猬魔小雕像", PM_BARBED_DEVIL, NEUTRAL, FIGURINE },
    { "猬魔的小雕像", PM_BARBED_DEVIL, NEUTRAL, FIGURINE },
    { "猬魔罐头", PM_BARBED_DEVIL, NEUTRAL, TIN },
    { "猬魔的罐头", PM_BARBED_DEVIL, NEUTRAL, TIN },
    { "猬魔肉罐头", PM_BARBED_DEVIL, NEUTRAL, TIN },
    { "六臂蛇魔雕像", PM_MARILITH, NEUTRAL, STATUE },
    { "六臂蛇魔的雕像", PM_MARILITH, NEUTRAL, STATUE },
    { "六臂蛇魔小雕像", PM_MARILITH, NEUTRAL, FIGURINE },
    { "六臂蛇魔的小雕像", PM_MARILITH, NEUTRAL, FIGURINE },
    { "六臂蛇魔罐头", PM_MARILITH, NEUTRAL, TIN },
    { "六臂蛇魔的罐头", PM_MARILITH, NEUTRAL, TIN },
    { "六臂蛇魔肉罐头", PM_MARILITH, NEUTRAL, TIN },
    { "弗洛魔雕像", PM_VROCK, NEUTRAL, STATUE },
    { "弗洛魔的雕像", PM_VROCK, NEUTRAL, STATUE },
    { "弗洛魔小雕像", PM_VROCK, NEUTRAL, FIGURINE },
    { "弗洛魔的小雕像", PM_VROCK, NEUTRAL, FIGURINE },
    { "弗洛魔罐头", PM_VROCK, NEUTRAL, TIN },
    { "弗洛魔的罐头", PM_VROCK, NEUTRAL, TIN },
    { "弗洛魔肉罐头", PM_VROCK, NEUTRAL, TIN },
    { "狂战魔雕像", PM_HEZROU, NEUTRAL, STATUE },
    { "狂战魔的雕像", PM_HEZROU, NEUTRAL, STATUE },
    { "狂战魔小雕像", PM_HEZROU, NEUTRAL, FIGURINE },
    { "狂战魔的小雕像", PM_HEZROU, NEUTRAL, FIGURINE },
    { "狂战魔罐头", PM_HEZROU, NEUTRAL, TIN },
    { "狂战魔的罐头", PM_HEZROU, NEUTRAL, TIN },
    { "狂战魔肉罐头", PM_HEZROU, NEUTRAL, TIN },
    { "骨魔雕像", PM_BONE_DEVIL, NEUTRAL, STATUE },
    { "骨魔的雕像", PM_BONE_DEVIL, NEUTRAL, STATUE },
    { "骨魔小雕像", PM_BONE_DEVIL, NEUTRAL, FIGURINE },
    { "骨魔的小雕像", PM_BONE_DEVIL, NEUTRAL, FIGURINE },
    { "骨魔罐头", PM_BONE_DEVIL, NEUTRAL, TIN },
    { "骨魔的罐头", PM_BONE_DEVIL, NEUTRAL, TIN },
    { "骨魔肉罐头", PM_BONE_DEVIL, NEUTRAL, TIN },
    { "冰魔雕像", PM_ICE_DEVIL, NEUTRAL, STATUE },
    { "冰魔的雕像", PM_ICE_DEVIL, NEUTRAL, STATUE },
    { "冰魔小雕像", PM_ICE_DEVIL, NEUTRAL, FIGURINE },
    { "冰魔的小雕像", PM_ICE_DEVIL, NEUTRAL, FIGURINE },
    { "冰魔罐头", PM_ICE_DEVIL, NEUTRAL, TIN },
    { "冰魔的罐头", PM_ICE_DEVIL, NEUTRAL, TIN },
    { "冰魔肉罐头", PM_ICE_DEVIL, NEUTRAL, TIN },
    { "判魂魔雕像", PM_NALFESHNEE, NEUTRAL, STATUE },
    { "判魂魔的雕像", PM_NALFESHNEE, NEUTRAL, STATUE },
    { "判魂魔小雕像", PM_NALFESHNEE, NEUTRAL, FIGURINE },
    { "判魂魔的小雕像", PM_NALFESHNEE, NEUTRAL, FIGURINE },
    { "判魂魔罐头", PM_NALFESHNEE, NEUTRAL, TIN },
    { "判魂魔的罐头", PM_NALFESHNEE, NEUTRAL, TIN },
    { "判魂魔肉罐头", PM_NALFESHNEE, NEUTRAL, TIN },
    { "深渊恶魔雕像", PM_PIT_FIEND, NEUTRAL, STATUE },
    { "深渊恶魔的雕像", PM_PIT_FIEND, NEUTRAL, STATUE },
    { "深渊恶魔小雕像", PM_PIT_FIEND, NEUTRAL, FIGURINE },
    { "深渊恶魔的小雕像", PM_PIT_FIEND, NEUTRAL, FIGURINE },
    { "深渊恶魔罐头", PM_PIT_FIEND, NEUTRAL, TIN },
    { "深渊恶魔的罐头", PM_PIT_FIEND, NEUTRAL, TIN },
    { "深渊恶魔肉罐头", PM_PIT_FIEND, NEUTRAL, TIN },
    { "桑德斯廷雕像", PM_SANDESTIN, NEUTRAL, STATUE },
    { "桑德斯廷的雕像", PM_SANDESTIN, NEUTRAL, STATUE },
    { "桑德斯廷小雕像", PM_SANDESTIN, NEUTRAL, FIGURINE },
    { "桑德斯廷的小雕像", PM_SANDESTIN, NEUTRAL, FIGURINE },
    { "桑德斯廷罐头", PM_SANDESTIN, NEUTRAL, TIN },
    { "桑德斯廷的罐头", PM_SANDESTIN, NEUTRAL, TIN },
    { "桑德斯廷肉罐头", PM_SANDESTIN, NEUTRAL, TIN },
    { "沙魔雕像", PM_SANDESTIN, NEUTRAL, STATUE },
    { "沙魔的雕像", PM_SANDESTIN, NEUTRAL, STATUE },
    { "沙魔小雕像", PM_SANDESTIN, NEUTRAL, FIGURINE },
    { "沙魔的小雕像", PM_SANDESTIN, NEUTRAL, FIGURINE },
    { "沙魔罐头", PM_SANDESTIN, NEUTRAL, TIN },
    { "沙魔的罐头", PM_SANDESTIN, NEUTRAL, TIN },
    { "沙魔肉罐头", PM_SANDESTIN, NEUTRAL, TIN },
    { "炎魔雕像", PM_BALROG, NEUTRAL, STATUE },
    { "炎魔的雕像", PM_BALROG, NEUTRAL, STATUE },
    { "炎魔小雕像", PM_BALROG, NEUTRAL, FIGURINE },
    { "炎魔的小雕像", PM_BALROG, NEUTRAL, FIGURINE },
    { "炎魔罐头", PM_BALROG, NEUTRAL, TIN },
    { "炎魔的罐头", PM_BALROG, NEUTRAL, TIN },
    { "炎魔肉罐头", PM_BALROG, NEUTRAL, TIN },
    { "朱比烈斯雕像", PM_JUIBLEX, NEUTRAL, STATUE },
    { "朱比烈斯的雕像", PM_JUIBLEX, NEUTRAL, STATUE },
    { "朱比烈斯小雕像", PM_JUIBLEX, NEUTRAL, FIGURINE },
    { "朱比烈斯的小雕像", PM_JUIBLEX, NEUTRAL, FIGURINE },
    { "朱比烈斯罐头", PM_JUIBLEX, NEUTRAL, TIN },
    { "朱比烈斯的罐头", PM_JUIBLEX, NEUTRAL, TIN },
    { "朱比烈斯肉罐头", PM_JUIBLEX, NEUTRAL, TIN },
    { "朱庇莱克斯雕像", PM_JUIBLEX, NEUTRAL, STATUE },
    { "朱庇莱克斯的雕像", PM_JUIBLEX, NEUTRAL, STATUE },
    { "朱庇莱克斯小雕像", PM_JUIBLEX, NEUTRAL, FIGURINE },
    { "朱庇莱克斯的小雕像", PM_JUIBLEX, NEUTRAL, FIGURINE },
    { "朱庇莱克斯罐头", PM_JUIBLEX, NEUTRAL, TIN },
    { "朱庇莱克斯的罐头", PM_JUIBLEX, NEUTRAL, TIN },
    { "朱庇莱克斯肉罐头", PM_JUIBLEX, NEUTRAL, TIN },
    { "伊诺胡雕像", PM_YEENOGHU, NEUTRAL, STATUE },
    { "伊诺胡的雕像", PM_YEENOGHU, NEUTRAL, STATUE },
    { "伊诺胡小雕像", PM_YEENOGHU, NEUTRAL, FIGURINE },
    { "伊诺胡的小雕像", PM_YEENOGHU, NEUTRAL, FIGURINE },
    { "伊诺胡罐头", PM_YEENOGHU, NEUTRAL, TIN },
    { "伊诺胡的罐头", PM_YEENOGHU, NEUTRAL, TIN },
    { "伊诺胡肉罐头", PM_YEENOGHU, NEUTRAL, TIN },
    { "耶诺古雕像", PM_YEENOGHU, NEUTRAL, STATUE },
    { "耶诺古的雕像", PM_YEENOGHU, NEUTRAL, STATUE },
    { "耶诺古小雕像", PM_YEENOGHU, NEUTRAL, FIGURINE },
    { "耶诺古的小雕像", PM_YEENOGHU, NEUTRAL, FIGURINE },
    { "耶诺古罐头", PM_YEENOGHU, NEUTRAL, TIN },
    { "耶诺古的罐头", PM_YEENOGHU, NEUTRAL, TIN },
    { "耶诺古肉罐头", PM_YEENOGHU, NEUTRAL, TIN },
    { "奥迦斯雕像", PM_ORCUS, NEUTRAL, STATUE },
    { "奥迦斯的雕像", PM_ORCUS, NEUTRAL, STATUE },
    { "奥迦斯小雕像", PM_ORCUS, NEUTRAL, FIGURINE },
    { "奥迦斯的小雕像", PM_ORCUS, NEUTRAL, FIGURINE },
    { "奥迦斯罐头", PM_ORCUS, NEUTRAL, TIN },
    { "奥迦斯的罐头", PM_ORCUS, NEUTRAL, TIN },
    { "奥迦斯肉罐头", PM_ORCUS, NEUTRAL, TIN },
    { "奥喀斯雕像", PM_ORCUS, NEUTRAL, STATUE },
    { "奥喀斯的雕像", PM_ORCUS, NEUTRAL, STATUE },
    { "奥喀斯小雕像", PM_ORCUS, NEUTRAL, FIGURINE },
    { "奥喀斯的小雕像", PM_ORCUS, NEUTRAL, FIGURINE },
    { "奥喀斯罐头", PM_ORCUS, NEUTRAL, TIN },
    { "奥喀斯的罐头", PM_ORCUS, NEUTRAL, TIN },
    { "奥喀斯肉罐头", PM_ORCUS, NEUTRAL, TIN },
    { "吉里昂雕像", PM_GERYON, NEUTRAL, STATUE },
    { "吉里昂的雕像", PM_GERYON, NEUTRAL, STATUE },
    { "吉里昂小雕像", PM_GERYON, NEUTRAL, FIGURINE },
    { "吉里昂的小雕像", PM_GERYON, NEUTRAL, FIGURINE },
    { "吉里昂罐头", PM_GERYON, NEUTRAL, TIN },
    { "吉里昂的罐头", PM_GERYON, NEUTRAL, TIN },
    { "吉里昂肉罐头", PM_GERYON, NEUTRAL, TIN },
    { "格殷永雕像", PM_GERYON, NEUTRAL, STATUE },
    { "格殷永的雕像", PM_GERYON, NEUTRAL, STATUE },
    { "格殷永小雕像", PM_GERYON, NEUTRAL, FIGURINE },
    { "格殷永的小雕像", PM_GERYON, NEUTRAL, FIGURINE },
    { "格殷永罐头", PM_GERYON, NEUTRAL, TIN },
    { "格殷永的罐头", PM_GERYON, NEUTRAL, TIN },
    { "格殷永肉罐头", PM_GERYON, NEUTRAL, TIN },
    { "迪斯帕特雕像", PM_DISPATER, NEUTRAL, STATUE },
    { "迪斯帕特的雕像", PM_DISPATER, NEUTRAL, STATUE },
    { "迪斯帕特小雕像", PM_DISPATER, NEUTRAL, FIGURINE },
    { "迪斯帕特的小雕像", PM_DISPATER, NEUTRAL, FIGURINE },
    { "迪斯帕特罐头", PM_DISPATER, NEUTRAL, TIN },
    { "迪斯帕特的罐头", PM_DISPATER, NEUTRAL, TIN },
    { "迪斯帕特肉罐头", PM_DISPATER, NEUTRAL, TIN },
    { "巴力西卜雕像", PM_BAALZEBUB, NEUTRAL, STATUE },
    { "巴力西卜的雕像", PM_BAALZEBUB, NEUTRAL, STATUE },
    { "巴力西卜小雕像", PM_BAALZEBUB, NEUTRAL, FIGURINE },
    { "巴力西卜的小雕像", PM_BAALZEBUB, NEUTRAL, FIGURINE },
    { "巴力西卜罐头", PM_BAALZEBUB, NEUTRAL, TIN },
    { "巴力西卜的罐头", PM_BAALZEBUB, NEUTRAL, TIN },
    { "巴力西卜肉罐头", PM_BAALZEBUB, NEUTRAL, TIN },
    { "别西卜雕像", PM_BAALZEBUB, NEUTRAL, STATUE },
    { "别西卜的雕像", PM_BAALZEBUB, NEUTRAL, STATUE },
    { "别西卜小雕像", PM_BAALZEBUB, NEUTRAL, FIGURINE },
    { "别西卜的小雕像", PM_BAALZEBUB, NEUTRAL, FIGURINE },
    { "别西卜罐头", PM_BAALZEBUB, NEUTRAL, TIN },
    { "别西卜的罐头", PM_BAALZEBUB, NEUTRAL, TIN },
    { "别西卜肉罐头", PM_BAALZEBUB, NEUTRAL, TIN },
    { "阿斯莫德雕像", PM_ASMODEUS, NEUTRAL, STATUE },
    { "阿斯莫德的雕像", PM_ASMODEUS, NEUTRAL, STATUE },
    { "阿斯莫德小雕像", PM_ASMODEUS, NEUTRAL, FIGURINE },
    { "阿斯莫德的小雕像", PM_ASMODEUS, NEUTRAL, FIGURINE },
    { "阿斯莫德罐头", PM_ASMODEUS, NEUTRAL, TIN },
    { "阿斯莫德的罐头", PM_ASMODEUS, NEUTRAL, TIN },
    { "阿斯莫德肉罐头", PM_ASMODEUS, NEUTRAL, TIN },
    { "狄摩高根雕像", PM_DEMOGORGO, STATUEN},
    { "狄摩高根的雕像", PM_DEMOGORGO, STATUEN},
    { "狄摩高根小雕像", PM_DEMOGORGO, FIGURINEN},
    { "狄摩高根的小雕像", PM_DEMOGORGO, FIGURINEN},
    { "狄摩高根罐头", PM_DEMOGORGO, TINN},
    { "狄摩高根的罐头", PM_DEMOGORGO, TINN},
    { "狄摩高根肉罐头", PM_DEMOGORGO, TINN},
    { "死亡雕像", PM_DEATH, NEUTRAL, STATUE },
    { "死亡的雕像", PM_DEATH, NEUTRAL, STATUE },
    { "死亡小雕像", PM_DEATH, NEUTRAL, FIGURINE },
    { "死亡的小雕像", PM_DEATH, NEUTRAL, FIGURINE },
    { "死亡罐头", PM_DEATH, NEUTRAL, TIN },
    { "死亡的罐头", PM_DEATH, NEUTRAL, TIN },
    { "死亡肉罐头", PM_DEATH, NEUTRAL, TIN },
    { "瘟疫雕像", PM_PESTILENCE, NEUTRAL, STATUE },
    { "瘟疫的雕像", PM_PESTILENCE, NEUTRAL, STATUE },
    { "瘟疫小雕像", PM_PESTILENCE, NEUTRAL, FIGURINE },
    { "瘟疫的小雕像", PM_PESTILENCE, NEUTRAL, FIGURINE },
    { "瘟疫罐头", PM_PESTILENCE, NEUTRAL, TIN },
    { "瘟疫的罐头", PM_PESTILENCE, NEUTRAL, TIN },
    { "瘟疫肉罐头", PM_PESTILENCE, NEUTRAL, TIN },
    { "饥荒雕像", PM_FAMINE, NEUTRAL, STATUE },
    { "饥荒的雕像", PM_FAMINE, NEUTRAL, STATUE },
    { "饥荒小雕像", PM_FAMINE, NEUTRAL, FIGURINE },
    { "饥荒的小雕像", PM_FAMINE, NEUTRAL, FIGURINE },
    { "饥荒罐头", PM_FAMINE, NEUTRAL, TIN },
    { "饥荒的罐头", PM_FAMINE, NEUTRAL, TIN },
    { "饥荒肉罐头", PM_FAMINE, NEUTRAL, TIN },
    { "邮件幽灵程序雕像", PM_MAIL_DAEMON, NEUTRAL, STATUE },
    { "邮件幽灵程序的雕像", PM_MAIL_DAEMON, NEUTRAL, STATUE },
    { "邮件幽灵程序小雕像", PM_MAIL_DAEMON, NEUTRAL, FIGURINE },
    { "邮件幽灵程序的小雕像", PM_MAIL_DAEMON, NEUTRAL, FIGURINE },
    { "邮件幽灵程序罐头", PM_MAIL_DAEMON, NEUTRAL, TIN },
    { "邮件幽灵程序的罐头", PM_MAIL_DAEMON, NEUTRAL, TIN },
    { "邮件幽灵程序肉罐头", PM_MAIL_DAEMON, NEUTRAL, TIN },
    { "灯神雕像", PM_DJINNI, NEUTRAL, STATUE },
    { "灯神的雕像", PM_DJINNI, NEUTRAL, STATUE },
    { "灯神小雕像", PM_DJINNI, NEUTRAL, FIGURINE },
    { "灯神的小雕像", PM_DJINNI, NEUTRAL, FIGURINE },
    { "灯神罐头", PM_DJINNI, NEUTRAL, TIN },
    { "灯神的罐头", PM_DJINNI, NEUTRAL, TIN },
    { "灯神肉罐头", PM_DJINNI, NEUTRAL, TIN },
    { "水母雕像", PM_JELLYFISH, NEUTRAL, STATUE },
    { "水母的雕像", PM_JELLYFISH, NEUTRAL, STATUE },
    { "水母小雕像", PM_JELLYFISH, NEUTRAL, FIGURINE },
    { "水母的小雕像", PM_JELLYFISH, NEUTRAL, FIGURINE },
    { "水母罐头", PM_JELLYFISH, NEUTRAL, TIN },
    { "水母的罐头", PM_JELLYFISH, NEUTRAL, TIN },
    { "水母肉罐头", PM_JELLYFISH, NEUTRAL, TIN },
    { "水虎鱼雕像", PM_PIRANHA, NEUTRAL, STATUE },
    { "水虎鱼的雕像", PM_PIRANHA, NEUTRAL, STATUE },
    { "水虎鱼小雕像", PM_PIRANHA, NEUTRAL, FIGURINE },
    { "水虎鱼的小雕像", PM_PIRANHA, NEUTRAL, FIGURINE },
    { "水虎鱼罐头", PM_PIRANHA, NEUTRAL, TIN },
    { "水虎鱼的罐头", PM_PIRANHA, NEUTRAL, TIN },
    { "水虎鱼肉罐头", PM_PIRANHA, NEUTRAL, TIN },
    { "鲨鱼雕像", PM_SHARK, NEUTRAL, STATUE },
    { "鲨鱼的雕像", PM_SHARK, NEUTRAL, STATUE },
    { "鲨鱼小雕像", PM_SHARK, NEUTRAL, FIGURINE },
    { "鲨鱼的小雕像", PM_SHARK, NEUTRAL, FIGURINE },
    { "鲨鱼罐头", PM_SHARK, NEUTRAL, TIN },
    { "鲨鱼的罐头", PM_SHARK, NEUTRAL, TIN },
    { "鲨鱼肉罐头", PM_SHARK, NEUTRAL, TIN },
    { "巨型鳗鱼雕像", PM_GIANT_EEL, NEUTRAL, STATUE },
    { "巨型鳗鱼的雕像", PM_GIANT_EEL, NEUTRAL, STATUE },
    { "巨型鳗鱼小雕像", PM_GIANT_EEL, NEUTRAL, FIGURINE },
    { "巨型鳗鱼的小雕像", PM_GIANT_EEL, NEUTRAL, FIGURINE },
    { "巨型鳗鱼罐头", PM_GIANT_EEL, NEUTRAL, TIN },
    { "巨型鳗鱼的罐头", PM_GIANT_EEL, NEUTRAL, TIN },
    { "巨型鳗鱼肉罐头", PM_GIANT_EEL, NEUTRAL, TIN },
    { "电鳗雕像", PM_ELECTRIC_EEL, NEUTRAL, STATUE },
    { "电鳗的雕像", PM_ELECTRIC_EEL, NEUTRAL, STATUE },
    { "电鳗小雕像", PM_ELECTRIC_EEL, NEUTRAL, FIGURINE },
    { "电鳗的小雕像", PM_ELECTRIC_EEL, NEUTRAL, FIGURINE },
    { "电鳗罐头", PM_ELECTRIC_EEL, NEUTRAL, TIN },
    { "电鳗的罐头", PM_ELECTRIC_EEL, NEUTRAL, TIN },
    { "电鳗肉罐头", PM_ELECTRIC_EEL, NEUTRAL, TIN },
    { "海妖雕像", PM_KRAKEN, NEUTRAL, STATUE },
    { "海妖的雕像", PM_KRAKEN, NEUTRAL, STATUE },
    { "海妖小雕像", PM_KRAKEN, NEUTRAL, FIGURINE },
    { "海妖的小雕像", PM_KRAKEN, NEUTRAL, FIGURINE },
    { "海妖罐头", PM_KRAKEN, NEUTRAL, TIN },
    { "海妖的罐头", PM_KRAKEN, NEUTRAL, TIN },
    { "海妖肉罐头", PM_KRAKEN, NEUTRAL, TIN },
    { "蝾螈雕像", PM_NEWT, NEUTRAL, STATUE },
    { "蝾螈的雕像", PM_NEWT, NEUTRAL, STATUE },
    { "蝾螈小雕像", PM_NEWT, NEUTRAL, FIGURINE },
    { "蝾螈的小雕像", PM_NEWT, NEUTRAL, FIGURINE },
    { "蝾螈罐头", PM_NEWT, NEUTRAL, TIN },
    { "蝾螈的罐头", PM_NEWT, NEUTRAL, TIN },
    { "蝾螈肉罐头", PM_NEWT, NEUTRAL, TIN },
    { "壁虎雕像", PM_GECKO, NEUTRAL, STATUE },
    { "壁虎的雕像", PM_GECKO, NEUTRAL, STATUE },
    { "壁虎小雕像", PM_GECKO, NEUTRAL, FIGURINE },
    { "壁虎的小雕像", PM_GECKO, NEUTRAL, FIGURINE },
    { "壁虎罐头", PM_GECKO, NEUTRAL, TIN },
    { "壁虎的罐头", PM_GECKO, NEUTRAL, TIN },
    { "壁虎肉罐头", PM_GECKO, NEUTRAL, TIN },
    { "鬣蜥雕像", PM_IGUANA, NEUTRAL, STATUE },
    { "鬣蜥的雕像", PM_IGUANA, NEUTRAL, STATUE },
    { "鬣蜥小雕像", PM_IGUANA, NEUTRAL, FIGURINE },
    { "鬣蜥的小雕像", PM_IGUANA, NEUTRAL, FIGURINE },
    { "鬣蜥罐头", PM_IGUANA, NEUTRAL, TIN },
    { "鬣蜥的罐头", PM_IGUANA, NEUTRAL, TIN },
    { "鬣蜥肉罐头", PM_IGUANA, NEUTRAL, TIN },
    { "幼鳄鱼雕像", PM_BABY_CROCODILE, NEUTRAL, STATUE },
    { "幼鳄鱼的雕像", PM_BABY_CROCODILE, NEUTRAL, STATUE },
    { "幼鳄鱼小雕像", PM_BABY_CROCODILE, NEUTRAL, FIGURINE },
    { "幼鳄鱼的小雕像", PM_BABY_CROCODILE, NEUTRAL, FIGURINE },
    { "幼鳄鱼罐头", PM_BABY_CROCODILE, NEUTRAL, TIN },
    { "幼鳄鱼的罐头", PM_BABY_CROCODILE, NEUTRAL, TIN },
    { "幼鳄鱼肉罐头", PM_BABY_CROCODILE, NEUTRAL, TIN },
    { "小鳄鱼雕像", PM_BABY_CROCODILE, NEUTRAL, STATUE },
    { "小鳄鱼的雕像", PM_BABY_CROCODILE, NEUTRAL, STATUE },
    { "小鳄鱼小雕像", PM_BABY_CROCODILE, NEUTRAL, FIGURINE },
    { "小鳄鱼的小雕像", PM_BABY_CROCODILE, NEUTRAL, FIGURINE },
    { "小鳄鱼罐头", PM_BABY_CROCODILE, NEUTRAL, TIN },
    { "小鳄鱼的罐头", PM_BABY_CROCODILE, NEUTRAL, TIN },
    { "小鳄鱼肉罐头", PM_BABY_CROCODILE, NEUTRAL, TIN },
    { "鳄鱼宝宝雕像", PM_BABY_CROCODILE, NEUTRAL, STATUE },
    { "鳄鱼宝宝的雕像", PM_BABY_CROCODILE, NEUTRAL, STATUE },
    { "鳄鱼宝宝小雕像", PM_BABY_CROCODILE, NEUTRAL, FIGURINE },
    { "鳄鱼宝宝的小雕像", PM_BABY_CROCODILE, NEUTRAL, FIGURINE },
    { "鳄鱼宝宝罐头", PM_BABY_CROCODILE, NEUTRAL, TIN },
    { "鳄鱼宝宝的罐头", PM_BABY_CROCODILE, NEUTRAL, TIN },
    { "鳄鱼宝宝肉罐头", PM_BABY_CROCODILE, NEUTRAL, TIN },
    { "蜥蜴雕像", PM_LIZARD, NEUTRAL, STATUE },
    { "蜥蜴的雕像", PM_LIZARD, NEUTRAL, STATUE },
    { "蜥蜴小雕像", PM_LIZARD, NEUTRAL, FIGURINE },
    { "蜥蜴的小雕像", PM_LIZARD, NEUTRAL, FIGURINE },
    { "蜥蜴罐头", PM_LIZARD, NEUTRAL, TIN },
    { "蜥蜴的罐头", PM_LIZARD, NEUTRAL, TIN },
    { "蜥蜴肉罐头", PM_LIZARD, NEUTRAL, TIN },
    { "变色龙雕像", PM_CHAMELEON, NEUTRAL, STATUE },
    { "变色龙的雕像", PM_CHAMELEON, NEUTRAL, STATUE },
    { "变色龙小雕像", PM_CHAMELEON, NEUTRAL, FIGURINE },
    { "变色龙的小雕像", PM_CHAMELEON, NEUTRAL, FIGURINE },
    { "变色龙罐头", PM_CHAMELEON, NEUTRAL, TIN },
    { "变色龙的罐头", PM_CHAMELEON, NEUTRAL, TIN },
    { "变色龙肉罐头", PM_CHAMELEON, NEUTRAL, TIN },
    { "鳄鱼雕像", PM_CROCODILE, NEUTRAL, STATUE },
    { "鳄鱼的雕像", PM_CROCODILE, NEUTRAL, STATUE },
    { "鳄鱼小雕像", PM_CROCODILE, NEUTRAL, FIGURINE },
    { "鳄鱼的小雕像", PM_CROCODILE, NEUTRAL, FIGURINE },
    { "鳄鱼罐头", PM_CROCODILE, NEUTRAL, TIN },
    { "鳄鱼的罐头", PM_CROCODILE, NEUTRAL, TIN },
    { "鳄鱼肉罐头", PM_CROCODILE, NEUTRAL, TIN },
    { "火蜥蜴雕像", PM_SALAMANDE, STATUER},
    { "火蜥蜴的雕像", PM_SALAMANDE, STATUER},
    { "火蜥蜴小雕像", PM_SALAMANDE, FIGURINER},
    { "火蜥蜴的小雕像", PM_SALAMANDE, FIGURINER},
    { "火蜥蜴罐头", PM_SALAMANDE, TINR},
    { "火蜥蜴的罐头", PM_SALAMANDE, TINR},
    { "火蜥蜴肉罐头", PM_SALAMANDE, TINR},
    { "长蠕虫尾雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长蠕虫尾的雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长蠕虫尾小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长蠕虫尾的小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长蠕虫尾罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长蠕虫尾的罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长蠕虫尾肉罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长蠕虫尾巴雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长蠕虫尾巴的雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长蠕虫尾巴小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长蠕虫尾巴的小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长蠕虫尾巴罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长蠕虫尾巴的罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长蠕虫尾巴肉罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长蠕虫的尾巴雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长蠕虫的尾巴的雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长蠕虫的尾巴小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长蠕虫的尾巴的小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长蠕虫的尾巴罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长蠕虫的尾巴的罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长蠕虫的尾巴肉罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长虫尾雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长虫尾的雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长虫尾小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长虫尾的小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长虫尾罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长虫尾的罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长虫尾肉罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长虫尾巴雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长虫尾巴的雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长虫尾巴小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长虫尾巴的小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长虫尾巴罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长虫尾巴的罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长虫尾巴肉罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长虫的尾巴雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长虫的尾巴的雕像", PM_LONG_WORM_TAIL, NEUTRAL, STATUE },
    { "长虫的尾巴小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长虫的尾巴的小雕像", PM_LONG_WORM_TAIL, NEUTRAL, FIGURINE },
    { "长虫的尾巴罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长虫的尾巴的罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "长虫的尾巴肉罐头", PM_LONG_WORM_TAIL, NEUTRAL, TIN },
    { "考古学家雕像", PM_ARCHEOLOGIST, NEUTRAL, STATUE },
    { "考古学家的雕像", PM_ARCHEOLOGIST, NEUTRAL, STATUE },
    { "考古学家小雕像", PM_ARCHEOLOGIST, NEUTRAL, FIGURINE },
    { "考古学家的小雕像", PM_ARCHEOLOGIST, NEUTRAL, FIGURINE },
    { "考古学家罐头", PM_ARCHEOLOGIST, NEUTRAL, TIN },
    { "考古学家的罐头", PM_ARCHEOLOGIST, NEUTRAL, TIN },
    { "考古学家肉罐头", PM_ARCHEOLOGIST, NEUTRAL, TIN },
    { "野蛮人雕像", PM_BARBARIAN, NEUTRAL, STATUE },
    { "野蛮人的雕像", PM_BARBARIAN, NEUTRAL, STATUE },
    { "野蛮人小雕像", PM_BARBARIAN, NEUTRAL, FIGURINE },
    { "野蛮人的小雕像", PM_BARBARIAN, NEUTRAL, FIGURINE },
    { "野蛮人罐头", PM_BARBARIAN, NEUTRAL, TIN },
    { "野蛮人的罐头", PM_BARBARIAN, NEUTRAL, TIN },
    { "野蛮人肉罐头", PM_BARBARIAN, NEUTRAL, TIN },
    { "男穴居人雕像", PM_CAVE_DWELLER, MALE, STATUE },
    { "男穴居人的雕像", PM_CAVE_DWELLER, MALE, STATUE },
    { "男穴居人小雕像", PM_CAVE_DWELLER, MALE, FIGURINE },
    { "男穴居人的小雕像", PM_CAVE_DWELLER, MALE, FIGURINE },
    { "男穴居人罐头", PM_CAVE_DWELLER, MALE, TIN },
    { "男穴居人的罐头", PM_CAVE_DWELLER, MALE, TIN },
    { "男穴居人肉罐头", PM_CAVE_DWELLER, MALE, TIN },
    { "男穴居人雕像", PM_CAVE_DWELLER, FEMALE, STATUE },
    { "男穴居人的雕像", PM_CAVE_DWELLER, FEMALE, STATUE },
    { "男穴居人小雕像", PM_CAVE_DWELLER, FEMALE, FIGURINE },
    { "男穴居人的小雕像", PM_CAVE_DWELLER, FEMALE, FIGURINE },
    { "男穴居人罐头", PM_CAVE_DWELLER, FEMALE, TIN },
    { "男穴居人的罐头", PM_CAVE_DWELLER, FEMALE, TIN },
    { "男穴居人肉罐头", PM_CAVE_DWELLER, FEMALE, TIN },
    { "女穴居人雕像", PM_CAVE_DWELLER, NEUTRAL, STATUE },
    { "女穴居人的雕像", PM_CAVE_DWELLER, NEUTRAL, STATUE },
    { "女穴居人小雕像", PM_CAVE_DWELLER, NEUTRAL, FIGURINE },
    { "女穴居人的小雕像", PM_CAVE_DWELLER, NEUTRAL, FIGURINE },
    { "女穴居人罐头", PM_CAVE_DWELLER, NEUTRAL, TIN },
    { "女穴居人的罐头", PM_CAVE_DWELLER, NEUTRAL, TIN },
    { "女穴居人肉罐头", PM_CAVE_DWELLER, NEUTRAL, TIN },
    { "医生雕像", PM_HEALER, NEUTRAL, STATUE },
    { "医生的雕像", PM_HEALER, NEUTRAL, STATUE },
    { "医生小雕像", PM_HEALER, NEUTRAL, FIGURINE },
    { "医生的小雕像", PM_HEALER, NEUTRAL, FIGURINE },
    { "医生罐头", PM_HEALER, NEUTRAL, TIN },
    { "医生的罐头", PM_HEALER, NEUTRAL, TIN },
    { "医生肉罐头", PM_HEALER, NEUTRAL, TIN },
    { "治疗师雕像", PM_HEALER, NEUTRAL, STATUE },
    { "治疗师的雕像", PM_HEALER, NEUTRAL, STATUE },
    { "治疗师小雕像", PM_HEALER, NEUTRAL, FIGURINE },
    { "治疗师的小雕像", PM_HEALER, NEUTRAL, FIGURINE },
    { "治疗师罐头", PM_HEALER, NEUTRAL, TIN },
    { "治疗师的罐头", PM_HEALER, NEUTRAL, TIN },
    { "治疗师肉罐头", PM_HEALER, NEUTRAL, TIN },
    { "骑士雕像", PM_KNIGHT, NEUTRAL, STATUE },
    { "骑士的雕像", PM_KNIGHT, NEUTRAL, STATUE },
    { "骑士小雕像", PM_KNIGHT, NEUTRAL, FIGURINE },
    { "骑士的小雕像", PM_KNIGHT, NEUTRAL, FIGURINE },
    { "骑士罐头", PM_KNIGHT, NEUTRAL, TIN },
    { "骑士的罐头", PM_KNIGHT, NEUTRAL, TIN },
    { "骑士肉罐头", PM_KNIGHT, NEUTRAL, TIN },
    { "僧侣雕像", PM_MON, STATUEK},
    { "僧侣的雕像", PM_MON, STATUEK},
    { "僧侣小雕像", PM_MON, FIGURINEK},
    { "僧侣的小雕像", PM_MON, FIGURINEK},
    { "僧侣罐头", PM_MON, TINK},
    { "僧侣的罐头", PM_MON, TINK},
    { "僧侣肉罐头", PM_MON, TINK},
    { "男牧师雕像", PM_CLERIC, MALE, STATUE },
    { "男牧师的雕像", PM_CLERIC, MALE, STATUE },
    { "男牧师小雕像", PM_CLERIC, MALE, FIGURINE },
    { "男牧师的小雕像", PM_CLERIC, MALE, FIGURINE },
    { "男牧师罐头", PM_CLERIC, MALE, TIN },
    { "男牧师的罐头", PM_CLERIC, MALE, TIN },
    { "男牧师肉罐头", PM_CLERIC, MALE, TIN },
    { "女牧师雕像", PM_CLERIC, FEMALE, STATUE },
    { "女牧师的雕像", PM_CLERIC, FEMALE, STATUE },
    { "女牧师小雕像", PM_CLERIC, FEMALE, FIGURINE },
    { "女牧师的小雕像", PM_CLERIC, FEMALE, FIGURINE },
    { "女牧师罐头", PM_CLERIC, FEMALE, TIN },
    { "女牧师的罐头", PM_CLERIC, FEMALE, TIN },
    { "女牧师肉罐头", PM_CLERIC, FEMALE, TIN },
    { "牧师雕像", PM_CLERIC, NEUTRAL, STATUE },
    { "牧师的雕像", PM_CLERIC, NEUTRAL, STATUE },
    { "牧师小雕像", PM_CLERIC, NEUTRAL, FIGURINE },
    { "牧师的小雕像", PM_CLERIC, NEUTRAL, FIGURINE },
    { "牧师罐头", PM_CLERIC, NEUTRAL, TIN },
    { "牧师的罐头", PM_CLERIC, NEUTRAL, TIN },
    { "牧师肉罐头", PM_CLERIC, NEUTRAL, TIN },
    { "游侠雕像", PM_RANGER, NEUTRAL, STATUE },
    { "游侠的雕像", PM_RANGER, NEUTRAL, STATUE },
    { "游侠小雕像", PM_RANGER, NEUTRAL, FIGURINE },
    { "游侠的小雕像", PM_RANGER, NEUTRAL, FIGURINE },
    { "游侠罐头", PM_RANGER, NEUTRAL, TIN },
    { "游侠的罐头", PM_RANGER, NEUTRAL, TIN },
    { "游侠肉罐头", PM_RANGER, NEUTRAL, TIN },
    { "盗贼雕像", PM_ROGUE, NEUTRAL, STATUE },
    { "盗贼的雕像", PM_ROGUE, NEUTRAL, STATUE },
    { "盗贼小雕像", PM_ROGUE, NEUTRAL, FIGURINE },
    { "盗贼的小雕像", PM_ROGUE, NEUTRAL, FIGURINE },
    { "盗贼罐头", PM_ROGUE, NEUTRAL, TIN },
    { "盗贼的罐头", PM_ROGUE, NEUTRAL, TIN },
    { "盗贼肉罐头", PM_ROGUE, NEUTRAL, TIN },
    { "武士雕像", PM_SAMURAI, NEUTRAL, STATUE },
    { "武士的雕像", PM_SAMURAI, NEUTRAL, STATUE },
    { "武士小雕像", PM_SAMURAI, NEUTRAL, FIGURINE },
    { "武士的小雕像", PM_SAMURAI, NEUTRAL, FIGURINE },
    { "武士罐头", PM_SAMURAI, NEUTRAL, TIN },
    { "武士的罐头", PM_SAMURAI, NEUTRAL, TIN },
    { "武士肉罐头", PM_SAMURAI, NEUTRAL, TIN },
    { "游客雕像", PM_TOURIST, NEUTRAL, STATUE },
    { "游客的雕像", PM_TOURIST, NEUTRAL, STATUE },
    { "游客小雕像", PM_TOURIST, NEUTRAL, FIGURINE },
    { "游客的小雕像", PM_TOURIST, NEUTRAL, FIGURINE },
    { "游客罐头", PM_TOURIST, NEUTRAL, TIN },
    { "游客的罐头", PM_TOURIST, NEUTRAL, TIN },
    { "游客肉罐头", PM_TOURIST, NEUTRAL, TIN },
    { "女武神雕像", PM_VALKYRIE, NEUTRAL, STATUE },
    { "女武神的雕像", PM_VALKYRIE, NEUTRAL, STATUE },
    { "女武神小雕像", PM_VALKYRIE, NEUTRAL, FIGURINE },
    { "女武神的小雕像", PM_VALKYRIE, NEUTRAL, FIGURINE },
    { "女武神罐头", PM_VALKYRIE, NEUTRAL, TIN },
    { "女武神的罐头", PM_VALKYRIE, NEUTRAL, TIN },
    { "女武神肉罐头", PM_VALKYRIE, NEUTRAL, TIN },
    { "巫师雕像", PM_WIZARD, NEUTRAL, STATUE },
    { "巫师的雕像", PM_WIZARD, NEUTRAL, STATUE },
    { "巫师小雕像", PM_WIZARD, NEUTRAL, FIGURINE },
    { "巫师的小雕像", PM_WIZARD, NEUTRAL, FIGURINE },
    { "巫师罐头", PM_WIZARD, NEUTRAL, TIN },
    { "巫师的罐头", PM_WIZARD, NEUTRAL, TIN },
    { "巫师肉罐头", PM_WIZARD, NEUTRAL, TIN },
    { "卡那封勋爵雕像", PM_LORD_CARNARVON, NEUTRAL, STATUE },
    { "卡那封勋爵的雕像", PM_LORD_CARNARVON, NEUTRAL, STATUE },
    { "卡那封勋爵小雕像", PM_LORD_CARNARVON, NEUTRAL, FIGURINE },
    { "卡那封勋爵的小雕像", PM_LORD_CARNARVON, NEUTRAL, FIGURINE },
    { "卡那封勋爵罐头", PM_LORD_CARNARVON, NEUTRAL, TIN },
    { "卡那封勋爵的罐头", PM_LORD_CARNARVON, NEUTRAL, TIN },
    { "卡那封勋爵肉罐头", PM_LORD_CARNARVON, NEUTRAL, TIN },
    { "珀利阿斯雕像", PM_PELIAS, NEUTRAL, STATUE },
    { "珀利阿斯的雕像", PM_PELIAS, NEUTRAL, STATUE },
    { "珀利阿斯小雕像", PM_PELIAS, NEUTRAL, FIGURINE },
    { "珀利阿斯的小雕像", PM_PELIAS, NEUTRAL, FIGURINE },
    { "珀利阿斯罐头", PM_PELIAS, NEUTRAL, TIN },
    { "珀利阿斯的罐头", PM_PELIAS, NEUTRAL, TIN },
    { "珀利阿斯肉罐头", PM_PELIAS, NEUTRAL, TIN },
    { "萨满卡诺夫雕像", PM_SHAMAN_KARNOV, NEUTRAL, STATUE },
    { "萨满卡诺夫的雕像", PM_SHAMAN_KARNOV, NEUTRAL, STATUE },
    { "萨满卡诺夫小雕像", PM_SHAMAN_KARNOV, NEUTRAL, FIGURINE },
    { "萨满卡诺夫的小雕像", PM_SHAMAN_KARNOV, NEUTRAL, FIGURINE },
    { "萨满卡诺夫罐头", PM_SHAMAN_KARNOV, NEUTRAL, TIN },
    { "萨满卡诺夫的罐头", PM_SHAMAN_KARNOV, NEUTRAL, TIN },
    { "萨满卡诺夫肉罐头", PM_SHAMAN_KARNOV, NEUTRAL, TIN },
    { "希波克拉底雕像", PM_HIPPOCRATES, NEUTRAL, STATUE },
    { "希波克拉底的雕像", PM_HIPPOCRATES, NEUTRAL, STATUE },
    { "希波克拉底小雕像", PM_HIPPOCRATES, NEUTRAL, FIGURINE },
    { "希波克拉底的小雕像", PM_HIPPOCRATES, NEUTRAL, FIGURINE },
    { "希波克拉底罐头", PM_HIPPOCRATES, NEUTRAL, TIN },
    { "希波克拉底的罐头", PM_HIPPOCRATES, NEUTRAL, TIN },
    { "希波克拉底肉罐头", PM_HIPPOCRATES, NEUTRAL, TIN },
    { "亚瑟王雕像", PM_KING_ARTHUR, NEUTRAL, STATUE },
    { "亚瑟王的雕像", PM_KING_ARTHUR, NEUTRAL, STATUE },
    { "亚瑟王小雕像", PM_KING_ARTHUR, NEUTRAL, FIGURINE },
    { "亚瑟王的小雕像", PM_KING_ARTHUR, NEUTRAL, FIGURINE },
    { "亚瑟王罐头", PM_KING_ARTHUR, NEUTRAL, TIN },
    { "亚瑟王的罐头", PM_KING_ARTHUR, NEUTRAL, TIN },
    { "亚瑟王肉罐头", PM_KING_ARTHUR, NEUTRAL, TIN },
    { "亚瑟雕像", PM_KING_ARTHUR, NEUTRAL, STATUE },
    { "亚瑟的雕像", PM_KING_ARTHUR, NEUTRAL, STATUE },
    { "亚瑟小雕像", PM_KING_ARTHUR, NEUTRAL, FIGURINE },
    { "亚瑟的小雕像", PM_KING_ARTHUR, NEUTRAL, FIGURINE },
    { "亚瑟罐头", PM_KING_ARTHUR, NEUTRAL, TIN },
    { "亚瑟的罐头", PM_KING_ARTHUR, NEUTRAL, TIN },
    { "亚瑟肉罐头", PM_KING_ARTHUR, NEUTRAL, TIN },
    { "宗师雕像", PM_GRAND_MASTER, NEUTRAL, STATUE },
    { "宗师的雕像", PM_GRAND_MASTER, NEUTRAL, STATUE },
    { "宗师小雕像", PM_GRAND_MASTER, NEUTRAL, FIGURINE },
    { "宗师的小雕像", PM_GRAND_MASTER, NEUTRAL, FIGURINE },
    { "宗师罐头", PM_GRAND_MASTER, NEUTRAL, TIN },
    { "宗师的罐头", PM_GRAND_MASTER, NEUTRAL, TIN },
    { "宗师肉罐头", PM_GRAND_MASTER, NEUTRAL, TIN },
    { "大祭司雕像", PM_ARCH_PRIEST, NEUTRAL, STATUE },
    { "大祭司的雕像", PM_ARCH_PRIEST, NEUTRAL, STATUE },
    { "大祭司小雕像", PM_ARCH_PRIEST, NEUTRAL, FIGURINE },
    { "大祭司的小雕像", PM_ARCH_PRIEST, NEUTRAL, FIGURINE },
    { "大祭司罐头", PM_ARCH_PRIEST, NEUTRAL, TIN },
    { "大祭司的罐头", PM_ARCH_PRIEST, NEUTRAL, TIN },
    { "大祭司肉罐头", PM_ARCH_PRIEST, NEUTRAL, TIN },
    { "俄里翁雕像", PM_ORION, NEUTRAL, STATUE },
    { "俄里翁的雕像", PM_ORION, NEUTRAL, STATUE },
    { "俄里翁小雕像", PM_ORION, NEUTRAL, FIGURINE },
    { "俄里翁的小雕像", PM_ORION, NEUTRAL, FIGURINE },
    { "俄里翁罐头", PM_ORION, NEUTRAL, TIN },
    { "俄里翁的罐头", PM_ORION, NEUTRAL, TIN },
    { "俄里翁肉罐头", PM_ORION, NEUTRAL, TIN },
    { "盗贼大师雕像", PM_MASTER_OF_THIEVES, NEUTRAL, STATUE },
    { "盗贼大师的雕像", PM_MASTER_OF_THIEVES, NEUTRAL, STATUE },
    { "盗贼大师小雕像", PM_MASTER_OF_THIEVES, NEUTRAL, FIGURINE },
    { "盗贼大师的小雕像", PM_MASTER_OF_THIEVES, NEUTRAL, FIGURINE },
    { "盗贼大师罐头", PM_MASTER_OF_THIEVES, NEUTRAL, TIN },
    { "盗贼大师的罐头", PM_MASTER_OF_THIEVES, NEUTRAL, TIN },
    { "盗贼大师肉罐头", PM_MASTER_OF_THIEVES, NEUTRAL, TIN },
    { "萨托领主雕像", PM_LORD_SATO, NEUTRAL, STATUE },
    { "萨托领主的雕像", PM_LORD_SATO, NEUTRAL, STATUE },
    { "萨托领主小雕像", PM_LORD_SATO, NEUTRAL, FIGURINE },
    { "萨托领主的小雕像", PM_LORD_SATO, NEUTRAL, FIGURINE },
    { "萨托领主罐头", PM_LORD_SATO, NEUTRAL, TIN },
    { "萨托领主的罐头", PM_LORD_SATO, NEUTRAL, TIN },
    { "萨托领主肉罐头", PM_LORD_SATO, NEUTRAL, TIN },
    { "双花雕像", PM_TWOFLOWER, NEUTRAL, STATUE },
    { "双花的雕像", PM_TWOFLOWER, NEUTRAL, STATUE },
    { "双花小雕像", PM_TWOFLOWER, NEUTRAL, FIGURINE },
    { "双花的小雕像", PM_TWOFLOWER, NEUTRAL, FIGURINE },
    { "双花罐头", PM_TWOFLOWER, NEUTRAL, TIN },
    { "双花的罐头", PM_TWOFLOWER, NEUTRAL, TIN },
    { "双花肉罐头", PM_TWOFLOWER, NEUTRAL, TIN },
    { "诺恩雕像", PM_NORN, NEUTRAL, STATUE },
    { "诺恩的雕像", PM_NORN, NEUTRAL, STATUE },
    { "诺恩小雕像", PM_NORN, NEUTRAL, FIGURINE },
    { "诺恩的小雕像", PM_NORN, NEUTRAL, FIGURINE },
    { "诺恩罐头", PM_NORN, NEUTRAL, TIN },
    { "诺恩的罐头", PM_NORN, NEUTRAL, TIN },
    { "诺恩肉罐头", PM_NORN, NEUTRAL, TIN },
    { "绿衣娜菲利特雕像", PM_NEFERET_THE_GREEN, NEUTRAL, STATUE },
    { "绿衣娜菲利特的雕像", PM_NEFERET_THE_GREEN, NEUTRAL, STATUE },
    { "绿衣娜菲利特小雕像", PM_NEFERET_THE_GREEN, NEUTRAL, FIGURINE },
    { "绿衣娜菲利特的小雕像", PM_NEFERET_THE_GREEN, NEUTRAL, FIGURINE },
    { "绿衣娜菲利特罐头", PM_NEFERET_THE_GREEN, NEUTRAL, TIN },
    { "绿衣娜菲利特的罐头", PM_NEFERET_THE_GREEN, NEUTRAL, TIN },
    { "绿衣娜菲利特肉罐头", PM_NEFERET_THE_GREEN, NEUTRAL, TIN },
    { "修堤库特里的奴才雕像", PM_MINION_OF_HUHETOTL, NEUTRAL, STATUE },
    { "修堤库特里的奴才的雕像", PM_MINION_OF_HUHETOTL, NEUTRAL, STATUE },
    { "修堤库特里的奴才小雕像", PM_MINION_OF_HUHETOTL, NEUTRAL, FIGURINE },
    { "修堤库特里的奴才的小雕像", PM_MINION_OF_HUHETOTL, NEUTRAL, FIGURINE },
    { "修堤库特里的奴才罐头", PM_MINION_OF_HUHETOTL, NEUTRAL, TIN },
    { "修堤库特里的奴才的罐头", PM_MINION_OF_HUHETOTL, NEUTRAL, TIN },
    { "修堤库特里的奴才肉罐头", PM_MINION_OF_HUHETOTL, NEUTRAL, TIN },
    { "休特奥特尔的爪牙雕像", PM_MINION_OF_HUHETOTL, NEUTRAL, STATUE },
    { "休特奥特尔的爪牙的雕像", PM_MINION_OF_HUHETOTL, NEUTRAL, STATUE },
    { "休特奥特尔的爪牙小雕像", PM_MINION_OF_HUHETOTL, NEUTRAL, FIGURINE },
    { "休特奥特尔的爪牙的小雕像", PM_MINION_OF_HUHETOTL, NEUTRAL, FIGURINE },
    { "休特奥特尔的爪牙罐头", PM_MINION_OF_HUHETOTL, NEUTRAL, TIN },
    { "休特奥特尔的爪牙的罐头", PM_MINION_OF_HUHETOTL, NEUTRAL, TIN },
    { "休特奥特尔的爪牙肉罐头", PM_MINION_OF_HUHETOTL, NEUTRAL, TIN },
    { "图特阿蒙雕像", PM_THOTH_AMO, STATUEN},
    { "图特阿蒙的雕像", PM_THOTH_AMO, STATUEN},
    { "图特阿蒙小雕像", PM_THOTH_AMO, FIGURINEN},
    { "图特阿蒙的小雕像", PM_THOTH_AMO, FIGURINEN},
    { "图特阿蒙罐头", PM_THOTH_AMO, TINN},
    { "图特阿蒙的罐头", PM_THOTH_AMO, TINN},
    { "图特阿蒙肉罐头", PM_THOTH_AMO, TINN},
    { "彩色龙雕像", PM_CHROMATIC_DRAGON, NEUTRAL, STATUE },
    { "彩色龙的雕像", PM_CHROMATIC_DRAGON, NEUTRAL, STATUE },
    { "彩色龙小雕像", PM_CHROMATIC_DRAGON, NEUTRAL, FIGURINE },
    { "彩色龙的小雕像", PM_CHROMATIC_DRAGON, NEUTRAL, FIGURINE },
    { "彩色龙罐头", PM_CHROMATIC_DRAGON, NEUTRAL, TIN },
    { "彩色龙的罐头", PM_CHROMATIC_DRAGON, NEUTRAL, TIN },
    { "彩色龙肉罐头", PM_CHROMATIC_DRAGON, NEUTRAL, TIN },
    { "哥布林王雕像", PM_GOBLIN_KING, NEUTRAL, STATUE },
    { "哥布林王的雕像", PM_GOBLIN_KING, NEUTRAL, STATUE },
    { "哥布林王小雕像", PM_GOBLIN_KING, NEUTRAL, FIGURINE },
    { "哥布林王的小雕像", PM_GOBLIN_KING, NEUTRAL, FIGURINE },
    { "哥布林王罐头", PM_GOBLIN_KING, NEUTRAL, TIN },
    { "哥布林王的罐头", PM_GOBLIN_KING, NEUTRAL, TIN },
    { "哥布林王肉罐头", PM_GOBLIN_KING, NEUTRAL, TIN },
    { "独眼巨人雕像", PM_CYCLOPS, NEUTRAL, STATUE },
    { "独眼巨人的雕像", PM_CYCLOPS, NEUTRAL, STATUE },
    { "独眼巨人小雕像", PM_CYCLOPS, NEUTRAL, FIGURINE },
    { "独眼巨人的小雕像", PM_CYCLOPS, NEUTRAL, FIGURINE },
    { "独眼巨人罐头", PM_CYCLOPS, NEUTRAL, TIN },
    { "独眼巨人的罐头", PM_CYCLOPS, NEUTRAL, TIN },
    { "独眼巨人肉罐头", PM_CYCLOPS, NEUTRAL, TIN },
    { "恶龙雕像", PM_IXOTH, NEUTRAL, STATUE },
    { "恶龙的雕像", PM_IXOTH, NEUTRAL, STATUE },
    { "恶龙小雕像", PM_IXOTH, NEUTRAL, FIGURINE },
    { "恶龙的小雕像", PM_IXOTH, NEUTRAL, FIGURINE },
    { "恶龙罐头", PM_IXOTH, NEUTRAL, TIN },
    { "恶龙的罐头", PM_IXOTH, NEUTRAL, TIN },
    { "恶龙肉罐头", PM_IXOTH, NEUTRAL, TIN },
    { "凯恩大师雕像", PM_MASTER_KAEN, NEUTRAL, STATUE },
    { "凯恩大师的雕像", PM_MASTER_KAEN, NEUTRAL, STATUE },
    { "凯恩大师小雕像", PM_MASTER_KAEN, NEUTRAL, FIGURINE },
    { "凯恩大师的小雕像", PM_MASTER_KAEN, NEUTRAL, FIGURINE },
    { "凯恩大师罐头", PM_MASTER_KAEN, NEUTRAL, TIN },
    { "凯恩大师的罐头", PM_MASTER_KAEN, NEUTRAL, TIN },
    { "凯恩大师肉罐头", PM_MASTER_KAEN, NEUTRAL, TIN },
    { "纳宗魔雕像", PM_NALZOK, NEUTRAL, STATUE },
    { "纳宗魔的雕像", PM_NALZOK, NEUTRAL, STATUE },
    { "纳宗魔小雕像", PM_NALZOK, NEUTRAL, FIGURINE },
    { "纳宗魔的小雕像", PM_NALZOK, NEUTRAL, FIGURINE },
    { "纳宗魔罐头", PM_NALZOK, NEUTRAL, TIN },
    { "纳宗魔的罐头", PM_NALZOK, NEUTRAL, TIN },
    { "纳宗魔肉罐头", PM_NALZOK, NEUTRAL, TIN },
    { "蝎弩雕像", PM_SCORPIUS, NEUTRAL, STATUE },
    { "蝎弩的雕像", PM_SCORPIUS, NEUTRAL, STATUE },
    { "蝎弩小雕像", PM_SCORPIUS, NEUTRAL, FIGURINE },
    { "蝎弩的小雕像", PM_SCORPIUS, NEUTRAL, FIGURINE },
    { "蝎弩罐头", PM_SCORPIUS, NEUTRAL, TIN },
    { "蝎弩的罐头", PM_SCORPIUS, NEUTRAL, TIN },
    { "蝎弩肉罐头", PM_SCORPIUS, NEUTRAL, TIN },
    { "刺客大师雕像", PM_MASTER_ASSASSIN, NEUTRAL, STATUE },
    { "刺客大师的雕像", PM_MASTER_ASSASSIN, NEUTRAL, STATUE },
    { "刺客大师小雕像", PM_MASTER_ASSASSIN, NEUTRAL, FIGURINE },
    { "刺客大师的小雕像", PM_MASTER_ASSASSIN, NEUTRAL, FIGURINE },
    { "刺客大师罐头", PM_MASTER_ASSASSIN, NEUTRAL, TIN },
    { "刺客大师的罐头", PM_MASTER_ASSASSIN, NEUTRAL, TIN },
    { "刺客大师肉罐头", PM_MASTER_ASSASSIN, NEUTRAL, TIN },
    { "足利尊氏雕像", PM_ASHIKAGA_TAKAUJI, NEUTRAL, STATUE },
    { "足利尊氏的雕像", PM_ASHIKAGA_TAKAUJI, NEUTRAL, STATUE },
    { "足利尊氏小雕像", PM_ASHIKAGA_TAKAUJI, NEUTRAL, FIGURINE },
    { "足利尊氏的小雕像", PM_ASHIKAGA_TAKAUJI, NEUTRAL, FIGURINE },
    { "足利尊氏罐头", PM_ASHIKAGA_TAKAUJI, NEUTRAL, TIN },
    { "足利尊氏的罐头", PM_ASHIKAGA_TAKAUJI, NEUTRAL, TIN },
    { "足利尊氏肉罐头", PM_ASHIKAGA_TAKAUJI, NEUTRAL, TIN },
    { "叙尔特领主雕像", PM_LORD_SURTUR, NEUTRAL, STATUE },
    { "叙尔特领主的雕像", PM_LORD_SURTUR, NEUTRAL, STATUE },
    { "叙尔特领主小雕像", PM_LORD_SURTUR, NEUTRAL, FIGURINE },
    { "叙尔特领主的小雕像", PM_LORD_SURTUR, NEUTRAL, FIGURINE },
    { "叙尔特领主罐头", PM_LORD_SURTUR, NEUTRAL, TIN },
    { "叙尔特领主的罐头", PM_LORD_SURTUR, NEUTRAL, TIN },
    { "叙尔特领主肉罐头", PM_LORD_SURTUR, NEUTRAL, TIN },
    { "苏尔特尔领主雕像", PM_LORD_SURTUR, NEUTRAL, STATUE },
    { "苏尔特尔领主的雕像", PM_LORD_SURTUR, NEUTRAL, STATUE },
    { "苏尔特尔领主小雕像", PM_LORD_SURTUR, NEUTRAL, FIGURINE },
    { "苏尔特尔领主的小雕像", PM_LORD_SURTUR, NEUTRAL, FIGURINE },
    { "苏尔特尔领主罐头", PM_LORD_SURTUR, NEUTRAL, TIN },
    { "苏尔特尔领主的罐头", PM_LORD_SURTUR, NEUTRAL, TIN },
    { "苏尔特尔领主肉罐头", PM_LORD_SURTUR, NEUTRAL, TIN },
    { "苏尔特领主雕像", PM_LORD_SURTUR, NEUTRAL, STATUE },
    { "苏尔特领主的雕像", PM_LORD_SURTUR, NEUTRAL, STATUE },
    { "苏尔特领主小雕像", PM_LORD_SURTUR, NEUTRAL, FIGURINE },
    { "苏尔特领主的小雕像", PM_LORD_SURTUR, NEUTRAL, FIGURINE },
    { "苏尔特领主罐头", PM_LORD_SURTUR, NEUTRAL, TIN },
    { "苏尔特领主的罐头", PM_LORD_SURTUR, NEUTRAL, TIN },
    { "苏尔特领主肉罐头", PM_LORD_SURTUR, NEUTRAL, TIN },
    { "黑暗魔君雕像", PM_DARK_ONE, NEUTRAL, STATUE },
    { "黑暗魔君的雕像", PM_DARK_ONE, NEUTRAL, STATUE },
    { "黑暗魔君小雕像", PM_DARK_ONE, NEUTRAL, FIGURINE },
    { "黑暗魔君的小雕像", PM_DARK_ONE, NEUTRAL, FIGURINE },
    { "黑暗魔君罐头", PM_DARK_ONE, NEUTRAL, TIN },
    { "黑暗魔君的罐头", PM_DARK_ONE, NEUTRAL, TIN },
    { "黑暗魔君肉罐头", PM_DARK_ONE, NEUTRAL, TIN },
    { "学者雕像", PM_STUDENT, NEUTRAL, STATUE },
    { "学者的雕像", PM_STUDENT, NEUTRAL, STATUE },
    { "学者小雕像", PM_STUDENT, NEUTRAL, FIGURINE },
    { "学者的小雕像", PM_STUDENT, NEUTRAL, FIGURINE },
    { "学者罐头", PM_STUDENT, NEUTRAL, TIN },
    { "学者的罐头", PM_STUDENT, NEUTRAL, TIN },
    { "学者肉罐头", PM_STUDENT, NEUTRAL, TIN },
    { "学生雕像", PM_STUDENT, NEUTRAL, STATUE },
    { "学生的雕像", PM_STUDENT, NEUTRAL, STATUE },
    { "学生小雕像", PM_STUDENT, NEUTRAL, FIGURINE },
    { "学生的小雕像", PM_STUDENT, NEUTRAL, FIGURINE },
    { "学生罐头", PM_STUDENT, NEUTRAL, TIN },
    { "学生的罐头", PM_STUDENT, NEUTRAL, TIN },
    { "学生肉罐头", PM_STUDENT, NEUTRAL, TIN },
    { "酋长雕像", PM_CHIEFTAIN, NEUTRAL, STATUE },
    { "酋长的雕像", PM_CHIEFTAIN, NEUTRAL, STATUE },
    { "酋长小雕像", PM_CHIEFTAIN, NEUTRAL, FIGURINE },
    { "酋长的小雕像", PM_CHIEFTAIN, NEUTRAL, FIGURINE },
    { "酋长罐头", PM_CHIEFTAIN, NEUTRAL, TIN },
    { "酋长的罐头", PM_CHIEFTAIN, NEUTRAL, TIN },
    { "酋长肉罐头", PM_CHIEFTAIN, NEUTRAL, TIN },
    { "尼安德特人雕像", PM_NEANDERTHAL, NEUTRAL, STATUE },
    { "尼安德特人的雕像", PM_NEANDERTHAL, NEUTRAL, STATUE },
    { "尼安德特人小雕像", PM_NEANDERTHAL, NEUTRAL, FIGURINE },
    { "尼安德特人的小雕像", PM_NEANDERTHAL, NEUTRAL, FIGURINE },
    { "尼安德特人罐头", PM_NEANDERTHAL, NEUTRAL, TIN },
    { "尼安德特人的罐头", PM_NEANDERTHAL, NEUTRAL, TIN },
    { "尼安德特人肉罐头", PM_NEANDERTHAL, NEUTRAL, TIN },
    { "高等精灵雕像", PM_HIGH_EL, STATUEF},
    { "高等精灵的雕像", PM_HIGH_EL, STATUEF},
    { "高等精灵小雕像", PM_HIGH_EL, FIGURINEF},
    { "高等精灵的小雕像", PM_HIGH_EL, FIGURINEF},
    { "高等精灵罐头", PM_HIGH_EL, TINF},
    { "高等精灵的罐头", PM_HIGH_EL, TINF},
    { "高等精灵肉罐头", PM_HIGH_EL, TINF},
    { "护理者雕像", PM_ATTENDANT, NEUTRAL, STATUE },
    { "护理者的雕像", PM_ATTENDANT, NEUTRAL, STATUE },
    { "护理者小雕像", PM_ATTENDANT, NEUTRAL, FIGURINE },
    { "护理者的小雕像", PM_ATTENDANT, NEUTRAL, FIGURINE },
    { "护理者罐头", PM_ATTENDANT, NEUTRAL, TIN },
    { "护理者的罐头", PM_ATTENDANT, NEUTRAL, TIN },
    { "护理者肉罐头", PM_ATTENDANT, NEUTRAL, TIN },
    { "实习骑士雕像", PM_PAGE, NEUTRAL, STATUE },
    { "实习骑士的雕像", PM_PAGE, NEUTRAL, STATUE },
    { "实习骑士小雕像", PM_PAGE, NEUTRAL, FIGURINE },
    { "实习骑士的小雕像", PM_PAGE, NEUTRAL, FIGURINE },
    { "实习骑士罐头", PM_PAGE, NEUTRAL, TIN },
    { "实习骑士的罐头", PM_PAGE, NEUTRAL, TIN },
    { "实习骑士肉罐头", PM_PAGE, NEUTRAL, TIN },
    { "方丈雕像", PM_ABBOT, NEUTRAL, STATUE },
    { "方丈的雕像", PM_ABBOT, NEUTRAL, STATUE },
    { "方丈小雕像", PM_ABBOT, NEUTRAL, FIGURINE },
    { "方丈的小雕像", PM_ABBOT, NEUTRAL, FIGURINE },
    { "方丈罐头", PM_ABBOT, NEUTRAL, TIN },
    { "方丈的罐头", PM_ABBOT, NEUTRAL, TIN },
    { "方丈肉罐头", PM_ABBOT, NEUTRAL, TIN },
    { "侍祭雕像", PM_ACOLYTE, NEUTRAL, STATUE },
    { "侍祭的雕像", PM_ACOLYTE, NEUTRAL, STATUE },
    { "侍祭小雕像", PM_ACOLYTE, NEUTRAL, FIGURINE },
    { "侍祭的小雕像", PM_ACOLYTE, NEUTRAL, FIGURINE },
    { "侍祭罐头", PM_ACOLYTE, NEUTRAL, TIN },
    { "侍祭的罐头", PM_ACOLYTE, NEUTRAL, TIN },
    { "侍祭肉罐头", PM_ACOLYTE, NEUTRAL, TIN },
    { "猎人雕像", PM_HUNTER, NEUTRAL, STATUE },
    { "猎人的雕像", PM_HUNTER, NEUTRAL, STATUE },
    { "猎人小雕像", PM_HUNTER, NEUTRAL, FIGURINE },
    { "猎人的小雕像", PM_HUNTER, NEUTRAL, FIGURINE },
    { "猎人罐头", PM_HUNTER, NEUTRAL, TIN },
    { "猎人的罐头", PM_HUNTER, NEUTRAL, TIN },
    { "猎人肉罐头", PM_HUNTER, NEUTRAL, TIN },
    { "刺客雕像", PM_THUG, NEUTRAL, STATUE },
    { "刺客的雕像", PM_THUG, NEUTRAL, STATUE },
    { "刺客小雕像", PM_THUG, NEUTRAL, FIGURINE },
    { "刺客的小雕像", PM_THUG, NEUTRAL, FIGURINE },
    { "刺客罐头", PM_THUG, NEUTRAL, TIN },
    { "刺客的罐头", PM_THUG, NEUTRAL, TIN },
    { "刺客肉罐头", PM_THUG, NEUTRAL, TIN },
    { "忍者雕像", PM_NINJA, NEUTRAL, STATUE },
    { "忍者的雕像", PM_NINJA, NEUTRAL, STATUE },
    { "忍者小雕像", PM_NINJA, NEUTRAL, FIGURINE },
    { "忍者的小雕像", PM_NINJA, NEUTRAL, FIGURINE },
    { "忍者罐头", PM_NINJA, NEUTRAL, TIN },
    { "忍者的罐头", PM_NINJA, NEUTRAL, TIN },
    { "忍者肉罐头", PM_NINJA, NEUTRAL, TIN },
    { "禅师雕像", PM_ROSHI, NEUTRAL, STATUE },
    { "禅师的雕像", PM_ROSHI, NEUTRAL, STATUE },
    { "禅师小雕像", PM_ROSHI, NEUTRAL, FIGURINE },
    { "禅师的小雕像", PM_ROSHI, NEUTRAL, FIGURINE },
    { "禅师罐头", PM_ROSHI, NEUTRAL, TIN },
    { "禅师的罐头", PM_ROSHI, NEUTRAL, TIN },
    { "禅师肉罐头", PM_ROSHI, NEUTRAL, TIN },
    { "导游雕像", PM_GUIDE, NEUTRAL, STATUE },
    { "导游的雕像", PM_GUIDE, NEUTRAL, STATUE },
    { "导游小雕像", PM_GUIDE, NEUTRAL, FIGURINE },
    { "导游的小雕像", PM_GUIDE, NEUTRAL, FIGURINE },
    { "导游罐头", PM_GUIDE, NEUTRAL, TIN },
    { "导游的罐头", PM_GUIDE, NEUTRAL, TIN },
    { "导游肉罐头", PM_GUIDE, NEUTRAL, TIN },
    { "战士雕像", PM_WARRIOR, NEUTRAL, STATUE },
    { "战士的雕像", PM_WARRIOR, NEUTRAL, STATUE },
    { "战士小雕像", PM_WARRIOR, NEUTRAL, FIGURINE },
    { "战士的小雕像", PM_WARRIOR, NEUTRAL, FIGURINE },
    { "战士罐头", PM_WARRIOR, NEUTRAL, TIN },
    { "战士的罐头", PM_WARRIOR, NEUTRAL, TIN },
    { "战士肉罐头", PM_WARRIOR, NEUTRAL, TIN },
    { "魔法学徒雕像", PM_APPRENTICE, NEUTRAL, STATUE },
    { "魔法学徒的雕像", PM_APPRENTICE, NEUTRAL, STATUE },
    { "魔法学徒小雕像", PM_APPRENTICE, NEUTRAL, FIGURINE },
    { "魔法学徒的小雕像", PM_APPRENTICE, NEUTRAL, FIGURINE },
    { "魔法学徒罐头", PM_APPRENTICE, NEUTRAL, TIN },
    { "魔法学徒的罐头", PM_APPRENTICE, NEUTRAL, TIN },
    { "魔法学徒肉罐头", PM_APPRENTICE, NEUTRAL, TIN },
    { (const char *) 0, 0, 0 },
    { (const char *) 0, 0, 0 },
};
staticfn short
rnd_otyp_by_wpnskill(schar skill)
{
    int i, n = 0;
    short otyp = STRANGE_OBJECT;

    for (i = svb.bases[WEAPON_CLASS];
         i < NUM_OBJECTS && objects[i].oc_class == WEAPON_CLASS; i++)
        if (objects[i].oc_skill == skill) {
            n++;
            otyp = i;
        }
    if (n > 0) {
        n = rn2(n);
        for (i = svb.bases[WEAPON_CLASS];
             i < NUM_OBJECTS && objects[i].oc_class == WEAPON_CLASS; i++)
            if (objects[i].oc_skill == skill)
                if (--n < 0)
                    return i;
    }
    return otyp;
}

staticfn short
rnd_otyp_by_namedesc(
    const char *name,
    char oclass,
    int xtra_prob) /* add to item's chance of being chosen; non-zero causes
                    * 0% random generation items to also be considered */
{
    int i, n = 0;
    short validobjs[NUM_OBJECTS];
    const char *zn, *of;
    boolean check_of;
    int lo, hi, minglob, maxglob, prob, maxprob = 0;

    if (!name || !*name)
        return STRANGE_OBJECT;

    /* only skip "foo of" for "foo of bar" if target doesn't contain " of " */
    check_of = (strstri(name, " of ") == 0);
    minglob = GLOB_OF_GRAY_OOZE;
    maxglob = GLOB_OF_BLACK_PUDDING;

    (void) memset((genericptr_t) validobjs, 0, sizeof validobjs);
    if (oclass) {
        lo = svb.bases[(uchar) oclass];
        hi = svb.bases[(uchar) oclass + 1] - 1;
    } else {
        lo = MAXOCLASSES; /* STRANGE_OBJECT + 1; */
        hi = NUM_OBJECTS - 1;
    }
    /* FIXME:
     * When this spans classes (the !oclass case), the item
     * probabilities are not very useful because they don't take
     * the class generation probability into account.  [If 10%
     * of spellbooks were blank and 1% of scrolls were blank,
     * "blank" would have 10/11 chance to yield a book even though
     * scrolls are supposed to be much more common than books.]
     */
    for (i = lo; i <= hi; ++i) {
        /* don't match extra descriptions (w/o real name) */
        if ((zn = OBJ_NAME(objects[i])) == 0)
            continue;
        if (wishymatch(name, zn, TRUE) /* objects[] name */
            /* let "<bar>" match "<foo> of <bar>" (already does if foo is
               an object class, but this is for lump of royal jelly,
               clove of garlic, bag of tricks, &c) with a few exceptions:
               for "opening", don't match "bell of opening"; for monster
               type ooze/pudding/slime don't match glob of same since that
               ought to match "corpse/egg/figurine of type" too but won't */
            || (check_of
                && i != BELL_OF_OPENING
                && (i < minglob || i > maxglob)
                && (of = strstri(zn, " of ")) != 0
                && wishymatch(name, of + 4, FALSE)) /* partial name */
            || ((zn = OBJ_DESCR(objects[i])) != 0
                && wishymatch(name, zn, FALSE)) /* objects[] description */
            /* "cloth" should match "piece of cloth"; there's only one
               description containing " of " so no special case handling */
            || (zn && check_of && (of = strstri(zn, " of ")) != 0
                && wishymatch(name, of + 4, FALSE)) /* partial description */
            || ((zn = objects[i].oc_uname) != 0
                && wishymatch(name, zn, FALSE)) /* user-called name */
            ) {
            validobjs[n++] = (short) i;
            maxprob += (objects[i].oc_prob + xtra_prob);
        }
    }

    if (n > 0 && maxprob) {
        prob = rn2(maxprob);
        for (i = 0; i < n - 1; i++)
            if ((prob -= (objects[validobjs[i]].oc_prob + xtra_prob)) < 0)
                break;
        return validobjs[i];
    }
    return STRANGE_OBJECT;
}

staticfn short
rnd_otyp_by_enameedesc(
    const char *name,
    char oclass,
    int xtra_prob) /* add to item's chance of being chosen; non-zero causes
                    * 0% random generation items to also be considered */
{
    int i, n = 0;
    short validobjs[NUM_OBJECTS];
    const char *zn, *of;
    boolean check_of;
    int lo, hi, minglob, maxglob, prob, maxprob = 0;

    if (!name || !*name)
        return STRANGE_OBJECT;

    /* only skip "foo of" for "foo of bar" if target doesn't contain " of " */
    check_of = (strstri(name, " of ") == 0);
    minglob = GLOB_OF_GRAY_OOZE;
    maxglob = GLOB_OF_BLACK_PUDDING;

    (void) memset((genericptr_t) validobjs, 0, sizeof validobjs);
    if (oclass) {
        lo = svb.bases[(uchar) oclass];
        hi = svb.bases[(uchar) oclass + 1] - 1;
    } else {
        lo = MAXOCLASSES; /* STRANGE_OBJECT + 1; */
        hi = NUM_OBJECTS - 1;
    }
    /* FIXME:
     * When this spans classes (the !oclass case), the item
     * probabilities are not very useful because they don't take
     * the class generation probability into account.  [If 10%
     * of spellbooks were blank and 1% of scrolls were blank,
     * "blank" would have 10/11 chance to yield a book even though
     * scrolls are supposed to be much more common than books.]
     */
    for (i = lo; i <= hi; ++i) {
        /* don't match extra descriptions (w/o real name) */
        if ((zn = OBJ_ENAME(objects[i])) == 0)
            continue;
        if (wishyematch(name, zn, TRUE) /* objects[] name */
            /* let "<bar>" match "<foo> of <bar>" (already does if foo is
               an object class, but this is for lump of royal jelly,
               clove of garlic, bag of tricks, &c) with a few exceptions:
               for "opening", don't match "bell of opening"; for monster
               type ooze/pudding/slime don't match glob of same since that
               ought to match "corpse/egg/figurine of type" too but won't */
            || (check_of
                && i != BELL_OF_OPENING
                && (i < minglob || i > maxglob)
                && (of = strstri(zn, " of ")) != 0
                && wishyematch(name, of + 4, FALSE)) /* partial name */
            || ((zn = OBJ_DESCR(objects[i])) != 0
                && wishyematch(name, zn, FALSE)) /* objects[] description */
            /* "cloth" should match "piece of cloth"; there's only one
               description containing " of " so no special case handling */
            || (zn && check_of && (of = strstri(zn, " of ")) != 0
                && wishyematch(name, of + 4, FALSE)) /* partial description */
            || ((zn = objects[i].oc_uname) != 0
                && wishyematch(name, zn, FALSE)) /* user-called name */
            ) {
            validobjs[n++] = (short) i;
            maxprob += (objects[i].oc_prob + xtra_prob);
        }
    }

    if (n > 0 && maxprob) {
        prob = rn2(maxprob);
        for (i = 0; i < n - 1; i++)
            if ((prob -= (objects[validobjs[i]].oc_prob + xtra_prob)) < 0)
                break;
        return validobjs[i];
    }
    return STRANGE_OBJECT;
}

int
shiny_obj(char oclass)
{
    return (int) rnd_otyp_by_enameedesc("shiny", oclass, 0);
}

/* set wall under hero undiggable/unphaseable from string */
staticfn void
set_wallprop_from_str(char *bp)
{
    int wall_prop = 0;

    if (strstr(bp, "undiggable ") || strstr(bp, "nondiggable "))
        wall_prop |= W_NONDIGGABLE;
    if (strstr(bp, "unphaseable ") || strstr(bp, "nonpasswall "))
        wall_prop |= W_NONPASSWALL;
    /* |= because wall_info (aka flags) is overloaded with other stuff */
    if (wall_prop)
        levl[u.ux][u.uy].wall_info |= wall_prop;
}

/* in wizard mode, readobjnam() can accept wishes for traps and terrain */
staticfn struct obj *
wizterrainwish(struct _readobjnam_data *d)
{
    struct rm *lev;
    boolean madeterrain = FALSE, badterrain = FALSE, is_dbridge;
    int trap;
    unsigned oldtyp, ltyp;
    coordxy x = u.ux, y = u.uy;
    char *bp = d->bp, *p;

    for (trap = NO_TRAP + 1; trap < TRAPNUM; trap++) {
        struct trap *t;
        const char *tname;

        tname = trapname(trap, TRUE);
        if (!str_start_is(bp, tname, TRUE))
            continue;
        /* found it; avoid stupid mistakes */
        if (is_hole(trap) && !Can_fall_thru(&u.uz))
            trap = ROCKTRAP;
        if ((t = maketrap(x, y, trap)) != 0) {
            trap = t->ttyp;
            tname = trapname(trap, TRUE);
            pline("%s%s.", An(tname),
                  (trap != MAGIC_PORTAL) ? "" : "通往虚无");
        } else {
            pline("生成%s失败.", an(tname));
        }
        return &hands_obj;
    }

    /* furniture and terrain (use at your own risk; can clobber stairs
       or place furniture on existing traps which shouldn't be allowed) */
    lev = &levl[x][y];
    oldtyp = lev->typ;
    is_dbridge = (oldtyp == DRAWBRIDGE_DOWN || oldtyp == DRAWBRIDGE_UP);
    p = eos(bp);
    if (!BSTRCMPI(bp, p - 8, "fountain")) {
        lev->typ = FOUNTAIN;
        if (oldtyp != FOUNTAIN)
            svl.level.flags.nfountains++;
        lev->looted = d->looted ? F_LOOTED : 0; /* overlays 'flags' */
        lev->blessedftn = d->blessed || !strncmpi(bp, "magic ", 6);
        pline("一个%s喷泉.", lev->blessedftn ? "魔法" : "");
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 6, "throne")) {
        lev->typ = THRONE;
        lev->looted = d->looted ? T_LOOTED : 0; /* overlays 'flags' */
        pline("一个王座.");
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 4, "sink")) {
        lev->typ = SINK;
        if (oldtyp != SINK)
            svl.level.flags.nsinks++;
        lev->looted = d->looted ? (S_LPUDDING | S_LDWASHER | S_LRING) : 0;
        pline("一个水槽.");
        madeterrain = TRUE;

    /* ("water" matches "potion of water" rather than terrain) */
    } else if (!BSTRCMPI(bp, p - 4, "pool")
               || !BSTRCMPI(bp, p - 4, "moat")
               || !BSTRCMPI(bp, p - 13, "wall of water")) {
        long save_prop;
        const char *new_water;

        ltyp = !BSTRCMPI(bp, p - 4, "pool") ? POOL
               : !BSTRCMPI(bp, p - 4, "moat") ? MOAT
                 : WATER;
        if (!is_dbridge) {
            lev->typ = ltyp;
            lev->flags = 0;
        } else {
            /* drawbridgemask overloads flags */
            lev->drawbridgemask &= ~DB_UNDER;
            lev->drawbridgemask |= DB_MOAT;
        }
        del_engr_at(x, y);
        if (!is_dbridge) {
            save_prop = EHalluc_resistance;
            EHalluc_resistance = 1;
            new_water = waterbody_name(x, y);
            EHalluc_resistance = save_prop;
            pline("%s。", An(new_water));
            /* Must manually make kelp! */
        } else {
            dbterrainmesg("Moat", x, y);
        }
        water_damage_chain(svl.level.objects[x][y], TRUE);
        madeterrain = TRUE;

    /* also matches "molten lava" */
    } else if (!BSTRCMPI(bp, p - 4, "lava")
               || !BSTRCMPI(bp, p - 12, "wall of lava")) {
        ltyp = !BSTRCMPI(bp, p - 12, "wall of lava") ? LAVAWALL : LAVAPOOL;
        if (!is_dbridge) {
            lev->typ = ltyp;
            lev->flags = 0;
        } else {
            /* drawbridgemask overloads flags */
            lev->drawbridgemask &= ~DB_UNDER;
            lev->drawbridgemask |= DB_LAVA;
        }
        del_engr_at(x, y);
        if (!is_dbridge) {
            pline("一%s熔岩.",
                  (lev->typ == LAVAPOOL) ? "池" : "墙");
            if (!(Levitation || Flying) || lev->typ == LAVAWALL)
                pooleffects(FALSE);
        } else {
            dbterrainmesg("Lava", x, y);
        }
        fire_damage_chain(svl.level.objects[x][y], TRUE, TRUE, x, y);
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 3, "ice")) {
        if (!is_dbridge) {
            lev->typ = ICE;
            /* icedpool overloads flags; specifies what ice will melt into */
            lev->icedpool = (oldtyp == ROOM) ? ICED_POOL : ICED_MOAT;
        } else {
            /* drawbridgemask overloads flags */
            lev->drawbridgemask &= ~DB_UNDER;
            lev->drawbridgemask |= DB_ICE;
        }
        del_engr_at(x, y);

        if (!strncmpi(bp, "melting ", 8))
            start_melt_ice_timeout(x, y, 0L);

        if (!is_dbridge) {
            char icebuf[40];

            pline("%s。", upstart(ice_descr(x, y, icebuf)));
        } else {
            dbterrainmesg("Ice", x, y);
        }
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 5, "altar")) {
        aligntyp al;

        lev->typ = ALTAR;
        if (!strncmpi(bp, "chaotic ", 8))
            al = A_CHAOTIC;
        else if (!strncmpi(bp, "neutral ", 8))
            al = A_NEUTRAL;
        else if (!strncmpi(bp, "lawful ", 7))
            al = A_LAWFUL;
        else if (!strncmpi(bp, "unaligned ", 10))
            al = A_NONE;
        else /* -1 - A_CHAOTIC, 0 - A_NEUTRAL, 1 - A_LAWFUL */
            al = !rn2(6) ? A_NONE : (rn2((int) A_LAWFUL + 2) - 1);
        lev->altarmask = Align2amask(al); /* overlays 'flags' */
        pline("一个%s祭坛.", An(align_str(al)));
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 5, "grave")
               || !BSTRCMPI(bp, p - 9, "headstone")) {
        make_grave(x, y, (char *) 0);
        if (IS_GRAVE(lev->typ)) {
            lev->looted = 0; /* overlays 'flags' */
            lev->disturbed = d->looted ? 1 : 0;
            pline("一座%s墓地.", lev->disturbed ? "被扰动的 " : "");
            madeterrain = TRUE;
        } else {
            pline("无法在此处放置坟墓.");
            badterrain = TRUE;
        }
    } else if (!BSTRCMPI(bp, p - 4, "tree")) {
        lev->typ = TREE;
        lev->looted = d->looted ? (TREE_LOOTED | TREE_SWARM) : 0;
        set_wallprop_from_str(bp);
        pline("一棵树.");
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 4, "bars")) {
        lev->typ = IRONBARS;
        lev->flags = 0;
        set_wallprop_from_str(bp);
        /* [FIXME: if this isn't a wall or door location where 'horizontal'
            is already set up, that should be calculated for this spot.
            Unfortunately, it can be tricky; placing one in open space
            and then another adjacent might need to recalculate first one.] */
        pline("铁栅栏.");
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 5, "cloud")) {
        lev->typ = CLOUD;
        lev->flags = 0;
        pline("一朵云.");
        del_engr_at(x, y);
        madeterrain = TRUE;
    } else if (!BSTRCMPI(bp, p - 4, "door")
               || (d->doorless && !BSTRCMPI(bp, p - 7, "doorway"))) {
        char dbuf[40];
        unsigned old_wall_info;
        boolean secret = !BSTRCMPI(bp, p - 11, "secret door");

        /* require door or wall so that the 'horizontal' flag will
           already have the correct value; player might choose to put
           DOOR on top of existing DOOR or SDOOR on top of existing SDOOR
           to control its trapped state; iron bars are surrogate walls;
           a previously dug wall looks like corridor but is actually a
           doorless doorway so will be acceptable here */
        if (lev->typ == DOOR || lev->typ == SDOOR
            || (IS_WALL(lev->typ) && lev->typ != DBWALL)
            || lev->typ == IRONBARS) {
            /* remember previous wall info [is this right for iron bars?] */
            old_wall_info = (lev->typ != DOOR) ? lev->wall_info : 0;
            /* set the new terrain type */
            lev->typ = secret ? SDOOR : DOOR;
            lev->wall_info = 0; /* overlays 'flags' */
            /* lev->horizontal stays as-is */
            if (Is_rogue_level(&u.uz)) {
                /* all doors on the rogue level are doorless; locking magic
                   there converts them into walls rather than closed doors */
                d->doorless = 1;
                d->locked = d->closed = d->open = d->broken = 0;
            }
            /* if not locked, secret doors are implicitly closed but
               mustn't be set that way explicitly because they use both
               doormask and wall_info which both overload rm[x][y].flags
               (CLOSED overlaps wall_info bits, LOCKED and TRAPPED don't);
               conversion from SDOOR to DOOR changes NODOOR to CLOSED */
            lev->doormask = d->locked ? D_LOCKED
                            : (d->doorless || secret) ? D_NODOOR
                              : d->open ? D_ISOPEN
                                : d->broken ? D_BROKEN
                                  : D_CLOSED;
            /* SDOOR uses wall_info, restore relevant bits.
             * FIXME? if we're changing a regular door into a secret door,
             * old_wall_info bits will be 0 instead of being set properly.
             * Probably only matters if player uses Passes_walls and a wish
             * to turn a T- or cross-wall into a door, losing wall info,
             * and then another wish to turn that door into a secret door. */
            if (secret)
                lev->wall_info |= (old_wall_info & WM_MASK);
            /* set up trapped flag; open door states aren't eligible */
            if (d->trapped == 2 /* 2: wish includes explicit "untrapped" */
                || ((lev->doormask & (D_LOCKED | D_CLOSED)) == 0
                    /* D_CLOSED is implicit for secret doors */
                    && !secret))
                d->trapped = 0;
            if (d->trapped)
                lev->doormask |= D_TRAPPED;
            /* feedback */
            dbuf[0] = '\0';
            if (lev->doormask & D_TRAPPED)
                Strcat(dbuf, "陷阱 ");
            if (lev->doormask & D_LOCKED)
                Strcat(dbuf, "坏锁的");
            if (lev->typ == SDOOR) {
                Strcat(dbuf, "秘密门");
            } else {
                /* these should be mutually exclusive but we describe them
                   as if they're independent to maybe catch future bugs... */
                if (lev->doormask & D_CLOSED)
                    Strcat(dbuf, "关闭的");
                if (lev->doormask & D_ISOPEN)
                    Strcat(dbuf, "开着的");
                if (lev->doormask & D_BROKEN)
                    Strcat(dbuf, "坏锁的");
                if ((lev->doormask & ~D_TRAPPED) == D_NODOOR)
                    Strcat(dbuf, "无门的门洞");
                else
                    Strcat(dbuf, "门");
            }
            pline("%s.", upstart(an(dbuf)));
            madeterrain = TRUE;
        } else {
            Strcpy(dbuf, secret ? "秘密门" : "门");
            pline("%s需要门或墙的位置.", upstart(dbuf));
            badterrain = TRUE;
        }
    } else if (!BSTRCMPI(bp, p - 4, "wall")
                         && (bp == p - 4 || p[-5] == ' ')) {
        schar wall = HWALL;

        if ((isok(u.ux, u.uy-1) && IS_WALL(levl[u.ux][u.uy-1].typ))
            || (isok(u.ux, u.uy+1) && IS_WALL(levl[u.ux][u.uy+1].typ)))
            wall = VWALL;
        madeterrain = TRUE;
        lev->typ = wall;
        lev->flags = 0;
        set_wallprop_from_str(bp);
        fix_wall_spines(max(0,u.ux-1), max(0,u.uy-1),
                        min(COLNO,u.ux+1), min(ROWNO,u.uy+1));
        pline("一堵墙.");
    } else if (!BSTRCMPI(bp, p - 15, "secret corridor")) {
        if (lev->typ == CORR) {
            lev->typ = SCORR;
            /* neither CORR nor SCORR uses 'flags' or 'horizontal' */
            pline("秘密走廊.");
            madeterrain = TRUE;
        } else {
            pline("秘密走廊需要走廊位置.");
            badterrain = TRUE;
        }
    } else if (!BSTRCMPI(bp, p - 4, "room")
               || !BSTRCMPI(bp, p - 5, "floor")
               || !BSTRCMPI(bp, p - 6, "ground")) {
        if (oldtyp == ROOM
            || (IS_FURNITURE(oldtyp) && CAN_OVERWRITE_TERRAIN(oldtyp))
            || oldtyp == ICE || is_pool_or_lava(x, y)) {
            struct trap *t;

            lev->typ = ROOM;
            pline("房间地板.");
            if (IS_FURNITURE(oldtyp))
                count_level_features();
            if ((t = t_at(x, y)) != 0 && t->ttyp != MAGIC_PORTAL)
                deltrap(t);
            madeterrain = TRUE;
        } else if (is_dbridge) {
            lev->drawbridgemask &= ~DB_UNDER;
            lev->drawbridgemask |= DB_FLOOR;
            dbterrainmesg("Floor", x, y);
            madeterrain = TRUE;
        } else {
            pline("此处不允许房间,地板或地面.");
            badterrain = TRUE;
        }
    }

    if (madeterrain) {
        feel_newsym(x, y); /* map the spot where the wish occurred */

        /* hero started at <x,y> but might not be there anymore (create
           lava, decline to die, and get teleported away to safety) */
        if (u.uinwater && !is_pool(u.ux, u.uy)) {
            set_uinwater(0); /* u.uinwater = 0; leave the water */
            docrt();
            /* [block/unblock_point handled by docrt -> vision_recalc] */
        } else {
            if (u.utrap && u.utraptype == TT_LAVA && !is_lava(u.ux, u.uy))
                reset_utrap(FALSE);
            recalc_block_point(x, y);
        }

        /* fixups for replaced terrain that aren't handled above */
        if (IS_FOUNTAIN(oldtyp) || IS_SINK(oldtyp))
            count_level_features(); /* update level.flags.nfountains,nsinks */
        if (!is_ice(x, y))
            spot_stop_timers(x, y, MELT_ICE_AWAY);
        /* horizontal is overlaid by fountain->blessedftn, grave->disturbed */
        if (IS_FOUNTAIN(oldtyp) || IS_GRAVE(oldtyp)
            || IS_WALL(oldtyp) || oldtyp == IRONBARS
            || IS_DOOR(oldtyp) || oldtyp == SDOOR) {
            /* when new terrain is a fountain, 'blessedftn' was explicitly
               set above; likewise for grave and 'disturbed'; when it's a
               door, the old type was a wall or a door and we retain the
               'horizontal' value from those */
            if (!IS_FOUNTAIN(lev->typ) && !IS_GRAVE(lev->typ)
                && !IS_DOOR(lev->typ) && lev->typ != SDOOR)
                lev->horizontal = 0; /* also clears blessedftn, disturbed */
        }
        /* note: lev->lit and lev->nondiggable retain their values even
           though those might not make sense with the new terrain */

        /* might have changed terrain from something that blocked
           levitation and flying to something that doesn't (levitating
           while in xorn form and replacing solid stone with furniture) */
        switch_terrain();
    }
    if (madeterrain || badterrain)
        return &hands_obj;

    return (struct obj *) 0;
}

/* message common to several wizterrainwish() results */
staticfn void
dbterrainmesg(
    const char *newtype,
    coordxy x, coordxy y)
{
    pline("%s在吊桥%s.", newtype,
          (levl[x][y].typ == DRAWBRIDGE_UP) ? "前面" : "下面");
}

#define TIN_UNDEFINED 0
#define TIN_EMPTY 1
#define TIN_SPINACH 2

staticfn void
readobjnam_init(char *bp, struct _readobjnam_data *d)
{
    d->otmp = (struct obj *) 0;
    d->cnt = d->spe = d->spesgn = d->typ = 0;
    d->very = d->rechrg = d->blessed = d->uncursed = d->iscursed
        = d->ispoisoned = d->isgreased = d->eroded = d->eroded2
        = d->erodeproof = d->halfeaten = d->islit = d->unlabeled
        = d->ishistoric = d->isdiluted /* statues, potions */
          /* box/chest and wizard mode door */
        = d->trapped = d->locked = d->unlocked = d->broken
        = d->open = d->closed = d->doorless /* wizard mode door */
        = d->looted /* wizard mode fountain/sink/throne/tree and grave */
        = d->real = d->fake = 0; /* Amulet */
    d->tvariety = RANDOM_TIN;
    d->mgend = -1; /* not specified, aka random */
    d->mntmp = NON_PM;
    d->contents = TIN_UNDEFINED;
    d->oclass = 0;
    d->actualn = d->dn = d->un = 0;
    d->wetness = 0;
    d->gsize = 0;
    d->zombify = FALSE;
    d->bp = d->origbp = bp;
    d->p = (char *) 0;
    d->name = (const char *) 0;
    d->ftype = svc.context.current_fruit;
    (void) memset(d->globbuf, '\0', sizeof d->globbuf);
    (void) memset(d->fruitbuf, '\0', sizeof d->fruitbuf);
}

/* return 1 if d->bp is empty or contains only various qualifiers like
   "blessed", "rustproof", and so on, or 0 if anything else is present */
staticfn int
readobjnam_preparse(struct _readobjnam_data *d)
{
    char *save_bp = 0;
    int more_l = 0, res = 1;

    for (;;) {
        int l;

        if (!d->bp || !*d->bp)
            break;
        res = 0;

        if (!strncmpi(d->bp, "an ", l = 3) || !strncmpi(d->bp, "a ", l = 2) ||
            !cnstrcmpi(d->bp, "一个", l)) {
            d->cnt = 1;
        } else if (!strncmpi(d->bp, "the ", l = 4)) {
            ; /* just increment `bp' by `l' below */
        } else if (!d->cnt && digit(*d->bp) && strcmp(d->bp, "0")) {
            d->cnt = atoi(d->bp);
            while (digit(*d->bp))
                d->bp++;
            while (*d->bp == ' ')
                d->bp++;
            l = 0;
        } else if (*d->bp == '+' || *d->bp == '-') {
            d->spesgn = (*d->bp++ == '+') ? 1 : -1;
            d->spe = atoi(d->bp);
            while (digit(*d->bp))
                d->bp++;
            while (*d->bp == ' ')
                d->bp++;
            l = 0;
        } else if (!strncmpi(d->bp, "blessed ", l = 8) || !strncmpi(d->bp, "holy ", l = 5) ||
            !cnstrcmpi(d->bp, "被祝福的", l) || !cnstrcmpi(d->bp, "受祝福的", l) || !cnstrcmpi(d->bp, "有祝福的", l) ||
            !cnstrcmpi(d->bp, "祝福的", l) || !cnstrcmpi(d->bp, "祝福", l) || !cnstrcmpi(d->bp, "圣", l)) {
            d->blessed = 1, d->uncursed = d->iscursed = 0;
        } else if (!strncmpi(d->bp, "cursed ", l = 7) || !strncmpi(d->bp, "unholy ", l = 7) || 
            !cnstrcmpi(d->bp, "被诅咒的", l) || !cnstrcmpi(d->bp, "受诅咒的", l) || !cnstrcmpi(d->bp, "有诅咒的", l) ||
            !cnstrcmpi(d->bp, "诅咒的", l) || !cnstrcmpi(d->bp, "诅咒", l) || !cnstrcmpi(d->bp, "邪", l)) {
            d->iscursed = 1, d->blessed = d->uncursed = 0;
        } else if (!strncmpi(d->bp, "uncursed ", l = 9) ||
            !cnstrcmpi(d->bp, "无诅咒的", l) || !cnstrcmpi(d->bp, "未诅咒的", l) || !cnstrcmpi(d->bp, "未受诅咒的", l) ||
            !cnstrcmpi(d->bp, "未被诅咒的", l) || !cnstrcmpi(d->bp, "没有诅咒的", l) || !cnstrcmpi(d->bp, "没诅咒的", l) ||
            !cnstrcmpi(d->bp, "无祝福的", l) || !cnstrcmpi(d->bp, "未祝福的", l) || !cnstrcmpi(d->bp, "未受祝福的", l) ||
            !cnstrcmpi(d->bp, "未被祝福的", l) || !cnstrcmpi(d->bp, "没有祝福的", l) || !cnstrcmpi(d->bp, "没祝福的", l) ||
            !cnstrcmpi(d->bp, "平凡的", l) || !cnstrcmpi(d->bp, "平庸的", l) || !cnstrcmpi(d->bp, "普通的", l) || !cnstrcmpi(d->bp, "一般的", l)
            ) {
            d->uncursed = 1, d->blessed = d->iscursed = 0;
        } else if (!strncmpi(d->bp, "rustproof ", l = 10) || 
                !cnstrcmpi(d->bp, "防锈的", l) || !cnstrcmpi(d->bp, "防生锈的", l) || !cnstrcmpi(d->bp, "防锈", l) ||
                !cnstrcmpi(d->bp, "耐锈的", l) || !cnstrcmpi(d->bp, "耐锈", l) ||
                !cnstrcmpi(d->bp, "抗锈的", l) || !cnstrcmpi(d->bp, "不怕锈的", l) || !cnstrcmpi(d->bp, "锈抗", l) ||
                !strncmpi(d->bp, "erodeproof ", l = 11) ||
                !cnstrcmpi(d->bp, "防腐蚀的", l) || !cnstrcmpi(d->bp, "防腐的", l) || !cnstrcmpi(d->bp, "防蚀的", l) ||
                !cnstrcmpi(d->bp, "防腐", l) || !cnstrcmpi(d->bp, "防蚀", l) || !cnstrcmpi(d->bp, "防腐蚀", l) ||
                !cnstrcmpi(d->bp, "耐腐蚀的", l) || !cnstrcmpi(d->bp, "耐腐的", l) || !cnstrcmpi(d->bp, "耐蚀的", l) ||
                !cnstrcmpi(d->bp, "耐腐蚀", l) ||
                !cnstrcmpi(d->bp, "抗腐蚀的", l) || !cnstrcmpi(d->bp, "抗腐的", l) || !cnstrcmpi(d->bp, "抗蚀的", l) ||
                !cnstrcmpi(d->bp, "抗腐", l) || !cnstrcmpi(d->bp, "抗蚀", l) || !cnstrcmpi(d->bp, "抗腐蚀", l) ||
                !cnstrcmpi(d->bp, "不怕腐蚀的", l) || !cnstrcmpi(d->bp, "腐抗", l) || !cnstrcmpi(d->bp, "蚀抗", l) ||
                !strncmpi(d->bp, "corrodeproof ", l = 13) ||
                !strncmpi(d->bp, "fixed ", l = 6) ||
                !cnstrcmpi(d->bp, "定形的", l) || !cnstrcmpi(d->bp, "定型的", l) ||
                !strncmpi(d->bp, "fireproof ", l = 10) ||
                !cnstrcmpi(d->bp, "防火的", l) || !cnstrcmpi(d->bp, "防火", l) ||
                !cnstrcmpi(d->bp, "耐火的", l) || !cnstrcmpi(d->bp, "耐火", l) ||
                !cnstrcmpi(d->bp, "抗火的", l) || !cnstrcmpi(d->bp, "抗火", l) || !cnstrcmpi(d->bp, "火抗", l) ||
                !cnstrcmpi(d->bp, "不怕火的", l) || !cnstrcmpi(d->bp, "不怕烧的", l) || !cnstrcmpi(d->bp, "不怕火烧的", l) ||
                !strncmpi(d->bp, "rotproof ", l = 9) ||
                !cnstrcmpi(d->bp, "抗腐烂的", l) || !cnstrcmpi(d->bp, "防腐烂的", l) ||
                !strncmpi(d->bp, "tempered ", l = 9) ||
                !cnstrcmpi(d->bp, "淬火的", l) || !cnstrcmpi(d->bp, "淬火过的", l) || !cnstrcmpi(d->bp, "淬火的", l) ||
                !strncmpi(d->bp, "crackproof ", l = 11) ||
                !cnstrcmpi(d->bp, "抗裂的", l) || !cnstrcmpi(d->bp, "不怕裂的", l) || !cnstrcmpi(d->bp, "裂抗", l)) {
            d->erodeproof = 1;
            d->eroded = 0;
            d->eroded = 0;
        } else if (!strncmpi(d->bp, "lit ", l = 4) || !strncmpi(d->bp, "burning ", l = 8) ||
            !cnstrcmpi(d->bp, "被点亮的", l) || !cnstrcmpi(d->bp, "被点燃的", l) || !cnstrcmpi(d->bp, "被点着的", l) ||
            !cnstrcmpi(d->bp, "点亮的", l) || !cnstrcmpi(d->bp, "点燃的", l) || !cnstrcmpi(d->bp, "点着的", l) || 
            !cnstrcmpi(d->bp, "烧着的", l) || !cnstrcmpi(d->bp, "燃烧的", l)) {
            d->islit = 1;
        } else if (!strncmpi(d->bp, "unlit ", l = 6) || !strncmpi(d->bp, "extinguished ", l = 13) ||
            !cnstrcmpi(d->bp, "未被点亮的", l) || !cnstrcmpi(d->bp, "未被点燃的", l) || !cnstrcmpi(d->bp, "未被点着的", l) ||
            !cnstrcmpi(d->bp, "未点亮的", l) || !cnstrcmpi(d->bp, "未点燃的", l) || !cnstrcmpi(d->bp, "未点着的", l) ||
            !cnstrcmpi(d->bp, "没点亮的", l) || !cnstrcmpi(d->bp, "没点燃的", l) || !cnstrcmpi(d->bp, "没点着的", l) ||
            !cnstrcmpi(d->bp, "灭掉了的", l) || !cnstrcmpi(d->bp, "灭掉的", l) || !cnstrcmpi(d->bp, "灭了的", l)) {
            d->islit = 0;

        /* "wet" and "moist" are only applicable for towels */
        } else if (!strncmpi(d->bp, "moist ", l = 6) || !strncmpi(d->bp, "wet ", l = 4) ||
            !cnstrcmpi(d->bp, "打湿的", l) || !cnstrcmpi(d->bp, "湿润的", l) || !cnstrcmpi(d->bp, "湿了的", l) || !cnstrcmpi(d->bp, "润湿的", l) ||
            !cnstrcmpi(d->bp, "蘸湿的", l) || !cnstrcmpi(d->bp, "湿的", l) || !cnstrcmpi(d->bp, "湿润", l)) {
            if (!cnstrcmpi(d->bp, "wet ", l) || !cnstrcmpi(d->bp, "蘸湿的", l) || !cnstrcmpi(d->bp, "湿的", l))
                d->wetness = 3 + rn2(3); /* 3..5 */
            else
                d->wetness = rnd(2); /* 1..2 */

        /* "unlabeled" and "blank" are synonymous */
        } else if (!strncmpi(d->bp, "unlabeled ", l = 10) || !strncmpi(d->bp, "unlabelled ", l = 11) || !strncmpi(d->bp, "blank ", l = 6) ||
            !cnstrcmpi(d->bp, "无标签的", l) || !cnstrcmpi(d->bp, "空白的", l) || !cnstrcmpi(d->bp, "空白", l) || !cnstrcmpi(d->bp, "白纸", l)) {
            d->unlabeled = 1;
        } else if (!strncmpi(d->bp, "poisoned ", l = 9) ||
            !cnstrcmpi(d->bp, "有毒的", l) || !cnstrcmpi(d->bp, "涂毒的", l) || !cnstrcmpi(d->bp, "带毒的", l) || !cnstrcmpi(d->bp, "上毒的", l) || !cnstrcmpi(d->bp, "涂了毒的", l)) {
            d->ispoisoned = 1;

        /* "trapped" recognized but not honored outside wizard mode */
        } else if (!strncmpi(d->bp, "trapped ", l = 8) ||
            !cnstrcmpi(d->bp, "有陷阱的", l)) {
            d->trapped = 0; /* undo any previous "untrapped" */
            if (wizard)
                d->trapped = 1;
        } else if (!strncmpi(d->bp, "untrapped ", l = 10) ||
            !cnstrcmpi(d->bp, "没有陷阱的", l) || !cnstrcmpi(d->bp, "无陷阱的", l)) {
            d->trapped = 2; /* not trapped */

        /* locked, unlocked, broken: box/chest lock states, also door states;
           open, closed, doorless: additional door states */
        } else if (!strncmpi(d->bp, "locked ", l = 7) ||
            !cnstrcmpi(d->bp, "上锁了的", l) || !cnstrcmpi(d->bp, "上了锁的", l) || !cnstrcmpi(d->bp, "上着锁的", l) || 
            !cnstrcmpi(d->bp, "上锁的", l) || !cnstrcmpi(d->bp, "锁着的", l) || !cnstrcmpi(d->bp, "上锁", l)) {
            d->locked = d->closed = 1,
                d->unlocked = d->broken = d->open = d->doorless = 0;
        } else if (!strncmpi(d->bp, "unlocked ", l = 9) ||
            !cnstrcmpi(d->bp, "解锁了的", l) || !cnstrcmpi(d->bp, "解锁的", l) || !cnstrcmpi(d->bp, "已解锁的", l) || !cnstrcmpi(d->bp, "开锁的", l)) {
            d->unlocked = d->closed = 1,
                d->locked = d->broken = d->open = d->doorless = 0;
        } else if (!strncmpi(d->bp, "broken ", l = 7) ||
            !cnstrcmpi(d->bp, "坏锁了的", l) || !cnstrcmpi(d->bp, "坏锁的", l)) {
            d->broken = 1,
                d->locked = d->unlocked = d->open = d->closed
                = d->doorless = 0;
        } else if (!strncmpi(d->bp, "open ", l = 5) ||
            !cnstrcmpi(d->bp, "打开的", l) || !cnstrcmpi(d->bp, "开着的", l) || !cnstrcmpi(d->bp, "开启的", l)) {
            d->open = 1,
                d->closed = d->locked = d->broken = d->doorless = 0;
        } else if (!strncmpi(d->bp, "closed ", l = 7) ||
            !cnstrcmpi(d->bp, "关上的", l) || !cnstrcmpi(d->bp, "关着的", l) || !cnstrcmpi(d->bp, "关了的", l) || !cnstrcmpi(d->bp, "关闭的", l)) {
            d->closed = 1,
                d->open = d->locked = d->broken = d->doorless = 0;
        } else if (!strncmpi(d->bp, "doorless ", l = 9) ||
            !cnstrcmpi(d->bp, "没有门的", l)) {
            d->doorless = 1,
                d->open = d->closed = d->locked = d->unlocked = d->broken = 0;
        /* looted: fountain/sink/throne/tree; disturbed: grave */
        } else if (!strncmpi(d->bp, "looted ", l = 7) ||
                !cnstrcmpi(d->bp, "被掠夺过的", l) ||
                   /* overload disturbed grave with looted fountain here
                      even though they're separate in struct rm */
                   !strncmpi(d->bp, "disturbed ", l = 10) ||
                !cnstrcmpi(d->bp, "被打扰的", l)) {
            d->looted = 1;
        } else if (!strncmpi(d->bp, "greased ", l = 8) ||
            !cnstrcmpi(d->bp, "上了油的", l) || !cnstrcmpi(d->bp, "上油的", l) || !cnstrcmpi(d->bp, "涂了油的", l) || !cnstrcmpi(d->bp, "涂油的", l)) {
            d->isgreased = 1;
        } else if (!strncmpi(d->bp, "zombifying ", l = 11) ||
            !cnstrcmpi(d->bp, "僵尸化的", l)) {
            d->zombify = TRUE;
        } else if (!strncmpi(d->bp, "very ", l = 5) ||
            !cnstrcmpi(d->bp, "非常", l) || !cnstrcmpi(d->bp, "重度", l)) {
            /* very rusted very heavy iron ball */
            d->very = 1;
        } else if (!strncmpi(d->bp, "thoroughly ", l = 11) ||
            !cnstrcmpi(d->bp, "完全", l) || !cnstrcmpi(d->bp, "彻底", l)) {
            d->very = 2;
        } else if (!strncmpi(d->bp, "rusty ", l = 6) ||
                    !strncmpi(d->bp, "rusted ", l = 7) ||
                    !cnstrcmpi(d->bp, "生锈了的", l) || !cnstrcmpi(d->bp, "生了锈的", l) || !cnstrcmpi(d->bp, "生锈的", l) ||
                    !strncmpi(d->bp, "burnt ", l = 6) ||
                    !strncmpi(d->bp, "burned ", l = 7) ||
                    !cnstrcmpi(d->bp, "烧坏的", l) || !cnstrcmpi(d->bp, "烧焦的", l) || !cnstrcmpi(d->bp, "被烧坏的", l) || !cnstrcmpi(d->bp, "被烧焦的", l) ||
                    !cnstrcmpi(d->bp, "烧坏了的", l) || !cnstrcmpi(d->bp, "烧焦了的", l) || !cnstrcmpi(d->bp, "被烧坏了的", l) || !cnstrcmpi(d->bp, "被烧焦了的", l) ||
                    !strncmpi(d->bp, "cracked ", l = 8) ||
                    !cnstrcmpi(d->bp, "碎裂的", l) || !cnstrcmpi(d->bp, "破裂的", l) || !cnstrcmpi(d->bp, "裂开的", l) || !cnstrcmpi(d->bp, "裂开了的", l)) {
            d->eroded = 1 + d->very;
            if (d->erodeproof)
            {
                d->eroded = 0;
            }
            d->very = 0;
        } else if (!strncmpi(d->bp, "corroded ", l = 9) ||
                    !cnstrcmpi(d->bp, "被腐蚀了的", l) || !cnstrcmpi(d->bp, "腐蚀了的", l) || !cnstrcmpi(d->bp, "被腐蚀的", l) || !cnstrcmpi(d->bp, "腐蚀的", l) ||
                    !strncmpi(d->bp, "rotted ", l = 7) ||
                    !cnstrcmpi(d->bp, "腐烂了的", l) || !cnstrcmpi(d->bp, "烂掉了的", l) || !cnstrcmpi(d->bp, "腐烂的", l) || !cnstrcmpi(d->bp, "烂掉的", l)
                    ) {
            d->eroded2 = 1 + d->very;
            if (d->erodeproof)
            {
                d->eroded2 = 0;
            }
            d->very = 0;
        } else if (!strncmpi(d->bp, "partly eaten ", l = 13) || !strncmpi(d->bp, "partially eaten ", l = 16) ||
            !cnstrcmpi(d->bp, "吃掉一部分的", l) || !cnstrcmpi(d->bp, "一部分吃掉的", l) || !cnstrcmpi(d->bp, "吃掉部分的", l) || !cnstrcmpi(d->bp, "部分吃掉的", l) ||
            !cnstrcmpi(d->bp, "吃了一部分的", l) || !cnstrcmpi(d->bp, "一部分吃了的", l) || !cnstrcmpi(d->bp, "吃了部分的", l) || !cnstrcmpi(d->bp, "部分吃了的", l) ||
            !cnstrcmpi(d->bp, "吃掉了一部分的", l) || !cnstrcmpi(d->bp, "一部分吃掉了的", l) || !cnstrcmpi(d->bp, "吃掉了部分的", l) || !cnstrcmpi(d->bp, "部分吃掉了的", l) ||
            !cnstrcmpi(d->bp, "部分食用的", l) || !cnstrcmpi(d->bp, "吃了一半的", l) ||
            !cnstrcmpi(d->bp, "被吃掉一部分的", l) || !cnstrcmpi(d->bp, "一部分被吃掉的", l) || !cnstrcmpi(d->bp, "被吃掉部分的", l) || !cnstrcmpi(d->bp, "部分被吃掉的", l) ||
            !cnstrcmpi(d->bp, "被吃了一部分的", l) || !cnstrcmpi(d->bp, "一部分被吃了的", l) || !cnstrcmpi(d->bp, "被吃了部分的", l) || !cnstrcmpi(d->bp, "部分被吃了的", l) ||
            !cnstrcmpi(d->bp, "被吃掉了一部分的", l) || !cnstrcmpi(d->bp, "一部分被吃掉了的", l) || !cnstrcmpi(d->bp, "被吃掉了部分的", l) || !cnstrcmpi(d->bp, "部分被吃掉了的", l) ||
            !cnstrcmpi(d->bp, "被部分食用的", l) || !cnstrcmpi(d->bp, "被吃了一半的", l)) {
            d->halfeaten = 1;
        } else if (!strncmpi(d->bp, "historic ", l = 9) ||
            !cnstrcmpi(d->bp, "有历史意义的", l) || !cnstrcmpi(d->bp, "历史感的", l) || !cnstrcmpi(d->bp, "历史的", l)) {
            d->ishistoric = 1;
        } else if (!strncmpi(d->bp, "diluted ", l = 8) ||
            !cnstrcmpi(d->bp, "被稀释了的", l) || !cnstrcmpi(d->bp, "稀释了的", l) || !cnstrcmpi(d->bp, "被稀释的", l) || !cnstrcmpi(d->bp, "稀释的", l)) {
            d->isdiluted = 1;
        } else if (!strncmpi(d->bp, "empty ", l = 6) ||
            !cnstrcmpi(d->bp, "空的", l) || !cnstrcmpi(d->bp, "空", l)) {
            d->contents = TIN_EMPTY;
        } else if (!strncmpi(d->bp, "small ", l = 6) ||
            !cnstrcmpi(d->bp, "小的", l) || !cnstrcmpi(d->bp, "小", l)) { /* glob sizes */
            /* "small" might be part of monster name (mimic, if wishing
               for its corpse) rather than prefix for glob size; when
               used for globs, it might be either "small glob of <foo>" or
               "small <foo> glob" and user might add 's' even though plural
               doesn't accomplish anything because globs don't stack */
            if ((strncmpi(d->bp + l, "glob", 4) && !strstri(d->bp + l, " glob")) ||
                !cnstrcmpi(d->bp, "布丁", l))
                break;
            d->gsize = 1;
        } else if (!strncmpi(d->bp, "medium ", l = 7) ||
                !cnstrcmpi(d->bp, "中等的", l) || !cnstrcmpi(d->bp, "中等", l) || !cnstrcmpi(d->bp, "中", l)) {
            /* 5.0: in 3.6, "medium" was only used during wishing and the
               mid-size glob had no adjective when formatted, but as of
               5.0, "medium" has become an explicit part of the name for
               combined globs of at least 5 individual ones (owt >= 100)
               and less than 15 (owt < 300) */
            d->gsize = 2;
        } else if (!strncmpi(d->bp, "large ", l = 6) ||
                !cnstrcmpi(d->bp, "大的", l) || !cnstrcmpi(d->bp, "大", l)) {
            /* "large" might be part of monster name (dog, cat, kobold,
               mimic) or object name (box, round shield) rather than
               prefix for glob size */
            if ((strncmpi(d->bp + l, "glob", 4) && !strstri(d->bp + l, " glob")) ||
                !cnstrcmpi(d->bp, "布丁", l))
                break;
            /* "very large " had "very " peeled off on previous iteration */
            d->gsize = (d->very != 1) ? 3 : 4;
        } else if (!strncmpi(d->bp, "real ", l = 5) ||
                !cnstrcmpi(d->bp, "真正的", l) || !cnstrcmpi(d->bp, "真的", l) || !cnstrcmpi(d->bp, "真", l)) {
            /* accept "real Amulet of Yendor" with "blessed" or "cursed"
               or useless "erodeproof" before or after "real" ... */
            d->real = 1; /* don't negate 'fake' here; "real fake amulet" and
                       * "fake real amulet" will both yield fake amulet
                       * (so will "real amulet" outside of wizard mode) */
        } else if (!strncmpi(d->bp, "fake ", l = 5) ||
                !cnstrcmpi(d->bp, "假冒的", l) || !cnstrcmpi(d->bp, "伪造的", l) || !cnstrcmpi(d->bp, "假的", l) || !cnstrcmpi(d->bp, "假", l)) {
            /* ... and "fake Amulet of Yendor" likewise */
            d->fake = 1, d->real = 0;
            /* ['real' isn't actually needed (unless we someday add
               "real gem" for random non-glass, non-stone)] */
        } else if (!strncmpi(d->bp, "female ", l = 7) ||
                !cnstrcmpi(d->bp, "女性的", l) || !cnstrcmpi(d->bp, "雌性的", l) ||
                !cnstrcmpi(d->bp, "女的", l)  || !cnstrcmpi(d->bp, "雌性", l)  || !cnstrcmpi(d->bp, "母的", l) ||
                !cnstrcmpi(d->bp, "女", l) || !cnstrcmpi(d->bp, "雌", l) || !cnstrcmpi(d->bp, "母", l)) {
            d->mgend = FEMALE;
            /* if after "corpse/statue/figurine of", remove from string */
            if (save_bp)
                strsubst(d->bp, "female ", ""), l = 0;
        } else if (!strncmpi(d->bp, "male ", l = 5) ||
                !cnstrcmpi(d->bp, "男性的", l) || !cnstrcmpi(d->bp, "雄性的", l) ||
                !cnstrcmpi(d->bp, "男的", l)  || !cnstrcmpi(d->bp, "雄性", l)  || !cnstrcmpi(d->bp, "公的", l) ||
                !cnstrcmpi(d->bp, "男", l) || !cnstrcmpi(d->bp, "雄", l) || !cnstrcmpi(d->bp, "公", l)) {
            d->mgend = MALE;
            if (save_bp)
                strsubst(d->bp, "male ", ""), l = 0;
        } else if (!strncmpi(d->bp, "neuter ", l = 7) ||
                !cnstrcmpi(d->bp, "中性的", l) || !cnstrcmpi(d->bp, "中性", l)) {
            d->mgend = NEUTRAL;
            if (save_bp)
                strsubst(d->bp, "neuter ", ""), l = 0;

        /*
         * Corpse/statue/figurine gender hack:  in order to accept
         * "statue of a female gnome ruler" for gnome queen we need
         * to recognize and skip over "statue of [a ]".  Otherwise
         * we would only accept "female gnome ruler statue" and the
         * viable but silly "female statue of a gnome ruler".
         */
        } else if ((!strncmpi(d->bp, "corpse ", l = 7)
                    || !strncmpi(d->bp, "statue ", l = 7)
                    || !strncmpi(d->bp, "figurine ", l = 9))
                   && !strncmpi(d->bp + l, "of ", more_l = 3)) {
            save_bp = d->bp; /* we'll backtrack to here later */
            l += more_l, more_l = 0;
            if (!strncmpi(d->bp + l, "a ", more_l = 2)
                || !strncmpi(d->bp + l, "an ", more_l = 3)
                || !strncmpi(d->bp + l, "the ", more_l = 4))
                l += more_l;
        } else {
            break;
        }
        d->bp += l;
    }
    if (save_bp)
        d->bp = save_bp;
    return res;
}

staticfn int
readobjenam_preparse(struct _readobjnam_data *d)
{
    char *save_bp = 0;
    int more_l = 0, res = 1;

    for (;;) {
        int l;

        if (!d->bp || !*d->bp)
            break;
        res = 0;

        if (!strncmpi(d->bp, "an ", l = 3) || !strncmpi(d->bp, "a ", l = 2)) {
            d->cnt = 1;
        } else if (!strncmpi(d->bp, "the ", l = 4)) {
            ; /* just increment `bp' by `l' below */
        } else if (!d->cnt && digit(*d->bp) && strcmp(d->bp, "0")) {
            d->cnt = atoi(d->bp);
            while (digit(*d->bp))
                d->bp++;
            while (*d->bp == ' ')
                d->bp++;
            l = 0;
        } else if (*d->bp == '+' || *d->bp == '-') {
            d->spesgn = (*d->bp++ == '+') ? 1 : -1;
            d->spe = atoi(d->bp);
            while (digit(*d->bp))
                d->bp++;
            while (*d->bp == ' ')
                d->bp++;
            l = 0;
        } else if (!strncmpi(d->bp, "blessed ", l = 8)
                   || !strncmpi(d->bp, "holy ", l = 5)) {
            d->blessed = 1, d->uncursed = d->iscursed = 0;
        } else if (!strncmpi(d->bp, "cursed ", l = 7)
                   || !strncmpi(d->bp, "unholy ", l = 7)) {
            d->iscursed = 1, d->blessed = d->uncursed = 0;
        } else if (!strncmpi(d->bp, "uncursed ", l = 9)) {
            d->uncursed = 1, d->blessed = d->iscursed = 0;
        } else if (!strncmpi(d->bp, "rustproof ", l = 10)
                   || !strncmpi(d->bp, "erodeproof ", l = 11)
                   || !strncmpi(d->bp, "corrodeproof ", l = 13)
                   || !strncmpi(d->bp, "fixed ", l = 6)
                   || !strncmpi(d->bp, "fireproof ", l = 10)
                   || !strncmpi(d->bp, "rotproof ", l = 9)
                   || !strncmpi(d->bp, "tempered ", l = 9)
                   || !strncmpi(d->bp, "crackproof ", l = 11)) {
            d->erodeproof = 1;
        } else if (!strncmpi(d->bp, "lit ", l = 4)
                   || !strncmpi(d->bp, "burning ", l = 8)) {
            d->islit = 1;
        } else if (!strncmpi(d->bp, "unlit ", l = 6)
                   || !strncmpi(d->bp, "extinguished ", l = 13)) {
            d->islit = 0;

        /* "wet" and "moist" are only applicable for towels */
        } else if (!strncmpi(d->bp, "moist ", l = 6)
                   || !strncmpi(d->bp, "wet ", l = 4)) {
            if (!strncmpi(d->bp, "wet ", 4))
                d->wetness = 3 + rn2(3); /* 3..5 */
            else
                d->wetness = rnd(2); /* 1..2 */

        /* "unlabeled" and "blank" are synonymous */
        } else if (!strncmpi(d->bp, "unlabeled ", l = 10)
                   || !strncmpi(d->bp, "unlabelled ", l = 11)
                   || !strncmpi(d->bp, "blank ", l = 6)) {
            d->unlabeled = 1;
        } else if (!strncmpi(d->bp, "poisoned ", l = 9)) {
            d->ispoisoned = 1;

        /* "trapped" recognized but not honored outside wizard mode */
        } else if (!strncmpi(d->bp, "trapped ", l = 8)) {
            d->trapped = 0; /* undo any previous "untrapped" */
            if (wizard)
                d->trapped = 1;
        } else if (!strncmpi(d->bp, "untrapped ", l = 10)) {
            d->trapped = 2; /* not trapped */

        /* locked, unlocked, broken: box/chest lock states, also door states;
           open, closed, doorless: additional door states */
        } else if (!strncmpi(d->bp, "locked ", l = 7)) {
            d->locked = d->closed = 1,
                d->unlocked = d->broken = d->open = d->doorless = 0;
        } else if (!strncmpi(d->bp, "unlocked ", l = 9)) {
            d->unlocked = d->closed = 1,
                d->locked = d->broken = d->open = d->doorless = 0;
        } else if (!strncmpi(d->bp, "broken ", l = 7)) {
            d->broken = 1,
                d->locked = d->unlocked = d->open = d->closed
                = d->doorless = 0;
        } else if (!strncmpi(d->bp, "open ", l = 5)) {
            d->open = 1,
                d->closed = d->locked = d->broken = d->doorless = 0;
        } else if (!strncmpi(d->bp, "closed ", l = 7)) {
            d->closed = 1,
                d->open = d->locked = d->broken = d->doorless = 0;
        } else if (!strncmpi(d->bp, "doorless ", l = 9)) {
            d->doorless = 1,
                d->open = d->closed = d->locked = d->unlocked = d->broken = 0;
        /* looted: fountain/sink/throne/tree; disturbed: grave */
        } else if (!strncmpi(d->bp, "looted ", l = 7)
                   /* overload disturbed grave with looted fountain here
                      even though they're separate in struct rm */
                   || !strncmpi(d->bp, "disturbed ", l = 10)) {
            d->looted = 1;
        } else if (!strncmpi(d->bp, "greased ", l = 8)) {
            d->isgreased = 1;
        } else if (!strncmpi(d->bp, "zombifying ", l = 11)) {
            d->zombify = TRUE;
        } else if (!strncmpi(d->bp, "very ", l = 5)) {
            /* very rusted very heavy iron ball */
            d->very = 1;
        } else if (!strncmpi(d->bp, "thoroughly ", l = 11)) {
            d->very = 2;
        } else if (!strncmpi(d->bp, "rusty ", l = 6)
                   || !strncmpi(d->bp, "rusted ", l = 7)
                   || !strncmpi(d->bp, "burnt ", l = 6)
                   || !strncmpi(d->bp, "burned ", l = 7)
                   || !strncmpi(d->bp, "cracked ", l = 8)) {
            d->eroded = 1 + d->very;
            d->very = 0;
        } else if (!strncmpi(d->bp, "corroded ", l = 9)
                   || !strncmpi(d->bp, "rotted ", l = 7)) {
            d->eroded2 = 1 + d->very;
            d->very = 0;
        } else if (!strncmpi(d->bp, "partly eaten ", l = 13)
                   || !strncmpi(d->bp, "partially eaten ", l = 16)) {
            d->halfeaten = 1;
        } else if (!strncmpi(d->bp, "historic ", l = 9)) {
            d->ishistoric = 1;
        } else if (!strncmpi(d->bp, "diluted ", l = 8)) {
            d->isdiluted = 1;
        } else if (!strncmpi(d->bp, "empty ", l = 6)) {
            d->contents = TIN_EMPTY;
        } else if (!strncmpi(d->bp, "small ", l = 6)) { /* glob sizes */
            /* "small" might be part of monster name (mimic, if wishing
               for its corpse) rather than prefix for glob size; when
               used for globs, it might be either "small glob of <foo>" or
               "small <foo> glob" and user might add 's' even though plural
               doesn't accomplish anything because globs don't stack */
            if (strncmpi(d->bp + l, "glob", 4) && !strstri(d->bp + l, " glob"))
                break;
            d->gsize = 1;
        } else if (!strncmpi(d->bp, "medium ", l = 7)) {
            /* 5.0: in 3.6, "medium" was only used during wishing and the
               mid-size glob had no adjective when formatted, but as of
               5.0, "medium" has become an explicit part of the name for
               combined globs of at least 5 individual ones (owt >= 100)
               and less than 15 (owt < 300) */
            d->gsize = 2;
        } else if (!strncmpi(d->bp, "large ", l = 6)) {
            /* "large" might be part of monster name (dog, cat, kobold,
               mimic) or object name (box, round shield) rather than
               prefix for glob size */
            if (strncmpi(d->bp + l, "glob", 4) && !strstri(d->bp + l, " glob"))
                break;
            /* "very large " had "very " peeled off on previous iteration */
            d->gsize = (d->very != 1) ? 3 : 4;
        } else if (!strncmpi(d->bp, "real ", l = 5)) {
            /* accept "real Amulet of Yendor" with "blessed" or "cursed"
               or useless "erodeproof" before or after "real" ... */
            d->real = 1; /* don't negate 'fake' here; "real fake amulet" and
                       * "fake real amulet" will both yield fake amulet
                       * (so will "real amulet" outside of wizard mode) */
        } else if (!strncmpi(d->bp, "fake ", l = 5)) {
            /* ... and "fake Amulet of Yendor" likewise */
            d->fake = 1, d->real = 0;
            /* ['real' isn't actually needed (unless we someday add
               "real gem" for random non-glass, non-stone)] */
        } else if (!strncmpi(d->bp, "female ", l = 7)) {
            d->mgend = FEMALE;
            /* if after "corpse/statue/figurine of", remove from string */
            if (save_bp)
                strsubst(d->bp, "female ", ""), l = 0;
        } else if (!strncmpi(d->bp, "male ", l = 5)) {
            d->mgend = MALE;
            if (save_bp)
                strsubst(d->bp, "male ", ""), l = 0;
        } else if (!strncmpi(d->bp, "neuter ", l = 7)) {
            d->mgend = NEUTRAL;
            if (save_bp)
                strsubst(d->bp, "neuter ", ""), l = 0;

        /*
         * Corpse/statue/figurine gender hack:  in order to accept
         * "statue of a female gnome ruler" for gnome queen we need
         * to recognize and skip over "statue of [a ]".  Otherwise
         * we would only accept "female gnome ruler statue" and the
         * viable but silly "female statue of a gnome ruler".
         */
        } else if ((!strncmpi(d->bp, "corpse ", l = 7)
                    || !strncmpi(d->bp, "statue ", l = 7)
                    || !strncmpi(d->bp, "figurine ", l = 9))
                   && !strncmpi(d->bp + l, "of ", more_l = 3)) {
            save_bp = d->bp; /* we'll backtrack to here later */
            l += more_l, more_l = 0;
            if (!strncmpi(d->bp + l, "a ", more_l = 2)
                || !strncmpi(d->bp + l, "an ", more_l = 3)
                || !strncmpi(d->bp + l, "the ", more_l = 4))
                l += more_l;
        } else {
            break;
        }
        d->bp += l;
    }
    if (save_bp)
        d->bp = save_bp;
    return res;
}

staticfn void
readobjnam_parse_charges(struct _readobjnam_data *d)
{
    if (strlen(d->bp) > 1 && (d->p = strrchr(d->bp, '(')) != 0) {
        boolean keeptrailingchars = TRUE;
        int idx = 0;

        if (d->p > d->bp && d->p[-1] == ' ')
            idx = -1;
        d->p[idx] = '\0'; /* terminate bp */
        ++d->p; /* advance past '(' */
        if (!strncmpi(d->p, "lit)", 4)) {
            d->islit = 1;
            d->p += 4 - 1; /* point at ')' */
        } else if (!strncmpi(d->p, "已点燃)", strlen("已点燃)"))) {
            d->islit = 1;
            d->p += strlen("已点燃)") - 1;
        } else {
            d->spe = atoi(d->p);
            while (digit(*d->p))
                d->p++;
            if (*d->p == ':') {
                d->p++;
                d->rechrg = d->spe;
                d->spe = atoi(d->p);
                while (digit(*d->p))
                    d->p++;
            }
            if (*d->p != ')') {
                d->spe = d->rechrg = 0;
                /* mis-matched parentheses; rest of string will be ignored
                 * [probably we should restore everything back to '('
                 * instead since it might be part of "named ..."]
                 */
                keeptrailingchars = FALSE;
            } else {
                d->spesgn = 1;
            }
        }
        if (keeptrailingchars) {
            char *pp = eos(d->bp);

            /* 'pp' points at 'pb's terminating '\0',
               'p' points at ')' and will be incremented past it */
            do {
                *pp++ = *++d->p;
            } while (*d->p);
        }
    }
    /*
     * otmp->spe is type schar, so we don't want spe to be any bigger or
     * smaller.  Also, spe should always be positive --some cheaters may
     * try to confuse atoi().
     */
    if (d->spe < 0) {
        d->spesgn = -1; /* cheaters get what they deserve */
        d->spe = abs(d->spe);
    }
    /* cap on obj->spe is independent of (and less than) SCHAR_LIM */
    if (d->spe > SPE_LIM)
        d->spe = SPE_LIM; /* slime mold uses d.ftype, so not affected */
    if (d->rechrg < 0 || d->rechrg > 7)
        d->rechrg = 7; /* recharge_limit */
}

staticfn void
readobjenam_parse_charges(struct _readobjnam_data *d)
{
    if (strlen(d->bp) > 1 && (d->p = strrchr(d->bp, '(')) != 0) {
        boolean keeptrailingchars = TRUE;
        int idx = 0;

        if (d->p > d->bp && d->p[-1] == ' ')
            idx = -1;
        d->p[idx] = '\0'; /* terminate bp */
        ++d->p; /* advance past '(' */
        if (!strncmpi(d->p, "lit)", 4)) {
            d->islit = 1;
            d->p += 4 - 1; /* point at ')' */
        } else {
            d->spe = atoi(d->p);
            while (digit(*d->p))
                d->p++;
            if (*d->p == ':') {
                d->p++;
                d->rechrg = d->spe;
                d->spe = atoi(d->p);
                while (digit(*d->p))
                    d->p++;
            }
            if (*d->p != ')') {
                d->spe = d->rechrg = 0;
                /* mis-matched parentheses; rest of string will be ignored
                 * [probably we should restore everything back to '('
                 * instead since it might be part of "named ..."]
                 */
                keeptrailingchars = FALSE;
            } else {
                d->spesgn = 1;
            }
        }
        if (keeptrailingchars) {
            char *pp = eos(d->bp);

            /* 'pp' points at 'pb's terminating '\0',
               'p' points at ')' and will be incremented past it */
            do {
                *pp++ = *++d->p;
            } while (*d->p);
        }
    }
    /*
     * otmp->spe is type schar, so we don't want spe to be any bigger or
     * smaller.  Also, spe should always be positive --some cheaters may
     * try to confuse atoi().
     */
    if (d->spe < 0) {
        d->spesgn = -1; /* cheaters get what they deserve */
        d->spe = abs(d->spe);
    }
    /* cap on obj->spe is independent of (and less than) SCHAR_LIM */
    if (d->spe > SPE_LIM)
        d->spe = SPE_LIM; /* slime mold uses d.ftype, so not affected */
    if (d->rechrg < 0 || d->rechrg > 7)
        d->rechrg = 7; /* recharge_limit */
}

staticfn int
readobjnam_postparse1(struct _readobjnam_data *d)
{
    int i;

    /* now we have the actual name, as delivered by xname, say
     *  green potions called whisky
     *  scrolls labeled "QWERTY"
     *  egg
     *  fortune cookies
     *  very heavy iron ball named hoei
     *  wand of wishing
     *  elven cloak
     */
    if ((d->p = strstri(d->bp, " named ")) != 0) {
        *d->p = 0;
        /* note: if 'name' is too long, oname() will truncate it */
        d->name = d->p + 7;
    }
    if ((d->p = strstri(d->bp, "名为")) != 0) {
        *d->p = 0;
        /* note: if 'name' is too long, oname() will truncate it */
        d->name = d->p + strlen("名为");
    }
    if ((d->p = strstri(d->bp, ",名为")) != 0) {
        *d->p = 0;
        /* note: if 'name' is too long, oname() will truncate it */
        d->name = d->p + strlen(",名为");
    }
    if ((d->p = strstri(d->bp, " called ")) != 0) {
        *d->p = 0;
        /* note: if 'un' is too long, obj lookup just won't match anything */
        d->un = d->p + 8;
        /* "helmet called telepathy" is not "helmet" (a specific type)
         * "shield called reflection" is not "shield" (a general type)
         */
        for (i = 0; i < SIZE(o_ranges); i++)
            if (!strcmpi(d->bp, o_ranges[i].name)) {
                d->oclass = o_ranges[i].oclass;
                return 1; /*goto srch;*/
            }
    }
    if ((d->p = strstri(d->bp, "被称为")) != 0) {
        *d->p = 0;
        /* note: if 'un' is too long, obj lookup just won't match anything */
        d->un = d->p + strlen("被称为");
        /* "helmet called telepathy" is not "helmet" (a specific type)
         * "shield called reflection" is not "shield" (a general type)
         */
        for (i = 0; i < SIZE(o_ranges); i++)
            if (!strcmpi(d->bp, o_ranges[i].name)) {
                d->oclass = o_ranges[i].oclass;
                return 1; /*goto srch;*/
            }
    }
    if ((d->p = strstri(d->bp, ",被称为")) != 0) {
        *d->p = 0;
        /* note: if 'un' is too long, obj lookup just won't match anything */
        d->un = d->p + strlen(",被称为");
        /* "helmet called telepathy" is not "helmet" (a specific type)
         * "shield called reflection" is not "shield" (a general type)
         */
        for (i = 0; i < SIZE(o_ranges); i++)
            if (!strcmpi(d->bp, o_ranges[i].name)) {
                d->oclass = o_ranges[i].oclass;
                return 1; /*goto srch;*/
            }
    }
    if ((d->p = strstri(d->bp, " labeled ")) != 0) {
        *d->p = 0;
        d->dn = d->p + 9;
    } else if ((d->p = strstri(d->bp, " labelled ")) != 0) {
        *d->p = 0;
        d->dn = d->p + 10;
    }
    if ((d->p = strstri(d->bp, "写着")) != 0) {
        *d->p = 0;
        d->dn = d->p + strlen("写着");
    }
    if ((d->p = strstri(d->bp, "上面写着")) != 0) {
        *d->p = 0;
        d->dn = d->p + strlen("上面写着");
    }
    if ((d->p = strstri(d->bp, "标签为")) != 0) {
        *d->p = 0;
        d->dn = d->p + strlen("标签为");
    }
    if ((d->p = strstri(d->bp, ",写着")) != 0) {
        *d->p = 0;
        d->dn = d->p + strlen(",写着");
    }
    if ((d->p = strstri(d->bp, ",上面写着")) != 0) {
        *d->p = 0;
        d->dn = d->p + strlen(",上面写着");
    }
    if ((d->p = strstri(d->bp, ",标签为")) != 0) {
        *d->p = 0;
        d->dn = d->p + strlen(",标签为");
    }
    if ((d->p = strstri(d->bp, " of spinach")) != 0) {
        *d->p = 0;
        d->contents = TIN_SPINACH;
    }
    if ((d->p = strstri(d->bp, "菠菜")) != 0) {
        *d->p = 0;
        d->contents = TIN_SPINACH;
    }
    /* real vs fake is only useful for wizard mode but we'll accept its
       parsing in normal play (result is never real Amulet for that case) */
    if ((d->p = strstri(d->bp, OBJ_DESCR(objects[AMULET_OF_YENDOR]))) != 0
        && (d->p == d->bp || d->p[-1] == ' ')) {
        char *s = d->bp;

        /* "Amulet of Yendor" matches two items, name of real Amulet
           and description of fake one; player can explicitly specify
           "real" to disambiguate, but not specifying "fake" achieves
           the same thing; "real" and "fake" are parsed above with other
           prefixes so that combinations like "blessed real" and "real
           blessed" work as expected; also accept partial specification
           of the full name of the fake; unlike the prefix recognition
           loop above, these have to be in the right order when more
           than one is present (similar to worthless glass gems below) */
        if (!strncmpi(s, "cheap ", 6))
            d->fake = 1, s += 6;
        if (!strncmpi(s, "plastic ", 8))
            d->fake = 1, s += 8;
        if (!strncmpi(s, "imitation ", 10))
            d->fake = 1, s += 10;
        nhUse(s); /* suppress potential assigned-but-not-used complaint */
        /* when 'fake' is True, it overrides 'real' if both were given;
           when it is False, force 'real' whether that was specified or not */
        d->real = !d->fake;
        d->typ = d->real ? AMULET_OF_YENDOR : FAKE_AMULET_OF_YENDOR;
        return 2; /*goto typfnd;*/
    }

    if ((d->p = strstri(d->bp, OBJ_DESCR(objects[AMULET_OF_YENDOR]))) != 0
        && (d->p == d->bp || d->p[-1] == ' ')) {
        char *s = d->bp;

        /* "Amulet of Yendor" matches two items, name of real Amulet
           and description of fake one; player can explicitly specify
           "real" to disambiguate, but not specifying "fake" achieves
           the same thing; "real" and "fake" are parsed above with other
           prefixes so that combinations like "blessed real" and "real
           blessed" work as expected; also accept partial specification
           of the full name of the fake; unlike the prefix recognition
           loop above, these have to be in the right order when more
           than one is present (similar to worthless glass gems below) */
        if (!strncmpi(s, "cheap ", 6))
            d->fake = 1, s += 6;
        if (!strncmpi(s, "plastic ", 8))
            d->fake = 1, s += 8;
        if (!strncmpi(s, "imitation ", 10))
            d->fake = 1, s += 10;
        if (!strncmpi(s, "假", strlen("假")) || !strncmpi(s, "伪", strlen("伪")) || !strncmpi(s, "仿", strlen("仿")))
            d->fake = 1, s += strlen("假");
        if (!strncmpi(s, "塑料", strlen("塑料")))
            d->fake = 1, s += strlen("塑料");
        nhUse(s); /* suppress potential assigned-but-not-used complaint */
        /* when 'fake' is True, it overrides 'real' if both were given;
           when it is False, force 'real' whether that was specified or not */
        d->real = !d->fake;
        d->typ = d->real ? AMULET_OF_YENDOR : FAKE_AMULET_OF_YENDOR;
        return 2; /*goto typfnd;*/
    }

    /*
     * Skip over "pair of ", "pairs of", "set of" and "sets of".
     *
     * Accept "3 pair of boots" as well as "3 pairs of boots".  It is
     * valid English either way.  See makeplural() for more on pair/pairs.
     *
     * We should only double count if the object in question is not
     * referred to as a "pair of".  E.g. We should double if the player
     * types "pair of spears", but not if the player types "pair of
     * lenses".  Luckily (?) all objects that are referred to as pairs
     * -- boots, gloves, and lenses -- are also not mergeable, so cnt is
     * ignored anyway.
     */
    if (!strncmpi(d->bp, "pair of ", 8)) {
        d->bp += 8;
        d->cnt *= 2;
    } else if (!strncmpi(d->bp, "pairs of ", 9)) {
        d->bp += 9;
        if (d->cnt > 1)
            d->cnt *= 2;
    } else if (!strncmpi(d->bp, "一双", strlen("一双"))) {
        d->bp += strlen("一双");
        d->cnt *= 2;
    } else if (!strncmpi(d->bp, "双", strlen("双"))) {
        d->bp += strlen("双");
        if (d->cnt > 1)
            d->cnt *= 2;
    } else if (!strncmpi(d->bp, "set of ", 7)) {
        d->bp += 7;
    } else if (!strncmpi(d->bp, "sets of ", 8)) {
        d->bp += 8;
    } else if (!strncmpi(d->bp, "一套", strlen("一套"))) {
        d->bp += strlen("一套");
    } else if (!strncmpi(d->bp, "套", strlen("套"))) {
        d->bp += strlen("套");
    } 

    /* Intercept pudding globs here; they're a valid wish target,
     * but we need them to not get treated like a corpse.
     * If a count is specified, it will be used to magnify weight
     * rather than to specify quantity (which is always 1 for globs).
     */
    i = (int) strlen(d->bp);
    d->p = (char *) 0;
    /* check for "glob", "<foo> glob", and "glob of <foo>" */
    if (!strcmpi(d->bp, "glob") || !BSTRCMPI(d->bp, d->bp + i - 5, " glob") /*危险:危险个毛线啊，我都不知道怎么改*/
        || !strcmpi(d->bp, "globs")
        || !BSTRCMPI(d->bp, d->bp + i - 6, " globs")
        || (d->p = strstri(d->bp, "glob of ")) != 0
        || (d->p = strstri(d->bp, "globs of ")) != 0 ||
        !strcmpi(d->bp, "团")
        ) {
        d->mntmp = name_to_mon(!d->p ? d->bp
                                     : (strstri(d->p, " of ") + 4), (int *) 0);
        /* if we didn't recognize monster type, pick a valid one at random */
        if (d->mntmp == NON_PM)
            d->mntmp = rn1(PM_BLACK_PUDDING - PM_GRAY_OOZE, PM_GRAY_OOZE);
        /* normally this would be done when makesingular() changes the value
           but canonical form here is already singular so that won't happen */
        if (d->cnt < 2 && strstri(d->bp, "globs"))
            d->cnt = 2; /* affects otmp->owt but not otmp->quan for globs */
        /* construct canonical spelling in case name_to_mon() recognized a
           variant (grey ooze) or player used inverted syntax (<foo> glob);
           if player has given a valid monster type but not valid glob type,
           object name lookup won't find it and wish attempt will fail */
        Sprintf(d->globbuf, "%s团", mons[d->mntmp].pmnames[NEUTRAL]);
        d->bp = d->globbuf;
        d->mntmp = NON_PM; /* not useful for "glob of <foo>" object lookup */
        d->oclass = FOOD_CLASS;
        d->actualn = d->bp, d->dn = 0;
        return 1; /*goto srch;*/
    } else {
        /*
         * Find corpse type using "of" (figurine of an orc, tin of orc meat)
         * Don't check if it's a wand or spellbook.
         * (avoid "wand/finger of death" confusion).
         * Don't match "ogre" or "giant" monster name inside alternate item
         * names "gauntlets of ogre power" and "gauntlets of giant strength"
         * (or the alternate spelling of those, "gloves of ...").
         */
        if (!strstri(d->bp, "wand ") && !strstri(d->bp, "spellbook ")
            && !strstri(d->bp, "gauntlets ") && !strstri(d->bp, "gloves ")
            && !strstri(d->bp, "finger ") && !strstri(d->bp, "魔杖")
            && !strstri(d->bp, "魔法书") && !strstri(d->bp, "拳套")
            && !strstri(d->bp, "手套")) {
            if ((d->p = strstri(d->bp, "tin of ")) != 0) {
                if (!strcmpi(d->p + 7, "spinach")) {
                    d->contents = TIN_SPINACH;
                    d->mntmp = NON_PM;
                } else {
                    d->tmp = tin_variety_txt(d->p + 7, &d->tinv);
                    d->tvariety = d->tinv;
                    d->mntmp = name_to_mon(d->p + 7 + d->tmp, &d->mgend);
                }
                d->typ = TIN;
                return 2; /*goto typfnd;*/
            } else if ((d->p = strstri(d->bp, "罐头")) != 0) {
                if (!strcmpi(d->p - strlen("菠菜罐头"), "菠菜")) {
                    d->contents = TIN_SPINACH;
                    d->mntmp = NON_PM;
                } else {
                    d->tmp = tin_variety_txt(d->p + 7, &d->tinv);
                    d->tvariety = d->tinv;
                    d->mntmp = name_to_mon(d->p + 7 + d->tmp, &d->mgend);
                }
                d->typ = TIN;
                return 2; /*goto typfnd;*/
            } else if ((d->p = strstri(d->bp, " of ")) != 0
                       && ((d->mntmp = name_to_mon(d->p + 4, &d->mgend))
                           >= LOW_PM))
                *d->p = 0;
        }
    }
    /* Find corpse type w/o "of" (red dragon scale mail, yeti corpse) */
    if (strncmpi(d->bp, "samurai sword", 13)  /* not the "samurai" monster! */
        && strncmpi(d->bp, "wizard lock", 11) /* not the "wizard" monster! */
        && strncmpi(d->bp, "death wand", 10)  /* 'of inversion', not Rider */
        && strncmpi(d->bp, "master key", 10)  /* not the "master" rank */
        && strncmpi(d->bp, "ninja-to", 8)     /* not the "ninja" rank */
        && strncmpi(d->bp, "magenta", 7)
        && strncmpi(d->bp, "武士刀", strlen("武士刀"))
        && strncmpi(d->bp, "武士剑", strlen("武士剑"))
        && strncmpi(d->bp, "武士长剑", strlen("武士长剑"))
        && strncmpi(d->bp, "巫师锁", strlen("巫师锁"))
        && strncmpi(d->bp, "巫师帽", strlen("巫师帽"))
        && strncmpi(d->bp, "死亡魔杖", strlen("死亡魔杖"))
        && strncmpi(d->bp, "忍者刀", strlen("忍者刀"))) {
        const char *rest = 0;

        if (d->mntmp < LOW_PM && strlen(d->bp) > 2
            && ((d->mntmp = name_to_monplus(d->bp, &rest, &d->mgend))
                >= LOW_PM)) {
            char *obp = d->bp;

            /* 'rest' is a pointer past the matching portion; if that was
               an alternate name or a rank title rather than the canonical
               monster name we wouldn't otherwise know how much to skip */
            d->bp = (char *) rest; /* cast away const */

            if (*d->bp == ' ') {
                d->bp++;
            } else if (!strncmpi(d->bp, "s ", 2)
                       || (d->bp > d->origbp
                           && !strncmpi(d->bp - 1, "s' ", 3))) {
                d->bp += 2;
            } else if (!strncmpi(d->bp, "es ", 3)
                       || !strncmpi(d->bp, "'s ", 3)) {
                d->bp += 3;
            } else if (!*d->bp && !d->actualn && !d->dn && !d->un
                       && !d->oclass) {
                /* no referent; they don't really mean a monster type */
                d->bp = obp;
                d->mntmp = NON_PM;
            }
        }
    }

    /* first change to singular if necessary */
    if (*d->bp
        /* we want "tricks" to match "bag of tricks" [rnd_otyp_by_namedesc()]
           but that wouldn't work if it gets singularized to "trick"
           ["tricks bag" matches whether or not this exception is present
           because singularize operates on "bag" and wishymatch()'s
           'of inversion' finds a match] */
        && strcmpi(d->bp, "tricks")
        /* an odd potential wish; fail rather than get a false match with
           "cloth" because it might yield a "cloth spellbook" rather than
           a "piece of cloth" cloak [maybe we should give random armor?] */
        && strcmpi(d->bp, "clothes")
        ) {
        char *sng = makesingular(d->bp);

        if (strcmp(d->bp, sng)) {
            if (d->cnt == 1)
                d->cnt = 2;
            Strcpy(d->bp, sng);
        }
    }

    /* Alternate spellings (pick-ax, silver sabre, &c) */
    {
        const struct alt_spellings *as = spellings;

        while (as->sp) {
            if (wishymatch(d->bp, as->sp, TRUE)) {
                d->typ = as->ob;
                return 2; /*goto typfnd;*/
            }
            as++;
        }
        /* can't use spellings list for this one due to shuffling */
        if (!strncmpi(d->bp, "grey spell", 10))
            *(d->bp + 2) = 'a';

        if ((d->p = strstri(d->bp, "armour")) != 0) {
            /* skip past "armo", then copy remainder beyond "u" */
            d->p += 4;
            while ((*d->p = *(d->p + 1)) != '\0')
                ++d->p; /* self terminating */
        }
    }

    /* Alternate spellings (pick-ax, silver sabre, &c) */
    {
        const struct figurine_spellings *fs = figurines;

        while (fs->sp) {
            if (wishymatch(d->bp, fs->sp, TRUE)) {
                d->typ = fs->whatitis;
                d->mgend = fs->itsgender;
                d->mntmp = fs->itsmonster;
                return 2; /*goto typfnd;*/
            }
            fs++;
        }
        /* can't use spellings list for this one due to shuffling */
        if (!strncmpi(d->bp, "grey spell", 10))
            *(d->bp + 2) = 'a';

        if ((d->p = strstri(d->bp, "armour")) != 0) {
            /* skip past "armo", then copy remainder beyond "u" */
            d->p += 4;
            while ((*d->p = *(d->p + 1)) != '\0')
                ++d->p; /* self terminating */
        }
    }

    /* dragon scales - assumes order of dragons */
    if (!strcmpi(d->bp, "scales") && d->mntmp >= PM_GRAY_DRAGON
        && d->mntmp <= PM_YELLOW_DRAGON) {
        d->typ = GRAY_DRAGON_SCALES + d->mntmp - PM_GRAY_DRAGON;
        d->mntmp = NON_PM; /* no monster */
        return 2; /*goto typfnd;*/
    }

    if (!strcmpi(d->bp, "鳞") && d->mntmp >= PM_GRAY_DRAGON
        && d->mntmp <= PM_YELLOW_DRAGON) {
        d->typ = GRAY_DRAGON_SCALES + d->mntmp - PM_GRAY_DRAGON;
        d->mntmp = NON_PM; /* no monster */
        return 2; /*goto typfnd;*/
    }

    d->p = eos(d->bp);
    if (!BSTRCMPI(d->bp, d->p - 10, "holy water")) {
        /* this isn't needed for "[un]holy water" because adjective parsing
           handles holy==blessed and unholy==cursed and leaves "water" for
           the object type, but it is needed for "potion of [un]holy water"
           since that parsing stops when it reaches "potion"; also, neither
           "holy water" nor "unholy water" is an actual type of potion */
        if (!BSTRNCMPI(d->bp, d->p - 10 - 2, "un", 2))
            d->iscursed = 1, d->blessed = d->uncursed = 0; /* unholy water */
        else
            d->blessed = 1, d->iscursed = d->uncursed = 0; /* holy water */
        d->typ = POT_WATER;
        return 2; /*goto typfnd;*/
    }
    if (!BSTRCMPI(d->bp, d->p - strlen("圣水"), "圣水")) {
        d->blessed = 1, d->iscursed = d->uncursed = 0; /* holy water */
        d->typ = POT_WATER;
        return 2; /*goto typfnd;*/
    }
    if (!BSTRCMPI(d->bp, d->p - strlen("邪水"), "邪水")) {
        d->iscursed = 1, d->blessed = d->uncursed = 0; /* holy water */
        d->typ = POT_WATER;
        return 2; /*goto typfnd;*/
    }
    /* accept "paperback" or "paperback book", reject "paperback spellbook" */
    if (!strncmpi(d->bp, "paperback", 9)) {
        char *dbp = d->bp + 9; /* just past "paperback" */

        if (!*dbp || !strncmpi(dbp, " book", 5)) {
            d->typ = SPE_NOVEL;
            return 2; /*goto typfnd;*/
        } else {
            d->otmp = (struct obj *) 0;
            return 3;
        }
    }
    if (d->unlabeled && !BSTRCMPI(d->bp, d->p - 6, "scroll")) {
        d->typ = SCR_BLANK_PAPER;
        return 2; /*goto typfnd;*/
    }
    if (d->unlabeled && !cnstrcmp(d->bp, "卷轴")) {
        d->typ = SCR_BLANK_PAPER;
        return 2; /*goto typfnd;*/
    }
    if (d->unlabeled && !BSTRCMPI(d->bp, d->p - 9, "spellbook")) {
        d->typ = SPE_BLANK_PAPER;
        return 2; /*goto typfnd;*/
    }
    if (d->unlabeled && !cnstrcmp(d->bp, "魔法书")) {
        d->typ = SPE_BLANK_PAPER;
        return 2; /*goto typfnd;*/
    }
    /* specific food rather than color of gem/potion/spellbook[/scales] */
    if (!BSTRCMPI(d->bp, d->p - 6, "orange") && d->mntmp == NON_PM) {
        d->typ = ORANGE;
        return 2; /*goto typfnd;*/
    }
    if (!cnbstrcmp(d->bp, d->p, "橙") && d->mntmp == NON_PM) {
        d->typ = ORANGE;
        return 2; /*goto typfnd;*/
    }
    /*
     * NOTE: Gold pieces are handled as objects nowadays, and therefore
     * this section should probably be reconsidered as well as the entire
     * gold/money concept.  Maybe we want to add other monetary units as
     * well in the future. (TH)
     */
    if (!BSTRCMPI(d->bp, d->p - 10, "gold piece")
        || !BSTRCMPI(d->bp, d->p - 7, "zorkmid")
        || !strcmpi(d->bp, "gold") || !strcmpi(d->bp, "money")
        || !strcmpi(d->bp, "coin") || *d->bp == GOLD_SYM
        || !cnbstrcmp(d->bp, d->p, "金币")
        || !strcmpi(d->bp, "块钱")) {
        if (d->cnt > 5000 && !wizard)
            d->cnt = 5000;
        else if (d->cnt < 1)
            d->cnt = 1;
        d->otmp = mksobj(GOLD_PIECE, FALSE, FALSE);
        d->otmp->quan = (long) d->cnt;
        d->otmp->owt = weight(d->otmp);
        disp.botl = TRUE;
        return 3; /*return otmp;*/
    }

    /* check for single character object class code ("/" for wand, &c) */
    if (strlen(d->bp) == 1 && (i = def_char_to_objclass(*d->bp)) < MAXOCLASSES
        && i > ILLOBJ_CLASS && (i != VENOM_CLASS || wizard)) {
        d->oclass = i;
        return 4; /*goto any;*/
    }

    /* Search for class names: XXXXX potion, scroll of XXXXX.
       Avoid false hits on, e.g., rings for "ring mail". */
    if (strncmpi(d->bp, "enchant ", 8)
        && strncmpi(d->bp, "destroy ", 8)
        && strncmpi(d->bp, "detect food", 11)
        && strncmpi(d->bp, "food detection", 14)
        && strncmpi(d->bp, "ring mail", 9)
        && strncmpi(d->bp, "studded leather armor", 21)
        && strncmpi(d->bp, "leather armor", 13)
        && strncmpi(d->bp, "tooled horn", 11)
        && strncmpi(d->bp, "food ration", 11)
        && strncmpi(d->bp, "meat ring", 9)
        && strncmpi(d->bp, "附魔", strlen("附魔"))
        && strncmpi(d->bp, "毁坏", strlen("毁坏"))
        && strncmpi(d->bp, "食物探测", strlen("食物探测"))
        && strncmpi(d->bp, "探测食物", strlen("探测食物"))
        && strncmpi(d->bp, "肉环", strlen("肉环")))
        for (i = 0; i < (int) (sizeof wrpsym); i++) {
            int j = Strlen(wrp[i]);

            /* check for "<class> [ of ] something" */
            if (!strncmpi(d->bp, wrp[i], j)) {
                d->oclass = wrpsym[i];
                if (d->oclass != AMULET_CLASS) {
                    d->bp += j;
                    if (!strncmpi(d->bp, " of ", 4))
                        d->actualn = d->bp + 4;
                    /* else if(*bp) ?? */
                } else
                    d->actualn = d->bp;
                return 1; /*goto srch;*/
            }
            /* check for "something <class>" */
            if (!BSTRCMPI(d->bp, d->p - j, wrp[i])) {
                d->oclass = wrpsym[i];
                /* for "foo amulet", leave the class name so that
                   wishymatch() can do "of inversion" to try matching
                   "amulet of foo"; other classes don't include their
                   class name in their full object names (where
                   "potion of healing" is just "healing", for instance) */
                if (d->oclass != AMULET_CLASS) {
                    d->p -= j;
                    *d->p = '\0';
                    if (d->p > d->bp && d->p[-1] == ' ')
                        d->p[-1] = '\0';
                } else {
                    int k, l;
                    char amubuf[BUFSZ];

                    /* amulet without "of"; convoluted wording but better a
                       special case that's handled than one that's missing */
                    if (!strncmpi(d->bp, "versus poison ", 14)) {
                        d->typ = AMULET_VERSUS_POISON;
                        return 2; /*goto typfnd;*/
                    }
                    /* check for "<shape> amulet"; strip off trailing
                       " amulet" for that w/o changing contents of d->bp */
                    l = (int) strlen(d->bp) - j;
                    if (l > 0 && d->bp[l - 1] == ' ')
                        l -= 1;
                    copynchars(amubuf, d->bp, min(l, (int) sizeof amubuf - 1));
                    k = rnd_otyp_by_namedesc(amubuf, AMULET_CLASS, 0);
                    if (k != STRANGE_OBJECT) {
                        d->typ = k;
                        return 2; /*goto typfnd;*/
                    }
                    k = rnd_otyp_by_enameedesc(amubuf, AMULET_CLASS, 0);
                    if (k != STRANGE_OBJECT) {
                        d->typ = k;
                        return 2; /*goto typfnd;*/
                    }
                }
                d->actualn = d->dn = d->bp;
                return 1; /*goto srch;*/
            }
        }

    /* Wishing in wizard mode can create traps and furniture.
     * Part I:  distinguish between trap and object for the two
     * types of traps which have corresponding objects:  bear trap
     * and land mine.  "beartrap" (object) and "bear trap" (trap)
     * have a difference in spelling which we used to exploit by
     * adding a special case in wishymatch(), but "land mine" is
     * spelled the same either way so needs different handing.
     * Since we need something else for land mine, we've dropped
     * the bear trap hack so that both are handled exactly the
     * same.  To get an armed trap instead of a disarmed object,
     * the player can prefix either the object name or the trap
     * name with "trapped " (which ordinarily applies to chests
     * and tins), or append something--anything at all except for
     * " object", but " trap" is suggested--to either the trap
     * name or the object name.
     */
    if (wizard && (!strncmpi(d->bp, "bear", 4)
                   || !strncmpi(d->bp, "land", 4))) {
        boolean beartrap = (lowc(*d->bp) == 'b');
        char *zp = d->bp + 4; /* skip "bear"/"land" */

        if (*zp == ' ')
            ++zp; /* embedded space is optional */
        if (!strncmpi(zp, beartrap ? "trap" : "mine", 4)) {
            zp += 4;
            if (d->trapped == 2 || !strcmpi(zp, " object")) {
                /* "untrapped <foo>" or "<foo> object" */
                d->typ = beartrap ? BEARTRAP : LAND_MINE;
                return 2; /*goto typfnd;*/
            } else if (d->trapped == 1 || *zp != '\0') {
                /* "trapped <foo>" or "<foo> trap" (actually "<foo>*") */
                /* use canonical trap spelling, skip object matching */
                Strcpy(d->bp, trapname(beartrap ? BEAR_TRAP : LANDMINE, TRUE));
                return 5; /*goto wiztrap;*/
            }
            /* [no prefix or suffix; we're going to end up matching
               the object name and getting a disarmed trap object] */
        }
    }

    return 0;
}


staticfn int
readobjenam_postparse1(struct _readobjnam_data *d)
{
    int i;

    /* now we have the actual name, as delivered by xname, say
     *  green potions called whisky
     *  scrolls labeled "QWERTY"
     *  egg
     *  fortune cookies
     *  very heavy iron ball named hoei
     *  wand of wishing
     *  elven cloak
     */
    if ((d->p = strstri(d->bp, " named ")) != 0) {
        *d->p = 0;
        /* note: if 'name' is too long, oname() will truncate it */
        d->name = d->p + 7;
    }
    if ((d->p = strstri(d->bp, " called ")) != 0) {
        *d->p = 0;
        /* note: if 'un' is too long, obj lookup just won't match anything */
        d->un = d->p + 8;
        /* "helmet called telepathy" is not "helmet" (a specific type)
         * "shield called reflection" is not "shield" (a general type)
         */
        for (i = 0; i < SIZE(o_ranges); i++)
            if (!strcmpi(d->bp, o_ranges[i].name)) {
                d->oclass = o_ranges[i].oclass;
                return 1; /*goto srch;*/
            }
    }
    if ((d->p = strstri(d->bp, " labeled ")) != 0) {
        *d->p = 0;
        d->dn = d->p + 9;
    } else if ((d->p = strstri(d->bp, " labelled ")) != 0) {
        *d->p = 0;
        d->dn = d->p + 10;
    }
    if ((d->p = strstri(d->bp, " of spinach")) != 0) {
        *d->p = 0;
        d->contents = TIN_SPINACH;
    }
    /* real vs fake is only useful for wizard mode but we'll accept its
       parsing in normal play (result is never real Amulet for that case) */
    if ((d->p = strstri(d->bp, OBJ_DESCR(objects[AMULET_OF_YENDOR]))) != 0
        && (d->p == d->bp || d->p[-1] == ' ')) {
        char *s = d->bp;

        /* "Amulet of Yendor" matches two items, name of real Amulet
           and description of fake one; player can explicitly specify
           "real" to disambiguate, but not specifying "fake" achieves
           the same thing; "real" and "fake" are parsed above with other
           prefixes so that combinations like "blessed real" and "real
           blessed" work as expected; also accept partial specification
           of the full name of the fake; unlike the prefix recognition
           loop above, these have to be in the right order when more
           than one is present (similar to worthless glass gems below) */
        if (!strncmpi(s, "cheap ", 6))
            d->fake = 1, s += 6;
        if (!strncmpi(s, "plastic ", 8))
            d->fake = 1, s += 8;
        if (!strncmpi(s, "imitation ", 10))
            d->fake = 1, s += 10;
        nhUse(s); /* suppress potential assigned-but-not-used complaint */
        /* when 'fake' is True, it overrides 'real' if both were given;
           when it is False, force 'real' whether that was specified or not */
        d->real = !d->fake;
        d->typ = d->real ? AMULET_OF_YENDOR : FAKE_AMULET_OF_YENDOR;
        return 2; /*goto typfnd;*/
    }

    /*
     * Skip over "pair of ", "pairs of", "set of" and "sets of".
     *
     * Accept "3 pair of boots" as well as "3 pairs of boots".  It is
     * valid English either way.  See makeplural() for more on pair/pairs.
     *
     * We should only double count if the object in question is not
     * referred to as a "pair of".  E.g. We should double if the player
     * types "pair of spears", but not if the player types "pair of
     * lenses".  Luckily (?) all objects that are referred to as pairs
     * -- boots, gloves, and lenses -- are also not mergeable, so cnt is
     * ignored anyway.
     */
    if (!strncmpi(d->bp, "pair of ", 8)) {
        d->bp += 8;
        d->cnt *= 2;
    } else if (!strncmpi(d->bp, "pairs of ", 9)) {
        d->bp += 9;
        if (d->cnt > 1)
            d->cnt *= 2;
    } else if (!strncmpi(d->bp, "set of ", 7)) {
        d->bp += 7;
    } else if (!strncmpi(d->bp, "sets of ", 8)) {
        d->bp += 8;
    }

    /* Intercept pudding globs here; they're a valid wish target,
     * but we need them to not get treated like a corpse.
     * If a count is specified, it will be used to magnify weight
     * rather than to specify quantity (which is always 1 for globs).
     */
    i = (int) strlen(d->bp);
    d->p = (char *) 0;
    /* check for "glob", "<foo> glob", and "glob of <foo>" */
    if (!strcmpi(d->bp, "glob") || !BSTRCMPI(d->bp, d->bp + i - 5, " glob")
        || !strcmpi(d->bp, "globs")
        || !BSTRCMPI(d->bp, d->bp + i - 6, " globs")
        || (d->p = strstri(d->bp, "glob of ")) != 0
        || (d->p = strstri(d->bp, "globs of ")) != 0) {
        d->mntmp = name_to_mon(!d->p ? d->bp
                                     : (strstri(d->p, " of ") + 4), (int *) 0);
        /* if we didn't recognize monster type, pick a valid one at random */
        if (d->mntmp == NON_PM)
            d->mntmp = rn1(PM_BLACK_PUDDING - PM_GRAY_OOZE, PM_GRAY_OOZE);
        /* normally this would be done when makesingular() changes the value
           but canonical form here is already singular so that won't happen */
        if (d->cnt < 2 && strstri(d->bp, "globs"))
            d->cnt = 2; /* affects otmp->owt but not otmp->quan for globs */
        /* construct canonical spelling in case name_to_mon() recognized a
           variant (grey ooze) or player used inverted syntax (<foo> glob);
           if player has given a valid monster type but not valid glob type,
           object name lookup won't find it and wish attempt will fail */
        Sprintf(d->globbuf, "%s团", mons[d->mntmp].pmnames[NEUTRAL]);
        d->bp = d->globbuf;
        d->mntmp = NON_PM; /* not useful for "glob of <foo>" object lookup */
        d->oclass = FOOD_CLASS;
        d->actualn = d->bp, d->dn = 0;
        return 1; /*goto srch;*/
    } else {
        /*
         * Find corpse type using "of" (figurine of an orc, tin of orc meat)
         * Don't check if it's a wand or spellbook.
         * (avoid "wand/finger of death" confusion).
         * Don't match "ogre" or "giant" monster name inside alternate item
         * names "gauntlets of ogre power" and "gauntlets of giant strength"
         * (or the alternate spelling of those, "gloves of ...").
         */
        if (!strstri(d->bp, "wand ") && !strstri(d->bp, "spellbook ")
            && !strstri(d->bp, "gauntlets ") && !strstri(d->bp, "gloves ")
            && !strstri(d->bp, "finger ")) {
            if ((d->p = strstri(d->bp, "tin of ")) != 0) {
                if (!strcmpi(d->p + 7, "spinach")) {
                    d->contents = TIN_SPINACH;
                    d->mntmp = NON_PM;
                } else {
                    d->tmp = tin_variety_txt(d->p + 7, &d->tinv);
                    d->tvariety = d->tinv;
                    d->mntmp = name_to_mon(d->p + 7 + d->tmp, &d->mgend);
                }
                d->typ = TIN;
                return 2; /*goto typfnd;*/
            } else if ((d->p = strstri(d->bp, " of ")) != 0
                       && ((d->mntmp = name_to_mon(d->p + 4, &d->mgend))
                           >= LOW_PM))
                *d->p = 0;
        }
    }
    /* Find corpse type w/o "of" (red dragon scale mail, yeti corpse) */
    if (strncmpi(d->bp, "samurai sword", 13)  /* not the "samurai" monster! */
        && strncmpi(d->bp, "wizard lock", 11) /* not the "wizard" monster! */
        && strncmpi(d->bp, "death wand", 10)  /* 'of inversion', not Rider */
        && strncmpi(d->bp, "master key", 10)  /* not the "master" rank */
        && strncmpi(d->bp, "ninja-to", 8)     /* not the "ninja" rank */
        && strncmpi(d->bp, "magenta", 7)) {   /* not the "mage" rank */
        const char *rest = 0;

        if (d->mntmp < LOW_PM && strlen(d->bp) > 2
            && ((d->mntmp = name_to_monplus(d->bp, &rest, &d->mgend))
                >= LOW_PM)) {
            char *obp = d->bp;

            /* 'rest' is a pointer past the matching portion; if that was
               an alternate name or a rank title rather than the canonical
               monster name we wouldn't otherwise know how much to skip */
            d->bp = (char *) rest; /* cast away const */

            if (*d->bp == ' ') {
                d->bp++;
            } else if (!strncmpi(d->bp, "s ", 2)
                       || (d->bp > d->origbp
                           && !strncmpi(d->bp - 1, "s' ", 3))) {
                d->bp += 2;
            } else if (!strncmpi(d->bp, "es ", 3)
                       || !strncmpi(d->bp, "'s ", 3)) {
                d->bp += 3;
            } else if (!*d->bp && !d->actualn && !d->dn && !d->un
                       && !d->oclass) {
                /* no referent; they don't really mean a monster type */
                d->bp = obp;
                d->mntmp = NON_PM;
            }
        }
    }

    /* first change to singular if necessary */
    if (*d->bp
        /* we want "tricks" to match "bag of tricks" [rnd_otyp_by_namedesc()]
           but that wouldn't work if it gets singularized to "trick"
           ["tricks bag" matches whether or not this exception is present
           because singularize operates on "bag" and wishymatch()'s
           'of inversion' finds a match] */
        && strcmpi(d->bp, "tricks")
        /* an odd potential wish; fail rather than get a false match with
           "cloth" because it might yield a "cloth spellbook" rather than
           a "piece of cloth" cloak [maybe we should give random armor?] */
        && strcmpi(d->bp, "clothes")
        ) {
        char *sng = makesingular(d->bp);

        if (strcmp(d->bp, sng)) {
            if (d->cnt == 1)
                d->cnt = 2;
            Strcpy(d->bp, sng);
        }
    }

    /* Alternate spellings (pick-ax, silver sabre, &c) */
    {
        const struct alt_spellings *as = spellings;

        while (as->sp) {
            if (wishymatch(d->bp, as->sp, TRUE)) {
                d->typ = as->ob;
                return 2; /*goto typfnd;*/
            }
            as++;
        }
        /* can't use spellings list for this one due to shuffling */
        if (!strncmpi(d->bp, "grey spell", 10))
            *(d->bp + 2) = 'a';

        if ((d->p = strstri(d->bp, "armour")) != 0) {
            /* skip past "armo", then copy remainder beyond "u" */
            d->p += 4;
            while ((*d->p = *(d->p + 1)) != '\0')
                ++d->p; /* self terminating */
        }
    }

    /* dragon scales - assumes order of dragons */
    if (!strcmpi(d->bp, "scales") && d->mntmp >= PM_GRAY_DRAGON
        && d->mntmp <= PM_YELLOW_DRAGON) {
        d->typ = GRAY_DRAGON_SCALES + d->mntmp - PM_GRAY_DRAGON;
        d->mntmp = NON_PM; /* no monster */
        return 2; /*goto typfnd;*/
    }

    d->p = eos(d->bp);
    if (!BSTRCMPI(d->bp, d->p - 10, "holy water")) {
        /* this isn't needed for "[un]holy water" because adjective parsing
           handles holy==blessed and unholy==cursed and leaves "water" for
           the object type, but it is needed for "potion of [un]holy water"
           since that parsing stops when it reaches "potion"; also, neither
           "holy water" nor "unholy water" is an actual type of potion */
        if (!BSTRNCMPI(d->bp, d->p - 10 - 2, "un", 2))
            d->iscursed = 1, d->blessed = d->uncursed = 0; /* unholy water */
        else
            d->blessed = 1, d->iscursed = d->uncursed = 0; /* holy water */
        d->typ = POT_WATER;
        return 2; /*goto typfnd;*/
    }
    /* accept "paperback" or "paperback book", reject "paperback spellbook" */
    if (!strncmpi(d->bp, "paperback", 9)) {
        char *dbp = d->bp + 9; /* just past "paperback" */

        if (!*dbp || !strncmpi(dbp, " book", 5)) {
            d->typ = SPE_NOVEL;
            return 2; /*goto typfnd;*/
        } else {
            d->otmp = (struct obj *) 0;
            return 3;
        }
    }
    if (d->unlabeled && !BSTRCMPI(d->bp, d->p - 6, "scroll")) {
        d->typ = SCR_BLANK_PAPER;
        return 2; /*goto typfnd;*/
    }
    if (d->unlabeled && !BSTRCMPI(d->bp, d->p - 9, "spellbook")) {
        d->typ = SPE_BLANK_PAPER;
        return 2; /*goto typfnd;*/
    }
    /* specific food rather than color of gem/potion/spellbook[/scales] */
    if (!BSTRCMPI(d->bp, d->p - 6, "orange") && d->mntmp == NON_PM) {
        d->typ = ORANGE;
        return 2; /*goto typfnd;*/
    }
    /*
     * NOTE: Gold pieces are handled as objects nowadays, and therefore
     * this section should probably be reconsidered as well as the entire
     * gold/money concept.  Maybe we want to add other monetary units as
     * well in the future. (TH)
     */
    if (!BSTRCMPI(d->bp, d->p - 10, "gold piece")
        || !BSTRCMPI(d->bp, d->p - 7, "zorkmid")
        || !strcmpi(d->bp, "gold") || !strcmpi(d->bp, "money")
        || !strcmpi(d->bp, "coin") || *d->bp == GOLD_SYM) {
        if (d->cnt > 5000 && !wizard)
            d->cnt = 5000;
        else if (d->cnt < 1)
            d->cnt = 1;
        d->otmp = mksobj(GOLD_PIECE, FALSE, FALSE);
        d->otmp->quan = (long) d->cnt;
        d->otmp->owt = weight(d->otmp);
        disp.botl = TRUE;
        return 3; /*return otmp;*/
    }

    /* check for single character object class code ("/" for wand, &c) */
    if (strlen(d->bp) == 1 && (i = def_char_to_objclass(*d->bp)) < MAXOCLASSES
        && i > ILLOBJ_CLASS && (i != VENOM_CLASS || wizard)) {
        d->oclass = i;
        return 4; /*goto any;*/
    }

    /* Search for class names: XXXXX potion, scroll of XXXXX.
       Avoid false hits on, e.g., rings for "ring mail". */
    if (strncmpi(d->bp, "enchant ", 8)
        && strncmpi(d->bp, "destroy ", 8)
        && strncmpi(d->bp, "detect food", 11)
        && strncmpi(d->bp, "food detection", 14)
        && strncmpi(d->bp, "ring mail", 9)
        && strncmpi(d->bp, "studded leather armor", 21)
        && strncmpi(d->bp, "leather armor", 13)
        && strncmpi(d->bp, "tooled horn", 11)
        && strncmpi(d->bp, "food ration", 11)
        && strncmpi(d->bp, "meat ring", 9))
        for (i = 0; i < (int) (sizeof wrpsym); i++) {
            int j = Strlen(wrp[i]);

            /* check for "<class> [ of ] something" */
            if (!strncmpi(d->bp, wrp[i], j)) {
                d->oclass = wrpsym[i];
                if (d->oclass != AMULET_CLASS) {
                    d->bp += j;
                    if (!strncmpi(d->bp, " of ", 4))
                        d->actualn = d->bp + 4;
                    /* else if(*bp) ?? */
                } else
                    d->actualn = d->bp;
                return 1; /*goto srch;*/
            }
            /* check for "something <class>" */
            if (!BSTRCMPI(d->bp, d->p - j, wrp[i])) {
                d->oclass = wrpsym[i];
                /* for "foo amulet", leave the class name so that
                   wishymatch() can do "of inversion" to try matching
                   "amulet of foo"; other classes don't include their
                   class name in their full object names (where
                   "potion of healing" is just "healing", for instance) */
                if (d->oclass != AMULET_CLASS) {
                    d->p -= j;
                    *d->p = '\0';
                    if (d->p > d->bp && d->p[-1] == ' ')
                        d->p[-1] = '\0';
                } else {
                    int k, l;
                    char amubuf[BUFSZ];

                    /* amulet without "of"; convoluted wording but better a
                       special case that's handled than one that's missing */
                    if (!strncmpi(d->bp, "versus poison ", 14)) {
                        d->typ = AMULET_VERSUS_POISON;
                        return 2; /*goto typfnd;*/
                    }
                    /* check for "<shape> amulet"; strip off trailing
                       " amulet" for that w/o changing contents of d->bp */
                    l = (int) strlen(d->bp) - j;
                    if (l > 0 && d->bp[l - 1] == ' ')
                        l -= 1;
                    copynchars(amubuf, d->bp, min(l, (int) sizeof amubuf - 1));
                    k = rnd_otyp_by_namedesc(amubuf, AMULET_CLASS, 0);
                    if (k != STRANGE_OBJECT) {
                        d->typ = k;
                        return 2; /*goto typfnd;*/
                    }
                }
                d->actualn = d->dn = d->bp;
                return 1; /*goto srch;*/
            }
        }

    /* Wishing in wizard mode can create traps and furniture.
     * Part I:  distinguish between trap and object for the two
     * types of traps which have corresponding objects:  bear trap
     * and land mine.  "beartrap" (object) and "bear trap" (trap)
     * have a difference in spelling which we used to exploit by
     * adding a special case in wishymatch(), but "land mine" is
     * spelled the same either way so needs different handing.
     * Since we need something else for land mine, we've dropped
     * the bear trap hack so that both are handled exactly the
     * same.  To get an armed trap instead of a disarmed object,
     * the player can prefix either the object name or the trap
     * name with "trapped " (which ordinarily applies to chests
     * and tins), or append something--anything at all except for
     * " object", but " trap" is suggested--to either the trap
     * name or the object name.
     */
    if (wizard && (!strncmpi(d->bp, "bear", 4) /*危险:摆烂了。。。。。。*/
                   || !strncmpi(d->bp, "land", 4))) {
        boolean beartrap = (lowc(*d->bp) == 'b');
        char *zp = d->bp + 4; /* skip "bear"/"land" */

        if (*zp == ' ')
            ++zp; /* embedded space is optional */
        if (!strncmpi(zp, beartrap ? "trap" : "mine", 4)) {
            zp += 4;
            if (d->trapped == 2 || !strcmpi(zp, " object")) {
                /* "untrapped <foo>" or "<foo> object" */
                d->typ = beartrap ? BEARTRAP : LAND_MINE;
                return 2; /*goto typfnd;*/
            } else if (d->trapped == 1 || *zp != '\0') {
                /* "trapped <foo>" or "<foo> trap" (actually "<foo>*") */
                /* use canonical trap spelling, skip object matching */
                Strcpy(d->bp, trapname(beartrap ? BEAR_TRAP : LANDMINE, TRUE));
                return 5; /*goto wiztrap;*/
            }
            /* [no prefix or suffix; we're going to end up matching
               the object name and getting a disarmed trap object] */
        }
    }

    return 0;
}

staticfn int
readobjnam_postparse2(struct _readobjnam_data *d)
{
    int i;

    /* "grey stone" check must be before general "stone" */
    for (i = 0; i < SIZE(o_ranges); i++)
        if (!strcmpi(d->bp, o_ranges[i].name)) {
            d->typ = rnd_class(o_ranges[i].f_o_range, o_ranges[i].l_o_range);
            return 2; /*goto typfnd;*/
        }

    if (!BSTRCMPI(d->bp, d->p - 6, " stone")
        || !BSTRCMPI(d->bp, d->p - 4, " gem")) {
        d->p[!strcmpi(d->p - 4, " gem") ? -4 : -6] = '\0';
        d->oclass = GEM_CLASS;
        d->dn = d->actualn = d->bp;
        return 1; /*goto srch;*/
    } else if (!BSTRCMPI(d->bp, d->p - strlen("石"), "石") || !BSTRCMPI(d->bp, d->p - strlen("宝石"), "宝石")) {
        d->p[!strcmpi(d->p - strlen("石"), "石") ? -strlen("石") : -strlen("宝石")] = '\0';
        d->oclass = GEM_CLASS;
        d->dn = d->actualn = d->bp;
        return 1; /*goto srch;*/
    } else if (!strcmpi(d->bp, "looking glass")) {
        ; /* avoid false hit on "* glass" */
    } else if (!BSTRCMPI(d->bp, d->p - 6, " glass")
               || !strcmpi(d->bp, "glass")) {
        char *s = d->bp;

        /* treat "broken glass" as a non-existent item; since "broken" is
           also a chest/box prefix it might have been stripped off above */
        if (d->broken || strstri(s, "broken")) {
            d->otmp = (struct obj *) 0;
            return 3; /* return otmp */
        }
        if (d->broken || strstri(s, "破损的")) {
            d->otmp = (struct obj *) 0;
            return 3; /* return otmp */
        }
        if (!strncmpi(s, "worthless ", 10))
            s += 10;
        if (!strncmpi(s, "毫无价值的", strlen("毫无价值的")))
            s += strlen("毫无价值的");
        if (!strncmpi(s, "不值钱的", strlen("不值钱的")))
            s += strlen("不值钱的");
        if (!strncmpi(s, "piece of ", 9))
            s += 9;
        if (!strncmpi(s, "一块", strlen("一块")))
            s += strlen("一块");
        if (!strncmpi(s, "colored ", 8))
            s += 8;
        else if (!strncmpi(s, "coloured ", 9))
            s += 9;
        if (!strncmpi(s, "有色的", strlen("有色的")))
            s += strlen("有色的");
        if (!strncmpi(s, "有色的", strlen("有色的")))
            s += strlen("有色的");
        if (!strcmpi(s, "glass")) { /* choose random color */
            /* 9 different kinds */
            d->typ = FIRST_GLASS_GEM + rn2(NUM_GLASS_GEMS);
            if (objects[d->typ].oc_class == GEM_CLASS)
                return 2; /*goto typfnd;*/
            else
                d->typ = 0; /* somebody changed objects[]? punt */
        } else { /* try to construct canonical form */
            char tbuf[BUFSZ];

            Strcpy(tbuf, "毫无价值的一块");
            Strcat(tbuf, s); /* assume it starts with the color */
            Strcpy(d->bp, tbuf);
        }
    }

    d->actualn = d->bp;
    if (!d->dn)
        d->dn = d->actualn; /* ex. "skull cap" */

    return 0;
}


staticfn int
readobjenam_postparse2(struct _readobjnam_data *d) /*AAAAAA*/
{
    int i;

    /* "grey stone" check must be before general "stone" */
    for (i = 0; i < SIZE(o_ranges); i++)
        if (!strcmpi(d->bp, o_ranges[i].name)) {
            d->typ = rnd_class(o_ranges[i].f_o_range, o_ranges[i].l_o_range);
            return 2; /*goto typfnd;*/
        }

    if (!BSTRCMPI(d->bp, d->p - 6, " stone")
        || !BSTRCMPI(d->bp, d->p - 4, " gem")) {
        d->p[!strcmpi(d->p - 4, " gem") ? -4 : -6] = '\0';
        d->oclass = GEM_CLASS;
        d->dn = d->actualn = d->bp;
        return 1; /*goto srch;*/
    } else if (!strcmpi(d->bp, "looking glass")) {
        ; /* avoid false hit on "* glass" */
    } else if (!BSTRCMPI(d->bp, d->p - 6, " glass")
               || !strcmpi(d->bp, "glass")) {
        char *s = d->bp;

        /* treat "broken glass" as a non-existent item; since "broken" is
           also a chest/box prefix it might have been stripped off above */
        if (d->broken || strstri(s, "broken")) {
            d->otmp = (struct obj *) 0;
            return 3; /* return otmp */
        }
        if (!strncmpi(s, "worthless ", 10))
            s += 10;
        if (!strncmpi(s, "piece of ", 9))
            s += 9;
        if (!strncmpi(s, "colored ", 8))
            s += 8;
        else if (!strncmpi(s, "coloured ", 9))
            s += 9;
        if (!strcmpi(s, "glass")) { /* choose random color */
            /* 9 different kinds */
            d->typ = FIRST_GLASS_GEM + rn2(NUM_GLASS_GEMS);
            if (objects[d->typ].oc_class == GEM_CLASS)
                return 2; /*goto typfnd;*/
            else
                d->typ = 0; /* somebody changed objects[]? punt */
        } else { /* try to construct canonical form */
            char tbuf[BUFSZ];

            Strcpy(tbuf, "worthless piece of ");
            Strcat(tbuf, s); /* assume it starts with the color */
            Strcpy(d->bp, tbuf);
        }
    }

    d->actualn = d->bp;
    if (!d->dn)
        d->dn = d->actualn; /* ex. "skull cap" */

    return 0;
}


staticfn int
readobjnam_postparse3(struct _readobjnam_data *d)
{
    int i;

    /* check real names of gems first */
    if (!d->oclass && d->actualn) {
        for (i = svb.bases[GEM_CLASS]; i <= LAST_REAL_GEM; i++) {
            const char *zn;
            if ((zn = OBJ_NAME(objects[i])) != 0 && !strcmpi(d->actualn, zn)) {
                d->typ = i;
                return 2; /*goto typfnd;*/
            }
            if ((zn = OBJ_ENAME(objects[i])) != 0 && !strcmpi(d->actualn, zn)) {
                d->typ = i;
                return 2; /*goto typfnd;*/
            }
        }
        /* "tin of foo" would be caught above, but plain "tin" has
           a random chance of yielding "tin wand" unless we do this */
        if (!strcmpi(d->actualn, "tin")) {
            d->typ = TIN;
            return 2; /*goto typfnd;*/
        }
        if (!strcmpi(d->actualn, "罐头")) {
            d->typ = TIN;
            return 2; /*goto typfnd;*/
        }
    }

    if (((d->typ = rnd_otyp_by_namedesc(d->actualn, d->oclass, 1))
         != STRANGE_OBJECT)
        || (d->dn != d->actualn
            && ((d->typ = rnd_otyp_by_namedesc(d->dn, d->oclass, 1))
                != STRANGE_OBJECT))
        || ((d->typ = rnd_otyp_by_namedesc(d->un, d->oclass, 1))
             != STRANGE_OBJECT)
        || (d->origbp != d->actualn
            && ((d->typ = rnd_otyp_by_namedesc(d->origbp, d->oclass, 1))
                != STRANGE_OBJECT)))
        return 2; /*goto typfnd;*/
    d->typ = 0;

    if (((d->typ = rnd_otyp_by_enameedesc(d->actualn, d->oclass, 1))
         != STRANGE_OBJECT)
        || (d->dn != d->actualn
            && ((d->typ = rnd_otyp_by_enameedesc(d->dn, d->oclass, 1))
                != STRANGE_OBJECT))
        || ((d->typ = rnd_otyp_by_enameedesc(d->un, d->oclass, 1))
             != STRANGE_OBJECT)
        || (d->origbp != d->actualn
            && ((d->typ = rnd_otyp_by_enameedesc(d->origbp, d->oclass, 1))
                != STRANGE_OBJECT)))
        return 2; /*goto typfnd;*/
    d->typ = 0;

    if (d->actualn) {
        const struct Jitem *j = Japanese_items;

        while (j->item) {
            if (!strcmpi(d->actualn, j->name)) {
                d->typ = j->item;
                return 2; /*goto typfnd;*/
            }
            j++;
        }
    }

    if (d->actualn) {
        const struct Jitem *j = eJapanese_items;

        while (j->item) {
            if (!strcmpi(d->actualn, j->name)) {
                d->typ = j->item;
                return 2; /*goto typfnd;*/
            }
            j++;
        }
    }

    /* if we've stripped off "armor" and failed to match anything
       in objects[], append "mail" and try again to catch misnamed
       requests like "plate armor" and "yellow dragon scale armor" */
    if (d->oclass == ARMOR_CLASS && !strstri(d->bp, "mail")) {
        /* modifying bp's string is ok; we're about to resort
           to random armor if this also fails to match anything */
        Strcat(d->bp, " mail");
        return 6; /*goto retry;*/
    }
    if (!strcmpi(d->bp, "spinach")) {
        d->contents = TIN_SPINACH;
        d->typ = TIN;
        return 2; /*goto typfnd;*/
    }
    if (!strcmpi(d->bp, "菠菜")) {
        d->contents = TIN_SPINACH;
        d->typ = TIN;
        return 2; /*goto typfnd;*/
    }
    /* Fruits must not mess up the ability to wish for real objects (since
     * you can leave a fruit in a bones file and it will be added to
     * another person's game), so they must be checked for last, after
     * stripping all the possible prefixes and seeing if there's a real
     * name in there.  So we have to save the full original name.  However,
     * it's still possible to do things like "uncursed burnt Alaska",
     * or worse yet, "2 burned 5 course meals", so we need to loop to
     * strip off the prefixes again, this time stripping only the ones
     * possible on food.
     * We could get even more detailed so as to allow food names with
     * prefixes that _are_ possible on food, so you could wish for
     * "2 3 alarm chilis".  Currently this isn't allowed; options.c
     * automatically sticks 'candied' in front of such names.
     */
    /* Note: not strcmpi.  2 fruits, one capital, one not, are possible.
       Also not strncmp.  We used to ignore trailing text with it, but
       that resulted in "grapefruit" matching "grape" if the latter came
       earlier than the former in the fruit list. */
    {
        char *fp;
        int l, cntf;
        int blessedf, iscursedf, uncursedf, halfeatenf;
        struct fruit *f;

        blessedf = iscursedf = uncursedf = halfeatenf = 0;
        cntf = 0;

        fp = d->fruitbuf;
        for (;;) {
            if (!fp || !*fp)
                break;
            if (!strncmpi(fp, "an ", l = 3) || !strncmpi(fp, "a ", l = 2)) {
                cntf = 1;
            } else if (!cnstrcmpi(fp, "一个", l)) {
                cntf = 1;
            } else if (!cntf && digit(*fp)) {
                cntf = atoi(fp);
                while (digit(*fp))
                    fp++;
                while (*fp == ' ')
                    fp++;
                l = 0;
            } else if (!strncmpi(fp, "blessed ", l = 8)) {
                blessedf = 1;
            } else if (!cnstrcmpi(fp, "被祝福的", l) || !cnstrcmpi(fp, "受祝福的", l) || !cnstrcmpi(fp, "有祝福的", l) || 
                        !cnstrcmpi(fp, "祝福的", l) || !cnstrcmpi(fp, "祝福", l) || !cnstrcmpi(fp, "圣", l)) {
                blessedf = 1;
            } else if (!strncmpi(fp, "cursed ", l = 7)) {
                iscursedf = 1;
            } else if (!cnstrcmpi(fp, "被诅咒的", l) || !cnstrcmpi(fp, "受诅咒的", l) || !cnstrcmpi(fp, "有诅咒的", l) ||
                        !cnstrcmpi(fp, "诅咒的", l) || !cnstrcmpi(fp, "诅咒", l) || !cnstrcmpi(fp, "邪", l)) {
                uncursedf = 1;
            } else if (!strncmpi(fp, "partly eaten ", l = 13) || !strncmpi(fp, "partially eaten ", l = 16)) {
                halfeatenf = 1;
            }  else if (!cnstrcmpi(fp, "吃掉一部分的", l) || !cnstrcmpi(fp, "一部分吃掉的", l) || !cnstrcmpi(fp, "吃掉部分的", l) || !cnstrcmpi(fp, "部分吃掉的", l) ||
                        !cnstrcmpi(fp, "吃了一部分的", l) || !cnstrcmpi(fp, "一部分吃了的", l) || !cnstrcmpi(fp, "吃了部分的", l) || !cnstrcmpi(fp, "部分吃了的", l) ||
                        !cnstrcmpi(fp, "吃掉了一部分的", l) || !cnstrcmpi(fp, "一部分吃掉了的", l) || !cnstrcmpi(fp, "吃掉了部分的", l) || !cnstrcmpi(fp, "部分吃掉了的", l) ||
                        !cnstrcmpi(fp, "部分食用的", l) || !cnstrcmpi(fp, "吃了一半的", l) ||
                        !cnstrcmpi(fp, "被吃掉一部分的", l) || !cnstrcmpi(fp, "一部分被吃掉的", l) || !cnstrcmpi(fp, "被吃掉部分的", l) || !cnstrcmpi(fp, "部分被吃掉的", l) ||
                        !cnstrcmpi(fp, "被吃了一部分的", l) || !cnstrcmpi(fp, "一部分被吃了的", l) || !cnstrcmpi(fp, "被吃了部分的", l) || !cnstrcmpi(fp, "部分被吃了的", l) ||
                        !cnstrcmpi(fp, "被吃掉了一部分的", l) || !cnstrcmpi(fp, "一部分被吃掉了的", l) || !cnstrcmpi(fp, "被吃掉了部分的", l) || !cnstrcmpi(fp, "部分被吃掉了的", l) ||
                        !cnstrcmpi(fp, "被部分食用的", l) || !cnstrcmpi(fp, "被吃了一半的", l)) {
                halfeatenf = 1;
            } else
                break;
            fp += l;
        }

        for (f = gf.ffruit; f; f = f->nextf) {
            /* match type: 0=none, 1=exact, 2=singular, 3=plural */
            int ftyp = 0;

            if (!strcmp(fp, f->fname))
                ftyp = 1;
            else if (!strcmp(fp, makesingular(f->fname)))
                ftyp = 2;
            else if (!strcmp(fp, makeplural(f->fname)))
                ftyp = 3;
            if (ftyp) {
                d->typ = SLIME_MOLD;
                d->blessed = blessedf;
                d->iscursed = iscursedf;
                d->uncursed = uncursedf;
                d->halfeaten = halfeatenf;
                /* adjust count if user explicitly asked for
                   singular amount (can't happen unless fruit
                   has been given an already pluralized name)
                   or for plural amount */
                if (ftyp == 2 && !cntf)
                    cntf = 1;
                else if (ftyp == 3 && !cntf)
                    cntf = 2;
                d->cnt = cntf;
                d->ftype = f->fid;
                return 2; /*goto typfnd;*/
            }
        }
    }

    if (!d->oclass && d->actualn) {
        short objtyp;

        /* Perhaps it's an artifact specified by name, not type */
        d->name = artifact_name(d->actualn, &objtyp, TRUE);
        if (d->name) {
            d->typ = objtyp;
            return 2; /*goto typfnd;*/
        }
    }

    /* got a class, but not specific type;
       check alternate spellings of items with matching classes */
    if (d->oclass && !d->typ) {
        const struct alt_spellings *as = spellings;

        while (as->sp) {
            if (objects[as->ob].oc_class == d->oclass
                && wishymatch(d->bp, as->sp, TRUE)) {
                d->typ = as->ob;
                return 2; /*goto typfnd;*/
            }
            as++;
        }
    }

    return 0;
}

staticfn int
readobjenam_postparse3(struct _readobjnam_data *d)
{
    int i;

    /* check real names of gems first */
    if (!d->oclass && d->actualn) {
        for (i = svb.bases[GEM_CLASS]; i <= LAST_REAL_GEM; i++) {
            const char *zn;

            if ((zn = OBJ_NAME(objects[i])) != 0 && !strcmpi(d->actualn, zn)) {
                d->typ = i;
                return 2; /*goto typfnd;*/
            }
        }
        /* "tin of foo" would be caught above, but plain "tin" has
           a random chance of yielding "tin wand" unless we do this */
        if (!strcmpi(d->actualn, "tin")) {
            d->typ = TIN;
            return 2; /*goto typfnd;*/
        }
    }

    if (((d->typ = rnd_otyp_by_namedesc(d->actualn, d->oclass, 1))
         != STRANGE_OBJECT)
        || (d->dn != d->actualn
            && ((d->typ = rnd_otyp_by_namedesc(d->dn, d->oclass, 1))
                != STRANGE_OBJECT))
        || ((d->typ = rnd_otyp_by_namedesc(d->un, d->oclass, 1))
             != STRANGE_OBJECT)
        || (d->origbp != d->actualn
            && ((d->typ = rnd_otyp_by_namedesc(d->origbp, d->oclass, 1))
                != STRANGE_OBJECT)))
        return 2; /*goto typfnd;*/
    d->typ = 0;

    if (d->actualn) {
        const struct Jitem *j = Japanese_items;

        while (j->item) {
            if (!strcmpi(d->actualn, j->name)) {
                d->typ = j->item;
                return 2; /*goto typfnd;*/
            }
            j++;
        }
    }
    /* if we've stripped off "armor" and failed to match anything
       in objects[], append "mail" and try again to catch misnamed
       requests like "plate armor" and "yellow dragon scale armor" */
    if (d->oclass == ARMOR_CLASS && !strstri(d->bp, "mail")) {
        /* modifying bp's string is ok; we're about to resort
           to random armor if this also fails to match anything */
        Strcat(d->bp, " mail");
        return 6; /*goto retry;*/
    }
    if (!strcmpi(d->bp, "spinach")) {
        d->contents = TIN_SPINACH;
        d->typ = TIN;
        return 2; /*goto typfnd;*/
    }
    /* Fruits must not mess up the ability to wish for real objects (since
     * you can leave a fruit in a bones file and it will be added to
     * another person's game), so they must be checked for last, after
     * stripping all the possible prefixes and seeing if there's a real
     * name in there.  So we have to save the full original name.  However,
     * it's still possible to do things like "uncursed burnt Alaska",
     * or worse yet, "2 burned 5 course meals", so we need to loop to
     * strip off the prefixes again, this time stripping only the ones
     * possible on food.
     * We could get even more detailed so as to allow food names with
     * prefixes that _are_ possible on food, so you could wish for
     * "2 3 alarm chilis".  Currently this isn't allowed; options.c
     * automatically sticks 'candied' in front of such names.
     */
    /* Note: not strcmpi.  2 fruits, one capital, one not, are possible.
       Also not strncmp.  We used to ignore trailing text with it, but
       that resulted in "grapefruit" matching "grape" if the latter came
       earlier than the former in the fruit list. */
    {
        char *fp;
        int l, cntf;
        int blessedf, iscursedf, uncursedf, halfeatenf;
        struct fruit *f;

        blessedf = iscursedf = uncursedf = halfeatenf = 0;
        cntf = 0;

        fp = d->fruitbuf;
        for (;;) {
            if (!fp || !*fp)
                break;
            if (!strncmpi(fp, "an ", l = 3) || !strncmpi(fp, "a ", l = 2)) {
                cntf = 1;
            } else if (!cntf && digit(*fp)) {
                cntf = atoi(fp);
                while (digit(*fp))
                    fp++;
                while (*fp == ' ')
                    fp++;
                l = 0;
            } else if (!strncmpi(fp, "blessed ", l = 8)) {
                blessedf = 1;
            } else if (!strncmpi(fp, "cursed ", l = 7)) {
                iscursedf = 1;
            } else if (!strncmpi(fp, "uncursed ", l = 9)) {
                uncursedf = 1;
            } else if (!strncmpi(fp, "partly eaten ", l = 13)
                       || !strncmpi(fp, "partially eaten ", l = 16)) {
                halfeatenf = 1;
            } else
                break;
            fp += l;
        }

        for (f = gf.ffruit; f; f = f->nextf) {
            /* match type: 0=none, 1=exact, 2=singular, 3=plural */
            int ftyp = 0;

            if (!strcmp(fp, f->fname))
                ftyp = 1;
            else if (!strcmp(fp, makesingular(f->fname)))
                ftyp = 2;
            else if (!strcmp(fp, makeplural(f->fname)))
                ftyp = 3;
            if (ftyp) {
                d->typ = SLIME_MOLD;
                d->blessed = blessedf;
                d->iscursed = iscursedf;
                d->uncursed = uncursedf;
                d->halfeaten = halfeatenf;
                /* adjust count if user explicitly asked for
                   singular amount (can't happen unless fruit
                   has been given an already pluralized name)
                   or for plural amount */
                if (ftyp == 2 && !cntf)
                    cntf = 1;
                else if (ftyp == 3 && !cntf)
                    cntf = 2;
                d->cnt = cntf;
                d->ftype = f->fid;
                return 2; /*goto typfnd;*/
            }
        }
    }

    if (!d->oclass && d->actualn) {
        short objtyp;

        /* Perhaps it's an artifact specified by name, not type */
        d->name = artifact_name(d->actualn, &objtyp, TRUE);
        if (d->name) {
            d->typ = objtyp;
            return 2; /*goto typfnd;*/
        }
    }

    /* got a class, but not specific type;
       check alternate spellings of items with matching classes */
    if (d->oclass && !d->typ) {
        const struct alt_spellings *as = spellings;

        while (as->sp) {
            if (objects[as->ob].oc_class == d->oclass
                && wishymatch(d->bp, as->sp, TRUE)) {
                d->typ = as->ob;
                return 2; /*goto typfnd;*/
            }
            as++;
        }
    }

    return 0;
}


/*
 * Return something wished for.  Specifying a null pointer for
 * the user request string results in a random object.  Otherwise,
 * if asking explicitly for "nothing" (or "nil") return no_wish;
 * if not an object return &hands_obj; if an error (no matching object),
 * return null.
 */
struct obj *
readobjnam(char *bp, struct obj *no_wish)
{
    struct _readobjnam_data d;

    readobjnam_init(bp, &d);
    if (!bp)
        goto any;

    /* first, remove extra whitespace they may have typed */
    (void) mungspaces(bp);
    /* allow wishing for "nothing" to preserve wishless conduct...
       [now requires "wand of nothing" if that's what was really wanted] */
    if (!strcmpi(bp, "nothing") || !strcmpi(bp, "nil")
        || !strcmpi(bp, "none") || !strcmpi(bp, "无"))
        return no_wish;
    /* save the [nearly] unmodified choice string */
    Strcpy(d.fruitbuf, bp);

    if (readobjnam_preparse(&d))
        goto any;

    if (!d.cnt)
        d.cnt = 1; /* will be changed to 2 if makesingular() changes string */

    readobjnam_parse_charges(&d);

    switch (readobjnam_postparse1(&d)) {
    default:
    case 0: break;
    case 1: goto srch;
    case 2: goto typfnd;
    case 3: return d.otmp;
    case 4: goto any;
    case 5: goto wiztrap;
    }

 retry:
    switch (readobjnam_postparse2(&d)) {
    default:
    case 0: break;
    case 1: goto srch;
    case 2: goto typfnd;
    case 3: return d.otmp;
    case 4: goto any;
    case 5: goto wiztrap;
    }

 srch:
    switch (readobjnam_postparse3(&d)) {
    default:
    case 0: break;
    case 1: goto srch;
    case 2: goto typfnd;
    case 3: return d.otmp;
    case 4: goto any;
    case 5: goto wiztrap;
    case 6: goto retry;
    }

    /*
     * Let wizards wish for traps and furniture.
     * Must come after objects check so wizards can still wish for
     * trap objects like beartraps.
     * Disallow such topology tweaks for WIZKIT startup wishes.
     */
 wiztrap: /*危险:你个邪恶的巫师够了没有，，，*/
    if (wizard && !program_state.wizkit_wishing && !d.oclass) {
        /* [inline code moved to separate routine to unclutter readobjnam] */
        if ((d.otmp = wizterrainwish(&d)) != 0)
            return d.otmp;
    }

    if (!d.oclass && !d.typ) {
        if (!strncmpi(d.bp, "polearm", 7)) {
            d.typ = rnd_otyp_by_wpnskill(P_POLEARMS);
            goto typfnd;
        } else if (!strncmpi(d.bp, "hammer", 6)) {
            d.typ = rnd_otyp_by_wpnskill(P_HAMMER);
            goto typfnd;
        }
    }

    if (!d.oclass)
        return ((struct obj *) 0);
 any:
    if (!d.oclass)
        d.oclass = wrpsym[rn2((int) sizeof wrpsym)];
 typfnd:
    if (d.typ)
        d.oclass = objects[d.typ].oc_class;

    /* handle some objects that are only allowed in wizard mode */
    if (d.typ && !wizard) {
        switch (d.typ) {
        case AMULET_OF_YENDOR:
            d.typ = FAKE_AMULET_OF_YENDOR;
            break;
        case CANDELABRUM_OF_INVOCATION:
            d.typ = rnd_class(TALLOW_CANDLE, WAX_CANDLE);
            break;
        case BELL_OF_OPENING:
            d.typ = BELL;
            break;
        case SPE_BOOK_OF_THE_DEAD:
            d.typ = SPE_BLANK_PAPER;
            break;
        case MAGIC_LAMP:
            d.typ = OIL_LAMP;
            break;
        default:
            /* catch any other non-wishable objects (venom) */
            if (objects[d.typ].oc_nowish)
                return (struct obj *) 0;
            break;
        }
    }

    /* if asking for corpse of a monster which leaves behind a glob, give
       glob instead of rejecting the monster type to create random corpse */
    if (d.typ == CORPSE && d.mntmp >= LOW_PM
        && mons[d.mntmp].mlet == S_PUDDING) {
        d.typ = GLOB_OF_GRAY_OOZE + (d.mntmp - PM_GRAY_OOZE);
        d.mntmp = NON_PM; /* not used for globs */
    }
    /*
     * Create the object, then fine-tune it.
     */
    d.otmp = d.typ ? mksobj(d.typ, TRUE, FALSE) : mkobj(d.oclass, FALSE);
    d.typ = d.otmp->otyp, d.oclass = d.otmp->oclass; /* what we actually got */

    /* if player specified a reasonable count, maybe honor it;
       quantity for gold is handled elsewhere and d.cnt is 0 for it here */
    if (d.otmp->globby) {
        /* for globs, calculate weight based on gsize, then multiply by cnt;
           asking for 2 globs or for 2 small globs produces 1 small glob
           weighing 40au instead of normal 20au; asking for 5 medium globs
           might produce 1 very large glob weighing 600au */
        d.otmp->quan = 1L; /* always 1 for globs */
        d.otmp->owt = weight(d.otmp);
        /* gsize 0: unspecified => small;
           1: small (1..5) => keep default owt for 1, yielding 20;
           2: medium (6..15) => use weight for 6, yielding 120;
           3: large (16..25) => 320; 4: very large (26+) => 520 */
        if (d.gsize > 1)
            d.otmp->owt += ((unsigned) (5 + (d.gsize - 2) * 10)
                            * d.otmp->owt);  /* 20 + {5|15|25} times 20 */
        /* limit overall weight which limits shrink-away time which in turn
           affects how long some of it will remain available to be eaten */
        if (d.cnt > 1) {
            int rn1cnt = rn1(5, 2); /* 2..6 */

            if (rn1cnt > 6 - d.gsize)
                rn1cnt = 6 - d.gsize;
            if (d.cnt > rn1cnt
                && (!wizard || program_state.wizkit_wishing
                    || y_n("Override glob weight limit?") != 'y'))
                d.cnt = rn1cnt;
            d.otmp->owt *= (unsigned) d.cnt;
        }
        /* note: the owt assignment below will not change glob's weight */
        d.cnt = 0;
    } else if (d.cnt > 0) {
        if (objects[d.typ].oc_merge
            && (wizard /* quantity isn't restricted when debugging */
                /* note: in normal play, explicitly asking for 1 might
                   fail the 'cnt < rnd(6)' test and could produce more
                   than 1 if mksobj() creates the item that way */
                || d.cnt < rnd(6)
                || (d.cnt <= 7 && Is_candle(d.otmp))
                || (d.cnt <= 20
                    && (d.typ == ROCK || d.typ == FLINT || is_missile(d.otmp)
                        /* WEAPON_CLASS test excludes gems, gray stones */
                        || (d.oclass == WEAPON_CLASS && is_ammo(d.otmp))))))
            d.otmp->quan = (long) d.cnt;
    }

    if (d.islit && (d.typ == OIL_LAMP || d.typ == MAGIC_LAMP
                    || d.typ == BRASS_LANTERN
                    || Is_candle(d.otmp) || d.typ == POT_OIL)) {
        place_object(d.otmp, u.ux, u.uy); /* make it viable light source */
        begin_burn(d.otmp, FALSE);
        obj_extract_self(d.otmp); /* now release it for caller's use */
    }

    if (d.spesgn == 0) {
        /* spe not specified; retain the randomly assigned value */
        d.spe = d.otmp->spe;
    } else if (wizard) {
        ; /* no restrictions except SPE_LIM */
    } else if (d.oclass == ARMOR_CLASS || d.oclass == WEAPON_CLASS
               || is_weptool(d.otmp)
               || (d.oclass == RING_CLASS && objects[d.typ].oc_charged)) {
        if (d.spe > rnd(5) && d.spe > d.otmp->spe)
            d.spe = 0;
        if (d.spe > 2 && Luck < 0)
            d.spesgn = -1;
    } else {
        /* crystal ball cancels like a wand, to (n:-1) */
        if (d.oclass == WAND_CLASS || d.typ == CRYSTAL_BALL) {
            if (d.spe > 1 && d.spesgn == -1)
                d.spe = 1;
        } else {
            if (d.spe > 0 && d.spesgn == -1)
                d.spe = 0;
        }
        if (d.spe > d.otmp->spe)
            d.spe = d.otmp->spe;
    }

    if (d.spesgn == -1)
        d.spe = -d.spe;

    /* set otmp->spe.  This may, or may not, use d.spe... */
    switch (d.typ) {
    case TIN:
        d.otmp->spe = 0; /* default: not spinach */
        if (d.contents == TIN_EMPTY) {
            d.otmp->corpsenm = NON_PM;
        } else if (d.contents == TIN_SPINACH) {
            d.otmp->corpsenm = NON_PM;
            d.otmp->spe = 1; /* spinach after all */
        }
        break;
    case TOWEL:
        if (d.wetness)
            d.otmp->spe = d.wetness;
        break;
    case SLIME_MOLD:
        d.otmp->spe = d.ftype;
        FALLTHROUGH;
    /* FALLTHRU */
    case SKELETON_KEY:
    case CHEST:
    case LARGE_BOX:
    case HEAVY_IRON_BALL:
    case IRON_CHAIN:
        break;
    case STATUE: /* otmp->cobj already done in mksobj() */
    case FIGURINE:
    case CORPSE: {
        struct permonst *P = (ismnum(d.mntmp)) ? &mons[d.mntmp] : 0;

        d.otmp->spe = !P ? CORPSTAT_RANDOM
                      /* if neuter, force neuter regardless of wish request */
                      : is_neuter(P) ? CORPSTAT_NEUTER
                        /* not neuter, honor wish unless it conflicts */
                        : (d.mgend == FEMALE && !is_male(P)) ? CORPSTAT_FEMALE
                          : (d.mgend == MALE && !is_female(P)) ? CORPSTAT_MALE
                            /* unspecified or wish conflicts */
                            : CORPSTAT_RANDOM;
        if (P && d.otmp->spe == CORPSTAT_RANDOM)
            d.otmp->spe = is_male(P) ? CORPSTAT_MALE
                          : is_female(P) ? CORPSTAT_FEMALE
                            : rn2(2) ? CORPSTAT_MALE : CORPSTAT_FEMALE;
        if (d.ishistoric && d.typ == STATUE)
            d.otmp->spe |= CORPSTAT_HISTORIC;
        break;
    };
#ifdef MAIL_STRUCTURES
    /* scroll of mail:  0: delivered in-game via external event (or randomly
       for fake mail); 1: from bones or wishing; 2: written with marker */
    case SCR_MAIL:
        d.otmp->spe = 1;
        break;
#endif
    /* splash of venom:  0: normal, and transitory; 1: wishing */
    case ACID_VENOM:
    case BLINDING_VENOM:
        d.otmp->spe = 1;
        break;
    case WAN_WISHING:
        if (!wizard) {
            d.otmp->spe = (rn2(10) ? -1 : 0);
            break;
        }
        FALLTHROUGH;
        /*FALLTHRU*/
    default:
        d.otmp->spe = d.spe;
    }

    /* set otmp->corpsenm or dragon scale [mail] */
    if (ismnum(d.mntmp)) {
        int humanwere;

        if (d.mntmp == PM_LONG_WORM_TAIL)
            d.mntmp = PM_LONG_WORM;
        /* werecreatures in beast form are all flagged no-corpse so for
           corpses and tins, switch to their corresponding human form;
           for figurines, override the can't-be-human restriction instead */
        if (d.typ != FIGURINE && is_were(&mons[d.mntmp])
            && (svm.mvitals[d.mntmp].mvflags & G_NOCORPSE) != 0
            && (humanwere = counter_were(d.mntmp)) != NON_PM)
            d.mntmp = humanwere;

        switch (d.typ) {
        case TIN:
            if (dead_species(d.mntmp, FALSE)) {
                d.otmp->corpsenm = NON_PM; /* it's empty */
            } else if ((!(mons[d.mntmp].geno & G_UNIQ) || wizard)
                       && !(svm.mvitals[d.mntmp].mvflags & G_NOCORPSE)
                       && mons[d.mntmp].cnutrit != 0) {
                d.otmp->corpsenm = d.mntmp;
            }
            break;
        case CORPSE:
            if ((!(mons[d.mntmp].geno & G_UNIQ) || wizard)
                && !(svm.mvitals[d.mntmp].mvflags & G_NOCORPSE)) {
                if (mons[d.mntmp].msound == MS_GUARDIAN)
                    d.mntmp = genus(d.mntmp, 1);
                set_corpsenm(d.otmp, d.mntmp);
            }
            if (d.zombify && zombie_form(&mons[d.mntmp])) {
                (void) start_timer(rn1(5, 10), TIMER_OBJECT,
                                   ZOMBIFY_MON, obj_to_any(d.otmp));
            }
            break;
        case EGG:
            d.mntmp = can_be_hatched(d.mntmp);
            /* this also sets hatch timer if appropriate */
            set_corpsenm(d.otmp, d.mntmp);
            break;
        case FIGURINE:
            if (!(mons[d.mntmp].geno & G_UNIQ)
                && (!is_human(&mons[d.mntmp]) || is_were(&mons[d.mntmp]))
#ifdef MAIL_STRUCTURES
                && d.mntmp != PM_MAIL_DAEMON
#endif
                )
                d.otmp->corpsenm = d.mntmp;
            break;
        case STATUE:
            d.otmp->corpsenm = d.mntmp;
            if (Has_contents(d.otmp) && verysmall(&mons[d.mntmp]))
                delete_contents(d.otmp); /* no spellbook */
            break;
        case SCALE_MAIL:
            /* Dragon mail - depends on the order of objects & dragons. */
            if (d.mntmp >= PM_GRAY_DRAGON && d.mntmp <= PM_YELLOW_DRAGON)
                d.otmp->otyp = GRAY_DRAGON_SCALE_MAIL
                              + d.mntmp - PM_GRAY_DRAGON;
            break;
        }
    }

    /* set blessed/cursed -- setting the fields directly is safe
     * since weight() is called below and addinv() will take care
     * of luck */
    if (d.iscursed) {
        curse(d.otmp);
    } else if (d.uncursed) {
        d.otmp->blessed = 0;
        d.otmp->cursed = (Luck < 0 && !wizard);
    } else if (d.blessed) {
        d.otmp->blessed = (Luck >= 0 || wizard);
        d.otmp->cursed = (Luck < 0 && !wizard);
    } else if (d.spesgn < 0) {
        curse(d.otmp);
    }

    /* set eroded and erodeproof */
    if (erosion_matters(d.otmp)) {
        /* wished-for item shouldn't be eroded unless specified */
        d.otmp->oeroded = d.otmp->oeroded2 = 0;
        if (d.eroded && (is_flammable(d.otmp) || is_rustprone(d.otmp)
                         || is_crackable(d.otmp)))
            d.otmp->oeroded = d.eroded;
        if (d.eroded2 && (is_corrodeable(d.otmp) || is_rottable(d.otmp)))
            d.otmp->oeroded2 = d.eroded2;
        /*
         * 3.6.1: earlier versions included `&& !eroded && !eroded2' here,
         * but damageproof combined with damaged is feasible (eroded
         * armor modified by confused reading of cursed destroy armor)
         * so don't prevent player from wishing for such a combination.
         */
        if (d.erodeproof
            && (is_damageable(d.otmp) || d.otmp->otyp == CRYSKNIFE))
            d.otmp->oerodeproof = (Luck >= 0 || wizard);
    }

    /* set otmp->recharged */
    if (d.oclass == WAND_CLASS) {
        /* prevent wishing abuse */
        if (d.otmp->otyp == WAN_WISHING && !wizard)
            d.rechrg = 1;
        d.otmp->recharged = (unsigned) d.rechrg;
    }

    /* set poisoned */
    if (d.ispoisoned) {
        if (is_poisonable(d.otmp))
            d.otmp->opoisoned = (Luck >= 0);
        else if (d.oclass == FOOD_CLASS)
            /* try to taint by making it as old as possible */
            d.otmp->age = 1L;
    }
    /* and [un]trapped */
    if (d.trapped) {
        if (Is_box(d.otmp) || d.typ == TIN)
            d.otmp->otrapped = (d.trapped == 1);
    }
    /* empty for containers rather than for tins */
    if (d.contents == TIN_EMPTY) {
        if (d.otmp->otyp == BAG_OF_TRICKS || d.otmp->otyp == HORN_OF_PLENTY) {
            if (d.otmp->spe > 0)
                d.otmp->spe = 0;
        } else if (Has_contents(d.otmp)) {
            /* this assumes that artifacts can't be randomly generated
               inside containers */
            delete_contents(d.otmp);
            d.otmp->owt = weight(d.otmp);
        }
    }
    /* set locked/unlocked/broken */
    if (Is_box(d.otmp)) {
        if (d.locked) {
            d.otmp->olocked = 1, d.otmp->obroken = 0;
        } else if (d.unlocked) {
            d.otmp->olocked = 0, d.otmp->obroken = 0;
        } else if (d.broken) {
            d.otmp->olocked = 0, d.otmp->obroken = 1;
        }
        if (d.otmp->obroken)
            d.otmp->otrapped = 0;
    }

    if (d.isgreased)
        d.otmp->greased = 1;

    if (d.isdiluted && d.otmp->oclass == POTION_CLASS)
        d.otmp->odiluted = (d.otmp->otyp != POT_WATER);

    /* set tin variety */
    if (d.otmp->otyp == TIN && d.tvariety >= 0 && (rn2(4) || wizard))
        set_tin_variety(d.otmp, d.tvariety);

    if (d.name) {
        const char *aname, *novelname;
        short objtyp;

        /* an artifact name might need capitalization fixing */
        aname = artifact_name(d.name, &objtyp, TRUE);
        if (aname && objtyp == d.otmp->otyp)
            d.name = aname;

        /* 3.6 tribute - fix up novel */
        if (d.otmp->otyp == SPE_NOVEL
            && (novelname = lookup_novel(d.name, &d.otmp->novelidx)) != 0)
            d.name = novelname;

        d.otmp = oname(d.otmp, d.name, ONAME_WISH);
        /* name==aname => wished for artifact (otmp->oartifact => got it) */
        if (d.otmp->oartifact || d.name == aname) {
            d.otmp->quan = 1L;
            u.uconduct.wisharti++; /* KMH, conduct */
        }
    }

    if (permapoisoned(d.otmp))
        d.otmp->opoisoned = 1;

    /* more wishing abuse: don't allow wishing for certain artifacts */
    /* and make them pay; charge them for the wish anyway! */
    if ((is_quest_artifact(d.otmp)
         || (d.otmp->oartifact && rn2(nartifact_exist()) > 1)) && !wizard) {
        artifact_exists(d.otmp, safe_oname(d.otmp), FALSE, ONAME_NO_FLAGS);
        obfree(d.otmp, (struct obj *) 0);
        d.otmp = &hands_obj;
        pline("片刻间,你感觉到%s到了你的%s里,但它随机消失了!",
              something, makeplural(body_part(HAND)));
        return d.otmp;
    }

    if (d.halfeaten && d.otmp->oclass == FOOD_CLASS) {
        unsigned nut = obj_nutrition(d.otmp);

        /* do this adjustment before setting up object's weight; skip
           "partly eaten" for food with 0 nutrition (wraith corpse) or for
           anything that couldn't take more than one bite (1 nutrition;
           ought to check for one-bite instead but that's complicated) */
        if (nut > 1) {
            d.otmp->oeaten = nut;
            consume_oeaten(d.otmp, 1);
        }
    }
    d.otmp->owt = weight(d.otmp);
    if (d.very && d.otmp->otyp == HEAVY_IRON_BALL)
        d.otmp->owt += WT_IRON_BALL_INCR;

    return d.otmp;
}

int
rnd_class(int first, int last)
{
    int i, x, sum = 0;

    if (last > first) {
        for (i = first; i <= last; i++)
            sum += objects[i].oc_prob;
        if (!sum) /* all zero, so equal probability */
            return rn1(last - first + 1, first);

        x = rnd(sum);
        for (i = first; i <= last; i++)
            if ((x -= objects[i].oc_prob) <= 0)
                return i;
    }
    return (first == last) ? first : STRANGE_OBJECT;
}

const char *
armor_simple_name(struct obj *armor)
{
    const char *result = 0;
    unsigned armcat = objects[armor->otyp].oc_armcat;

    switch (armcat) {
    case ARM_SUIT:
        result = suit_simple_name(armor);
        break;
    case ARM_CLOAK:
        result = cloak_simple_name(armor);
        break;
    case ARM_HELM:
        result = helm_simple_name(armor);
        break;
    case ARM_GLOVES:
        result = gloves_simple_name(armor);
        break;
    case ARM_BOOTS:
        result = boots_simple_name(armor);
        break;
    case ARM_SHIELD:
        result = shield_simple_name(armor);
        break;
    case ARM_SHIRT:
        result = shirt_simple_name(armor);
        break;
    default:
        result = simpleonames(armor);
        impossible("unknown armor category (%s => %u)", result, armcat);
        break;
    }
    return result;
}

const char *
suit_simple_name(struct obj *suit)
{
    const char *suitnm, *esuitp;

    if (suit) {
        if (Is_dragon_mail(suit))
            return "dragon mail"; /* <color> dragon scale mail */
        else if (Is_dragon_scales(suit))
            return "dragon scales";
        suitnm = OBJ_NAME(objects[suit->otyp]);
        esuitp = eos((char *) suitnm);
        if (strlen(suitnm) > 5 && !strcmp(esuitp - 5, " mail"))
            return "mail"; /* most suits fall into this category */
        else if (strlen(suitnm) > 7 && !strcmp(esuitp - 7, " jacket"))
            return "jacket"; /* leather jacket */
    }
    /* "suit" is lame but "armor" is ambiguous and "body armor" is absurd */
    return "suit";
}

const char *
cloak_simple_name(struct obj *cloak)
{
    if (cloak) {
        switch (cloak->otyp) {
        case ROBE:
            return "robe";
        case MUMMY_WRAPPING:
            return "wrapping";
        case ALCHEMY_SMOCK:
            return (objects[cloak->otyp].oc_name_known && cloak->dknown)
                       ? "smock"
                       : "apron";
        default:
            break;
        }
    }
    return "cloak";
}

/* helm vs hat for messages */
const char *
helm_simple_name(struct obj *helmet)
{
    /*
     *  There is some wiggle room here; the result has been chosen
     *  for consistency with the "protected by hard helmet" messages
     *  given for various bonks on the head:  headgear that provides
     *  such protection is a "helm", that which doesn't is a "hat".
     *
     *      elven leather helm / leather hat    -> hat
     *      dwarvish iron helm / hard hat       -> helm
     *  The rest are completely straightforward:
     *      fedora, cornuthaum, dunce cap       -> hat
     *      all other types of helmets          -> helm
     */
    return !hard_helmet(helmet) ? "hat" : "helm";
}

/* gloves vs gauntlets; depends upon discovery state */
const char *
gloves_simple_name(struct obj *gloves)
{
    static const char gauntlets[] = "护手";

    if (gloves && gloves->dknown) {
        int otyp = gloves->otyp;
        struct objclass *ocl = &objects[otyp];
        const char *actualn = OBJ_NAME(*ocl),
                   *descrpn = OBJ_DESCR(*ocl);

        if (strstri(objects[otyp].oc_name_known ? actualn : descrpn,
                    gauntlets))
            return gauntlets;
    }
    return "手套";
}

/* boots vs shoes; depends upon discovery state */
const char *
boots_simple_name(struct obj *boots)
{
    static const char shoes[] = "shoes";

    if (boots && boots->dknown) {
        int otyp = boots->otyp;
        struct objclass *ocl = &objects[otyp];
        const char *actualn = OBJ_NAME(*ocl),
                   *descrpn = OBJ_DESCR(*ocl);

        if (strstri(descrpn, shoes)
            || (objects[otyp].oc_name_known && strstri(actualn, shoes)))
            return shoes;
    }
    return "boots";
}

/* simplified shield for messages */
const char *
shield_simple_name(struct obj *shield)
{
    if (shield) {
        /* xname() describes unknown (unseen) reflection as smooth */
        if (shield->otyp == SHIELD_OF_REFLECTION)
            return shield->dknown ? "silver shield" : "smooth shield";
        /*
         * We might distinguish between wooden vs metallic or
         * light vs heavy to give small benefit to spell casters.
         * Fighter types probably care more about the former for
         * vulnerability to fire or rust.
         *
         * We could do that both ways: light wooden shield, light
         * metallic shield (there aren't any), heavy wooden shield,
         * and heavy metallic shield but that's getting away from
         * "simple name" which is intended to be shorter as well
         * as less detailed than xname().
         */
#if 0
        /* spellcasting uses a division like this */
        return (weight(shield) > (int) objects[SMALL_SHIELD].oc_weight)
               ? "heavy shield"
               : "light shield";
#endif
    }
    return "shield";
}

/* for completeness */
const char *
shirt_simple_name(struct obj *shirt UNUSED)
{
    return "shirt";
}

const char *
mimic_obj_name(struct monst *mtmp)
{
    if (M_AP_TYPE(mtmp) == M_AP_OBJECT) {
        if (mtmp->mappearance == GOLD_PIECE)
            return "gold";
        if (mtmp->mappearance != STRANGE_OBJECT)
            return simple_typename(mtmp->mappearance);
    }
    return "whatcha-may-callit";
}

/*
 * Construct a query prompt string, based around an object name, which is
 * guaranteed to fit within [QBUFSZ].  Takes an optional prefix, three
 * choices for filling in the middle (two object formatting functions and a
 * last resort literal which should be very short), and an optional suffix.
 */
char *
safe_qbuf(
    char *qbuf, /* output buffer */
    const char *qprefix,
    const char *qsuffix,
    struct obj *obj,
    char *(*func)(OBJ_P),
    char *(*altfunc)(OBJ_P),
    const char *lastR)
{
    char *bufp, *endp;
    /* convert size_t (or int for ancient systems) to ordinary unsigned */
    unsigned len, lenlimit,
        len_qpfx = (unsigned) (qprefix ? strlen(qprefix) : 0),
        len_qsfx = (unsigned) (qsuffix ? strlen(qsuffix) : 0),
        len_lastR = (unsigned) strlen(lastR);

    lenlimit = QBUFSZ - 1;
    endp = qbuf + lenlimit;
    assert(endp != NULL); /* workaround for static analyzer issue */
    /* sanity check, aimed mainly at paniclog (it's conceivable for
       the result of short_oname() to be shorter than the length of
       the last resort string, but we ignore that possibility here) */
    if (len_qpfx > lenlimit)
        impossible("safe_qbuf: prefix too long (%u characters).", len_qpfx);
    else if (len_qpfx + len_qsfx > lenlimit)
        impossible("safe_qbuf: suffix too long (%u + %u characters).",
                   len_qpfx, len_qsfx);
    else if (len_qpfx + len_lastR + len_qsfx > lenlimit)
        impossible("safe_qbuf: filler too long (%u + %u + %u characters).",
                   len_qpfx, len_lastR, len_qsfx);

    /* the output buffer might be the same as the prefix if caller
       has already partially filled it */
    if (qbuf == qprefix) {
        /* prefix is already in the buffer */
        *endp = '\0';
    } else if (qprefix) {
        /* put prefix into the buffer */
        (void) strncpy(qbuf, qprefix, lenlimit);
        *endp = '\0';
    } else {
        /* no prefix; output buffer starts out empty */
        qbuf[0] = '\0';
    }
    len = (unsigned) strlen(qbuf);

    if (len + len_lastR + len_qsfx > lenlimit) {
        /* too long; skip formatting, last resort output is truncated */
        if (len < lenlimit) {
            (void) strncpy(&qbuf[len], lastR, lenlimit - len);
            *endp = '\0';
            len = (unsigned) strlen(qbuf);
            if (qsuffix && len < lenlimit) {
                (void) strncpy(&qbuf[len], qsuffix, lenlimit - len);
                *endp = '\0';
                /* len = (unsigned) strlen(qbuf); */
            }
        }
    } else {
        /* suffix and last resort are guaranteed to fit */
        len += len_qsfx; /* include the pending suffix */
        /* format the object */
        bufp = short_oname(obj, func, altfunc, lenlimit - len);
        if (len + strlen(bufp) <= lenlimit)
            Strcat(qbuf, bufp); /* formatted name fits */
        else
            Strcat(qbuf, lastR); /* use last resort */
        releaseobuf(bufp);

        if (qsuffix)
            Strcat(qbuf, qsuffix);
    }
    /* assert( strlen(qbuf) < QBUFSZ ); */
    return qbuf;
}

/*objnam.c*/
