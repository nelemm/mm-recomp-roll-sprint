// Roll Sprint : après une roulade, garder A enfoncé fait courir Link à la vitesse de la roulade.
//
// Fonctionnement :
// - La roulade (Player_Action_26) vise "vitesse de course x 1.5".
// - Pendant le sprint, on applique le même multiplicateur à la vitesse visée calculée à partir
//   du stick, uniquement dans l'action de course normale (Player_Action_13).
// - Le sprint s'arrête quand A est relâché ou que Link s'arrête.
//
// Le mod n'utilise que des hooks (aucun RECOMP_PATCH) pour rester compatible avec
// d'autres mods qui modifient la roulade ou la course (Roll Master, Enhanced Roll...).

#include "modding.h"
#include "global.h"
#include "recomputils.h"
#include "recompconfig.h"

// Fonctions du jeu (overlay du joueur), résolues par les symboles du recomp.
void Player_Action_26(Player* this, PlayState* play); // roulade
void Player_Action_13(Player* this, PlayState* play); // course normale

typedef enum {
    SPRINT_IDLE,    // rien
    SPRINT_ARMED,   // roulade en cours avec A enfoncé
    SPRINT_ACTIVE,  // roulade terminée, A toujours enfoncé : on sprinte
} SprintState;

#define STICK_NEUTRAL_RADIUS 15 // sur une échelle d'environ +/-80
#define STICK_NEUTRAL_FRAMES 8  // ~0,15 s à 60 fps logiques / 0,4 s à 20 fps

static SprintState sSprintState = SPRINT_IDLE;
static s32 sNeutralFrames = 0;
static Player* sCalcPlayer = NULL;
static f32* sCalcOutSpeed = NULL;

static bool sprint_enabled(void) {
    return recomp_get_config_u32("enabled") == 0; // 0 = "Active"
}

#define SPRINT_SPEED_MIN 1.0f
#define SPRINT_SPEED_MAX 2.5f

// Valeur bornée : le menu limite déjà la plage, mais le fichier de config peut être édité à la main.
static f32 sprint_multiplier(void) {
    f32 mult = (f32)recomp_get_config_double("speed");
    if (!(mult >= SPRINT_SPEED_MIN)) { // couvre aussi NaN
        return SPRINT_SPEED_MIN;
    }
    if (mult > SPRINT_SPEED_MAX) {
        return SPRINT_SPEED_MAX;
    }
    return mult;
}

// Réglage "Virages" (index des options dans mod.toml).
typedef enum {
    TURNING_CLASSIC,      // comportement du jeu (rotation progressive)
    TURNING_FAST_SPRINT,  // virages rapides seulement pendant le sprint
    TURNING_FAST_ALWAYS,  // virages rapides en course et en sprint
    TURNING_INSTANT,      // direction = stick immédiatement, en course et en sprint
} TurningMode;

// Pas de rotation des virages rapides : 0x2000 x 1,5 (20 fps logiques) = ~67° par frame,
// soit un demi-tour en ~3 frames (0,15 s).
#define FAST_TURN_STEP 0x2000

static TurningMode turning_mode(void) {
    return (TurningMode)recomp_get_config_u32("turning");
}


static bool fast_turn_active(void) {
    if (!sprint_enabled()) {
        return false;
    }
    switch (turning_mode()) {
        case TURNING_FAST_SPRINT:
            return sSprintState == SPRINT_ACTIVE;
        case TURNING_FAST_ALWAYS:
        case TURNING_INSTANT:
            return true;
        default:
            return false;
    }
}

// Mise à jour de l'état du sprint, une fois par frame, avant la logique du joueur.
RECOMP_HOOK("Player_Update")
void RollSprint_OnPlayerUpdate(Actor* thisx, PlayState* play) {
    Player* this = (Player*)thisx;

    if (!sprint_enabled() || this != GET_PLAYER(play)) {
        sSprintState = SPRINT_IDLE;
        return;
    }

    Input* input = CONTROLLER1(&play->state);
    bool aHeld = CHECK_BTN_ALL(input->cur.button, BTN_A);
    bool rolling = (this->actionFunc == Player_Action_26);
    s32 stickX = input->cur.stick_x;
    s32 stickY = input->cur.stick_y;
    bool stickNeutral = (stickX * stickX + stickY * stickY) < (STICK_NEUTRAL_RADIUS * STICK_NEUTRAL_RADIUS);

    if (!aHeld) {
        sSprintState = SPRINT_IDLE;
    } else if (rolling) {
        if (sSprintState == SPRINT_IDLE) {
            sSprintState = SPRINT_ARMED;
        }
    } else if (sSprintState == SPRINT_ARMED) {
        // Fin de la roulade avec A toujours enfoncé.
        // Note : le jeu repasse par l'action "immobile" puis relance la course depuis une
        // vitesse quasi nulle ; on ne se base donc pas sur la vitesse pour arrêter le sprint.
        sSprintState = SPRINT_ACTIVE;
        sNeutralFrames = 0;
    } else if (sSprintState == SPRINT_ACTIVE) {
        // Arrêt quand le stick reste au neutre quelques frames (Link s'arrête).
        sNeutralFrames = stickNeutral ? sNeutralFrames + 1 : 0;
        if (sNeutralFrames > STICK_NEUTRAL_FRAMES) {
            sSprintState = SPRINT_IDLE;
        }
    }
}

