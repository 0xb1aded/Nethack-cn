/* NetHack 5.0	mondata.c	$NHDT-Date: 1738638877 2025/02/03 19:14:37 $  $NHDT-Branch: NetHack-3.7 $:$NHDT-Revision: 1.140 $ */
/* Copyright (c) Stichting Mathematisch Centrum, Amsterdam, 1985. */
/*-Copyright (c) Robert Patrick Rankin, 2011. */
/* NetHack may be freely redistributed.  See license for details. */

#include "hack.h"
/*
 *      These routines provide basic data for any type of monster.
 */

/* set up an individual monster's base type (initial creation, shapechange) */
void
set_mon_data(struct monst *mon, struct permonst *ptr)
{
    int new_speed, old_speed = mon->data ? mon->data->mmove : 0;
    short *movement_p = (mon == &gy.youmonst) ? &u.umovement : &mon->movement;

    mon->data = ptr;
    mon->mnum = (short) monsndx(ptr);

    if (*movement_p) { /* used to adjust poly'd hero as well as monsters */
        new_speed = ptr->mmove;
        /* prorate unused movement if new form is slower so that
           it doesn't get extra moves leftover from previous form;
           if new form is faster, leave unused movement as is */
        if (new_speed < old_speed) {
            /*
             * Some static analysis warns that this might divide by 0
               mon->movement = new_speed * mon->movement / old_speed;
             * so add a redundant test to suppress that.
             */
            *movement_p *= new_speed;
            if (old_speed > 0) /* old > new and new >= 0, so always True */
                *movement_p /= old_speed;
        }
    }
    return;
}

/* does monster-type have any attack for a specific type of damage? */
struct attack *
attacktype_fordmg(struct permonst *ptr, int atyp, int dtyp)
{
    struct attack *a;

    for (a = &ptr->mattk[0]; a < &ptr->mattk[NATTK]; a++)
        if (a->aatyp == atyp && (dtyp == AD_ANY || a->adtyp == dtyp))
            return a;
    return (struct attack *) 0;
}

/* does monster-type have a particular type of attack */
boolean
attacktype(struct permonst *ptr, int atyp)
{
    return attacktype_fordmg(ptr, atyp, AD_ANY) ? TRUE : FALSE;
}

/* returns True if monster doesn't attack, False if it does */
boolean
noattacks(struct permonst *ptr)
{
    int i;
    struct attack *mattk = ptr->mattk;

    for (i = 0; i < NATTK; i++) {
        /* AT_BOOM "passive attack" (gas spore's explosion upon death)
           isn't an attack as far as our callers are concerned */
        if (mattk[i].aatyp == AT_BOOM)
            continue;

        if (mattk[i].aatyp)
            return FALSE;
    }
    return TRUE;
}

/* does monster-type transform into something else when petrified? */
boolean
poly_when_stoned(struct permonst *ptr)
{
    /* non-stone golems turn into stone golems unless latter is genocided */
    return (boolean) (is_golem(ptr) && ptr != &mons[PM_STONE_GOLEM]
                      && !(svm.mvitals[PM_STONE_GOLEM].mvflags & G_GENOD));
    /* allow G_EXTINCT */
}

/* is 'mon' (possibly youmonst) protected against damage type 'adtype' via
   wielded weapon or worn dragon scales? [or by virtue of being a dragon?] */
boolean
defended(struct monst *mon, int adtyp)
{
    struct obj *o, otemp;
    int mndx;
    boolean is_you = (mon == &gy.youmonst);

    /* is 'mon' wielding an artifact that protects against 'adtyp'? */
    o = is_you ? uwep : MON_WEP(mon);
    if (o && o->oartifact && defends(adtyp, o))
        return TRUE;

    /* if 'mon' is an adult dragon, treat it as if it was wearing scales
       so that it has the same benefit as a hero wearing dragon scales */
    mndx = monsndx(mon->data);
    if (mndx >= PM_GRAY_DRAGON && mndx <= PM_YELLOW_DRAGON) {
        /* a dragon is its own suit...  if mon is poly'd hero, we don't
           care about embedded scales (uskin) because being a dragon with
           embedded scales is no better than just being a dragon */
        otemp = cg.zeroobj;
        otemp.oclass = ARMOR_CLASS;
        otemp.otyp = GRAY_DRAGON_SCALES + (mndx - PM_GRAY_DRAGON);
        /* defends() and Is_dragon_armor() only care about otyp so ignore
           the rest of otemp's fields */
        o = &otemp;
    } else {
        /* ordinary case: not an adult dragon */
        o = is_you ? uarm : which_armor(mon, W_ARM);
    }
    /* is 'mon' wearing dragon scales that protect against 'adtyp'? */
    if (o && Is_dragon_armor(o) && defends(adtyp, o))
        return TRUE;

    return FALSE;
}

/* returns True if monster resists particular elemental damage;
   handles 'carry' effects of artifacts as well as worn/wielded items */
boolean
Resists_Elem(struct monst *mon, int propindx)
{
    struct obj *o;
    long slotmask;
    boolean is_you = (mon == &gy.youmonst);
    int u_resist = 0, damgtype = 0, rsstmask = 0;

    /*
     * Main damage/resistance types, mostly matching dragon breath values.
     *  propindx = property index, fire (1), cold, (2) through stone (8);
     *  damgtype = damage type, 2 through 9 (0 and 1 aren't used here);
     *  rsstmask = resistance mask, 1, 2, 4, ..., 64, 128.
     */

    switch (propindx) {
    case FIRE_RES:   /* 1 */
    case COLD_RES:   /* 2 */
    case SLEEP_RES:  /* 3 */
    case DISINT_RES: /* 4 */
    case SHOCK_RES:  /* 5 */
    case POISON_RES: /* 6 */
    case ACID_RES:   /* 7 */
    case STONE_RES:  /* 8 */
        damgtype = propindx + 1; /* valid for propindx 1..8, damgtype 2..9 */
        rsstmask = 1 << (propindx - 1); /* valid for propindx 1..8 */
        u_resist = u.uprops[propindx].intrinsic
                   || u.uprops[propindx].extrinsic;
        break;

    /* accept these, but we expect callers to use their routines directly */
    case ANTIMAGIC:
        return resists_magm(mon);
    case DRAIN_RES:
        return resists_drli(mon);
    case BLND_RES:
        return resists_blnd(mon);

    default:
        impossible("Resists_Elem(%d), unexpected property type", propindx);
        return FALSE;
    }

    if (is_you ? u_resist : ((mon_resistancebits(mon) & rsstmask) != 0))
        return TRUE;
    /* check for resistance granted by wielded weapon */
    o = is_you ? uwep : MON_WEP(mon);
    if (o && o->oartifact && defends(damgtype, o))
        return TRUE;
    /* check for resistance granted by worn or carried items */
    o = is_you ? gi.invent : mon->minvent;
    slotmask = W_ARMOR | W_ACCESSORY;
    if (!is_you /* assumes monsters don't wield non-weapons */
        || (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep))))
        slotmask |= W_WEP;
    if (is_you && u.twoweap)
        slotmask |= W_SWAPWEP;
    for (; o; o = o->nobj)
        if (((o->owornmask & slotmask) != 0L
             && objects[o->otyp].oc_oprop == propindx)
            || ((o->owornmask & W_ARMC) == W_ARMC
                /* worn apron confers a pair of resistances but
                   objects[ALCHEMY_SMOCK].oc_oprop can only represent one;
                   we check both so won't need to know which one that is */
                && o->otyp == ALCHEMY_SMOCK
                && (propindx == POISON_RES || propindx == ACID_RES))
            || (o->oartifact && defends_when_carried(damgtype, o)))
            return TRUE;
    return FALSE;
}

/* returns True if monster is drain-life resistant */
boolean
resists_drli(struct monst *mon)
{
    struct permonst *ptr = mon->data;

    if (is_undead(ptr) || is_demon(ptr) || is_were(ptr)
        /* is_were() doesn't handle hero in human form */
        || (mon == &gy.youmonst && u.ulycn >= LOW_PM)
        || ptr == &mons[PM_DEATH] || is_vampshifter(mon))
        return TRUE;
    return defended(mon, AD_DRLI);
}

/* True if monster is magic-missile (actually, general magic) resistant */
boolean
resists_magm(struct monst *mon)
{
    struct permonst *ptr = mon->data;
    boolean is_you = (mon == &gy.youmonst);
    long slotmask;
    struct obj *o;

    /* as of 3.2.0:  gray dragons, Angels, Oracle, Yeenoghu */
    if (dmgtype(ptr, AD_MAGM) || ptr == &mons[PM_BABY_GRAY_DRAGON]
        || dmgtype(ptr, AD_RBRE)) /* Chromatic Dragon */
        return TRUE;
    /* check for magic resistance granted by wielded weapon */
    o = is_you ? uwep : MON_WEP(mon);
    if (o && o->oartifact && defends(AD_MAGM, o))
        return TRUE;
    /* check for magic resistance granted by worn or carried items */
    o = is_you ? gi.invent : mon->minvent;
    slotmask = W_ARMOR | W_ACCESSORY;
    if (!is_you /* assumes monsters don't wield non-weapons */
        || (uwep && (uwep->oclass == WEAPON_CLASS || is_weptool(uwep))))
        slotmask |= W_WEP;
    if (is_you && u.twoweap)
        slotmask |= W_SWAPWEP;
    for (; o; o = o->nobj)
        if (((o->owornmask & slotmask) != 0L
             && objects[o->otyp].oc_oprop == ANTIMAGIC)
            || (o->oartifact && defends_when_carried(AD_MAGM, o)))
            return TRUE;
    return FALSE;
}

/* True if monster is resistant to light-induced blindness */
boolean
resists_blnd(struct monst *mon)
{
    struct permonst *ptr = mon->data;
    boolean is_you = (mon == &gy.youmonst);

    if (is_you ? (Blind || Unaware)
               : (mon->mblinded || !mon->mcansee || !haseyes(ptr)
                  /* BUG: temporary sleep sets mfrozen, but since
                          paralysis does too, we can't check it */
                  || mon->msleeping))
        return TRUE;
    /* yellow light, Archon; !dust vortex, !cobra, !raven */
    if (dmgtype_fromattack(ptr, AD_BLND, AT_EXPL)
        || dmgtype_fromattack(ptr, AD_BLND, AT_GAZE))
        return TRUE;
    /* Sunsword */
    if (resists_blnd_by_arti(mon))
        return TRUE;
    /* catchall */
    if (is_you && Blnd_resist) {
        impossible("'Blnd_resist' but not resists_blnd()?");
        return TRUE;
    }
    return FALSE;
}

/* True iff monster is resistant to light-induced blindness due to worn
   or wielded magical equipment (used to decide whether to show sparkle
   animation when resisting) */
boolean
resists_blnd_by_arti(struct monst *mon)
{
    struct obj *o;
    boolean is_you = (mon == &gy.youmonst);

    o = is_you ? uwep : MON_WEP(mon);
    if (o && o->oartifact && defends(AD_BLND, o))
        return TRUE;
    o = is_you ? gi.invent : mon->minvent;
    for (; o; o = o->nobj)
        if (defends_when_carried(AD_BLND, o))
            return TRUE;
#if 0   /* omit this; the Eyes of the Overworld have no carry property and
         * their worn property is magic resistance rather than blindness
         * resistance; wearing them blocks blindness without actually
         * preventing it, so don't classify them as providing resistance */
    if (is_you && is_art(uamul, ART_EYES_OF_THE_OVERWORLD))
        return TRUE;
#endif /* 0 */
    return FALSE;
}

/* True iff monster can be blinded by the given attack;
   note: may return True when mdef is blind (e.g. new cream-pie attack)
   magr can be NULL.
*/
boolean
can_blnd(
    struct monst *magr, /* NULL == no specific aggressor */
    struct monst *mdef,
    uchar aatyp,
    struct obj *obj) /* aatyp == AT_WEAP, AT_SPIT */
{
    boolean is_you = (mdef == &gy.youmonst);
    boolean check_visor = FALSE;
    struct obj *o;

    /* no eyes protect against all attacks for now */
    if (!haseyes(mdef->data))
        return FALSE;

    /* if monster has been permanently blinded, the deed is already done */
    if (!is_you && mon_perma_blind(mdef))
        return FALSE;

    /* /corvus oculum corvi non eruit/
       a saying expressed in Latin rather than a zoological observation:
       "a crow will not pluck out the eye of another crow"
       so prevent ravens from blinding each other */
    if (magr && magr->data == &mons[PM_RAVEN] && mdef->data == &mons[PM_RAVEN])
        return FALSE;

    switch (aatyp) {
    case AT_EXPL:
    case AT_BOOM:
    case AT_GAZE:
    case AT_MAGC:
    case AT_BREA: /* assumed to be lightning */
        /* light-based attacks may be cancelled or resisted */
        if (magr && magr->mcan)
            return FALSE;
        return !resists_blnd(mdef);

    case AT_WEAP:
    case AT_SPIT:
    case AT_NONE:
        /* an object is used (thrown/spit/other) */
        if (obj && (obj->otyp == CREAM_PIE)) {
            if (is_you && Blindfolded)
                return FALSE;
        } else if (obj && (obj->otyp == BLINDING_VENOM)) {
            /* all ublindf, including LENSES, protect, cream-pies too */
            if (is_you && (ublindf || u.ucreamed))
                return FALSE;
            check_visor = TRUE;
        } else if (obj && (obj->otyp == POT_BLINDNESS)) {
            return TRUE; /* no defense */
        } else
            return FALSE; /* other objects cannot cause blindness yet */
        if ((magr == &gy.youmonst) && u.uswallow)
            return FALSE; /* can't affect eyes while inside monster */
        break;

    case AT_ENGL:
        if (is_you && (Blindfolded || Unaware || u.ucreamed))
            return FALSE;
        if (!is_you && mdef->msleeping)
            return FALSE;
        break;

    case AT_CLAW:
        /* e.g. raven: all ublindf, including LENSES, protect */
        if (is_you && ublindf)
            return FALSE;
        if ((magr == &gy.youmonst) && u.uswallow)
            return FALSE; /* can't affect eyes while inside monster */
        check_visor = TRUE;
        break;

    case AT_TUCH:
    case AT_STNG:
        /* some physical, blind-inducing attacks can be cancelled */
        if (magr && magr->mcan)
            return FALSE;
        break;

    default:
        break;
    }