// Pendant le sprint, la course normale garde ses réglages de rotation, de freinage et
// d'accélération pensés pour la vitesse de base. Résultat : Link tourne trop lentement pour
// sa vitesse, le stick passe à plus de ~135° de sa direction, et le jeu le fait freiner
// jusqu'à l'arrêt (func_8083A4A4), ce qui dure d'autant plus longtemps qu'il va vite.
// On met donc ces registres à l'échelle de la vitesse du sprint, uniquement le temps
// de l'action de course, puis on restaure les valeurs d'origine.
// Registres mis à l'échelle (x multiplicateur) pendant le sprint :
//   REG(19) accélération de la course
//   REG(27) vitesse de rotation pendant la course
//   REG(43) R_DECELERATE_RATE, freinage avant un demi-tour
// Animation (func_8083F57C / func_8083EE60) : la vitesse d'animation vaut
//   marche : REG(35)/1000 + REG(36)/1000 * vitesse
//   course : REG(39)/100  + REG(38)/1000 * (vitesse - REG(48)/100)
// En multipliant REG(35), REG(39) et REG(48) par le facteur (et pas les pentes REG(36)/REG(38),
// déjà multipliées par la vitesse), on obtient exactement "animation normale x facteur" :
// les pas de Link suivent sa vitesse réelle au lieu de glisser.
// Ces registres ne servent qu'à ces calculs (les autres "48" sont MREG/IREG, d'autres groupes).
static const u8 sScaledRegs[] = { 19, 27, 43, 35, 39, 48 };
#define SCALED_REG_COUNT (sizeof(sScaledRegs) / sizeof(sScaledRegs[0]))

static bool sRegsScaled = false;
static s16 sSavedRegs[SCALED_REG_COUNT];

static s16 scale_reg(s16 value, f32 mult) {
    f32 scaled = value * mult;
    if (scaled > 0x7FFF) {
        scaled = 0x7FFF;
    } else if (scaled < -0x8000) {
        scaled = -0x8000;
    }
    return (s16)scaled;
}

RECOMP_HOOK("Player_Action_13")
void RollSprint_OnRunEnter(Player* this, PlayState* play) {
    if (sSprintState != SPRINT_ACTIVE || sRegsScaled) {
        return;
    }
    f32 mult = sprint_multiplier();
    for (u32 i = 0; i < SCALED_REG_COUNT; i++) {
        sSavedRegs[i] = REG(sScaledRegs[i]);
        REG(sScaledRegs[i]) = scale_reg(sSavedRegs[i], mult);
    }
    sRegsScaled = true;
}

RECOMP_HOOK_RETURN("Player_Action_13")
void RollSprint_OnRunReturn(void) {
    if (sRegsScaled) {
        for (u32 i = 0; i < SCALED_REG_COUNT; i++) {
            REG(sScaledRegs[i]) = sSavedRegs[i];
        }
        sRegsScaled = false;
    }
}

// Orientation du corps : Player_UpdateShapeYaw fait tourner le modèle (shape.rot.y) vers la
// direction de déplacement (yaw) à une vitesse fixe de 0x7D0. Comme le sprint accélère la
// rotation de la direction (REG(27)), le corps prenait du retard et pouvait regarder à
// l'opposé de la course en faisant des cercles. On ajoute donc le complément de rotation
// pour que le corps tourne au même rythme (0x7D0 x multiplicateur).
#define SHAPE_YAW_STEP 0x7D0

static Player* sShapePlayer = NULL;

RECOMP_HOOK("Player_UpdateShapeYaw")
void RollSprint_OnShapeYaw(Player* this, PlayState* play) {
    sShapePlayer = this;
}