    /* check if wearing a visor (only checked if visor might help) */
    if (check_visor) {
        o = (mdef == &gy.youmonst) ? gi.invent : mdef->minvent;
        for (; o; o = o->nobj)
            if ((o->owornmask & W_ARMH)
                && objdescr_is(o, "visored helmet"))
                return FALSE;
    }

    return TRUE;
}

/* returns True if monster can attack at range */
boolean
ranged_attk(struct permonst *ptr)
{
    int i;

    for (i = 0; i < NATTK; i++)
        if (DISTANCE_ATTK_TYPE(ptr->mattk[i].aatyp))
            return TRUE;
    return FALSE;
}

#if defined(MAKEDEFS_C) \
    || (NH_DEVEL_STATUS != NH_STATUS_RELEASED) || defined(DEBUG)
/*
 * If adding a new monster, include a guestimate for difficulty,
 * build the program, then run it in wizard mode and use the
 * #mondifficulty command.  If it reports a discrepancy, update
 * the monsters array with the more accurate value (or possibly
 * modify the 'mstrength()' algorithm to generate the guessed one).
 */
static boolean mstrength_ranged_attk(struct permonst *);


/* This routine is designed to return an integer value which represents
   an approximation of monster strength.  It uses a similar method of
   determination as "experience()" to arrive at the strength. */
int
mstrength(struct permonst *ptr)
{
    int i, tmp2, n, tmp = ptr->mlevel;

    if (tmp > 49) /* special fixed hp monster */
        tmp = 2 * (tmp - 6) / 4;

    /* for creation in groups */
    n = (!!(ptr->geno & G_SGROUP));
    n += (!!(ptr->geno & G_LGROUP)) << 1;

    /* for ranged attacks */
    if (mstrength_ranged_attk(ptr))
        n++;

    /* for higher ac values */
    n += (ptr->ac < 4);
    n += (ptr->ac < 0);

    /* for very fast monsters */
    n += (ptr->mmove >= 18);

    /* for each attack and "special" attack */
    for (i = 0; i < NATTK; i++) {
        tmp2 = ptr->mattk[i].aatyp;
        n += (tmp2 > 0);
        n += (tmp2 == AT_MAGC);
        n += (tmp2 == AT_WEAP && (ptr->mflags2 & M2_STRONG));
        if (tmp2 == AT_EXPL) {
            int tmp3 = ptr->mattk[i].adtyp;
            /* {freezing,flaming,shocking} spheres are fairly weak but
               can destroy equipment; {yellow,black} lights can't */
            n += ((tmp3 == AD_COLD || tmp3 == AD_FIRE) ? 3
                  : (tmp3 == AD_ELEC) ? 5
                    : 0);
        }
    }

    /* for each "special" damage type */
    for (i = 0; i < NATTK; i++) {
        tmp2 = ptr->mattk[i].adtyp;
        if ((tmp2 == AD_DRLI) || (tmp2 == AD_STON) || (tmp2 == AD_DRST)
            || (tmp2 == AD_DRDX) || (tmp2 == AD_DRCO) || (tmp2 == AD_WERE))
            n += 2;
        else if (strcmp(ptr->epmnames[NEUTRAL], "grid bug")) /*危险:原为pmnames,下同*/
            n += (tmp2 != AD_PHYS);
        n += ((int) (ptr->mattk[i].damd * ptr->mattk[i].damn) > 23);
    }

    /* Leprechauns are a special case.  They have many hit dice so they can
       hit and are hard to kill, but they don't really do much damage. */
    if (!strcmp(ptr->epmnames[NEUTRAL], "leprechaun"))
        n -= 2;

    /* despite group and poison increments, soldier ants and killer bees are
       underestimated by the formula, so have an artificial +1 difficulty */
    if (!strcmp(ptr->epmnames[NEUTRAL], "killer bee") ||
        !strcmp(ptr->epmnames[NEUTRAL], "soldier ant"))
        n += 2; /* +1 after 'tmp += n/2' below */

    /* finally, adjust the monster level  0 <= n <= 24 (approx.) */
    if (n == 0)
        tmp -= 1;
    else if (n < 6)
        tmp += (n / 3 + 1);
    else
        tmp += (n / 2);

    return (tmp >= 0) ? tmp : 0;
}

/* returns True if monster can attack at range */
staticfn boolean
mstrength_ranged_attk(struct permonst *ptr)
{
    int i, j;
    int atk_mask = (1 << AT_BREA) | (1 << AT_SPIT) | (1 << AT_GAZE);

    for (i = 0; i < NATTK; i++) {
        if ((j = ptr->mattk[i].aatyp) >= AT_WEAP
            || (j < 32 && (atk_mask & (1 << j)) != 0))
            return TRUE;
    }
    return FALSE;
}
#endif /* (NH_DEVEL_STATUS != NH_STATUS_RELEASED) || DEBUG || MAKEDEFS_C */

/* True if specific monster is especially affected by silver weapons */
boolean
mon_hates_silver(struct monst *mon)
{
    return (boolean) (is_vampshifter(mon) || hates_silver(mon->data));
}

/* True if monster-type is especially affected by silver weapons */
boolean
hates_silver(struct permonst *ptr)
{
    return (boolean) (is_were(ptr) || ptr->mlet == S_VAMPIRE || is_demon(ptr)
                      || ptr == &mons[PM_SHADE]
                      || (ptr->mlet == S_IMP && ptr != &mons[PM_TENGU]));
}

/* True if specific monster is especially affected by blessed objects */
boolean
mon_hates_blessings(struct monst *mon)
{
    return (boolean) (is_vampshifter(mon) || hates_blessings(mon->data));
}

/* True if monster-type is especially affected by blessed objects */
boolean
hates_blessings(struct permonst *ptr)
{
    return (boolean) (is_undead(ptr) || is_demon(ptr));
}

/* True if specific monster is especially affected by light-emitting weapons */
boolean
mon_hates_light(struct monst *mon)
{
    return (boolean) hates_light(mon->data);
}

/* True iff the type of monster pass through iron bars */
boolean
passes_bars(struct permonst *mptr)
{
    return (boolean) (passes_walls(mptr) || amorphous(mptr) || unsolid(mptr)
                      || is_whirly(mptr) || verysmall(mptr)
                      /* rust monsters and some puddings can destroy bars */
                      || dmgtype(mptr, AD_RUST) || dmgtype(mptr, AD_CORR)
                      /* rock moles can eat bars */
                      || metallivorous(mptr)
                      || (slithy(mptr) && !bigmonst(mptr)));
}

/* returns True if monster can blow (whistle, etc) */
boolean
can_blow(struct monst *mtmp)
{
    if ((is_silent(mtmp->data) || mtmp->data->msound == MS_BUZZ)
        && (breathless(mtmp->data) || verysmall(mtmp->data)
            || !has_head(mtmp->data) || mtmp->data->mlet == S_EEL))
        return FALSE;
    if ((mtmp == &gy.youmonst) && Strangled)
        return FALSE;
    return TRUE;
}

/* for casting spells and reading scrolls while blind */
boolean
can_chant(struct monst *mtmp)
{
    if ((mtmp == &gy.youmonst && Strangled)
        || is_silent(mtmp->data) || !has_head(mtmp->data)
        || mtmp->data->msound == MS_BUZZ || mtmp->data->msound == MS_BURBLE)
        return FALSE;
    return TRUE;
}

/* True if mon is vulnerable to strangulation */
boolean
can_be_strangled(struct monst *mon)
{
    struct obj *mamul;
    boolean nonbreathing, nobrainer;

    /* For amulet of strangulation support:  here we're considering
       strangulation to be loss of blood flow to the brain due to
       constriction of the arteries in the neck, so all headless
       creatures are immune (no neck) as are mindless creatures
       who don't need to breathe (brain, if any, doesn't care).
       Mindless creatures who do need to breath are vulnerable, as
       are non-breathing creatures which have higher brain function. */
    if (!has_head(mon->data))
        return FALSE;
    if (mon == &gy.youmonst) {
        /* hero can't be mindless but poly'ing into mindless form can
           confer strangulation protection */
        nobrainer = mindless(gy.youmonst.data);
        nonbreathing = Breathless;
    } else {
        nobrainer = mindless(mon->data);
        /* monsters don't wear amulets of magical breathing,
           so second part doesn't achieve anything useful... */
        nonbreathing = (breathless(mon->data)
                        || ((mamul = which_armor(mon, W_AMUL)) != 0
                            && (mamul->otyp == AMULET_OF_MAGICAL_BREATHING)));
    }
    return (boolean) (!nobrainer || !nonbreathing);
}

/* returns True if monster can track well */
boolean
can_track(struct permonst *ptr)
{
    if (u_wield_art(ART_EXCALIBUR))
        return TRUE;
    return (boolean) haseyes(ptr);
}

/* creature will slide out of armor */
boolean
sliparm(struct permonst *ptr)
{
    return (boolean) (is_whirly(ptr) || ptr->msize <= MZ_SMALL
                      || noncorporeal(ptr));
}

/* creature will break out of armor */
boolean
breakarm(struct permonst *ptr)
{
    if (sliparm(ptr))
        return FALSE;

    return (boolean) (bigmonst(ptr)
                      || (ptr->msize > MZ_SMALL && !humanoid(ptr))
                      /* special cases of humanoids that cannot wear suits */
                      || ptr == &mons[PM_MARILITH]
                      || ptr == &mons[PM_WINGED_GARGOYLE]);
}

/* creature sticks other creatures it hits */
boolean
sticks(struct permonst *ptr)
{
    return (boolean) (dmgtype(ptr, AD_STCK)
                      || (dmgtype(ptr, AD_WRAP) && !attacktype(ptr, AT_ENGL))
                      || attacktype(ptr, AT_HUGS));
}

/* some monster-types can't vomit */
boolean
cantvomit(struct permonst *ptr)
{
    /* rats and mice are incapable of vomiting; likewise with horses;
       which other creatures have the same limitation? */
    if (ptr->mlet == S_RODENT && ptr != &mons[PM_ROCK_MOLE]
        && ptr != &mons[PM_WOODCHUCK])
        return TRUE;
    if (ptr == &mons[PM_WARHORSE] || ptr == &mons[PM_HORSE]
        || ptr == &mons[PM_PONY])
        return TRUE;
    return FALSE;
}

/* number of horns this type of monster has on its head */
int
num_horns(struct permonst *ptr)
{
    switch (monsndx(ptr)) {
    case PM_HORNED_DEVIL: /* ? "more than one" */
    case PM_MINOTAUR:
    case PM_ASMODEUS:
    case PM_BALROG:
        return 2;
    case PM_WHITE_UNICORN:
    case PM_GRAY_UNICORN:
    case PM_BLACK_UNICORN:
    case PM_KI_RIN:
        return 1;
    default:
        break;
    }
    return 0;
}

/* does monster-type deal out a particular type of damage from a particular
   type of attack? */
struct attack *
dmgtype_fromattack(struct permonst *ptr, int dtyp, int atyp)
{
    struct attack *a;

    for (a = &ptr->mattk[0]; a < &ptr->mattk[NATTK]; a++)
        if (a->adtyp == dtyp && (atyp == AT_ANY || a->aatyp == atyp))
            return a;
    return (struct attack *) 0;
}

/* does monster-type deal out a particular type of damage from any attack */
boolean
dmgtype(struct permonst *ptr, int dtyp)
{
    return dmgtype_fromattack(ptr, dtyp, AT_ANY) ? TRUE : FALSE;
}

/* returns the maximum damage a defender can do to the attacker via
   a passive defense */
int
max_passive_dmg(struct monst *mdef, struct monst *magr)
{
    int i, dmg, multi2 = 0;
    uchar adtyp;

    /* each attack by magr can result in passive damage */
    for (i = 0; i < NATTK; i++)
        switch (magr->data->mattk[i].aatyp) {
        case AT_CLAW:
        case AT_BITE:
        case AT_KICK:
        case AT_BUTT:
        case AT_TUCH:
        case AT_STNG:
        case AT_HUGS:
        case AT_ENGL:
        case AT_TENT:
        case AT_WEAP:
            multi2++;
            break;
        default:
            break;
        }

    dmg = 0;
    for (i = 0; i < NATTK; i++)
        if (mdef->data->mattk[i].aatyp == AT_NONE
            || mdef->data->mattk[i].aatyp == AT_BOOM) {
            adtyp = mdef->data->mattk[i].adtyp;
            if ((adtyp == AD_FIRE && completelyburns(magr->data))
                || (adtyp == AD_DCAY && completelyrots(magr->data))
                || (adtyp == AD_RUST && completelyrusts(magr->data))) {
                dmg = magr->mhp;
            } else if ((adtyp == AD_ACID && !resists_acid(magr))
                       || (adtyp == AD_COLD && !resists_cold(magr))
                       || (adtyp == AD_FIRE && !resists_fire(magr))
                       || (adtyp == AD_ELEC && !resists_elec(magr))
                       || adtyp == AD_PHYS) {
                dmg = mdef->data->mattk[i].damn;
                if (!dmg)
                    dmg = mdef->data->mlevel + 1;
                dmg *= mdef->data->mattk[i].damd;
            }
            dmg *= multi2;
            break;
        }
    return dmg;
}

/* determine whether two monster types are from the same species */
boolean
same_race(struct permonst *pm1, struct permonst *pm2)
{
    char let1 = pm1->mlet, let2 = pm2->mlet;

    if (pm1 == pm2)
        return TRUE; /* exact match */
    /* player races have their own predicates */
    if (is_human(pm1))
        return is_human(pm2);
    if (is_elf(pm1))
        return is_elf(pm2);
    if (is_dwarf(pm1))
        return is_dwarf(pm2);
    if (is_gnome(pm1))
        return is_gnome(pm2);
    if (is_orc(pm1))
        return is_orc(pm2);
    /* other creatures are less precise */
    if (is_giant(pm1))
        return is_giant(pm2); /* open to quibbling here */
    if (is_golem(pm1))
        return is_golem(pm2); /* even moreso... */
    if (is_mind_flayer(pm1))
        return is_mind_flayer(pm2);
    if (let1 == S_KOBOLD || pm1 == &mons[PM_KOBOLD_ZOMBIE]
        || pm1 == &mons[PM_KOBOLD_MUMMY])
        return (let2 == S_KOBOLD || pm2 == &mons[PM_KOBOLD_ZOMBIE]
                || pm2 == &mons[PM_KOBOLD_MUMMY]);
    if (let1 == S_OGRE)
        return (let2 == S_OGRE);
    if (let1 == S_NYMPH)
        return (let2 == S_NYMPH);
    if (let1 == S_CENTAUR)
        return (let2 == S_CENTAUR);
    if (is_unicorn(pm1))
        return is_unicorn(pm2);
    if (let1 == S_DRAGON)
        return (let2 == S_DRAGON);
    if (let1 == S_NAGA)
        return (let2 == S_NAGA);
    /* other critters get steadily messier */
    if (is_rider(pm1))
        return is_rider(pm2); /* debatable */
    if (is_minion(pm1))
        return is_minion(pm2); /* [needs work?] */
    /* tengu don't match imps (first test handled case of both being tengu) */
    if (pm1 == &mons[PM_TENGU] || pm2 == &mons[PM_TENGU])
        return FALSE;
    if (let1 == S_IMP)
        return (let2 == S_IMP);
    /* and minor demons (imps) don't match major demons */
    else if (let2 == S_IMP)
        return FALSE;
    if (is_demon(pm1))
        return is_demon(pm2);
    if (is_undead(pm1)) {
        if (let1 == S_ZOMBIE)
            return (let2 == S_ZOMBIE);
        if (let1 == S_MUMMY)
            return (let2 == S_MUMMY);
        if (let1 == S_VAMPIRE)
            return (let2 == S_VAMPIRE);
        if (let1 == S_LICH)
            return (let2 == S_LICH);
        if (let1 == S_WRAITH)
            return (let2 == S_WRAITH);
        if (let1 == S_GHOST)
            return (let2 == S_GHOST);
    } else if (is_undead(pm2))
        return FALSE;

    /* check for monsters which grow into more mature forms */
    if (let1 == let2) {
        int m1 = monsndx(pm1), m2 = monsndx(pm2), prv, nxt;

        /* we know m1 != m2 (very first check above); test all smaller
           forms of m1 against m2, then all larger ones; don't need to
           make the corresponding tests for variants of m2 against m1 */
        for (prv = m1, nxt = big_to_little(m1); nxt != prv;
             prv = nxt, nxt = big_to_little(nxt))
            if (nxt == m2)
                return TRUE;
        for (prv = m1, nxt = little_to_big(m1); nxt != prv;
             prv = nxt, nxt = little_to_big(nxt))
            if (nxt == m2)
                return TRUE;
    }
    /* not caught by little/big handling */
    if (pm1 == &mons[PM_GARGOYLE] || pm1 == &mons[PM_WINGED_GARGOYLE])
        return (pm2 == &mons[PM_GARGOYLE]
                || pm2 == &mons[PM_WINGED_GARGOYLE]);
    if (pm1 == &mons[PM_KILLER_BEE] || pm1 == &mons[PM_QUEEN_BEE])
        return (pm2 == &mons[PM_KILLER_BEE] || pm2 == &mons[PM_QUEEN_BEE]);

    if (is_longworm(pm1))
        return is_longworm(pm2); /* handles tail */
    /* [currently there's no reason to bother matching up
        assorted bugs and blobs with their closest variants] */
    /* didn't match */
    return FALSE;
}

/* for handling alternate spellings */
struct alt_spl {
    const char *name;
    short pm_val;
    int genderhint;
};

/* figure out what type of monster a user-supplied string is specifying;
   ignore anything past the monster name */
int
name_to_mon(const char *in_str, int *gender_name_var)
{
    return name_to_monplus(in_str, (const char **) 0, gender_name_var);
}

/* figure out what type of monster a user-supplied string is specifying;
   return a pointer to whatever is past the monster name--necessary if
   caller wants to strip off the name and it matches one of the alternate
   names rather the canonical mons[].mname */