RECOMP_HOOK_RETURN("Player_UpdateShapeYaw")
void RollSprint_OnShapeYawReturn(void) {
    Player* this = sShapePlayer;
    sShapePlayer = NULL;

    if (this == NULL || this->actionFunc != Player_Action_13) {
        return;
    }
    // Même condition que la branche "rotation vers la direction de déplacement" du jeu.
    if (!(this->stateFlags2 & PLAYER_STATE2_20) || (this->stateFlags2 & PLAYER_STATE2_40)) {
        return;
    }

    // Virages rapides : le corps suit la direction au même rythme qu'elle.
    if (fast_turn_active()) {
        s16 before = this->actor.shape.rot.y;
        if (turning_mode() == TURNING_INSTANT) {
            this->actor.shape.rot.y = this->yaw;
        } else {
            Math_ScaledStepToS(&this->actor.shape.rot.y, this->yaw, FAST_TURN_STEP);
        }
        this->unk_B4C += (s16)(this->actor.shape.rot.y - before);
        return;
    }

    if (sSprintState != SPRINT_ACTIVE) {
        return;
    }

    f32 extra = SHAPE_YAW_STEP * (sprint_multiplier() - 1.0f);
    if (extra < 1.0f) {
        return;
    }
    s16 before = this->actor.shape.rot.y;
    Math_ScaledStepToS(&this->actor.shape.rot.y, this->yaw, scale_reg(SHAPE_YAW_STEP, sprint_multiplier()) - SHAPE_YAW_STEP);
    this->unk_B4C += (s16)(this->actor.shape.rot.y - before);
}

// Virages rapides : Player_GetMovementSpeedAndYaw donne la direction visée finale (caméra
// comprise). Juste après, la course (Player_Action_13) compare cette cible à la direction
// actuelle : au-delà de ~135° elle force un freinage jusqu'à l'arrêt (func_8083A4A4), sinon
// elle tourne progressivement. En rapprochant ici la direction actuelle de la cible, Link
// pivote presque sur place sans ce freinage, comme dans les jeux récents.
static Player* sMovePlayer = NULL;
static f32* sMoveOutSpeed = NULL;
static s16* sMoveOutYaw = NULL;

RECOMP_HOOK("Player_GetMovementSpeedAndYaw")
void RollSprint_OnGetMovement(Player* this, f32* outSpeedTarget, s16* outYawTarget, f32 speedMode, PlayState* play) {
    sMovePlayer = this;
    sMoveOutSpeed = outSpeedTarget;
    sMoveOutYaw = outYawTarget;
}

RECOMP_HOOK_RETURN("Player_GetMovementSpeedAndYaw")
void RollSprint_OnGetMovementReturn(void) {
    Player* this = sMovePlayer;
    f32* outSpeed = sMoveOutSpeed;
    s16* outYaw = sMoveOutYaw;
    sMovePlayer = NULL;
    sMoveOutSpeed = NULL;
    sMoveOutYaw = NULL;

    if (this == NULL || outSpeed == NULL || outYaw == NULL || !sprint_enabled()) {
        return;
    }
    // Seulement pendant la course normale, et seulement quand le stick est poussé.
    if (this->actionFunc != Player_Action_13 || *outSpeed <= 0.0f || !fast_turn_active()) {
        return;
    }

    if (turning_mode() == TURNING_INSTANT) {
        this->yaw = *outYaw;
    } else {
        Math_ScaledStepToS(&this->yaw, *outYaw, FAST_TURN_STEP);
    }
}

RECOMP_HOOK("Player_CalcSpeedAndYawFromControlStick")
void RollSprint_OnCalcSpeed(PlayState* play, Player* this, f32* outSpeedTarget, s16* outYawTarget, f32 speedMode) {
    sCalcPlayer = this;
    sCalcOutSpeed = outSpeedTarget;
}

RECOMP_HOOK_RETURN("Player_CalcSpeedAndYawFromControlStick")
void RollSprint_OnCalcSpeedReturn(void) {
    // Uniquement pendant la course normale (Player_Action_13), qui quitte d'elle-même
    // la course dès qu'on vise avec Z.
    if (sSprintState == SPRINT_ACTIVE && sCalcOutSpeed != NULL && sCalcPlayer != NULL &&
        sCalcPlayer->actionFunc == Player_Action_13) {
        // Même calcul que la roulade : vitesse visée (déjà plafonnée) x multiplicateur.
        *sCalcOutSpeed *= sprint_multiplier();
    }
    sCalcPlayer = NULL;
    sCalcOutSpeed = NULL;
}