int
name_to_monplus(
    const char *in_str,
    const char **remainder_p,
    int *gender_name_var)
{
    /* Be careful.  We must check the entire string in case it was
     * something such as "ettin zombie corpse".  The calling routine
     * doesn't know about the "corpse" until the monster name has
     * already been taken off the front, so we have to be able to
     * read the name with extraneous stuff such as "corpse" stuck on
     * the end.
     * This causes a problem for names which prefix other names such
     * as "ettin" on "ettin zombie".  In this case we want the _longest_
     * name which exists.
     * This also permits plurals created by adding suffixes such as 's'
     * or 'es'.  Other plurals must still be handled explicitly.
     */
    int i;
    int mntmp = NON_PM;
    char *s, *str, *term;
    char buf[BUFSZ];
    int len, mgend, matchgend = -1;
    size_t slen;
    boolean exact_match = FALSE;

    if (remainder_p)
        *remainder_p = (const char *) 0;

    str = strcpy(buf, in_str);

    if (!strncmp(str, "a ", 2))
        str += 2;
    else if (!strncmp(str, "an ", 3))
        str += 3;
    else if (!strncmp(str, "the ", 4))
        str += 4;

    slen = strlen(str);
    term = str + slen;

    if ((s = strstri(str, "vortices")) != 0)
        Strcpy(s + 4, "ex");
    /* be careful with "ies"; "priest", "zombies" */
    else if (slen > 3 && !strcmpi(term - 3, "ies")
             && (slen < 7 || strcmpi(term - 7, "zombies")))
        Strcpy(term - 3, "y");
    /* luckily no monster names end in fe or ve with ves plurals */
    else if (slen > 3 && !strcmpi(term - 3, "ves"))
        Strcpy(term - 3, "f");

    slen = strlen(str); /* length possibly needs recomputing */

    {
        static const struct alt_spl names[] = {
            /* Alternate spellings */
            { "grey dragon", PM_GRAY_DRAGON, NEUTRAL },
            { "baby grey dragon", PM_BABY_GRAY_DRAGON, NEUTRAL },
            { "grey unicorn", PM_GRAY_UNICORN, NEUTRAL },
            { "grey ooze", PM_GRAY_OOZE, NEUTRAL },
            { "gray-elf", PM_GREY_ELF, NEUTRAL },
            { "mindflayer", PM_MIND_FLAYER, NEUTRAL },
            { "master mindflayer", PM_MASTER_MIND_FLAYER, NEUTRAL },
            /* More alternates; priest and priestess are separate monster
               types but that isn't the case for {aligned,high} priests */
            { "aligned priest", PM_ALIGNED_CLERIC, MALE },
            { "aligned priestess", PM_ALIGNED_CLERIC, FEMALE },
            { "high priest", PM_HIGH_CLERIC, MALE },
            { "high priestess", PM_HIGH_CLERIC, FEMALE },
            /* Inappropriate singularization by -ves check above */
            { "master of thief", PM_MASTER_OF_THIEVES, NEUTRAL },
            /* Potential misspellings where we want to avoid falling back
               to the rank title prefix (input has been singularized) */
            { "master thief", PM_MASTER_OF_THIEVES, NEUTRAL },
            { "master of assassin", PM_MASTER_ASSASSIN, NEUTRAL },
            { "master-lich", PM_MASTER_LICH, NEUTRAL }, /* cf arch-lich */
            { "masterlich", PM_MASTER_LICH, NEUTRAL }, /* cf demilich */
            /* Outdated names */
            { "invisible stalker", PM_STALKER, NEUTRAL },
            { "high-elf", PM_ELVEN_MONARCH, NEUTRAL }, /* PM_HIGH_ELF is
                                                        * obsolete */
            /* other misspellings or incorrect words */
            { "wood-elf", PM_WOODLAND_ELF, NEUTRAL },
            { "wood elf", PM_WOODLAND_ELF, NEUTRAL },
            { "woodland nymph", PM_WOOD_NYMPH, NEUTRAL },
            { "halfling", PM_HOBBIT, NEUTRAL },    /* potential guess for
                                                    * polyself */
            { "genie", PM_DJINNI, NEUTRAL }, /* potential guess for
                                              * ^G/#wizgenesis */
            /* prefix used to workaround duplicate monster names for
               monsters with alternate forms */
            { "human wererat", PM_HUMAN_WERERAT, NEUTRAL },
            { "human werejackal", PM_HUMAN_WEREJACKAL, NEUTRAL },
            { "human werewolf", PM_HUMAN_WEREWOLF, NEUTRAL },
            /* for completeness */
            { "rat wererat", PM_WERERAT, NEUTRAL },
            { "jackal werejackal", PM_WEREJACKAL, NEUTRAL },
            { "wolf werewolf", PM_WEREWOLF, NEUTRAL },
            /* Hyphenated names -- it would be nice to handle these via
               fuzzymatch() but it isn't able to ignore trailing stuff */
            { "ki rin", PM_KI_RIN, NEUTRAL },
            { "kirin", PM_KI_RIN, NEUTRAL },
            { "uruk hai", PM_URUK_HAI, NEUTRAL },
            { "orc captain", PM_ORC_CAPTAIN, NEUTRAL },
            { "woodland elf", PM_WOODLAND_ELF, NEUTRAL },
            { "green elf", PM_GREEN_ELF, NEUTRAL },
            { "grey elf", PM_GREY_ELF, NEUTRAL },
            { "gray elf", PM_GREY_ELF, NEUTRAL },
            { "elf lady", PM_ELF_NOBLE, FEMALE },
            { "elf lord", PM_ELF_NOBLE, MALE },
            { "elf noble", PM_ELF_NOBLE, NEUTRAL },
            { "olog hai", PM_OLOG_HAI, NEUTRAL },
            { "arch lich", PM_ARCH_LICH, NEUTRAL },
            { "archlich", PM_ARCH_LICH, NEUTRAL },
            /* Some irregular plurals */
            { "incubi", PM_AMOROUS_DEMON, MALE },
            { "succubi", PM_AMOROUS_DEMON, FEMALE },
            { "violet fungi", PM_VIOLET_FUNGUS, NEUTRAL },
            { "homunculi", PM_HOMUNCULUS, NEUTRAL },
            { "baluchitheria", PM_BALUCHITHERIUM, NEUTRAL },
            { "lurkers above", PM_LURKER_ABOVE, NEUTRAL },
            { "cavemen", PM_CAVE_DWELLER, MALE },
            { "cavewomen", PM_CAVE_DWELLER, FEMALE },
            { "watchmen", PM_WATCHMAN, NEUTRAL },
            { "djinn", PM_DJINNI, NEUTRAL },
            { "mumakil", PM_MUMAK, NEUTRAL },
            { "erinyes", PM_ERINYS, NEUTRAL },
            /*Francium-223: 如果要修改,应与objnam.c的monster_aliases保持同步*/
            { "巨型蚂蚁", PM_GIANT_ANT, NEUTRAL },
            { "巨蚂蚁", PM_GIANT_ANT, NEUTRAL },
            { "巨蚁", PM_GIANT_ANT, NEUTRAL },
            { "杀人蜂", PM_KILLER_BEE, NEUTRAL },
            { "巨蜂", PM_KILLER_BEE, NEUTRAL },
            { "兵蚁", PM_SOLDIER_ANT, NEUTRAL },
            { "火蚁", PM_FIRE_ANT, NEUTRAL },
            { "巨型甲虫", PM_GIANT_BEETLE, NEUTRAL },
            { "巨甲虫", PM_GIANT_BEETLE, NEUTRAL },
            { "蜂后", PM_QUEEN_BEE, NEUTRAL },
            { "酸滴", PM_ACID_BLOB, NEUTRAL },
            { "酸性团块", PM_ACID_BLOB, NEUTRAL },
            { "强酸团怪", PM_ACID_BLOB, NEUTRAL },
            { "酸块", PM_ACID_BLOB, NEUTRAL },
            { "颤抖的斑点", PM_QUIVERING_BLOB, NEUTRAL },
            { "颤抖斑点", PM_QUIVERING_BLOB, NEUTRAL },
            { "颤抖的团块", PM_QUIVERING_BLOB, NEUTRAL },
            { "颤抖团块", PM_QUIVERING_BLOB, NEUTRAL },
            { "颤动团怪", PM_QUIVERING_BLOB, NEUTRAL },
            { "黏胶立方怪", PM_GELATINOUS_CUBE, NEUTRAL },
            { "黏胶立方", PM_GELATINOUS_CUBE, NEUTRAL },
            { "凝胶方块", PM_GELATINOUS_CUBE, NEUTRAL },
            { "小鸡蛇", PM_CHICKATRICE, NEUTRAL },
            { "鸡蛇", PM_COCKATRICE, NEUTRAL },
            { "火鸡蛇", PM_PYROLISK, NEUTRAL },
            { "豺狼", PM_JACKAL, NEUTRAL },
            { "狐狸", PM_FOX, NEUTRAL },
            { "土狼", PM_COYOTE, NEUTRAL },
            { "郊狼", PM_COYOTE, NEUTRAL },
            { "豺狼人", PM_WEREJACKAL, NEUTRAL },
            { "小狗", PM_LITTLE_DOG, NEUTRAL },
            { "澳洲野狗", PM_DINGO, NEUTRAL },
            { "狗", PM_DOG, NEUTRAL },
            { "大狗", PM_LARGE_DOG, NEUTRAL },
            { "狼", PM_WOLF, NEUTRAL },
            { "狼人", PM_WEREWOLF, NEUTRAL },
            { "冬狼崽", PM_WINTER_WOLF_CUB, NEUTRAL },
            { "小冬狼", PM_WINTER_WOLF_CUB, NEUTRAL },
            { "座狼", PM_WARG, NEUTRAL },
            { "冬狼", PM_WINTER_WOLF, NEUTRAL },
            { "地狱小猎犬", PM_HELL_HOUND_PUP, NEUTRAL },
            { "地狱小狗", PM_HELL_HOUND_PUP, NEUTRAL },
            { "小地狱猎犬", PM_HELL_HOUND_PUP, NEUTRAL },
            { "小地狱狗", PM_HELL_HOUND_PUP, NEUTRAL },
            { "地狱猎犬", PM_HELL_HOUND, NEUTRAL },
            { "地狱狗", PM_HELL_HOUND, NEUTRAL },
            { "气体孢子", PM_GAS_SPORE, NEUTRAL },
            { "浮眼", PM_FLOATING_EYE, NEUTRAL },
            { "悬浮眼", PM_FLOATING_EYE, NEUTRAL },
            { "浮空眼", PM_FLOATING_EYE, NEUTRAL },
            { "漂浮眼", PM_FLOATING_EYE, NEUTRAL },
            { "飘浮眼", PM_FLOATING_EYE, NEUTRAL },
            { "悬浮的眼", PM_FLOATING_EYE, NEUTRAL },
            { "悬浮之眼", PM_FLOATING_EYE, NEUTRAL },
            { "悬浮的眼睛", PM_FLOATING_EYE, NEUTRAL },
            { "浮空的眼", PM_FLOATING_EYE, NEUTRAL },
            { "浮空之眼", PM_FLOATING_EYE, NEUTRAL },
            { "浮空的眼睛", PM_FLOATING_EYE, NEUTRAL },
            { "漂浮的眼", PM_FLOATING_EYE, NEUTRAL },
            { "漂浮之眼", PM_FLOATING_EYE, NEUTRAL },
            { "漂浮的眼睛", PM_FLOATING_EYE, NEUTRAL },
            { "飘浮的眼", PM_FLOATING_EYE, NEUTRAL },
            { "飘浮之眼", PM_FLOATING_EYE, NEUTRAL },
            { "飘浮的眼睛", PM_FLOATING_EYE, NEUTRAL },
            { "冻结球", PM_FREEZING_SPHERE, NEUTRAL },
            { "冰球", PM_FREEZING_SPHERE, NEUTRAL },
            { "火焰球", PM_FLAMING_SPHERE, NEUTRAL },
            { "火球", PM_FLAMING_SPHERE, NEUTRAL },
            { "电球", PM_SHOCKING_SPHERE, NEUTRAL },
            { "小猫", PM_KITTEN, NEUTRAL },
            { "家猫", PM_HOUSECAT, NEUTRAL },
            { "美洲豹", PM_JAGUAR, NEUTRAL },
            { "猞猁", PM_LYNX, NEUTRAL },
            { "黑豹", PM_PANTHER, NEUTRAL },
            { "大猫", PM_LARGE_CAT, NEUTRAL },
            { "老虎", PM_TIGER, NEUTRAL },
            { "幻影兽", PM_DISPLACER_BEAST, NEUTRAL },
            { "移位兽", PM_DISPLACER_BEAST, NEUTRAL },
            { "小鬼", PM_GREMLIN, NEUTRAL },
            { "石像鬼", PM_GARGOYLE, NEUTRAL },
            { "飞翼石像鬼", PM_WINGED_GARGOYLE, NEUTRAL },
            { "霍比特人", PM_HOBBIT, NEUTRAL },
            { "矮人", PM_DWARF, NEUTRAL },
            { "熊地精", PM_BUGBEAR, NEUTRAL },
            { "矮人领主", PM_DWARF_LEADER, MALE },
            { "矮人女领主", PM_DWARF_LEADER, FEMALE },
            { "矮人领袖", PM_DWARF_LEADER, NEUTRAL },
            { "矮人王", PM_DWARF_RULER, MALE },
            { "矮人女王", PM_DWARF_RULER, FEMALE },
            { "矮人统治者", PM_DWARF_RULER, NEUTRAL },
            { "夺心魔", PM_MIND_FLAYER, NEUTRAL },
            { "夺心魔大师", PM_MASTER_MIND_FLAYER, NEUTRAL },
            { "主宰夺心魔", PM_MASTER_MIND_FLAYER, NEUTRAL },
            { "高阶夺心魔", PM_MASTER_MIND_FLAYER, NEUTRAL },
            { "灵魂", PM_MANES, NEUTRAL },
            { "幽魂", PM_MANES, NEUTRAL },
            { "雏形人", PM_HOMUNCULUS, NEUTRAL },
            { "人造人", PM_HOMUNCULUS, NEUTRAL },
            { "造妖", PM_HOMUNCULUS, NEUTRAL },
            { "小恶魔", PM_IMP, NEUTRAL },
            { "劣魔", PM_LEMURE, NEUTRAL },
            { "夸塞魔", PM_QUASIT, NEUTRAL },
            { "天狗", PM_TENGU, NEUTRAL },
            { "蓝色果冻", PM_BLUE_JELLY, NEUTRAL },
            { "蓝冻怪", PM_BLUE_JELLY, NEUTRAL },
            { "珍珠果冻", PM_SPOTTED_JELLY, NEUTRAL },
            { "斑点凝胶怪", PM_SPOTTED_JELLY, NEUTRAL },
            { "斑冻怪", PM_SPOTTED_JELLY, NEUTRAL },
            { "赭冻怪", PM_OCHRE_JELLY, NEUTRAL },
            { "赭色凝胶怪", PM_OCHRE_JELLY, NEUTRAL },
            { "狗头人", PM_KOBOLD, NEUTRAL },
            { "大狗头人", PM_LARGE_KOBOLD, NEUTRAL },
            { "狗头人领主", PM_KOBOLD_LEADER, MALE },
            { "狗头人女领主", PM_KOBOLD_LEADER, FEMALE },
            { "狗头人领袖", PM_KOBOLD_LEADER, NEUTRAL },
            { "狗头人萨满", PM_KOBOLD_SHAMAN, NEUTRAL },
            { "小矮妖", PM_LEPRECHAUN, NEUTRAL },
            { "小拟形怪", PM_SMALL_MIMIC, NEUTRAL },
            { "小拟身怪", PM_SMALL_MIMIC, NEUTRAL },
            { "大拟形怪", PM_LARGE_MIMIC, NEUTRAL },
            { "大拟身怪", PM_LARGE_MIMIC, NEUTRAL },
            { "巨型拟形怪", PM_GIANT_MIMIC, NEUTRAL },
            { "巨型拟身怪", PM_GIANT_MIMIC, NEUTRAL },
            { "木仙女", PM_WOOD_NYMPH, NEUTRAL },
            { "水仙女", PM_WATER_NYMPH, NEUTRAL },
            { "山仙女", PM_MOUNTAIN_NYMPH, NEUTRAL },
            { "木仙子", PM_WOOD_NYMPH, NEUTRAL },
            { "水仙子", PM_WATER_NYMPH, NEUTRAL },
            { "山仙子", PM_MOUNTAIN_NYMPH, NEUTRAL },
            { "木妖精", PM_WOOD_NYMPH, NEUTRAL },
            { "水妖精", PM_WATER_NYMPH, NEUTRAL },
            { "山妖精", PM_MOUNTAIN_NYMPH, NEUTRAL },
            { "木宁芙", PM_WOOD_NYMPH, NEUTRAL },
            { "水宁芙", PM_WATER_NYMPH, NEUTRAL },
            { "山宁芙", PM_MOUNTAIN_NYMPH, NEUTRAL },
            { "地精", PM_GOBLIN, NEUTRAL },
            { "哥布林", PM_GOBLIN, NEUTRAL },
            { "大地精", PM_HOBGOBLIN, NEUTRAL },
            { "大哥布林", PM_HOBGOBLIN, NEUTRAL },
            { "兽人", PM_ORC, NEUTRAL },
            { "丘陵兽人", PM_HILL_ORC, NEUTRAL },
            { "魔多兽人", PM_MORDOR_ORC, NEUTRAL },
            { "强兽人", PM_URUK_HAI, NEUTRAL },
            { "乌鲁克", PM_URUK_HAI, NEUTRAL },
            { "兽人萨满", PM_ORC_SHAMAN, NEUTRAL },
            { "兽人队长", PM_ORC_CAPTAIN, NEUTRAL },
            { "岩石锥子", PM_ROCK_PIERCER, NEUTRAL },
            { "岩石锥怪", PM_ROCK_PIERCER, NEUTRAL },
            { "铁锥子", PM_IRON_PIERCER, NEUTRAL },
            { "铁锥怪", PM_IRON_PIERCER, NEUTRAL },
            { "玻璃锥子", PM_GLASS_PIERCER, NEUTRAL },
            { "玻璃锥怪", PM_GLASS_PIERCER, NEUTRAL },
            { "洛斯兽", PM_ROTHE, NEUTRAL },
            { "猛犸", PM_MUMAK, NEUTRAL },
            { "狼狗", PM_LEOCROTTA, NEUTRAL },
            { "狮头象", PM_WUMPUS, NEUTRAL },
            { "雷兽", PM_TITANOTHERE, NEUTRAL },
            { "俾路支兽", PM_BALUCHITHERIUM, NEUTRAL },
            { "巨犀", PM_BALUCHITHERIUM, NEUTRAL },
            { "乳齿象", PM_MASTODON, NEUTRAL },
            { "褐鼠", PM_SEWER_RAT, NEUTRAL },
            { "巨鼠", PM_GIANT_RAT, NEUTRAL },
            { "狂鼠", PM_RABID_RAT, NEUTRAL },
            { "鼠人", PM_WERERAT, NEUTRAL },
            { "岩石鼹鼠", PM_ROCK_MOLE, NEUTRAL },
            { "岩鼹鼠", PM_ROCK_MOLE, NEUTRAL },
            { "土拨鼠", PM_WOODCHUCK, NEUTRAL },
            { "洞穴蜘蛛", PM_CAVE_SPIDER, NEUTRAL },
            { "蜈蚣", PM_CENTIPEDE, NEUTRAL },
            { "巨型蜘蛛", PM_GIANT_SPIDER, NEUTRAL },
            { "巨蜘蛛", PM_GIANT_SPIDER, NEUTRAL },
            { "巨蛛", PM_GIANT_SPIDER, NEUTRAL },
            { "蝎子", PM_SCORPION, NEUTRAL },
            { "潜伏者", PM_LURKER_ABOVE, NEUTRAL },
            { "蛰伏怪", PM_LURKER_ABOVE, NEUTRAL },
            { "捕兽者", PM_TRAPPER, NEUTRAL },
            { "诱陷者", PM_TRAPPER, NEUTRAL },
            { "小马", PM_PONY, NEUTRAL },
            { "白色独角兽", PM_WHITE_UNICORN, NEUTRAL },
            { "白独角兽", PM_WHITE_UNICORN, NEUTRAL },
            { "灰色独角兽", PM_GRAY_UNICORN, NEUTRAL },
            { "灰独角兽", PM_GRAY_UNICORN, NEUTRAL },
            { "黑色独角兽", PM_BLACK_UNICORN, NEUTRAL },
            { "黑独角兽", PM_BLACK_UNICORN, NEUTRAL },
            { "马", PM_HORSE, NEUTRAL },
            { "战马", PM_WARHORSE, NEUTRAL },
            { "雾云", PM_FOG_CLOUD, NEUTRAL },
            { "云雾", PM_FOG_CLOUD, NEUTRAL },
            { "尘埃漩涡", PM_DUST_VORTEX, NEUTRAL },
            { "尘埃旋涡", PM_DUST_VORTEX, NEUTRAL },
            { "冰漩涡", PM_ICE_VORTEX, NEUTRAL },
            { "冰旋涡", PM_ICE_VORTEX, NEUTRAL },
            { "寒冰漩涡", PM_ICE_VORTEX, NEUTRAL },
            { "寒冰旋涡", PM_ICE_VORTEX, NEUTRAL },
            { "能量漩涡", PM_ENERGY_VORTEX, NEUTRAL },
            { "能量旋涡", PM_ENERGY_VORTEX, NEUTRAL },
            { "蒸汽漩涡", PM_STEAM_VORTEX, NEUTRAL },
            { "蒸汽旋涡", PM_STEAM_VORTEX, NEUTRAL },
            { "火焰漩涡", PM_FIRE_VORTEX, NEUTRAL },
            { "火焰旋涡", PM_FIRE_VORTEX, NEUTRAL },
            { "火漩涡", PM_FIRE_VORTEX, NEUTRAL },
            { "火旋涡", PM_FIRE_VORTEX, NEUTRAL },
            { "幼长蠕虫", PM_BABY_LONG_WORM, NEUTRAL },
            { "长蠕虫幼体", PM_BABY_LONG_WORM, NEUTRAL },
            { "幼紫蠕虫", PM_BABY_PURPLE_WORM, NEUTRAL },
            { "紫蠕虫幼体", PM_BABY_PURPLE_WORM, NEUTRAL },
            { "长蠕虫", PM_LONG_WORM, NEUTRAL },
            { "紫蠕虫", PM_PURPLE_WORM, NEUTRAL },
            { "幼长虫", PM_BABY_LONG_WORM, NEUTRAL },
            { "长虫幼体", PM_BABY_LONG_WORM, NEUTRAL },
            { "幼紫虫", PM_BABY_PURPLE_WORM, NEUTRAL },
            { "紫虫幼体", PM_BABY_PURPLE_WORM, NEUTRAL },
            { "长虫", PM_LONG_WORM, NEUTRAL },
            { "紫虫", PM_PURPLE_WORM, NEUTRAL },
            { "电子虫", PM_GRID_BUG, NEUTRAL },
            { "玄蚊", PM_XAN, NEUTRAL },
            { "黄光", PM_YELLOW_LIGHT, NEUTRAL },
            { "黑光", PM_BLACK_LIGHT, NEUTRAL },
            { "山区巨人", PM_ZRUTY, NEUTRAL },
            { "羽蛇", PM_COUATL, NEUTRAL },
            { "亚历克斯", PM_ALEAX, NEUTRAL },
            { "神罚化身", PM_ALEAX, NEUTRAL },
            { "天使", PM_ANGEL, NEUTRAL },
            { "麒麟", PM_KI_RIN, NEUTRAL },
            { "执政官", PM_ARCHON, NEUTRAL },
            { "亚空天族", PM_ARCHON, NEUTRAL },
            { "蝙蝠", PM_BAT, NEUTRAL },
            { "巨型蝙蝠", PM_GIANT_BAT, NEUTRAL },
            { "巨蝙蝠", PM_GIANT_BAT, NEUTRAL },
            { "巨蝠", PM_GIANT_BAT, NEUTRAL },
            { "乌鸦", PM_RAVEN, NEUTRAL },
            { "吸血蝙蝠", PM_VAMPIRE_BAT, NEUTRAL },
            { "平原半人马", PM_PLAINS_CENTAUR, NEUTRAL },
            { "森林半人马", PM_FOREST_CENTAUR, NEUTRAL },
            { "山地半人马", PM_MOUNTAIN_CENTAUR, NEUTRAL },
            { "山半人马", PM_MOUNTAIN_CENTAUR, NEUTRAL },
            { "幼灰龙", PM_BABY_GRAY_DRAGON, NEUTRAL },
            { "幼金龙", PM_BABY_GOLD_DRAGON, NEUTRAL },
            { "幼银龙", PM_BABY_SILVER_DRAGON, NEUTRAL },
            { "幼红龙", PM_BABY_RED_DRAGON, NEUTRAL },
            { "幼白龙", PM_BABY_WHITE_DRAGON, NEUTRAL },
            { "幼橙龙", PM_BABY_ORANGE_DRAGON, NEUTRAL },
            { "幼黑龙", PM_BABY_BLACK_DRAGON, NEUTRAL },
            { "幼蓝龙", PM_BABY_BLUE_DRAGON, NEUTRAL },
            { "幼绿龙", PM_BABY_GREEN_DRAGON, NEUTRAL },
            { "幼黄龙", PM_BABY_YELLOW_DRAGON, NEUTRAL },
            { "小灰龙", PM_BABY_GRAY_DRAGON, NEUTRAL },
            { "小金龙", PM_BABY_GOLD_DRAGON, NEUTRAL },
            { "小银龙", PM_BABY_SILVER_DRAGON, NEUTRAL },
            { "小红龙", PM_BABY_RED_DRAGON, NEUTRAL },
            { "小白龙", PM_BABY_WHITE_DRAGON, NEUTRAL },
            { "小橙龙", PM_BABY_ORANGE_DRAGON, NEUTRAL },
            { "小黑龙", PM_BABY_BLACK_DRAGON, NEUTRAL },
            { "小蓝龙", PM_BABY_BLUE_DRAGON, NEUTRAL },
            { "小绿龙", PM_BABY_GREEN_DRAGON, NEUTRAL },
            { "小黄龙", PM_BABY_YELLOW_DRAGON, NEUTRAL },
            { "灰龙宝宝", PM_BABY_GRAY_DRAGON, NEUTRAL },
            { "金龙宝宝", PM_BABY_GOLD_DRAGON, NEUTRAL },
            { "银龙宝宝", PM_BABY_SILVER_DRAGON, NEUTRAL },
            { "红龙宝宝", PM_BABY_RED_DRAGON, NEUTRAL },
            { "白龙宝宝", PM_BABY_WHITE_DRAGON, NEUTRAL },
            { "橙龙宝宝", PM_BABY_ORANGE_DRAGON, NEUTRAL },
            { "黑龙宝宝", PM_BABY_BLACK_DRAGON, NEUTRAL },
            { "蓝龙宝宝", PM_BABY_BLUE_DRAGON, NEUTRAL },
            { "绿龙宝宝", PM_BABY_GREEN_DRAGON, NEUTRAL },
            { "黄龙宝宝", PM_BABY_YELLOW_DRAGON, NEUTRAL },
            { "灰龙", PM_GRAY_DRAGON, NEUTRAL },
            { "金龙", PM_GOLD_DRAGON, NEUTRAL },
            { "银龙", PM_SILVER_DRAGON, NEUTRAL },
            { "红龙", PM_RED_DRAGON, NEUTRAL },
            { "白龙", PM_WHITE_DRAGON, NEUTRAL },
            { "橙龙", PM_ORANGE_DRAGON, NEUTRAL },
            { "黑龙", PM_BLACK_DRAGON, NEUTRAL },
            { "蓝龙", PM_BLUE_DRAGON, NEUTRAL },
            { "绿龙", PM_GREEN_DRAGON, NEUTRAL },
            { "黄龙", PM_YELLOW_DRAGON, NEUTRAL },
            { "潜行者", PM_STALKER, NEUTRAL },
            { "气元素", PM_AIR_ELEMENTAL, NEUTRAL },
            { "空气元素", PM_AIR_ELEMENTAL, NEUTRAL },
            { "火元素", PM_FIRE_ELEMENTAL, NEUTRAL },
            { "土元素", PM_EARTH_ELEMENTAL, NEUTRAL },
            { "水元素", PM_WATER_ELEMENTAL, NEUTRAL },
            { "地衣", PM_LICHEN, NEUTRAL },
            { "棕霉菌", PM_BROWN_MOLD, NEUTRAL },
            { "黄霉菌", PM_YELLOW_MOLD, NEUTRAL },
            { "绿霉菌", PM_GREEN_MOLD, NEUTRAL },
            { "红霉菌", PM_RED_MOLD, NEUTRAL },
            { "棕色霉菌", PM_BROWN_MOLD, NEUTRAL },
            { "黄色霉菌", PM_YELLOW_MOLD, NEUTRAL },
            { "绿色霉菌", PM_GREEN_MOLD, NEUTRAL },
            { "红色霉菌", PM_RED_MOLD, NEUTRAL },
            { "尖叫蕈", PM_SHRIEKER, NEUTRAL },
            { "紫真菌", PM_VIOLET_FUNGUS, NEUTRAL },
            { "紫色真菌", PM_VIOLET_FUNGUS, NEUTRAL },
            { "侏儒", PM_GNOME, NEUTRAL },
            { "侏儒领主", PM_GNOME_LEADER, MALE },
            { "侏儒女领主", PM_GNOME_LEADER, FEMALE },
            { "侏儒领袖", PM_GNOME_LEADER, NEUTRAL },
            { "侏儒巫师", PM_GNOMISH_WIZARD, NEUTRAL },
            { "侏儒王", PM_GNOME_RULER, MALE },
            { "侏儒女王", PM_GNOME_RULER, FEMALE },
            { "侏儒统治者", PM_GNOME_RULER, NEUTRAL },
            { "巨人", PM_GIANT, NEUTRAL },
            { "石头巨人", PM_STONE_GIANT, NEUTRAL },
            { "石巨人", PM_STONE_GIANT, NEUTRAL },
            { "丘陵巨人", PM_HILL_GIANT, NEUTRAL },
            { "火巨人", PM_FIRE_GIANT, NEUTRAL },
            { "火焰巨人", PM_FIRE_GIANT, NEUTRAL },
            { "冰巨人", PM_FROST_GIANT, NEUTRAL },
            { "雪巨人", PM_FROST_GIANT, NEUTRAL },
            { "霜巨人", PM_FROST_GIANT, NEUTRAL },
            { "冰霜巨人", PM_FROST_GIANT, NEUTRAL },
            { "双头巨人", PM_ETTIN, NEUTRAL },
            { "风巨人", PM_STORM_GIANT, NEUTRAL },
            { "风暴巨人", PM_STORM_GIANT, NEUTRAL },
            { "提坦", PM_TITAN, NEUTRAL },
            { "泰坦", PM_TITAN, NEUTRAL },
            { "弥诺陶洛斯", PM_MINOTAUR, NEUTRAL },
            { "米诺陶洛斯", PM_MINOTAUR, NEUTRAL },
            { "米诺陶", PM_MINOTAUR, NEUTRAL },
            { "牛头人", PM_MINOTAUR, NEUTRAL },
            { "颊脖龙", PM_JABBERWOCK, NEUTRAL },
            { "炸脖龙", PM_JABBERWOCK, NEUTRAL },
            { "贾巴沃克", PM_JABBERWOCK, NEUTRAL },
            { "吉斯通警察", PM_KEYSTONE_KOP, NEUTRAL },
            { "吉斯通警司", PM_KOP_SERGEANT, NEUTRAL },
            { "吉斯通警督", PM_KOP_LIEUTENANT, NEUTRAL },
            { "吉斯通警监", PM_KOP_KAPTAIN, NEUTRAL },
            { "巫妖", PM_LICH, NEUTRAL },
            { "半巫妖", PM_DEMILICH, NEUTRAL },
            { "巫妖大师", PM_MASTER_LICH, NEUTRAL },
            { "主宰巫妖", PM_MASTER_LICH, NEUTRAL },
            { "高阶巫妖", PM_MASTER_LICH, NEUTRAL },
            { "大巫妖", PM_ARCH_LICH, NEUTRAL },
            { "狗头人木乃伊", PM_KOBOLD_MUMMY, NEUTRAL },
            { "侏儒木乃伊", PM_GNOME_MUMMY, NEUTRAL },
            { "兽人木乃伊", PM_ORC_MUMMY, NEUTRAL },
            { "矮人木乃伊", PM_DWARF_MUMMY, NEUTRAL },
            { "精灵木乃伊", PM_ELF_MUMMY, NEUTRAL },
            { "人类木乃伊", PM_HUMAN_MUMMY, NEUTRAL },
            { "双头木乃伊", PM_ETTIN_MUMMY, NEUTRAL },
            { "双头巨人木乃伊", PM_ETTIN_MUMMY, NEUTRAL },
            { "巨人木乃伊", PM_GIANT_MUMMY, NEUTRAL },
            { "红幼纳迦", PM_RED_NAGA_HATCHLING, NEUTRAL },
            { "黑幼纳迦", PM_BLACK_NAGA_HATCHLING, NEUTRAL },
            { "金幼纳迦", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL },
            { "幼纳迦守卫", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL },
            { "幼红纳迦", PM_RED_NAGA_HATCHLING, NEUTRAL },
            { "幼黑纳迦", PM_BLACK_NAGA_HATCHLING, NEUTRAL },
            { "幼金纳迦", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL },
            { "幼纳迦守卫", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL },
            { "小红纳迦", PM_RED_NAGA_HATCHLING, NEUTRAL },
            { "小黑纳迦", PM_BLACK_NAGA_HATCHLING, NEUTRAL },
            { "小金纳迦", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL },
            { "小纳迦守卫", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL },
            { "红纳迦宝宝", PM_RED_NAGA_HATCHLING, NEUTRAL },
            { "黑纳迦宝宝", PM_BLACK_NAGA_HATCHLING, NEUTRAL },
            { "金纳迦宝宝", PM_GOLDEN_NAGA_HATCHLING, NEUTRAL },
            { "纳迦守卫宝宝", PM_GUARDIAN_NAGA_HATCHLING, NEUTRAL },
            { "红纳迦", PM_RED_NAGA, NEUTRAL },
            { "黑纳迦", PM_BLACK_NAGA, NEUTRAL },
            { "金纳迦", PM_GOLDEN_NAGA, NEUTRAL },
            { "纳迦守卫", PM_GUARDIAN_NAGA, NEUTRAL },
            { "食人魔", PM_OGRE, NEUTRAL },
            { "食人魔领主", PM_OGRE_LEADER, MALE },
            { "食人魔女领主", PM_OGRE_LEADER, FEMALE },
            { "食人魔领袖", PM_OGRE_LEADER, NEUTRAL },
            { "食人魔王", PM_OGRE_TYRANT, MALE },
            { "食人魔女王", PM_OGRE_TYRANT, FEMALE },
            { "食人魔暴君", PM_OGRE_TYRANT, NEUTRAL },
            { "食人魔统治者", PM_OGRE_TYRANT, NEUTRAL },
            { "灰色软泥", PM_GRAY_OOZE, NEUTRAL },
            { "灰泥怪", PM_GRAY_OOZE, NEUTRAL },
            { "棕色布丁", PM_BROWN_PUDDING, NEUTRAL },
            { "棕布丁", PM_BROWN_PUDDING, NEUTRAL },
            { "绿色黏液", PM_GREEN_SLIME, NEUTRAL },
            { "绿黏液", PM_GREEN_SLIME, NEUTRAL },
            { "绿色史莱姆", PM_GREEN_SLIME, NEUTRAL },
            { "绿史莱姆", PM_GREEN_SLIME, NEUTRAL },
            { "黑色布丁", PM_BLACK_PUDDING, NEUTRAL },
            { "黑布丁", PM_BLACK_PUDDING, NEUTRAL },
            { "量子力学", PM_QUANTUM_MECHANIC, NEUTRAL },
            { "量子技工", PM_QUANTUM_MECHANIC, NEUTRAL },
            { "量子工程师", PM_QUANTUM_MECHANIC, NEUTRAL },
            { "基因工程师", PM_GENETIC_ENGINEER, NEUTRAL },
            { "锈怪", PM_RUST_MONSTER, NEUTRAL },
            { "锈蚀怪", PM_RUST_MONSTER, NEUTRAL },
            { "解魔怪", PM_DISENCHANTER, NEUTRAL },
            { "祛魔怪", PM_DISENCHANTER, NEUTRAL },
            { "束带蛇", PM_GARTER_SNAKE, NEUTRAL },
            { "蛇", PM_SNAKE, NEUTRAL },
            { "水蝮蛇", PM_WATER_MOCCASIN, NEUTRAL },
            { "巨蟒", PM_PYTHON, NEUTRAL },
            { "响尾蛇", PM_PIT_VIPER, NEUTRAL },
            { "眼镜蛇", PM_COBRA, NEUTRAL },
            { "巨魔", PM_TROLL, NEUTRAL },
            { "冰巨魔", PM_ICE_TROLL, NEUTRAL },
            { "寒冰巨魔", PM_ICE_TROLL, NEUTRAL },
            { "岩石巨魔", PM_ROCK_TROLL, NEUTRAL },
            { "石巨魔", PM_ROCK_TROLL, NEUTRAL },
            { "水巨魔", PM_WATER_TROLL, NEUTRAL },
            { "欧罗海", PM_OLOG_HAI, NEUTRAL },
            { "奥洛格", PM_OLOG_HAI, NEUTRAL },
            { "土巨怪", PM_UMBER_HULK, NEUTRAL },
            { "吸血鬼", PM_VAMPIRE, NEUTRAL },
            { "吸血鬼领主", PM_VAMPIRE_LEADER, MALE },
            { "吸血鬼女领主", PM_VAMPIRE_LEADER, FEMALE },
            { "吸血鬼领袖", PM_VAMPIRE_LEADER, NEUTRAL },
            { "穿刺者弗拉德", PM_VLAD_THE_IMPALER, NEUTRAL },
            { "弗拉德", PM_VLAD_THE_IMPALER, NEUTRAL },
            { "古墓尸妖", PM_BARROW_WIGHT, NEUTRAL },
            { "古冢尸妖", PM_BARROW_WIGHT, NEUTRAL },
            { "尸妖", PM_BARROW_WIGHT, NEUTRAL },
            { "幽灵", PM_WRAITH, NEUTRAL },
            { "戒灵", PM_NAZGUL, NEUTRAL },
            { "索尔石怪", PM_XORN, NEUTRAL },
            { "猴子", PM_MONKEY, NEUTRAL },
            { "猴", PM_MONKEY, NEUTRAL },
            { "猿", PM_APE, NEUTRAL },
            { "枭熊", PM_OWLBEAR, NEUTRAL },
            { "雪人", PM_YETI, NEUTRAL },
            { "食肉猿", PM_CARNIVOROUS_APE, NEUTRAL },
            { "北美野人", PM_SASQUATCH, NEUTRAL },
            { "狗头人僵尸", PM_KOBOLD_ZOMBIE, NEUTRAL },
            { "侏儒僵尸", PM_GNOME_ZOMBIE, NEUTRAL },
            { "兽人僵尸", PM_ORC_ZOMBIE, NEUTRAL },
            { "矮人僵尸", PM_DWARF_ZOMBIE, NEUTRAL },
            { "精灵僵尸", PM_ELF_ZOMBIE, NEUTRAL },
            { "人类僵尸", PM_HUMAN_ZOMBIE, NEUTRAL },
            { "双头僵尸", PM_ETTIN_ZOMBIE, NEUTRAL },
            { "双头巨人僵尸", PM_ETTIN_ZOMBIE, NEUTRAL },
            { "食尸鬼", PM_GHOUL, NEUTRAL },
            { "巨人僵尸", PM_GIANT_ZOMBIE, NEUTRAL },
            { "骷髅", PM_SKELETON, NEUTRAL },
            { "稻草魔像", PM_STRAW_GOLEM, NEUTRAL },
            { "纸魔像", PM_PAPER_GOLEM, NEUTRAL },
            { "绳子魔像", PM_ROPE_GOLEM, NEUTRAL },
            { "金魔像", PM_GOLD_GOLEM, NEUTRAL },
            { "皮革魔像", PM_LEATHER_GOLEM, NEUTRAL },
            { "皮魔像", PM_LEATHER_GOLEM, NEUTRAL },
            { "木魔像", PM_WOOD_GOLEM, NEUTRAL },
            { "肉魔像", PM_FLESH_GOLEM, NEUTRAL },
            { "土魔像", PM_CLAY_GOLEM, NEUTRAL },
            { "石魔像", PM_STONE_GOLEM, NEUTRAL },
            { "玻璃魔像", PM_GLASS_GOLEM, NEUTRAL },
            { "铁魔像", PM_IRON_GOLEM, NEUTRAL },
            { "稻草傀儡", PM_STRAW_GOLEM, NEUTRAL },
            { "纸傀儡", PM_PAPER_GOLEM, NEUTRAL },
            { "绳子傀儡", PM_ROPE_GOLEM, NEUTRAL },
            { "金傀儡", PM_GOLD_GOLEM, NEUTRAL },
            { "皮革傀儡", PM_LEATHER_GOLEM, NEUTRAL },
            { "皮傀儡", PM_LEATHER_GOLEM, NEUTRAL },
            { "木傀儡", PM_WOOD_GOLEM, NEUTRAL },
            { "肉傀儡", PM_FLESH_GOLEM, NEUTRAL },
            { "土傀儡", PM_CLAY_GOLEM, NEUTRAL },
            { "石傀儡", PM_STONE_GOLEM, NEUTRAL },
            { "玻璃傀儡", PM_GLASS_GOLEM, NEUTRAL },
            { "铁傀儡", PM_IRON_GOLEM, NEUTRAL },
            { "人", PM_HUMAN, NEUTRAL },
            { "人类", PM_HUMAN, NEUTRAL },
            { "智人", PM_HUMAN, NEUTRAL },
            { "鼠人", PM_HUMAN_WERERAT, NEUTRAL },
            { "豺狼人", PM_HUMAN_WEREJACKAL, NEUTRAL },
            { "狼人", PM_HUMAN_WEREWOLF, NEUTRAL },
            { "精灵", PM_ELF, NEUTRAL },
            { "伍德兰精灵", PM_WOODLAND_ELF, NEUTRAL },
            { "林地精灵", PM_WOODLAND_ELF, NEUTRAL },
            { "西尔凡精灵", PM_WOODLAND_ELF, NEUTRAL },
            { "绿精灵", PM_GREEN_ELF, NEUTRAL },
            { "绿色精灵", PM_GREEN_ELF, NEUTRAL },
            { "灰精灵", PM_GREY_ELF, NEUTRAL },
            { "灰色精灵", PM_GREY_ELF, NEUTRAL },
            { "精灵领主", PM_ELF_NOBLE, MALE },
            { "精灵女领主", PM_ELF_NOBLE, FEMALE },
            { "精灵贵族", PM_ELF_NOBLE, NEUTRAL },
            { "精灵王", PM_ELVEN_MONARCH, MALE },
            { "精灵女王", PM_ELVEN_MONARCH, FEMALE },
            { "精灵统治者", PM_ELVEN_MONARCH, NEUTRAL },
            { "变形人", PM_DOPPELGANGER, NEUTRAL },
            { "二重身", PM_DOPPELGANGER, NEUTRAL },
            { "店主", PM_SHOPKEEPER, NEUTRAL },
            { "警卫", PM_GUARD, NEUTRAL },
            { "警官", PM_GUARD, NEUTRAL },
            { "囚犯", PM_PRISONER, NEUTRAL },
            { "神谕", PM_ORACLE, NEUTRAL },
            { "神谕者", PM_ORACLE, NEUTRAL },
            { "男牧师", PM_ALIGNED_CLERIC, MALE },
            { "女牧师", PM_ALIGNED_CLERIC, FEMALE },
            { "阵营牧师", PM_ALIGNED_CLERIC, NEUTRAL },
            { "牧师", PM_ALIGNED_CLERIC, NEUTRAL },
            { "男祭司", PM_ALIGNED_CLERIC, MALE },
            { "女祭司", PM_ALIGNED_CLERIC, FEMALE },
            { "阵营祭司", PM_ALIGNED_CLERIC, NEUTRAL },
            { "祭司", PM_ALIGNED_CLERIC, NEUTRAL },
            { "高阶男牧师", PM_HIGH_CLERIC, MALE },
            { "高阶女牧师", PM_HIGH_CLERIC, FEMALE },
            { "高阶牧师", PM_HIGH_CLERIC, NEUTRAL },
            { "高阶男祭司", PM_HIGH_CLERIC, MALE },
            { "高阶女祭司", PM_HIGH_CLERIC, FEMALE },
            { "高阶祭司", PM_HIGH_CLERIC, NEUTRAL },
            { "高级男牧师", PM_HIGH_CLERIC, MALE },
            { "高级女牧师", PM_HIGH_CLERIC, FEMALE },
            { "高级牧师", PM_HIGH_CLERIC, NEUTRAL },
            { "高级男祭司", PM_HIGH_CLERIC, MALE },
            { "高级女祭司", PM_HIGH_CLERIC, FEMALE },
            { "高级祭司", PM_HIGH_CLERIC, NEUTRAL },
            { "士兵", PM_SOLDIER, NEUTRAL },
            { "下士", PM_SOLDIER, NEUTRAL },
            { "中士", PM_SERGEANT, NEUTRAL },
            { "护士", PM_NURSE, NEUTRAL },
            { "中尉", PM_LIEUTENANT, NEUTRAL },
            { "上尉", PM_CAPTAIN, NEUTRAL },
            { "警卫员", PM_WATCHMAN, NEUTRAL },
            { "警卫", PM_WATCHMAN, NEUTRAL },
            { "警卫员队长", PM_WATCH_CAPTAIN, NEUTRAL },
            { "警卫队长", PM_WATCH_CAPTAIN, NEUTRAL },
            { "警卫长", PM_WATCH_CAPTAIN, NEUTRAL },
            { "美杜莎", PM_MEDUSA, NEUTRAL },
            { "岩德巫师", PM_WIZARD_OF_YENDOR, NEUTRAL },
            { "岩德的巫师", PM_WIZARD_OF_YENDOR, NEUTRAL },
            { "克罗伊斯", PM_CROESUS, NEUTRAL },
            { "鬼魂", PM_GHOST, NEUTRAL },
            { "魂灵", PM_SHADE, NEUTRAL },
            { "暗影", PM_SHADE, NEUTRAL },
            { "黑影", PM_SHADE, NEUTRAL },
            { "水妖", PM_WATER_DEMON, NEUTRAL },
            { "梦魇", PM_AMOROUS_DEMON, MALE },
            { "魅魔", PM_AMOROUS_DEMON, FEMALE },
            { "多情的恶魔", PM_AMOROUS_DEMON, NEUTRAL },
            { "多情恶魔", PM_AMOROUS_DEMON, NEUTRAL },
            { "有角的魔鬼", PM_HORNED_DEVIL, NEUTRAL },
            { "有角魔鬼", PM_HORNED_DEVIL, NEUTRAL },
            { "有角的恶魔", PM_HORNED_DEVIL, NEUTRAL },
            { "有角恶魔", PM_HORNED_DEVIL, NEUTRAL },
            { "角魔", PM_HORNED_DEVIL, NEUTRAL },
            { "伊里逆丝", PM_ERINYS, NEUTRAL },
            { "欲魔", PM_ERINYS, NEUTRAL },
            { "罪魔", PM_ERINYS, NEUTRAL },
            { "厄里倪厄斯", PM_ERINYS, NEUTRAL },
            { "哈玛魔", PM_BARBED_DEVIL, NEUTRAL },
            { "猬魔", PM_BARBED_DEVIL, NEUTRAL },
            { "六臂蛇魔", PM_MARILITH, NEUTRAL },
            { "弗洛魔", PM_VROCK, NEUTRAL },
            { "狂战魔", PM_HEZROU, NEUTRAL },
            { "骨魔", PM_BONE_DEVIL, NEUTRAL },
            { "冰魔", PM_ICE_DEVIL, NEUTRAL },
            { "判魂魔", PM_NALFESHNEE, NEUTRAL },
            { "深渊恶魔", PM_PIT_FIEND, NEUTRAL },
            { "桑德斯廷", PM_SANDESTIN, NEUTRAL },
            { "沙魔", PM_SANDESTIN, NEUTRAL },
            { "炎魔", PM_BALROG, NEUTRAL },
            { "朱比烈斯", PM_JUIBLEX, NEUTRAL },
            { "朱庇莱克斯", PM_JUIBLEX, NEUTRAL },
            { "伊诺胡", PM_YEENOGHU, NEUTRAL },
            { "耶诺古", PM_YEENOGHU, NEUTRAL },
            { "奥迦斯", PM_ORCUS, NEUTRAL },
            { "奥喀斯", PM_ORCUS, NEUTRAL },
            { "吉里昂", PM_GERYON, NEUTRAL },
            { "格殷永", PM_GERYON, NEUTRAL },
            { "迪斯帕特", PM_DISPATER, NEUTRAL },
            { "巴力西卜", PM_BAALZEBUB, NEUTRAL },
            { "别西卜", PM_BAALZEBUB, NEUTRAL },
            { "阿斯莫德", PM_ASMODEUS, NEUTRAL },
            { "阿斯蒙蒂斯", PM_ASMODEUS, NEUTRAL },
            { "狄摩高根", PM_DEMOGORGON, NEUTRAL },
            { "死亡", PM_DEATH, NEUTRAL },
            { "瘟疫", PM_PESTILENCE, NEUTRAL },
            { "饥荒", PM_FAMINE, NEUTRAL },
            { "邮件幽灵程序", PM_MAIL_DAEMON, NEUTRAL },
            { "邮件守护灵", PM_MAIL_DAEMON, NEUTRAL },
            { "传信小鬼", PM_MAIL_DAEMON, NEUTRAL },
            { "灯神", PM_DJINNI, NEUTRAL },
            { "水母", PM_JELLYFISH, NEUTRAL },
            { "水虎鱼", PM_PIRANHA, NEUTRAL },
            { "鲨鱼", PM_SHARK, NEUTRAL },
            { "巨型鳗鱼", PM_GIANT_EEL, NEUTRAL },
            { "电鳗", PM_ELECTRIC_EEL, NEUTRAL },
            { "海妖", PM_KRAKEN, NEUTRAL },
            { "蝾螈", PM_NEWT, NEUTRAL },
            { "壁虎", PM_GECKO, NEUTRAL },
            { "鬣蜥", PM_IGUANA, NEUTRAL },
            { "幼鳄鱼", PM_BABY_CROCODILE, NEUTRAL },
            { "小鳄鱼", PM_BABY_CROCODILE, NEUTRAL },
            { "鳄鱼宝宝", PM_BABY_CROCODILE, NEUTRAL },
            { "蜥蜴", PM_LIZARD, NEUTRAL },
            { "变色龙", PM_CHAMELEON, NEUTRAL },
            { "鳄鱼", PM_CROCODILE, NEUTRAL },
            { "火蜥蜴", PM_SALAMANDER, NEUTRAL },
            { "长蠕虫尾", PM_LONG_WORM_TAIL, NEUTRAL },
            { "长蠕虫尾巴", PM_LONG_WORM_TAIL, NEUTRAL },
            { "长蠕虫的尾巴", PM_LONG_WORM_TAIL, NEUTRAL },
            { "长虫尾", PM_LONG_WORM_TAIL, NEUTRAL },
            { "长虫尾巴", PM_LONG_WORM_TAIL, NEUTRAL },
            { "长虫的尾巴", PM_LONG_WORM_TAIL, NEUTRAL },
            { "考古学家", PM_ARCHEOLOGIST, NEUTRAL },
            { "野蛮人", PM_BARBARIAN, NEUTRAL },
            { "男穴居人", PM_CAVE_DWELLER, MALE },
            { "男穴居人", PM_CAVE_DWELLER, FEMALE },
            { "穴居人", PM_CAVE_DWELLER, NEUTRAL },
            { "医生", PM_HEALER, NEUTRAL },
            { "治疗师", PM_HEALER, NEUTRAL },
            { "骑士", PM_KNIGHT, NEUTRAL },
            { "僧侣", PM_MONK, NEUTRAL },
            { "男牧师", PM_CLERIC, MALE },
            { "女牧师", PM_CLERIC, FEMALE },
            { "牧师", PM_CLERIC, NEUTRAL },
            { "游侠", PM_RANGER, NEUTRAL },
            { "盗贼", PM_ROGUE, NEUTRAL },
            { "武士", PM_SAMURAI, NEUTRAL },
            { "游客", PM_TOURIST, NEUTRAL },
            { "女武神", PM_VALKYRIE, NEUTRAL },
            { "巫师", PM_WIZARD, NEUTRAL },
            { "卡那封勋爵", PM_LORD_CARNARVON, NEUTRAL },
            { "珀利阿斯", PM_PELIAS, NEUTRAL },
            { "萨满卡诺夫", PM_SHAMAN_KARNOV, NEUTRAL },
            { "希波克拉底", PM_HIPPOCRATES, NEUTRAL },
            { "亚瑟王", PM_KING_ARTHUR, NEUTRAL },
            { "亚瑟", PM_KING_ARTHUR, NEUTRAL },
            { "宗师", PM_GRAND_MASTER, NEUTRAL },
            { "大祭司", PM_ARCH_PRIEST, NEUTRAL },
            { "俄里翁", PM_ORION, NEUTRAL },
            { "盗贼大师", PM_MASTER_OF_THIEVES, NEUTRAL },
            { "萨托领主", PM_LORD_SATO, NEUTRAL },
            { "双花", PM_TWOFLOWER, NEUTRAL },
            { "诺恩", PM_NORN, NEUTRAL },
            { "诺伦", PM_NORN, NEUTRAL },
            { "绿衣娜菲利特", PM_NEFERET_THE_GREEN, NEUTRAL },
            { "绿肤娜菲利特", PM_NEFERET_THE_GREEN, NEUTRAL },
            { "修堤库特里的奴才", PM_MINION_OF_HUHETOTL, NEUTRAL },
            { "修堤库特里的爪牙", PM_MINION_OF_HUHETOTL, NEUTRAL },
            { "休特奥特尔的奴才", PM_MINION_OF_HUHETOTL, NEUTRAL },
            { "休特奥特尔的爪牙", PM_MINION_OF_HUHETOTL, NEUTRAL },
            { "图特阿蒙", PM_THOTH_AMON, NEUTRAL },
            { "彩色龙", PM_CHROMATIC_DRAGON, NEUTRAL },
            { "独眼巨人", PM_CYCLOPS, NEUTRAL },
            { "恶龙", PM_IXOTH, NEUTRAL },
            { "恶龙埃索斯", PM_IXOTH, NEUTRAL },
            { "凯恩大师", PM_MASTER_KAEN, NEUTRAL },
            { "纳宗魔", PM_NALZOK, NEUTRAL },
            { "蝎弩", PM_SCORPIUS, NEUTRAL },
            { "天蝎", PM_SCORPIUS, NEUTRAL },
            { "刺客大师", PM_MASTER_ASSASSIN, NEUTRAL },
            { "足利尊氏", PM_ASHIKAGA_TAKAUJI, NEUTRAL },
            { "叙尔特领主", PM_LORD_SURTUR, NEUTRAL },
            { "苏尔特尔领主", PM_LORD_SURTUR, NEUTRAL },
            { "苏尔特领主", PM_LORD_SURTUR, NEUTRAL },
            { "黑暗魔君", PM_DARK_ONE, NEUTRAL },
            { "学者", PM_STUDENT, NEUTRAL },
            { "学生", PM_STUDENT, NEUTRAL },
            { "酋长", PM_CHIEFTAIN, NEUTRAL },
            { "尼安德特人", PM_NEANDERTHAL, NEUTRAL },
            { "护理者", PM_ATTENDANT, NEUTRAL },
            { "实习骑士", PM_PAGE, NEUTRAL },
            { "方丈", PM_ABBOT, NEUTRAL },
            { "侍祭", PM_ACOLYTE, NEUTRAL },
            { "猎人", PM_HUNTER, NEUTRAL },
            { "刺客", PM_THUG, NEUTRAL },
            { "忍者", PM_NINJA, NEUTRAL },
            { "禅师", PM_ROSHI, NEUTRAL },
            { "导游", PM_GUIDE, NEUTRAL },
            { "战士", PM_WARRIOR, NEUTRAL },
            { "魔法学徒", PM_APPRENTICE, NEUTRAL },
            /* end of list */
            { 0, NON_PM, NEUTRAL }
        };
        const struct alt_spl *namep;

        for (namep = names; namep->name; namep++) {
            len = (int) strlen(namep->name);
            if (!strncmpi(str, namep->name, len)
                /* force full word (which could conceivably be possessive) */
                && (!str[len] || str[len] == ' ' || str[len] == '\'')) {
                if (remainder_p)
                    *remainder_p = in_str + (&str[len] - buf);
                if (gender_name_var)
                    *gender_name_var = namep->genderhint;
                return namep->pm_val;
            }
        }
    }

    for (len = 0, i = LOW_PM; i < NUMMONS; i++) {
      for (mgend = MALE; mgend < NUM_MGENDERS; mgend++) {
        size_t m_i_len;

        if (!mons[i].epmnames[mgend])
            continue;

        m_i_len = strlen(mons[i].epmnames[mgend]);
        if (m_i_len > (size_t) len
            && !strncmpi(mons[i].epmnames[mgend], str, (int) m_i_len)) {
            if (m_i_len == slen) {
                mntmp = i;
                len = (int) m_i_len;
                matchgend = mgend;
                exact_match = TRUE;
                break; /* exact match */
            } else if (slen > m_i_len
                       && (str[m_i_len] == ' '
                           || !strcmpi(&str[m_i_len], "s")
                           || !strncmpi(&str[m_i_len], "s ", 2)
                           || !strcmpi(&str[m_i_len], "'")
                           || !strncmpi(&str[m_i_len], "' ", 2)
                           || !strcmpi(&str[m_i_len], "'s")
                           || !strncmpi(&str[m_i_len], "'s ", 3)
                           || !strcmpi(&str[m_i_len], "es")
                           || !strncmpi(&str[m_i_len], "es ", 3))) {
                mntmp = i;
                len = (int) m_i_len;
                matchgend = mgend;
            }
        }
      }
      if (exact_match)
        break;
    }
    /* FIXME: some titles have gender; title_to_mon() doesn't propagate it */
    if (mntmp == NON_PM)
        mntmp = title_to_mon(str, (int *) 0, &len);
    if (len && remainder_p)
        *remainder_p = in_str + (&str[len] - buf);
    if (gender_name_var && matchgend != -1) {
        /* don't override with neuter if caller has already specified male
           or female and we've matched the neuter name */
        if (*gender_name_var == -1 || matchgend != NEUTRAL)
            *gender_name_var = matchgend;
    }
    return mntmp;
}

/* monster class from user input; used for genocide and controlled polymorph;
   returns 0 rather than MAXMCLASSES if no match is found */
int
name_to_monclass(const char *in_str, int * mndx_p)
{
    /* Single letters are matched against def_monsyms[].sym; words
       or phrases are first matched against def_monsyms[].explain
       to check class description; if not found there, then against
       mons[].pmnames[] to test individual monster types.  Input can be a
       substring of the full description or pmname, but to be accepted,
       such partial matches must start at beginning of a word.  Some
       class descriptions include "foo or bar" and "foo or other foo"
       so we don't want to accept "or", "other", "or other" there. */
    static NEARDATA const char *const falsematch[] = {
        /* multiple-letter input which matches any of these gets rejected */
        "an", "the", "or", "other", "or other", 0
    };
    /* positive pm_val => specific monster; negative => class */
    static NEARDATA const struct alt_spl truematch[] = {
        /* "long worm" won't match "worm" class but would accidentally match
           "long worm tail" class before the comparison with monster types */
        { "long worm", PM_LONG_WORM, NEUTRAL },
        /* matches wrong--or at least suboptimal--class */
        { "demon", -S_DEMON, NEUTRAL }, /* hits "imp or minor demon" */
        /* matches specific monster (overly restrictive) */
        { "devil", -S_DEMON, NEUTRAL }, /* always "horned devil" */
        /* some plausible guesses which need help */
        { "bug", -S_XAN, NEUTRAL },  /* would match bugbear... */
        { "fish", -S_EEL, NEUTRAL }, /* wouldn't match anything */
        /* end of list */
        { 0, NON_PM, NEUTRAL}
    };
    const char *p, *x;
    int i, len;

    if (mndx_p)
        *mndx_p = NON_PM; /* haven't [yet] matched a specific type */

    if (!in_str || !in_str[0]) {
        /* empty input */
        return 0;
    } else if (!in_str[1]) {
        /* single character */
        i = def_char_to_monclass(*in_str);
        if (i == S_MIMIC_DEF) { /* ']' -> 'm' */
            i = S_MIMIC;
        } else if (i == S_WORM_TAIL) { /* '~' -> 'w' */
            i = S_WORM;
            if (mndx_p)
                *mndx_p = PM_LONG_WORM;
        } else if (i == MAXMCLASSES) /* maybe 'I' */
            i = (*in_str == DEF_INVISIBLE) ? S_invisible : 0;
        return i;
    } else {
        /* multiple characters */
        if (!strcmpi(in_str, "long")) /* not enough to match "long worm" */
            return 0; /* avoid false whole-word match with "long worm tail" */
        in_str = makesingular(in_str);
        /* check for special cases */
        for (i = 0; falsematch[i]; i++)
            if (!strcmpi(in_str, falsematch[i]))
                return 0;
        for (i = 0; truematch[i].name; i++)
            if (!strcmpi(in_str, truematch[i].name)) {
                i = truematch[i].pm_val;
                if (i < 0)
                    return -i; /* class */
                if (mndx_p)
                    *mndx_p = i; /* monster */
                return mons[i].mlet;
            }
        /* check monster class descriptions */
        len = (int) strlen(in_str);
        for (i = 1; i < MAXMCLASSES; i++) {
            x = def_monsyms[i].explain;
            if ((p = strstri(x, in_str)) != 0 && (p == x || *(p - 1) == ' ')
                && ((int) strlen(p) >= len
                    && (p[len] == '\0' || p[len] == ' ')))
                return i;
        }
        /* check individual species names */
        i = name_to_mon(in_str, (int *) 0);
        if (i != NON_PM) {
            if (mndx_p)
                *mndx_p = i;
            return mons[i].mlet;
        }
    }
    return 0;
}

/* returns 3 values (0=male, 1=female, 2=none) */
int
gender(struct monst *mtmp)
{
    if (is_neuter(mtmp->data))
        return 2;
    return mtmp->female;
}

/* Like gender(), but unseen humanoids are "it" rather than "he" or "she"
   and lower animals and such are "it" even when seen; hallucination might
   yield "they".  This is the one we want to use when printing messages. */
int
pronoun_gender(
    struct monst *mtmp,
    unsigned pg_flags) /* flags&1: 'no it' unless neuter,
                        * flags&2: random if hallucinating */
{
    boolean override_vis = (pg_flags & PRONOUN_NO_IT) ? TRUE : FALSE,
            hallu_rand = (pg_flags & PRONOUN_HALLU) ? TRUE : FALSE;

    if (hallu_rand && Hallucination)
        return rn2(4); /* 0..3 */
    if (!override_vis && !canspotmon(mtmp))
        return 2;
    if (is_neuter(mtmp->data))
        return 2;
    return (humanoid(mtmp->data) || (mtmp->data->geno & G_UNIQ)
            || type_is_pname(mtmp->data)) ? (int) mtmp->female : 2;
}

/* used for nearby monsters when you go to another level */
boolean
levl_follower(struct monst *mtmp)
{
    if (mtmp == u.usteed)
        return TRUE;

    /* Wizard with Amulet won't bother trying to follow across levels */
    if (mtmp->iswiz && mon_has_amulet(mtmp))
        return FALSE;
    /* some monsters will follow even while intending to flee from you */
    if (mtmp->mtame || mtmp->iswiz || is_fshk(mtmp))
        return TRUE;
    /* stalking types follow, but won't when fleeing unless you hold
       the Amulet */
    return (boolean) ((mtmp->data->mflags2 & M2_STALK)
                      && (!mtmp->mflee || u.uhave.amulet));
}

static const short grownups[][2] = {
    { PM_CHICKATRICE, PM_COCKATRICE },
    { PM_LITTLE_DOG, PM_DOG },
    { PM_DOG, PM_LARGE_DOG },
    { PM_HELL_HOUND_PUP, PM_HELL_HOUND },
    { PM_WINTER_WOLF_CUB, PM_WINTER_WOLF },
    { PM_KITTEN, PM_HOUSECAT },
    { PM_HOUSECAT, PM_LARGE_CAT },
    { PM_PONY, PM_HORSE },
    { PM_HORSE, PM_WARHORSE },
    { PM_KOBOLD, PM_LARGE_KOBOLD },
    { PM_LARGE_KOBOLD, PM_KOBOLD_LEADER },
    { PM_GNOME, PM_GNOME_LEADER },
    { PM_GNOME_LEADER, PM_GNOME_RULER },
    { PM_DWARF, PM_DWARF_LEADER },
    { PM_DWARF_LEADER, PM_DWARF_RULER },
    { PM_MIND_FLAYER, PM_MASTER_MIND_FLAYER },
    { PM_ORC, PM_ORC_CAPTAIN },
    { PM_HILL_ORC, PM_ORC_CAPTAIN },
    { PM_MORDOR_ORC, PM_ORC_CAPTAIN },
    { PM_URUK_HAI, PM_ORC_CAPTAIN },
    { PM_SEWER_RAT, PM_GIANT_RAT },
    { PM_CAVE_SPIDER, PM_GIANT_SPIDER },
    { PM_OGRE, PM_OGRE_LEADER },
    { PM_OGRE_LEADER, PM_OGRE_TYRANT },
    { PM_ELF, PM_ELF_NOBLE },
    { PM_WOODLAND_ELF, PM_ELF_NOBLE },
    { PM_GREEN_ELF, PM_ELF_NOBLE },
    { PM_GREY_ELF, PM_ELF_NOBLE },
    { PM_ELF_NOBLE, PM_ELVEN_MONARCH },
    { PM_LICH, PM_DEMILICH },
    { PM_DEMILICH, PM_MASTER_LICH },
    { PM_MASTER_LICH, PM_ARCH_LICH },
    { PM_VAMPIRE, PM_VAMPIRE_LEADER },
    { PM_BAT, PM_GIANT_BAT },
    { PM_BABY_GRAY_DRAGON, PM_GRAY_DRAGON },
    { PM_BABY_GOLD_DRAGON, PM_GOLD_DRAGON },
    { PM_BABY_SILVER_DRAGON, PM_SILVER_DRAGON },
#if 0 /* DEFERRED */
    {PM_BABY_SHIMMERING_DRAGON, PM_SHIMMERING_DRAGON},
#endif
    { PM_BABY_RED_DRAGON, PM_RED_DRAGON },
    { PM_BABY_WHITE_DRAGON, PM_WHITE_DRAGON },
    { PM_BABY_ORANGE_DRAGON, PM_ORANGE_DRAGON },
    { PM_BABY_BLACK_DRAGON, PM_BLACK_DRAGON },
    { PM_BABY_BLUE_DRAGON, PM_BLUE_DRAGON },
    { PM_BABY_GREEN_DRAGON, PM_GREEN_DRAGON },
    { PM_BABY_YELLOW_DRAGON, PM_YELLOW_DRAGON },
    { PM_RED_NAGA_HATCHLING, PM_RED_NAGA },
    { PM_BLACK_NAGA_HATCHLING, PM_BLACK_NAGA },
    { PM_GOLDEN_NAGA_HATCHLING, PM_GOLDEN_NAGA },
    { PM_GUARDIAN_NAGA_HATCHLING, PM_GUARDIAN_NAGA },
    { PM_SMALL_MIMIC, PM_LARGE_MIMIC },
    { PM_LARGE_MIMIC, PM_GIANT_MIMIC },
    { PM_BABY_LONG_WORM, PM_LONG_WORM },
    { PM_BABY_PURPLE_WORM, PM_PURPLE_WORM },
    { PM_BABY_CROCODILE, PM_CROCODILE },
    { PM_SOLDIER, PM_SERGEANT },
    { PM_SERGEANT, PM_LIEUTENANT },
    { PM_LIEUTENANT, PM_CAPTAIN },
    { PM_WATCHMAN, PM_WATCH_CAPTAIN },
    { PM_ALIGNED_CLERIC, PM_HIGH_CLERIC },
    { PM_STUDENT, PM_ARCHEOLOGIST },
    { PM_ATTENDANT, PM_HEALER },
    { PM_PAGE, PM_KNIGHT },
    { PM_ACOLYTE, PM_CLERIC },
    { PM_APPRENTICE, PM_WIZARD },
    { PM_MANES, PM_LEMURE },
    { PM_KEYSTONE_KOP, PM_KOP_SERGEANT },
    { PM_KOP_SERGEANT, PM_KOP_LIEUTENANT },
    { PM_KOP_LIEUTENANT, PM_KOP_KAPTAIN },
    { NON_PM, NON_PM }
};

int
little_to_big(int montype)
{
    int i;

    for (i = 0; grownups[i][0] >= LOW_PM; i++)
        if (montype == grownups[i][0]) {
            montype = grownups[i][1];
            break;
        }
    return montype;
}

int
big_to_little(int montype)
{
    int i;

    for (i = 0; grownups[i][0] >= LOW_PM; i++)
        if (montype == grownups[i][1]) {
            montype = grownups[i][0];
            break;
        }
    return montype;
}

/* determine whether two permonst indices are part of the same progression;
   existence of progressions with more than one step makes it a bit tricky */
boolean
big_little_match(int montyp1, int montyp2)
{
    int l, b;

    /* simplest case: both are same pm */
    if (montyp1 == montyp2)
        return TRUE;
    /* assume it isn't possible to grow from one class letter to another */
    if (mons[montyp1].mlet != mons[montyp2].mlet)
        return FALSE;
    /* check whether montyp1 can grow up into montyp2 */
    for (l = montyp1; (b = little_to_big(l)) != l; l = b)
        if (b == montyp2)
            return TRUE;
    /* check whether montyp2 can grow up into montyp1 */
    for (l = montyp2; (b = little_to_big(l)) != l; l = b)
        if (b == montyp1)
            return TRUE;
    /* neither grows up to become the other; no match */
    return FALSE;
}

/*
 * Return the permonst ptr for the race of the monster.
 * Returns correct pointer for non-polymorphed and polymorphed
 * player.  It does not return a pointer to player role character.
 */
const struct permonst *
raceptr(struct monst *mtmp)
{
    if (mtmp == &gy.youmonst && !Upolyd)
        return &mons[gu.urace.mnum];
    return mtmp->data;
}

typedef const char *const locoverbs[4];
static locoverbs levitate = { "漂", "漂", "摇晃", "摇晃" },
                 flys = { "飞", "飞", "扑腾", "扑腾" },
                 flyl = { "飞", "飞", "踉跄", "踉跄" },
                 slither = { "滑", "滑", "蹒跚", "蹒跚" },
                 /* it would be useful to incorporate "swim" but we lack
                  * sufficient information to know whether water is involved
                 swim = { "swim", "Swim", "flop", "Flop" },
                  */
                 ooze = { "渗", "渗", "颤抖", "颤抖" },
                 immobile = { "扭动", "扭动", "震动", "震动" },
                 crawl = { "爬", "爬", "蹒跚", "蹒跚" };

const char *
locomotion(const struct permonst *ptr, const char *def)
{
    int locoindx = 0; /*危险:int locoindx = (*def != highc(*def)) ? 0 : 1;*/

    return (is_floater(ptr) ? levitate[locoindx]
            : (is_flyer(ptr) && ptr->msize <= MZ_SMALL) ? flys[locoindx]
              : (is_flyer(ptr) && ptr->msize > MZ_SMALL) ? flyl[locoindx]
                : slithy(ptr) ? slither[locoindx]
                  : amorphous(ptr) ? ooze[locoindx]
                    : !ptr->mmove ? immobile[locoindx]
                      : nolimbs(ptr) ? crawl[locoindx]
                        : def);
}

const char *
stagger(const struct permonst *ptr, const char *def)
{
    int locoindx = 2;

    return (is_floater(ptr) ? levitate[locoindx]
            : (is_flyer(ptr) && ptr->msize <= MZ_SMALL) ? flys[locoindx]
              : (is_flyer(ptr) && ptr->msize > MZ_SMALL) ? flyl[locoindx]
                : slithy(ptr) ? slither[locoindx]
                  : amorphous(ptr) ? ooze[locoindx]
                    : !ptr->mmove ? immobile[locoindx]
                      : nolimbs(ptr) ? crawl[locoindx]
                        : def);
}

/* return phrase describing the effect of fire attack on a type of monster */
const char *
on_fire(struct permonst *mptr, struct attack *mattk)
{
    const char *what;

    switch (monsndx(mptr)) {
    case PM_FLAMING_SPHERE:
    case PM_FIRE_VORTEX:
    case PM_FIRE_ELEMENTAL:
    case PM_SALAMANDER:
        what = "本来就是着火的";
        break;
    case PM_WATER_ELEMENTAL:
    case PM_FOG_CLOUD:
    case PM_STEAM_VORTEX:
        what = "在沸腾";
        break;
    case PM_ICE_VORTEX:
    case PM_GLASS_GOLEM:
        what = "在熔化";
        break;
    case PM_STONE_GOLEM:
    case PM_CLAY_GOLEM:
    case PM_GOLD_GOLEM:
    case PM_AIR_ELEMENTAL:
    case PM_EARTH_ELEMENTAL:
    case PM_DUST_VORTEX:
    case PM_ENERGY_VORTEX:
        what = "在升温";
        break;
    default:
        what = (mattk->aatyp == AT_HUGS) ? "正在被烤" : "着火了";
        break;
    }
    return what;
}

/* similar to on_fire(); creature is summoned in a cloud of <something> */
const char *
msummon_environ(struct permonst *mptr, const char **cloud)
{
    const char *what;
    int mndx = ((mptr->mlet == S_ANGEL) ? PM_ANGEL
                : (mptr->mlet == S_LIGHT) ? PM_YELLOW_LIGHT
                  : monsndx(mptr));

    *cloud = "云"; /* default is "cloud of <something>" */
    switch (mndx) {
    case PM_WATER_DEMON:
    case PM_AIR_ELEMENTAL:
    case PM_WATER_ELEMENTAL:
    case PM_FOG_CLOUD:
    case PM_ICE_VORTEX:
    case PM_FREEZING_SPHERE:
        what = "蒸汽";
        break;
    case PM_STEAM_VORTEX:
        what = "水汽";
        break;
    case PM_ENERGY_VORTEX:
    case PM_SHOCKING_SPHERE:
        *cloud = "四溅的"; /* "shower of sparks" instead of "cloud of..." */
        what = "火花";
        break;
    case PM_EARTH_ELEMENTAL:
    case PM_DUST_VORTEX:
        what = "尘土";
        break;
    case PM_FIRE_ELEMENTAL:
    case PM_FIRE_VORTEX:
    case PM_FLAMING_SPHERE:
    /*case PM_SALAMANDER:*/
        *cloud = "火焰"; /* "ball of flame" instead of "cloud of..." */
        what = "球";
        break;
    case PM_ANGEL: /* actually any 'A'-class */
    case PM_YELLOW_LIGHT: /* any 'y'-class */
        *cloud = "一道"; /* "flash of light" instead of "cloud of..." */
        what = "闪光";
        break;
    default:
        what = "烟雾";
        break;
    }
    return what;
}

/*
 * Returns:
 *      True if monster is presumed to have a sense of smell.
 *      False if monster definitely does not have a sense of smell.
 *
 * Do not base this on presence of a head or nose, since many
 * creatures sense smells other ways (feelers, forked-tongues, etc).
 * We're assuming all insects can smell at a distance too.
 */
boolean
olfaction(struct permonst *mdat)
{
    if (is_golem(mdat)
        || mdat->mlet == S_EYE /* spheres  */
        || mdat->mlet == S_JELLY || mdat->mlet == S_PUDDING
        || mdat->mlet == S_BLOB || mdat->mlet == S_VORTEX
        || mdat->mlet == S_ELEMENTAL
        || mdat->mlet == S_FUNGUS /* mushrooms and fungi */
        || mdat->mlet == S_LIGHT)
        return FALSE;
    return TRUE;
}

/* Convert attack damage type AD_foo to M_SEEN_bar */
unsigned long
cvt_adtyp_to_mseenres(uchar adtyp)
{
    switch (adtyp) {
    case AD_MAGM: return M_SEEN_MAGR;
    case AD_FIRE: return M_SEEN_FIRE;
    case AD_COLD: return M_SEEN_COLD;
    case AD_SLEE: return M_SEEN_SLEEP;
    case AD_DISN: return M_SEEN_DISINT;
    case AD_ELEC: return M_SEEN_ELEC;
    case AD_DRST: return M_SEEN_POISON;
    case AD_ACID: return M_SEEN_ACID;
    /* M_SEEN_REFL has no corresponding AD_foo type */
    default: return M_SEEN_NOTHING;
    }
}

/* Convert property resistance to M_SEEN_bar */
unsigned long
cvt_prop_to_mseenres(uchar prop)
{
    switch (prop) {
    case ANTIMAGIC: return M_SEEN_MAGR;
    case FIRE_RES: return M_SEEN_FIRE;
    case COLD_RES: return M_SEEN_COLD;
    case SLEEP_RES: return M_SEEN_SLEEP;
    case DISINT_RES: return M_SEEN_DISINT;
    case POISON_RES: return M_SEEN_POISON;
    case SHOCK_RES: return M_SEEN_ELEC;
    case ACID_RES: return M_SEEN_ACID;
    case REFLECTING: return M_SEEN_REFL;
    default: return M_SEEN_NOTHING;
    }
}

/* Monsters in line of sight remember hero resisting effect M_SEEN_foo */
void
monstseesu(unsigned long seenres)
{
    struct monst *mtmp;

    if (seenres == M_SEEN_NOTHING || u.uswallow)
        return;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (!DEADMONSTER(mtmp) && m_canseeu(mtmp))
            m_setseenres(mtmp, seenres);
}

/* Monsters in line of sight forget hero resistance to M_SEEN_foo */
void
monstunseesu(unsigned long seenres)
{
    struct monst *mtmp;

    if (seenres == M_SEEN_NOTHING || u.uswallow)
        return;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon)
        if (!DEADMONSTER(mtmp) && m_canseeu(mtmp))
            m_clearseenres(mtmp, seenres);
}

/* give monster mtmp the same intrinsics hero has */
void
give_u_to_m_resistances(struct monst *mtmp)
{
    int intr;

    /* convert the hero's current set of intrinsics to their monster
       equivalents -- FIRE_RES to MR_FIRE, COLD_RES to MR_COLD, etc -- and
       add each to the mintrinsics field for the given monster */
    for (intr = FIRE_RES; intr <= STONE_RES; intr++) {
        if ((u.uprops[intr].intrinsic & INTRINSIC) != 0L) {
            mtmp->mintrinsics |= (unsigned short) res_to_mr(intr);
        }
    }
}

/* Can monster resist conflict caused by hero?

   High-CHA heroes will be able to 'convince' monsters
   (through the magic of the ring, of course) to fight
   for them much more easily than low-CHA ones.
*/
boolean
resist_conflict(struct monst *mtmp)
{
    /* always a small chance at 19 */
    int resist_chance = min(19, (ACURR(A_CHA) - mtmp->m_lev + u.ulevel));

    return (rnd(20) > resist_chance);
}

/* does monster mtmp know traps of type ttyp */
boolean
mon_knows_traps(struct monst *mtmp, int ttyp)
{
    if (ttyp == ALL_TRAPS)
        return (boolean)(mtmp->mtrapseen);
    else if (ttyp == NO_TRAP)
        return !(boolean)(mtmp->mtrapseen);
    else
        return ((mtmp->mtrapseen & (1L << (ttyp - 1))) != 0);
}

/* monster mtmp learns all traps of type ttyp */
void
mon_learns_traps(struct monst *mtmp, int ttyp)
{
    if (ttyp == ALL_TRAPS)
        mtmp->mtrapseen = ~0L;
    else if (ttyp == NO_TRAP)
        mtmp->mtrapseen = 0L;
    else
        mtmp->mtrapseen |= (1L << (ttyp - 1));
}

/* monsters see a trap trigger, and remember it */
void
mons_see_trap(struct trap *ttmp)
{
    struct monst *mtmp;
    coordxy tx = ttmp->tx, ty = ttmp->ty;
    int maxdist = levl[tx][ty].lit ? 7*7 : 2;

    for (mtmp = fmon; mtmp; mtmp = mtmp->nmon) {
        if (is_animal(mtmp->data) || mindless(mtmp->data)
            || !haseyes(mtmp->data) || !mtmp->mcansee)
            continue;
        if (dist2(mtmp->mx, mtmp->my, tx, ty) > maxdist)
            continue;
        if (!m_cansee(mtmp, tx, ty))
            continue;
        mon_learns_traps(mtmp, ttmp->ttyp);
    }
}

int
get_atkdam_type(int adtyp)
{
    if (adtyp == AD_RBRE) {
        static const int rnd_breath_typ[] = {
            AD_MAGM, AD_FIRE, AD_COLD, AD_SLEE,
            AD_DISN, AD_ELEC, AD_DRST, AD_ACID };
        return ROLL_FROM(rnd_breath_typ);
    }
    return adtyp;
}

/*mondata.c*/
